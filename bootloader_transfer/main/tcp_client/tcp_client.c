/************************* INCLUDES ******************************/
#include "tcp_client.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "lwip/sockets.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

/************************* MACROS ******************************/
#define ROUTER_SSID "your_router_ssid"
#define PASSWORD    "your_router_password"

#define WIFI_CONNECTED         BIT0
#define WIFI_FAILED_TO_CONNECT BIT1

#define MAX_RETRY 15

/************************* STATIC VARIABLES ******************************/
static EventGroupHandle_t wifi_events = NULL;
static uint8_t retry_counter = 0;

/************************* STATIC FUNCTION DECLARATIONS ******************************/


/*
    * @brief Handle WiFi events.
    *
    * This function is called when a WiFi event occurs.
    *
    * @param arg A pointer to the argument passed to the event handler.
    * @param event_base The base of the event.
    * @param event_id The ID of the event.
    * @param event_data A pointer to the data associated with the event.
*/
static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data);


/* 
*@breif :ip event handler, it will be called when the station gets an IP address from the router, it will set the WIFI_CONNECTED bit in the wifi_events event group to unblock the task waiting for connection.
*@param arg: not used
*@param event_base: the base of the event, in this case it will be IP_EVENT
*@param event_id: the id of the event, in this case it will be IP_EVENT_STA_GOT_IP
*@param event_data: the data of the event, in this case it will be a pointer to an ip_event_got_ip_t structure that contains the IP address,
 netmask and gateway assigned to the station.
*/                               
static void ip_event_handler(void *arg,
                             esp_event_base_t event_base,
                             int32_t event_id,
                             void *event_data);

/************************* function definitions ******************************/
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
esp_err_t esp_wifi_sta_init(void)
{
    wifi_events = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = ROUTER_SSID,
            .password = PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID,
        &wifi_event_handler, NULL, NULL));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, ESP_EVENT_ANY_ID,
        &ip_event_handler, NULL, NULL));

    ESP_ERROR_CHECK(esp_wifi_start());

    EventBits_t bits = xEventGroupWaitBits(
        wifi_events,
        WIFI_CONNECTED | WIFI_FAILED_TO_CONNECT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );

    if(bits & WIFI_CONNECTED)
    {
        printf("WiFi Connected\n");
    }
    else
    {
        printf("WiFi Failed\n");
    }

    return ESP_OK;
}



/**
    * @brief Deinitialize WiFi in station mode.
    *
    * This function deinitializes the WiFi driver and frees up any resources allocated for WiFi operation.
    *
    * @return ESP_OK on successful deinitialization, or an appropriate error code on failure.
*/
esp_err_t esp_wifi_sta_deinit(void)
{
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
    return ESP_OK;
}



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
esp_err_t esp_connect_to_server(const char *ip,uint16_t port,int *client)
{
    if(NULL==ip || NULL==client)
    {
        return ESP_FAIL;
    }
    else
    {
        *client = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

        if(*client < 0)
            return ESP_FAIL;

        struct sockaddr_in dest = {0};
        dest.sin_family = AF_INET;
        dest.sin_port = htons(port);
        dest.sin_addr.s_addr = inet_addr(ip);

        if(connect(*client, (struct sockaddr *)&dest, sizeof(dest)) < 0)
        {
            close(*client);
            return ESP_FAIL;
        }

        printf("TCP Connected\n");
    }
    return ESP_OK;
}


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
esp_err_t esp_send_to_server(int client,const void *data,size_t len)
{

    size_t total = 0;
    if(NULL==data || len == 0)
    {
        return ESP_FAIL;
    }
        
    while(total < len)
    {
        int sent = send(client,(const uint8_t *)data + total,len - total,0);

        if(sent <= 0)
        {
            return ESP_FAIL;
        }
            
        total += sent;
    }

    return ESP_OK;
}


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


uint16_t esp_receive_from_server(int client,void *data,size_t len)
{
    uint16_t length_received = 0;
    if(NULL==data || len == 0)
    {
        return ESP_FAIL;
    }
        
    else
    {
        length_received= recv(client, data, len, 0);
        
        if(length_received <= 0)
        {
            return 0;
        }
        
    }

    return length_received;
}
/*
    * @brief Deinitialize WiFi in station mode.
    *
    * This function deinitializes the WiFi driver and frees up any resources allocated for WiFi operation.
    *
    * @return ESP_OK on successful deinitialization, or an appropriate error code on failure.
*/
esp_err_t esp_close_socket(int client)
{
    if(close(client) < 0)
    {   
        return ESP_FAIL;
    }
        

    return ESP_OK;
}


/************************* static functtion definitions  ******************************/

/*
    * @brief Handle WiFi events.
    *
    * This function is called when a WiFi event occurs.
    *
    * @param arg A pointer to the argument passed to the event handler.
    * @param event_base The base of the event.
    * @param event_id The ID of the event.
    * @param event_data A pointer to the data associated with the event.
*/
static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if(event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if(event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if(retry_counter < MAX_RETRY)
        {
            esp_wifi_connect();
            retry_counter++;
        }
        else
        {
            xEventGroupSetBits(wifi_events, WIFI_FAILED_TO_CONNECT);
        }
    }
}


/* 
*@breif :ip event handler, it will be called when the station gets an IP address from the router, it will set the WIFI_CONNECTED bit in the wifi_events event group to unblock the task waiting for connection.
*@param arg: not used
*@param event_base: the base of the event, in this case it will be IP_EVENT
*@param event_id: the id of the event, in this case it will be IP_EVENT_STA_GOT_IP
*@param event_data: the data of the event, in this case it will be a pointer to an ip_event_got_ip_t structure that contains the IP address,
 netmask and gateway assigned to the station.
*/
static void ip_event_handler(void *arg,
                             esp_event_base_t event_base,
                             int32_t event_id,
                             void *event_data)
{
    if(event_id == IP_EVENT_STA_GOT_IP)
    {
        retry_counter = 0;
        xEventGroupSetBits(wifi_events, WIFI_CONNECTED);
    }
}
