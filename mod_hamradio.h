////////////////////
// mod_hamradio.h //
////////////////////
#if	!defined(MOD_HAMRADIO_H)
#define	MOD_HAMRADIO_H
#include <gpiod.h>
#include <time.h>
#include <stdlib.h>
#include "config.h"
// Common to all radios
#include "radio.h"
// Support for GPIO controlled (relays/optocouplers and COS/TOS inputs) radios
#include "radio_gpio.h"
// Support for rigctl controlled radios (NYI)
#include "radio_rigctl.h"

// This is the maximum number of radios that can be configured
// I'm not sure why you would have more than 8 on a pi, but it might work!
#if	!defined(MAX_RADIOS)
#warning "MAX_RADIOS undefined. Assuming 8 but ALL source files must use the same value or crashes/weird bugs WILL occur!"
#define	MAX_RADIOS	8
#endif

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
   int max_radios;			// Highest radio # configured
   struct Radio Radios[MAX_RADIOS];	// Radio structures
   switch_mutex_t *mutex;
   dict *cfg;				// configuration from .conf
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
