#ifndef __ASM_BARRIER_H
#define __ASM_BARRIER_H

#include "xpseudo_asm_gcc.h"

#define mb()		__arm_heavy_mb()
#define rmb()		dsb()
#define wmb()		__arm_heavy_mb(st)
#define dma_rmb()	dmb(osh)
#define dma_wmb()	dmb(oshst)

#define smp_mb()	dmb()
#define smp_rmb()	smp_mb()
#define smp_wmb()	dmb()

#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))

#define read_barrier_depends()		do { } while(0)
#define smp_read_barrier_depends()	do { } while(0)

#endif /* __ASM_BARRIER_H */
