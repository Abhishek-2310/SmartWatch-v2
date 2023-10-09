#ifndef COMMON_H
#define COMMON_H

/*********************
 *      DEFINES
 *********************/
#define SET_PIN 19
#define MODE_PIN  26
#define RESET_PIN 18
#define DEBOUNCE_DELAY 30   // in ms
/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include <time.h>

/**********************
 *      STRUCTS
 **********************/
typedef struct
{
    uint8_t hours;
    uint8_t minutes;
}Alarm_t;

/**********************
 *      ENUMS
 **********************/
typedef enum
{
    TIME_MODE,
    WEATHER_MODE,
    ALARM_MODE,
    STOPWATCH_MODE
}Mode_t;


#endif