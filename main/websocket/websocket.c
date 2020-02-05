#include "websocket.h"

#include <esp_http_server.h>
#include <esp_httpd_priv.h>
#include <esp_log.h>

#include <string.h>
#include <mbedtls/sha1.h>
#include <mbedtls/base64.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAG "websocket"
#define BUFFER_SIZE 100

static esp_err_t websocket_handler(httpd_req_t *request);
static esp_err_t validate_websocket_request(httpd_req_t *request);
static esp_err_t perform_websocket_handshake(httpd_req_t *request);
static esp_err_t websocket_listener(void *pvParameters);
static esp_err_t websocket_callback(httpd_req_t *request, char *data, int length);

static const char *RFC6455_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

static char buffer[BUFFER_SIZE] = {0};

static httpd_uri_t websocket_uri = {
    .uri = "/api/v1/websocket",
    .method = HTTP_GET,
    .handler = websocket_handler,
    .user_ctx = NULL
};

httpd_handle_t start_websocket(uint32_t port) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    config.ctrl_port = 32767;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering websocket");
        httpd_register_uri_handler(server, &websocket_uri);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
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

    char send_buffer[100] = {0};

    ESP_LOGI(TAG, "Starting data transmission");

    xTaskHandle listener_handle;
    xTaskCreate(websocket_listener, "websocket-listener", 2048, request, 2, &listener_handle);

    for (int i = 0; ; i++) {
        uint8_t total = snprintf(&send_buffer[2], sizeof(send_buffer)-2, "Hello there %d", i);
        send_buffer[0] = 0x80 | 0x01;
        send_buffer[1] = total;
        if (httpd_send(request, send_buffer, total+2) <= 0) {
            ESP_LOGI(TAG, "Closing websocket!");
            break;
        }
        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    vTaskDelete(listener_handle);

    return ESP_OK;
}

esp_err_t websocket_listener(void *pvParameters) {
    httpd_req_t *request = (httpd_req_t *)pvParameters;
    ESP_LOGI(TAG, "Starting data listener");
    char recieve_buffer[100] = {0};

    while (1) {
        int total_data = httpd_recv_with_opt(request, recieve_buffer, sizeof(recieve_buffer), false);
        if (total_data >= 7) {            
            uint8_t opcode = recieve_buffer[0] & 0x7F;
            // unmask
            switch (opcode) {
            case 0x01: // binary
            case 0x02: // text
                if (total_data > 6) {
                    total_data -= 6;
                    for (int i = 0; i < total_data; i++) {
                        recieve_buffer[i+6] ^= recieve_buffer[2 + i % 4];
                    }
                    int8_t length = recieve_buffer[1] + 128;
                    websocket_callback(request, &recieve_buffer[6], length);
                }
                break;
            case 0x08:
                break;
            }

        }
        vTaskDelay(1);
    }

    vTaskDelete(NULL);
    return ESP_OK;
}

esp_err_t websocket_callback(httpd_req_t *request, char *data, int length) {
    ESP_LOGI(TAG, "Got unmasked data: %.*s", length, data);
    return ESP_OK;
}

esp_err_t perform_websocket_handshake(httpd_req_t *request) {

    // copy socket key and concatenate guid
    // calculate SHA-1 hash, then encode in base 64
    // already validated header
    if (httpd_req_get_hdr_value_str(request, "Sec-WebSocket-Key", buffer, BUFFER_SIZE) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get web socket key");
        httpd_resp_set_status(request, HTTPD_400);
        httpd_resp_send(request, "Web socket key missing", -1);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Got websocket key %s", buffer);

    strncat(buffer, RFC6455_GUID, BUFFER_SIZE);
    int key_length = strnlen(buffer, BUFFER_SIZE);
    ESP_LOGI(TAG, "Concantenated key as: %s", buffer);
    // sha1
    static uint8_t sha1_sum[20] = {0};
    mbedtls_sha1((unsigned char *)buffer, key_length, sha1_sum);
    // base64
    static unsigned char encoded_key[100] = {0};
    size_t encoded_key_length = 0;
    int status = mbedtls_base64_encode(encoded_key, sizeof(encoded_key), &encoded_key_length, sha1_sum, 20);
    if (status != 0) {
        ESP_LOGI(TAG, "Failed to calculate base64 encoding");
        httpd_resp_send_500(request);
        return ESP_FAIL;
    }
    encoded_key[encoded_key_length] = '\0';
    ESP_LOGI(TAG, "Got encoded key: %s", encoded_key);

    // response headers
    httpd_resp_set_status(request, "101 Switching Protocols");
    httpd_resp_set_hdr(request, "Upgrade", "websocket");
    httpd_resp_set_hdr(request, "Connection", "Upgrade");
    httpd_resp_set_hdr(request, "Sec-WebSocket-Accept", (char *)encoded_key);
    httpd_resp_send(request, "", 0);

    return ESP_OK;
}

esp_err_t validate_websocket_request(httpd_req_t *request) {
    // validate upgrade header
    if (httpd_req_get_hdr_value_str(request, "Upgrade", buffer, BUFFER_SIZE) != ESP_OK) {
        httpd_resp_set_status(request, HTTPD_400);
        httpd_resp_send(request, "Missing 'Upgrade' header", -1);
        return ESP_FAIL;
    }

    if (strncmp(buffer, "websocket", 9) != 0) {
        ESP_LOGI(TAG, "Invalid upgrade type: %s", buffer);
        httpd_resp_set_status(request, HTTPD_400);
        httpd_resp_send(request, "Invalid upgrade type", -1);
        return ESP_FAIL;
    }

    // validate connection header
    if (httpd_req_get_hdr_value_str(request, "Connection", buffer, BUFFER_SIZE) != ESP_OK) {
        httpd_resp_set_status(request, HTTPD_400);
        httpd_resp_send(request, "Missing 'Connection' header", -1);
        return ESP_FAIL;
    }

    if (strncmp(buffer, "Upgrade", 7) != 0) {
        ESP_LOGI(TAG, "Invalid connection type: %s", buffer);
        httpd_resp_set_status(request, HTTPD_400);
        httpd_resp_send(request, "Invalid connection type", -1);
        return ESP_FAIL;
    }

    if (httpd_req_get_hdr_value_len(request, "Sec-WebSocket-Key") == 0) {
        ESP_LOGI(TAG, "Missing websocket key");
        httpd_resp_set_status(request, HTTPD_400);
        httpd_resp_send(request, "Missing websocket key", -1);
        return ESP_FAIL;
    }


    return ESP_OK;

}