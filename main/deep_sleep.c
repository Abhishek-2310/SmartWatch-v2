#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "common.h"

#define INACTIVITY_TIMEOUT_SECONDS 30  // Adjust as needed
#define uS_TO_S_FACTOR 1000000
// #define MODE_PIN 26

extern uint8_t deep_sleep_reset;
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
    ESP_LOGI(TAG, "Entering Deep sleep");

    esp_err_t saveResult = saveStructToNVS("storage", "alarm", &alarm1);
    if (saveResult == ESP_OK) {
        printf("Struct saved to NVS\n");
    } else {
        printf("Failed to save struct to NVS\n");
    }
    // esp_sleep_enable_timer_wakeup(INACTIVITY_TIMEOUT_SECONDS * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
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
    // esp_err_t getResult = getStructFromNVS("storage", "alarm", &alarm1);

    // if (getResult == ESP_OK) {
    //     printf("Retrieved alarm from NVS: hours=%d, minutes=%d, enabled=%d\n", alarm1.hours, alarm1.minutes, alarm1.enabled);
    // } else {
    //     printf("Failed to retrieve struct from NVS\n");
    // }
    // Initialize your hardware and application
    
    esp_sleep_enable_ext0_wakeup(MODE_PIN, 0); 

    // Create a FreeRTOS task to monitor watch activity
    xTaskCreate(watchActivityMonitor, "watch_activity_task", 2048, NULL, 1, NULL);
}