#ifndef ARM_EMU_H
#define ARM_EMU_H

#include <stdbool.h>
#include <stdint.h>

bool arm_emu_init(void);
void arm_emu_set_image(uint32_t start, uint32_t end);
void arm_emu_register_code(uint32_t start, uint32_t size);
void arm_emu_trace_ignore(uint32_t fn);
uint64_t arm_emu_call(uint32_t fn, int argc, const uint32_t *argv);
void arm_emu_thread_exit(void);
uint32_t arm_emu_call_scratch(void);
uint64_t arm_emu_call_host(uint32_t fn, const uint32_t *args);

/* A guest memory fault. r[15] is the faulting instruction (no Thumb bit); thumb is its mode. */
struct arm_emu_fault {
    uint32_t r[16];
    uint32_t address;
    bool write;
    bool thumb;
};

/* Called when guest code touches unmapped memory. Return true after editing r[] (and thumb) to
 * resume at r[15]; false lets the fault stop the game. */
typedef bool (*arm_emu_fault_handler)(struct arm_emu_fault *fault);
void arm_emu_set_fault_handler(arm_emu_fault_handler handler);

#endif
