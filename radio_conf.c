/*
 * Integration with mod_conference
 *
 * Here we manage creating/deleting conferences and joining/removing radios from them
 */
#include <switch.h>
#include "mod_hamradio.h"

int radio_conference_init(void) {
    return SWITCH_STATUS_SUCCESS;
}
