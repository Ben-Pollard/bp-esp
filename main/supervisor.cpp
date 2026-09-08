#include "supervisor.h"
#include "logic/brightness.h"
#include "esp_task_wdt.h"
#include "esp_log.h"

static const char *TAG = "SUPER";

#define TICK_MS       10
#define TICKS_LDR     10
#define TICKS_BRIGHT  25

Supervisor::Supervisor() {}
Supervisor::~Supervisor() {}

IHardware &Supervisor::hardware() const { return *m_hw; }

bool Supervisor::start(IHardware &hw)
{
    m_hw = &hw;
    m_cmd_queue = xQueueCreate(8, sizeof(Command));
    m_mutex = xSemaphoreCreateMutex();
    if (!m_cmd_queue || !m_mutex) {
        ESP_LOGE(TAG, "Queue/mutex creation failed");
        return false;
    }

    BaseType_t r = xTaskCreatePinnedToCore(task_main, "supervisor", 3072, this, 1, &m_task, 1);
    if (r != pdPASS) {
        ESP_LOGE(TAG, "Task creation failed");
        return false;
    }
    esp_task_wdt_add(m_task);
    ESP_LOGI(TAG, "Started on core 1, tick=%dms", TICK_MS);
    return true;
}

bool Supervisor::submit(const Command &cmd)
{
    return xQueueSend(m_cmd_queue, &cmd, 0) == pdTRUE;
}

AppState Supervisor::snapshot() const
{
    xSemaphoreTake(m_mutex, portMAX_DELAY);
    AppState s = m_state;
    xSemaphoreGive(m_mutex);
    return s;
}

void Supervisor::task_main(void *arg)
{
    Supervisor *self = (Supervisor *)arg;
    TickType_t last_wake = xTaskGetTickCount();
    while (1) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(TICK_MS));
        esp_task_wdt_reset();
        self->tick();
    }
}

void Supervisor::tick()
{
    Command cmd;
    while (xQueueReceive(m_cmd_queue, &cmd, 0) == pdTRUE) {
        switch (cmd.kind) {
        case CommandKind::SetRgb:
            m_hw->set_rgb(cmd.a, cmd.b, cmd.c);
            m_state.rgb[0] = cmd.a;
            m_state.rgb[1] = cmd.b;
            m_state.rgb[2] = cmd.c;
            break;
        case CommandKind::SetBacklight:
            m_state.backlight = cmd.a;
            break;
        }
    }

    m_ticks++;

    if (m_ticks % TICKS_LDR == 0) {
        uint16_t raw = 0;
        if (m_hw->read_ldr(&raw)) {
            m_state.ldr_raw = raw;
            if (raw < m_raw_min) m_raw_min = raw;
            if (raw > m_raw_max) m_raw_max = raw;
        }
    }

    if (m_ticks % TICKS_BRIGHT == 0) {
        m_state.ldr_factor = brightness_factor(m_state.ldr_raw, m_raw_min, m_raw_max);
        m_state.backlight_eff = effective_backlight(m_state.backlight, m_state.ldr_factor);
        m_hw->set_backlight(m_state.backlight_eff);
    }

    xSemaphoreTake(m_mutex, portMAX_DELAY);
    m_state.version++;
    xSemaphoreGive(m_mutex);
}