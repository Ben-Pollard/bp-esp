# Debugging & Testing Strategy

Note: this is written for an agent (non-interactive tools). Avoid methods requiring human-in-the-loop (GDB interactive step, GPIO probing, oscilloscope).

## Debugging — approach by symptom

| Symptom | First step | Escalate to |
|---|---|---|
| Boot loop / panic | `idf.py -p <port> monitor` — read the panic reason | Core dump decode (`idf.py coredump-info`) or `idf.py coredump-debug` |
| Wrong behaviour, no crash | `ESP_LOGW/TAG` at key points; check Kconfig log level | GDB Stub runtime (`CONFIG_ESP_SYSTEM_GDBSTUB_RUNTIME`) via batch GDB over UART, or OpenOCD + scripted JTAG |
| Task starvation / watchdog | Check TWDT config; add `vTaskDelay()` or yield | Trace FreeRTOS task stats (`vTaskGetRunTimeStats`) |
| Memory corruption | Enable stack watchpoints, heap poisoning, `CONFIG_HEAP_CORRUPTION_DETECTION` | Heap tracing or core dump analysis |
| Hardware peripheral not responding | `ioctl`-style register read via monitor; verify peripheral init order | JTAG register inspection via OpenOCD script |
| Intermittent / timing-sensitive | Reduce optimisation (`-Og`); narrow window with conditional logs | JTAG hardware breakpoint via OpenOCD script |

## Testing — when to use what

| Layer | Tool | When |
|---|---|---|
| **Unit (host-side)** | CMock + Unity on Linux (no HW) | Pure logic: math, state machines, protocol parsers |
| **Unit (on-target)** | ESP-IDF test framework (`idf.py test`) + `pytest-embedded[serial]` | Driver behaviour: I2C sensor read, SPI DMA, button debounce |
| **Integration** | `pytest-embedded[serial]` on a reference board | Multi-component flow: WiFi connect → MQTT publish → OTA |
| **System / stress** | Long-running `pytest-embedded[serial]` | Memory leaks over hours, WiFi reconnection, power management |
| **Bring-up / one-off** | Manual `idf.py flash monitor` | First flash on new hardware, after schematic change |

## Principles

- **Hardware-in-the-loop** from day one — mock/fakes catch logic bugs but miss real-world timing and electrical issues
- **Log verbosity** is a dial: start verbose during dev, tighten for release, never strip entirely
- **Failing test first**: reproduce the bug with a test before fixing (pin the regression)
- **Two-tier monitor**: brief boot check after every flash; deep log analysis only on failure
- **Watchdog discipline**: enable in every build, even debug — catch hangs early, disable only when hunting a specific freeze