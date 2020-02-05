#ifndef __WEBSOCKET_H__
#define __WEBSOCKET_H__

#include <esp_http_server.h>

typedef esp_err_t (*websocket_recieve_callback) (httpd_req_t *, uint8_t *, int);
httpd_handle_t start_websocket(uint32_t port);

#endif