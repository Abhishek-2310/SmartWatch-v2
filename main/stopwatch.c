#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "common.h"

extern uint8_t deep_sleep_reset;

esp_timer_handle_t timer;
// bool stopWatch_running = false;
bool start_watch = false;
bool reset_watch = false;
uint64_t elapsed_time = 0;
StopWatch_t stopWatch1;

TaskHandle_t StopWatchTask_Handle;
extern TaskHandle_t StateTask_Handle;


void timer_callback(void* arg) {
    // if (stopWatch_running) {
        elapsed_time++;
    // }
}


void stopwatch_task(void *pvParameters) 
{
    while (1) 
    {
        if (!stopWatch1.isRunning) 
        {
            if(start_watch)
            {
                // paused_time = elapsed_time - start_time;
                printf("Stopwatch paused\n");
                esp_timer_stop(timer);
            }

            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

            if(reset_watch)
            {
                elapsed_time = 0;
                printf("Stopwatch resetted\n");
                reset_watch = false;
                start_watch = false;
            }
            else
            {            
                printf("Stopwatch started\n");
                esp_timer_start_periodic(timer, 1000000); // 1 centisecond
                start_watch = true;
            }
        }

        deep_sleep_reset = 1;
        stopWatch1.seconds = (elapsed_time ) % 60;
        stopWatch1.minutes = (elapsed_time / 60) % 60;
        printf("minutes: %d mins\n", stopWatch1.minutes);
        printf("seconds: %d secs\n", stopWatch1.seconds);

        xTaskNotifyGive(StateTask_Handle);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


void stopWatch_config()
{
    stopWatch1.isRunning = false;
    esp_timer_create_args_t timer_config = {
        .callback = &timer_callback,
        .name = "stopwatch_timer"
    };
    esp_timer_create(&timer_config, &timer);

    xTaskCreate(stopwatch_task, "stopwatch_task", 2048, NULL, 0, &StopWatchTask_Handle);

    // esp_timer_start_periodic(timer, 10000); // 1 centisecond
    // start_time = esp_timer_get_time();

    // stopWatch_running = !stopWatch_running;
}
