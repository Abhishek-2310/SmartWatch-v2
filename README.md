| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-H2 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- | -------- |

# ESP32-Powered SmartWatch
## Overview
The ESP32 microcontroller provides a powerful platform for developing embedded applications, and when combined with FreeRTOS (Real-Time Operating System) and LVGL (LittlevGL), it becomes even more versatile.
It offers:
* precise timekeeping through Network Time Protocol (NTP).
* allows users to set alarms
* features a stopwatch for various time-related tasks
* provides real-time weather updates, keeping you informed and prepared for changing conditions.
* simple home automation application using MQTT protocol

<br></br>

## Middlewares & Drivers
### FreeRTOS 
[FreeRTOS](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos.html) is a real-time operating system kernel designed for embedded systems. It allows you to run multiple tasks concurrently, each with its own priority, making it ideal for applications where timing is crucial.

### LVGL
[LittlevGL](https://github.com/lvgl/lvgl) is an open-source graphics library that provides everything you need to create a graphical user interface (GUI) on embedded systems. When integrated with FreeRTOS on ESP32, LVGL allows you to build sophisticated and visually appealing user interfaces.

### LCD Drivers
[esp_lcd](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/lcd.html) provides several panel drivers out-of box, e.g. ST7789, SSD1306, NT35510. However, there're a lot of other panels on the market, it's beyond `esp_lcd` component's responsibility to include them all.

<br></br>

## How to use the example
This example shows how to use ST7789 display driver from Component manager in esp-idf project. These components are using API provided by `esp_lcd` component. This example will draw a fancy dash board with the LVGL library. For more information about porting the LVGL library, you can also refer to [another lvgl porting example](../i80_controller/README.md).


### Hardware Required

* An ESP32 WROOM 32 SoC Module
* An GC9A01/ILI9341/ST7789 LCD panel, with SPI interface (with/without STMPE610 SPI touch)
* An USB cable for power supply and programming

### Hardware Connection

The connection between ESP Board and the LCD is as follows:

```
       ESP Chip                         ST7789 Panel + TOUCH
┌──────────────────────┐              ┌────────────────────┐
│             GND      ├─────────────►│ GND                │
│                      │              │                    │
│             3V3      ├─────────────►│ VCC                │
│                      │              │                    │
│             PCLK     ├─────────────►│ SCL                │
│                      │              │                    │
│             MOSI     ├─────────────►│ MOSI               │
│                      │              │                    │
│             MISO     |◄─────────────┤ MISO               │
│                      │              │                    │
│             RST      ├─────────────►│ RES                │
│                      │              │                    │
│             DC       ├─────────────►│ DC                 │
│                      │              │                    │
│             LCD CS   ├─────────────►│ LCD CS             │
│                      │              │                    │
│             TOUCH CS ├─────────────►│ TOUCH CS           │
│                      │              │                    │
│             BK_LIGHT ├─────────────►│ BLK                │
└──────────────────────┘              └────────────────────┘
```

The GPIO number used by this example can be changed in [lvgl_example_main.c](main/spi_lcd_touch_example_main.c).
Especially, please pay attention to the level used to turn on the LCD backlight, some LCD module needs a low level to turn it on, while others take a high level. You can change the backlight level macro `EXAMPLE_LCD_BK_LIGHT_ON_LEVEL` in [lvgl_example_main.c](main/spi_lcd_touch_example_main.c).

### Build and Flash

Run `idf.py -p PORT build flash monitor` to build, flash and monitor the project. A fancy animation will show up on the LCD as expected.

The first time you run `idf.py` for the example will cost extra time as the build system needs to address the component dependencies and downloads the missing components from registry into `managed_components` folder.

(To exit the serial monitor, type ``Ctrl-]``.)

See the [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/get-started/index.html) for full steps to configure and use ESP-IDF to build projects.

### Example Output

```bash
...
I (409) cpu_start: Starting scheduler on APP CPU.
I (419) example: Turn off LCD backlight
I (419) gpio: GPIO[2]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (429) example: Initialize SPI bus
I (439) example: Install panel IO
I (439) gpio: GPIO[5]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (449) example: Install GC9A01 panel driver
I (459) gpio: GPIO[3]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (589) gpio: GPIO[0]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0
I (589) example: Initialize touch controller STMPE610
I (589) STMPE610: TouchPad ID: 0x0811
I (589) STMPE610: TouchPad Ver: 0x03
I (599) example: Turn on LCD backlight
I (599) example: Initialize LVGL library
I (609) example: Register display driver to LVGL
I (619) example: Install LVGL tick timer
I (619) example: Display LVGL Meter Widget
...
```


## Troubleshooting

* Why the LCD doesn't light up?
  * Check the backlight's turn-on level, and update it in `EXAMPLE_LCD_BK_LIGHT_ON_LEVEL`

For any technical queries, please open an [issue] (https://github.com/espressif/esp-idf/issues) on GitHub. We will get back to you soon.
