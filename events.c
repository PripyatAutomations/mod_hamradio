#include "mod_hamradio.h"

static void radio_event_handler(switch_event_t *event) {
   // Module reload
   if (event->event_id == SWITCH_EVENT_RELOADXML) {
      radio_load_configuration(true);
   }
}

void radio_events_init(void) {
   // bind configuration reload event to our event handler
   if ((switch_event_bind(globals.modname, SWITCH_EVENT_RELOADXML, NULL, radio_event_handler, NULL) != SWITCH_STATUS_SUCCESS)) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind reloadxml handler!");
   }
}

void radio_events_fini(void) {
   switch_event_unbind_callback(radio_event_handler);
}
