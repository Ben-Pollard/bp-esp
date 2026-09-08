# bp-esp Context

An ESP32 firmware project that drives a CYD (ESP32-2432S028) touchscreen with LED PWM, ambient-light sensing, and an interactive particle playground. Designed as a foundation for future application logic (e.g. a docker dashboard over WiFi).

## Language

**Supervisor**: The time-driven RTOS task pinned to core 1 that owns the tick cadence (10 ms), drains the command queue, runs scheduled jobs (LDR read, brightness recompute), and publishes the state snapshot.
_Avoid_: Core loop, main loop, scheduler.

**Snapshot**: A plain struct (`AppState`) of sensor/LED/configuration values published by the supervisor under a mutex and polled by the UI. Single-writer, multiple-reader.
_Avoid_: Shared state, globals, context.

**Command**: An intent from the UI to the supervisor (e.g. "set LED to 50%"). Carried over a FreeRTOS queue.
_Avoid_: Event, message, action.

**Job**: A unit of scheduled work the supervisor runs at a cadence — currently the LDR read (100 ms) and the brightness recompute (250 ms).

**Adapter**: (Hardware) The concrete `BoardAdapter` that implements the `IHardware` port. The only file importing `esp_board_manager` types.
_Avoid_: Driver, handler, backend.

**Activity state** (reserved): A future field in `AppState` (e.g. `display_on`) that allows the supervisor to drop its cadence and gate the display for power management.

## Relationships

- The **Supervisor** runs **Jobs** at fixed cadences and drains **Commands** from the queue.
- The UI submits **Commands** to the supervisor via `submit()`.
- The UI reads the **Snapshot** via `snapshot()` (mutex copy).
- The **Adapter** sits behind the `IHardware` port — the supervisor sees only the port.

## Example dialogue

> **Dev**: "When the user drags the backlight slider, does the LED change immediately?"
> **Architect**: "The slider knob moves instantly (render-local, LVGL). A **Command** goes to the **Supervisor** queue. On the next tick (≤10 ms), the **Adapter** applies it. The **Snapshot** updates, and the UI polls it to sync the label."

## Flagged ambiguities

- "double-buffering" was used to mean both *framebuffer* (pixel memory, LVGL) and *snapshot* (state). Resolved: use **framebuffer** for pixels, **snapshot** for state.