/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "driver/uart.h"

#include "shifted_pwm.h"

#define LED_GPIO 2

void ICACHE_FLASH_ATTR blink(void *ignore) {
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);

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

    while (1) {
        int16_t pwm = get_pwm_value(0);
        if (is_rising) {
            pwm += 1;
            if (pwm >= MAX_PWM_CYCLES) {
                pwm = MAX_PWM_CYCLES;
                is_rising = 0;
            }
        } else {
            pwm -= 1;
            if (pwm <= 0) {
                pwm = 0;
                is_rising = 1;
            }
        }

        set_pwm_value(0, pwm & 0xFF);
        os_delay_us(1000);
    }

    vTaskDelete(NULL);
}

void app_main()
{
    printf("Entering main function!\n");
    shifted_pwm_init();

    PIN_FUNC_SELECT(PERIPHS_GPIO_MUX_REG(2), FUNC_GPIO2);
    xTaskCreate(&blink, "ledblink", 256, NULL, 1, NULL);
    xTaskCreate(&update_pwm, "pwm_update", 2048, NULL, 1, NULL);
    for (int i = 0; i < 8; i++) {
        set_pwm_value(i, i << 5);
    }
}
