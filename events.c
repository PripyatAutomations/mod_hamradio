/*
 * FreeSWITCH event core
 *
 * Here we handle interaction with the switch and other modules, in particular:
 *	mod_portaudio	- Support for PCI and USB audio widgets
 *	mod_conference	- Conferences
 */
#include "mod_hamradio.h"

static void radio_reload_configuration(switch_event_t *evt) {
   radio_load_configuration(true);
}

// Just dump the event information - this is useful for instrumenting new events */
static void radio_cry_event(switch_event_t *evt) {
   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "\n");
}

// Here we map each event + subclass to it's handler function, so radio_events_init
// can set it up in freeswitch
static RadioEvent_t radio_events[] = {
   ////////////////////
   // Audio activity //
   ////////////////////
   // Speech activity detected
   { SWITCH_EVENT_DETECTED_SPEECH, SWITCH_EVENT_SUBCLASS_ANY, radio_cry_event },
   // Detected tones (not DTMF?)
   { SWITCH_EVENT_DETECTED_TONE, SWITCH_EVENT_SUBCLASS_ANY, radio_cry_event },
   // Detected DTMF
   { SWITCH_EVENT_DTMF, SWITCH_EVENT_SUBCLASS_ANY, radio_cry_event },
   // Audio is coming in
   { SWITCH_EVENT_TALK, SWITCH_EVENT_SUBCLASS_ANY, radio_cry_event },
   // Audio has ended
   { SWITCH_EVENT_NOTALK, SWITCH_EVENT_SUBCLASS_ANY, radio_cry_event },
   // Reload the configuration
   { SWITCH_EVENT_RELOADXML, SWITCH_EVENT_SUBCLASS_ANY, radio_reload_configuration },

   ////////////////////
   // Channel Events //
   ////////////////////
   { SWITCH_EVENT_CHANNEL_CREATE, SWITCH_EVENT_SUBCLASS_ANY, radio_cry_event },
   { SWITCH_EVENT_CHANNEL_DESTROY, SWITCH_EVENT_SUBCLASS_ANY, radio_cry_event },
   { SWITCH_EVENT_CHANNEL_STATE, SWITCH_EVENT_SUBCLASS_ANY, radio_cry_event },

   // Terminating element - Do not modify.
   { 0, NULL, NULL }
};

// Register all our event hooks
void radio_events_init(void) {
   // bind all events we care about
   for (int i = 0; i < (sizeof(radio_events) / sizeof(RadioEvent_t)); i++) {
      // last element, exit the loop
      if (radio_events[i].event_type == 0 && radio_events[i].event_subclass == NULL && radio_events[i].event_handler == NULL)
         break;

      // If no handler, this event is disabled
      if (radio_events[i].event_handler == NULL) {
         continue;
      }

      // Hook the event
      if ((switch_event_bind(globals.modname, radio_events[i].event_type, radio_events[i].event_subclass, radio_events[i].event_handler, NULL) != SWITCH_STATUS_SUCCESS)) {
         switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind %i:%s handler!", radio_events[i].event_type, (radio_events[i].event_subclass != NULL ? radio_events[i].event_subclass : ""));
         break;
      }
   }
}

// Unregister event hooks
void radio_events_fini(void) {
   for (int i = 0; i < (sizeof(radio_events) / sizeof(RadioEvent_t)); i++) {
      // last element, exit the loop
      if (radio_events[i].event_type == 0 && radio_events[i].event_subclass == NULL && radio_events[i].event_handler == NULL)
         break;

      if (radio_events[i].event_handler != NULL)
         switch_event_unbind_callback(radio_events[i].event_handler);
   }
}
