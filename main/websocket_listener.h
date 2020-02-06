#ifndef __WEBSOCKET_LISTENER_H__
#define __WEBSOCKET_LISTENER_H__

#include "websocket.h"
#include "websocket_io.h"

void ICACHE_FLASH_ATTR listen_websocket(httpd_req_t *request, uint8_t opcode, uint8_t *data, int length);

#endif