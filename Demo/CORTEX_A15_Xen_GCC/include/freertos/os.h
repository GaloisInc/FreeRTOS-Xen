/*
 * Copyright (C) 2014-2015 Galois, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License** as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.

 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _OS_H_
#define _OS_H_

#include <stdint.h>
#include <xen/xen.h>

void arch_fini(void);

#define BUG() while(1){}

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define ASSERT(x)                                              \
do {                                                           \
        if (!(x)) {                                                \
                printk("ASSERTION FAILED: %s at %s:%d.\n",             \
                           # x ,                                           \
                           __FILE__,                                       \
                           __LINE__);                                      \
        BUG();                                                 \
        }                                                          \
} while(0)

#define BUG_ON(x) ASSERT(!(x))

#define smp_processor_id() 0

#define barrier() __asm__ __volatile__("": : :"memory")

extern shared_info_t *HYPERVISOR_shared_info;

#if 0
static inline void force_evtchn_callback(void)
{
    int save;
    vcpu_info_t *vcpu;
    vcpu = &HYPERVISOR_shared_info->vcpu_info[smp_processor_id()];
    save = vcpu->evtchn_upcall_mask;

    while (vcpu->evtchn_upcall_pending) {
        vcpu->evtchn_upcall_mask = 1;
        barrier();
        do_hypervisor_callback(NULL);
        barrier();
        vcpu->evtchn_upcall_mask = save;
        barrier();
    };
}
#else
#define force_evtchn_callback(void) do {} while(0)
#endif

// disable interrupts
static inline void __cli(void) {
	int x;
	__asm__ __volatile__("mrs %0, cpsr;cpsid i":"=r"(x)::"memory");
}

// enable interrupts
static inline void __sti(void) {
	int x;
	__asm__ __volatile__("mrs %0, cpsr\n"
						"bic %0, %0, #0x80\n"
						"msr cpsr_c, %0"
						:"=r"(x)::"memory");
}

static inline int irqs_disabled() {
	int x;
	__asm__ __volatile__("mrs %0, cpsr\n":"=r"(x)::"memory");
	return (x & 0x80);
}

#define local_irq_save(x) { \
	__asm__ __volatile__("mrs %0, cpsr;cpsid i; and %0, %0, #0x80":"=r"(x)::"memory");	\
}

#define local_irq_restore(x) {	\
	__asm__ __volatile__("msr cpsr_c, %0"::"r"(x):"memory");	\
}

#define local_save_flags(x)	{ \
		__asm__ __volatile__("mrs %0, cpsr; and %0, %0, 0x80":"=r"(x)::"memory");	\
}

#define local_irq_disable()	__cli()
#define local_irq_enable() __sti()

#define mb() __asm__("dmb");
#define rmb() __asm__("dmb");
#define wmb() __asm__("dmb");

#define LOCK_PREFIX ""
#define LOCK ""

#define unlikely(x)  __builtin_expect((x),0)
#define likely(x)  __builtin_expect((x),1)

//#define ADDR (*(volatile long *) addr)

/************************** arm *******************************/
#define xchg(ptr,v) ((__typeof__(*(ptr)))__xchg((unsigned long)(v),(ptr),sizeof(*(ptr))))
#define __xg(x) ((volatile long *)(x))

static inline unsigned long __xchg(unsigned long x, volatile void * ptr, int size)
{
	//TODO
	unsigned volatile long y;
	switch(size){
	case 1:
#if CPU_EXCLUSIVE_LDST
		__asm__ __volatile__("1:ldrexb %0, [%1]\n"
			"strexb %3, %2, [%1]\n"
			"cmp %3, #1\n"
			"beq 1b\n\n"
			"dmb\n":"=&r"(y):"r"(ptr), "r"(x), "r"(tmp):"memory");
#else
		y = (*(char *)ptr) & 0x000000ff;
		*((char *)ptr) = (char)x;
#endif
		break;
	case 2:
#if CPU_EXCLUSIVE_LDST
		__asm__ __volatile__("1:ldrexh %0, [%1]\n"
			"strexh %3, %2, [%1]\n"
			"cmp %3, #1\n"
			"beq 1b\n\n"
			"dmb\n":"=&r"(y):"r"(ptr), "r"(x), "r"(tmp):"memory");
#else
		y = (*(short *)ptr) & 0x0000ffff;
		*((short *)ptr) = (short)x;
#endif
		break;
	default: // 4
#if CPU_EXCLUSIVE_LDST
		__asm__ __volatile__("1:ldrex %0, [%1]\n"
			"strex %3, %2, [%1]\n"
			"cmp %3, #1\n"
			"beq 1b\n\n"
			"dmb\n":"=&r"(y):"r"(ptr), "r"(x), "r"(tmp):"memory");
#else
		y = (*(unsigned long *)ptr) & 0xffffffff;
		*((unsigned long *)ptr) = x;
#endif
		break;
	}
    return y;
}

/**
 * test_and_clear_bit - Clear a bit and return its old value
 * @nr: Bit to clear
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.
 * It also implies a memory barrier.
 */
static __inline__ int test_and_clear_bit(int nr, volatile void * addr)
{
	//TODO
	unsigned long *tmp = (unsigned long *)addr;

	int x = tmp[nr >> 5] & (1 << (nr & 0x1f));
	tmp[nr >> 5] &= ~(1 << (nr & 0x1f));
    return x;
}

static __inline__ int test_and_set_bit(int nr, volatile void * addr)
{
	//TODO
	unsigned long *tmp = (unsigned long *)addr;

	int x = tmp[nr >> 5] & (1 << (nr & 0x1f));
	tmp[nr >> 5] |= (1 << (nr & 0x1f));
    return x;
}

static __inline__ int constant_test_bit(int nr, const volatile void * addr)
{
	//TODO
	unsigned long *tmp = (unsigned long *)addr;
    return tmp[nr >> 5] & (1 << (nr & 0x1f));
}

static __inline__ int variable_test_bit(int nr, volatile const void * addr)
{
	//TODO:
	unsigned long *tmp = (unsigned long *)addr;
	return tmp[nr >> 5] & (1 << (nr & 0x1f));
}

//TODO
#define test_bit(nr,addr) (((unsigned long *)addr)[nr >> 5] & (1 << (nr & 0x1f)))


/**
 * set_bit - Atomically set a bit in memory
 * @nr: the bit to set
 * @addr: the address to start counting from
 *
 * This function is atomic and may not be reordered.  See __set_bit()
 * if you do not require the atomic guarantees.
 * Note that @nr may be almost arbitrarily large; this function is not
 * restricted to acting on a single-word quantity.
 */
static __inline__ void set_bit(int nr, volatile void * addr)
{
	//TODO:
	unsigned long *tmp = (unsigned long *)addr;
	tmp[nr >> 5] |= (1 << (nr & 0x1f));
}

/**
 * clear_bit - Clears a bit in memory
 * @nr: Bit to clear
 * @addr: Address to start counting from
 *
 * clear_bit() is atomic and may not be reordered.  However, it does
 * not contain a memory barrier, so if it is used for locking purposes,
 * you should call smp_mb__before_clear_bit() and/or smp_mb__after_clear_bit()
 * in order to ensure changes are visible on other processors.
 */
static __inline__ void clear_bit(int nr, volatile void * addr)
{
	//TODO
	unsigned long *tmp = (unsigned long *)addr;
	tmp[nr >> 5] &= (unsigned long)~(1 << (nr & 0x1f));
}

/**
 * __ffs - find first bit in word.
 * @word: The word to search
 *
 * Undefined if no bit exists, so code should check against 0 first.
 */
static __inline__ unsigned long __ffs(unsigned long word)
{
	//TODO
	int index = 0;
	while(!(word & (1 << index++))){};

	return index - 1;
}

//FIXME
#define rdtscll(val) (val = 0)
#define wrmsr(msr,val1,val2) (0)
#define wrmsrl(msr,val) wrmsr(msr,(uint32_t)((uint64_t)(val)),((uint64_t)(val))>>32)

/********************* common arm32 and arm64  ****************************/
struct __synch_xchg_dummy { unsigned long a[100]; };
#define __synch_xg(x) ((struct __synch_xchg_dummy *)(x))

#define synch_cmpxchg(ptr, old, new) (0)

static inline unsigned long __synch_cmpxchg(volatile void *ptr,
        unsigned long old,
        unsigned long new, int size)
{
	//TODO:
    //BUG();
    return 0;
}


static __inline__ void synch_set_bit(int nr, volatile void * addr)
{
	//TODO:
	set_bit(nr, addr);
}

static __inline__ void synch_clear_bit(int nr, volatile void * addr)
{
	//TODO:
    clear_bit(nr, addr);
}

static __inline__ int synch_test_and_set_bit(int nr, volatile void * addr)
{
	//TODO:
	return test_and_set_bit(nr, addr);
}

static __inline__ int synch_test_and_clear_bit(int nr, volatile void * addr)
{
	//TODO:
    return test_and_clear_bit(nr, addr);
}

#define synch_test_bit(nr,addr) test_bit(nr, addr)

//#undef ADDR

#endif
