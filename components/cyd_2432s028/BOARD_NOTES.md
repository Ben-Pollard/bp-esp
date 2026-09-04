# Board: cyd_2432s028

- **Display**: ILI9341 320×240, SPI mode 0 on HSPI (SPI2_HOST), pins: MOSI=13, CLK=14, CS=15, DC=2, RST=4, BL=21
- **Touch (XPT2046)**: Bit-banged GPIO (not real HW SPI), pins: CLK=25, MOSI=32, MISO=39, CS=33, IRQ=36. Init manually in app.
- **SD card**: SPI on VSPI (SPI3_HOST), pins: MOSI=23, MISO=19, CLK=18, CS=5
- **RGB LED**: Single GPIO16, active low. Init via `gpio_set_level` in app.
- **Light sensor (photoresistor)**: GPIO34 (ADC1_CH6, input-only). Read via ADC in app.
- **IR receiver**: GPIO35 (input-only), IR transmitter: GPIO17
- **DHT11**: GPIO26
- **Speaker amplifier**: GPIO26 (shared with DHT11?)
- **Free GPIOs**: GPIO22 (P3/CN1), GPIO27 (CN1)
- **Serial TX/RX**: GPIO1/GPIO3 on P1 connector.
- **Source**: Manufacturer SDK from esp32-2432s028.rar, IDF v4.3, lvgl_esp32_drivers, LVGL v7