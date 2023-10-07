/*********************
 *      INCLUDES
 *********************/
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "lvgl.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_display_time_create(lv_obj_t * parent);
static void lv_display_weather_create(lv_obj_t * parent);

static void tab_changer_task_cb(lv_timer_t * task);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_obj_t * tv;
static lv_obj_t * t1;
static lv_obj_t * t2;
static lv_obj_t * t3;
static lv_obj_t * kb;

static lv_style_t style_box;

/**********************
 * EXTERNAL VARIABLES
 **********************/
// extern char strftime_buf[64];
// extern char weather_status[20];
// extern double weather_temp;
// extern int weather_pressure;
// extern int weather_humidity;

/**********************
 *        TAGS
 **********************/
static const char *TAG = "task_changer";

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_task_modes(void)
{
    /*Create a Tab view object*/

    tv = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 50);
    lv_btnmatrix_t * tab_buttons = lv_tabview_get_tab_btns(tv);
    lv_btnmatrix_set_btn_ctrl(tab_buttons, 0, LV_BTNMATRIX_CTRL_HIDDEN);
    lv_btnmatrix_set_btn_ctrl(tab_buttons, 1, LV_BTNMATRIX_CTRL_HIDDEN);

    t1 = lv_tabview_add_tab(tv, "Time");
    t2 = lv_tabview_add_tab(tv, "Weather");

    // Background Style to turn it black
    static lv_style_t style_screen;
    lv_style_init(&style_screen);
    lv_style_set_bg_color(&style_screen, lv_color_black());
    lv_obj_add_style(tv, &style_screen, 0); 

    // lv_style_init(&style_box);
    // lv_style_set_value_align(&style_box, LV_STATE_DEFAULT, LV_ALIGN_OUT_TOP_LEFT);
    // lv_style_set_value_ofs_y(&style_box, LV_STATE_DEFAULT, - LV_DPX(10));
    // lv_style_set_margin_top(&style_box, LV_STATE_DEFAULT, LV_DPX(30));

    lv_display_time_create(t1);
    lv_display_weather_create(t2);

    lv_timer_create(tab_changer_task_cb, 5000, NULL);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/


static void lv_display_time_create(lv_obj_t * parent)
{
    // lv_page_set_scrl_layout(parent, LV_LAYOUT_PRETTY_TOP);
    ESP_LOGI(TAG, "lv_display time");

    static lv_style_t style_time;
    lv_style_init(&style_time);
	lv_obj_t *label_time = lv_label_create(parent);
    
    lv_style_set_text_font(&style_time, &lv_font_montserrat_16); 
    lv_style_set_text_color(&style_time, lv_palette_main(LV_PALETTE_YELLOW));
    lv_style_set_text_letter_space(&style_time, 5);

    lv_obj_add_style(label_time, &style_time, 0);
    lv_label_set_text(label_time, "time tab");  // set text

    lv_obj_align(label_time, LV_ALIGN_TOP_MID, 0, 50);
    // time_t now;
    // struct tm timeinfo;
    // time(&now);

    // localtime_r(&now, &timeinfo);
    // strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    // ESP_LOGI(TAG, "The current date/time in Canada is: %s", strftime_buf);

    // // String slicing to get day, date and time
    // char * day_str = strftime_buf;
    // *(day_str + 3) = '\0';

    // int i = 0;

    // while (*(day_str + i))
    // {
    //     *(day_str + i) = toupper(*(day_str + i));
    //     i++;
    // }
    // ESP_LOGI(TAG, "The current day is: %s", day_str);

    // char * date_str = &strftime_buf[8];
    // *(date_str + 2) = '\0';
    // ESP_LOGI(TAG, "The current date is: %s", date_str);

    // char * tim_str = &strftime_buf[11];
    // *(tim_str + 5) = '\0';
    // ESP_LOGI(TAG, "The current time is: %s", tim_str);

    // // Clean the view
    // lv_obj_clean(parent);

    // lv_obj_t *tim_label = lv_label_create(parent, NULL);
    // lv_label_set_text(tim_label, tim_str);  // set text

    // lv_obj_set_style_local_bg_opa(tim_label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    // lv_obj_set_style_local_text_color(tim_label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_YELLOW);           // fg color
    // lv_obj_set_style_local_text_font(tim_label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_40);  // font size(template：lv_font_montserrat_xx)

	// // lv_obj_set_pos(hour_label, 60,50);
    // lv_obj_align(tim_label, NULL, LV_ALIGN_CENTER, 0, -60);

    // lv_obj_t *day_label = lv_label_create(parent, NULL);
    // lv_label_set_text(day_label, day_str);  // set text

    // lv_obj_set_style_local_bg_opa(day_label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    // lv_obj_set_style_local_text_color(day_label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);           // fg color
    // lv_obj_set_style_local_text_font(day_label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_24);  // font size(template：lv_font_montserrat_xx)

	// // lv_obj_set_pos(hour_label, 60,50);
    // lv_obj_align(day_label, NULL, LV_ALIGN_CENTER, 0, 30);


    // lv_obj_t *date_label = lv_label_create(parent, NULL);
    // lv_label_set_text(date_label, date_str);  // set text

    // lv_obj_set_style_local_bg_opa(date_label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    // lv_obj_set_style_local_text_color(date_label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_YELLOW);           // fg color
    // lv_obj_set_style_local_text_font(date_label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_40);  // font size(template：lv_font_montserrat_xx)

	// // lv_obj_set_pos(hour_label, 60,50);
    // lv_obj_align(date_label, NULL, LV_ALIGN_CENTER, 0, 80);
// #if LV_DEMO_WIDGETS_SLIDESHOW
//     tab_content_anim_create(parent);
// #endif
}

static void lv_display_weather_create(lv_obj_t * parent)
{

    ESP_LOGI(TAG, "lv_display weather");

    lv_obj_clean(parent);

    char weather_temp_buffer[20];
    char weather_pressure_buffer[20];
    char weather_humidity_buffer[20];

    static lv_style_t style_time;
    lv_style_init(&style_time);
	lv_obj_t *label_time = lv_label_create(parent);
    
    lv_style_set_text_font(&style_time, &lv_font_montserrat_16); 
    lv_style_set_text_color(&style_time, lv_palette_main(LV_PALETTE_YELLOW));
    lv_style_set_text_letter_space(&style_time, 5);

    lv_obj_add_style(label_time, &style_time, 0);
    lv_label_set_text(label_time, "display tab");  // set text

    lv_obj_align(label_time, LV_ALIGN_TOP_MID, 0, 50);

    // lv_obj_t * weather_status_label = lv_label_create(parent, NULL);
    // lv_label_set_text(weather_status_label, weather_status);  // set text

    // lv_obj_set_style_local_bg_opa(weather_status_label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    // lv_obj_set_style_local_text_color(weather_status_label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_YELLOW);           // fg color
    // lv_obj_set_style_local_text_font(weather_status_label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_24);  // font size(template：lv_font_montserrat_xx)

	// // lv_obj_set_pos(hour_label, 60,50);
    // lv_obj_align(weather_status_label, NULL, LV_ALIGN_CENTER, 0, -60);


    // snprintf(weather_temp_buffer, 20, "Temp: %0.00f°C", weather_temp);

    // lv_obj_t * weather_temp_label = lv_label_create(parent, NULL);
    // lv_label_set_text(weather_temp_label, weather_temp_buffer);  // set text

    // lv_obj_set_style_local_bg_opa(weather_temp_label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    // lv_obj_set_style_local_text_color(weather_temp_label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);           // fg color
    // lv_obj_set_style_local_text_font(weather_temp_label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);  // font size(template：lv_font_montserrat_xx)

	// // lv_obj_set_pos(hour_label, 60,50);
    // lv_obj_align(weather_temp_label, NULL, LV_ALIGN_CENTER, 0, -20);


    // snprintf(weather_pressure_buffer, 20, "Press: %d mBar", weather_pressure);

    // lv_obj_t * weather_pressure_label = lv_label_create(parent, NULL);
    // lv_label_set_text(weather_pressure_label, weather_pressure_buffer);  // set text

    // lv_obj_set_style_local_bg_opa(weather_pressure_label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    // lv_obj_set_style_local_text_color(weather_pressure_label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);           // fg color
    // lv_obj_set_style_local_text_font(weather_pressure_label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);  // font size(template：lv_font_montserrat_xx)

	// // lv_obj_set_pos(hour_label, 60,50);
    // lv_obj_align(weather_pressure_label, NULL, LV_ALIGN_CENTER, 0, 20);


    // snprintf(weather_humidity_buffer, 20, "Humidity: %d%%", weather_humidity);

    // lv_obj_t * weather_humidity_label = lv_label_create(parent, NULL);
    // lv_label_set_text(weather_humidity_label, weather_humidity_buffer);  // set text

    // lv_obj_set_style_local_bg_opa(weather_humidity_label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    // lv_obj_set_style_local_text_color(weather_humidity_label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);           // fg color
    // lv_obj_set_style_local_text_font(weather_humidity_label, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_16);  // font size(template：lv_font_montserrat_xx)

	// // lv_obj_set_pos(hour_label, 60,50);
    // lv_obj_align(weather_humidity_label, NULL, LV_ALIGN_CENTER, 0, 60);
    
}


static void tab_changer_task_cb(lv_timer_t * task)
{
    ESP_LOGI(TAG, "Switching tasks");

    uint16_t tab_id = lv_tabview_get_tab_act(tv);
    tab_id++;
    if(tab_id >= 2) tab_id = 0;

    lv_tabview_set_act(tv, tab_id, LV_ANIM_ON);

    switch(tab_id) {
    case 0:
        lv_display_time_create(t1);
        // tab_content_anim_create(t1);
        break;
    case 1:
        lv_display_weather_create(t2);
        // tab_content_anim_create(t2);
        break;
    }
}