#ifndef SWEET_SYS_SYSINFO_H
#define SWEET_SYS_SYSINFO_H

#include <sys/types.h>
#include <sys/sysctl.h>
#include <string.h>

struct sysinfo {
	unsigned long totalram;
	unsigned long freeram;
	unsigned int mem_unit;
};

static inline int
sysinfo(struct sysinfo *info)
{
	uint64_t mem = 1024ULL * 1024ULL * 1024ULL;
	size_t len = sizeof(mem);
	if (!info)
		return -1;
	memset(info, 0, sizeof(*info));
	if (sysctlbyname("hw.memsize", &mem, &len, NULL, 0) != 0)
		mem = 1024ULL * 1024ULL * 1024ULL;
	info->totalram = (unsigned long)mem;
	info->freeram = (unsigned long)(mem / 2);
	info->mem_unit = 1;
	return 0;
}

#endif
