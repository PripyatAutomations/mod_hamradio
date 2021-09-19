#if	!defined(__RADIO_H)
#define	__RADIO_H
#if	!defined(NO_LIBGPIOD)
#include <gpiod.h>
#endif

////////////////
// Radio Data //
////////////////
typedef enum RadioRXMode {
   SQUELCH_MANUAL = 0,		// Manual control via API only
   SQUELCH_GPIO,		// Squelch GPIO is present
   SQUELCH_VOX			// Use Voice Activity Detection code
} RadioRXMode_t;

//
// Negative returns are an error indicator and must NEVER be stored.
// errors:
// 	RADIO_BLOCKED		TOT penalty is in effect.
//	RADIO_ERROR		Invalid or misconfigured radio for request
//	RADIO_DISABLED		Radio is not in enabled state (hamradio enable radio# to fix)
// Valid states:
//	RADIO_OFF		Radio is enabled but powered off
//	RADIO_IDLE		Radio is powered on and listening
//	RADIO_RX		Radio is actively receiving
//	RADIO_TX		Radio is actively transmitting
//	RADIO_TX_DATA		Radio is actively transmitting data burst
typedef enum RadioStatus {
   RADIO_BLOCKED = -3, RADIO_ERROR = -2, RADIO_DISABLED = -1,
   RADIO_OFF = 0, RADIO_IDLE, RADIO_RX, RADIO_TX, RADIO_TX_DATA
} RadioStatus_t;

struct Radio {
   ///////////////////
   // configuration //
   ///////////////////
   int		enabled;		// Is channel enabled?
   char		description[160];	// Describe the interface
   RadioRXMode_t RX_mode;		// How do we decide this radio is hearing something that should be relayed?
   time_t	timeout_talk;		// How long do we allow someone to talk before stopping TX?
   time_t	timeout_holdoff;	// How long do we punish triggering the TOT?
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

#if	!defined(NO_LIBGPIOD)
   // libgpiod data
   struct gpiod_line *gpio_power;	// Power or ignition sense output
   struct gpiod_line *gpio_ptt;		// Push To Talk output
   struct gpiod_line *gpio_squelch;	// squelch (COS or TOS) output from radio
#endif

   ////////////////
   // Statistics //
   ////////////////
   // Running totals since restart
   time_t	total_rx, total_tx;
   time_t	last_tx, last_id, last_rx;
   // When was the current TX/RX started?
   time_t	talk_start, listen_start;
   // Remaining holdoff (penalty) time for exceeding TOT, before we will allow a TX
   time_t	penalty;
};

////////////////
// Prototypes //
////////////////
typedef struct Radio Radio_t;

extern void radio_print_status(switch_stream_handle_t *stream, int radio);
extern RadioStatus_t radio_set_state(int radio, enum RadioStatus val);
extern RadioStatus_t radio_get_state(int radio);
// Enable/disable a radio
extern RadioStatus_t radio_enable(int radio);
extern RadioStatus_t radio_disable(int radio);

// Turn POWER on/off for a radio
extern void radio_power_on(int radio);
extern void radio_power_off(int radio);

// Control PTT (Push To Talk) on to start/stop transmitting
extern void radio_ptt_on(int radio);
extern void radio_ptt_off(int radio);
// And for all the radios in a conference?
extern void radio_conf_ptt_on(int radio);
extern void radio_conf_ptt_off(int radio);

// Status messages, etc
extern int radio_dump_state_var(const int radio, switch_bool_t detailed);

///////////////////////////////////////
// And some inlines that belong here //
///////////////////////////////////////
// Is the radio enabled??
static inline int is_radio_enabled(int radio) {
//    return globals.Radios[radio].enabled;
return 1;
}

// User has asked us to operate on an invalid radio
static inline void err_invalid_radio(int radio) {
   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_CRIT, "* ERROR - radio%d requested, but system only supports %d radios!\n", radio, dconf_get_int("max_radios", 8));
}

#endif	// !defined(__RADIO_H)
