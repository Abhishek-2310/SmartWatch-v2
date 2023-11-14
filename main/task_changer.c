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
static void lv_display_stopwatch_create(lv_timer_t * timer);
static void State_task(void * pvParameters);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_obj_t * tv;
static lv_obj_t * t0;   // comms display
static lv_obj_t * t1;
static lv_obj_t * t2;
static lv_obj_t * t3;
static lv_obj_t * t4;
static lv_obj_t * t5;

lv_obj_t* led1_img;
lv_obj_t* led2_img;
lv_obj_t* sw1;
lv_obj_t* sw2;
bool secondRun = false;


lv_obj_t* today_weather_img;

lv_timer_t * alarm_timer;
lv_timer_t * stopwatch_timer;
/**********************
 * EXTERNAL VARIABLES
 **********************/
LV_IMG_DECLARE(bulb_on_png);
LV_IMG_DECLARE(bulb_off_png);

LV_IMG_DECLARE(sunny_img_png);
LV_IMG_DECLARE(cloudy_img_png);
LV_IMG_DECLARE(snow_img_png);
LV_IMG_DECLARE(rain_img_png);

bool led1_state = false;
bool led2_state = false;

extern char strftime_buf[64];

extern char weather_status[10];
extern double weather_temp;
extern double weather_speed;
extern int weather_pressure;
extern int weather_humidity;
extern char description_array[4][10];
bool set_weather_mode = 0;

extern RTC_DATA_ATTR Mode_t Mode;
extern RTC_DATA_ATTR int bootcount;


extern Alarm_t alarm1;
extern StopWatch_t stopWatch1;

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

    t0 = lv_tabview_add_tab(tv, "ESP_Comms");
    t1 = lv_tabview_add_tab(tv, "Time");
    t2 = lv_tabview_add_tab(tv, "Weather_mode0");
    t3 = lv_tabview_add_tab(tv, "Weather_mode1");
    t4 = lv_tabview_add_tab(tv, "Alarm");
    t5 = lv_tabview_add_tab(tv, "StopWatch");

    // Background Style to turn it black
    static lv_style_t style_screen;
    lv_style_init(&style_screen);
    lv_style_set_bg_color(&style_screen, lv_color_black());
    lv_obj_add_style(tv, &style_screen, 0); 

    
    // lv_display_time_create(t1);
    alarm_timer = lv_timer_create(lv_display_alarm_create, 500, t4);
    lv_timer_pause(alarm_timer);

    stopwatch_timer = lv_timer_create(lv_display_stopwatch_create, 100, t5);
    lv_timer_pause(stopwatch_timer);
    // lv_display_weather_mode0_create(t2);

    // lv_timer_create(State_task, 5000, NULL);
    xTaskCreatePinnedToCore(State_task, "State", 2048*2, NULL, 0, &StateTask_Handle, 1);

    if(bootcount > 1)
    {
        // Boot from deep sleep, not power on reset
        xTaskNotifyGive(StateTask_Handle);
    }
}


static void lv_Print_Weather_Logo(char* weather_descp, lv_obj_t * screen, bool shrink_img)
{
    today_weather_img = lv_img_create(screen);

    if(memcmp(weather_descp, "Clouds", 7) == 0)
    {
        lv_img_set_src(today_weather_img, &cloudy_img_png);
        lv_obj_set_size(today_weather_img, 98, 67);

        if(shrink_img)
            lv_img_set_zoom(today_weather_img, 128);

    }
    else if (memcmp(weather_descp, "Rain", 4) == 0)
    {
        lv_img_set_src(today_weather_img, &rain_img_png);
        lv_obj_set_size(today_weather_img, 85, 74);

        if(shrink_img)
            lv_img_set_zoom(today_weather_img, 144);
    }
    else if (memcmp(weather_descp, "Clear", 5) == 0)
    {
        lv_img_set_src(today_weather_img, &sunny_img_png);
        lv_obj_set_size(today_weather_img, 86, 89);

        if(shrink_img)
            lv_img_set_zoom(today_weather_img, 128);
    }
    else if (memcmp(weather_descp, "Snow", 4) == 0)
    {
        lv_img_set_src(today_weather_img, &snow_img_png);
        lv_obj_set_size(today_weather_img, 81, 83);

        if(shrink_img)
            lv_img_set_zoom(today_weather_img, 128);
    }
   
}
/**********************
 STATIC DISPLAY FUNCTIONS
 **********************/
static void lv_display_esp_comms_create(lv_obj_t * parent)
{
    ESP_LOGI(TAG, "lv_display esp_comms tab");
    
    lv_obj_clean(parent);

    static lv_style_t style_esp_comms_title;
    lv_style_init(&style_esp_comms_title);
	lv_obj_t * label_esp_comms_title = lv_label_create(parent);
    
    lv_style_set_text_font(&style_esp_comms_title, &lv_font_montserrat_20); 
    lv_style_set_text_color(&style_esp_comms_title, lv_color_white());
    lv_style_set_text_letter_space(&style_esp_comms_title, 2);

    lv_obj_add_style(label_esp_comms_title, &style_esp_comms_title, 0);
    lv_label_set_text(label_esp_comms_title, "ESP COMMS");  // set text

    lv_obj_align(label_esp_comms_title, LV_ALIGN_TOP_MID, 0, 40);


	lv_obj_t * led1_label = lv_label_create(parent);
    lv_obj_add_style(led1_label, &style_esp_comms_title, 0);
    lv_label_set_text(led1_label, "LED1");  // set text
    lv_obj_align(led1_label, LV_ALIGN_LEFT_MID, 20, -40);


    lv_obj_t * led2_label = lv_label_create(parent);
    lv_obj_add_style(led2_label, &style_esp_comms_title, 0);
    lv_label_set_text(led2_label, "LED2");  // set text
    lv_obj_align(led2_label, LV_ALIGN_LEFT_MID, 20, 50);


    led1_img = lv_img_create(parent);
    led2_img = lv_img_create(parent);
    sw1 = lv_switch_create(parent);
    sw2 = lv_switch_create(parent);

    if(led1_state)
    {
        lv_img_set_src(led1_img, &bulb_on_png);
        lv_obj_add_state(sw1, LV_STATE_CHECKED);
    }
    else
    {
        lv_img_set_src(led1_img, &bulb_off_png);
        lv_obj_add_state(sw1, !LV_STATE_CHECKED);
    }
    lv_obj_set_size(led1_img, 32, 32);
    lv_obj_align(led1_img, LV_ALIGN_RIGHT_MID, -20, -10);
    lv_obj_align(sw1, LV_ALIGN_LEFT_MID, 20, -10);

    if(led2_state)
    {
        lv_img_set_src(led2_img, &bulb_on_png);
        lv_obj_add_state(sw2, LV_STATE_CHECKED);
    }
    else
    {
        lv_img_set_src(led2_img, &bulb_off_png);
        lv_obj_add_state(sw1, !LV_STATE_CHECKED);
    }
    lv_obj_set_size(led2_img, 32, 32);
    lv_obj_align(led2_img, LV_ALIGN_RIGHT_MID, -20, 80);
    lv_obj_align(sw2, LV_ALIGN_LEFT_MID, 20, 80);

}


static void lv_display_time_create(lv_obj_t * parent)
{
    // lv_page_set_scrl_layout(parent, LV_LAYOUT_PRETTY_TOP);
    ESP_LOGI(TAG, "lv_display time tab");

    time_t now;
    struct tm timeinfo;
    time(&now);

    // Set timezone to Eastern Standard Time and print local time
    setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
    tzset();
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
}


static void lv_display_weather_mode0_create(lv_obj_t * parent)
{

    ESP_LOGI(TAG, "lv_display weather mode 0");

    lv_obj_clean(parent);

    lv_Print_Weather_Logo(weather_status, parent, false);

    lv_obj_align(today_weather_img, LV_ALIGN_TOP_MID, 0, 40);

    char weather_display_buffer[80];

    snprintf(weather_display_buffer, 80, "Temp      : %0.00f°C\nPressure : %d hPa\n"
    "Humidity: %d%%\nSpeed     : %0.0f mph", weather_temp, weather_pressure, weather_humidity, weather_speed);

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

    lv_obj_clean(t3);   // Clean 5 day forecast tab to increase performance
    lv_obj_clean(parent);

    static lv_style_t style_weather_status_mode1;
    lv_style_init(&style_weather_status_mode1);
    lv_style_set_text_font(&style_weather_status_mode1, &lv_font_montserrat_20); 
    lv_style_set_text_color(&style_weather_status_mode1, lv_palette_main(LV_PALETTE_YELLOW));
    lv_style_set_text_letter_space(&style_weather_status_mode1, 2);


	lv_obj_t * label_weather_status_mode1_day0 = lv_label_create(parent);
    lv_obj_add_style(label_weather_status_mode1_day0, &style_weather_status_mode1, 0);
    lv_label_set_text(label_weather_status_mode1_day0, "Today");  // set text
    lv_obj_align(label_weather_status_mode1_day0, LV_ALIGN_LEFT_MID, 10, -100);

    lv_Print_Weather_Logo(weather_status, parent, true);
    lv_obj_align(today_weather_img, LV_ALIGN_RIGHT_MID, 20, -100);


	lv_obj_t * label_weather_status_mode1_day1 = lv_label_create(parent);
    lv_obj_add_style(label_weather_status_mode1_day1, &style_weather_status_mode1, 0);
    lv_label_set_text(label_weather_status_mode1_day1, "Tomorrow");  // set text
    lv_obj_align(label_weather_status_mode1_day1, LV_ALIGN_LEFT_MID, 10, -50);

    lv_Print_Weather_Logo(description_array[0], parent, true);
    lv_obj_align(today_weather_img, LV_ALIGN_RIGHT_MID, 20, -50);

    // snprintf(weather_mode1_display_buffer[2], 30, "Day %d   %s\n", 2, description_array[1]);

	lv_obj_t * label_weather_status_mode1_day2 = lv_label_create(parent);
    lv_obj_add_style(label_weather_status_mode1_day2, &style_weather_status_mode1, 0);
    lv_label_set_text(label_weather_status_mode1_day2, "Day 3");  // set text
    lv_obj_align(label_weather_status_mode1_day2, LV_ALIGN_LEFT_MID, 10, 0);

    lv_Print_Weather_Logo(description_array[1], parent, true);
    lv_obj_align(today_weather_img, LV_ALIGN_RIGHT_MID, 20, 0);
    // snprintf(weather_mode1_display_buffer[3], 30, "Day %d   %s\n", 3, description_array[2]);

	lv_obj_t * label_weather_status_mode1_day3 = lv_label_create(parent);
    lv_obj_add_style(label_weather_status_mode1_day3, &style_weather_status_mode1, 0);
    lv_label_set_text(label_weather_status_mode1_day3, "Day 4");  // set text
    lv_obj_align(label_weather_status_mode1_day3, LV_ALIGN_LEFT_MID, 10, 50);

    lv_Print_Weather_Logo(description_array[2], parent, true);
    lv_obj_align(today_weather_img, LV_ALIGN_RIGHT_MID, 20, 50);
    // snprintf(weather_mode1_display_buffer[4], 30, "Day %d   %s", 4, description_array[3]);

	lv_obj_t * label_weather_status_mode1_day4 = lv_label_create(parent);
    lv_obj_add_style(label_weather_status_mode1_day4, &style_weather_status_mode1, 0);
    lv_label_set_text(label_weather_status_mode1_day4, "Day 5");  // set text
    lv_obj_align(label_weather_status_mode1_day4, LV_ALIGN_LEFT_MID, 10, 100);

    lv_Print_Weather_Logo(description_array[3], parent, true);
    lv_obj_align(today_weather_img, LV_ALIGN_RIGHT_MID, 20, 100);
}


static void lv_display_alarm_create(lv_timer_t * timer)
{
    ESP_LOGI(TAG, "lv_display alarm");

    lv_obj_clean(t3);   // Clean 5 day forecast tab to increase performance
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


static void lv_display_stopwatch_create(lv_timer_t * timer)
{
    ESP_LOGI(TAG, "lv_display stopwatch");

    lv_obj_clean(timer->user_data);

    static lv_style_t style_alarm_title;
    lv_style_init(&style_alarm_title);
	lv_obj_t * label_alarm_title = lv_label_create(timer->user_data);
    
    lv_style_set_text_font(&style_alarm_title, &lv_font_montserrat_24); 
    lv_style_set_text_color(&style_alarm_title, lv_color_white());
    lv_style_set_text_letter_space(&style_alarm_title, 2);

    lv_obj_add_style(label_alarm_title, &style_alarm_title, 0);
    lv_label_set_text(label_alarm_title, "Stopwatch");  // set text

    lv_obj_align(label_alarm_title, LV_ALIGN_TOP_MID, 0, 60);

    char alarm_buffer[15];

    snprintf(alarm_buffer, 15, "%02d:%02d:%02d", stopWatch1.minutes, stopWatch1.seconds, stopWatch1.centiSeconds);

    static lv_style_t style_alarm;
    lv_style_init(&style_alarm);
	lv_obj_t * label_alarm = lv_label_create(timer->user_data);
    
    lv_style_set_text_font(&style_alarm, &lv_font_montserrat_36); 
    lv_style_set_text_color(&style_alarm, lv_palette_main(LV_PALETTE_YELLOW));
    lv_style_set_text_letter_space(&style_alarm, 2);

    lv_obj_add_style(label_alarm, &style_alarm, 0);
    lv_label_set_text(label_alarm, alarm_buffer);  // set text

    lv_obj_align(label_alarm, LV_ALIGN_CENTER, 0, 0);
    
}

/* STATE CHANGING TASK */
static void State_task(void * pvParameters)
{

    while(1)
    {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10000));

        if(pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))
        {
            switch(Mode)
            {
                case ESP_COMMS_MODE:
                    gpio_intr_enable(SET_PIN);
                    gpio_intr_enable(RESET_PIN);
                    lv_tabview_set_act(tv, 0, LV_ANIM_ON);
                    lv_display_esp_comms_create(t0);

                    break;

                case TIME_MODE:
                    lv_timer_pause(stopwatch_timer);
                    gpio_intr_disable(SET_PIN);
                    gpio_intr_disable(RESET_PIN);

                    ESP_LOGI(TAG, "Time State");
                    lv_tabview_set_act(tv, 1, LV_ANIM_ON);
                    lv_display_time_create(t1);

                    break;

                case WEATHER_MODE:
                    ESP_LOGI(TAG, "Weather State");
                    gpio_intr_enable(SET_PIN);

                    if(set_weather_mode == 0)
                    {
                        ESP_LOGI(TAG, "Weather State: Mode0");
                        lv_tabview_set_act(tv, 2, LV_ANIM_ON);
                        lv_display_weather_mode0_create(t2);
                    }
                    else
                    {
                        ESP_LOGI(TAG, "Weather State: Mode1");
                        lv_tabview_set_act(tv, 3, LV_ANIM_ON);
                        lv_display_weather_mode1_create(t3);
                    }

                    break;

                case ALARM_MODE:
                    gpio_intr_enable(RESET_PIN);
                    ESP_LOGI(TAG, "Alarm State");
                    lv_tabview_set_act(tv, 4, LV_ANIM_ON);
                    lv_timer_resume(alarm_timer);

                    break;

                case STOPWATCH_MODE:
                    
                    lv_timer_pause(alarm_timer);
                    ESP_LOGI(TAG, "StopWatch State");
                    lv_tabview_set_act(tv, 5, LV_ANIM_ON);
                    lv_timer_resume(stopwatch_timer);

                    break;
            }

            xSemaphoreGive(xGuiSemaphore);
        }

    }
}

