/*
 * mod_hamradio/radio.c
 *
 * Here we handle the radio control logics
 */
#include <switch.h>
#include "mod_hamradio.h"

//////////////////////////
// radio_get_status_str //
//////////////////////////
// Reduce duplication of code...
static const char *radio_status_msgs[6] = {
 // RADIO_OFF		RADIO_IDLE	RADIO_RX
 "OFF", 		"Idle", 	"Receiving",
 // RADIO_TX		RADIO_TX_DATA	N/A
 "Transmitting", 	"TX Data", 	NULL
};

static const char *radio_get_status_str(const int radio) {
    Radio_t *r = NULL;

    if (radio < 0 || radio >= globals.max_radios) {
       err_invalid_radio(radio);
       return NULL;
    }
    r = &Radios(radio);

    // Should NEVER be negative!
    if (r->status < 0) {
       switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
         "radio_get_status_str ERROR radio[%d]->status = %d which is invalid. Negative values are error returns and indicate a bug somewhere if saved!\n", radio, r->status);
       return NULL;
    }
    return radio_status_msgs[r->status];
}

RadioStatus_t radio_enable(const int radio) {
   Radio_t *r = NULL;

   if (radio < 0 || radio >= globals.max_radios) {
      err_invalid_radio(radio);
      return RADIO_ERROR;
   }

   r = &Radios(radio);

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

   if (radio < 0 || radio >= globals.max_radios) {
      err_invalid_radio(radio);
      return RADIO_ERROR;
   }

   r = &Radios(radio);

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

///////////////////////////////////////////////////////////
// Main function for controlling radio state             //
// - Use this interface to ensure TOT, idents, etc work! //
// All of the radio_*_[on|off] functions call into here. //
///////////////////////////////////////////////////////////
RadioStatus_t radio_set_state(const int radio, RadioStatus_t val) {
   Radio_t *r = NULL;
   RadioStatus_t old_status;
   switch_time_t qso_length = 0, now = switch_micro_time_now();
   switch_status_t rv = SWITCH_STATUS_SUCCESS;

   // Negative values aren't allowed in the struct but can be returned in case of error
   if (val < 0) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[radio] radio_set_state(%d) called for radio%d - A negative value means an error was not correctly handled somewhere upstream... File a bug!\n", val, radio);
      return RADIO_ERROR;
   }

//   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "[radio] radio_set_state(%s) called for radio%d\n", radio_status_msgs[val], radio);

   if (radio < 0 || radio >= globals.max_radios) {
      err_invalid_radio(radio);
      return RADIO_ERROR;
   }

   // pointer to the radio struct
   r = &Radios(radio);

   if (r == NULL) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[radio] set_state(%s) called for radio%d - Cannot find radio data structure\n", radio_status_msgs[val], radio);
      return RADIO_ERROR;
   }

   // Save the old status, for our informational log message below
   old_status = r->status;

   // Set the new channel state
   r->status = val;

   // If we're using GPIO for a pin (pin_* is set) then make sure it's connected
   if (r->pin_ptt >= 0 && r->gpio_ptt == NULL) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[radio] set_state(%s) called but radio %d doesn't have PTT gpio plumbed. [ptr:%p]\n", radio_status_msgs[val], radio, r->gpio_ptt);
      rv = SWITCH_STATUS_FALSE;
   }

   if (r->pin_power >= 0 && r->gpio_power == NULL) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[radio] set_state(%s) called but radio %d doesn't have POWER gpio plumbed. [ptr:%p]\n", radio_status_msgs[val], radio, r->gpio_power);
      rv = SWITCH_STATUS_FALSE;
   }

   // Are any configured controls missing?
   if (rv == SWITCH_STATUS_FALSE)
      return RADIO_ERROR;

   // What status has been requested?
   switch (val) {
     //////////////////////
     // Error conditions //
     //////////////////////
     case RADIO_BLOCKED:
        // This is an error - blocked state is decided by the value of (r->talk_start + r->timer_talk) and whether it has passed or not...
        break;
     case RADIO_ERROR:
        err_invalid_radio(radio);
        break;
     case RADIO_DISABLED:
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "[radio] set_state() Ignoring state change for radio%d because it is disabled.\n", radio);
        break;
     ////////////////////////
     // Valid radio states //
     ////////////////////////
     case RADIO_OFF:
        // Clear PTT
        if (r->pin_ptt)
           radio_gpio_ptt_off(radio);

        // Turn off IGN SENS or POWER RELAY
        if (r->pin_power)
           radio_gpio_power_off(radio);
        break;
     case RADIO_IDLE:
        if (r->status == RADIO_TX) {
           // XXX: Check when last IDed and pause then send a tailing identification
           if (r->talk_start > 0) {
              // record statistics about the QSO length
              qso_length = now - r->talk_start;
              switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "[radio] radio%d was transmitting for %s...\n", radio, time_to_timestr(qso_length));
              // save the total time transmitted
              r->total_tx += qso_length;
           }
           // save last time transmitted
           r->last_tx = now;
        }

        // If we were receiving, record statistics
        if (r->status == RADIO_RX) {
           if (r->listen_start > 0) {
              qso_length = now - r->listen_start;
              switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "[radio] radio%d was receiving for %s...\n", radio, time_to_timestr(qso_length));
              r->total_rx += qso_length;
           }
           // save last time received
           r->last_rx = now;
        }

        // Clear PTT before powering on
        if (r->pin_ptt)
           radio_gpio_ptt_off(radio);

        // Ensure POWER is ON, if it wasn't previously
        if (r->pin_power)
           radio_gpio_power_on(radio);

        // Clear talk time for TOT
        r->talk_start = 0;
        break;
     case RADIO_RX:
        // Clear PTT before powering on
        if (r->pin_ptt)
           radio_gpio_ptt_off(radio);

        // Ensure POWER is ON, if it wasn't previously
        if (r->pin_power)
           radio_gpio_power_on(radio);

        r->listen_start = now;
        break;
     case RADIO_TX:
     case RADIO_TX_DATA:
        // Is there a penalty pending on this radio?
        if (r->penalty > 0) {
           // Add additional penalty XXX: Should this be here?
           r->penalty = (r->penalty < r->timeout_holdoff ? r->timeout_holdoff : (r->penalty + r->timeout_holdoff));
           return RADIO_BLOCKED;
        }

        // Start timers here for TOT, but don't restart it if we didn't stop TXing...
        if (r->talk_start == 0)
           r->talk_start = now;

        // XXX: If a CAT PTT is available, raise it
        // radio_cat_ptt_on(radio);

        // if a PTT GPIO is configured, raise it now
        if (r->pin_ptt)
           radio_gpio_ptt_on(radio);

        break;
   }

   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "[radio] radio%d STATUS change (%s) => (%s)\n", radio, radio_status_msgs[old_status], radio_get_status_str(radio));
   return r->status;
}

// Get the current combined (power and ptt) state of the radio
RadioStatus_t radio_get_state(const int radio) {
   if (radio < 0 || radio >= globals.max_radios) {
      err_invalid_radio(radio);
      return RADIO_ERROR;
   }

   if (Radios(radio).enabled == 0)
      return RADIO_DISABLED;

   return (Radios(radio).status);
}

void radio_ptt_on(const int radio) {
   switch_channel_t *channel;

//   if (!session) {
//      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "No session\n");
//      return;
//   }
//   channel = switch_core_session_get_channel(session);
//   switch_assert(channel);

   // Refuse to TX on disabled radio
   if (!Radios(radio).enabled) {
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

void radio_conf_ptt_on(const int radio) {
  // XXX: Enumerate the radios attached to the channel
  // XXX: Call radio_ptt_on for each
}

void radio_conf_ptt_off(const int radio) {
  // XXX: Enumerate the radios attached to the channel
  // XXX: Call radio_ptt_on for each
}

void radio_ptt_off(const int radio) {
   switch_channel_t *channel;

//   if (!session) {
//      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "No session\n");
//      return;
//   }
//   channel = switch_core_session_get_channel(session);
//   switch_assert(channel);

   if (radio_get_state(radio) == RADIO_OFF)
      return;

   radio_set_state(radio, RADIO_IDLE);
}

void radio_power_on(const int radio) {
   switch_channel_t *channel;

//   if (!session) {
//      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "No session\n");
//      return;
//   }
//   channel = switch_core_session_get_channel(session);
//   switch_assert(channel);

   if (!Radios(radio).enabled) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Refusing to power on radio%d in DISABLED state. requested by app\n", radio);
      return;
   } else {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Powering ON radio%d by app request\n", radio);
      radio_set_state(radio, RADIO_IDLE);
   }
}

void radio_power_off(const int radio) {
   switch_channel_t *channel;

//   if (!session) {
//      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "No session\n");
//      return;
//   }
//   channel = switch_core_session_get_channel(session);

   switch_assert(channel);
   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Powering OFF radio%d by app request\n", radio);
   radio_set_state(radio, RADIO_OFF);
}

void radio_print_status(switch_stream_handle_t *stream, const int radio) {
   Radio_t *r;
   
   if (radio < 0 || radio > globals.max_radios) {
      stream->write_function(stream, "invalid radio %d specified\n", radio);
      return;
   }

   r = &Radios(radio);
   stream->write_function(stream, "radio%d: ", radio);

   switch(radio_get_state(radio)) {
      // Error conditions are handled here (except DISABLED, since it's not really an error)
      case RADIO_ERROR:
         if (radio >= globals.max_radios) {
            stream->write_function(stream, "invalid radio %d specified\n", radio);
            err_invalid_radio(radio);
         } else
            stream->write_function(stream, "ERROR unknown\n");
         break;
      case RADIO_DISABLED:
         stream->write_function(stream, "DISABLED\n");
         break;
      // Any normal state of the radio is handled here
      case RADIO_BLOCKED:
         stream->write_function(stream, "Blocked due to TOT exceeded. Penalty remaining: %s\n", time_to_timestr(r->penalty));
      case RADIO_OFF:
      case RADIO_IDLE:
      case RADIO_RX:
      case RADIO_TX:
      case RADIO_TX_DATA:
         stream->write_function(stream, "%s\n", radio_get_status_str(radio));
         break;
   }
}

// XXX: This needs to be improved so that it uses stream->write_function(stream, ...) instead of switch_log_printf *IF* coming from api
int radio_dump_state_var(const int radio, switch_bool_t detailed) {
   Radio_t *r;
   switch_time_t now = switch_micro_time_now();

   // try to prevent invalid radios as this is used to index an array
   if (radio < 0 || radio >= globals.max_radios) {
      err_invalid_radio(radio);
      return SWITCH_STATUS_FALSE;
   }

   // pointer to the radio struct
   r = &Radios(radio);

   if (r == NULL) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[radio] radio_dump_state_var(%d) cannot find radio data structure\n", radio);
      return SWITCH_STATUS_FALSE;
   }

   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO,    "* radio%d: %s\n", radio, r->description);
     /*(r->description ? r->description : "")); */
   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO,    "    enabled: %s\t\t\tstatus: %s\n",
          (r->enabled ? "true" : "false"), radio_get_status_str(radio));

   if (detailed) {
      char tmp1[30], tmp2[30];	// date string buffers
      const char date_fmt[19] = "%Y-%d-%m %H:%M:%S";

      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "   sq. mode: %d %s\t\tinband ctcss: %s\n", r->RX_mode,
          (r->squelch_invert ? "(invert)" : ""), (r->ctcss_inband ? "true" : "false"));
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "   total_rx: %s\t\t\ttotal_tx: %s\n", time_to_timestr(r->total_rx), time_to_timestr(r->total_tx));

      // Show time stamps with date for last TX/RX times
      memset(tmp1, 0, sizeof(tmp1));
      memset(tmp2, 0, sizeof(tmp2));
      if (r->last_rx > 0)
         strftime(tmp1, sizeof(tmp1), date_fmt, localtime(&r->last_rx));
      else
         sprintf(tmp1, "Never");

      if (r->last_tx > 0)
         strftime(tmp2, sizeof(tmp2), date_fmt, localtime(&r->last_tx));
      else
         sprintf(tmp2, "Never");
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "    last_rx: %s\t\tlast_tx: %s\n", tmp1, tmp2);
 
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "    curr_rx: %s\t\tcurr_tx: %s\n",
          ((r->listen_start > 0) ? time_to_timestr(now - r->listen_start) : "off"),
          ((r->talk_start > 0) ? time_to_timestr(now - r->talk_start) : "off"));
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "        tot: %s\tholdoff:%s\tpenalty:%s\n",
          time_to_timestr(r->timeout_talk), time_to_timestr(r->timeout_holdoff), time_to_timestr(r->penalty));
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "   pa_indev: %s\t\t\tpa_outdev: %s\n", r->pa_indev, r->pa_outdev);
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, " GPIO:  PTT: %d\tPower: %d\tSquelch: %d\n", r->pin_ptt, r->pin_power, r->pin_squelch);
   }
   return SWITCH_STATUS_SUCCESS;
}
