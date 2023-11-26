#ifndef COMMON_H
#define COMMON_H

/*********************
 *      DEFINES
 *********************/
#define DISPLAY_POWER 27
#define SET_PIN 19
#define MODE_PIN  26
#define RESET_PIN 18
#define COMMS_PIN 25
#define MOTOR_PIN 23
#define DEBOUNCE_DELAY 50   // in ms
/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

/**********************
 *      STRUCTS
 **********************/
typedef struct
{
    int8_t hours;
    int8_t minutes;
    uint8_t enabled;
}Alarm_t;

typedef struct 
{
    uint8_t minutes;
    uint8_t seconds;
    uint8_t centiSeconds;
    /* data */
}StopWatch_t;
/**********************
 *      ENUMS
 **********************/
typedef enum
{
    ESP_COMMS_MODE = -1,
    TIME_MODE,
    WEATHER_MODE,
    ALARM_MODE,
    STOPWATCH_MODE
}Mode_t;

#endif