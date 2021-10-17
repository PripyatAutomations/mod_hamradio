/*
 * Here we provide audio controls interface for ALSA devices
 */

#include "mod_hamradio.h"
#include <alsa/asoundlib.h>
#include <alsa/error.h>
#include <alsa/output.h>
#include <alsa/timer.h>
#include <alsa/pcm.h>
#include <alsa/mixer.h>


switch_status_t alsa_set_volume(const char *path, unsigned int level) {
    // Connect to alsa
    // Set the volume
    // Free resources
    return SWITCH_STATUS_SUCCESS;
}
