
#include <stdint.h>
#include <xen/hvm/hvm_op.h>
#include <xen/hvm/params.h>
#include <platform/console.h>
#include <platform/hypercalls.h>

int hvm_get_parameter(int idx, uint64_t *value)
{
        int r;
	struct xen_hvm_param xhv;

        xhv.domid = DOMID_SELF;
        xhv.index = idx;

        r = HYPERVISOR_hvm_op(HVMOP_get_param, &xhv);

        if (r < 0) {
                printk("Cannot get hvm parameter %d: %d!\n", idx, r);
        } else {
		*value = xhv.value;
	}

        return r;
}


