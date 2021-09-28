/*
 * radio_rigctl.c: An interface to rigctl connected radios such as uBITX ;)
 */
#include <switch.h>
#include <hamlib/rig.h>
#include <hamlib/amplifier.h>
#include <hamlib/rotator.h>
#include "mod_hamradio.h"

// Initialize the connection to rigctl for the selected radio
switch_status_t radio_rigctl_init(const int radio) {
    return SWITCH_STATUS_SUCCESS;
}
