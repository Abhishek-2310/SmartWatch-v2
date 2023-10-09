#ifndef COMMON_H
#define COMMON_H

/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>

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