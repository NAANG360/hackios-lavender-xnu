/* Minimal Linux <sys/sysinfo.h> shim for cross-compiling Mesa freedreno to iOS/Darwin. */
#ifndef SWEET_SYS_SYSINFO_H
#define SWEET_SYS_SYSINFO_H

#include <sys/types.h>
#include <sys/sysctl.h>
#include <stdint.h>
#include <string.h>

struct sysinfo {
	long uptime;
	unsigned long loads[3];
	unsigned long totalram;
	unsigned long freeram;
	unsigned long sharedram;
	unsigned long bufferram;
	unsigned long totalswap;
	unsigned long freeswap;
	unsigned short procs;
	unsigned long totalhigh;
	unsigned long freehigh;
	unsigned int mem_unit;
};

static inline int sysinfo(struct sysinfo *info)
{
	if (!info)
		return -1;
	memset(info, 0, sizeof(*info));
	uint64_t mem = 1024ULL * 1024ULL * 1024ULL;
	size_t len = sizeof(mem);
	if (sysctlbyname("hw.memsize", &mem, &len, NULL, 0) != 0)
		mem = 1024ULL * 1024ULL * 1024ULL;
	info->totalram = (unsigned long)mem;
	info->freeram = (unsigned long)(mem / 2);
	info->mem_unit = 1;
	return 0;
}

#endif
