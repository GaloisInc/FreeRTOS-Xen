#ifndef __WAITTYPES_H__
#define __WAITTYPES_H__

#include <FreeRTOS.h>
#include <list.h>

#define DECLARE_WAIT_QUEUE_HEAD(name) xList name

#define init_waitqueue_head vListInitialise

#endif /* __WAIT_H__ */

/*
 * Local variables:
 * mode: C
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
