#include "dht11_uri.h"

#include "dht11.h"
#include "cJSON.h"
#include "esp_log.h"


esp_err_t dht11_get_handler(httpd_req_t *req) {
    cJSON *root_json = cJSON_CreateObject();
    if (dht11_read() == ESP_OK) {
        cJSON_AddNumberToObject(root_json, "temperature", dht11_get_temperature());
        cJSON_AddNumberToObject(root_json, "humidity", dht11_get_humidity());
        cJSON_AddBoolToObject(root_json, "success", true);
    } else {
        ESP_LOGE("dht11", "Failed to read dht11 remotely");
        cJSON_AddBoolToObject(root_json, "success", false);
    }

    char *json_string = cJSON_Print(root_json);
    httpd_resp_send(req, json_string, strlen(json_string));
    free(json_string);
    cJSON_Delete(root_json);
    
    return ESP_OK;
}