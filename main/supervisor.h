#pragma once
#include "app_context.h"
#include "hardware/hardware.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

class Supervisor {
public:
    Supervisor();
    ~Supervisor();

    bool start(IHardware &hw);
    bool submit(const Command &cmd);
    AppState snapshot() const;
    IHardware &hardware() const;

private:
    static void task_main(void *arg);
    void tick();

    IHardware *m_hw = nullptr;
    QueueHandle_t m_cmd_queue = nullptr;
    SemaphoreHandle_t m_mutex = nullptr;
    TaskHandle_t m_task = nullptr;
    AppState m_state;
    uint16_t m_raw_min = 4095;
    uint16_t m_raw_max = 0;
    int m_ticks = 0;
};