
#include <stdint.h>
#include <inttypes.h>
#include <xen/hvm/params.h>
#include <xen/io/xs_wire.h>

#include <freertos/os.h>
#include <freertos/arch_mm.h>
#include <freertos/mmu.h>
#include <platform/hypervisor.h>
#include <platform/console.h>
#include <platform/xen_hvm.h>

void arch_init_xenbus(struct xenstore_domain_interface **xenstore_buf, uint32_t *store_evtchn) {
	uint64_t value;
	uint64_t xenstore_pfn;

	if (hvm_get_parameter(HVM_PARAM_STORE_EVTCHN, &value))
		BUG();

	*store_evtchn = (int)value;

	if(hvm_get_parameter(HVM_PARAM_STORE_PFN, &xenstore_pfn))
		BUG();

	printk("arch_init_xenbus: xenstore pfn = %llu\n", xenstore_pfn);

	*xenstore_buf = (struct xenstore_domain_interface *) map_frame(xenstore_pfn);
}
