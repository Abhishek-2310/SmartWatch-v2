#include "driver/gpio.h"
#include "common.h"

#define LONG_PRESS_DELAY    3000   // Button press duration for long action in milliseconds

/**********************
 *  GLOBAL VARIABLES
 **********************/
extern RTC_DATA_ATTR Mode_t Mode; 
extern Alarm_t alarm1;
extern uint8_t deep_sleep_reset;
extern bool set_weather_mode;

extern bool stopWatch_running;
extern bool pause_watch;
extern bool reset_watch;

BaseType_t set_hour = pdTRUE;

static const char *TAG = "alarm";

TaskHandle_t AlarmTask_Handle;
TaskHandle_t Set_task_handle;
TaskHandle_t Reset_task_handle;
extern TaskHandle_t StateTask_Handle;
extern TaskHandle_t StopWatchTask_Handle;

/**********************
 * INTERRUPT CALLBACKS
 **********************/
static void IRAM_ATTR set_interrupt_handler(void *args)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Notify the Set_task that the button was pressed
    vTaskNotifyGiveFromISR(Set_task_handle, &xHigherPriorityTaskWoken);

    // Clear the interrupt flag and exit
    gpio_intr_disable(SET_PIN);
    gpio_intr_enable(SET_PIN);
}

static void IRAM_ATTR reset_interrupt_handler(void *args)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Notify the Set_task that the button was pressed
    vTaskNotifyGiveFromISR(Reset_task_handle, &xHigherPriorityTaskWoken);

    // Clear the interrupt flag and exit
    gpio_intr_disable(RESET_PIN);
    gpio_intr_enable(RESET_PIN);
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
            ESP_LOGI(TAG, "Alarm time match!!!");

        vTaskDelay(pdMS_TO_TICKS(5000));

    }
    vTaskDelete(NULL);
}


void Set_Task(void *params)
{
    uint32_t button_down_time = 0;
    bool button_pressed = false;
    bool block_task = true;

    while (1) 
    {
        if(block_task)
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_DELAY));

        if (gpio_get_level(SET_PIN) == 0) // Button is pressed
        {
            if (!button_pressed) // Button was just pressed
            {
                button_down_time = xTaskGetTickCount();
                button_pressed = true;
                block_task = false;
            }

            // Check if the button is held for a long time
            if ((xTaskGetTickCount() - button_down_time) >= (LONG_PRESS_DELAY / portTICK_PERIOD_MS))
            {
                ESP_LOGI(TAG, "Set Long Press");
                alarm1.enabled = 1;
                xTaskNotifyGive(AlarmTask_Handle);
                ESP_LOGI(TAG, "Enabled Alarm");
                button_pressed = false; // Release the button
                block_task = true;
            }
        }
        else // Button is not pressed
        {
            // Check if the button was pressed for a short duration
            if (button_pressed)
            {
                ESP_LOGI(TAG, "Set Short Press");
                switch (Mode)
                {
                    case WEATHER_MODE:

                        set_weather_mode = !set_weather_mode;
                        ESP_LOGI(TAG, "weather mode set to: %d", set_weather_mode);
                        xTaskNotifyGive(StateTask_Handle);
                        break;

                    case ALARM_MODE:

                        if(set_hour)
                        {
                            alarm1.hours = (alarm1.hours + 1) % 24;
                            ESP_LOGI(TAG, "Hours Inc Button pressed!, hours: %d", alarm1.hours);
                            ESP_LOGI(TAG, "minutes: %d", alarm1.minutes);
                        }
                        else
                        {
                            alarm1.minutes = (alarm1.minutes + 1) % 60;
                            ESP_LOGI(TAG, "hours: %d", alarm1.hours);
                            ESP_LOGI(TAG, "Minutes Inc Button pressed!, minutes: %d", alarm1.minutes);
                        }

                        break;
                    
                    case STOPWATCH_MODE:
                        // start and stop stopwatch
                        stopWatch_running = !stopWatch_running;
                        ESP_LOGI(TAG, "stopwatch state: %d", stopWatch_running);
                        if(stopWatch_running)
                            xTaskNotifyGive(StopWatchTask_Handle);

                        break;

                    default:
                        break;
                }
                // Perform the short-press action here
                
                button_pressed = false; // Release the button
            }

            block_task = true;
        }

        deep_sleep_reset = 1;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void Reset_Task(void *params)
{
    uint32_t button_down_time = 0;
    bool button_pressed = false;
    bool block_task = true;

    while (1) 
    {
        if(block_task)
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_DELAY));

        if (gpio_get_level(RESET_PIN) == 0) // Button is pressed
        {
            if (!button_pressed) // Button was just pressed
            {
                button_down_time = xTaskGetTickCount();
                button_pressed = true;
                block_task = false;
            }

            // Check if the button is held for a long time
            if ((xTaskGetTickCount() - button_down_time) >= (LONG_PRESS_DELAY / portTICK_PERIOD_MS))
            {
                ESP_LOGI(TAG, "Reset Long Press");
                alarm1.enabled = false;
                ESP_LOGI(TAG, "Disabled Alarm");
                button_pressed = false; // Release the button
                block_task = true;
            }
        }
        else // Button is not pressed
        {
            // Check if the button was pressed for a short duration
            if (button_pressed)
            {
                // Perform the short-press action here
                ESP_LOGI(TAG, "Reset Short Press");

                switch (Mode)
                {
                    case ALARM_MODE:

                        set_hour = !set_hour;
                        ESP_LOGI(TAG, "set hour: %d", set_hour);

                        break;
                    
                    case STOPWATCH_MODE:
                        // start and stop stopwatch
                        if(!stopWatch_running)
                        {
                            reset_watch = true;
                            xTaskNotifyGive(StopWatchTask_Handle);
                        }
                        break;

                    default:
                        break;
                }

                
                button_pressed = false; // Release the button
            }

            block_task = true;
        }

        deep_sleep_reset = 1;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void alarm_config(void)
{
    ESP_LOGI(TAG, "PushButton Config");

    gpio_config_t io_conf_set, io_conf_rst;
    // Configure the GPIO pin for the button as an input
    io_conf_set.intr_type = GPIO_INTR_NEGEDGE;
    io_conf_set.mode = GPIO_MODE_INPUT;
    io_conf_set.pin_bit_mask = (1ULL << SET_PIN);
    io_conf_set.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf_set.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf_set);

    // Configure the GPIO pin for the button as an input
    io_conf_rst.intr_type = GPIO_INTR_NEGEDGE;
    io_conf_rst.mode = GPIO_MODE_INPUT;
    io_conf_rst.pin_bit_mask = (1ULL << RESET_PIN);
    io_conf_rst.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf_rst.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf_rst);

    gpio_config_t io_conf_mode;
     // Configure the GPIO pin for the button as an input
    io_conf_mode.intr_type = GPIO_INTR_NEGEDGE;
    io_conf_mode.mode = GPIO_MODE_INPUT;
    io_conf_mode.pin_bit_mask = (1ULL << MODE_PIN);
    io_conf_mode.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf_mode.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf_mode);

    //  // Configure the GPIO pin for the button as an input
    // io_conf_mode.intr_type = GPIO_INTR_NEGEDGE;
    // io_conf_mode.mode = GPIO_MODE_INPUT;
    // io_conf_mode.pin_bit_mask = (1ULL << COMMS_PIN);
    // io_conf_mode.pull_up_en = GPIO_PULLUP_ENABLE;
    // io_conf_mode.pull_down_en = GPIO_PULLDOWN_DISABLE;
    // gpio_config(&io_conf_mode);

    gpio_install_isr_service(0);

    gpio_isr_handler_add(SET_PIN, set_interrupt_handler, (void *)SET_PIN);
    gpio_isr_handler_add(RESET_PIN, reset_interrupt_handler, (void *)RESET_PIN);

    xTaskCreate(Set_Task, "Set_Task", 2048, NULL, 1, &Set_task_handle);
    xTaskCreate(Reset_Task, "Reset_Task", 2048, NULL, 1, &Reset_task_handle);
    xTaskCreate(Alarm_Task, "Alarm_Task", 2048, NULL, 1, &AlarmTask_Handle);
}