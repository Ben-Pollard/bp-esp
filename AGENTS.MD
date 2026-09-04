## Tooling & CLI
- run idf via the esp-idf mcp if available
- to activate the python env for idf.py:
    - `eim list`
    - `eim select <version>`
    - `source <path/to/activate_idf*.sh>`

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