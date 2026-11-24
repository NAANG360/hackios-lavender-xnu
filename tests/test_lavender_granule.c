#include <assert.h>
#include <stdint.h>
#include "lavender_granule.h"

int main(void)
{
    assert(lavender_tgran16_supported(1ULL << 20));
    assert(lavender_tgran16_supported(2ULL << 20));
    assert(!lavender_tgran16_supported(0));
    assert(!lavender_tgran16_supported(3ULL << 20));
    assert(!lavender_tgran16_supported(0xfULL << 20));
    assert(lavender_tgran16_supported((0x123ULL << 32) | (1ULL << 20)));
    return 0;
}
