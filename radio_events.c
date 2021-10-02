/*
 * FreeSWITCH event core
 *
 * Here we handle interaction with the switch and other modules, in particular:
 *	mod_portaudio	- Support for PCI and USB audio widgets
 *	mod_conference	- Conferences
 */
#include "mod_hamradio.h"

static void radio_reload_configuration(switch_event_t *evt) {
   // Exclude some excessively noisy, yet useless events
   if ((evt->event_id == SWITCH_EVENT_HEARTBEAT) ||
       (evt->event_id == SWITCH_EVENT_RE_SCHEDULE) ||
       (evt->event_id == SWITCH_EVENT_API) ||
       (evt->event_id == SWITCH_EVENT_LOG) ||
       (evt->event_id == SWITCH_EVENT_STARTUP) ||
       (evt->event_id == SWITCH_EVENT_SHUTDOWN))
      return;

   radio_load_configuration(true);
}

// Just dump the event information - this is useful for instrumenting new events */
static void radio_cry_event(switch_event_t *evt) {
   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "--Unhandled Event--\n");
   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Key: %lu Flags: %i\n", evt->key, evt->flags);
   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "ID: %s Subclass: %s Priority: %03i \n", switch_event_name(evt->event_id), evt->subclass_name, evt->priority);
   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Owner: %s\n", evt->owner);
   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "bind_user_data: %p, event_user_data: %p\n", evt->bind_user_data, evt->event_user_data);
   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "--Unhandled Event--\n");
}

// Here we map each event + subclass to it's handler function, so radio_events_init
// can set it up in freeswitch
static RadioEvent_t radio_events[] = {
   ////////////////////
   // Audio activity //
   ////////////////////

   // log all events... This is extremely verbose and likely only useful for developers ;)
   { SWITCH_EVENT_ALL, SWITCH_EVENT_SUBCLASS_ANY, radio_cry_event },
#if	0
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
#endif
   // Reload the configuration
   { SWITCH_EVENT_RELOADXML, SWITCH_EVENT_SUBCLASS_ANY, radio_reload_configuration },

#if	0
   ////////////////////
   // Channel Events //
   ////////////////////
   { SWITCH_EVENT_CHANNEL_CREATE, SWITCH_EVENT_SUBCLASS_ANY, radio_cry_event },
   { SWITCH_EVENT_CHANNEL_DESTROY, SWITCH_EVENT_SUBCLASS_ANY, radio_cry_event },
   { SWITCH_EVENT_CHANNEL_STATE, SWITCH_EVENT_SUBCLASS_ANY, radio_cry_event },
   { SWITCH_EVENT_CHANNEL_CALLSTATE, SWITCH_EVENT_SUBCLASS_ANY, radio_cry_event },
   { SWITCH_EVENT_CHANNEL_BRIDGE, SWITCH_EVENT_SUBCLASS_ANY, radio_cry_event },
   { SWITCH_EVENT_CHANNEL_UNBRIDGE, SWITCH_EVENT_SUBCLASS_ANY, radio_cry_event },
   { SWITCH_EVENT_CHANNEL_HANGUP, SWITCH_EVENT_SUBCLASS_ANY, radio_cry_event },
#endif

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
         switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind event %s:%s handler!", switch_event_name(radio_events[i].event_type), (radio_events[i].event_subclass != NULL ? radio_events[i].event_subclass : ""));
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
