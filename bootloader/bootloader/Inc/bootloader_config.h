#ifndef _BOOTLOADER_CONFIG_H_
#define _BOOTLOADER_CONFIG_H_

/******************************Section : Include********************************************/


/******************************Section : Macro definitions********************************************/
/* select the the Uart preipheral you want to implement bootloader on it */
/* 
*USART1 
*USART2
*USART3
*/
#define BOOTLOADER_UART USART1




/* host commands that will be supported */
#define BOOTLOADER_NUMBER_OF_SUPPORTED_COMMANDS 10
#define BOOTLOADER_GET_VERSION 0x10
#define BOOTLOADER_GET_HELP    0x11
#define BOOTLOADER_GET_CHIP_ID 0x12
#define BOOTLOADER_ERASE_SECTORS 0x13
#define BOOTLOADER_ERASE_ALL     0x14
#define BOOTLOADER_JUMP_TO_ADDRESS 0x15
#define BOOTLOADER_JUMP_TO_APP      0x16
#define BOOTLOADER_WRITE_FLASH_DATA 0x17
#define BOOTLOADER_READ_WPR_LEVEL   0x18
#define BOOTLOADER_READ_OPTION_BYTES 0x19

#define BOOTLOADER_SUPPORTED_COMMANDS {\
BOOTLOADER_GET_VERSION,          \
BOOTLOADER_GET_HELP,             \
BOOTLOADER_GET_CHIP_ID,          \
BOOTLOADER_ERASE_SECTORS,        \
BOOTLOADER_ERASE_ALL,            \
BOOTLOADER_JUMP_TO_ADDRESS,      \
BOOTLOADER_JUMP_TO_APP,          \
BOOTLOADER_WRITE_FLASH_DATA,     \
BOOTLOADER_READ_WPR_LEVEL,       \
BOOTLOADER_READ_OPTION_BYTES     \
}

/* bootloader version */
#define BOOTLOADER_VERSION_ID 0x70
#define BOOTLOADER_VERSION_MAJOR 10
#define BOOTLOADER_VERSION_MINOR 10
#define BOOTLOADER_VERSION_PATCH 10


/******************************Section : Macro functions*********************************************/



/******************************Section :user-defined data types********************************************/



/******************************Section :functions decleration********************************************/


#endif /*_BOOTLOADER_CONFIG_H_*/