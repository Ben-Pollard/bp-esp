#include "blockhaus_signals.h"

blockhaus_color_t blockhaus_color_of(int hue, int sig)
{
    switch (sig) {
    case BLOCKHAUS_SIGNAL_IDLE:
        return blockhaus_resting(hue);
    case BLOCKHAUS_SIGNAL_ACTIVE:
        return blockhaus_active(hue);
    case BLOCKHAUS_SIGNAL_WARNING:
        return blockhaus_active(BLOCKHAUS_HUE_MUSTARD);
    case BLOCKHAUS_SIGNAL_SUCCESS:
        return blockhaus_active(BLOCKHAUS_HUE_FOREST);
    case BLOCKHAUS_SIGNAL_FAULT:
        return blockhaus_active(BLOCKHAUS_HUE_MAROON);
    default:
        return blockhaus_resting(hue);
    }
}

int blockhaus_signal_hue(int hue, int sig)
{
    switch (sig) {
    case BLOCKHAUS_SIGNAL_WARNING: return BLOCKHAUS_HUE_MUSTARD;
    case BLOCKHAUS_SIGNAL_SUCCESS: return BLOCKHAUS_HUE_FOREST;
    case BLOCKHAUS_SIGNAL_FAULT:   return BLOCKHAUS_HUE_MAROON;
    default:                       return hue;
    }
}

int blockhaus_signal_period(int sig)
{
    switch (sig) {
    case BLOCKHAUS_SIGNAL_ACTIVE:  return 150;
    case BLOCKHAUS_SIGNAL_WARNING: return 110;
    case BLOCKHAUS_SIGNAL_FAULT:   return 75;
    default:                       return 0; /* idle: rest, success: hold */
    }
}

int blockhaus_signal_blink_period(int sig)
{
    switch (sig) {
    case BLOCKHAUS_SIGNAL_ACTIVE:  return 1200; /* calm on/off breathing */
    case BLOCKHAUS_SIGNAL_FAULT:   return 500;  /* distinct, urgent blink */
    default:                       return 0;    /* idle/warning/success: hold */
    }
}