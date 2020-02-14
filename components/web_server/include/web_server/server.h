
#ifndef __WEBSERVER_H__ 
#define __WEBSERVER_H__

#include <esp_http_server.h>

httpd_handle_t start_webserver(uint32_t port);

#endif
    