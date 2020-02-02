/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "rom/ets_sys.h"

#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/uart.h"

#include "nvs_flash.h"

#include "shifted_pwm.h"
#include "wifi_sta.h"
#include "web_server/web_server.h"
#include "pc_io.h"

#define INIT_TAG "initialisation"
#define LED_GPIO 2

void ICACHE_FLASH_ATTR blink(void *ignore);
void ICACHE_FLASH_ATTR update_pwm(void *ignore);


static httpd_handle_t server = NULL;

void app_main()
{
    ESP_LOGI(INIT_TAG, "Entering main function!\n");
    shifted_pwm_init();

    ESP_LOGI(INIT_TAG, "Starting NVS!\n");
    esp_err_t nvs_status = nvs_flash_init();
    if (nvs_status == ESP_ERR_NVS_NO_FREE_PAGES) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    wifi_init_sta();
    pc_io_init();

    PIN_FUNC_SELECT(PERIPHS_GPIO_MUX_REG(2), FUNC_GPIO2);
    xTaskCreate(&blink, "ledblink", 256, NULL, 1, NULL);
    xTaskCreate(&update_pwm, "pwm_update", 256, NULL, 1, NULL);
    for (int i = 0; i < 8; i++) {
        set_pwm_value(i, i << 5);
    }

    server = start_webserver();
    
    // vTaskStartScheduler();
    // ESP_LOGI(INIT_TAG, "Starting task scheduler!\n");
    ESP_LOGI(INIT_TAG, "Finished initialisation!");
}

void ICACHE_FLASH_ATTR blink(void *ignore) {
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    while (1) {
        gpio_set_level(GPIO_NUM_2, 0);
        vTaskDelay(500 / portTICK_RATE_MS);
        gpio_set_level(GPIO_NUM_2, 1);
        vTaskDelay(500 / portTICK_RATE_MS);
    }

    vTaskDelete(NULL);
}

void ICACHE_FLASH_ATTR update_pwm(void *ignore) {
    uint8_t is_rising = 1;
    const uint8_t step_size = 8; 

    while (1) {
        int16_t pwm = get_pwm_value(0);
        if (is_rising) {
            pwm += step_size;
            if (pwm >= MAX_PWM_CYCLES) {
                pwm = MAX_PWM_CYCLES;
                is_rising = 0;
            }
        } else {
            pwm -= step_size;
            if (pwm <= 0) {
                pwm = 0;
                is_rising = 1;
            }
        }

        set_pwm_value(0, pwm & 0xFF);
        vTaskDelay(100 / portTICK_RATE_MS);
    }

    vTaskDelete(NULL);
}