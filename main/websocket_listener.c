#include "websocket_listener.h"

#include "shifted_pwm.h"
#include "pc_io.h"
#include "dht11.h"

void ICACHE_FLASH_ATTR listen_websocket(httpd_req_t *request, uint8_t opcode, uint8_t *data, int length) {
    if (opcode == WEBSOCKET_OPCODE_TEXT) {
        ESP_LOGI("websocket-listener", "%.*s", length, data);
    }
    for (int i = 1; i < length; i=i+2) {
        uint8_t pin = data[i-1];
        uint8_t value = data[i];
        // ESP_LOGI("websocket-listener", "pin %u %u", pin, value);
        set_pwm_value(pin, value);
        if (pin >= MAX_PWM_PINS) continue;
    }
    // websocket_write(request, (char *)data, length, opcode);
}