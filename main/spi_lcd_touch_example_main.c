/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"

#include "protocol_examples_common.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "common.h"

/**********************
 *      MACROS
 **********************/
// Using SPI2 in the example
#define LCD_HOST  SPI2_HOST

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ     (80 * 1000 * 1000)
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_SCLK           14
#define EXAMPLE_PIN_NUM_MOSI           13
#define EXAMPLE_PIN_NUM_MISO           12
#define EXAMPLE_PIN_NUM_LCD_DC         2
#define EXAMPLE_PIN_NUM_LCD_RST        4
#define EXAMPLE_PIN_NUM_LCD_CS         15
#define EXAMPLE_PIN_NUM_BK_LIGHT       21
#define EXAMPLE_PIN_NUM_TOUCH_CS       18

// The pixel number in horizontal and vertical
#define EXAMPLE_LCD_H_RES              240
#define EXAMPLE_LCD_V_RES              320
// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8

#define EXAMPLE_LVGL_TICK_PERIOD_MS    1

// Pushbuttons
#define SET_PIN 19
#define MODE_PIN  26
#define RESET_PIN 18
#define DEBOUNCE_DELAY 30   // in ms

/**********************
 *   ESP LOG TAGS
 **********************/
static const char * guiTag = "guiTask";
static const char * mainTag = "app_main";
static const char * TAG = "Button";



/**********************
 *  GLOBAL VARIABLES
 **********************/
Alarm_t alarm1;
uint8_t count = 0;
// uint8_t mode_count = 0;
Mode_t Mode = TIME_MODE;
const Mode_t Mode_Table[4] = {TIME_MODE,
                            WEATHER_MODE,
                            ALARM_MODE,
                            STOPWATCH_MODE};
uint8_t mode_index = 0;

BaseType_t set_hour = pdTRUE;

/**********************
 *      HANDLES
 **********************/
TaskHandle_t AlarmTask_Handle;
TaskHandle_t ModeTask_Handle;
TaskHandle_t Set_Rst_task_handle;
extern TaskHandle_t StateTask_Handle;
/**********************
 *  STATIC PROTOTYPES
 **********************/
static void guiTask(void *pvParameter);
// extern void example_lvgl_demo_ui(lv_disp_t *disp);
extern void get_ntp_time(void);
extern void get_weather_update(void);
extern void lv_task_modes(void);

/**********************
 * INTERRUPT CALLBACKS
 **********************/
static void IRAM_ATTR set_rst_interrupt_handler(void *args)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Notify the Set_Rst_task that the button was pressed
    vTaskNotifyGiveFromISR(Set_Rst_task_handle, &xHigherPriorityTaskWoken);

    // Clear the interrupt flag and exit
    gpio_intr_disable(SET_PIN);
    gpio_intr_enable(SET_PIN);
    gpio_intr_disable(RESET_PIN);
    gpio_intr_enable(RESET_PIN);
}


static void IRAM_ATTR mode_interrupt_handler(void *args)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Notify the Set_Rst_task that the button was pressed
    vTaskNotifyGiveFromISR(ModeTask_Handle, &xHigherPriorityTaskWoken);

    // Clear the interrupt flag and exit
    gpio_intr_disable(MODE_PIN);
    gpio_intr_enable(MODE_PIN);
}
/**********************
 *   TASK FUNCTIONS
 **********************/
void Mode_Task(void* pvParameters)
{
    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_DELAY));

        int mode_button_state = gpio_get_level(MODE_PIN);
        if(mode_button_state == 0)
        {
            mode_index = (mode_index + 1) % 4;
            Mode = Mode_Table[mode_index];
            ESP_LOGI(TAG, "mode: %d", mode_index);
            xTaskNotifyGive(StateTask_Handle);
        }
    }
}

void Alarm_Task(void * pvParameters)
{
    time_t now;
    struct tm timeinfo;

    while(1)
    {
        time(&now);
        localtime_r(&now, &timeinfo);
        // ESP_LOGI(TAG, "The current date/time renewed");

        if(timeinfo.tm_hour == alarm1.hours && timeinfo.tm_min == alarm1.minutes)
            ESP_LOGI(TAG, "Alarm time match!!!");

        vTaskDelay(pdMS_TO_TICKS(1000));

    }
    vTaskDelete(NULL);
}


void Set_Rst_Task(void *params)
{

    while (1) 
    {
        // Block on entry until notification from mode recieved
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Wait for a short debounce delay
        vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_DELAY));

        int set_button_state = gpio_get_level(SET_PIN);
        int reset_button_state = gpio_get_level(RESET_PIN);
        if (set_button_state == 0) 
        {
            if(set_hour)
            {
                alarm1.hours = (alarm1.hours + 1) % 24;
                ESP_LOGI(TAG, "Hours Inc Button pressed!, hours: %d", alarm1.hours);
                ESP_LOGI(TAG, "minutes: %d", alarm1.minutes);
            }
            else
            {
                alarm1.minutes = (alarm1.minutes + 1) % 60;
                ESP_LOGI(TAG, "hours: %d", alarm1.hours);
                ESP_LOGI(TAG, "Minutes Inc Button pressed!, minutes: %d", alarm1.minutes);
            }
        }
        if (reset_button_state == 0) 
        {
            set_hour = !set_hour;
            ESP_LOGI(TAG, "Set hour: %d", set_hour);
        }
    }
}

/**********************
 *  STATIC FUNCTIONS
 **********************/
static bool example_notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

static void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    // copy a buffer's content to a specific area of the display
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

/* Rotate display and touch, when rotated screen in LVGL. Called when driver parameters are updated. */
static void example_lvgl_port_update_callback(lv_disp_drv_t *drv)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t) drv->user_data;

    switch (drv->rotated) {
    case LV_DISP_ROT_NONE:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, true, false);

        break;
    case LV_DISP_ROT_90:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, true, true);

        break;
    case LV_DISP_ROT_180:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, false, true);

        break;
    case LV_DISP_ROT_270:
        // Rotate LCD display
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, false, false);

        break;
    }
}

static void example_increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

void app_main(void)
{

    ESP_LOGI(mainTag, "Connect to WiFi");
    // Connect to WiFi     
    ESP_ERROR_CHECK( nvs_flash_init() );
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK( esp_event_loop_create_default() );

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    ESP_LOGI(mainTag, "PushButton Config");

    gpio_config_t io_conf_set, io_conf_rst, io_conf_mode;
    // Configure the GPIO pin for the button as an input
    io_conf_set.intr_type = GPIO_INTR_NEGEDGE;
    io_conf_set.mode = GPIO_MODE_INPUT;
    io_conf_set.pin_bit_mask = (1ULL << SET_PIN);
    io_conf_set.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf_set.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf_set);

    // Configure the GPIO pin for the button as an input
    io_conf_rst.intr_type = GPIO_INTR_NEGEDGE;
    io_conf_rst.mode = GPIO_MODE_INPUT;
    io_conf_rst.pin_bit_mask = (1ULL << RESET_PIN);
    io_conf_rst.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf_rst.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf_rst);

     // Configure the GPIO pin for the button as an input
    io_conf_mode.intr_type = GPIO_INTR_NEGEDGE;
    io_conf_mode.mode = GPIO_MODE_INPUT;
    io_conf_mode.pin_bit_mask = (1ULL << MODE_PIN);
    io_conf_mode.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf_mode.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf_mode);

    gpio_install_isr_service(0);

    gpio_isr_handler_add(SET_PIN, set_rst_interrupt_handler, (void *)SET_PIN);
    gpio_isr_handler_add(RESET_PIN, set_rst_interrupt_handler, (void *)RESET_PIN);
    gpio_isr_handler_add(MODE_PIN, mode_interrupt_handler, (void *)MODE_PIN);

    get_ntp_time();
    get_weather_update();
    
    xTaskCreate(Set_Rst_Task, "Set_Rst_Task", 2048, NULL, 1, &Set_Rst_task_handle);
    xTaskCreate(Alarm_Task, "Alarm_Task", 2048, NULL, 1, &AlarmTask_Handle);
    xTaskCreate(Mode_Task, "Mode_Task", 2048, NULL, 1, &ModeTask_Handle);


    ESP_LOGI(mainTag, "Create guiTask");
    //  /* If you want to use a task to create the graphic, you NEED to create a Pinned task
    //  * Otherwise there can be problem such as memory corruption and so on.
    //  * NOTE: When not using Wi-Fi nor Bluetooth you can pin the guiTask to core 0 */
    xTaskCreatePinnedToCore(guiTask, "gui", 4096*2, NULL, 0, NULL, 1);
}


/* Creates a semaphore to handle concurrent call to lvgl stuff
 * If you wish to call *any* lvgl function from other threads/tasks
 * you should lock on the very same semaphore! */
SemaphoreHandle_t xGuiSemaphore;

static void guiTask(void *pvParameter)
{
    xGuiSemaphore = xSemaphoreCreateMutex();

    static lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
    static lv_disp_drv_t disp_drv;      // contains callback functions

    ESP_LOGI(guiTag, "Turn off LCD backlight");
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    ESP_LOGI(guiTag, "Initialize SPI bus");
    spi_bus_config_t buscfg = {
        .sclk_io_num = EXAMPLE_PIN_NUM_SCLK,
        .mosi_io_num = EXAMPLE_PIN_NUM_MOSI,
        .miso_io_num = EXAMPLE_PIN_NUM_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = EXAMPLE_LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(guiTag, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = EXAMPLE_PIN_NUM_LCD_DC,
        .cs_gpio_num = EXAMPLE_PIN_NUM_LCD_CS,
        .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
        .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = example_notify_lvgl_flush_ready,
        .user_ctx = &disp_drv,
    };
    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_LCD_RST,
        .rgb_endian = LCD_RGB_ENDIAN_RGB,
        .bits_per_pixel = 16,
    };

    ESP_LOGI(guiTag, "Install ST7789 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, false));

    // user can flush pre-defined pattern to the screen before we turn on the screen or backlight
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    ESP_LOGI(guiTag, "Turn on LCD backlight");
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);

    ESP_LOGI(guiTag, "Initialize LVGL library");
    lv_init();

    // alloc draw buffers used by LVGL
    // it's recommended to choose the size of the draw buffer(s) to be at least 1/10 screen sized
    lv_color_t *buf1 = heap_caps_malloc(EXAMPLE_LCD_H_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1);
    lv_color_t *buf2 = heap_caps_malloc(EXAMPLE_LCD_H_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf2);
    // initialize LVGL draw buffers
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, EXAMPLE_LCD_H_RES * 20);

    ESP_LOGI(guiTag, "Register display driver to LVGL");
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = EXAMPLE_LCD_H_RES;
    disp_drv.ver_res = EXAMPLE_LCD_V_RES;
    disp_drv.flush_cb = example_lvgl_flush_cb;
    disp_drv.drv_update_cb = example_lvgl_port_update_callback;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;
    lv_disp_drv_register(&disp_drv);

    ESP_LOGI(guiTag, "Install LVGL tick timer");
    // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));
    

    ESP_LOGI(guiTag, "Demo starts");
    // example_lvgl_demo_ui(disp);
    // display_time();
    lv_task_modes();

    while (1) 
    {
        // raise the task priority of LVGL and/or reduce the handler period can improve the performance
        vTaskDelay(pdMS_TO_TICKS(10));

        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) 
        {
            // The task running lv_timer_handler should have lower priority than that running `lv_tick_inc`
            lv_timer_handler();
            xSemaphoreGive(xGuiSemaphore);
        }
       
    }
}
