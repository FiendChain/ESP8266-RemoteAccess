#ifndef __PC_IO_H__
#define __PC_IO_H__

#include <stdbool.h>
#include "esp_err.h"

void pc_io_init();
esp_err_t pc_io_power_on();
esp_err_t pc_io_power_off();
esp_err_t pc_io_reset();
bool pc_io_is_powered();

#endif
