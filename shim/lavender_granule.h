#ifndef LAVENDER_GRANULE_H
#define LAVENDER_GRANULE_H
#include <stdint.h>

/* ID_AA64MMFR0_EL1.TGran16 is bits [23:20]. Values 1 and 2 indicate
 * 16 KiB granule support (the latter also supports the LPA2 regime); 0 and
 * reserved encodings must not be treated as support. */
static inline int lavender_tgran16_supported(uint64_t id_aa64mmfr0)
{
    uint64_t value = (id_aa64mmfr0 >> 20) & 0xfU;
    return value == 1U || value == 2U;
}
#endif
