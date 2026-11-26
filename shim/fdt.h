#pragma once
#include <stdint.h>

/* Read a u32/u64 (cell-size agnostic) property of /chosen. Returns 0 if absent. */
int fdt_chosen_u64(uintptr_t fdt, const char *prop, uint64_t *out);

/*
 * Find the safe upper physical address of the DRAM bank containing phys_base
 * from ABL's live FDT (/memory + /reserved-memory no-map carve-outs).
 * Returns 1 and writes the ceiling PA to *out_end, or 0 if unavailable.
 */
int fdt_memory_ceiling(uintptr_t fdt, uint64_t phys_base, uint64_t *out_end);
