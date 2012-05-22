#ifndef _TIMER_H
#define _TIMER_H

#define _RPC_COUNT
#define _LIBCALL_COUNT_TIME
#define _SYSCALL_COUNT_TIME

#define _CPU_FRQ 2566.099  //MHz

void start_timer();

double stop_timer(long num, FILE *fp);

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif
 
static __inline__ uint64_t rdtsc(void) {
	uint32_t lo, hi;
	__asm__ __volatile__ (
	"        xorl %%eax,%%eax \n"
	"        cpuid"      // serialize
	::: "%rax", "%rbx", "%rcx", "%rdx");
	/* We cannot use "=A", since this would use %rax on x86_64 and return only the lower 32bits of the TSC */
	__asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
	return (uint64_t)hi << 32 | lo;
}

#endif //_TIMER_H
