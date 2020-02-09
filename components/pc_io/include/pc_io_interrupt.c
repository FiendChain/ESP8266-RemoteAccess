#include "pc_io.h"
#include "pc_io_interrupt.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

static xQueueHandle event_queue = NULL;
static void pc_io_interrupt_task(void *arg);

void pc_io_interrupt_init() {
    event_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(pc_io_interrupt_task, "pc-io-int-task", 2048, NULL, 10, NULL);
}

void pc_io_status_interrupt(void *ignore) {
    uint32_t data = 0;
    xQueueSendFromISR(event_queue, &data, NULL);
}

void pc_io_interrupt_task(void *arg) {
    uint32_t buffer;
    while (1) {
        if (xQueueReceive(event_queue, &buffer, portMAX_DELAY)) {
            ESP_LOGI("pc-io-int", "PC status: %s", pc_io_is_powered() ? "on" : "off"); 
        }
    }
}
