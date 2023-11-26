#include "common.h"
#include "driver/gpio.h"


/**********************
 *  GLOBAL VARIABLES
 **********************/
Alarm_t alarm1;
bool alarm_match = false;
static const char *TAG = "alarm";

TaskHandle_t AlarmTask_Handle;
TaskHandle_t VibrationAlertTask_Handle;
extern TaskHandle_t StateTask_Handle;

extern RTC_DATA_ATTR Mode_t Mode;


void Vibration_Alert_Task(void * pvParameters)
{
    bool motor_state = false;
    while(1)
    {
        if(!alarm1.enabled || !alarm_match)
        {
            ESP_LOGI(TAG, "Vibration task blocked!");
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }
        ESP_LOGI(TAG, "Vibration task running! %d", alarm1.enabled);

        motor_state = !motor_state;
        gpio_set_level(MOTOR_PIN, motor_state);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    vTaskDelete(NULL);
}


void Alarm_Task(void * pvParameters)
{
    time_t now;
    struct tm timeinfo;

    while(1)
    {
        if(alarm1.enabled == 0)
        {
            ESP_LOGI(TAG, "Alarm task blocked!");
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }

        ESP_LOGI(TAG, "Alarm task running! %d", alarm1.enabled);
        

        time(&now);
        localtime_r(&now, &timeinfo);

        if(timeinfo.tm_hour == alarm1.hours && timeinfo.tm_min == alarm1.minutes)
        {
            ESP_LOGI(TAG, "Alarm time match!!!");
            Mode = ALARM_MODE;
            alarm_match = true;
            xTaskNotifyGive(StateTask_Handle);

            xTaskNotifyGive(VibrationAlertTask_Handle);
        }    

        vTaskDelay(pdMS_TO_TICKS(5000));

    }
    vTaskDelete(NULL);
}



void alarm_config(void)
{
    xTaskCreate(Alarm_Task, "Alarm_Task", 2048, NULL, 1, &AlarmTask_Handle);
    xTaskCreate(Vibration_Alert_Task, "Vibration_Alert_Task", 2048, NULL, 0, &VibrationAlertTask_Handle);
}