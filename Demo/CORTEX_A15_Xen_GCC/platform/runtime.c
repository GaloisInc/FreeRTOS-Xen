
#include <stdint.h>
#include <runtime_reqs.h>
#include <platform/console.h>
#include <platform/hypercalls.h>
#include <freertos/os.h>
#include <FreeRTOS.h>
#include <task.h>

void *pvPortMalloc(size_t);

void runtime_write(size_t len, char *buffer)
{
	(void)HYPERVISOR_console_io(CONSOLEIO_WRITE, len, buffer);
}

void runtime_exit(void)
{
	printk("<<< FreeRTOS exit >>>\n");
	vTaskSuspendAll();
	taskDISABLE_INTERRUPTS();
	BUG();
}

inline void *runtime_libc_malloc(size_t len)
{
	return pvPortMalloc(len);
}
