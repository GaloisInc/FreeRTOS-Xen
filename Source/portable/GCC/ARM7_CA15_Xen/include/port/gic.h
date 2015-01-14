
#ifndef __ASSEMBLER__
#include <stdint.h>

extern uint32_t ulPriorityShift;
extern uint32_t ulGICUniqueIntPriorities;

void handle_event();
void gic_enable_event_irq(void);
void gic_enable_vtimer_irq(void);
int gic_register_handler(uint32_t irq, void (*)(uint32_t));

void gic_enable_interrupt(unsigned char irq_number, unsigned char cpu_set,
        unsigned char level_sensitive, unsigned char ppi);
void gic_set_priority(unsigned char irq_number, unsigned char priority);
#endif

// Distributor Interface
#define GICD_CTLR		0x0
#define GICD_ISR                0x80
#define GICD_ISENABLER		0x100
#define GICD_PRIORITY		0x400
#define GICD_ITARGETSR		0x800
#define GICD_ICFGR		0xC00

// CPU Interface
#define GICC_CTLR		0x0
#define GICC_PMR		0x4
#define GICC_BPR		0x8
#define GICC_IAR		0xc
#define GICC_EOIR		0x10
#define GICC_RPR		0x14
#define GICC_HPIR		0x18

// From the GIC Architecture Specification 1.0, page 3-9
#define SPURIOUS_IRQ            1023

#define VTIMER_TICK		0x6000000
#define GICD_BASE		0x2c001000
#define GICC_BASE		0x2c002000

#define gicd(offset) (gic.gicd_base + (offset))
#define gicc(offset) (gic.gicc_base + (offset))

#define REG(addr) ((uint32_t *)(addr))

#define mb()			__asm__("dmb");
#define rmb()			__asm__("dmb");
#define wmb()			__asm__("dmb");
