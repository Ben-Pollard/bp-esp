# Board: cyd_2432s028 — Known Issues

- **Touch (XPT2046)**: Not in BMGR device matrix. Init manually in app. Pins: CLK=25, MOSI=32, MISO=39, CS=33, IRQ=36.
- **RGB LEDs (GPIO 4, 16, 17)**: Active-low, no BMGR device type. Init via `gpio_set_level` in app.
- **Light sensor (GPIO 34)**: Input-only pin (ADC1_CH6), no BMGR device. Read via ADC in app.
- **Speaker (GPIO 26)**: Connected to amplifier (active-high PA?), no BMGR audio device defined.
- **ST7789 display**: Non-R variant uses ST7789 (not ILI9341). May need color inversion (`invert_color: true`).
- **Free GPIOs broken out**: GPIO22 (P3/CN1), GPIO27 (CN1), GPIO35 (P3, input-only).
- **Serial TX/RX**: GPIO1/GPIO3 on P1 connector.