#ifndef __DHT11_URI_H__
#define __DHT11_URI_H__

#include "web_server.h"
#include "esp_log.h" 
#include "esp_http_server.h"

esp_err_t dht11_get_handler(httpd_req_t *req);

#endif