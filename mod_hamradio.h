////////////////////
// mod_hamradio.h //
////////////////////
#if	!defined(MOD_HAMRADIO_H)
#define	MOD_HAMRADIO_H
#include <gpiod.h>
#include <time.h>
#include <stdlib.h>

// (dict)ionary data type like other languages have
#include "dict.h"

// ini-style configuration support (yes, i know fs has its own...)
#include "config.h"

// Common to all radios
#include "radio.h"

// Support for GPIO controlled (relays/optocouplers and COS/TOS inputs) radios
#include "radio_gpio.h"

// Support for rigctl controlled radios (NYI)
#include "radio_rigctl.h"

// Support for playing back saved short tone melodies
#include "tones.h"

#define	MAX_GPIO	128		// maximum GPIO pin # (this is intentionally high)
#define	HAMRADIO_CONF	"/etc/freeswitch/hamradio.conf" // configuration file

// This will be used to keep track of the state of a channel's voice activity
typedef enum {
   CHAN_QUIET = 0,
   CHAN_NOISE,
   CHAN_VOICE,
   CHAN_ERROR
} chan_vad_activity;

// Global data
struct Globals {
   int alive;				// are we shutting down?
   int max_radios;			// Highest radio # allowed to be configured
   int max_conferences;			// Maximum allowed concurrent conferences
   int poll_interval;			// How long to sleep in the housekeeping thread
                                        // before rescanning the radios. This controls CPU load
   struct Radio *Radios;		// radio structures
   switch_mutex_t *mutex;
   switch_memory_pool_t  *pool;		// our memory pool
   dict *cfg;				// configuration from .conf
   dict *radio_tones;			// Radio tones
   // XXX: This needs moved when we add support for multiple GPIO chips...
   struct gpiod_chip *gpiochip;
};
typedef struct Globals Globals_t;

extern Globals_t globals;		// in mod_hamradio.c

////////////////
// Prototypes //
////////////////
// Configuration file
extern switch_status_t load_configuration(switch_bool_t reload);
extern time_t timestr_to_time(const char *str, const time_t def);
extern char *time_to_timestr(time_t itime);

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
