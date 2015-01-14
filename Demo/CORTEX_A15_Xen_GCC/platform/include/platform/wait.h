#ifndef __WAIT_H__
#define __WAIT_H__

#include <freertos/os.h>
#include "waittypes.h"
#include "console.h"
#include <FreeRTOS.h>
#include <list.h>
#include <task.h>

#define WAIT_DEBUG 0
#define wait_dprintk if (WAIT_DEBUG) printk

static inline void wake_up_from_isr(xList *q)
{
	while (listLIST_IS_EMPTY(q) == pdFALSE) {
		wait_dprintk("wake_up_from_isr: waking up a task from queue 0x%x\n", q);
		xTaskRemoveFromEventList(q);
	}
}

static inline void wake_up(xList *q)
{
	__disable_irq();
	wake_up_from_isr(q);
	__enable_irq();
}

#define wait_on(wq) do {                                        \
    __disable_irq(); \
	wait_dprintk("wait_on: adding task to queue 0x%x\n", wq); \
    vTaskPlaceOnEventList(wq, portMAX_DELAY);                   \
    __enable_irq(); \
    portYIELD();                                                \
} while (0)

#define wait_event(wq, condition) do {       \
    for(;;)                                                     \
    {                                                           \
        if(condition)                                           \
            break;                                              \
	wait_dprintk("wait_event: condition false, putting task to sleep on queue 0x%x\n", wq); \
        __disable_irq(); \
     	vTaskPlaceOnEventList(wq, 0);                    \
        __enable_irq(); \
	taskYIELD();                                            \
    }                                                           \
} while(0) 

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
