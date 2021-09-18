/*
 * mod_hamradio.c: A simple Raspberry Pi GPIO control for GPIOs
 *
 *	We use this to control PTT and POWERON relays for our radios.
 *
 * We hijack some of VAD (voice activity detection) code from elsewhere
 * to allow us to only raise PTT when there is meaningful voice traffic
 * to send.
 *
 * Eventually we hope to add more, such as a means of using COS/TOS
 * (carrier and tone operated squelch) modes.
 *
 * Created by rustytaco. No warranties. If it breaks, you get to keep the pieces.
 *
 * Please ensure you comply with applicable laws and regulations in your country.
 *
 * No support provided. Use at your own risk. Good luck!
 */
#include <switch.h>
#include "mod_hamradio.h"
Globals_t globals;

SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_hamradio_shutdown);
SWITCH_MODULE_RUNTIME_FUNCTION(mod_hamradio_runtime);
SWITCH_MODULE_LOAD_FUNCTION(mod_hamradio_load);
SWITCH_MODULE_DEFINITION(mod_hamradio, mod_hamradio_load, mod_hamradio_shutdown, mod_hamradio_runtime);

// Wrap some of our radio.c stuff for presentation towards the user
SWITCH_STANDARD_APP(app_radio_ptt_on) {
   int radio = 0;
   radio_ptt_on(radio);
}

SWITCH_STANDARD_APP(app_radio_conference_ptt_on) {
   int radio = 0;
   radio_conf_ptt_on(radio);
}

SWITCH_STANDARD_APP(app_radio_conference_ptt_off) {
   int radio = 0;
   radio_conf_ptt_off(radio);
}

SWITCH_STANDARD_APP(app_radio_ptt_off) {
   int radio = 0;
   radio_ptt_off(radio);
}

SWITCH_STANDARD_APP(app_radio_power_on) {
   int radio = 0;
   radio_power_on(radio);
}

SWITCH_STANDARD_APP(app_radio_power_off) {
   int radio = 0;
   radio_power_off(radio);
}

SWITCH_STANDARD_APP(app_radio_enable) {
    // XXX: Figure out which radio need's enabled
    int radio = 0;
    radio_enable(radio);
}

SWITCH_STANDARD_APP(app_radio_disable) {
   int radio = 0;
   radio_disable(radio);
}

// XXX: Here we need to figure out what radios are active in a conference and haven't IDed in awhile...
SWITCH_STANDARD_APP(app_radio_morse_id) {
   // XXX: Scan all radios and determine if they have been used since last ident
   // XXX: Send Morse code ID to chosen radio channels
}

//////////////////////////////////////////////////////////////////////
// This is our cli interface. Try to make it simple and consistent! //
//////////////////////////////////////////////////////////////////////
SWITCH_STANDARD_API(hamradio_function) {
   int argc, val = 0;
   char *mycmd = NULL, *argv[3] = { 0 };
   switch_status_t status = SWITCH_STATUS_SUCCESS;

   const char *usage = "USAGE:\n"
                       "   hamradio help\n"
                       "   hamradio power [radio] <on|off>\n"
                       "   hamradio ptt [radio] <on|off>\n"
                       "   hamradio reload\n"
                       "   hamradio status <radio>\n"
                       "   hamradio disable [radio]\n"
                       "   hamradio enable [radio]\n"
                       "   hamradio id <radio>\n";
   const char *power_usage = "USAGE:\n"
                       "   hamradio power\n"
                       "     Get all radios POWER status\n"
                       "   hamradio power [radio]\n"
                       "     Get radio [radio] POWER status\n"
                       "   hamradio power [radio] [on|off]\n"
                       "     Set radio [radio] POWER on or off\n";
   const char *ptt_usage = "USAGE:\n"
                       "   hamradio ptt\n"
                       "     Get all radios PTT status\n"
                       "   hamradio ptt [radio]\n"
                       "     Get radio [radio] PTT status\n"
                       "   hamradio ptt [radio] [on|off]\n"
                       "     Set radio [radio] PTT on or off\n";
   const char *id_usage = "USAGE:\n"
                       "   hamradio id <radio>\n"
                       "     Send Morse code IDentification on chosen radio or all radios, if not specified\n";

   if (zstr(cmd)) {
      stream->write_function(stream, "%s", usage);
      goto done;
   }
	 
   if (!(mycmd = strdup(cmd))) {
      status = SWITCH_STATUS_MEMERR;
      goto done;
   }

   if (!(argc = switch_separate_string(mycmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])))) || !argv[0]) {
      stream->write_function(stream, "%s", usage);
      goto done;
   }

   if (!strcasecmp(argv[0], "disable")) {
      if (argc < 2) {
         stream->write_function(stream, "USAGE:\n   disable [chan]\t- Disable radio channel [radio]\n");
         goto done;
      }

      int radio = atoi(argv[1]);
      if (radio < 0 || radio >= MAX_RADIOS) {
         err_invalid_radio(radio);
         status = SWITCH_STATUS_FALSE;
         goto done;
      }
      radio_disable(radio);
   } else if (!strcasecmp(argv[0], "enable")) {
      if (argc < 2) {
         stream->write_function(stream, "USAGE:\n   enable [chan]\t- Enable radio channel [radio]\n");
         goto done;
      }

      int radio = atoi(argv[1]);
      if (radio < 0 || radio >= MAX_RADIOS) {
         err_invalid_radio(radio);
         status = SWITCH_STATUS_FALSE;
         goto done;
      }
      radio_enable(radio);
   } else if (!strcasecmp(argv[0], "id")) {
      // Has the user specified a channel?
      if (argc == 2) {
         // XXX: Send Morse code ID to JUST selected channel
      } else if (argc == 1) {
         // XXX: Send Morse code ID to *ALL* enabled channels
      }
   } else if (!strcasecmp(argv[0], "power")) {
      if (argc == 1) {
         stream->write_function(stream, "POWER STATUS for ALL radios:\n");

         for (int radio = 0; radio < globals.max_radios; radio++) {
            stream->write_function(stream, "radio%d: power ", radio);

            if (globals.Radios[radio].status == RADIO_OFF)
               stream->write_function(stream, "off\n");
            else
	       stream->write_function(stream, "on\n");
         }
	 goto done;
      } else if (argc == 2) {
         const int radio = atoi(argv[1]);

         if (radio < 0 || radio >= MAX_RADIOS) {
            err_invalid_radio(radio);
            status = SWITCH_STATUS_FALSE;
            goto done;
         }

	 stream->write_function(stream, "radio%d: power ", radio);

	 if (globals.Radios[radio].status == RADIO_OFF)
	    stream->write_function(stream, "off\n");
         else
            stream->write_function(stream, "on\n");
      } else if (argc == 3) {
         const int radio = atoi(argv[1]);

         if (radio < 0 || radio >= MAX_RADIOS) {
            err_invalid_radio(radio);
            status = SWITCH_STATUS_FALSE;
            goto done;
         }

         if (!globals.Radios[radio].enabled) {
	    stream->write_function(stream, "Ignoring POWER ON for radio%d because it is in DISABLED state\n", radio);
	    status = SWITCH_STATUS_FALSE;
	    goto done;
	 }

         if ((val = str_to_intbool(argv[2])) == 1)
            radio_set_state(radio, RADIO_IDLE);
         else
            radio_set_state(radio, RADIO_OFF);

         stream->write_function(stream, "POWER for radio%d SET to %s\n", radio, (val ? "ON" : "OFF"));
      }
      goto done;
   } else if (!strcasecmp(argv[0], "ptt")) {
      if (argc == 1) {
         stream->write_function(stream, "PTT status for ALL radios:\n");

         for (int radio = 0; radio < globals.max_radios; radio++) {
            stream->write_function(stream, "radio%d: ");

	    if (radio_get_state(radio) == RADIO_TX)
               stream->write_function(stream, "transmitting\n");
            else
	       stream->write_function(stream, "off\n");
         }
         goto done;
      } else if (argc == 2) {
         const int radio = atoi(argv[1]);

         if (radio < 0 || radio >= MAX_RADIOS) {
            err_invalid_radio(radio);
            status = SWITCH_STATUS_FALSE;
            goto done;
         }

	 stream->write_function(stream, "radio%d: ", radio);

	 if (radio_get_state(radio) == RADIO_TX)
	    stream->write_function(stream, "transmitting\n");
	 else
	    stream->write_function(stream, "idle\n");
      } else if (argc == 3) {
         int radio = atoi(argv[1]);

	 if (radio < 0 || radio >= MAX_RADIOS) {
	    err_invalid_radio(radio);
	    status = SWITCH_STATUS_FALSE;
	    goto done;
	 }

	 if (!globals.Radios[radio].enabled) {
	    stream->write_function(stream, "Denying PTT request (via cli) for radio%d because it is in DISABLED state\n", radio);
	    status = SWITCH_STATUS_FALSE;
	    goto done;
	 }

         if (radio_get_state(radio) == RADIO_OFF) {
	    stream->write_function(stream, "Ignoring PTT request (via cli) for radio%d because it is in DISABLED state\n", radio);
	    status = SWITCH_STATUS_FALSE;
	    goto done;
         }

	 if ((val = str_to_intbool(argv[2])) == 1)
	    radio_set_state(radio, RADIO_TX);
	 else
	    radio_set_state(radio, RADIO_IDLE);

         stream->write_function(stream, "PTT for radio %d SET to %s\n", radio, (val ? "ON" : "OFF"));
      }
      goto done;
   } else if (!strcasecmp(argv[0], "reload")) {
      load_configuration(1);
   } else if (!strcasecmp(argv[0], "status")) {
      int active_radios = 0;
      
      if (argc == 1) {
         stream->write_function(stream, "*** Status for ALL radios ***\n");
         for (int i = 0; i < globals.max_radios; i++) {
            enum RadioStatus rs = radio_get_state(i);

	    if (rs > RADIO_OFF)
	       active_radios++;

            radio_dump_state_var(i, false);
         }
         stream->write_function(stream, "*** (%d/%d units active) ***\n", active_radios, globals.max_radios);
      } else if (argc == 2) {
         int radio = atoi(argv[1]);

	 if (radio < 0 || radio >= MAX_RADIOS) {
	    err_invalid_radio(radio);
	    status = SWITCH_STATUS_FALSE;
	    goto done;
	 }

         radio_dump_state_var(radio, true);
      } else {
         stream->write_function(stream, "USAGE:\nhamradio status <radio>\n");
         goto done;
      }
   }

// free up any allocated memories, etc here before returning.
done:
   switch_safe_free(mycmd);
   return status;
}

////////////////////////
// Configuration Load //
////////////////////////
switch_status_t load_configuration(switch_bool_t reload) {
   switch_status_t status = SWITCH_STATUS_FALSE;

   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[mod_hamradio] %sloading configuration from hamradio.conf\n", (reload ? "re" : ""));

// XXX: get this shite sorted out so we can reload safely
//   if (globals.mutex == NULL)
//      switch_mutex_init(globals.mutex, SWITCH_THREAD_MUTEX_UNNESTED, /* XXX Figure out pool */
//   switch_mutex_lock(globals.mutex);

   if (reload == true) {
      radio_gpio_fini();

      // is dictionary existing already? free it if so
      if (globals.cfg != NULL) {
         dict_free(globals.cfg);
         globals.cfg = NULL;
      }

   }

   // Zero out the configuration structure, such as radio data
   memset(&globals, 0, sizeof(globals));

   // load the dictionary configuration
   if (!(globals.cfg = dconf_load(HAMRADIO_CONF))) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[mod_hamradio] %sloading configuration from hamradio.conf failed. Please examine the DEBUG level log output from mod_hamradio to see why!\n", (reload ? "re" : ""));
      return SWITCH_STATUS_FALSE;
   }

   // Initialize GPIO chip(s)
   radio_gpiochip_init(dconf_get_str("gpiochip", "gpiochip0"));

   // step through all the configured radios and initialize their GPIO lines
   for (int radio = 0; radio < globals.max_radios; radio++) {
      Radio_t *r = &globals.Radios[radio];

      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[mod_hamradio] Bringing up radio%d\n", radio);

      // initialize it's GPIO interfaces, if any
      radio_gpio_init(radio);

      // Show some userful information in the log
      radio_dump_state_var(radio, true);

      // Power it up and make it available for use, if enabled
      if (r->enabled)
         radio_enable(radio);
   }

//   switch_mutex_unlock(globals.mutex);
   return status;
}

static void event_handler(switch_event_t *event) {
   // Module reload
   if (event->event_id == SWITCH_EVENT_RELOADXML) {
      load_configuration(true);
   }
}

static void channel_cb(switch_core_session_t *session, switch_channel_callstate_t callstate, switch_device_record_t *drec) {
   switch_channel_t *channel = switch_core_session_get_channel(session);

   switch_log_printf(SWITCH_CHANNEL_CHANNEL_LOG(channel), SWITCH_LOG_CRIT,
                     "%s device: %s\nState: %s Dev State: %s/%s Total:%u Offhook:%u Active:%u Held:%u Hungup:%u Dur: %u %s\n",
                     switch_channel_get_name(channel), drec->device_id, switch_channel_callstate2str(callstate),
                     switch_channel_device_state2str(drec->last_state), switch_channel_device_state2str(drec->state),
                     drec->stats.total, drec->stats.offhook, drec->stats.active, drec->stats.held, drec->stats.hup,
                     drec->active_stop ? (uint32_t)(drec->active_stop - drec->active_start) / 1000 : 0,
                     switch_channel_test_flag(channel, CF_FINAL_DEVICE_LEG) ? "FINAL LEG" : "");
}

SWITCH_MODULE_LOAD_FUNCTION(mod_hamradio_load) {
   switch_api_interface_t *api_interface;
   switch_application_interface_t *app_interface;
   *module_interface = switch_loadable_module_create_module_interface(pool, modname);
   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "mod_hamradio loaded. please be sure your usage is compliant with regulations!\n");

   // Load config and handle reloads
   load_configuration(0);
   if ((switch_event_bind(modname, SWITCH_EVENT_RELOADXML, NULL, event_handler, NULL) != SWITCH_STATUS_SUCCESS)) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Couldn't bind reloadxml handler!");
   }

   // Define our CLI interface
   SWITCH_ADD_API(api_interface, "hamradio", "hamradio channel controls", hamradio_function, "shows status");	
   switch_console_set_complete("add hamradio help");
   switch_console_set_complete("add hamradio status");
   switch_console_set_complete("add hamradio disable");
   switch_console_set_complete("add hamradio enable");
   switch_console_set_complete("add hamradio power");
   switch_console_set_complete("add hamradio ptt");
   switch_console_set_complete("add hamradio reload");

   // Define our app (dialplan) interface
   SWITCH_ADD_APP(app_interface, "radio_disable", "DISable a radio channel", "", app_radio_disable, "", SAF_NONE);
   SWITCH_ADD_APP(app_interface, "radio_enable", "ENable a radio channel", "", app_radio_enable, "", SAF_NONE);
   SWITCH_ADD_APP(app_interface, "radio_morse_id", "Send Morse code ID via radio channel", "", app_radio_morse_id, "", SAF_NONE);
   SWITCH_ADD_APP(app_interface, "radio_power_on", "Turn POWER relay ON for radio", "", app_radio_power_on, "", SAF_NONE);
   SWITCH_ADD_APP(app_interface, "radio_power_off", "Turn POWER relay OFF for radio", "", app_radio_power_off, "", SAF_NONE);
   SWITCH_ADD_APP(app_interface, "radio_ptt_on", "Turn Push To Talk (PTT) relay ON", "", app_radio_ptt_on, "", SAF_NONE);
   SWITCH_ADD_APP(app_interface, "radio_ptt_off", "Turn Push To Talk (PTT) relay OFF", "", app_radio_ptt_off, "", SAF_NONE);
   SWITCH_ADD_APP(app_interface, "radio_conference_ptt_on", "Turn PTT on for all radios in conference except active RX (Repeater mode)", "", app_radio_conference_ptt_on, "", SAF_NONE);
   SWITCH_ADD_APP(app_interface, "radio_conference_ptt_off", "Turn PTT off for all radios in conference except active RX (Repeater mode)", "", app_radio_conference_ptt_off, "", SAF_NONE);

 
   // Hook a channel callback so we can see channel events
   switch_channel_bind_device_state_handler(channel_cb, NULL);

   // We're running, let our child thread do it's thing!
   globals.alive = 1;

   /* indicate that the module should continue to be loaded */
   return SWITCH_STATUS_SUCCESS;
}

/* Called when the system shuts down:  Macro expands to: switch_status_t mod_hamradio_shutdown() */
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_hamradio_shutdown) {
   // Signal our thread that it should die...
   globals.alive = 0;
   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "shutting down radio interfaces due to freeswitch shutdown or reload...\n");

   // turn off PTT and POWER pins, DISABLE the radio
   for (int radio = 0; radio < globals.max_radios; radio++) {
      radio_set_state(radio, RADIO_OFF);
      globals.Radios[radio].enabled = 0;
   }

   // Free some memory
   switch_event_unbind_callback(event_handler);

   // Clear our memory before it's returned to freeswitch for reuse...
   memset(&globals, 0, sizeof(globals));

   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Goodbye from mod_hamradio, have a great day! We hope to see you back soon!\n");
   return SWITCH_STATUS_UNLOAD;
}

SWITCH_MODULE_RUNTIME_FUNCTION(mod_hamradio_runtime) {
   while (globals.alive) {
      for (int radio = 0; radio < globals.max_radios; radio++) {
         Radio_t *r = &globals.Radios[radio];

         // Another second has passed, reduce penalty time on this radio
         if (r->penalty > 0)
            r->penalty--;

         // penalty expired or in case somewhere screwed up...
         if (r->penalty <= 0) {
            r->penalty = 0;
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "radio%d penalty cleared\n", radio);
         }

         if (r->status == RADIO_RX) {
            // Here we should do receive radio stuff
         } else if (r->status == RADIO_TX) {
            // is a timeout timer set on this channel?
            if (r->timeout_talk > 0) {
               // Has the timer expired?
               if (r->talk_start + r->timeout_talk <= time(NULL)) {
                  switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "radio%d ending transmission (TOT expired: %s)\n", radio, time_to_timestr(r->timeout_talk));
                  // XXX: Optionally Play a status tone to indicate penalty status
                  // XXX: Figure out which RX radio is causing this TOT and punish it with the transmitter's penalty time
                  // globals.Radios[rx_radio]->penalty += r->timeout_penalty;
                  radio_ptt_off(radio);
               }
            }
         }

         // XXX: Check ident timeouts
      }
      // Sleep for about a second then repeat!
      sleep(1);
      switch_cond_next();
   }
   return SWITCH_STATUS_TERM;
}
