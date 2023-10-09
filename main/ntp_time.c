#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>

#include "esp_system.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "esp_netif_sntp.h"
#include "lwip/ip_addr.h"
#include "esp_sntp.h"

#include "lvgl.h"

static const char *TAG = "display_time";

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 48
#endif

/* Variable holding number of times ESP32 restarted since first boot.
 * It is placed into RTC memory using RTC_DATA_ATTR and
 * maintains its value when ESP32 wakes from deep sleep.
 */

char strftime_buf[64];

/* Prototypes */
static void obtain_time(void);
void lv_display_time(lv_timer_t* task);

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}


void get_ntp_time(void)
{
    time_t now;
    struct tm timeinfo;
    
    time(&now);

    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in Canada is: %s", strftime_buf);

    ESP_LOGI(TAG, "Time is not set yet");
    obtain_time();
    time(&now);

    // Set timezone to Eastern Standard Time and print local time
    setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in Canada is: %s", strftime_buf);

}


static void print_servers(void)
{
    ESP_LOGI(TAG, "List of configured NTP servers:");

    for (uint8_t i = 0; i < SNTP_MAX_SERVERS; ++i){
        if (esp_sntp_getservername(i)){
            ESP_LOGI(TAG, "server %d: %s", i, esp_sntp_getservername(i));
        } else {
            // we have either IPv4 or IPv6 address, let's print it
            char buff[INET6_ADDRSTRLEN];
            ip_addr_t const *ip = esp_sntp_getserver(i);
            if (ipaddr_ntoa_r(ip, buff, INET6_ADDRSTRLEN) != NULL)
                ESP_LOGI(TAG, "server %d: %s", i, buff);
        }
    }
}


static void obtain_time(void)
{
    ESP_LOGI(TAG, "Initializing and starting SNTP");

    /*
     * This is the basic default config with one server and starting the service
     */
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(CONFIG_SNTP_TIME_SERVER);

    config.sync_cb = time_sync_notification_cb;     // Note: This is only needed if we want

    esp_netif_sntp_init(&config);

    print_servers();

    // wait for time to be set
    int retry = 0;
    const int retry_count = 15;
    while (esp_netif_sntp_sync_wait(2000 / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    }

    esp_netif_sntp_deinit();
}


// void display_time()
// {
//     get_ntp_time();

//     lv_timer_create(lv_display_time, 1000, NULL);
//     //Deep Sleep
//     // const int deep_sleep_sec = 10;
//     // ESP_LOGI(TAG, "Entering deep sleep for %d seconds", deep_sleep_sec);
//     // esp_deep_sleep(1000000LL * deep_sleep_sec);
// }


// void lv_display_time(lv_timer_t* task)
// {
//     time_t now;
//     struct tm timeinfo;
//     time(&now);

//     localtime_r(&now, &timeinfo);
//     strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
//     ESP_LOGI(TAG, "The current date/time in Canada is: %s", strftime_buf);

//     // String slicing to get day, date and time
//     char * day_str = strftime_buf;
//     *(day_str + 3) = '\0';

//     int i = 0;

//     while (*(day_str + i))
//     {
//         *(day_str + i) = toupper(*(day_str + i));
//         i++;
//     }
//     ESP_LOGI(TAG, "The current day is: %s", day_str);

//     char * date_str = &strftime_buf[8];
//     *(date_str + 2) = '\0';
//     ESP_LOGI(TAG, "The current date is: %s", date_str);

//     char * tim_str = &strftime_buf[11];
//     *(tim_str + 5) = '\0';
//     ESP_LOGI(TAG, "The current time is: %s", tim_str);

//     lv_obj_clean(lv_scr_act());

//     // Background Style to turn it black
//     static lv_style_t style_screen;
//     lv_style_init(&style_screen);
//     lv_style_set_bg_color(&style_screen, lv_color_black());
//     lv_obj_add_style(lv_scr_act(), &style_screen, 0); 

//     // time label
//     static lv_style_t style_time;
//     lv_style_init(&style_time);
// 	lv_obj_t *label_time = lv_label_create(lv_scr_act());
    
//     lv_style_set_text_font(&style_time, &lv_font_montserrat_48); 
//     lv_style_set_text_color(&style_time, lv_palette_main(LV_PALETTE_YELLOW));
//     lv_style_set_text_letter_space(&style_time, 5);

//     lv_obj_add_style(label_time, &style_time, 0);
//     lv_label_set_text(label_time, tim_str);  // set text

//     lv_obj_align(label_time, LV_ALIGN_TOP_MID, 0, 50);

//     // day label
//     static lv_style_t style_day;
//     lv_style_init(&style_day);
//     lv_obj_t *label_day = lv_label_create(lv_scr_act());

//     lv_style_set_text_font(&style_day, &lv_font_montserrat_36); 
//     lv_style_set_text_color(&style_day, lv_color_white());
//     lv_style_set_text_letter_space(&style_day, 2);

//     lv_obj_add_style(label_day, &style_day, 0);
//     lv_label_set_text(label_day, day_str);  // set text

//     lv_obj_align(label_day, LV_ALIGN_CENTER, 0, 30);

//     // date label
//     static lv_style_t style_date;
//     lv_style_init(&style_date);
//     lv_obj_t *label_date = lv_label_create(lv_scr_act());

//     lv_style_set_text_font(&style_date, &lv_font_montserrat_48); 
//     lv_style_set_text_color(&style_date, lv_palette_main(LV_PALETTE_YELLOW));
//     lv_style_set_text_letter_space(&style_date, 2);

//     lv_obj_add_style(label_date, &style_date, 0);
//     lv_label_set_text(label_date, date_str);  // set text

//     lv_obj_align(label_date, LV_ALIGN_CENTER, -10, 80);
// }