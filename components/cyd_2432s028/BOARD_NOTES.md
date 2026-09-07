# Board: cyd_2432s028

## Pinout (confirmed via RNT + community)

| Function | Pin | Details |
|---|---|---|
| **Display** | ILI9341 | SPI mode 0 on HSPI (SPI2_HOST): MOSI=13, CLK=14, CS=15, DC=2, RST=4 |
| **Backlight** | GPIO21 | PWM-capable via LEDC (TIMER_0, CHANNEL_0). Active high. |
| **Touch (XPT2046)** | Bit-banged | CLK=25, MOSI=32, MISO=39, CS=33, IRQ=36. Not HW SPI — software bit-bang. |
| **SD card** | SPI3 (VSPI) | MOSI=23, MISO=19, CLK=18, CS=5 |
| **RGB LED (Red)** | GPIO4 | Active low. Separate pin per colour (not addressable). |
| **RGB LED (Green)** | GPIO16 | Active low. |
| **RGB LED (Blue)** | GPIO17 | Active low. |
| **LDR (photoresistor)** | GPIO34 | GT36516 photoconductive cell. ADC1_CH6. Voltage divider with 1M pull-up (poor dynamic range — relative only). |
| **Speaker** | GPIO26 | 2P 1.25mm JST connector. No speaker shipped with board; you plug one in. |
| **BOOT button** | GPIO0 | Active low, internal pull-up. |
| **Extended IO** | P3: GPIO35(input-only), GPIO22; CN1: GPIO22, GPIO27 |
| **Serial** | TX=GPIO1, RX=GPIO3 | On P1 connector via CH340. |

## Notes

- This is the `cyd_2432s028` variant (2.8", ESP32-WROOM-32). There are multiple CYD variants with different pin reassignments (e.g. some use GPIO27 for backlight).
- **No IR sensor** on this board (contra some manufacturer docs referencing "reserved IO interface").
- The "temperature and humidity sensor interface" is just a connector — you plug a DHT11 (or similar) into it; it's not populated.
- LDR circuit: R15=1MΩ pull-up, R19=1MΩ in parallel with LDR → very shallow voltage swing (~0.02V–0.6V). Community suggests hacking R15/R19 for usable range.
- The display RST pin shares GPIO4 with the red LED. After reset is released, the red LED is free for PWM.

## Product Description
This LCD module uses ESP32-WROOM-32 module as the main control, the main
frequency can reach 240MHz, 520KB SRAM, 448KB ROM, Flash size is 4MB. The
display resolution is 240x320, resistive touch. The module includes LCD display
screen, backlight control circuit, touch screen control circuit, speaker drive circuit,
photosensitive circuit and RGB-LED control circuit. TF card interface, serial interface,
DHT11-compatible sensor interface, and reserved IO port interfaces.

## Source
Manufacturer SDK from esp32-2432s028.rar, IDF v4.3, lvgl_esp32_drivers, LVGL v7.