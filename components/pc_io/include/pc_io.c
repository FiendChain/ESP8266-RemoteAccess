#include "pc_io.h"
#include "pc_io_interrupt.h"

#include "driver/gpio.h"

#include "FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "esp_log.h"

#define POWER_SW_PIN 5
#define POWER_SW_FUNC FUNC_GPIO5

#define RESET_SW_PIN 4
#define RESET_SW_FUNC FUNC_GPIO4

#define POWER_STATUS_PIN 12
#define POWER_STATUS_FUNC FUNC_GPIO12

#define GPIO_PIN_TO_MASK(x) (1 << x)

#define TAG "pc-io"

static TimerHandle_t power_on_timer = NULL;
static TimerHandle_t reset_timer = NULL;
static TimerHandle_t power_off_timer = NULL;

static void pc_io_power_on_task(TimerHandle_t handle);
static void pc_io_reset_task(TimerHandle_t handle);
static void pc_io_power_off_task(TimerHandle_t handle);

static bool pc_io_busy = false;

static esp_err_t start_pc_io_timer(TimerHandle_t handle);

void pc_io_init() {
    ESP_LOGD(TAG, "Initialising pc io");

    PIN_FUNC_SELECT(PERIPHS_GPIO_MUX_REG(POWER_SW_PIN), POWER_SW_FUNC);    
    PIN_FUNC_SELECT(PERIPHS_GPIO_MUX_REG(RESET_SW_PIN), RESET_SW_FUNC);    
    PIN_FUNC_SELECT(PERIPHS_GPIO_MUX_REG(POWER_STATUS_PIN), POWER_STATUS_FUNC);    
    ESP_LOGD(TAG, "Initialised pc io pin functions");

    gpio_set_direction(POWER_SW_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(POWER_SW_PIN, 0);
    gpio_set_direction(RESET_SW_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(RESET_SW_PIN, 0);
    gpio_set_direction(POWER_STATUS_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(POWER_STATUS_PIN, GPIO_PULLDOWN_ONLY);
    // ISR for power status
    pc_io_interrupt_init();
    gpio_set_intr_type(POWER_STATUS_PIN, GPIO_INTR_ANYEDGE);
    gpio_install_isr_service(0);
    if (gpio_isr_handler_add(POWER_STATUS_PIN, pc_io_status_interrupt, NULL)) {
        ESP_LOGE(TAG, "Failed to setup ISR for pc status");
    } else {
        ESP_LOGI(TAG, "Successfully register ISR for pc status");
    }
    ESP_LOGD(TAG, "Finished setting up gpio");

    // create async timers for tasks
    power_on_timer = xTimerCreate("power-on-timer", 0, pdFALSE, NULL, pc_io_power_on_task);
    reset_timer = xTimerCreate("reset-timer", 0, pdFALSE, NULL, pc_io_reset_task);
    power_off_timer = xTimerCreate("power-off-timer", 0, pdFALSE, NULL, pc_io_power_off_task);

    ESP_LOGD(TAG, "Finished initialising pc io");
}

esp_err_t start_pc_io_timer(TimerHandle_t handle) {
    if (!pc_io_busy) {
        pc_io_busy = true;
        xTimerStart(handle, 0);
        ESP_LOGI(TAG, "pc io executed timer");
        return ESP_OK;
    }
    ESP_LOGI(TAG, "pc io busy");
    return ESP_FAIL;
}

esp_err_t pc_io_power_on() {
    return start_pc_io_timer(power_on_timer);
}

esp_err_t pc_io_reset() {
    return start_pc_io_timer(reset_timer);
}

esp_err_t pc_io_power_off() {
    return start_pc_io_timer(power_off_timer);
}

bool pc_io_is_powered() {
    return gpio_get_level(POWER_STATUS_PIN);
}

void pc_io_power_on_task(TimerHandle_t handle) {
    gpio_set_level(POWER_SW_PIN, 1);
    vTaskDelay(100 / portTICK_RATE_MS);
    gpio_set_level(POWER_SW_PIN, 0);
    pc_io_busy = false;
}

void pc_io_reset_task(TimerHandle_t handle) {
    gpio_set_level(RESET_SW_PIN, 1);
    vTaskDelay(100 / portTICK_RATE_MS);
    gpio_set_level(RESET_SW_PIN, 0);
    pc_io_busy = false;
}

void pc_io_power_off_task(TimerHandle_t handle) {
    gpio_set_level(POWER_SW_PIN, 1);
    vTaskDelay(6000 / portTICK_RATE_MS);
    gpio_set_level(POWER_SW_PIN, 0);
    pc_io_busy = false;
}

