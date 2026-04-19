#ifndef _WIFI_TCP_H_
#define _WIFI_TCP_H_

/*************************SECTION:includes******************************/
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"


#include "lwip/sockets.h"
#include "lwip/netdb.h"

/*************************SECTION:macro defintions******************************/


/*************************SECTION:macro functions******************************/


/*************************SECTION:user-defined data types******************************/

/*************************SECTION:function decleration******************************/

/*
    * @brief Initialize WiFi in station mode and connect to the specified router.
    *
    * This function initializes the WiFi driver, configures it to operate in station mode,
    * and attempts to connect to the specified WiFi network using the provided SSID and password.
    * It also sets up event handlers to monitor the connection status and waits for either a successful
    * connection or a failure before returning.
    *
    * @return ESP_OK on successful initialization and connection, or an appropriate error code on failure.
*/
esp_err_t esp_wifi_sta_init();

/**
    * @brief Deinitialize WiFi in station mode.
    *
    * This function deinitializes the WiFi driver and frees up any resources allocated for WiFi operation.
    *
    * @return ESP_OK on successful deinitialization, or an appropriate error code on failure.
*/
esp_err_t esp_wifi_sta_deinit();


/*
    * @brief Connect to a TCP server.
    *
    * This function establishes a TCP connection to the specified server IP address and port.
    *
    * @param ip The IP address of the server to connect to.
    * @param port The port number of the server to connect to.
    * @param client A pointer to an integer where the client socket descriptor will be stored.
    * @return ESP_OK on successful connection, or an appropriate error code on failure.
*/

esp_err_t esp_connect_to_server(const char *ip,uint16_t port,int *client);

/*
    * @brief Close a TCP socket.
    *
    * This function closes the specified TCP socket.
    *
    * @param client The socket descriptor to close.
    * @return ESP_OK on successful closure, or an appropriate error code on failure.
*/
esp_err_t esp_close_socket(int client);

/*
    * @brief Send data to a TCP server.
    *
    * This function sends the specified data to the TCP server through the given client socket.
    *
    * @param client The socket descriptor to send data through.
    * @param data A pointer to the data to send.
    * @param len The length of the data to send.
    * @return ESP_OK on successful sending, or an appropriate error code on failure.
*/
esp_err_t esp_send_to_server(int client,const void *data,size_t len);

/*
    * @brief Receive data from a TCP server.
    *
    * This function receives data from the TCP server through the given client socket.
    *
    * @param client The socket descriptor to receive data from.
    * @param data A pointer to the buffer where received data will be stored.
    * @param len The length of the buffer.
    * @return The number of bytes received, or an appropriate error code on failure.
*/
uint16_t esp_receive_from_server(int client,void *data,size_t len);
/*
    * @brief Deinitialize WiFi in station mode.
    *
    * This function deinitializes the WiFi driver and frees up any resources allocated for WiFi operation.
    *
    * @return ESP_OK on successful deinitialization, or an appropriate error code on failure.
*/
esp_err_t esp_close_socket(int client);

#endif /*_WIFI_TCP_H_*/
