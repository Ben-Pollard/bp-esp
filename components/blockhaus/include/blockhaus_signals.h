#pragma once

#include "blockhaus_palette.h"

#ifdef __cplusplus
extern "C" {
#endif

enum blockhaus_signal_t {
    BLOCKHAUS_SIGNAL_IDLE = 0,
    BLOCKHAUS_SIGNAL_ACTIVE,
    BLOCKHAUS_SIGNAL_WARNING,
    BLOCKHAUS_SIGNAL_SUCCESS,
    BLOCKHAUS_SIGNAL_FAULT,
    BLOCKHAUS_SIGNAL_COUNT
};

blockhaus_color_t blockhaus_color_of(int hue, int sig);

/* The hue a signal should render in, decoupled from the object's own hue.
 * IDLE/ACTIVE keep the object hue; WARNING/SUCCESS/FAULT override to their
 * semantic hue (mustard/forest/maroon). Single-block indicators and strips
 * both read their colour from here so the meanings can never drift apart. */
int blockhaus_signal_hue(int hue, int sig);

/* The pulse tempo for a signal, in ms. IDLE and SUCCESS return 0 (static:
 * idle rests, success holds). ACTIVE pulses slowly, WARNING/FAULT pulse
 * faster. Zero means "no motion, hold the colour". This drives strips. */
int blockhaus_signal_period(int sig);

/* The blink tempo for a single-block indicator, in ms. A blink is a clear
 * on/off alternation (dark <-> light), not a tremble, so these are much
 * slower than strip pulse periods. 0 means "no blink, hold the colour". */
int blockhaus_signal_blink_period(int sig);

#ifdef __cplusplus
}
#endif