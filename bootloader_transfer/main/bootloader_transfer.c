#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "hal/gpio_types.h"
#include "nvs_flash.h"
#include "portmacro.h"
#include "tcp_client.h"
#include "driver/uart.h"
#include <stdint.h>
#include <string.h>
#include "driver/gpio.h"


#define DATA_LENGTH_INDEX 0
#define CRC_LENGTH_AND_DATA_SIZE_LENGTH 5
#define TCP_READY_FOR_RECEIVING 0
#define UART_READY_FOR_RECEIVING 1
#define ONE_BYTE 1
#define END_OF_MESSAGE 0xff
#define UART_ARRAY_SIZE 10
#define TCP_ARRAY_SIZE 300

#define STM_RESET_PIN               GPIO_NUM_4
#define STM_RESET_ASSERT_LEVEL      0   // NRST active low
#define STM_RESET_RELEASE_LEVEL     1
#define STM_RESET_PULSE_MS          50
#define STM_BOOT_WAIT_MS            50
#define RESET_STM_COMMAND 0xFF  
#define COMMAND_INDEX 1

static const char *TAG = "APP";

uint8_t uart_array[UART_ARRAY_SIZE];
uint8_t tcp_array[TCP_ARRAY_SIZE];
int client = 0;
uint8_t uart_rx;
uint8_t tcp_rx;
uint8_t flag=0;
uint8_t l_counter=0;
uint16_t tcp_length=0;

uart_config_t uart_config =
{
    .baud_rate = 115200,
    .data_bits = UART_DATA_8_BITS,
    .parity = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
};
gpio_config_t io_conf = 
{
  .pin_bit_mask = (1ULL << STM_RESET_PIN),
  .mode = GPIO_MODE_OUTPUT,
  .pull_up_en = GPIO_PULLUP_ENABLE,
  .pull_down_en = GPIO_PULLDOWN_DISABLE,
  .intr_type = GPIO_INTR_DISABLE,
};



static void stm_reset_to_bootloader(void);

void app_main(void)
{
      // Init NVS
      esp_err_t ret = nvs_flash_init();
      if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
      {
          ESP_ERROR_CHECK(nvs_flash_erase());
          ESP_ERROR_CHECK(nvs_flash_init());
      }

      //gpio reset pin init 
      ESP_ERROR_CHECK(gpio_config(&io_conf));
      ESP_ERROR_CHECK(gpio_set_level(STM_RESET_PIN, STM_RESET_RELEASE_LEVEL));

      // UART init
      uart_param_config(UART_NUM_2, &uart_config);
      uart_driver_install(UART_NUM_2, 512, 0, 0, NULL, 0);
      uart_set_pin(UART_NUM_2, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
      gpio_set_pull_mode(GPIO_NUM_16, GPIO_PULLUP_ONLY);


      // WiFi
      esp_wifi_sta_init();

      // TCP connect
      ESP_ERROR_CHECK(esp_connect_to_server("192.168.1.3", 5000, &client));
      ESP_LOGI(TAG, "Connected to server");


      while (1)
      {


          /**************** TCP -> UART ****************/
          if(TCP_READY_FOR_RECEIVING==flag)
          {
            //reset the tcp_array and tcp_length before receiving new message
            memset(tcp_array,0,sizeof(tcp_array));
            tcp_length=0;

            //starting to receive data from server 
            tcp_length+=esp_receive_from_server(client, tcp_array+tcp_length, sizeof(tcp_array));
            while((tcp_array[COMMAND_INDEX] != RESET_STM_COMMAND)&&(tcp_length!=tcp_array[DATA_LENGTH_INDEX]+CRC_LENGTH_AND_DATA_SIZE_LENGTH))//not the end of message
            {
              tcp_length+=esp_receive_from_server(client, tcp_array+tcp_length, sizeof(tcp_array));
            } 
            ESP_LOGI("TCP","checking ");
            if(((tcp_array[COMMAND_INDEX] == RESET_STM_COMMAND)||(tcp_length==tcp_array[DATA_LENGTH_INDEX]+CRC_LENGTH_AND_DATA_SIZE_LENGTH)) && (tcp_length!=0)) //end of message
            {
              //RESET the uart_array and l_counter before sending new message to UART
              memset(uart_array,0,sizeof(uart_array));
              l_counter=0;


              if(tcp_array[COMMAND_INDEX] == RESET_STM_COMMAND)
              {
                stm_reset_to_bootloader();
              }
              else 
              {
                //send the length byte and data bytes to UART
                flag=UART_READY_FOR_RECEIVING;
                uart_write_bytes(UART_NUM_2, (const char *)tcp_array,1);
                vTaskDelay(pdMS_TO_TICKS(2));
                uart_write_bytes(UART_NUM_2, (const char *)tcp_array+1,tcp_length-1); 
                ESP_LOGI("TCP","finish sending to uart ");
              }

            }
            else
            {
              ESP_LOGI("TCP","error in receiving from server or end of message not received");       
            }
          }

          

          /**************** UART -> TCP ****************/
          if(UART_READY_FOR_RECEIVING==flag)
          {
            int len = uart_read_bytes(UART_NUM_2, &uart_rx, ONE_BYTE, pdMS_TO_TICKS(1000));
            if(len > 0)
            {
              if(END_OF_MESSAGE==uart_rx) //end of message
              {
                flag=TCP_READY_FOR_RECEIVING;
                esp_send_to_server(client, uart_array, l_counter);
                ESP_LOGI("TCP","finish receiving from uart ");

              }
              else
              {
                uart_array[l_counter] = uart_rx;
                l_counter++;
               
              }
              
            }
          }

          vTaskDelay(pdMS_TO_TICKS(2));
      }
  }


static void stm_reset_to_bootloader(void)
{
  ESP_ERROR_CHECK(gpio_set_level(STM_RESET_PIN, STM_RESET_ASSERT_LEVEL));
  vTaskDelay(pdMS_TO_TICKS(STM_RESET_PULSE_MS));
  ESP_ERROR_CHECK(gpio_set_level(STM_RESET_PIN, STM_RESET_RELEASE_LEVEL));
  vTaskDelay(pdMS_TO_TICKS(STM_BOOT_WAIT_MS));
  ESP_LOGI(TAG, "STM reset pulse sent");
}