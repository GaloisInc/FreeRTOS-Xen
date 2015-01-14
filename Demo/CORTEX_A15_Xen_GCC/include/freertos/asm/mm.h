
#ifndef _ASM_MM_H
#define _ASM_MM_H

void invalidate_tlb(void);
void install_pt_address(void);
void set_domain_permissions(void);
void flush_caches(void);
void enable_mmu(void);

#endif
