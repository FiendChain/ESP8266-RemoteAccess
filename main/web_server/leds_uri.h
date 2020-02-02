#ifndef __LEDS_URI_H__
#define __LEDS_URI_H__

#include "web_server.h"
#include "esp_log.h" 
#include "esp_http_server.h"

esp_err_t leds_post_handler(httpd_req_t *req);
esp_err_t leds_get_handler(httpd_req_t *req);

#endif