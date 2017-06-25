#ifndef ONEWIRE_H_
#define ONEWIRE_H_

#include "stm32f10x.h"

#define OW_SEND_RESET	1
#define OW_NO_RESET		2

#define OW_OK			1
#define OW_ERROR		2
#define OW_NO_DEVICE	3

#define OW_NO_READ		0xff
#define OW_READ_SLOT	0xff

typedef struct
{
    uint8_t rom_code[8];
    uint8_t id;
}owdevice_t;

uint8_t OW_Init(void);
uint8_t OW_Reset(void);
uint8_t OW_Search(owdevice_t *owdevices);
uint8_t OW_Send(uint8_t sendReset, uint8_t *command, uint8_t cLen, uint8_t *data, uint8_t dLen, uint8_t readStart);
void OW_MatchRom(uint8_t *romValue);

#endif /* ONEWIRE_H_ */
