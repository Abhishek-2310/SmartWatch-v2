#include "common.h"
#include "driver/gpio.h"

#define LONG_PRESS_DELAY 3000 // Button press duration for long action in milliseconds

static const char *TAG = "buttons";

extern Alarm_t alarm1;
extern StopWatch_t stopWatch1;

Mode_t Mode = TIME_MODE;
const Mode_t Mode_Table[4] = {TIME_MODE,
                              WEATHER_MODE,
                              ALARM_MODE,
                              STOPWATCH_MODE};
uint8_t mode_index = 0;

bool led1_state_cmd;
bool led2_state_cmd;

BaseType_t set_hour = pdTRUE;

TaskHandle_t Set_task_handle;
TaskHandle_t Reset_task_handle;
TaskHandle_t EspCommsTask_Handle;
TaskHandle_t ModeTask_Handle;
extern TaskHandle_t StateTask_Handle;
extern TaskHandle_t StopWatchTask_Handle;
extern TaskHandle_t AlarmTask_Handle;

extern bool set_weather_mode;

// extern bool stopWatch_running;
extern bool reset_watch;

extern uint8_t deep_sleep_reset;

extern void Esp_Comms_Task(void *pvParameter);

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

static void IRAM_ATTR mode_interrupt_handler(void *args)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Notify the Set_Rst_task that the button was pressed
    vTaskNotifyGiveFromISR(ModeTask_Handle, &xHigherPriorityTaskWoken);

    // Clear the interrupt flag and exit
    gpio_intr_disable(MODE_PIN);
    gpio_intr_enable(MODE_PIN);
}

static void IRAM_ATTR comms_interrupt_handler(void *args)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Notify the Set_Rst_task that the button was pressed
    vTaskNotifyGiveFromISR(ModeTask_Handle, &xHigherPriorityTaskWoken);
    // vTaskNotifyGiveFromISR(EspCommsTask_Handle, &xHigherPriorityTaskWoken);

    // Clear the interrupt flag and exit
    gpio_intr_disable(COMMS_PIN);
    gpio_intr_enable(COMMS_PIN);
}

/**********************
 *    BUTTON TASKS
 **********************/
void Mode_Task(void *pvParameters)
{
    while (1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_DELAY));

        int mode_button_state = gpio_get_level(MODE_PIN);
        int comms_button_state = gpio_get_level(COMMS_PIN);
        if (mode_button_state == 0)
        {
            mode_index = (mode_index + 1) % 4;
            Mode = Mode_Table[mode_index];
            ESP_LOGI(TAG, "mode_index: %d", mode_index);
            xTaskNotifyGive(StateTask_Handle);
            deep_sleep_reset = 1;
        }

        if (comms_button_state == 0)
        {
            Mode = (Mode == ESP_COMMS_MODE) ? mode_index : ESP_COMMS_MODE;
            ESP_LOGI(TAG, "mode: %d", ESP_COMMS_MODE);
            xTaskNotifyGive(StateTask_Handle);
            deep_sleep_reset = 1;
        }
    }
}

void Set_Task(void *params)
{
    uint32_t button_down_time = 0;
    bool button_pressed = false;
    bool block_task = true;

    while (1)
    {
        if (block_task)
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

                switch (Mode)
                {
                    case ALARM_MODE:
                        alarm1.enabled = true;
                        xTaskNotifyGive(AlarmTask_Handle);
                        ESP_LOGI(TAG, "Enabled Alarm");
                        xTaskNotifyGive(StateTask_Handle);  // Change alarm state button to ON
                        break;
                    
                    default:
                        break;
                }

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
                case ESP_COMMS_MODE:
                    led1_state_cmd = !led1_state_cmd;
                    ESP_LOGI(TAG, "Command LED1 state to: %d", led1_state_cmd);
                    xTaskNotifyGive(EspCommsTask_Handle);
                    break;

                case WEATHER_MODE:

                    set_weather_mode = !set_weather_mode;
                    ESP_LOGI(TAG, "weather mode set to: %d", set_weather_mode);
                    xTaskNotifyGive(StateTask_Handle);
                    break;

                case ALARM_MODE:

                    if (set_hour)
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
                    xTaskNotifyGive(StateTask_Handle);

                    break;

                case STOPWATCH_MODE:
                    // start and stop stopwatch
                    stopWatch1.isRunning = !stopWatch1.isRunning;
                    ESP_LOGI(TAG, "stopwatch state: %d", stopWatch1.isRunning);
                    if (stopWatch1.isRunning)
                        xTaskNotifyGive(StopWatchTask_Handle);
                    xTaskNotifyGive(StateTask_Handle);

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
        if (block_task)
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

                switch (Mode)
                {
                    case ALARM_MODE:
                        alarm1.enabled = false;
                        ESP_LOGI(TAG, "Disabled Alarm");
                        xTaskNotifyGive(StateTask_Handle); // Change alarm state button to OFF
                        break;
                    
                    default:
                        break;
                }

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
                case ESP_COMMS_MODE:
                    led2_state_cmd = !led2_state_cmd;
                    ESP_LOGI(TAG, "Command LED2 state to: %d", led2_state_cmd);
                    xTaskNotifyGive(EspCommsTask_Handle);
                    break;

                case ALARM_MODE:

                    set_hour = !set_hour;
                    ESP_LOGI(TAG, "set hour: %d", set_hour);
                    xTaskNotifyGive(StateTask_Handle);

                    break;

                case STOPWATCH_MODE:
                    // start and stop stopwatch
                    if (!stopWatch1.isRunning)
                    {
                        reset_watch = true;
                        xTaskNotifyGive(StopWatchTask_Handle);
                        xTaskNotifyGive(StateTask_Handle);
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

void button_config(void)
{
    ESP_LOGI(TAG, "Vibration Motor IO Config");
    gpio_config_t io_conf_motor;
    io_conf_motor.intr_type = GPIO_INTR_DISABLE;      // Disable interrupt
    io_conf_motor.mode = GPIO_MODE_OUTPUT;            // Set the GPIO pin as an output
    io_conf_motor.pin_bit_mask = (1ULL << MOTOR_PIN); // Choose the GPIO pin you want to configure (GPIO_NUM_2 in this case)
    io_conf_motor.pull_down_en = 0;                   // Disable pull-down resistor
    io_conf_motor.pull_up_en = 0;                     // Disable pull-up resistor
    gpio_config(&io_conf_motor);

    gpio_config_t io_conf_display_power;
    // Configure the GPIO pin for the button as an input
    io_conf_display_power.intr_type = GPIO_INTR_DISABLE;
    io_conf_display_power.mode = GPIO_MODE_OUTPUT;
    io_conf_display_power.pin_bit_mask = (1ULL << DISPLAY_POWER);
    io_conf_display_power.pull_down_en = 0; // Disable pull-down resistor
    io_conf_display_power.pull_up_en = 0;   // Disable pull-up resistor
    gpio_config(&io_conf_display_power);

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

    gpio_config_t io_conf_comms;
    // Configure the GPIO pin for the button as an input
    io_conf_comms.intr_type = GPIO_INTR_NEGEDGE;
    io_conf_comms.mode = GPIO_MODE_INPUT;
    io_conf_comms.pin_bit_mask = (1ULL << COMMS_PIN);
    io_conf_comms.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf_comms.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf_comms);

    gpio_install_isr_service(0);

    gpio_isr_handler_add(SET_PIN, set_interrupt_handler, (void *)SET_PIN);
    gpio_isr_handler_add(RESET_PIN, reset_interrupt_handler, (void *)RESET_PIN);
    gpio_isr_handler_add(MODE_PIN, mode_interrupt_handler, (void *)MODE_PIN);
    gpio_isr_handler_add(COMMS_PIN, comms_interrupt_handler, (void *)COMMS_PIN);

    xTaskCreate(Set_Task, "Set_Task", 2048, NULL, 1, &Set_task_handle);
    xTaskCreate(Reset_Task, "Reset_Task", 2048, NULL, 1, &Reset_task_handle);
    xTaskCreate(Mode_Task, "Mode_Task", 2048, NULL, 1, &ModeTask_Handle);
    xTaskCreate(Esp_Comms_Task, "Esp_Comms_Task", 1024*3, NULL, 1, &EspCommsTask_Handle);
}