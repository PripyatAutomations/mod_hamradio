/*
 * mod_hamradio/radio.c
 *
 * Here we handle the radio control logics
 */
#include <switch.h>
#include <gpiod.h>
#include "mod_hamradio.h"

//////////////////////////
// radio_get_status_str //
//////////////////////////
// Reduce duplication of code...
static const char *radio_status_msgs[4] = { "OFF", "Idle", "Transmitting", NULL };
static const char *radio_get_status_str(const int radio) {
    Radio_t *r = NULL;

    if (radio < 0 || radio >= MAX_RADIOS) {
       err_invalid_radio(radio);
       return NULL;
    }
    r = &globals.Radios[radio];
    return radio_status_msgs[r->status];
}

RadioStatus_t radio_enable(const int radio) {
   Radio_t *r = NULL;

   if (radio < 0 || radio >= MAX_RADIOS) {
      err_invalid_radio(radio);
      return RADIO_ERROR;
   }

   r = &globals.Radios[radio];

   if (r->enabled && (r->status >= RADIO_IDLE)) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "[radio] radio%d is already enabled!\n", radio);
      return r->status;
   }

   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[radio] enabling radio%d as requested.\n", radio);
   // Enable the radio
   r->enabled = 1;
   // Power up the radio
   radio_set_state(radio, RADIO_IDLE);
   return RADIO_IDLE;
}

int radio_disable(const int radio) {
   Radio_t *r = NULL;

   if (radio < 0 || radio >= MAX_RADIOS) {
      err_invalid_radio(radio);
      return RADIO_ERROR;
   }

   r = &globals.Radios[radio];

   if (!r->enabled) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "[radio] radio%d is already disabled!\n", radio);
      return RADIO_OFF;
   }

   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[radio] disabling radio%d as requested.\n", radio);
   // Disable the radio
   r->enabled = 0;
   // Power down the radio
   radio_set_state(radio, RADIO_OFF);
   return RADIO_OFF;
}

// Set the radio combinead (power and ptt) state in one call
RadioStatus_t radio_set_state(const int radio, RadioStatus_t val) {
   Radio_t *r = NULL;
   RadioStatus_t old_status;

   // Negative values aren't allowed in the struct but can be returned in case of error
   if (val < 0) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[radio] radio_set_state(%d) called for radio%d - A negative value means an error was not correctly handled somewhere upstream... File a bug!\n", val, radio);
      return RADIO_ERROR;
   }

   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[radio] radio_set_state(%s) called for radio%d\n", radio_status_msgs[val], radio);
   // try to prevent invalid radios as this is used to index an array
   if (radio < 0 || radio >= MAX_RADIOS) {
      err_invalid_radio(radio);
      return RADIO_ERROR;
   }

   // pointer to the radio struct
   r = &globals.Radios[radio];

   if (r == NULL) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[radio] set_state(%s) called for radio%d - Cannot find radio data structure\n", radio_status_msgs[val], radio);
      return RADIO_ERROR;
   }

   // Save the old status, for our informational log message below
   old_status = r->status;

   // Set the new channel state
   r->status = val;

   // Are either gpio pin disconnected from libgpiod?
   if (r->gpio_ptt == NULL)
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[radio] set_state(%s) called but radio %d doesn't have PTT gpio plumbed. [ptr:%p]\n", radio_status_msgs[val], radio, r->gpio_ptt);

   if (r->gpio_power == NULL)
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[radio] set_state(%s) called but radio %d doesn't have POWER gpio plumbed. [ptr:%p]\n", radio_status_msgs[val], radio, r->gpio_power);

   if (r->gpio_ptt == NULL || r->gpio_power == NULL)
      return RADIO_ERROR;

   // What status has been requested?
   switch (val) {
     // Catch errors
     case RADIO_ERROR:
        err_invalid_radio(radio);
        break;
     case RADIO_DISABLED:
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "[radio] set_state() Ignoring state change for radio%d because it is disabled.\n", radio);
        break;
     case RADIO_OFF:
        // Clear PTT
        gpiod_line_set_value(r->gpio_ptt, 0);
        // Turn off IGN SENS or POWER RELAY
        gpiod_line_set_value(r->gpio_power, 0);
        break;
     case RADIO_IDLE:
     case RADIO_RX:		// This is essentially the same thing
        // Clear PTT
        gpiod_line_set_value(r->gpio_ptt, 0);
        // Ensure POWER is ON, if it wasn't previously
        gpiod_line_set_value(r->gpio_power, 1);
        break;
     case RADIO_TX:
        gpiod_line_set_value(r->gpio_ptt, 1);
        // XXX: Start timers here for TOT & periodic ident
        break;
   }

   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "[radio] radio%d STATUS change (%s) => (%s)\n", radio, radio_status_msgs[old_status], radio_get_status_str(radio));

   return r->status;
}

// Get the current combined (power and ptt) state of the radio
RadioStatus_t radio_get_state(const int radio) {
   if (radio < 0 || radio >= MAX_RADIOS) {
      err_invalid_radio(radio);
      return RADIO_ERROR;
   }

   if (globals.Radios[radio].enabled == 0)
      return RADIO_DISABLED;

   return (globals.Radios[radio].status);
}

void radio_ptt_on(switch_core_session_t *session) {
   switch_channel_t *channel;

   if (!session) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "No session\n");
      return;
   }
   channel = switch_core_session_get_channel(session);

   switch_assert(channel);

   // XXX: Find the radio attached to this channel...
   int radio = 0;

   // Refuse to TX on disabled radio
   if (!globals.Radios[radio].enabled) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Denying request to TX on radio%d in DISABLED state!\n", radio);
      return;
   }

   // Refuse to TX on radio that is turned off!
   if (radio_get_state(radio) == RADIO_OFF) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Ignoring request to TX on radio%d in POWERED OFF state!\n", radio);
      return;
   }

   radio_set_state(radio, RADIO_TX);
}

void radio_conference_ptt_on(switch_core_session_t *session) {
  // XXX: Enumerate the radios attached to the channel
  // XXX: Call radio_ptt_on for each
}

void radio_conference_ptt_off(switch_core_session_t *session) {
  // XXX: Enumerate the radios attached to the channel
  // XXX: Call radio_ptt_on for each
}

void radio_ptt_off(switch_core_session_t *session) {
   switch_channel_t *channel;

   if (!session) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "No session\n");
      return;
   }
   channel = switch_core_session_get_channel(session);

   switch_assert(channel);

   // XXX: Find the radio attached to the channel...
   int radio = 0;

   if (radio_get_state(radio) == RADIO_OFF)
      return;

   radio_set_state(radio, RADIO_IDLE);
}

void radio_power_on(switch_core_session_t *session) {
   switch_channel_t *channel;

   if (!session) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "No session\n");
      return;
   }
   channel = switch_core_session_get_channel(session);

   switch_assert(channel);

   // XXX: Find the radio attached to the channel...
   int radio = 0;

   if (!globals.Radios[radio].enabled) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Refusing to power on radio%d in DISABLED state. requested by app\n", radio);
      return;
   } else {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Powering ON radio%d by app request\n", radio);
      radio_set_state(radio, RADIO_IDLE);
   }
}

void radio_power_off(switch_core_session_t *session) {
   switch_channel_t *channel;

   if (!session) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "No session\n");
      return;
   }
   channel = switch_core_session_get_channel(session);

   switch_assert(channel);

   // XXX: Find the radio attached to the channel...
   int radio = 0;

   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Powering OFF radio%d by app request\n", radio);
   radio_set_state(radio, RADIO_OFF);
}

void radio_print_status(switch_stream_handle_t *stream, const int radio) {
   stream->write_function(stream, "radio%d: ", radio);

   switch(radio_get_state(radio)) {
      // Error conditions are handled here (except DISABLED, since it's not really an error)
      case RADIO_ERROR:
         if (radio >= MAX_RADIOS) {
            stream->write_function(stream, "invalid radio %d specified\n", radio);
            err_invalid_radio(radio);
         } else
            stream->write_function(stream, "ERROR unknown\n");
         break;
      case RADIO_DISABLED:
         stream->write_function(stream, "DISABLED\n");
         break;
      // Any normal state of the radio is handled here
      case RADIO_OFF:
      case RADIO_IDLE:
      case RADIO_RX:
      case RADIO_TX:
         stream->write_function(stream, "%s\n", radio_get_status_str(radio));
         break;
   }
}

int radio_dump_state_var(const int radio) {
   Radio_t *r;

   // try to prevent invalid radios as this is used to index an array
   if (radio < 0 || radio >= MAX_RADIOS) {
      err_invalid_radio(radio);
      return SWITCH_STATUS_FALSE;
   }

   // pointer to the radio struct
   r = &globals.Radios[radio];

   if (r == NULL) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[radio] radio_dump_state_var(%d) cannot find radio data structure\n", radio);
      return SWITCH_STATUS_FALSE;
   }

//   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "  => enabled: %d rx_mode: %d status: %s\n", r->enabled, r->RX_mode, radio_status_msgs[r->status]);
//   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "  => pin_power: %d pin_ptt: %d pin_squelch: %d\n", r->pin_power, r->pin_ptt, r->pin_squelch);
//   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "  => gpiod addr power: %p ptt: %p squelch: %p\n", r->gpio_power, r->gpio_ptt, r->gpio_squelch);

   return SWITCH_STATUS_SUCCESS;
}
