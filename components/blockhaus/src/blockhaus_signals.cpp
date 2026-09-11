#include "blockhaus_signals.h"

blockhaus_color_t blockhaus_color_of(int hue, int sig)
{
    if (sig == BLOCKHAUS_SIGNAL_IDLE)
        return blockhaus_resting(hue);

    if (sig == BLOCKHAUS_SIGNAL_ACTIVE)
        return blockhaus_active(hue);

    if (sig == BLOCKHAUS_SIGNAL_WARNING)
        return blockhaus_active(BLOCKHAUS_HUE_MUSTARD);

    if (sig == BLOCKHAUS_SIGNAL_CRITICAL)
        return blockhaus_active(BLOCKHAUS_HUE_MAROON);

    if (sig == BLOCKHAUS_SIGNAL_ATTENTION)
        return blockhaus_active(BLOCKHAUS_HUE_MUSTARD);

    return blockhaus_resting(hue);
}