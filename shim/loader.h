#pragma once
#include <stdint.h>

/* Boot the SWEETXNU payload found in the ABL ramdisk; returns only on error. */
const char *loader_boot(uintptr_t dtb);

#ifdef TARGET_LAVENDER
extern volatile uintptr_t lavender_guardian_cell_pa;
#endif
