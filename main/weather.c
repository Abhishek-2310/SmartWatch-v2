#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "esp_http_client.h"
#include <cJSON.h>

static const char *TAG = "display_weather";

// API key from OpenWeatherMap 
char open_weather_map_api_key[] = "87d57e01d9131a17b0f4b049bbb8be19";

char city[] = "cambridge";
char country_code[] = "ca";

#define HTTP_RESPONSE_BUFFER_SIZE 1024

char *response_data = NULL;
size_t response_len = 0;
bool all_chunks_received = false;

char weather_status[10];
double weather_temp = 0.0;
double weather_speed = 0.0;
int weather_pressure = 0;
int weather_humidity = 0;

char description_array[4][10];
uint8_t description_index = 0;

void get_temp_pressure_humidity(const char *json_string)
{
   cJSON *root = cJSON_Parse(json_string);
    cJSON *list = cJSON_GetObjectItem(root, "list");
    if (!list || !cJSON_IsArray(list)) {
        printf("Error: 'list' not found or is not an array.\n");
        cJSON_Delete(root);
        return;
    }
    
    for (int i = 0; i < cJSON_GetArraySize(list); i=i+8) 
    {
        cJSON *item = cJSON_GetArrayItem(list, i);
        cJSON *main = cJSON_GetObjectItem(item, "main");
        cJSON *weather = cJSON_GetObjectItem(item, "weather");
        cJSON *wind = cJSON_GetObjectItem(item, "wind");

        if (main && cJSON_IsArray(weather) && cJSON_GetArraySize(weather) > 0) 
        {
            if(i == 0)
            {
                double temp1 = cJSON_GetObjectItem(main, "temp")->valuedouble;
                int pressure1 = cJSON_GetObjectItem(main, "pressure")->valueint;
                int humidity1 = cJSON_GetObjectItem(main, "humidity")->valueint;
                const char* description1 = cJSON_GetObjectItem(cJSON_GetArrayItem(weather, 0), "main")->valuestring;
                double speed1 = cJSON_GetObjectItem(wind, "speed")->valuedouble;

                weather_temp = temp1;
                weather_pressure = pressure1;
                weather_humidity = humidity1;
                weather_speed = speed1;
                memcpy(weather_status, description1, strlen(description1));
                weather_status[strlen(description1)] = '\0';
                printf("Status: %s\nTemperature: %0.00f°C\nPressure: %d hPa\nHumidity: %d%%\nSpeed: %0.0f mph\n", weather_status, temp1, pressure1, humidity1, speed1);
            }
            else
            {            
                const char* description2 = cJSON_GetObjectItem(cJSON_GetArrayItem(weather, 0), "main")->valuestring;
                memcpy(description_array[description_index], description2, strlen(description2));
                description_array[description_index][strlen(description2)] = '\0';
                printf("Day: %d Weather: %s\n", i/8, description_array[description_index]);
                description_index = (description_index + 1) % 4;
            }
        }
    }

    cJSON_Delete(root);
    ESP_LOGI(TAG, "root deleted!");
    
    if (response_data) {
        free(response_data);
        ESP_LOGI(TAG, "response_data freed!");
    }

    // Reset Response parameters
    response_data = NULL;
    response_len = 0;
    // all_chunks_received = false;
}

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            // Resize the buffer to fit the new chunk of data
            response_data = realloc(response_data, response_len + evt->data_len);
            memcpy(response_data + response_len, evt->data, evt->data_len);
            response_len += evt->data_len;
            break;
        case HTTP_EVENT_ON_FINISH:
            all_chunks_received = true;
            // ESP_LOGI("OpenWeatherAPI", "Received data: %s", response_data);
            get_temp_pressure_humidity(response_data);
            break;
        default:
            break;
    }
    return ESP_OK;
}


void openweather_api_http(void *pvParameters)
{

    char open_weather_map_url[200];

    while (1) 
    { 
        snprintf(open_weather_map_url,
             sizeof(open_weather_map_url),
             "%s%s%s%s%s%s%s",
             "http://api.openweathermap.org/data/2.5/forecast?q=",
             city,
             ",",
             country_code,
             "&APPID=",
             open_weather_map_api_key,
             "&units=metric");

        esp_http_client_config_t config = {
            .url = open_weather_map_url,
            .method = HTTP_METHOD_GET,
            .event_handler = _http_event_handler,
        };

        esp_http_client_handle_t client = esp_http_client_init(&config);
        esp_http_client_set_header(client, "Content-Type", "application/x-www-form-urlencoded");

        esp_err_t err = esp_http_client_perform(client);

        if (err == ESP_OK)
        {
            int status_code = esp_http_client_get_status_code(client);
            if (status_code == 200)
            {
                ESP_LOGI(TAG, "Message sent Successfully");
            }
            else
            {
                ESP_LOGI(TAG, "Message sent Failed");
            }
        }
        else
        {
            ESP_LOGI(TAG, "Message sent Failed");
        }
        esp_http_client_cleanup(client);
    
        vTaskDelay(pdMS_TO_TICKS(30000));
    }

    vTaskDelete(NULL);
}

void get_weather_update(void)
{
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	// connect_wifi();
	// if (wifi_connect_status)
	// {
        xTaskCreate(&openweather_api_http, "openweather_api_http", 8192, NULL, 1, NULL);
	// }
}
