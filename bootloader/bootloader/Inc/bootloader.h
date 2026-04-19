#ifndef _BOOTLOADER_H_
#define _BOOTLOADER_H_

/******************************Section : Include********************************************/
#include "stm32f4xx_hal_uart.h"
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal_crc.h"


/******************************Section : Macro definitions********************************************/



/******************************Section : Macro functions********************************************/


/******************************Section :user-defined data types********************************************/
typedef enum
{
    BL_NOK,
    BL_OK
}BOOTLOADER_STATUS_t;


/******************************Section :functions decleration********************************************/

/*
@brief: this used to init the all peripheral related to bootloader work (RCC,UART,CRC)
@para: void parameter
@ret: BL_OK if all the peripheral is initialized successfully, BL_NOK if any peripheral failed to initialize
*/
BOOTLOADER_STATUS_t bl_init();
/*
@brief: used to deinit all the peripheral used in the bootloader
@para: void 
@ret : BL_OK if all the peripheral is initialized successfully, BL_NOK if any peripheral failed to initialize
*/

BOOTLOADER_STATUS_t bl_deinit();

/*
@brief: this used to fetch the command from the host
@para: void
@ret: BL_OK if the command is fetched successfully, BL_NOK if any error occurs
*/

BOOTLOADER_STATUS_t bl_fetch_command();

#endif /*_BOOTLOADER_H_*/