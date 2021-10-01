/*
 * FreeSWITCH core processing (main thread)
 *
 * Here we handle tasks that must be completed on a periodic basis
 * such as polling PTTs and squelches. Try to keep code here lean.
 */
#include "mod_hamradio.h"

///////////////////////////////////////
// Here is where out main logic runs //
///////////////////////////////////////
// We poll the radio stack periodically (poll_interval setting)
// and do things based on what we find.
// We need to be reasonably fast here and try to keep CPU usage
// responsible...
SWITCH_MODULE_RUNTIME_FUNCTION(mod_hamradio_runtime) {
   time_t last_tick;

   // Wait for the main process to be ready
   while (!globals.alive)
      sleep(1);

   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "hamradio interface control thread waking up!\n");

   // As long as we aren't shutting down, scan the radios
   while (globals.alive) {
//      switch_time_t now = switch_micro_time_now();
      switch_time_t now = time(NULL);

      for (int radio = 0; radio < globals.max_radios; radio++) {
         Radio_t *r = &Radios(radio);
         int sqval = 0;
         switch_bool_t squelch_state = false;

         // Another second has passed, reduce penalty time on this radio
         if (r->penalty > 0) {
            // This should only happen once per second, make sure that's the case...
            if (last_tick && (now - last_tick >= 1))
               r->penalty -= (now - last_tick);

            last_tick = now;

            // penalty expired?
            if (r->penalty == 0) {
               switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "radio%d penalty cleared\n", radio);

               // Optionally Play a status tone to indicate penalty time over
               radio_send_tones(radio, "penalty_clear");
            }
         }

         // Check the GPIO squelch input line here
         if (r->gpio_squelch != NULL) {
            switch_bool_t gpio_squelch_state = false;
            sqval = radio_gpio_read_squelch(radio);
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "radio%d read squelch val = %d\n", radio, sqval);

            // Handle inverting the signal, if needed for this radio
            if (r->squelch_invert) {
               // XXX
            }

            // There is received activity
            if (gpio_squelch_state == true) {
               // Are we in automatic control mode? If not, ignore the input
               if ((r->RX_mode == SQUELCH_GPIO)) {
                  // XXX: Find all conferences this radio is in and raise it's RXing flag
                  // radio_confs_find_byradio(radio)
                  squelch_state = true;
               }
            }
         }

         // If we are in VAD mod, try to determine if this radio has activity
         if (r->RX_mode == SQUELCH_VOX) {
            // XXX: Check squelch PTT status
            // if (vad_is_voice(radio)) {
            //    squelch_state = true;
         }
 
         // If any of the squelch inputs were configured and came up, set RX flags
         if (squelch_state == true) {
            // Set receiving state on
            radio_set_state(radio, RADIO_RX);
            // Store last received time
            r->last_rx = now;
         } else if ((r->status == RADIO_RX) && (squelch_state == false)) {
            radio_set_state(radio, RADIO_IDLE);
            r->last_rx = now;
         }

         // Handle tasks specific to the state of the selected radio (RX, TX, TXDATA)
         if (r->status == RADIO_RX) {
            // Here we should do receive radio stuff, like establish audio if not already done
         } else if (r->status == RADIO_TX) {
            // is a timeout timer set on this channel?
            if (r->timeout_talk > 0) {
               // Has the timer expired?
               if (now >= (r->talk_start + r->timeout_talk)) {
                  switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "radio%d ending transmission (TOT expired: %lu, adding %lu penalty)\n", radio, r->timeout_talk, r->timeout_holdoff);

                  // Apply a delay before allowing TX again
                  r->penalty += r->timeout_holdoff;

                  // Turn the PTT off
                  radio_ptt_off(radio);
               }

               // store last TX as now
               r->last_tx = now;
            }
         } else if (r->status == RADIO_TX_DATA) {
               // XXX: Implement duty cycle management!
               // XXX: Handle modem tasks here
               // store last TX time
               r->last_tx = now;
         }

         // XXX: Check ident timeouts
      }

      // This allows reducing CPU load in the polling code, if needed
      if (globals.poll_interval > 0)
         usleep(globals.poll_interval);
      switch_cond_next();
   }
   return SWITCH_STATUS_TERM;
}
