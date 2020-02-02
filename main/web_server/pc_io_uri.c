#include "pc_io_uri.h"
#include "../pc_io.h"

#include "cJSON.h"
#include "esp_log.h"

#define TAG "pc-io-uri"

esp_err_t pc_io_get_handler(httpd_req_t *req) {
    cJSON *root_json = cJSON_CreateObject();
    bool is_powered = pc_io_is_powered();
    cJSON *is_powered_json = cJSON_CreateBool(is_powered);
    cJSON_AddItemToObject(root_json, "is_powered", is_powered_json);

    char *json_string = cJSON_Print(root_json); 
    httpd_resp_send(req, json_string, strlen(json_string));
    free(json_string);
    cJSON_Delete(root_json);

    return ESP_OK;
}

esp_err_t pc_io_post_handler(httpd_req_t *req) {
    int buffer_length = httpd_req_get_url_query_len(req) + 1;
    if (buffer_length < 1) {
        httpd_resp_send_408(req);
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Got query string of length %d", buffer_length);

    char *buffer = malloc(buffer_length);
    esp_err_t status = httpd_req_get_url_query_str(req, buffer, buffer_length);
    buffer[buffer_length-1] = '\0';
    ESP_LOGI(TAG, "Got pc io: %s", buffer);

    static const char *busy_message = "pc io busy";
    static const char *invalid_command = "invalid pc io command";

    if (status != ESP_OK) {
        httpd_resp_send_408(req);
    } else {
        esp_err_t pc_io_status = ESP_OK;
        bool is_valid_command = true;
        if (strncmp(buffer, "on", 2) == 0) {
            pc_io_status = pc_io_power_on();
        } else if (strncmp(buffer, "off", 3) == 0) {
            pc_io_status = pc_io_power_off();
        } else if (strncmp(buffer, "reset", 5) == 0) {
            pc_io_status = pc_io_reset();
        } else {
            is_valid_command = false;
        }

        if (is_valid_command) {
            if (pc_io_status == ESP_OK) httpd_resp_send(req, buffer, buffer_length);
            else                        httpd_resp_send(req, busy_message, strlen(busy_message));
        } else {
            httpd_resp_send(req, invalid_command, strlen(invalid_command));
        }
    }

    free(buffer);

    return ESP_OK;
}