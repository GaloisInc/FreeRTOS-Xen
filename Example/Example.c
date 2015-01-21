
#include <stdint.h>

#include <FreeRTOS.h>
#include <string.h>
#include <task.h>
#include <platform/libIVC.h>
#include <platform/ivcdev.h>
#include <platform/console.h>
#include <platform/wait.h>

void firstTask(void *unused)
{
    printk("In first task\n");
    vTaskDelete(NULL);
}

void secondTask(void *unused)
{
    printk("In second task\n");
    vTaskDelete(NULL);
}

int main(void)
{
	portBASE_TYPE ret;

	printk("Creating first task\n");
	ret = xTaskCreate(firstTask, (signed portCHAR *) "firstTask", 4096, NULL, configTIMER_TASK_PRIORITY - 1, NULL);
	if (ret != pdPASS) {
		printk("Error creating task, status was %d\n", ret);
		return 1;
	}

	printk("Creating second task\n");
	ret = xTaskCreate(secondTask, (signed portCHAR *) "secondTask", 4096, NULL, configTIMER_TASK_PRIORITY - 1, NULL);
	if (ret != pdPASS) {
		printk("Error creating task, status was %d\n", ret);
		return 1;
	}

	printk("Starting scheduler\n");
	vTaskStartScheduler();

	printk("Scheduler exited\n");
	return 0;
}
