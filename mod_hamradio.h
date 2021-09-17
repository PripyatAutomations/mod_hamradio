////////////////////
// mod_hamradio.h //
////////////////////
#if	!defined(MOD_HAMRADIO_H)
#define	MOD_HAMRADIO_H
#include <gpiod.h>
#include <time.h>
#include <stdlib.h>
#include "config.h"
// Support for GPIO controlled (relays/optocouplers and COS/TOS inputs) radios
#include "radio_gpio.h"
// Support for rigctl controlled radios (NYI)
#include "radio_rigctl.h"

// This is the maximum number of radios that can be configured
// I'm not sure why you would have more than 8 on a pi, but it might work!
#define	MAX_RADIOS	8
#define	MAX_GPIO	128		// maximum GPIO pin # (this is intentionally high)
#define	HAMRADIO_CONF	"/etc/freeswitch/hamradio.conf" // configuration file

// This will be used to keep track of the state of a channel's voice activity
typedef enum {
   CHAN_QUIET = 0,
   CHAN_NOISE,
   CHAN_VOICE,
   CHAN_ERROR
} chan_vad_activity;

////////////////
// Radio Data //
////////////////
typedef enum RadioRXMode {
   SQUELCH_MANUAL = 0,		// No automatic TX, manual control via API
   SQUELCH_GPIO,			// TX when squelch GPIO is present
   SQUELCH_VOX			// Use Voice Activity Detection to decide
} RadioRXMode_t;

typedef enum RadioStatus { RADIO_ERROR = -2, RADIO_DISABLED = -1, RADIO_OFF = 0, RADIO_IDLE, RADIO_RX, RADIO_TX } RadioStatus_t;

struct Radio {
   ///////////////////
   // configuration //
   ///////////////////
   int		enabled;		// Is channel enabled?
   RadioRXMode_t RX_mode;		// How do we decide this radio is hearing something that should be relayed?
   int		timeout_talk;		// How long do we allow someone to talk before stopping TX?
   int 		ctcss_inband;		// Does radio pass CTCSS tones?
   int 		squelch_invert;		// Is squelch inpout inverted?

   // GPIO pins
   int		pin_power;		// Power or ignition sense relay output
   int		pin_ptt;		// Push to Talk output
   int		pin_squelch;		// Squelch input from radio (optional voltage divider or optocoupler)

   // mod_portaudio devices to provide the audio channel
   char	pa_indev[PATH_MAX];		// Input device
   char	pa_outdev[PATH_MAX];		// Output device

   ///////////////////
   // Run-time data //
   ///////////////////
   enum RadioStatus status;

   // libgpiod data
   struct gpiod_line *gpio_power;	// Power or ignition sense output
   struct gpiod_line *gpio_ptt;		// Push To Talk output
   struct gpiod_line *gpio_squelch;	// squelch (COS or TOS) output from radio

   ////////////////
   // Statistics //
   ////////////////
   // Running totals since restart
   time_t	total_rx, total_tx;
   time_t	last_tx, last_id, last_rx;
   // When was the current TX started?
   time_t	talk_start;
   // when we have timers, they belong here...
   struct {
       switch_timer_t *talk_timeout;
   } timers;
};
typedef struct Radio Radio_t;

// Global data
struct Globals {
   int alive;				// are we shutting down?
   int radios;
   struct Radio Radios[MAX_RADIOS];	// Radio structures
   switch_mutex_t *mutex;
   dict *cfg;				// configuration from .conf

   // libgpiod structures
   struct gpiod_chip *gpiochip;
};
typedef struct Globals Globals_t;

extern Globals_t globals;		// in mod_hamradio.c

////////////////
// Prototypes //
////////////////
extern RadioStatus_t radio_set_state(int radio, enum RadioStatus val);
extern RadioStatus_t radio_get_state(int radio);

// Reduce duplication - this prints the basic channel status
extern void radio_print_status(switch_stream_handle_t *stream, int radio);

// Configuration file
extern switch_status_t load_configuration(switch_bool_t reload);

// Enable/disable a radio
extern RadioStatus_t radio_enable(int radio);
extern RadioStatus_t radio_disable(int radio);

// Turn POWER on/off for a radio
extern void radio_power_on(switch_core_session_t *session);
extern void radio_power_off(switch_core_session_t *session);

// Control PTT (Push To Talk) on to start/stop transmitting
extern void radio_ptt_on(switch_core_session_t *session);
extern void radio_ptt_off(switch_core_session_t *sssion);
// And for all the radios in a conference?
extern void radio_conference_ptt_on(switch_core_session_t *sssion);
extern void radio_conference_ptt_off(switch_core_session_t *sssion);

// Status messages, etc
extern void radio_print_status(switch_stream_handle_t *stream, const int radio);
extern int radio_dump_state_var(const int radio);

extern time_t timestr_to_time(const char *str, const time_t def);
extern char *time_to_timestr(time_t itime);

///////////////////////////////////////
// And some inlines that belong here //
///////////////////////////////////////
// Is the radio enabled??
static inline int is_radio_enabled(int radio) {
    return globals.Radios[radio].enabled;
}

// User has asked us to operate on an invalid radio
static inline void err_invalid_radio(int radio) {
   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "* ERROR - radio%d requested, but system only supports %d radios!\n", radio, MAX_RADIOS);
}

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
