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