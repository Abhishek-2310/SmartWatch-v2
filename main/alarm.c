#include "driver/gpio.h"
#include "common.h"

/**********************
 *  GLOBAL VARIABLES
 **********************/
Alarm_t alarm1 = {12,30};
uint8_t count = 0;
BaseType_t set_hour = pdTRUE;
static const char *TAG = "alarm";
extern uint8_t buttonPressed;


TaskHandle_t AlarmTask_Handle;
TaskHandle_t Set_Rst_task_handle;

/**********************
 * INTERRUPT CALLBACKS
 **********************/
static void IRAM_ATTR set_rst_interrupt_handler(void *args)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Notify the Set_Rst_task that the button was pressed
    vTaskNotifyGiveFromISR(Set_Rst_task_handle, &xHigherPriorityTaskWoken);

    // Clear the interrupt flag and exit
    gpio_intr_disable(SET_PIN);
    gpio_intr_enable(SET_PIN);
    gpio_intr_disable(RESET_PIN);
    gpio_intr_enable(RESET_PIN);
}


void Alarm_Task(void * pvParameters)
{
    time_t now;
    struct tm timeinfo;

    while(1)
    {
        time(&now);
        localtime_r(&now, &timeinfo);

        if(timeinfo.tm_hour == alarm1.hours && timeinfo.tm_min == alarm1.minutes)
            ESP_LOGI(TAG, "Alarm time match!!!");

        vTaskDelay(pdMS_TO_TICKS(1000));

    }
    vTaskDelete(NULL);
}


void Set_Rst_Task(void *params)
{

    while (1) 
    {
        // Block on entry until notification from mode recieved
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Wait for a short debounce delay
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_DELAY));

        int set_button_state = gpio_get_level(SET_PIN);
        int reset_button_state = gpio_get_level(RESET_PIN);
        if (set_button_state == 0) 
        {
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
            
        }
        if (reset_button_state == 0) 
        {
            set_hour = !set_hour;
            ESP_LOGI(TAG, "Set hour: %d", set_hour);
        }

        buttonPressed = 1;

    }
}


void alarm_config(void)
{
    ESP_LOGI(TAG, "PushButton Config");

    gpio_config_t io_conf_set, io_conf_rst, io_conf_mode;
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

     // Configure the GPIO pin for the button as an input
    io_conf_mode.intr_type = GPIO_INTR_NEGEDGE;
    io_conf_mode.mode = GPIO_MODE_INPUT;
    io_conf_mode.pin_bit_mask = (1ULL << MODE_PIN);
    io_conf_mode.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf_mode.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf_mode);

     // Configure the GPIO pin for the button as an input
    io_conf_mode.intr_type = GPIO_INTR_NEGEDGE;
    io_conf_mode.mode = GPIO_MODE_INPUT;
    io_conf_mode.pin_bit_mask = (1ULL << COMMS_PIN);
    io_conf_mode.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf_mode.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf_mode);

    gpio_install_isr_service(0);

    gpio_isr_handler_add(SET_PIN, set_rst_interrupt_handler, (void *)SET_PIN);
    gpio_isr_handler_add(RESET_PIN, set_rst_interrupt_handler, (void *)RESET_PIN);

    xTaskCreate(Set_Rst_Task, "Set_Rst_Task", 2048, NULL, 1, &Set_Rst_task_handle);
    xTaskCreate(Alarm_Task, "Alarm_Task", 2048, NULL, 1, &AlarmTask_Handle);
}