#include "onewire.h"

#define OW_USART 		USART1
#define OW_DMA_CH_RX 	DMA1_Channel5
#define OW_DMA_CH_TX 	DMA1_Channel4
#define OW_DMA_FLAG		DMA1_FLAG_TC5


uint8_t ow_buf[8];

#define OW_0	        0x00
#define OW_1	        0xff
#define OW_R_1	        0xff

#define OW_CMD_SEARCH   0xf0
#define OW_CMD_MATCH    0x55

static uint8_t LastDiscrepancy;       /*!< Search private */
static uint8_t LastDeviceFlag;        /*!< Search private */
static uint8_t DeviceID;
static uint8_t ROM_NO[8];

static void OW_toBits(uint8_t ow_byte, uint8_t *ow_bits) {
	uint8_t i;
	for (i = 0; i < 8; i++) {
		if (ow_byte & 0x01) {
			*ow_bits = OW_1;
		} else {
			*ow_bits = OW_0;
		}
		ow_bits++;
		ow_byte = ow_byte >> 1;
	}
}

static uint8_t OW_toByte(uint8_t *ow_bits) {
	uint8_t ow_byte, i;
	ow_byte = 0;
	for (i = 0; i < 8; i++) {
		ow_byte = ow_byte >> 1;
		if (*ow_bits == OW_R_1) {
			ow_byte |= 0x80;
		}
		ow_bits++;
	}
	return ow_byte;
}

uint8_t OW_Init(void) {  
	GPIO_InitTypeDef GPIO_InitStruct;
	USART_InitTypeDef USART_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO,
        ENABLE);

    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl =
			USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

	USART_Init(OW_USART, &USART_InitStructure);
	USART_Cmd(OW_USART, ENABLE);
    
    USART_HalfDuplexCmd(OW_USART, ENABLE);
    
    if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_9) == 0) {
        return OW_ERROR;
    }

	return OW_OK;
}

uint8_t OW_Reset(void) {
	uint8_t ow_presence;
	USART_InitTypeDef USART_InitStructure;

	USART_InitStructure.USART_BaudRate = 9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_Init(OW_USART, &USART_InitStructure);

	USART_ClearFlag(OW_USART, USART_FLAG_TC);
	USART_SendData(OW_USART, 0xf0);
	while (USART_GetFlagStatus(OW_USART, USART_FLAG_TC) == RESET);

	ow_presence = USART_ReceiveData(OW_USART);

	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl =
			USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	USART_Init(OW_USART, &USART_InitStructure);

	if (ow_presence != 0xf0) {
		return OW_OK;
	}

	return OW_NO_DEVICE;
}

uint8_t OW_Send(uint8_t sendReset, uint8_t *command, uint8_t cLen,
		uint8_t *data, uint8_t dLen, uint8_t readStart) 
{

	if (sendReset == OW_SEND_RESET) {
		if (OW_Reset() == OW_NO_DEVICE) {
			return OW_NO_DEVICE;
		}
	}

	while (cLen > 0) {

		OW_toBits(*command, ow_buf);
		command++;
		cLen--;

		DMA_InitTypeDef DMA_InitStructure;

		DMA_DeInit(OW_DMA_CH_RX);
		DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &(OW_USART->DR);
		DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) ow_buf;
		DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
		DMA_InitStructure.DMA_BufferSize = 8;
		DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
		DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
		DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
		DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
		DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
		DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
		DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
		DMA_Init(OW_DMA_CH_RX, &DMA_InitStructure);

		DMA_DeInit(OW_DMA_CH_TX);
		DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &(OW_USART->DR);
		DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) ow_buf;
		DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
		DMA_InitStructure.DMA_BufferSize = 8;
		DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
		DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
		DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
		DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
		DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
		DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
		DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
		DMA_Init(OW_DMA_CH_TX, &DMA_InitStructure);

		USART_ClearFlag(OW_USART, USART_FLAG_RXNE | USART_FLAG_TC | USART_FLAG_TXE);
		USART_DMACmd(OW_USART, USART_DMAReq_Tx | USART_DMAReq_Rx, ENABLE);
		DMA_Cmd(OW_DMA_CH_RX, ENABLE);
		DMA_Cmd(OW_DMA_CH_TX, ENABLE);

		while (DMA_GetFlagStatus(OW_DMA_FLAG) == RESET);

		DMA_Cmd(OW_DMA_CH_TX, DISABLE);
		DMA_Cmd(OW_DMA_CH_RX, DISABLE);
		USART_DMACmd(OW_USART, USART_DMAReq_Tx | USART_DMAReq_Rx, DISABLE);

        if (readStart == 0 && dLen > 0) {
			*data = OW_toByte(ow_buf);
			data++;
			dLen--;
		} else {
			if (readStart != OW_NO_READ) {
				readStart--;
			}
		}
	}

	return OW_OK;
}
        
static void OW_SendBits(uint8_t num_bits) {
	DMA_InitTypeDef DMA_InitStructure;
    
	DMA_DeInit(OW_DMA_CH_RX);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &(OW_USART->DR);
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) ow_buf;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = num_bits;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(OW_DMA_CH_RX, &DMA_InitStructure);

	DMA_DeInit(OW_DMA_CH_TX);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) &(OW_USART->DR);
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t) ow_buf;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_BufferSize = num_bits;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Low;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(OW_DMA_CH_TX, &DMA_InitStructure);

	USART_ClearFlag(OW_USART, USART_FLAG_RXNE | USART_FLAG_TC | USART_FLAG_TXE);
	USART_DMACmd(OW_USART, USART_DMAReq_Tx | USART_DMAReq_Rx, ENABLE);
	DMA_Cmd(OW_DMA_CH_RX, ENABLE);
	DMA_Cmd(OW_DMA_CH_TX, ENABLE);

	while (DMA_GetFlagStatus(OW_DMA_FLAG) == RESET);

	DMA_Cmd(OW_DMA_CH_TX, DISABLE);
	DMA_Cmd(OW_DMA_CH_RX, DISABLE);
	USART_DMACmd(OW_USART, USART_DMAReq_Tx | USART_DMAReq_Rx, DISABLE);

}

static uint8_t OW_ReadBit(void)
{
    OW_toBits(OW_READ_SLOT, ow_buf);
	OW_SendBits(1);
    
    if(ow_buf[0] == 0xff) {
        return 1;
    }
    else {
        return 0;
    }
}

void OW_MatchRom(uint8_t *romValue)
{
    uint8_t cmd = OW_CMD_MATCH;
    
    OW_Send(OW_SEND_RESET, &cmd, sizeof(cmd), 0, 0, OW_NO_READ);  
    OW_Send(OW_NO_RESET, romValue, 8, 0, 0, OW_NO_READ);
}

uint8_t OW_Search(owdevice_t *owdevices) 
{
    
	uint8_t id_bit_number = 1;
	uint8_t last_zero = 0, rom_byte_number = 0, search_result = 0;
	uint8_t id_bit, cmp_id_bit;
	uint8_t rom_byte_mask = 1, search_direction;
    uint8_t cmd;

	// if the last call was not the last one
	if (!LastDeviceFlag) {
        
        cmd = OW_CMD_SEARCH;
		if (OW_Send(OW_SEND_RESET, &cmd, sizeof(cmd), 0, 0, OW_NO_READ) == OW_NO_DEVICE) {			
			LastDiscrepancy = 0;   /* Reset the search */
			LastDeviceFlag = 0;
            DeviceID = 0;
			return 0;
		}
        
		// loop to do the search
		do {
			// read a bit and its complement
			id_bit = OW_ReadBit();
			cmp_id_bit = OW_ReadBit();

			// check for no devices on 1-wire
			if ((id_bit == 1) && (cmp_id_bit == 1)) {
				break;
			} else {
				// all devices coupled have 0 or 1
				if (id_bit != cmp_id_bit) {
					search_direction = id_bit;  // bit write value for search
				} else {
					// if this discrepancy if before the Last Discrepancy
					// on a previous next then pick the same as last time
					if (id_bit_number < LastDiscrepancy) {
						search_direction = ((ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
					} else {
						// if equal to last pick 1, if not then pick 0
						search_direction = (id_bit_number == LastDiscrepancy);
					}
					
					// if 0 was picked then record its position in LastZero
					if (search_direction == 0) {
						last_zero = id_bit_number;
					}
				}

				// set or clear the bit in the ROM byte rom_byte_number
				// with mask rom_byte_mask
				if (search_direction == 1) {
					ROM_NO[rom_byte_number] |= rom_byte_mask;
				} else {
					ROM_NO[rom_byte_number] &= ~rom_byte_mask;
				}
				
				// serial number search direction write bit
                OW_toBits(search_direction, ow_buf);
                OW_SendBits(1);
                
				// increment the byte counter id_bit_number
				// and shift the mask rom_byte_mask
				id_bit_number++;
				rom_byte_mask <<= 1;

				// if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
				if (rom_byte_mask == 0) {
					//docrc8(ROM_NO[rom_byte_number]);  // accumulate the CRC
					rom_byte_number++;
					rom_byte_mask = 1;
				}
			}
		} while (rom_byte_number < 8);  // loop until through all ROM bytes 0-7

		// if the search was successful then
		if (!(id_bit_number < 65)) {
			// search successful so set LastDiscrepancy,LastDeviceFlag,search_result
			LastDiscrepancy = last_zero;

			// check for last device
			if (LastDiscrepancy == 0) {
				LastDeviceFlag = 1;
			}

			search_result = 1;
            owdevices[DeviceID].id = DeviceID;
            for (int i = 0; i < 8; i++) owdevices[DeviceID].rom_code[i] = ROM_NO[i];
            DeviceID++;
		}
	}

	// if no device found then reset counters so next 'search' will be like a first
	if (!search_result) {
		LastDiscrepancy = 0;
		LastDeviceFlag = 0;
		search_result = 0;
        DeviceID = 0;
	}
    
	return search_result;
}


