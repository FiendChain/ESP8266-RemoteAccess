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
#include "dht11.h"

#include "websocket.h"
#include "websocket_io.h"

// #include <dht/dht.h>

#define INIT_TAG "initialisation"

static ICACHE_FLASH_ATTR void get_websocket(httpd_req_t *request, uint8_t opcode, uint8_t *data, int length);

static httpd_handle_t server = NULL;
static httpd_handle_t websocket = NULL;

    
static httpd_uri_t websocket_uri = {
    .uri = "/api/v1/websocket",
    .method = HTTP_GET,
    .handler = websocket_handler,
    .user_ctx = get_websocket
};

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
    dht11_init();

    for (int i = 0; i < 8; i++) {
        set_pwm_value(i, i << 5);
    }

    server = start_webserver();
    websocket = start_websocket(8200);
    httpd_register_uri_handler(websocket, &websocket_uri);
    
    // vTaskStartScheduler();
    // ESP_LOGI(INIT_TAG, "Starting task scheduler!\n");
    ESP_LOGI(INIT_TAG, "Finished initialisation!");
}

void ICACHE_FLASH_ATTR get_websocket(httpd_req_t *request, uint8_t opcode, uint8_t *data, int length) {
    if (opcode == WEBSOCKET_OPCODE_TEXT) {
        ESP_LOGI("websocket-listener", "%.*s", length, data);
    }
    for (int i = 1; i < length; i=i+2) {
        uint8_t pin = data[i-1];
        uint8_t value = data[i];
        ESP_LOGI("websocket-listener", "pin %u %u", pin, value);
        set_pwm_value(pin, value);
        if (pin >= MAX_PWM_PINS) continue;
    }
    // websocket_write(request, (char *)data, length, opcode);
}