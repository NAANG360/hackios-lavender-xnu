/*
 * Just enough flattened-device-tree parsing to find what ABL put in
 * /chosen (initrd location), /memory (DRAM banks), and /reserved-memory
 * (no-map Qualcomm carve-outs). Big-endian, 4-byte aligned tokens.
 */
#include "fdt.h"

#define FDT_MAGIC	0xd00dfeed
#define FDT_BEGIN_NODE	1
#define FDT_END_NODE	2
#define FDT_PROP	3
#define FDT_NOP		4
#define FDT_END		9

static uint32_t be32(uintptr_t a)
{
	return __builtin_bswap32(*(volatile uint32_t *)a);
}

static uint64_t be64(uintptr_t a)
{
	return ((uint64_t)be32(a) << 32) | be32(a + 4);
}

static int streq(const char *a, const char *b)
{
	while (*a && *a == *b)
		a++, b++;
	return *a == *b;
}

static int strprefix(const char *s, const char *p)
{
	while (*p) {
		if (*s++ != *p++)
			return 0;
	}
	return 1;
}

static uintptr_t align4(uintptr_t a)
{
	return (a + 3) & ~3UL;
}

static uintptr_t skip_str(uintptr_t p)
{
	/* byte loads are fine on Device memory as long as they are aligned to 1 */
	while (*(volatile char *)p)
		p++;
	return p + 1;
}

int fdt_chosen_u64(uintptr_t fdt, const char *prop, uint64_t *out)
{
	if (!fdt || be32(fdt) != FDT_MAGIC)
		return 0;
	uintptr_t p = fdt + be32(fdt + 8);		/* off_dt_struct */
	uintptr_t strings = fdt + be32(fdt + 12);	/* off_dt_strings */
	int depth = 0, in_chosen = 0;

	for (;;) {
		uint32_t tok = be32(p);
		p += 4;
		switch (tok) {
		case FDT_BEGIN_NODE: {
			const char *name = (const char *)p;
			depth++;
			in_chosen = depth == 2 && streq(name, "chosen");
			p = align4(skip_str(p));
			break;
		}
		case FDT_END_NODE:
			if (in_chosen)
				return 0;
			depth--;
			break;
		case FDT_PROP: {
			uint32_t len = be32(p), nameoff = be32(p + 4);
			uintptr_t val = p + 8;
			if (in_chosen && streq((const char *)(strings + nameoff), prop)) {
				if (len == 4)
					*out = be32(val);
				else if (len == 8)
					*out = be64(val);
				else
					return 0;
				return 1;
			}
			p = align4(val + len);
			break;
		}
		case FDT_NOP:
			break;
		default:	/* FDT_END or garbage */
			return 0;
		}
	}
}

/*
 * Parse ABL's live FDT to discover the full usable DRAM ceiling above phys_base:
 *   1. Find the /memory (or /memory@...) bank covering phys_base and take its end.
 *   2. Clamp below any /reserved-memory children child with `no-map` whose base >= phys_base,
 *      so Qualcomm TZ/HYP/modem/ADSP/CDSP/WLAN carve-outs are never disturbed.
 */
int fdt_memory_ceiling(uintptr_t fdt, uint64_t phys_base, uint64_t *out_end)
{
	if (!fdt || be32(fdt) != FDT_MAGIC)
		return 0;
	uintptr_t p = fdt + be32(fdt + 8);
	uintptr_t strings = fdt + be32(fdt + 12);

	uint32_t root_ac = 2, root_sc = 2;
	uint32_t rsv_ac = 2, rsv_sc = 2;
	int depth = 0, in_mem = 0, in_rsv = 0, in_rsv_child = 0;

	uint64_t best_end = 0;
	uint64_t rsv_clamp = ~0ULL;

	/* Honor FDT header memreserve entries above phys_base. */
	uintptr_t rsvmap = fdt + be32(fdt + 16);
	for (int i = 0; i < 64; i++) {
		uint64_t rbase = be64(rsvmap + (uintptr_t)i * 16);
		uint64_t rsz = be64(rsvmap + (uintptr_t)i * 16 + 8);
		if (rbase == 0 && rsz == 0)
			break;
		if (rsz && rbase >= phys_base && rbase < rsv_clamp)
			rsv_clamp = rbase;
	}

	/* Per-child state for /reserved-memory children */
	int child_nomap = 0;
	uintptr_t child_reg = 0;
	uint32_t child_reg_len = 0;

	for (;;) {
		uint32_t tok = be32(p);
		p += 4;
		switch (tok) {
		case FDT_BEGIN_NODE: {
			const char *name = (const char *)p;
			depth++;
			if (depth == 2) {
				in_mem = streq(name, "memory") || strprefix(name, "memory@");
				in_rsv = streq(name, "reserved-memory");
				if (in_rsv) {
					rsv_ac = root_ac;
					rsv_sc = root_sc;
				}
			} else if (depth == 3 && in_rsv) {
				in_rsv_child = 1;
				child_nomap = 0;
				child_reg = 0;
				child_reg_len = 0;
			}
			p = align4(skip_str(p));
			break;
		}
		case FDT_END_NODE:
			if (depth == 3 && in_rsv_child) {
				if (child_nomap && child_reg && child_reg_len) {
					uint32_t stride = (rsv_ac + rsv_sc) * 4;
					for (uint32_t off = 0; stride && off + stride <= child_reg_len; off += stride) {
						uint64_t base = (rsv_ac == 2) ? be64(child_reg + off) : be32(child_reg + off);
						uint64_t sz = (rsv_sc == 2) ? be64(child_reg + off + rsv_ac * 4) : be32(child_reg + off + rsv_ac * 4);
						if (sz && base >= phys_base && base < rsv_clamp)
							rsv_clamp = base;
					}
				}
				in_rsv_child = 0;
			} else if (depth == 2) {
				in_mem = 0;
				in_rsv = 0;
			}
			depth--;
			break;
		case FDT_PROP: {
			uint32_t len = be32(p), nameoff = be32(p + 4);
			uintptr_t val = p + 8;
			const char *pname = (const char *)(strings + nameoff);
			if (depth == 1 && len == 4) {
				if (streq(pname, "#address-cells"))
					root_ac = be32(val);
				else if (streq(pname, "#size-cells"))
					root_sc = be32(val);
			} else if (depth == 2 && in_rsv && len == 4) {
				if (streq(pname, "#address-cells"))
					rsv_ac = be32(val);
				else if (streq(pname, "#size-cells"))
					rsv_sc = be32(val);
			} else if (depth == 2 && in_mem && streq(pname, "reg")) {
				uint32_t stride = (root_ac + root_sc) * 4;
				for (uint32_t off = 0; stride && off + stride <= len; off += stride) {
					uint64_t base = (root_ac == 2) ? be64(val + off) : be32(val + off);
					uint64_t sz = (root_sc == 2) ? be64(val + off + root_ac * 4) : be32(val + off + root_ac * 4);
					if (sz && base <= phys_base && base + sz > phys_base) {
						if (base + sz > best_end)
							best_end = base + sz;
					}
				}
			} else if (depth == 3 && in_rsv_child) {
				if (streq(pname, "no-map"))
					child_nomap = 1;
				else if (streq(pname, "reg")) {
					child_reg = val;
					child_reg_len = len;
				}
			}
			p = align4(val + len);
			break;
		}
		case FDT_NOP:
			break;
		default:
			if (best_end > phys_base) {
				if (rsv_clamp < best_end)
					best_end = rsv_clamp;
				if (best_end > phys_base) {
					*out_end = best_end;
					return 1;
				}
			}
			return 0;
		}
	}
}
