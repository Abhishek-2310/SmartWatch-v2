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
static void lv_display_alarm_create(lv_obj_t * parent);
static void lv_display_stopwatch_create(lv_timer_t * timer);
static void State_task(void * pvParameters);
static void Charge_icon_task(void * pvParameters);

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

extern uint16_t voltage;

// lv_obj_t * charge_img;

lv_obj_t* weather_img;

lv_timer_t * alarm_timer;
lv_timer_t * stopwatch_timer;
/**********************
 * EXTERNAL VARIABLES
 **********************/
LV_IMG_DECLARE(full_charge_icon_png);
LV_IMG_DECLARE(eighty_charge_icon_png);
LV_IMG_DECLARE(fifty_charge_icon_png);
LV_IMG_DECLARE(twenty_charge_icon_png);

LV_IMG_DECLARE(bulb_on_png);
LV_IMG_DECLARE(bulb_off_png);

LV_IMG_DECLARE(sunny_img_png);
LV_IMG_DECLARE(cloudy_img_png);
LV_IMG_DECLARE(snow_img_png);
LV_IMG_DECLARE(rain_img_png);
LV_IMG_DECLARE(thunderstorm_img_png);
LV_IMG_DECLARE(humidity_png);
LV_IMG_DECLARE(wind_speed_png);

LV_IMG_DECLARE(alarm_ON_orange_png);
LV_IMG_DECLARE(alarm_OFF_orange_png);
LV_IMG_DECLARE(alarm_orange_png);

LV_IMG_DECLARE(stopwatch_outline_png);

bool led1_state = false;
bool led2_state = false;

char * day_str;
extern char strftime_buf[64];

extern char weather_status[10];
extern double weather_temp;
extern double weather_speed;
extern int weather_pressure;
extern int weather_humidity;
extern char description_array[4][10];
char days_of_the_week[7][4] = {"MON\0","TUE\0","WED\0","THU\0","FRI\0","SAT\0","SUN\0"};
uint8_t weather_mode1_set_index = 0;
bool set_weather_mode = 0;

extern Mode_t Mode;
extern RTC_DATA_ATTR int bootcount;


extern Alarm_t alarm1;
extern StopWatch_t stopWatch1;
extern bool stopWatch_running;


TaskHandle_t StateTask_Handle;
TaskHandle_t ChargeIconTask_Handle;
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
    // alarm_timer = lv_timer_create(lv_display_alarm_create, 500, t4);
    // lv_timer_pause(alarm_timer);

    stopwatch_timer = lv_timer_create(lv_display_stopwatch_create, 300, t5);
    lv_timer_pause(stopwatch_timer);
    // lv_display_weather_mode0_create(t2);

    // lv_timer_create(State_task, 5000, NULL);
    xTaskCreatePinnedToCore(State_task, "State_task", 2048*2, NULL, 1, &StateTask_Handle, 1);
    xTaskCreatePinnedToCore(Charge_icon_task, "Charge_icon_task", 2048, NULL, 0, &ChargeIconTask_Handle, 1);

    if(bootcount > 1)
    {
        // Boot from deep sleep, not power on reset
        xTaskNotifyGive(StateTask_Handle);
    }
}

static void weather_mode1_set_days()
{
    for(uint8_t i = 0; i < 7; i++)
    {
        if(memcmp(day_str, days_of_the_week[i], 4) == 0)
        {
            weather_mode1_set_index = i;
            break;
        }
    }
    
}

static void lv_Print_Weather_Logo(char* weather_descp, lv_obj_t * screen, bool shrink_img)
{
    weather_img = lv_img_create(screen);

    if(memcmp(weather_descp, "Clouds", 7) == 0)
    {
        lv_img_set_src(weather_img, &cloudy_img_png);
        lv_obj_set_size(weather_img, 98, 67);

        if(shrink_img)
            lv_img_set_zoom(weather_img, 128);

    }
    else if (memcmp(weather_descp, "Rain", 4) == 0)
    {
        lv_img_set_src(weather_img, &rain_img_png);
        lv_obj_set_size(weather_img, 85, 74);

        if(shrink_img)
            lv_img_set_zoom(weather_img, 144);
    }
    else if (memcmp(weather_descp, "Clear", 5) == 0)
    {
        lv_img_set_src(weather_img, &sunny_img_png);
        lv_obj_set_size(weather_img, 86, 89);

        if(shrink_img)
            lv_img_set_zoom(weather_img, 128);
    }
    else if (memcmp(weather_descp, "Snow", 4) == 0)
    {
        lv_img_set_src(weather_img, &snow_img_png);
        lv_obj_set_size(weather_img, 81, 83);

        if(shrink_img)
            lv_img_set_zoom(weather_img, 128);
    }
    else if (memcmp(weather_descp, "Thunderstorm", 12) == 0)
    {
        lv_img_set_src(weather_img, &thunderstorm_img_png);
        lv_obj_set_size(weather_img, 90, 81);

        if(shrink_img)
            lv_img_set_zoom(weather_img, 128);
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
    lv_label_set_text(label_esp_comms_title, "ESP HOME");  // set text

    lv_obj_align(label_esp_comms_title, LV_ALIGN_TOP_MID, 0, 40);


	lv_obj_t * led1_label = lv_label_create(parent);
    lv_obj_add_style(led1_label, &style_esp_comms_title, 0);
    lv_label_set_text(led1_label, "LED1");  // set text
    lv_obj_align(led1_label, LV_ALIGN_LEFT_MID, 30, -40);


    lv_obj_t * led2_label = lv_label_create(parent);
    lv_obj_add_style(led2_label, &style_esp_comms_title, 0);
    lv_label_set_text(led2_label, "LED2");  // set text
    lv_obj_align(led2_label, LV_ALIGN_LEFT_MID, 30, 50);


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
    lv_obj_align(sw1, LV_ALIGN_LEFT_MID, 30, -10);

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
    lv_obj_align(sw2, LV_ALIGN_LEFT_MID, 30, 80);

}

// static void lv_display_charge_icon_create(lv_obj_t * parent)
// {
    
// }

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
    day_str = strftime_buf;
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

    // lv_display_charge_icon_create(parent);
}


static void lv_display_weather_mode0_create(lv_obj_t * parent)
{

    ESP_LOGI(TAG, "lv_display weather mode 0");

    lv_obj_clean(t3);   // Clean 5 day forecast tab to increase performance
    lv_obj_clean(parent);

    lv_Print_Weather_Logo(weather_status, parent, false);
    lv_img_set_zoom(weather_img, 320);
    lv_obj_align(weather_img, LV_ALIGN_TOP_RIGHT, -20, 60);

    char weather_temp_buffer[5];

    snprintf(weather_temp_buffer, 5, "%0.0f°", weather_temp);

    static lv_style_t style_weather_temp;
    lv_style_init(&style_weather_temp);
    lv_style_set_text_font(&style_weather_temp, &lv_font_montserrat_48); 
    lv_style_set_text_color(&style_weather_temp, lv_palette_main(LV_PALETTE_YELLOW));
    lv_style_set_text_letter_space(&style_weather_temp, 1);

	lv_obj_t * label_weather_temp = lv_label_create(parent);
    lv_obj_add_style(label_weather_temp, &style_weather_temp, 0);
    lv_label_set_text(label_weather_temp, weather_temp_buffer);  // set text
    lv_obj_align(label_weather_temp, LV_ALIGN_LEFT_MID, 10,-10);


    static lv_style_t style_weather_status;
    lv_style_init(&style_weather_status);
    lv_style_set_text_font(&style_weather_status, &lv_font_montserrat_24); 
    lv_style_set_text_color(&style_weather_status, lv_color_white());
    lv_style_set_text_letter_space(&style_weather_status, 1);

	lv_obj_t * label_weather_status = lv_label_create(parent);
    lv_obj_add_style(label_weather_status, &style_weather_status, 0);
    lv_label_set_text(label_weather_status, weather_status);  // set text
    lv_obj_align(label_weather_status, LV_ALIGN_LEFT_MID, 10, 30);

    lv_obj_t * humidity_img = lv_img_create(parent);

    lv_img_set_src(humidity_img, &humidity_png);
    lv_obj_set_size(humidity_img, 48, 48);
    lv_img_set_zoom(humidity_img, 156);
    lv_obj_align(humidity_img, LV_ALIGN_BOTTOM_RIGHT, -25, -40);

    char humidty_buffer[20];
    snprintf(humidty_buffer, 20, "Humidity: %d%%", weather_humidity);

    static lv_style_t style_humidity_wind;
    lv_style_init(&style_humidity_wind);
    lv_style_set_text_font(&style_humidity_wind, &lv_font_montserrat_12); 
    lv_style_set_text_color(&style_humidity_wind, lv_color_white());
    lv_style_set_text_letter_space(&style_humidity_wind, 1);

	lv_obj_t * label_weather_humidity = lv_label_create(parent);
    lv_obj_add_style(label_weather_humidity, &style_humidity_wind, 0);
    lv_label_set_text(label_weather_humidity, humidty_buffer);  // set text
    lv_obj_align(label_weather_humidity, LV_ALIGN_BOTTOM_RIGHT, 0, -30);

    lv_obj_t * wind_speed_img = lv_img_create(parent);

    lv_img_set_src(wind_speed_img, &wind_speed_png);
    lv_obj_set_size(wind_speed_img, 48, 48);
    lv_img_set_zoom(wind_speed_img, 156);
    lv_obj_align(wind_speed_img, LV_ALIGN_BOTTOM_LEFT, 30, -40);

    char wind_speed_buffer[20];
    snprintf(wind_speed_buffer, 20, "Wind: %0.0lf mph", weather_speed);

	lv_obj_t * label_weather_wind_speed = lv_label_create(parent);
    lv_obj_add_style(label_weather_wind_speed, &style_humidity_wind, 0);
    lv_label_set_text(label_weather_wind_speed, wind_speed_buffer);  // set text
    lv_obj_align(label_weather_wind_speed, LV_ALIGN_BOTTOM_LEFT, 10, -30);
}


static void lv_display_weather_mode1_create(lv_obj_t * parent)
{
    
    ESP_LOGI(TAG, "lv_display weather mode 1");

    lv_obj_clean(parent);

    static lv_style_t style_weather_status_mode1;
    lv_style_init(&style_weather_status_mode1);
    lv_style_set_text_font(&style_weather_status_mode1, &lv_font_montserrat_20); 
    lv_style_set_text_color(&style_weather_status_mode1, lv_palette_main(LV_PALETTE_YELLOW));
    lv_style_set_text_letter_space(&style_weather_status_mode1, 2);

    weather_mode1_set_days();

	lv_obj_t * label_weather_status_mode1_day0 = lv_label_create(parent);
    lv_obj_add_style(label_weather_status_mode1_day0, &style_weather_status_mode1, 0);
    lv_label_set_text(label_weather_status_mode1_day0, days_of_the_week[(weather_mode1_set_index % 7)]);  // set text
    lv_obj_align(label_weather_status_mode1_day0, LV_ALIGN_LEFT_MID, 10, -90);

    lv_Print_Weather_Logo(weather_status, parent, true);
    lv_obj_align(weather_img, LV_ALIGN_RIGHT_MID, 20, -90);


	lv_obj_t * label_weather_status_mode1_day1 = lv_label_create(parent);
    lv_obj_add_style(label_weather_status_mode1_day1, &style_weather_status_mode1, 0);
    lv_label_set_text(label_weather_status_mode1_day1, days_of_the_week[(weather_mode1_set_index + 1) % 7]);  // set text
    lv_obj_align(label_weather_status_mode1_day1, LV_ALIGN_LEFT_MID, 10, -40);

    lv_Print_Weather_Logo(description_array[0], parent, true);
    lv_obj_align(weather_img, LV_ALIGN_RIGHT_MID, 20, -40);

    // snprintf(weather_mode1_display_buffer[2], 30, "Day %d   %s\n", 2, description_array[1]);

	lv_obj_t * label_weather_status_mode1_day2 = lv_label_create(parent);
    lv_obj_add_style(label_weather_status_mode1_day2, &style_weather_status_mode1, 0);
    lv_label_set_text(label_weather_status_mode1_day2, days_of_the_week[(weather_mode1_set_index + 2) % 7]);  // set text
    lv_obj_align(label_weather_status_mode1_day2, LV_ALIGN_LEFT_MID, 10, 10);

    lv_Print_Weather_Logo(description_array[1], parent, true);
    lv_obj_align(weather_img, LV_ALIGN_RIGHT_MID, 20, 10);
    // snprintf(weather_mode1_display_buffer[3], 30, "Day %d   %s\n", 3, description_array[2]);

	lv_obj_t * label_weather_status_mode1_day3 = lv_label_create(parent);
    lv_obj_add_style(label_weather_status_mode1_day3, &style_weather_status_mode1, 0);
    lv_label_set_text(label_weather_status_mode1_day3, days_of_the_week[(weather_mode1_set_index + 3) % 7]);  // set text
    lv_obj_align(label_weather_status_mode1_day3, LV_ALIGN_LEFT_MID, 10, 60);

    lv_Print_Weather_Logo(description_array[2], parent, true);
    lv_obj_align(weather_img, LV_ALIGN_RIGHT_MID, 20, 60);
    // snprintf(weather_mode1_display_buffer[4], 30, "Day %d   %s", 4, description_array[3]);

	lv_obj_t * label_weather_status_mode1_day4 = lv_label_create(parent);
    lv_obj_add_style(label_weather_status_mode1_day4, &style_weather_status_mode1, 0);
    lv_label_set_text(label_weather_status_mode1_day4, days_of_the_week[(weather_mode1_set_index + 4) % 7]);  // set text
    lv_obj_align(label_weather_status_mode1_day4, LV_ALIGN_LEFT_MID, 10, 110);

    lv_Print_Weather_Logo(description_array[3], parent, true);
    lv_obj_align(weather_img, LV_ALIGN_RIGHT_MID, 20, 110);
}


static void lv_display_alarm_create(lv_obj_t * parent)
{
    ESP_LOGI(TAG, "lv_display alarm");

    lv_obj_clean(t3);   // Clean 5 day forecast tab to increase performance
    lv_obj_clean(parent);

    lv_obj_t * alarm_title_img = lv_img_create(parent);
    lv_img_set_src(alarm_title_img, &alarm_orange_png);
    lv_obj_set_size(alarm_title_img, 88, 87);
    lv_img_set_zoom(alarm_title_img, 128);
    lv_obj_align(alarm_title_img, LV_ALIGN_TOP_MID, 0, 30);

    char alarm_buffer[15];

    snprintf(alarm_buffer, 15, " %02d:%02d ", alarm1.hours, alarm1.minutes);

    static lv_style_t style_alarm;
    lv_style_init(&style_alarm);
	lv_obj_t * label_alarm = lv_label_create(parent);

    lv_style_set_radius(&style_alarm, 50);
    lv_style_set_bg_opa(&style_alarm, LV_OPA_10);
    // lv_style_set_border_width(&style_alarm, 10);
    lv_style_set_bg_color(&style_alarm, lv_palette_lighten(LV_PALETTE_ORANGE, 1));
    
    lv_style_set_text_font(&style_alarm, &lv_font_montserrat_48); 
    lv_style_set_text_color(&style_alarm, lv_palette_main(LV_PALETTE_ORANGE));
    lv_style_set_text_letter_space(&style_alarm, 2);

    lv_obj_add_style(label_alarm, &style_alarm, 0);
    lv_label_set_text(label_alarm, alarm_buffer);  // set text

    lv_obj_align(label_alarm, LV_ALIGN_CENTER, 0, 10);
    
    lv_obj_t * alarm_state_img = lv_img_create(parent);

    if(alarm1.enabled)
        lv_img_set_src(alarm_state_img, &alarm_ON_orange_png);
    else
        lv_img_set_src(alarm_state_img, &alarm_OFF_orange_png);

    lv_obj_set_size(alarm_state_img, 64, 64);
    lv_img_set_zoom(alarm_state_img, 192);
    lv_obj_align(alarm_state_img, LV_ALIGN_BOTTOM_MID, 0, -20);
}


static void lv_display_stopwatch_create(lv_timer_t * timer)
{
    ESP_LOGI(TAG, "lv_display stopwatch");

    lv_obj_clean(timer->user_data);

    lv_obj_t * stopwatch_title_img = lv_img_create(timer->user_data);
    lv_img_set_src(stopwatch_title_img, &stopwatch_outline_png);
    lv_obj_set_size(stopwatch_title_img, 128, 133);
    lv_img_set_zoom(stopwatch_title_img, 424);
    lv_obj_align(stopwatch_title_img, LV_ALIGN_CENTER, 0, 5);


    char stopwatch_buffer[11];

    snprintf(stopwatch_buffer, 11, " %02d:%02d ", stopWatch1.minutes, stopWatch1.seconds);

    static lv_style_t style_stopwatch;
    lv_style_init(&style_stopwatch);
	lv_obj_t * label_stopwatch = lv_label_create(timer->user_data);

    lv_style_set_radius(&style_stopwatch, 20);
    lv_style_set_bg_opa(&style_stopwatch, LV_OPA_10);
    lv_style_set_bg_color(&style_stopwatch, lv_palette_lighten(LV_PALETTE_LIGHT_GREEN, 1));
    
    lv_style_set_text_font(&style_stopwatch, &lv_font_montserrat_40); 
    lv_style_set_text_color(&style_stopwatch, lv_color_white());
    lv_style_set_text_letter_space(&style_stopwatch, 2);

    lv_obj_add_style(label_stopwatch, &style_stopwatch, 0);
    lv_label_set_text(label_stopwatch, stopwatch_buffer);  // set text

    lv_obj_align(label_stopwatch, LV_ALIGN_CENTER, 0, 0);


    char stopwatch_centi_buffer[6];
    snprintf(stopwatch_centi_buffer, 6, " %02d ", stopWatch1.centiSeconds);

    static lv_style_t style_stopwatch_centi;
    lv_style_init(&style_stopwatch_centi);
	lv_obj_t * label_stopwatch_centi = lv_label_create(timer->user_data);

    lv_style_set_radius(&style_stopwatch_centi, 20);
    lv_style_set_bg_opa(&style_stopwatch_centi, LV_OPA_10);
    lv_style_set_bg_color(&style_stopwatch_centi, lv_palette_lighten(LV_PALETTE_LIGHT_GREEN, 1));
    
    lv_style_set_text_font(&style_stopwatch_centi, &lv_font_montserrat_24); 
    lv_style_set_text_color(&style_stopwatch_centi, lv_color_white());
    lv_style_set_text_letter_space(&style_stopwatch_centi, 2);

    lv_obj_add_style(label_stopwatch_centi, &style_stopwatch_centi, 0);
    lv_label_set_text(label_stopwatch_centi, stopwatch_centi_buffer);  // set text

    lv_obj_align(label_stopwatch_centi, LV_ALIGN_CENTER, 30, 45);
    
}

static void Charge_icon_task(void * pvParameters)
{
    lv_obj_t * charge_img = lv_img_create(lv_scr_act());

    static lv_anim_t animation_template;
    static lv_style_t label_style;
    lv_obj_t * battery_low_label = NULL;

    lv_anim_init(&animation_template);
    lv_anim_set_delay(&animation_template, 1000);           /*Wait 1 second to start the first scroll*/
    lv_anim_set_repeat_delay(&animation_template, 3000);    /*Repeat the scroll 3 seconds after the label scrolls back to the initial position*/

    /*Initialize the label style with the animation template*/
    lv_style_init(&label_style);
    lv_style_set_anim(&label_style, &animation_template);
    lv_style_set_text_font(&label_style, &lv_font_montserrat_12); 
    lv_style_set_text_color(&label_style, lv_color_white());
    lv_style_set_text_letter_space(&label_style, 2);

    
    while(1)
    {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(60000));

        if(pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))
        {
            if(battery_low_label != NULL)
                lv_obj_del(battery_low_label);
            
            if(voltage >= 2640)                         // greater than 80 
                lv_img_set_src(charge_img, &full_charge_icon_png);
            else if(voltage >= 1650 && voltage < 2640)  // 50 - 80
                lv_img_set_src(charge_img, &eighty_charge_icon_png);
            else if(voltage >= 660 && voltage < 1650)  // 20 -50
                lv_img_set_src(charge_img, &fifty_charge_icon_png);
            else if(voltage > 0 && voltage < 660)     // 0 - 20
            {
                lv_img_set_src(charge_img, &twenty_charge_icon_png);

                battery_low_label = lv_label_create(lv_scr_act());
                lv_label_set_long_mode(battery_low_label, LV_LABEL_LONG_SCROLL_CIRCULAR);      /*Circular scroll*/
                lv_label_set_recolor(battery_low_label, true);                      /*Enable re-coloring by commands in the text*/
                lv_obj_set_width(battery_low_label, 150);
                lv_label_set_text(battery_low_label, "Hey Boss! Battery #ff0000 LOW     ");
                lv_obj_add_style(battery_low_label, &label_style, LV_STATE_DEFAULT);
                lv_obj_align(battery_low_label, LV_ALIGN_TOP_LEFT, 20, 35);
            }
                
            lv_obj_set_size(charge_img, 56, 32);
            lv_img_set_zoom(charge_img, 128);
            lv_obj_align(charge_img, LV_ALIGN_TOP_RIGHT, -10, 25);

            xSemaphoreGive(xGuiSemaphore);
        }
        ESP_LOGI("Charge_icon_task", "stack left: %d", uxTaskGetStackHighWaterMark(NULL));
    }

    lv_obj_del(charge_img);
    vTaskDelete(NULL);
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
                    // gpio_intr_enable(SET_PIN);
                    // gpio_intr_enable(RESET_PIN);
                    lv_tabview_set_act(tv, 0, LV_ANIM_ON);
                    lv_display_esp_comms_create(t0);

                    break;

                case TIME_MODE:
                    // lv_timer_pause(stopwatch_timer);
                    // gpio_intr_disable(SET_PIN);
                    // gpio_intr_disable(RESET_PIN);

                    ESP_LOGI(TAG, "Time State");
                    lv_tabview_set_act(tv, 1, LV_ANIM_ON);
                    lv_display_time_create(t1);

                    break;

                case WEATHER_MODE:
                    ESP_LOGI(TAG, "Weather State");
                    // gpio_intr_enable(SET_PIN);

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
                    // gpio_intr_enable(RESET_PIN);
                    ESP_LOGI(TAG, "Alarm State");
                    lv_tabview_set_act(tv, 4, LV_ANIM_ON);
                    // lv_timer_resume(alarm_timer);
                    lv_display_alarm_create(t4);

                    break;

                case STOPWATCH_MODE:
                    
                    // lv_timer_pause(alarm_timer);
                    ESP_LOGI(TAG, "StopWatch State");
                    lv_tabview_set_act(tv, 5, LV_ANIM_ON);
                    if(stopWatch_running)
                    {
                        lv_timer_resume(stopwatch_timer);
                    }
                    else
                    {
                        lv_timer_pause(stopwatch_timer);
                        lv_display_stopwatch_create(stopwatch_timer);
                    }

                    break;
            }
            // ESP_LOGI(TAG, "total min free memory: %ld", esp_get_minimum_free_heap_size());
            ESP_LOGI(TAG, "Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());

            xSemaphoreGive(xGuiSemaphore);
        }

    }
}

