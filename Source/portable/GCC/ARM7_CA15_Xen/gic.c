
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

#include <stdint.h>
#include <stdlib.h>
#include <libfdt.h>
#include <port/gic.h>
#include <portmacro.h>
#include <platform/console.h>
#include <freertos/popcount.h>
#include <freertos/mmu.h>

void schedule_timer();

struct gic_info gic;

extern unsigned long IRQ_handler;

uint32_t ulGICUniqueIntPriorities;
uint32_t ulPriorityShift;
uint32_t ulLowestPriority;

#define NUM_IRQS 1024
void (*handlers[NUM_IRQS])(uint32_t);

static inline uint32_t REG_READ32(volatile uint32_t *addr)
{
	uint32_t value;
	__asm__ __volatile__("ldr %0, [%1]":"=&r"(value):"r"(addr));
	rmb();
	return value;
}

static inline void REG_WRITE32(volatile uint32_t *addr, unsigned int value)
{
	__asm__ __volatile__("str %0, [%1]"::"r"(value), "r"(addr));
	wmb();
}

void gic_set_priority(unsigned char irq_number, unsigned char priority)
{
	uint32_t regnum, offset;
	volatile uint32_t *ptr;
	uint32_t value;

	// The register number is irq_num `div` 4
	regnum = irq_number >> 2;

	// The offset in the register (starting from bit 0 on the right) is
	// irq_num `mod` 4
	offset = irq_number & 0x3;

	ptr = (uint32_t *) (gic.gicd_base + GICD_PRIORITY + (4 * regnum));

	value = *ptr;
	dprintk("gic: set prio: regnum %x, offset %x, old value %x\n",
			regnum, offset, value);

	value &= ~(0xff << (offset * 8));
	value |= priority << (offset * 8);

	// Write the register.
	*ptr = value;
	wmb();
}

static void gic_route_interrupt(unsigned char irq_number, unsigned char cpu_set)
{
	uint32_t regnum, offset;
	volatile uint32_t *ptr;
	uint32_t value;

	// The register number is irq_num `div` 4
	regnum = irq_number >> 2;

	// The offset in the register (starting from bit 0 on the right) is
	// irq_num `mod` 4
	offset = irq_number & 0x3;

	ptr = (uint32_t *) (gic.gicd_base + GICD_ITARGETSR + (4 * regnum));

	value = *ptr;
	dprintk("gic: route int: regnum %x, offset %x, old value %x\n",
			regnum, offset, value);
	value &= ~(0xff << (offset * 8));
	value |= cpu_set << (offset * 8);

	// Write the register.
	*ptr = value;
	wmb();
}

void gic_enable_interrupt(unsigned char irq_number,
		unsigned char cpu_set, unsigned char level_sensitive, unsigned char ppi)
{
	void *set_enable_reg;
	void *cfg_reg;

	// set priority
	gic_set_priority(irq_number, 0x0);

	// set target cpus for this interrupt
	gic_route_interrupt(irq_number, cpu_set);

	// set level/edge triggered
	cfg_reg = (void *)gicd(GICD_ICFGR);
	level_sensitive ? clear_bit((irq_number * 2) + 1, cfg_reg) : set_bit((irq_number * 2) + 1, cfg_reg);
	if (ppi)
		clear_bit((irq_number * 2), cfg_reg);

	wmb();

	// enable forwarding interrupt from distributor to cpu interface
	set_enable_reg = (void *)gicd(GICD_ISENABLER);
	set_bit(irq_number, set_enable_reg);
	wmb();

	printk("Interrupt enabled: 0x%x\n", irq_number);
}

static void gic_enable_interrupts()
{
	// Global enable forwarding interrupts from distributor to cpu interface
	REG_WRITE32(REG(gicd(GICD_CTLR)), 0x00000001);

	// Global enable signalling of interrupt from the cpu interface
	REG_WRITE32(REG(gicc(GICC_CTLR)), 0x00000001);
}

static void gic_disable_interrupts()
{
	// Global disable signalling of interrupt from the cpu interface
	REG_WRITE32(REG(gicc(GICC_CTLR)), 0x00000000);

	// Global disable forwarding interrupts from distributor to cpu interface
	REG_WRITE32(REG(gicd(GICD_CTLR)), 0x00000000);
}

void gic_cpu_set_priority_mask(char priority)
{
	REG_WRITE32(REG(gicc(GICC_PMR)), priority & 0x000000FF);
}

/*
 * Read the interrupt ID from the ack register.
 */
uint32_t gic_readiar() {
	uint32_t irq_id;
	irq_id = REG_READ32(REG(gicc(GICC_IAR))) & 0x000003FF;
	dprintk("IAR: id = 0x%x\n", irq_id);
	return irq_id;
}

/*
 * Signal an end of interrupt to the GIC.
 */
void gic_eoir(uint32_t irq) {
	REG_WRITE32(REG(gicc(GICC_EOIR)), irq & 0x000003FF);
}

void gic_handler(uint32_t irq) {
	dprintk("irq = %d, PMR = 0x%x\n", irq, portICCPMR_PRIORITY_MASK_REGISTER_VALUE);
	(handlers[irq])(irq);
}

void determine_priorities() {
	// Determine how many distinct priority values the GIC supports.
	//
	// Write 0xff to ICCPMR; read the result and check the number of
	// low-order zero bits.

	uint32_t new, num, total, step;

	new = REG_READ32(REG(gicc(GICC_PMR)));
	REG_WRITE32(REG(gicc(GICC_PMR)), new | 0xff);
	new = REG_READ32(REG(gicc(GICC_PMR)));

	num = new & 0xff;
	step = 256 - num;
	total = 256 / step;

	ulGICUniqueIntPriorities = total;
	ulLowestPriority = total - 1;
	ulPriorityShift = popcount(~new & 0xff);

	printk("GIC supports %d priority levels in steps of %d, %d low-order priority zero bits\n",
			total, step, ulPriorityShift);
}

void gic_default_handler(uint32_t irq)
{
	printk("Unhandled IRQ %u\n", irq);
}

int gic_register_handler(uint32_t irq, void (*h)(uint32_t))
{
	if (irq >= NUM_IRQS)
		return 1;

	if (handlers[irq] != gic_default_handler)
		return 1;

	handlers[irq] = h;

	return 0;
}

void gic_find_base_addrs(void *device_tree)
{
    int node = 0;
    int depth = 0;
    for (;;)
    {
        node = fdt_next_node(device_tree, node, &depth);
        if (node <= 0 || depth < 0)
            break;

        if (fdt_getprop(device_tree, node, "interrupt-controller", NULL)) {
            int len = 0;

            if (fdt_node_check_compatible(device_tree, node, "arm,cortex-a15-gic") &&
                fdt_node_check_compatible(device_tree, node, "arm,cortex-a7-gic")) {
                printk("Skipping incompatible interrupt-controller node\n");
                continue;
            }

            const uint64_t *reg = fdt_getprop(device_tree, node, "reg", &len);

            /* We have two registers (GICC and GICD), each of which contains
             * two parts (an address and a size), each of which is a 64-bit
             * value (8 bytes), so we expect a length of 2 * 2 * 8 = 32.
             * If any extra values are passed in future, we ignore them. */
            if (reg == NULL || len < 32) {
                printk("Bad 'reg' property: %p %d\n", reg, len);
                continue;
            }

            gic.gicd_base = (void *) ((long)fdt64_to_cpu(reg[0]));
            gic.gicc_base = (void *) ((long)fdt64_to_cpu(reg[2]));
            printk("Found GIC: gicd_base = %p, gicc_base = %p\n", gic.gicd_base, gic.gicc_base);
            break;
        }
    }

    if (!gic.gicd_base) {
        printk("GIC not found!\n");
        BUG();
    }

    wmb();
}

void gic_init(void) {
	int i, r;

	printk("device_tree = 0x%x\n", device_tree);
    if ((r = fdt_check_header(device_tree))) {
        printk("Invalid DTB from Xen: %s\n", fdt_strerror(r));
    } else {
        printk("Valid DTB pointer\n");
    }

	printk("Enabling GIC ...\n");

    gic_find_base_addrs(device_tree);

	for (i = 0; i < NUM_IRQS; i++)
		handlers[i] = gic_default_handler;

	gic_disable_interrupts();

	determine_priorities();
	gic_cpu_set_priority_mask(0xff);

	gic_enable_interrupts();
	printk("GIC enabled.\n");
}
