/* Fake physical address space for the host tests: ml_io_map() hands out
 * host buffers registered by the test, keyed by physical address. */
#ifndef MOCK_MMIO_H
#define MOCK_MMIO_H
#include "mock/xnu_mock.h"
#include <stdlib.h>

#define MOCK_MAX_REGIONS 64

struct mock_region {
	uint64_t pa;
	uint64_t size;
	uint8_t *host;
	unsigned maps;          /* how many times the driver mapped it */
};

static struct mock_region mock_regions[MOCK_MAX_REGIONS];
static unsigned mock_nregions;
static unsigned mock_failures;

#define CHECK(cond, ...) do { \
	if (!(cond)) { \
		mock_failures++; \
		fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__); \
		fprintf(stderr, __VA_ARGS__); \
		fprintf(stderr, "\n"); \
	} \
} while (0)

static uint8_t *
mock_add_region(uint64_t pa, uint64_t size)
{
	struct mock_region *r = &mock_regions[mock_nregions++];
	r->pa = pa;
	r->size = size;
	r->host = calloc(1, size);
	return r->host;
}

static struct mock_region *
mock_find(uint64_t pa, uint64_t size)
{
	for (unsigned i = 0; i < mock_nregions; i++) {
		struct mock_region *r = &mock_regions[i];
		if (pa >= r->pa && pa + size <= r->pa + r->size) {
			return r;
		}
	}
	return NULL;
}

static uint32_t *
mock_reg(uint64_t pa)
{
	struct mock_region *r = mock_find(pa, 4);
	if (r == NULL) {
		fprintf(stderr, "no mock region for PA 0x%llx\n", (unsigned long long)pa);
		abort();
	}
	return (uint32_t *)(r->host + (pa - r->pa));
}

vm_offset_t ml_io_map(vm_offset_t pa, vm_size_t size);
vm_offset_t
ml_io_map(vm_offset_t pa, vm_size_t size)
{
	struct mock_region *r = mock_find(pa, size);
	if (r == NULL) {
		mock_failures++;
		fprintf(stderr, "FAIL: driver mapped unexpected PA 0x%llx size 0x%llx\n",
		    (unsigned long long)pa, (unsigned long long)size);
		return 0;
	}
	r->maps++;
	return (vm_offset_t)(r->host + (pa - r->pa));
}

/* Physical address of a host pointer inside a region (for checks). */
static uint64_t
mock_pa_of(vm_offset_t va)
{
	for (unsigned i = 0; i < mock_nregions; i++) {
		struct mock_region *r = &mock_regions[i];
		if (va >= (vm_offset_t)r->host && va < (vm_offset_t)r->host + r->size) {
			return r->pa + (va - (vm_offset_t)r->host);
		}
	}
	return 0;
}

/* Boot-args */
static int mock_bootarg_sweetusb = -1, mock_bootarg_sweetkeys = -1, mock_bootarg_usblog = -1;
static int mock_bootarg_usbhold = -1, mock_bootarg_usbfix = -1;
static int mock_bootarg_rtcoff_set, mock_bootarg_rtcoff;

int
PE_parse_boot_argn(const char *name, void *value, int size)
{
	int v = -1;
	if (strcmp(name, "sweetusb") == 0) {
		v = mock_bootarg_sweetusb;
	} else if (strcmp(name, "sweetkeys") == 0) {
		v = mock_bootarg_sweetkeys;
	} else if (strcmp(name, "sweetusblog") == 0) {
		v = mock_bootarg_usblog;
	} else if (strcmp(name, "sweetusbhold") == 0) {
		v = mock_bootarg_usbhold;
	} else if (strcmp(name, "sweetrtcoff") == 0) {
		if (!mock_bootarg_rtcoff_set || size != sizeof(int)) {
			return 0;
		}
		*(int *)value = mock_bootarg_rtcoff;
		return 1;
	} else if (strcmp(name, "sweetusbfix") == 0) {
		v = mock_bootarg_usbfix;
	}
	if (v < 0 || size != sizeof(int)) {
		return 0;
	}
	*(int *)value = v;
	return 1;
}

/* Threads: record what was started, never run it. */
static thread_continue_t mock_threads[8];
static unsigned mock_nthreads;

kern_return_t
kernel_thread_start(thread_continue_t fn, void *arg, thread_t *thread)
{
	(void)arg;
	mock_threads[mock_nthreads++] = fn;
	*thread = (thread_t)1;
	return KERN_SUCCESS;
}

void
thread_deallocate(thread_t thread)
{
	(void)thread;
}

void
clock_interval_to_deadline(uint32_t interval, uint64_t scale, uint64_t *deadline)
{
	*deadline = interval * scale;
}

int
assert_wait_deadline(void *event, int interruptible, uint64_t deadline)
{
	(void)event; (void)interruptible; (void)deadline;
	return 0;
}
#endif
