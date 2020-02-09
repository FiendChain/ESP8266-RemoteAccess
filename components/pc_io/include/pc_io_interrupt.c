#include "pc_io.h"
#include "pc_io_interrupt.h"

#include <stdlib.h>
#include <stdbool.h>

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#define LL_TAG "pc-io-linked-list"

typedef struct pc_io_status_listener_node {
    pc_io_status_listener_t listener;
    void *args;
    struct pc_io_status_listener_node *next;
} pc_io_status_listener_node;

static pc_io_status_listener_node *listeners = NULL; 
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
            bool is_powered = pc_io_is_powered();
            pc_io_status_listener_node *head = listeners;
            int i = 0;
            while (head != NULL) {
                ESP_LOGD("pc-io-linked-list", "calling %d", i++);
                head->listener(is_powered, head->args);
                head = head->next;
            }
        }
    }
}

esp_err_t pc_io_status_listen(pc_io_status_listener_t listener, void *args) {
    if (listener == NULL) {
        return ESP_FAIL;
    }

    pc_io_status_listener_node **head = &listeners;
    pc_io_status_listener_node *new_node = malloc(sizeof(pc_io_status_listener_node));
    new_node->listener = listener;
    new_node->args = args;
    new_node->next = *head;
    *head = new_node;

    ESP_LOGD(LL_TAG, "added node %p, set head to %p", new_node, *head);

    return ESP_OK;
}

esp_err_t pc_io_status_unlisten(pc_io_status_listener_t listener, void *args) {
    if (listener == NULL) {
        return ESP_FAIL;
    }

    pc_io_status_listener_node **head = &listeners;
    while (*head != NULL) {
        pc_io_status_listener_node *node = *head;
        if (node->listener == listener && node->args == args) {
            *head = node->next;
            ESP_LOGD(LL_TAG, "freed node %p, set head to %p", node, *head);
            free(node);
            return ESP_OK;
        }
        head = &(node->next);
    }
    return ESP_FAIL;
}
