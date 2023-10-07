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

char city[] = "kitchener";
char country_code[] = "ca";

#define HTTP_RESPONSE_BUFFER_SIZE 1024

char *response_data = NULL;
size_t response_len = 0;
bool all_chunks_received = false;

char weather_status[20];
double weather_temp = 0.0;
int weather_pressure = 0;
int weather_humidity = 0;

void get_temp_pressure_humidity(const char *json_string)
{
   
    cJSON *root = cJSON_Parse(json_string);
    cJSON *weather_param = cJSON_GetObjectItemCaseSensitive(root, "weather");
    cJSON *obj = cJSON_GetObjectItemCaseSensitive(root, "main");

    cJSON *parameter;
    cJSON_ArrayForEach(parameter, weather_param) 
    {
    /* Each element is an object with unknown field(s) */
        cJSON *elem;
        cJSON_ArrayForEach(elem, parameter) 
        {
            if(cJSON_IsString(elem) && (memcmp(elem->string, "main", 4) == 0) && elem->valuestring != NULL)
            {
                memcpy(weather_status, elem->valuestring, strlen(elem->valuestring));
                printf("Weather: %s\n", weather_status);  
            }   
        }
    }

    double temp = cJSON_GetObjectItemCaseSensitive(obj, "temp")->valuedouble;
    int pressure = cJSON_GetObjectItemCaseSensitive(obj, "pressure")->valueint;
    int humidity = cJSON_GetObjectItemCaseSensitive(obj, "humidity")->valueint;
    printf("Temperature: %0.00f°C\nPressure: %d hPa\nHumidity: %d%%\n", temp, pressure, humidity);
    
    weather_temp = temp;
    weather_pressure = pressure;
    weather_humidity = humidity;
    printf("Temperature: %0.00f°C\nPressure: %d hPa\nHumidity: %d%%\n", weather_temp, weather_pressure, weather_humidity);

    cJSON_Delete(root);
    free(response_data);
    ESP_LOGI(TAG, "response_data freed!");

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
            ESP_LOGI("OpenWeatherAPI", "Received data: %s", response_data);
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
             "http://api.openweathermap.org/data/2.5/weather?q=",
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
    
        vTaskDelay(pdMS_TO_TICKS(5000));
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