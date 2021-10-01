/*
 * hamlib.c: An interface to hamlib connected radios such as uBITX ;)
 *
 * API hooks are provided to control the following functions:
 *	VFO frequency
 *	Modulation mode
 */
#include <switch.h>
#include <hamlib/rig.h>
#include <hamlib/amplifier.h>
#include <hamlib/rotator.h>
#include "mod_hamradio.h"

// Initialize the connection to hamlib for the selected radio
switch_status_t radio_hamlib_init_radio(const int radio) {
    Radio_t *r = NULL;
    int rc = 0;

    if (radio < 0 || radio > globals.max_radios) {
       err_invalid_radio(radio);
       return SWITCH_STATUS_FALSE;
    }
    r = &Radios(radio);

    // Interface is already up, don't change it
    if (r->rig) {
       return SWITCH_STATUS_SUCCESS;
    }    

    if ((r->rig = rig_init(r->rig_model)) == NULL) {
       switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "radio%d connecting to hamlib failed (model: %d)\n", radio, r->rig_model);
       return SWITCH_STATUS_FALSE;
    }

    // XXX: We need to parse hamlib configuration URL
    r->rig_port.type.rig = RIG_PORT_SERIAL;
    r->rig_port.parm.serial.rate = 9600;
    r->rig_port.parm.serial.data_bits = 8;
    r->rig_port.parm.serial.stop_bits = 1;
    r->rig_port.parm.serial.parity = RIG_PARITY_NONE;
    r->rig_port.parm.serial.handshake = RIG_HANDSHAKE_NONE;
    strncpy(r->rig_port.pathname, r->rig_path, PATH_MAX);
    strncpy(r->rig->state.rigport.pathname, r->rig_path, PATH_MAX);

    if ((rc = rig_open(r->rig)) != RIG_OK) {
       switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "radio%d connecting to hamlib returned %s\n", radio, rigerror(rc));
       return SWITCH_STATUS_FALSE;
    }

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "radio%d connecting to hamlib succeeded.\n", radio);
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t radio_hamlib_init(void) {
    rig_set_debug_level(RIG_DEBUG_NONE);
    rig_load_all_backends();

    return SWITCH_STATUS_SUCCESS;
}

switch_status_t radio_hamlib_fini(void) {
    return SWITCH_STATUS_SUCCESS;
}
