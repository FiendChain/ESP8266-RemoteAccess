#include "web_server.h"
#include "esp_log.h"
#include "esp_http_server.h"

#include "pc_io_uri.h"
#include "leds_uri.h"
#include "dht11_uri.h"

#include <string.h>
#include <stdbool.h>

#define TAG "simple-web-server"

const httpd_uri_t leds_post_uri = {
    .uri = "/api/v1/leds",
    .method = HTTP_POST,
    .handler = leds_post_handler,
    .user_ctx = NULL,
};

const httpd_uri_t leds_get_uri = {
    .uri = "/api/v1/leds",
    .method = HTTP_GET,
    .handler = leds_get_handler,
    .user_ctx = NULL,
};

const httpd_uri_t pc_io_post_uri = {
    .uri = "/api/v1/pc",
    .method = HTTP_POST,
    .handler = pc_io_post_handler,
    .user_ctx = NULL,
};

const httpd_uri_t pc_io_get_uri = {
    .uri = "/api/v1/pc",
    .method = HTTP_GET,
    .handler = pc_io_get_handler,
    .user_ctx = NULL,
};

const httpd_uri_t dht11_get_url = {
    .uri = "/api/v1/dht11",
    .method = HTTP_GET,
    .handler = dht11_get_handler,
    .user_ctx = NULL,
};

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &leds_post_uri);
        httpd_register_uri_handler(server, &leds_get_uri);
        httpd_register_uri_handler(server, &pc_io_get_uri);
        httpd_register_uri_handler(server, &pc_io_post_uri);
        httpd_register_uri_handler(server, &dht11_get_url);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}



