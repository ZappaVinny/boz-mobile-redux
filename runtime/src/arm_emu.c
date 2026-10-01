#include "arm_emu.h"

#include <dlfcn.h>
#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unicorn/unicorn.h>

enum {
    PAGE_SIZE_EMU = 0x1000,
    STACK_SIZE = 2 * 1024 * 1024,
    HOST_ARG_WORDS = 16,
    SVC_ARM_WORD = 0xef00df00u,
};

struct arm_emu {
    uc_engine *uc;
    uint8_t *stack;
    uint32_t sentinel;
    int depth;
};

typedef uint64_t (*host_fn16)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                              uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t,
                              uint32_t, uint32_t, uint32_t, uint32_t);

static uint32_t *g_svc_page;
static __thread struct arm_emu *t_emu;
static long g_trace_limit;
static _Atomic unsigned long g_host_calls;
static _Atomic unsigned long g_traced;
static _Atomic uint32_t g_last_target;
static uc_engine *_Atomic g_main_uc;

static void *status_thread(void *arg) {
    (void)arg;
    unsigned long previous = 0;
    for (;;) {
        struct timespec ts = {2, 0};
        nanosleep(&ts, NULL);
        unsigned long now = g_host_calls;
        Dl_info info;
        uint32_t last = g_last_target;
        const char *name = dladdr((void *)(uintptr_t)last, &info) && info.dli_sname ? info.dli_sname
                                                                                  : "?";
        uint32_t pc = 0, lr = 0;
        uc_engine *uc = g_main_uc;
        if (uc) {
            uc_reg_read(uc, UC_ARM_REG_PC, &pc);
            uc_reg_read(uc, UC_ARM_REG_LR, &lr);
        }
        fprintf(stderr, "[status] host calls %lu (+%lu/2s) last=%s main pc=%08x lr=%08x\n", now,
                now - previous, name, pc, lr);
        previous = now;
    }
    return NULL;
}

static void trace_call(uint32_t target, const uint32_t *a, uint32_t lr) {
    unsigned long n = ++g_host_calls;
    g_last_target = target;
    if (g_trace_limit <= 0 || (long)++g_traced > g_trace_limit) {
        return;
    }
    Dl_info info;
    const char *name = dladdr((void *)(uintptr_t)target, &info) && info.dli_sname ? info.dli_sname
                                                                                    : "?";
    fprintf(stderr, "[call %lu] %s(%08x, %08x, %08x, %08x) from %08x\n", n, name, a[0], a[1], a[2],
            a[3], lr);
}

static bool find_host_vma(uint32_t address, uint64_t *start, uint64_t *end) {
    FILE *maps = fopen("/proc/self/maps", "r");
    if (!maps) {
        return false;
    }
    char line[512];
    bool found = false;
    while (fgets(line, sizeof(line), maps)) {
        unsigned long long lo, hi;
        if (sscanf(line, "%llx-%llx", &lo, &hi) == 2 && address >= lo && address < hi) {
            *start = lo;
            *end = hi;
            found = true;
            break;
        }
    }
    fclose(maps);
    return found;
}

static void clip_to_unmapped(uc_engine *uc, uint32_t address, uint64_t *lo, uint64_t *hi) {
    uc_mem_region *regions = NULL;
    uint32_t count = 0;
    if (uc_mem_regions(uc, &regions, &count) != UC_ERR_OK) {
        return;
    }
    for (uint32_t i = 0; i < count; ++i) {
        if (regions[i].end < address && regions[i].end + 1 > *lo) {
            *lo = regions[i].end + 1;
        }
        if (regions[i].begin > address && regions[i].begin < *hi) {
            *hi = regions[i].begin;
        }
    }
    uc_free(regions);
}

static void *svc_shadow(size_t size) {
    uint32_t *shadow = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (shadow == MAP_FAILED) {
        return NULL;
    }
    for (size_t i = 0; i < size / sizeof(uint32_t); ++i) {
        shadow[i] = SVC_ARM_WORD;
    }
    return shadow;
}

static bool host_code_address(uint32_t address) {
    Dl_info info;
    return dladdr((void *)(uintptr_t)address, &info) != 0 && info.dli_fbase != NULL;
}

static void report_fault(uc_engine *uc, const char *what, uint64_t address) {
    uint32_t r[16];
    static const int ids[16] = {
        UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2,  UC_ARM_REG_R3,  UC_ARM_REG_R4,  UC_ARM_REG_R5,
        UC_ARM_REG_R6, UC_ARM_REG_R7, UC_ARM_REG_R8,  UC_ARM_REG_R9,  UC_ARM_REG_R10, UC_ARM_REG_R11,
        UC_ARM_REG_R12, UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_PC,
    };
    for (int i = 0; i < 16; ++i) {
        uc_reg_read(uc, ids[i], &r[i]);
    }
    fprintf(stderr, "[arm] %s at 0x%08" PRIx64 " pc=%08x lr=%08x sp=%08x\n", what, address, r[15],
            r[14], r[13]);
    for (int i = 0; i < 13; i += 4) {
        fprintf(stderr, "[arm]   r%-2d=%08x r%-2d=%08x r%-2d=%08x r%-2d=%08x\n", i, r[i], i + 1,
                r[i + 1], i + 2, r[i + 2], i + 3, r[i + 3]);
    }
}

static bool on_unmapped(uc_engine *uc, uc_mem_type type, uint64_t address, int size, int64_t value,
                        void *user_data) {
    (void)size;
    (void)value;
    (void)user_data;
    uint64_t lo, hi;
    if ((uint32_t)address >= PAGE_SIZE_EMU && find_host_vma((uint32_t)address, &lo, &hi)) {
        clip_to_unmapped(uc, (uint32_t)address, &lo, &hi);
        size_t length = (size_t)(hi - lo);
        if (type == UC_MEM_FETCH_UNMAPPED && host_code_address((uint32_t)address)) {
            void *shadow = svc_shadow(length);
            if (shadow && uc_mem_map_ptr(uc, lo, length, UC_PROT_READ | UC_PROT_EXEC, shadow) ==
                              UC_ERR_OK) {
                return true;
            }
        } else if (uc_mem_map_ptr(uc, lo, length, UC_PROT_ALL, (void *)(uintptr_t)lo) == UC_ERR_OK) {
            return true;
        }
    }
    report_fault(uc, type == UC_MEM_WRITE_UNMAPPED ? "write to unmapped" : type == UC_MEM_FETCH_UNMAPPED ? "fetch from unmapped" : "read from unmapped",
                 address);
    return false;
}

static void on_interrupt(uc_engine *uc, uint32_t intno, void *user_data) {
    (void)user_data;
    uint32_t pc, cpsr;
    uc_reg_read(uc, UC_ARM_REG_PC, &pc);
    uc_reg_read(uc, UC_ARM_REG_CPSR, &cpsr);
    uint32_t target = (cpsr & (1u << 5)) ? ((pc - 2u) | 1u) : pc - 4u;
    if (intno != 2 || !host_code_address(target)) {
        report_fault(uc, "unexpected interrupt", target);
        uc_emu_stop(uc);
        return;
    }

    uint32_t a[HOST_ARG_WORDS];
    uint32_t sp, lr;
    uc_reg_read(uc, UC_ARM_REG_R0, &a[0]);
    uc_reg_read(uc, UC_ARM_REG_R1, &a[1]);
    uc_reg_read(uc, UC_ARM_REG_R2, &a[2]);
    uc_reg_read(uc, UC_ARM_REG_R3, &a[3]);
    uc_reg_read(uc, UC_ARM_REG_SP, &sp);
    uc_reg_read(uc, UC_ARM_REG_LR, &lr);
    const uint32_t *stack = (const uint32_t *)(uintptr_t)sp;
    for (int i = 4; i < (int)HOST_ARG_WORDS; ++i) {
        a[i] = stack[i - 4];
    }

    trace_call(target, a, lr);
    if (g_main_uc == NULL) {
        g_main_uc = uc;
    }
    host_fn16 fn = (host_fn16)(uintptr_t)target;
    uint64_t result = fn(a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10],
                         a[11], a[12], a[13], a[14], a[15]);

    uint32_t lo = (uint32_t)result;
    uint32_t hi = (uint32_t)(result >> 32);
    uc_reg_write(uc, UC_ARM_REG_R0, &lo);
    uc_reg_write(uc, UC_ARM_REG_R1, &hi);
    uc_reg_write(uc, UC_ARM_REG_PC, &lr);
}

static struct arm_emu *emu_create(void) {
    struct arm_emu *emu = calloc(1, sizeof(*emu));
    if (!emu) {
        return NULL;
    }
    if (uc_open(UC_ARCH_ARM, UC_MODE_ARM, &emu->uc) != UC_ERR_OK) {
        free(emu);
        return NULL;
    }
    uc_ctl_set_cpu_model(emu->uc, UC_CPU_ARM_CORTEX_A15);

    uint32_t cpacr = 0xfu << 20;
    uint32_t fpexc = 0x40000000u;
    uc_reg_write(emu->uc, UC_ARM_REG_C1_C0_2, &cpacr);
    uc_reg_write(emu->uc, UC_ARM_REG_FPEXC, &fpexc);

    uc_hook hook;
    uc_hook_add(emu->uc, &hook, UC_HOOK_MEM_UNMAPPED, (void *)on_unmapped, NULL, 1, 0);
    uc_hook_add(emu->uc, &hook, UC_HOOK_INTR, (void *)on_interrupt, NULL, 1, 0);

    emu->stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (emu->stack == MAP_FAILED) {
        uc_close(emu->uc);
        free(emu);
        return NULL;
    }
    uint32_t sp = (uint32_t)(uintptr_t)(emu->stack + STACK_SIZE - 64);
    uc_reg_write(emu->uc, UC_ARM_REG_SP, &sp);
    emu->sentinel = (uint32_t)(uintptr_t)g_svc_page + PAGE_SIZE_EMU - 4;
    return emu;
}

bool arm_emu_init(void) {
    if (g_svc_page) {
        return true;
    }
    void *page = mmap(NULL, PAGE_SIZE_EMU, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (page == MAP_FAILED) {
        fprintf(stderr, "[arm] svc page: %s\n", strerror(errno));
        return false;
    }
    g_svc_page = page;
    const char *trace = getenv("BOZ_TRACE_CALLS");
    g_trace_limit = trace ? strtol(trace, NULL, 10) : 0;
    if (getenv("BOZ_TRACE_STATUS")) {
        pthread_t thread;
        pthread_create(&thread, NULL, status_thread, NULL);
        pthread_detach(thread);
    }
    for (size_t i = 0; i < PAGE_SIZE_EMU / sizeof(uint32_t); ++i) {
        g_svc_page[i] = SVC_ARM_WORD;
    }
    return true;
}

uint64_t arm_emu_call(uint32_t fn, int argc, const uint32_t *argv) {
    if (!t_emu) {
        t_emu = emu_create();
        if (!t_emu) {
            fprintf(stderr, "[arm] unable to create CPU\n");
            abort();
        }
    }
    uc_engine *uc = t_emu->uc;
    uc_context *saved = NULL;
    if (t_emu->depth > 0) {
        uc_context_alloc(uc, &saved);
        uc_context_save(uc, saved);
    }

    static const int arg_regs[4] = {UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3};
    uint32_t sp;
    uc_reg_read(uc, UC_ARM_REG_SP, &sp);
    int stack_args = argc > 4 ? argc - 4 : 0;
    sp -= (uint32_t)((stack_args * 4 + 7) & ~7);
    for (int i = 0; i < argc; ++i) {
        if (i < 4) {
            uc_reg_write(uc, arg_regs[i], &argv[i]);
        } else {
            ((uint32_t *)(uintptr_t)sp)[i - 4] = argv[i];
        }
    }
    uc_reg_write(uc, UC_ARM_REG_SP, &sp);
    uint32_t lr = t_emu->sentinel;
    uc_reg_write(uc, UC_ARM_REG_LR, &lr);

    t_emu->depth++;
    uc_err err = uc_emu_start(uc, fn, t_emu->sentinel, 0, 0);
    t_emu->depth--;
    if (err != UC_ERR_OK) {
        uint32_t pc;
        uc_reg_read(uc, UC_ARM_REG_PC, &pc);
        fprintf(stderr, "[arm] call 0x%08x stopped: %s (pc=%08x)\n", fn, uc_strerror(err), pc);
        abort();
    }

    uint32_t lo, hi;
    uc_reg_read(uc, UC_ARM_REG_R0, &lo);
    uc_reg_read(uc, UC_ARM_REG_R1, &hi);
    if (saved) {
        uc_context_restore(uc, saved);
        uc_context_free(saved);
    } else {
        sp += (uint32_t)((stack_args * 4 + 7) & ~7);
        uc_reg_write(uc, UC_ARM_REG_SP, &sp);
    }
    return ((uint64_t)hi << 32) | lo;
}

void arm_emu_thread_exit(void) {
    if (!t_emu) {
        return;
    }
    uc_close(t_emu->uc);
    munmap(t_emu->stack, STACK_SIZE);
    free(t_emu);
    t_emu = NULL;
}
