#include <switch.h>
#include "mod_hamradio.h"

// We should get called when [tones] section is encountered
int radio_tones_init(void) {
    if (globals.radio_tones != NULL)
       dict_free(globals.radio_tones);

    globals.radio_tones = dict_new();
    return SWITCH_STATUS_SUCCESS;
}

// Play Nokia-encoded tones into the radio
int radio_send_tones(const int radio, const char *tone) {
    return SWITCH_STATUS_SUCCESS;
}

int radio_tone_store(const char *tone, const char *data) {
    return SWITCH_STATUS_SUCCESS;
}
