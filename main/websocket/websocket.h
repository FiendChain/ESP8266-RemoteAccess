#ifndef __WEBSOCKET_H__
#define __WEBSOCKET_H__

#include <esp_http_server.h>

httpd_handle_t start_websocket(uint32_t port);

#endif