#pragma once

#include "blockhaus_palette.h"

#ifdef __cplusplus
extern "C" {
#endif

enum blockhaus_signal_t {
    BLOCKHAUS_SIGNAL_IDLE = 0,
    BLOCKHAUS_SIGNAL_ACTIVE,
    BLOCKHAUS_SIGNAL_ATTENTION,
    BLOCKHAUS_SIGNAL_WARNING,
    BLOCKHAUS_SIGNAL_CRITICAL,
    BLOCKHAUS_SIGNAL_COUNT
};

blockhaus_color_t blockhaus_color_of(int hue, int sig);

#ifdef __cplusplus
}
#endif