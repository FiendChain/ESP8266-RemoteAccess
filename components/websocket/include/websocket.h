#ifndef __WEBSOCKET_H__
#define __WEBSOCKET_H__

#include <esp_http_server.h>

#define WEBSOCKET_OPCODE_TEXT 0x01
#define WEBSOCKET_OPCODE_BIN 0x02

typedef esp_err_t (*websocket_recieve_callback) (httpd_req_t *, uint8_t opcode, uint8_t *, int);
httpd_handle_t start_websocket(uint32_t port);

#endif