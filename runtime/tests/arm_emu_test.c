#include "arm_emu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

static uint32_t *g_code;
static int g_failures;

static void expect(const char *name, uint64_t got, uint64_t want) {
    if (got != want) {
        fprintf(stderr, "FAIL %s: got 0x%llx want 0x%llx\n", name, (unsigned long long)got,
                (unsigned long long)want);
        g_failures++;
    } else {
        fprintf(stderr, "ok   %s\n", name);
    }
}

static uint32_t host_add(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f) {
    return a + b + c + d + e + f;
}

static uint32_t __attribute__((stdcall)) host_stdcall(uint32_t a, uint32_t b, uint32_t c,
                                                      uint32_t d, uint32_t e, uint32_t f) {
    return a * 1000 + b * 100 + c * 10 + d + e + f;
}

static uint64_t host_wide(void) {
    return 0x1122334455667788ull;
}

static uint32_t g_recovered_value = 0x5eed;
static int g_recoveries;

/* Repairs "ldr r0, [r0]" with r0 = 0 by pointing r0 at a valid word and retrying. */
static bool recover_null_load(struct arm_emu_fault *fault) {
    if (fault->write || fault->address != 0 || fault->r[0] != 0) {
        return false;
    }
    fault->r[0] = (uint32_t)(uintptr_t)&g_recovered_value;
    g_recoveries++;
    return true;
}

static uint32_t host_reenter(uint32_t x) {
    uint32_t arg = x * 2;
    return (uint32_t)arm_emu_call((uint32_t)(uintptr_t)&g_code[0], 1, &arg);
}

int main(void) {
    if (!arm_emu_init()) {
        return 1;
    }
    g_code = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    uint32_t c[] = {
        /* 0: r0 + 1 */
        0xe2800001u, 0xe12fff1eu,
        /* 2: call r1(r0, 2, 3, 4, 5, 6) + 100 */
        0xe92d4010u, 0xe24dd008u, 0xe1a0c001u, 0xe3a01002u, 0xe3a02003u, 0xe3a03004u,
        0xe3a04005u, 0xe58d4000u, 0xe3a04006u, 0xe58d4004u, 0xe12fff3cu, 0xe2800064u,
        0xe28dd008u, 0xe8bd8010u,
        /* 16: tail call r0 */
        0xe12fff10u,
    };
    memcpy(g_code, c, sizeof(c));

    uint32_t args[6];
    args[0] = 41;
    expect("guest arithmetic", (uint32_t)arm_emu_call((uint32_t)(uintptr_t)&g_code[0], 1, args),
           42);

    args[0] = 1;
    args[1] = (uint32_t)(uintptr_t)&host_add;
    expect("guest calls host with stack args",
           (uint32_t)arm_emu_call((uint32_t)(uintptr_t)&g_code[2], 2, args), 1 + 2 + 3 + 4 + 5 + 6 + 100);

    args[0] = 1;
    args[1] = (uint32_t)(uintptr_t)&host_stdcall;
    expect("guest calls stdcall host (callee pops args)",
           (uint32_t)arm_emu_call((uint32_t)(uintptr_t)&g_code[2], 2, args),
           1 * 1000 + 2 * 100 + 3 * 10 + 4 + 5 + 6 + 100);

    args[0] = (uint32_t)(uintptr_t)&host_wide;
    expect("64-bit host return", arm_emu_call((uint32_t)(uintptr_t)&g_code[16], 1, args),
           0x1122334455667788ull);

    args[0] = 10;
    args[1] = (uint32_t)(uintptr_t)&host_reenter;
    expect("host re-enters guest", (uint32_t)arm_emu_call((uint32_t)(uintptr_t)&g_code[2], 2, args),
           10 * 2 + 1 + 100);

    static const char text[] = "host string";
    args[0] = (uint32_t)(uintptr_t)text;
    args[1] = (uint32_t)(uintptr_t)&strlen;
    uint32_t tail[] = {0xe12fff11u};
    memcpy(&g_code[20], tail, sizeof(tail));
    expect("guest passes host pointer to libc",
           (uint32_t)arm_emu_call((uint32_t)(uintptr_t)&g_code[20], 2, args), strlen(text));

    /* 24: ldr r0, [r0]; bx lr */
    uint32_t load[] = {0xe5900000u, 0xe12fff1eu};
    memcpy(&g_code[24], load, sizeof(load));
    arm_emu_set_fault_handler(recover_null_load);
    args[0] = 0;
    expect("fault handler repairs a null load",
           (uint32_t)arm_emu_call((uint32_t)(uintptr_t)&g_code[24], 1, args), 0x5eed);
    expect("fault handler ran once", (uint64_t)g_recoveries, 1);
    arm_emu_set_fault_handler(NULL);

    return g_failures ? 1 : 0;
}
