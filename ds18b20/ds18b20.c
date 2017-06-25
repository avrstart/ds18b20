#include "onewire.h"
#include "ds18b20.h"

static owdevice_t ds18_sensors[MAX_SENSORS];       //rom code
static uint8_t owdevices = 0;                      //devices index

uint8_t ds18b20_crc8(uint8_t *addr, uint8_t len) {
	uint8_t crc = 0, inbyte, i, mix;
	
	while (len--) {
		inbyte = *addr++;
		for (i = 8; i; i--) {
			mix = (crc ^ inbyte) & 0x01;
			crc >>= 1;
			if (mix) {
				crc ^= 0x8C;
			}
			inbyte >>= 1;
		}
	}
	return crc;
}

uint8_t ds18b20_init(void)
{
    if(!OW_Init()) {
        return 0;
    }
    while(OW_Search(ds18_sensors)) {       
        if(ds18b20_crc8(ds18_sensors[owdevices].rom_code, 7) == ds18_sensors[owdevices].rom_code[7]) {
            owdevices++;
        }
    }

    return owdevices;
}

uint8_t ds18b20_start_convert(void) 
{
    uint8_t send_buf[2] = {0xcc, 0x44};
    OW_Send(OW_SEND_RESET, send_buf, sizeof(send_buf), NULL, NULL, OW_NO_READ);
    
    return 0;
}

float ds18b20_get_temp(uint8_t dev_id)
{   
    uint8_t fbuf[2];
    uint8_t send_buf[12];
    
    float temp;
    
    send_buf[0] = 0x55;
    memcpy(&send_buf[1], ds18_sensors[dev_id].rom_code, 8);
    send_buf[9] = 0xbe;
    send_buf[10] = 0xff;
    send_buf[11] = 0xff;
    
    OW_Send(OW_SEND_RESET, send_buf, sizeof(send_buf), fbuf, sizeof(fbuf), 10);

    temp = ds18b20_tconvert(fbuf[0], fbuf[1]);
    return temp;
}

float ds18b20_tconvert(uint8_t LSB, uint8_t MSB)
{
    float data;
    
    uint16_t temperature;
    
    temperature = LSB | (MSB << 8);

	if (temperature & 0x8000) {
		temperature = ~temperature + 1;
        data = 0.0 - (temperature / 16.0);
        return data;
	}
    data = temperature / 16.0;
    
    return data ;
}

