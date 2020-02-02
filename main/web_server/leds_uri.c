#include "leds_uri.h"
#include "../shifted_pwm.h"

#include "cJSON.h"
#include "esp_log.h"


#define MIN(x, y) ((x > y) ? y : x)

#define SCRATCH_BUFSIZE (10240) // for post buffering?
#define TAG "leds-uri"

static char scratch_buffer[SCRATCH_BUFSIZE] = {0};

esp_err_t leds_post_handler(httpd_req_t *req) {
    int total_length = req->content_len;

    if (total_length >= SCRATCH_BUFSIZE) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    char *buffer = scratch_buffer;

    int total_read = 0;
    int status = 0;
    while (total_read < total_length) {
        // try to one shot get all the data - unlikely
        status = httpd_req_recv(req, &buffer[total_read], total_length);
        if (status <= 0) {
            if (status == HTTPD_SOCK_ERR_TIMEOUT) continue; // timeout retry
            httpd_resp_send_500(req);
            return ESP_FAIL; // other codes are failure
        }
        total_read += status;
    }
    buffer[total_length] = '\0'; // null terminate for cjson

    cJSON *root_json = cJSON_Parse(buffer);
    if (root_json == NULL) { 
        httpd_resp_send_408(req);
        return ESP_FAIL;
    }

    char *json_string = cJSON_Print(root_json);
    ESP_LOGI(TAG, "data: %s", json_string);
    free(json_string);

    const cJSON *leds_json = cJSON_GetObjectItem(root_json, "leds");
    const cJSON *led_json = NULL;
    cJSON_ArrayForEach(led_json, leds_json) {
        cJSON *pin_json = cJSON_GetObjectItem(led_json, "pin");
        cJSON *value_json = cJSON_GetObjectItem(led_json, "value");

        if (!cJSON_IsNumber(pin_json) || !cJSON_IsNumber(value_json)) continue;

        int pin = pin_json->valueint;
        int value = value_json->valueint;

        if (pin < 0 || pin >= MAX_PWM_PINS || value < 0) continue;
        set_pwm_value(pin, value);
        ESP_LOGI(TAG, "Remote set pwm %d to %d", pin, value);
    }


    cJSON_Delete(root_json);
    static const char *success_message = "Successfully sent leds config";
    httpd_resp_send(req, success_message, strlen(success_message));

    return ESP_OK;
}

esp_err_t leds_get_handler(httpd_req_t *req) {
    cJSON *root_json = cJSON_CreateObject();
    cJSON *leds_json = cJSON_CreateArray();
    cJSON_AddItemToObject(root_json, "leds", leds_json);

    cJSON *pin_json = NULL;
    cJSON *value_json = NULL;
    cJSON *led_json = NULL;

    for (int pin = 0; pin < MAX_PWM_PINS; pin++) {
        int value = get_pwm_value(pin);
        led_json = cJSON_CreateObject();
        cJSON_AddItemToArray(leds_json, led_json);
        pin_json = cJSON_CreateNumber(pin);
        cJSON_AddItemToObject(led_json, "pin", pin_json);
        value_json = cJSON_CreateNumber(value);
        cJSON_AddItemToObject(led_json, "value", value_json);
    }

    char *json_string = cJSON_Print(root_json);
    httpd_resp_send(req, json_string, strlen(json_string));
    free(json_string);
    cJSON_Delete(root_json);

    return ESP_OK;
}