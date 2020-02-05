#include "websocket.h"
#include "websocket_io.h"
#include "websocket_handshake.h"

#include <esp_http_server.h>
#include <esp_httpd_priv.h>
#include <esp_log.h>

#include <string.h>
#include <mbedtls/sha1.h>
#include <mbedtls/base64.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAG "websocket-io"

#define PROTOCOL_BUFFER_SIZE 125
// #define MIN(x, y) ((x > y) ? y : x)

static uint8_t write_buffer[PROTOCOL_BUFFER_SIZE+2] = {0};
static uint8_t read_buffer[PROTOCOL_BUFFER_SIZE+2] = {0};

static esp_err_t websocket_read_data(httpd_req_t *request);

esp_err_t websocket_write(httpd_req_t *request, char *data, int _length) {
    uint8_t length = MIN(PROTOCOL_BUFFER_SIZE, _length);
    write_buffer[0] = 0x80 | 0x01;
    write_buffer[1] = length;
    memcpy(&write_buffer[2], data, length);

    if (httpd_send(request, (char *)write_buffer, length+2) <= 0) {
        ESP_LOGI(TAG, "Closing websocket!");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t websocket_handler(httpd_req_t *request) {
    if (validate_websocket_request(request) != ESP_OK) {
        ESP_LOGE(TAG, "Failed validation");
        return ESP_OK;
    } 

    if (perform_websocket_handshake(request) != ESP_OK) {
        ESP_LOGE(TAG, "Failed handshake");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Starting websocket");
    while (1) {
        if (websocket_read_data(request) != ESP_OK) {
            break;
        }
        vTaskDelay(1);
    }
    ESP_LOGI(TAG, "Closing websocket");

    return ESP_OK;
}

esp_err_t websocket_read_data(httpd_req_t *request) {
    websocket_recieve_callback callback = (websocket_recieve_callback)(request->user_ctx);
    if (callback == NULL) {
        return ESP_OK;
    }

    int total_data = httpd_recv_with_opt(request, (char *)read_buffer, sizeof(read_buffer), false);
    if (total_data >= 7) {            
        uint8_t opcode = read_buffer[0] & 0x7F;
        // unmask
        switch (opcode) {
        case 0x01: // binary
        case 0x02: // text
            if (total_data > 6) {
                total_data -= 6;
                for (int i = 0; i < total_data; i++) {
                    read_buffer[i+6] ^= read_buffer[2 + i % 4];
                }
                int8_t length = read_buffer[1] + 128;
                callback(request, &read_buffer[6], length);
            }
            break;
        case 0x08:
            return ESP_FAIL;
        }

    }
    return ESP_OK;
}