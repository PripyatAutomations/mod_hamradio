////////////////////
// mod_hamradio.h //
////////////////////
#if	!defined(MOD_HAMRADIO_H)
#define	MOD_HAMRADIO_H
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <gpiod.h>
#include <time.h>
#include <stdlib.h>

// our logging facility
#include "logging.h"

// freeswitch stuff
#include "radio_channel.h"
#include "radio_events.h"
#include "radio_endpoint.h"
// conferences
#include "radio_conf.h"

// (dict)ionary data type like other languages have
#include "dict.h"

// ini-style configuration support (yes, i know fs has its own...)
#include "radio_cfg.h"

// Common to all radios
#include "radio.h"

// Support for GPIO controlled (relays/optocouplers and COS/TOS inputs) radios
#include "radio_gpio.h"

// Support for rigctl controlled radios (NYI)
#include "hamlib.h"

// Support for playing back saved short tone melodies
#include "radio_tones.h"

// IDentification (CW + voice)
#include "radio_id.h"
#include "radio_tts.h"

// Voice Activity Detection
#include "radio_vad.h"


#define	MAX_GPIO	128		// maximum GPIO pin # (this is intentionally high)
#define	HAMRADIO_CONF	"/etc/freeswitch/hamradio.conf" // configuration file

struct RadioEvent {
   switch_event_types_t event_type;
   char *event_subclass;
   void (*event_handler)(switch_event_t *evt);
};
typedef struct RadioEvent RadioEvent_t;

struct Globals {
   char *modname;
   int alive;				// are we shutting down?
   int max_radios;			// Highest radio # allowed to be configured
   int max_conferences;			// Maximum allowed concurrent conferences
   int poll_interval;			// How long to sleep in the housekeeping thread
                                        // before rescanning the radios. This controls CPU load
   struct Radio *Radios;		// radio structures
   switch_mutex_t *mutex;
   switch_memory_pool_t  *pool;		// our memory pool
   switch_api_interface_t *api_interface;
   switch_application_interface_t *app_interface;
   dict *cfg;				// configuration from .conf
   dict *radio_tones;			// Radio tones
   // XXX: This needs moved when we add support for multiple GPIO chips...
   struct gpiod_chip *gpiochip;

   // Auto-ID stuff
   time_t timeout_id;			// max times between IDs
   ID_Mode_t id_type;			// Type of ID (None, CW, Voice, or Both)
};
typedef struct Globals Globals_t;

extern Globals_t globals;		// in mod_hamradio.c

////////////////
// Prototypes //
////////////////
// Configuration file
extern switch_status_t radio_load_configuration(switch_bool_t reload);

static inline int str_to_intbool(const char *str) {
   if (!strcasecmp(str, "ON") || !strcasecmp(str, "TRUE") || !strcasecmp(str, "1")) {
      return 1; // True
   } else if (!strcasecmp(str, "OFF") || !strcasecmp(str, "FALSE") || !strcasecmp(str, "0")) {
      return 0;	// False
   } else {	// Invalid boolean value
      return -1;
   }
}

#endif	// (MOD_HAMRADIO_H)
