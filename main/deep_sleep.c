#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "common.h"
#include "time.h"

#include "protocol_examples_common.h"

#define INACTIVITY_TIMEOUT_SECONDS 60  // Adjust as needed
#define uS_TO_S_FACTOR 1000000

// #define MODE_PIN 26
extern bool wifi_connected;
uint8_t deep_sleep_reset;
extern Alarm_t alarm1;

static const char *TAG = "deep-sleep";
/* Variable holding number of times ESP32 restarted since first boot.
 * It is placed into RTC memory using RTC_DATA_ATTR and
 * maintains its value when ESP32 wakes from deep sleep.
 */

// Function to save the struct into NVS
esp_err_t saveStructToNVS(const char *namespace, const char *key, Alarm_t *data) {
    nvs_handle_t nvsHandle;
    esp_err_t err = nvs_open(namespace, NVS_READWRITE, &nvsHandle);

    if (err == ESP_OK) {
        // Serialize the struct into bytes
        err = nvs_set_blob(nvsHandle, key, data, sizeof(Alarm_t));
        if (err == ESP_OK) {
            err = nvs_commit(nvsHandle); // Save the data
        }
        nvs_close(nvsHandle);
    }

    return err;
}

// Function to retrieve the struct from NVS
esp_err_t getStructFromNVS(const char *namespace, const char *key, Alarm_t *data) {
    nvs_handle_t nvsHandle;
    esp_err_t err = nvs_open(namespace, NVS_READONLY, &nvsHandle);

    if (err == ESP_OK) {
        // Get the serialized struct from NVS
        size_t blobSize = sizeof(Alarm_t);
        err = nvs_get_blob(nvsHandle, key, data, &blobSize);
        nvs_close(nvsHandle);
    }

    return err;
}


void enterDeepSleep() {
    // Configure the sleep timer and enter deep sleep
    // ESP_LOGI(TAG, "Entering Deep sleep");
    if(alarm1.enabled)
    {
        esp_err_t saveAlarm = saveStructToNVS("storage", "alarm", &alarm1);
        if (saveAlarm == ESP_OK) {
            printf("Alarm Struct saved to NVS\n");
        } else {
            printf("Failed to save struct to NVS\n");
        }
    }
    printf("Disonnecting WiFi\n");
    if(wifi_connected)
        ESP_ERROR_CHECK( example_disconnect() );

    printf("Entering Deep sleep\n");
    esp_deep_sleep_start();
}

void setRTCAlarm(void)
{
    int64_t wakeup_time;
    int8_t neg_hour_adjust = 0; 
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);

    if(alarm1.hours < timeinfo.tm_hour)
    {
        ESP_LOGI(TAG, "Hour adjusted");
        neg_hour_adjust = 24;
    }
    // else
    //     neg_hour_adjust = 0;

    wakeup_time = (((alarm1.hours + neg_hour_adjust - (int8_t) timeinfo.tm_hour) * 3600 + (alarm1.minutes - (int8_t) timeinfo.tm_min ) * 60) - 40) * 1000000;

    printf("wakeup time: %lld\n", wakeup_time);
    if(wakeup_time < 0)
    {
        ESP_LOGI(TAG, "Alarm turned OFF");
        alarm1.enabled = false;
    }
    else
    {
        ESP_LOGI(TAG, "wakeup time set");
        esp_sleep_enable_timer_wakeup(wakeup_time);
    }
}

void watchActivityMonitor(void* pvParameter)
{
    TickType_t lastActivityTime = xTaskGetTickCount();

    while (1) 
    {
        // Check for smartwatch activity
        // If active, reset the inactivity timer
        // If inactive for the specified timeout, enter deep sleep
        if (deep_sleep_reset) 
        {
            ESP_LOGI(TAG, "Reset Deep sleep counter");
            lastActivityTime = xTaskGetTickCount(); // Reset the inactivity timer
            deep_sleep_reset = 0;
        } 
        else 
        {
            // Check if the inactivity timeout has been reached
            if ((xTaskGetTickCount() - lastActivityTime) >= (INACTIVITY_TIMEOUT_SECONDS * configTICK_RATE_HZ)) {
                // Smartwatch is inactive, enter deep sleep
                gpio_set_level(DISPLAY_POWER, 0);
                if(alarm1.enabled)
                {
                    setRTCAlarm();
                }
                // gpio_set_level(MOTOR_PIN, 1);
                // vTaskDelay(2000);
                // gpio_set_level(MOTOR_PIN, 0);

                enterDeepSleep();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Adjust the polling interval as needed
    }
}


void deep_sleep_config(void) 
{
    deep_sleep_reset = 0;

// Retrieve the struct from NVS
    esp_err_t getResult = getStructFromNVS("storage", "alarm", &alarm1);

    if (getResult == ESP_OK) {
        printf("Retrieved alarm from NVS: hours=%d, minutes=%d, enabled=%d\n", alarm1.hours, alarm1.minutes, alarm1.enabled);
    } else {
        printf("Failed to retrieve struct from NVS\n");
    }
    // Initialize your hardware and application
    
    esp_sleep_enable_ext0_wakeup(SET_PIN, 0); 

    // Create a FreeRTOS task to monitor watch activity
    xTaskCreate(watchActivityMonitor, "watch_activity_task", 2048, NULL, 1, NULL);
}