#pragma once
#include <stdint.h>
extern volatile uintptr_t lavender_guardian_cell_pa;
void lavender_guardian_run(void) __attribute__((noreturn));
