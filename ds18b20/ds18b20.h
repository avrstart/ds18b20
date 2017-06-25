#ifndef DS18X20_H_
#define DS18X20_H_

#include "include.h"

#define MAX_SENSORS    3

uint8_t ds18b20_start_convert(void);
uint8_t ds18b20_init(void);
float ds18b20_get_temp(uint8_t dev_id);

float ds18b20_tconvert(uint8_t LSB, uint8_t MSB);

#endif
