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
```
git clone https://github.com/Abhishek-2310/SmartWatch-v2.git
```
This example shows how to use ST7789 display driver from Component manager in esp-idf project. These components are using API provided by `esp_lcd` component. This example will draw a fancy dash board with the LVGL library. For more information about porting the LVGL library, you can also refer to [another lvgl porting example](../i80_controller/README.md).


### Hardware Required

* An ESP32 WROOM 32 SoC Module
* A ST7789 LCD panel, with SPI interface (with/without STMPE610 SPI touch)
* A LiPo battery / port accessible to power supply and programming
* MCP73871 Battery management module
* Vibration motor
* Custom PCB

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

## Results
<p align="center">
<img src = "https://user-images.githubusercontent.com/9260214/28747595-19a41090-7471-11e7-826c-42c28ea7ae6e.jpeg" width = "500" height = "300"></p>

## Troubleshooting

* Why the LCD doesn't light up?
  * Check the backlight's turn-on level, and update it in `EXAMPLE_LCD_BK_LIGHT_ON_LEVEL`

## Contributing
Contributions are welcome! Feel free to open issues, submit pull requests, or provide feedback.

## License
This project is licensed under the MIT License.
