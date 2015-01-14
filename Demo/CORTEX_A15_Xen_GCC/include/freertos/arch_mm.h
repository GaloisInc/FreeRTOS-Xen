#ifndef _ARCH_MM_H_
#define _ARCH_MM_H_

#define PAGE_SHIFT		12
#define PAGE_SIZE		(1 << PAGE_SHIFT)
#define PAGE_MASK       (~(PAGE_SIZE-1))

#define L1_PAGETABLE_SHIFT      12

#define VIRT_START                 ((unsigned long)0)

#define to_phys(x)                 ((unsigned long)(x)-VIRT_START)
#define to_virt(x)                 ((void *)((unsigned long)(x)+VIRT_START))

#define PFN_UP(x)	(((x) + PAGE_SIZE-1) >> L1_PAGETABLE_SHIFT)
#define PFN_DOWN(x)	((x) >> L1_PAGETABLE_SHIFT)
#define PFN_PHYS(x)	((uint64_t)(x) << L1_PAGETABLE_SHIFT)
#define PHYS_PFN(x)	((x) >> L1_PAGETABLE_SHIFT)

#define virt_to_pfn(_virt)         (PFN_DOWN(to_phys(_virt)))
#define virt_to_mfn(_virt)         (0)
#define mach_to_virt(_mach)        (0)
#define virt_to_mach(_virt)        (0)
#define mfn_to_virt(_mfn)          (0)
#define pfn_to_virt(_pfn)          (to_virt(PFN_PHYS(_pfn)))

#endif
