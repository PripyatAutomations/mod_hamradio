#if	!defined(__RADIO_H)
#define	__RADIO_H

#include <gpiod.h>

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

#endif	// !defined(__RADIO_H)
