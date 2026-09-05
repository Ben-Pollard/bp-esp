## Connect to device
Prove the device is connected and identify its port:
- `./scripts/connect.sh`

## idf.py
idf.py runs in a python environment managed by eim. You MUST invoke it using eim run, wrapping all python options and args in quotes, like this:
- `eim run "idf.py build"`

## Configuration — Commands

| Command | Purpose | When to use |
|---|---|---|
| `eim run "idf.py --preset debug build"` | Build using the `debug` preset from `CMakePresets.json` | Normal development build |
| `eim run "idf.py --preset release build"` | Build using the `release` preset | Production build |
| `eim run "idf.py reconfigure"` | Re-run CMake **without building** | After modifying `sdkconfig.defaults*` or `CMakePresets.json` — validates the config merge before a full build |
| `eim run "idf.py refresh-config"` | Resolve default-value conflicts between Kconfig definitions and current `sdkconfig` | When Kconfig defaults change (e.g. after a component update) and sdkconfig holds stale values. Takes `--policy sdkconfig\|kconfig\|interactive` |
| `eim run "idf.py config-report"` | Generate JSON config diagnostics (`build/<preset>/config/kconfig_parse_report.json`) | Debugging config issues — shows duplicate symbol defs, missing deps, warnings |
| `eim run "idf.py save-defconfig"` | Write current `sdkconfig` values (that differ from IDF defaults) back to `sdkconfig.defaults` | After `menuconfig`-style changes *if you were forced to use it*, or when capturing a known-good config state |
| `eim run "idf.py bmgr -b <board>"` | Validate board YAML and regenerate `board_manager.defaults` + C structs + `Kconfig.projbuild` into `gen_bmgr_codes/` | After editing `board_peripherals.yaml` or `board_devices.yaml` |
| `eim run "idf.py bmgr -l"` | List known boards | Checking if a board is already registered |
| `eim run "idf.py set-target <chip>"` | Change target chip | **WARNING**: wipes `sdkconfig`. Must re-run `bmgr -b` + `reconfigure` afterward |

### Config Conventions

- **No `menuconfig`**: It writes directly to `sdkconfig`, which is auto-generated and gets overwritten by `set-target`, `bmgr`, or `reconfigure`. Source of truth is the `.defaults` files, not `sdkconfig` itself.
- **Always use presets**: `eim run "idf.py --preset debug build"`, not bare `idf.py build`. Presets define build directory and `SDKCONFIG_DEFAULTS` merge order.
- **Kconfig vs sdkconfig.defaults**: `Kconfig`/`Kconfig.projbuild` defines *what options exist* (name, type, default, constraints). Create one only when you need a custom `CONFIG_` symbol in C/C++ code that IDF and managed components don't already provide.

### Config Merge Order

Controlled by `SDKCONFIG_DEFAULTS` in `CMakePresets.json`. Later files override earlier ones.

| Preset | Files (in order) |
|---|---|
| `debug` | `sdkconfig.defaults` → `board_manager.defaults` |
| `release` | `sdkconfig.defaults` → `sdkconfig.defaults.release` → `board_manager.defaults` |

### Board Manager

Pinout is defined in `components/<board_name>/`:
- `board_info.yaml` — metadata (chip, version, manufacturer)
- `board_peripherals.yaml` — physical interfaces (SPI buses with pin numbers, GPIOs)
- `board_devices.yaml` — functional devices (ILI9341 display, SD card, button) mapped to their peripherals

`eim run "idf.py bmgr -b <board>"` generates into `components/gen_bmgr_codes/`:

| Generated file | Purpose |
|---|---|
| `board_manager.defaults` | Kconfig feature flags (`CONFIG_ESP_BOARD_DEV_DISPLAY_LCD_SUPPORT=y`) — merged into `sdkconfig` |
| `gen_board_periph_config.c` | Pin assignments as C structs (`.mosi_io_num = 13`) — linked directly into the binary |
| `gen_board_device_config.c` | Device params as C structs (`.cs_gpio_num = 15`, ILI9341 config) — linked directly |


Note: Pin data skips Kconfig entirely and goes straight to C structs. Feature flags go through Kconfig → sdkconfig; pin assignments go through C code generation. Both are generated from the same YAML source by `bmgr`.

### Integrating generated code

Generated structs are the only source of pin/config values — never hardcode pins
in C. At runtime, get a device with `esp_board_manager_get_device_handle("<name>", &handle)` and cast to its published handles struct. Two integration paths:

- **Extant driver**: use an existing board-manager `type` (handlers live under `managed_components/.../esp_board_manager/devices/<type>/`). Board-specific tweaks go through a weak factory hook in `components/<board>/setup_device.c` — never edit generated files directly.
- **Custom driver**: set `type: custom` in `board_devices.yaml`; bmgr emits a `dev_custom_<name>_config_t` struct with fields inferred from `config:`. Keep the driver in its own component (no board-manager dependency), register it from a board source file with `CUSTOM_DEVICE_IMPLEMENT(<name>, init, deinit)`, and reference the component in `dependencies:` via `${BOARD_PATH}/...` (flows into `gen_bmgr_codes/idf_component.yml`). Custom devices support at most 4 peripherals.

Before writing a custom driver, check whether a `sub_type` handler already exists for the target `type` — some are shipped as stubs (e.g. `dev_lcd_touch` is I2C-only).

### Typical Configuration Workflow

```
# After changing board YAML:
eim run "idf.py bmgr -b cyd_2432s028"

# After changing sdkconfig.defaults or CMakePresets.json:
eim run "idf.py --preset debug reconfigure"     # validate merge, no build
eim run "idf.py --preset debug config-report"   # check for config issues

# After component updates (Kconfig defaults changed):
eim run "idf.py --preset debug refresh-config --policy interactive"

# Normal build:
eim run "idf.py --preset debug build"
```


## Memory & Firmware Standards
* **Memory Hierarchy:** DRAM (Data), IRAM (Instructions - must hold ISRs/Flash-write code), RTC Memory (Deep Sleep), PSRAM (External).
* **Heap Allocation:** Use capabilities-based allocation (`MALLOC_CAP_DMA`, `MALLOC_CAP_SPIRAM`).
* **Modern C++:** Apply RAII universally. Never use raw `new`/`delete`. Enforce static allocation or smart pointers.
* **Reliability:** Always include Watchdog Timers (IWDT/TWDT). Implement short ISRs.

## Testing — when to use what
| Layer | Tool | When |
|---|---|---|
| **Unit (host-side)** | CMock + Unity on Linux (no HW) | Pure logic: math, state machines, protocol parsers |
| **Unit (on-target)** | ESP-IDF test framework (`idf.py test`) + `pytest-embedded[serial]` | Driver behaviour: I2C sensor read, SPI DMA, button debounce |
| **Integration** | `pytest-embedded[serial]` on a reference board | Multi-component flow: WiFi connect → MQTT publish → OTA |
| **System / stress** | Long-running `pytest-embedded[serial]` | Memory leaks over hours, WiFi reconnection, power management |
| **Bring-up / one-off** | Manual `idf.py flash monitor` | First flash on new hardware, after schematic change |

## Testing Principles
- **Hardware-in-the-loop** from day one — mock/fakes catch logic bugs but miss real-world timing and electrical issues
- **Watchdog discipline**: enable in every build, even debug — catch hangs early, disable only when hunting a specific freeze