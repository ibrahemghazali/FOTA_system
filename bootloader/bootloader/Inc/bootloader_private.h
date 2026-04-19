#ifndef _BOOTLOADER_PRIVATE_H_
#define _BOOTLOADER_PRIVATE_H_

/******************************Section : Include********************************************/

/******************************Section : Macro definitions********************************************/
#define HOST_DATA_MAX_SIZE 300
#define ONE_BYTE 1
#define LENGTH_BYTE_INDEX 0
#define COMMAND_INDEX 1
#define JUMPING_ADDRESS_INDEX 2


#define SIZE_OF_WRITING_FLASH_DATA_INDEX 6
#define START_OF_DATA_INDEX              7


#define CRC_SIZE 4


#define CRC_PASSED 1
#define CRC_FAILED 0


#define SIZE_OF_BOOTLOADER_VERSION 4


/* bootloader (stm32 flash definitions)*/
#define FLASH_START_SECTOR 2
#define FLASH_END_SECTOR 5
#define COMMAND_FLASH_STARTING_SECTOR_INDEX 2
#define COMMAND_FLASH_NUMBER_OF_SECTORS_INDEX 3
#define ALL_SECTORS_ERASED 0xFFFFFFFFU

#define FLASH_START_ADDRESS 0x08008000UL /* start of sector 2 address */
#define FLASH_END_ADDRESS   0x08020000UL /* start of sector 5 address */

//SRAM addresses
#define SRAM_START_ADDRESS 0x20000000UL
#define SRAM_END_ADDRESS   0x2000FFFFUL


#define RESET_HANDLER_OFFSET 4


#define THUMB_BIT 1



#define BOOTLOADER_FLASH_WRITING_DATA_LENGTH 4
#define BOOTLOADER_FLasH_DATA_ALIGNED         0



/******************************Section : Macro functions********************************************/


//validate sector number 
#define VALIDATE_FLASH_SECTOR() (((data[COMMAND_FLASH_STARTING_SECTOR_INDEX]<=FLASH_END_SECTOR)\
                                            &&(data[COMMAND_FLASH_STARTING_SECTOR_INDEX]>=FLASH_START_SECTOR)))

#define VALIDATE_FLASH_NUMBER_OF_SECTORS() \
((data[COMMAND_FLASH_STARTING_SECTOR_INDEX] + data[COMMAND_FLASH_NUMBER_OF_SECTORS_INDEX] - 1) <= FLASH_END_SECTOR)

#define VALIDATE_FLASH_ADDRESS(ADDRESS) (((ADDRESS>=FLASH_START_ADDRESS)&&(ADDRESS<=FLASH_END_ADDRESS)))

#define VALIDATE_JUMPING_ADDRESS(ADDRESS)   (((ADDRESS>=FLASH_START_ADDRESS)&&(ADDRESS<=FLASH_END_ADDRESS))|| \
                                            ((ADDRESS>=SRAM_START_ADDRESS)&&(ADDRESS<=SRAM_END_ADDRESS)))



#define VALIDATE_DATA_SIZE_ALIGNMENT(DATA_SIZE) (BOOTLOADER_FLasH_DATA_ALIGNED==DATA_SIZE%BOOTLOADER_FLASH_WRITING_DATA_LENGTH)



/******************************Section :user-defined data types********************************************/
typedef void (*pvfunction)(void);


/******************************Section :functions decleration********************************************/


#endif /*_BOOTLOADER_PRIVATE_H_*/