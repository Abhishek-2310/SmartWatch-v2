#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

static const char * TAG = "Battery-Monitor";

static esp_adc_cal_characteristics_t adc1_chars;


void Battery_Monitor_Task(void* pvParameter)
{
    while (1) 
    {
        int adc_value = adc1_get_raw(ADC1_CHANNEL_4);
        float voltage = (adc_value / (float)(1 << 12)) * 3.3;
        ESP_LOGI(TAG, "Raw value: %i, Voltage: %.2f V", adc_value, voltage);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
    vTaskDelete(NULL);
}

void battery_monitor_config(void)
{
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_DEFAULT, 0, &adc1_chars);

    adc1_config_width(ADC_WIDTH_BIT_DEFAULT);
    adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_DB_11);

    xTaskCreate(Battery_Monitor_Task, "Battery_Monitor_Task", 2048, NULL, 1, NULL);
}
