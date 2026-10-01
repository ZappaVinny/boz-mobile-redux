#ifndef ARM_EMU_H
#define ARM_EMU_H

#include <stdbool.h>
#include <stdint.h>

bool arm_emu_init(void);
uint64_t arm_emu_call(uint32_t fn, int argc, const uint32_t *argv);
void arm_emu_thread_exit(void);

#endif
