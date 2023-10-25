/*********************
 *      INCLUDES
 *********************/
#include <string.h>
#include <sys/time.h>
#include <ctype.h>

#include "lvgl.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "common.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_display_time_create(lv_obj_t * parent);
static void lv_display_weather_mode0_create(lv_obj_t * parent);
static void lv_display_alarm_create(lv_timer_t * timer);
static void State_task(void * pvParameters);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_obj_t * tv;
static lv_obj_t * t1;
static lv_obj_t * t2;
static lv_obj_t * t3;
static lv_obj_t * t4;

lv_timer_t * alarm_timer;
/**********************
 * EXTERNAL VARIABLES
 **********************/
extern char strftime_buf[64];

extern char weather_status[10];
extern double weather_temp;
extern double weather_speed;
extern int weather_pressure;
extern int weather_humidity;

extern char description_array[4][10];

extern RTC_DATA_ATTR Mode_t Mode;

bool set_weather_mode = 0;
extern Alarm_t alarm1;
TaskHandle_t StateTask_Handle;
extern SemaphoreHandle_t xGuiSemaphore;
/**********************
 *        TAGS
 **********************/
static const char *TAG = "task_changer";


/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_task_modes(void)
{
    /*Create a Tab view object*/

    tv = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 50);
    // lv_obj_add_event_cb(lv_tabview_get_content(tv), CbScrollBeginEvent, LV_EVENT_SCROLL_BEGIN, NULL);
	lv_obj_clear_flag(lv_tabview_get_content(tv), LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * tab_buttons = lv_tabview_get_tab_btns(tv);
    lv_obj_add_flag(tab_buttons, LV_OBJ_FLAG_HIDDEN);

    t1 = lv_tabview_add_tab(tv, "Time");
    t2 = lv_tabview_add_tab(tv, "Weather_mode0");
    t3 = lv_tabview_add_tab(tv, "Weather_mode1");
    t4 = lv_tabview_add_tab(tv, "Alarm");

    // Background Style to turn it black
    static lv_style_t style_screen;
    lv_style_init(&style_screen);
    lv_style_set_bg_color(&style_screen, lv_color_black());
    lv_obj_add_style(tv, &style_screen, 0); 

    // lv_display_time_create(t1);
    alarm_timer = lv_timer_create(lv_display_alarm_create, 500, t4);
    lv_timer_pause(alarm_timer);
    // lv_display_weather_mode0_create(t2);

    // lv_timer_create(State_task, 5000, NULL);
    xTaskCreatePinnedToCore(State_task, "State", 2048*2, NULL, 0, &StateTask_Handle, 1);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/


static void lv_display_time_create(lv_obj_t * parent)
{
    // lv_page_set_scrl_layout(parent, LV_LAYOUT_PRETTY_TOP);
    ESP_LOGI(TAG, "lv_display time tab");

    time_t now;
    struct tm timeinfo;
    time(&now);

    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in Canada is: %s", strftime_buf);

    // String slicing to get day, date and time
    char * day_str = strftime_buf;
    *(day_str + 3) = '\0';

    int i = 0;

    while (*(day_str + i))
    {
        *(day_str + i) = toupper(*(day_str + i));
        i++;
    }
    ESP_LOGI(TAG, "The current day is: %s", day_str);

    char * date_str = &strftime_buf[8];
    *(date_str + 2) = '\0';
    ESP_LOGI(TAG, "The current date is: %s", date_str);

    char * tim_str = &strftime_buf[11];
    *(tim_str + 5) = '\0';
    ESP_LOGI(TAG, "The current time is: %s", tim_str);

    // Clean the view
    lv_obj_clean(parent);

    // time label
    static lv_style_t style_time;
    lv_style_init(&style_time);
	lv_obj_t *label_time = lv_label_create(parent);
    
    lv_style_set_text_font(&style_time, &lv_font_montserrat_48); 
    lv_style_set_text_color(&style_time, lv_palette_main(LV_PALETTE_LIGHT_BLUE));
    lv_style_set_text_letter_space(&style_time, 5);

    lv_obj_add_style(label_time, &style_time, 0);
    lv_label_set_text(label_time, tim_str);  // set text

    lv_obj_align(label_time, LV_ALIGN_TOP_MID, 0, 50);

    // day label
    static lv_style_t style_day;
    lv_style_init(&style_day);
    lv_obj_t *label_day = lv_label_create(parent);

    lv_style_set_text_font(&style_day, &lv_font_montserrat_36); 
    lv_style_set_text_color(&style_day, lv_color_white());
    lv_style_set_text_letter_space(&style_day, 2);

    lv_obj_add_style(label_day, &style_day, 0);
    lv_label_set_text(label_day, day_str);  // set text

    lv_obj_align(label_day, LV_ALIGN_CENTER, 0, 30);

    // date label
    static lv_style_t style_date;
    lv_style_init(&style_date);
    lv_obj_t *label_date = lv_label_create(parent);

    lv_style_set_text_font(&style_date, &lv_font_montserrat_48); 
    lv_style_set_text_color(&style_date, lv_palette_main(LV_PALETTE_LIGHT_BLUE));
    lv_style_set_text_letter_space(&style_date, 2);

    lv_obj_add_style(label_date, &style_date, 0);
    lv_label_set_text(label_date, date_str);  // set text

    lv_obj_align(label_date, LV_ALIGN_CENTER, 0, 80);
// #if LV_DEMO_WIDGETS_SLIDESHOW
//     tab_content_anim_create(parent);
// #endif
}

static void lv_display_weather_mode0_create(lv_obj_t * parent)
{

    ESP_LOGI(TAG, "lv_display weather mode 0");

    lv_obj_clean(parent);

    char weather_display_buffer[80];

    static lv_style_t style_weather_status;
    lv_style_init(&style_weather_status);
	lv_obj_t * label_weather_status = lv_label_create(parent);
    
    lv_style_set_text_font(&style_weather_status, &lv_font_montserrat_24); 
    lv_style_set_text_color(&style_weather_status, lv_palette_main(LV_PALETTE_YELLOW));
    lv_style_set_text_letter_space(&style_weather_status, 2);

    lv_obj_add_style(label_weather_status, &style_weather_status, 0);
    lv_label_set_text(label_weather_status, weather_status);  // set text

    lv_obj_align(label_weather_status, LV_ALIGN_TOP_MID, 0, 60);

    snprintf(weather_display_buffer, 80, "Temp      : %0.00f°C\nPressure : %d mBar\nHumidity: %d%%\nSpeed     :%0.0f mph", weather_temp, weather_pressure, weather_humidity, weather_speed);

    static lv_style_t style_weather_temp;
    lv_style_init(&style_weather_temp);
	lv_obj_t * label_weather_temp = lv_label_create(parent);
    
    lv_style_set_text_font(&style_weather_temp, &lv_font_montserrat_16); 
    lv_style_set_text_color(&style_weather_temp, lv_color_white());
    lv_style_set_text_letter_space(&style_weather_temp, 2);

    lv_obj_add_style(label_weather_temp, &style_weather_temp, 0);
    lv_label_set_text(label_weather_temp, weather_display_buffer);  // set text

    lv_obj_align(label_weather_temp, LV_ALIGN_CENTER, 0, 40);
}


static void lv_display_weather_mode1_create(lv_obj_t * parent)
{

    ESP_LOGI(TAG, "lv_display weather mode 1");

    lv_obj_clean(parent);

    char weather_mode1_display_buffer[5][30];

    snprintf(weather_mode1_display_buffer[0], 30, "Day %d   %s\n", 0, weather_status);

    static lv_style_t style_weather_status_mode1;
    lv_style_init(&style_weather_status_mode1);
	lv_obj_t * label_weather_status_mode1_day0 = lv_label_create(parent);
    
    lv_style_set_text_font(&style_weather_status_mode1, &lv_font_montserrat_24); 
    lv_style_set_text_color(&style_weather_status_mode1, lv_palette_main(LV_PALETTE_YELLOW));
    lv_style_set_text_letter_space(&style_weather_status_mode1, 2);

    lv_obj_add_style(label_weather_status_mode1_day0, &style_weather_status_mode1, 0);
    lv_label_set_text(label_weather_status_mode1_day0, weather_mode1_display_buffer[0]);  // set text

    lv_obj_align(label_weather_status_mode1_day0, LV_ALIGN_TOP_MID, 0, 40);


    snprintf(weather_mode1_display_buffer[1], 30, "Day %d   %s\n", 1, description_array[0]);

	lv_obj_t * label_weather_status_mode1_day1 = lv_label_create(parent);

    lv_obj_add_style(label_weather_status_mode1_day1, &style_weather_status_mode1, 0);
    lv_label_set_text(label_weather_status_mode1_day1, weather_mode1_display_buffer[1]);  // set text

    lv_obj_align(label_weather_status_mode1_day1, LV_ALIGN_CENTER, 0, -40);


    snprintf(weather_mode1_display_buffer[2], 30, "Day %d   %s\n", 2, description_array[1]);

	lv_obj_t * label_weather_status_mode1_day2 = lv_label_create(parent);

    lv_obj_add_style(label_weather_status_mode1_day2, &style_weather_status_mode1, 0);
    lv_label_set_text(label_weather_status_mode1_day2, weather_mode1_display_buffer[2]);  // set text

    lv_obj_align(label_weather_status_mode1_day2, LV_ALIGN_CENTER, 0, 0);


    snprintf(weather_mode1_display_buffer[3], 30, "Day %d   %s\n", 3, description_array[2]);

	lv_obj_t * label_weather_status_mode1_day3 = lv_label_create(parent);

    lv_obj_add_style(label_weather_status_mode1_day3, &style_weather_status_mode1, 0);
    lv_label_set_text(label_weather_status_mode1_day3, weather_mode1_display_buffer[3]);  // set text

    lv_obj_align(label_weather_status_mode1_day3, LV_ALIGN_CENTER, 0, 40);


    snprintf(weather_mode1_display_buffer[4], 30, "Day %d   %s", 4, description_array[3]);

	lv_obj_t * label_weather_status_mode1_day4 = lv_label_create(parent);

    lv_obj_add_style(label_weather_status_mode1_day4, &style_weather_status_mode1, 0);
    lv_label_set_text(label_weather_status_mode1_day4, weather_mode1_display_buffer[4]);  // set text

    lv_obj_align(label_weather_status_mode1_day4, LV_ALIGN_BOTTOM_MID, 0, -40);

    // for(uint8_t i = 0; i < 5; i++)
    // {
    //     if(i == 0)
    //     {

    //     }
    // }
}


static void lv_display_alarm_create(lv_timer_t * timer)
{
    ESP_LOGI(TAG, "lv_display alarm");

    lv_obj_clean(timer->user_data);

    static lv_style_t style_alarm_title;
    lv_style_init(&style_alarm_title);
	lv_obj_t * label_alarm_title = lv_label_create(timer->user_data);
    
    lv_style_set_text_font(&style_alarm_title, &lv_font_montserrat_24); 
    lv_style_set_text_color(&style_alarm_title, lv_color_white());
    lv_style_set_text_letter_space(&style_alarm_title, 2);

    lv_obj_add_style(label_alarm_title, &style_alarm_title, 0);
    lv_label_set_text(label_alarm_title, "Set alarm");  // set text

    lv_obj_align(label_alarm_title, LV_ALIGN_TOP_MID, 0, 60);

    char alarm_buffer[10];

    snprintf(alarm_buffer, 10, "%02d:%02d", alarm1.hours, alarm1.minutes);

    static lv_style_t style_alarm;
    lv_style_init(&style_alarm);
	lv_obj_t * label_alarm = lv_label_create(timer->user_data);
    
    lv_style_set_text_font(&style_alarm, &lv_font_montserrat_48); 
    lv_style_set_text_color(&style_alarm, lv_palette_main(LV_PALETTE_YELLOW));
    lv_style_set_text_letter_space(&style_alarm, 2);

    lv_obj_add_style(label_alarm, &style_alarm, 0);
    lv_label_set_text(label_alarm, alarm_buffer);  // set text

    lv_obj_align(label_alarm, LV_ALIGN_CENTER, 0, 0);
    
}
// static void tab_changer_task_cb(lv_timer_t * task)
// {
//     ESP_LOGI(TAG, "Switching tasks");

//     uint16_t act = lv_tabview_get_tab_act(tv);
//     act++;
//     if(act >= 2) act = 0;

//     lv_tabview_set_act(tv, act, LV_ANIM_ON);

//     switch(act) {
//     case 0:
//         lv_display_time_create(t1);
//         // tab_content_anim_create(t1);
//         break;
//     case 1:
//         lv_display_weather_mode0_create(t2);
//         // tab_content_anim_create(t2);
//         break;
//     }
// }

static void State_task(void * pvParameters)
{

    while(1)
    {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10000));

        if(pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))
        {
            switch(Mode)
            {
                case TIME_MODE:
                    ESP_LOGI(TAG, "Time State");
                    lv_tabview_set_act(tv, 0, LV_ANIM_ON);
                    lv_display_time_create(t1);
                    break;

                case WEATHER_MODE:
                    ESP_LOGI(TAG, "Weather State");
                    gpio_intr_enable(SET_PIN);

                    if(set_weather_mode == 0)
                    {
                        ESP_LOGI(TAG, "Weather State: Mode0");
                        lv_tabview_set_act(tv, 1, LV_ANIM_ON);
                        lv_display_weather_mode0_create(t2);
                    }
                    else
                    {
                        ESP_LOGI(TAG, "Weather State: Mode1");
                        lv_tabview_set_act(tv, 2, LV_ANIM_ON);
                        lv_display_weather_mode1_create(t3);
                    }
                    break;

                case ALARM_MODE:
                    gpio_intr_enable(RESET_PIN);
                    ESP_LOGI(TAG, "Alarm State");
                    lv_tabview_set_act(tv, 3, LV_ANIM_ON);
                    lv_timer_resume(alarm_timer);

                    break;

                case STOPWATCH_MODE:
                    gpio_intr_disable(SET_PIN);
                    gpio_intr_disable(RESET_PIN);
                    lv_timer_pause(alarm_timer);
                    ESP_LOGI(TAG, "StopWatch State");

                    break;
            }

            xSemaphoreGive(xGuiSemaphore);
        }

        // vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

