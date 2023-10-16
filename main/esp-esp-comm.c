
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "protocol_examples_common.h"
#include "common.h"

/** DEFINES **/
#define WIFI_SUCCESS 1 << 0
#define WIFI_FAILURE 1 << 1
#define TCP_SUCCESS 1 << 0
#define TCP_FAILURE 1 << 1
#define MAX_FAILURES 10
#define PORT 3000
#define IP_ADDRESS "10.0.0.181"

// task tag
static const char *TAG = "WIFI-client";

// connect to the server and return the result
esp_err_t connect_tcp_server(void)
{
	int client_socket;
    struct sockaddr_in server_address;
    // char *response = "Hello from ESP32!\n";

    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Set up server address
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(IP_ADDRESS);
    server_address.sin_port = htons(PORT);

    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Send a message
    const char *message = "TURN_ON_LED";
    if (send(client_socket, message, strlen(message), 0) == -1) {
        perror("Message send failed");
        exit(EXIT_FAILURE);
    }

    printf("Message sent: %s\n", message);

    // Close the server socket
    close(client_socket);

    return TCP_SUCCESS;
}


void Esp_Comms_Task(void *pvParameter)
{
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_DELAY));

    int comms_button_state = gpio_get_level(COMMS_PIN);
    if(comms_button_state == 0)
    {
        ESP_LOGI(TAG, "Comms turned ON");

        esp_err_t status = WIFI_FAILURE;

        status = connect_tcp_server();
        if (TCP_SUCCESS != status)
        {
            ESP_LOGI(TAG, "Failed to connect to remote server, dying...");
            return;
        }
    }
}