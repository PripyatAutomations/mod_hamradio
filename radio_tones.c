/*
 * Support for storing and playing back TGML tones
 *
 * These come from the [tones] section of hamradio.conf
 */
#include <switch.h>
#include "mod_hamradio.h"

// We should get called when [tones] section is encountered
int radio_tones_init(void) {
    if (globals.radio_tones != NULL)
       dict_free(globals.radio_tones);

    globals.radio_tones = dict_new();
    return SWITCH_STATUS_SUCCESS;
}

void radio_tones_fini(void) {
   if (globals.radio_tones != NULL)
      dict_free(globals.radio_tones);

   globals.radio_tones = NULL;
}

// Play Nokia-encoded tones into the radio
int radio_send_tones(const int radio, const char *tone) {
    const char *tone_str = NULL;	// the retrieved data

    // Empty ringtones aren't an error but show throw a warning in debug mode
    if (tone == NULL) {
       switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "radio_send_tones(%d) called with empty tonestring, ignoring!\n", radio);
       return SWITCH_STATUS_SUCCESS;
    }

    // Retrieve the tone from the dictionary
    if ((tone_str = dict_get(globals.radio_tones, tone, NULL)) == NULL) {
       // XXX: Play the tone string into the channel
    }
    return SWITCH_STATUS_SUCCESS;
}

int radio_tone_store(const char *tone, const char *data) {
    return SWITCH_STATUS_SUCCESS;
}
