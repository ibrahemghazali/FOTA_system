
/******************************Section : Include********************************************/
#include "bootloader.h"
#include "bootloader_config.h"
#include "bootloader_private.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_flash.h"
#include "stm32f4xx_hal_flash_ex.h"
#include <string.h>
/******************************Section : Macro definitions********************************************/

static RCC_OscInitTypeDef bl_rcc_osc_cfg = {
    .OscillatorType = RCC_OSCILLATORTYPE_HSI,
    .HSIState = RCC_HSI_ON,
    .HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT,
    .PLL.PLLState = RCC_PLL_NONE
};

static RCC_ClkInitTypeDef bl_rcc_clk_cfg = {
    .ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                 RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2,
    .SYSCLKSource = RCC_SYSCLKSOURCE_HSI,
    .AHBCLKDivider = RCC_SYSCLK_DIV1,
    .APB1CLKDivider = RCC_HCLK_DIV1,
    .APB2CLKDivider = RCC_HCLK_DIV1
};
static UART_HandleTypeDef bl_uart = {
    .Instance = BOOTLOADER_UART ,
    .Init = {
        .BaudRate   = 115200,
        .WordLength = UART_WORDLENGTH_8B,
        .StopBits   = UART_STOPBITS_1,
        .Parity     = UART_PARITY_NONE,
        .Mode       = UART_MODE_TX_RX,
        .OverSampling = UART_OVERSAMPLING_8
    }
};
static GPIO_InitTypeDef bl_uart_tx_gpio_cfg = {
    .Pin = GPIO_PIN_9,
    .Mode = GPIO_MODE_AF_PP,
    .Pull = GPIO_NOPULL,
    .Speed = GPIO_SPEED_FREQ_HIGH,
    .Alternate = GPIO_AF7_USART1
};

static GPIO_InitTypeDef bl_uart_rx_gpio_cfg = {
    .Pin = GPIO_PIN_10,
    .Mode = GPIO_MODE_AF_PP,
    .Pull = GPIO_PULLUP,
    .Speed = GPIO_SPEED_FREQ_HIGH,
    .Alternate = GPIO_AF7_USART1
};


static CRC_HandleTypeDef bl_crc_cfg = {
    .Instance = CRC
};


static GPIO_InitTypeDef blue_led = {
    .Pin =GPIO_PIN_13,
    .Mode = GPIO_MODE_OUTPUT_PP,
    .Pull = GPIO_NOPULL,
    .Speed = GPIO_SPEED_FREQ_HIGH
};


/******************************Section : Macro functions********************************************/


/******************************Section :static variable definitions********************************************/
static uint8_t data[HOST_DATA_MAX_SIZE]={0};

/******************************Section :static functions declerations********************************************/

/* helper functions for bootlaoder*/

/*
*@breif: fetch the crc from data array and return if verfied or not 
*@para:voio
*@ret: CRC_PASSED if the crc is correct, CRC_FAILED if the crc is incorrect 
*/


static uint8_t bl_verify_crc(void);

/*
*@breif :send BL_OK to the host 
*@para:void 
*@ret:void 
*/
static void bl_send_ok(void);

/*
*@breif :send BL_NOK to the host 
*@para: void 
*@ret: void 
*/

static void bl_send_nok(void);



/*
*@breif : send the end of data for host makes host stop receiveing 
*@para: void 
*@ret: void 
*/
static void bl_send_end_of_data(void);


/*static functions for bootloader commands*/
/*
*@breif : get the version of the bootloader 
*@para: void 
*@ret: BL_OK if no issue happened in sending  and BL_NOK if have an issue 
*/
static BOOTLOADER_STATUS_t bl_get_version(void);
/*
*@breif : get the valid commands in this bootloader 
*@para: void
*@ret: BL_OK if no issue happened in sending  and BL_NOK if have an issue 
*/
static BOOTLOADER_STATUS_t bl_get_help(void);
/*
*@breif : get the chip id for the chip
*@para: void
*@ret: BL_OK if no issue happened in sending  and BL_NOK if have an issue 
*/
static BOOTLOADER_STATUS_t bl_get_chip_id(void);
/*
*@breif : erase specific sector sended in data array 
*@para: void
*@ret: BL_OK if no issue happened in erasing and BL_NOK if have an issue 
*/
static BOOTLOADER_STATUS_t bl_erase_sectors(void);
/*
*@breif : erase all sectors except bootloader first two sectors
*@para: void
*@ret: BL_OK if no issue happened in erasing  and BL_NOK if have an issue 
*/
static BOOTLOADER_STATUS_t bl_erase_all_sectors(void);
/*
*@breif : jump to specific address 
*@para: void
*@ret: BL_OK if no issue happened in jumping and BL_NOK if have an issue 
*/
static BOOTLOADER_STATUS_t bl_jump_to_address(void);
/*
*@breif : jump to address contains app (the bootloader won't run after this )
*@para: void 
*@ret: BL_OK if no issue happened in jumping and BL_NOK if have an issue 
*/
static BOOTLOADER_STATUS_t bl_jump_to_app(void);
/*
*@breif : write data in the flash 
*@para: void
*@ret: BL_OK if no issue happened in reading or sending and BL_NOK if have an issue
*/
static BOOTLOADER_STATUS_t bl_write_flash_data(void);
/*
*@breif : read the read write protection level
*@para: void
*@ret: BL_OK if no issue happened in reading or sending and BL_NOK if have an issue
*/
static BOOTLOADER_STATUS_t bl_read_wpr_level(void);
/*
*@breif : read the option bytes for the host 
*@para: void 
*@ret: BL_OK if no issue happened in reading or sending and BL_NOK if have an issue
*/
static BOOTLOADER_STATUS_t bl_read_option_bytes(void);

/******************************Section :functions definitions********************************************/
/*
@brief: this used to init the all peripheral related to bootloader work (RCC,UART,CRC)
@para: void parameter
@ret: BL_OK if all the peripheral is initialized successfully, BL_NOK if any peripheral failed to initialize
*/
BOOTLOADER_STATUS_t bl_init()
{
    BOOTLOADER_STATUS_t ret=BL_OK;
    HAL_StatusTypeDef hal_status=HAL_OK;
    
    hal_status=HAL_Init();
    if(hal_status!=HAL_OK){return BL_NOK;}

    hal_status=HAL_RCC_OscConfig(&bl_rcc_osc_cfg);
    if(hal_status!=HAL_OK){return BL_NOK;}

    hal_status=HAL_RCC_ClockConfig(&bl_rcc_clk_cfg,FLASH_LATENCY_0);
    if(hal_status!=HAL_OK){return BL_NOK;}

    __HAL_RCC_CRC_CLK_ENABLE();
    hal_status=HAL_CRC_Init(&bl_crc_cfg);
    if(hal_status!=HAL_OK){return BL_NOK;}

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();
    HAL_GPIO_Init(GPIOA,&bl_uart_tx_gpio_cfg);
    HAL_GPIO_Init(GPIOA,&bl_uart_rx_gpio_cfg);
    __HAL_RCC_GPIOC_CLK_ENABLE();
    HAL_GPIO_Init(GPIOC,&blue_led);
    hal_status=HAL_UART_Init(&bl_uart);
    if(hal_status!=HAL_OK){return BL_NOK;}

    return ret;
}


/*
@brief: used to deinit all the peripheral used in the bootloader
@para: void 
@ret : BL_OK if all the peripheral is initialized successfully, BL_NOK if any peripheral failed to initialize
*/


BOOTLOADER_STATUS_t bl_deinit()
{
    BOOTLOADER_STATUS_t ret=BL_OK;
    HAL_StatusTypeDef hal_status=HAL_OK;
    hal_status=HAL_DeInit();
    if(hal_status!=HAL_OK){return BL_NOK;}

    hal_status=HAL_RCC_DeInit();
    if(hal_status!=HAL_OK){return BL_NOK;}

    hal_status=HAL_CRC_DeInit(&bl_crc_cfg);
    if(hal_status!=HAL_OK){return BL_NOK;}

    hal_status=HAL_UART_DeInit(&bl_uart);
    if(hal_status!=HAL_OK){return BL_NOK;}
    return ret;    
}


/*
@brief: this used to fetch the command from the host
@para: void
@ret: BL_OK if the command is fetched successfully, BL_NOK if any error occurs
*/

BOOTLOADER_STATUS_t bl_fetch_command()
{
    BOOTLOADER_STATUS_t ret=BL_OK;
    HAL_StatusTypeDef hal_status=HAL_OK;

    //clear all the previous data
    memset(data,0,HOST_DATA_MAX_SIZE);

    //receive through uart
    HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_13);
    hal_status=HAL_UART_Receive(&bl_uart,data,ONE_BYTE,HAL_MAX_DELAY);
    if(hal_status!=HAL_OK)
    {
        bl_send_nok();
        return BL_NOK;
    }
    //receive the length of the commands +4bytes CRC
    hal_status=HAL_UART_Receive(&bl_uart,data+ONE_BYTE,data[LENGTH_BYTE_INDEX]+CRC_SIZE,HAL_MAX_DELAY);
    if(hal_status!=HAL_OK)
    {
        bl_send_nok();
        bl_send_end_of_data();
        return BL_NOK;
    }

    HAL_GPIO_TogglePin(GPIOC,GPIO_PIN_13);
    //verify the crc 
    if(bl_verify_crc()==CRC_PASSED)
    {
        //nothing for now 
    }
    else
    {
        bl_send_nok();
        bl_send_end_of_data();
        return BL_NOK;
    }

    switch(data[COMMAND_INDEX])
    {
    case BOOTLOADER_GET_VERSION:
        bl_send_ok();
        bl_get_version();
        break;

    case BOOTLOADER_GET_HELP:
        bl_send_ok();
        bl_get_help();
        break;

    case BOOTLOADER_GET_CHIP_ID:
        bl_send_ok();
        bl_get_chip_id();
        break;

    case BOOTLOADER_ERASE_SECTORS:
        ret=bl_erase_sectors();
        if(BL_OK==ret)
        {
            bl_send_ok();
        }
        else
        {
            bl_send_nok();
        }
        break;
    
    case BOOTLOADER_ERASE_ALL:
        ret=bl_erase_all_sectors();
        if(ret==BL_OK)
        {
            bl_send_ok();
        }
        else
        {
            bl_send_nok();
        }
        break;
    case BOOTLOADER_JUMP_TO_ADDRESS:
        bl_send_ok();
        bl_send_end_of_data();
        ret=bl_jump_to_address();
        break;
    
    case BOOTLOADER_JUMP_TO_APP:
        bl_send_ok();
        bl_send_end_of_data();
        ret=bl_jump_to_app();
        break;

    case BOOTLOADER_WRITE_FLASH_DATA:
        ret=bl_write_flash_data();
        if(BL_OK==ret)
        {
            bl_send_ok();

        }
        else
        {
            bl_send_nok();
        }
        break;

    case BOOTLOADER_READ_WPR_LEVEL:
        bl_send_ok();
        ret=bl_read_wpr_level();
        break;

    case BOOTLOADER_READ_OPTION_BYTES:
        bl_send_ok();
        ret=bl_read_option_bytes();
        break;

    default:
        ret = BL_NOK;
        break;
    }
    //send the end of message
    uint8_t end_message=0xFF;
    HAL_UART_Transmit(&bl_uart,&end_message,ONE_BYTE,HAL_MAX_DELAY);
    return ret;
}


/******************************Section :static functions definitions********************************************/
/* helper functions */
/*
*@breif: fetch the crc from data array and return if verfied or not 
*@para:voio
*@ret: CRC_PASSED if the crc is correct, CRC_FAILED if the crc is incorrect 
*/
static uint8_t bl_verify_crc(void)
{
    uint32_t bl_crc_value = 0;
    uint32_t host_crc_value = 0;

    __HAL_CRC_DR_RESET(&bl_crc_cfg);

    uint8_t len = data[LENGTH_BYTE_INDEX];

    for(uint8_t i = 0; i < len + 1; i++)
    {
        uint32_t input = (uint32_t)data[i];
        bl_crc_value = HAL_CRC_Accumulate(&bl_crc_cfg, &input, 1);
    }

    // fetch host CRC (4 bytes)
    host_crc_value = *((uint32_t *)(data + len + 1));
    if(host_crc_value==bl_crc_value)
    {
        return CRC_PASSED;
    }
    else
    {
        return CRC_FAILED;
    }
}

/*
*@breif :send BL_OK to the host 
*@para:void 
*@ret:void 
*/
static void bl_send_ok(void)
{
    uint8_t state=BL_OK;
    HAL_UART_Transmit(&bl_uart,&state,ONE_BYTE,HAL_MAX_DELAY);
}

/*
*@breif :send BL_NOK to the host 
*@para: void 
*@ret: void 
*/
static void bl_send_nok(void)
{
    uint8_t state=BL_NOK;
    HAL_UART_Transmit(&bl_uart,&state,ONE_BYTE,HAL_MAX_DELAY);
}

/*
*@breif : send the end of data for host makes host stop receiveing 
*@para: void 
*@ret: void 
*/
static void bl_send_end_of_data(void)
{
    uint8_t end_message=0xFF;
    HAL_UART_Transmit(&bl_uart,&end_message,ONE_BYTE,HAL_MAX_DELAY);
}

/*
*@breif : get the version of the bootloader 
*@para: void 
*@ret: BL_OK if no issue happened in sending  and BL_NOK if have an issue 
*/
static BOOTLOADER_STATUS_t bl_get_version(void)
{
    static uint8_t booloader_version[SIZE_OF_BOOTLOADER_VERSION]={BOOTLOADER_VERSION_ID,BOOTLOADER_VERSION_MAJOR,
                                                                    BOOTLOADER_VERSION_MINOR,BOOTLOADER_VERSION_PATCH};
    BOOTLOADER_STATUS_t ret=BL_OK;
    HAL_StatusTypeDef hal_status=HAL_OK;
    uint8_t l_data_length=SIZE_OF_BOOTLOADER_VERSION;

    //send the length of the data 
    // hal_status=HAL_UART_Transmit(&bl_uart,&l_data_length,ONE_BYTE,HAL_MAX_DELAY);
    // if(hal_status!=HAL_OK){return BL_NOK;}
    
    //send the data
    hal_status=HAL_UART_Transmit(&bl_uart,booloader_version,SIZE_OF_BOOTLOADER_VERSION,HAL_MAX_DELAY);
    if(hal_status!=HAL_OK){return BL_NOK;}

    return ret;
}

/*
*@breif : get the valid commands in this bootloader 
*@para: void
*@ret: BL_OK if no issue happened in sending  and BL_NOK if have an issue 
*/

static BOOTLOADER_STATUS_t bl_get_help(void)
{
    BOOTLOADER_STATUS_t ret=BL_OK;
    HAL_StatusTypeDef hal_status=HAL_OK;
    uint8_t l_data_length=BOOTLOADER_NUMBER_OF_SUPPORTED_COMMANDS;
    static uint8_t bl_supported_commands[BOOTLOADER_NUMBER_OF_SUPPORTED_COMMANDS]=BOOTLOADER_SUPPORTED_COMMANDS;
    //send the length of the data
    // hal_status=HAL_UART_Transmit(&bl_uart,&l_data_length,ONE_BYTE,HAL_MAX_DELAY);
    // if(hal_status!=HAL_OK){return BL_NOK;}

    //send the data
    hal_status=hal_status=HAL_UART_Transmit(&bl_uart,bl_supported_commands,BOOTLOADER_NUMBER_OF_SUPPORTED_COMMANDS,HAL_MAX_DELAY);
    if(hal_status!=HAL_OK){return BL_NOK;} 
    return ret;
}


/*
*@breif : get the chip id for the chip
*@para: void
*@ret: BL_OK if no issue happened in sending  and BL_NOK if have an issue 
*/
static BOOTLOADER_STATUS_t bl_get_chip_id(void)
{
    BOOTLOADER_STATUS_t ret=BL_OK;
    HAL_StatusTypeDef hal_status=HAL_OK;
    uint8_t l_data_length=2;
    uint16_t chip_id=HAL_GetDEVID();

    //send the length of the data
    // hal_status=HAL_UART_Transmit(&bl_uart,&l_data_length,ONE_BYTE,HAL_MAX_DELAY);
    // if(hal_status!=HAL_OK){return BL_NOK;}

    //send the data
    hal_status=hal_status=HAL_UART_Transmit(&bl_uart,(uint8_t *)&chip_id,l_data_length,HAL_MAX_DELAY);
    if(hal_status!=HAL_OK){return BL_NOK;} 
    return ret;
}


/*
*@breif : erase specific sector sended in data array 
*@para: void
*@ret: BL_OK if no issue happened in erasing and BL_NOK if have an issue 
*/

static BOOTLOADER_STATUS_t bl_erase_sectors(void)
{
    BOOTLOADER_STATUS_t ret=BL_OK;
    HAL_StatusTypeDef hal_status=HAL_OK;
    uint32_t sector_error=0;
    static FLASH_EraseInitTypeDef flash_erase_obj=
    {
        .TypeErase=FLASH_TYPEERASE_SECTORS,
        .Banks=FLASH_BANK_1,
        .VoltageRange=FLASH_VOLTAGE_RANGE_3,
    };
    //validate the sector number and the number of the sectors 
    if((VALIDATE_FLASH_SECTOR())&&(VALIDATE_FLASH_NUMBER_OF_SECTORS()))
    {
        flash_erase_obj.Sector=data[COMMAND_FLASH_STARTING_SECTOR_INDEX];
        flash_erase_obj.NbSectors=data[COMMAND_FLASH_NUMBER_OF_SECTORS_INDEX];
        
        //flash unlock
        hal_status=HAL_FLASH_Unlock();
        if(hal_status!=HAL_OK){return BL_NOK;}

        //erasing 
        hal_status=HAL_FLASHEx_Erase(&flash_erase_obj,&sector_error);//hardfault handler (erasing sector 2 )
        if((hal_status!=HAL_OK)||(sector_error!=ALL_SECTORS_ERASED))
        {
            HAL_FLASH_Lock();//ensure the flash is locked 
            return BL_NOK;
        }
        HAL_FLASH_Lock();

    }
    else
    {
        return BL_NOK;
    }

    return ret;
}

/*
*@breif : erase all sectors except bootloader first two sectors
*@para: void
*@ret: BL_OK if no issue happened in erasing  and BL_NOK if have an issue 
*/
static BOOTLOADER_STATUS_t bl_erase_all_sectors(void)
{
    BOOTLOADER_STATUS_t ret=BL_OK;
    HAL_StatusTypeDef hal_status=HAL_OK;
    uint32_t sector_error=0;
    static FLASH_EraseInitTypeDef flash_erase_obj=
    {
        .TypeErase=FLASH_TYPEERASE_SECTORS,
        .Banks=FLASH_BANK_1,
        .VoltageRange=FLASH_VOLTAGE_RANGE_3,
        .Sector=FLASH_SECTOR_2,
        .NbSectors=4
    }; 

    //flash unlock
    hal_status=HAL_FLASH_Unlock();
    if(hal_status!=HAL_OK)
    {
        return BL_NOK;
    }

    hal_status=HAL_FLASHEx_Erase(&flash_erase_obj, &sector_error);
    if((hal_status!=HAL_OK)||(sector_error!=ALL_SECTORS_ERASED))
    {
        HAL_FLASH_Lock();
        return BL_NOK;
    }
    //flash lock
    HAL_FLASH_Lock();

    return ret;
}




/*
*@breif : jump to specific address 
*@para: void
*@ret: BL_OK if no issue happened in jumping and BL_NOK if have an issue 
*/
static BOOTLOADER_STATUS_t bl_jump_to_address(void)
{
    BOOTLOADER_STATUS_t ret=BL_OK;
    HAL_StatusTypeDef hal_status=HAL_OK;
    uint32_t jumping_address=0;
    pvfunction jumping_fun=NULL;
    //fetch the address
    jumping_address=*((uint32_t *)(data+JUMPING_ADDRESS_INDEX));
    
    //validate the jumping address
    if(VALIDATE_JUMPING_ADDRESS(jumping_address))
    {
        jumping_fun=((pvfunction)(jumping_address|THUMB_BIT));//ensure thumb instructions

        //jump 
        jumping_fun();
    }
    else
    {
        return BL_NOK;
    }
    return ret;

}


/*
*@breif : jump to address contains app (the bootloader won't run after this )
*@para: void 
*@ret: BL_OK if no issue happened in jumping and BL_NOK if have an issue 
*/

static BOOTLOADER_STATUS_t bl_jump_to_app(void)
{
    BOOTLOADER_STATUS_t ret=BL_OK;
    HAL_StatusTypeDef hal_status=HAL_OK;

    uint32_t jumping_address=0;
    pvfunction jumping_reset=NULL;

    //fetch the address
    jumping_address=*((uint32_t *)(data+JUMPING_ADDRESS_INDEX));

    //validate the jumping address
    if(VALIDATE_JUMPING_ADDRESS(jumping_address))
    {   
        bl_deinit();
        //set the vector table 
        SCB->VTOR=jumping_address;
        //set the main stack pointer
        __set_MSP(*((uint32_t *)(jumping_address))); 

        //call the reset handler 
        jumping_reset=(pvfunction)(*((uint32_t *)(jumping_address+RESET_HANDLER_OFFSET)));
        jumping_reset();
    }
    else
    {
        return BL_NOK;
    }

}


/*
*@breif : write data in the flash 
*@para: void
*@ret: BL_OK if no issue happened in reading or sending and BL_NOK if have an issue
*/
static BOOTLOADER_STATUS_t bl_write_flash_data(void)
{
    
    BOOTLOADER_STATUS_t ret=BL_OK;
    HAL_StatusTypeDef hal_status=HAL_OK;
    uint8_t data_size=data[SIZE_OF_WRITING_FLASH_DATA_INDEX];
    uint32_t start_address=0;
    pvfunction jumping_reset=NULL;
    uint8_t l_counter=0;
    //fetch tha address 
    start_address=*((uint32_t *)(data+JUMPING_ADDRESS_INDEX));

    if((VALIDATE_FLASH_ADDRESS(start_address))&&(VALIDATE_DATA_SIZE_ALIGNMENT(data_size)))
    {
        //flash unlock 
        hal_status=HAL_FLASH_Unlock();
        if(hal_status!=HAL_OK){return BL_NOK;}

        //write the data 
        for(l_counter=0;l_counter<data_size;l_counter+=BOOTLOADER_FLASH_WRITING_DATA_LENGTH)   
        {
            uint32_t temp=*((uint32_t *)(data+START_OF_DATA_INDEX+l_counter));
            hal_status=HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,start_address+l_counter,(uint64_t )temp);
            if(hal_status!=HAL_OK)
            {
                HAL_FLASH_Lock();
                return BL_NOK;
            }
        }
        
        //flash lock
        HAL_FLASH_Lock();
    }
    else
    {
        ret=BL_NOK;
    }

    return ret;
}

/*
*@breif : read the read write protection level
*@para: void
*@ret: BL_OK if no issue happened in reading or sending and BL_NOK if have an issue
*/

static BOOTLOADER_STATUS_t bl_read_wpr_level(void)
{
    BOOTLOADER_STATUS_t ret = BL_OK;
    HAL_StatusTypeDef hal_status = HAL_OK;
    FLASH_OBProgramInitTypeDef ob_cfg;
    uint8_t l_data_length = ONE_BYTE;
    uint8_t rdp_level = 0xFF;

    memset(&ob_cfg, 0, sizeof(ob_cfg));
    HAL_FLASHEx_OBGetConfig(&ob_cfg);

    switch (ob_cfg.RDPLevel)
    {
        case OB_RDP_LEVEL_0:
            rdp_level = 0U;
            break;
        case OB_RDP_LEVEL_1:
            rdp_level = 1U;
            break;
          default:
            return BL_NOK;
    }

    // hal_status = HAL_UART_Transmit(&bl_uart, &l_data_length, ONE_BYTE, HAL_MAX_DELAY);
    // if (hal_status != HAL_OK) { return BL_NOK; }

    hal_status = HAL_UART_Transmit(&bl_uart, &rdp_level, l_data_length, HAL_MAX_DELAY);
    if (hal_status != HAL_OK) { return BL_NOK; }

    return ret;
}


/*
*@breif : read the option bytes for the host 
*@para: void 
*@ret: BL_OK if no issue happened in reading or sending and BL_NOK if have an issue
*/
static BOOTLOADER_STATUS_t bl_read_option_bytes(void)
{
    BOOTLOADER_STATUS_t ret = BL_OK;
    HAL_StatusTypeDef hal_status = HAL_OK;
    FLASH_OBProgramInitTypeDef ob_cfg;
    uint32_t ob_data[6];
    uint8_t l_data_length = (uint8_t)sizeof(ob_data);

    memset(&ob_cfg, 0, sizeof(ob_cfg));
    HAL_FLASHEx_OBGetConfig(&ob_cfg);

    ob_data[0] = ob_cfg.RDPLevel;
    ob_data[1] = ob_cfg.USERConfig;
    ob_data[2] = ob_cfg.BORLevel;
    ob_data[3] = ob_cfg.WRPState;
    ob_data[4] = ob_cfg.WRPSector;
    ob_data[5] = ob_cfg.Banks;

    // hal_status = HAL_UART_Transmit(&bl_uart, &l_data_length, ONE_BYTE, HAL_MAX_DELAY);
    // if (hal_status != HAL_OK) { return BL_NOK; }

    hal_status = HAL_UART_Transmit(&bl_uart, (uint8_t *)ob_data, l_data_length, HAL_MAX_DELAY);
    if (hal_status != HAL_OK) { return BL_NOK; }

    return ret;
}