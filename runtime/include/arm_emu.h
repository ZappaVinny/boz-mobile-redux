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

#endif
