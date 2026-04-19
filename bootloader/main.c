#include "bootloader.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal.h"


int main()
{
    bl_init();

    while(1)
    {   
        bl_fetch_command();
        // for(uint32_t i=0;i<1000000;i++);//can this line is the problem 
    }
    return 0;
}