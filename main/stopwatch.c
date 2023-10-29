#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "common.h"


esp_timer_handle_t timer;
bool stopWatch_running = false;
bool start_watch = false;
bool reset_watch = false;
uint64_t elapsed_time = 0;
StopWatch_t stopWatch1;

TaskHandle_t StopWatchTask_Handle;


void timer_callback(void* arg) {
    // if (stopWatch_running) {
        elapsed_time++;
    // }
}


void stopwatch_task(void *pvParameters) 
{
    while (1) 
    {
        if (!stopWatch_running) 
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
                esp_timer_start_periodic(timer, 10000); // 1 centisecond
                start_watch = true;
            }
        }

        stopWatch1.centiSeconds = (elapsed_time) % 100;
        stopWatch1.seconds = (elapsed_time / 100) % 60;
        stopWatch1.minutes = (elapsed_time / ( 100 * 60 )) % 60;
        printf("minutes: %d mins\n", stopWatch1.minutes);
        printf("seconds: %d secs\n", stopWatch1.seconds);
        printf("centiSeconds: %d centisecs\n", stopWatch1.centiSeconds);

        vTaskDelay(pdMS_TO_TICKS(1010));
    }
}


void stopWatch_config()
{
    esp_timer_create_args_t timer_config = {
        .callback = &timer_callback,
        .name = "stopwatch_timer"
    };
    esp_timer_create(&timer_config, &timer);

    xTaskCreate(stopwatch_task, "stopwatch_task", 2048, NULL, 5, &StopWatchTask_Handle);

    // esp_timer_start_periodic(timer, 10000); // 1 centisecond
    // start_time = esp_timer_get_time();

    // stopWatch_running = !stopWatch_running;
}
