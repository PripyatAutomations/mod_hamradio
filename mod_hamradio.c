/*
 * mod_hamradio.c:
 *
 *	An interface for hooking FreeSWITCH to hamradio equipment.
 *
 *   This code was originally made to use a Raspberry Pi for this, but
 * any GPIO interace could in theory be used.
 *
 *   I have broken up most of the code into smaller bits to help keep it maintainable
 * Feel free to contribute patches and bugfixes. Particularly areas of using FreeSWITCH
 * APIs rather than libc (for strings, memory, etc) - this is in work for 1.0.0 release
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
      if (radio < 0 || radio >= globals.max_radios) {
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
      if (radio < 0 || radio >= globals.max_radios) {
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

            if (Radios(radio).status == RADIO_OFF)
               stream->write_function(stream, "off\n");
            else
	       stream->write_function(stream, "on\n");
         }
	 goto done;
      } else if (argc == 2) {
         const int radio = atoi(argv[1]);

         if (radio < 0 || radio >= globals.max_radios) {
            err_invalid_radio(radio);
            status = SWITCH_STATUS_FALSE;
            goto done;
         }

	 stream->write_function(stream, "radio%d: power ", radio);

	 if (Radios(radio).status == RADIO_OFF)
	    stream->write_function(stream, "off\n");
         else
            stream->write_function(stream, "on\n");
      } else if (argc == 3) {
         const int radio = atoi(argv[1]);

         if (radio < 0 || radio >= globals.max_radios) {
            err_invalid_radio(radio);
            status = SWITCH_STATUS_FALSE;
            goto done;
         }

         if (!Radios(radio).enabled) {
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
            stream->write_function(stream, "radio%d: ", radio);

	    if (radio_get_state(radio) == RADIO_TX)
               stream->write_function(stream, "transmitting\n");
            else
	       stream->write_function(stream, "off\n");
         }
         goto done;
      } else if (argc == 2) {
         const int radio = atoi(argv[1]);

         if (radio < 0 || radio >= globals.max_radios) {
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

	 if (radio < 0 || radio >= globals.max_radios) {
	    err_invalid_radio(radio);
	    status = SWITCH_STATUS_FALSE;
	    goto done;
	 }

	 if (!Radios(radio).enabled) {
	    stream->write_function(stream, "Denying PTT request (via cli) for radio%d because it is in DISABLED state.\n", radio);
	    status = SWITCH_STATUS_FALSE;
	    goto done;
	 }

         if (radio_get_state(radio) == RADIO_OFF) {
	    stream->write_function(stream, "Ignoring PTT request (via cli) for radio%d because it is powered off.\n", radio);
	    status = SWITCH_STATUS_FALSE;
	    goto done;
         }

	 if ((val = str_to_intbool(argv[2])) == 1)
	    radio_set_state(radio, RADIO_TX);
	 else
	    radio_set_state(radio, RADIO_IDLE);

         stream->write_function(stream, "PTT for radio %d SET to %s.\n", radio, (val ? "ON" : "OFF"));

      }
      goto done;
   } else if (!strcasecmp(argv[0], "reload")) {
      radio_load_configuration(1);
   } else if (!strcasecmp(argv[0], "status")) {
      int active_radios = 0;
      switch_bool_t full = 0;
      
      if (argc == 1)
         full = false;
      else
         full = true;

      // This is a little gross, but reduces duplication...
      if (argc == 1 || (argc == 2 && (strcasecmp(argv[1], "all") == 0))) {
         stream->write_function(stream, "*** Status for ALL radios ***\n");
         for (int i = 0; i < globals.max_radios; i++) {
            enum RadioStatus rs = radio_get_state(i);

	    if (rs > RADIO_OFF)
	       active_radios++;
            radio_dump_state_var(i, full);
         }
         stream->write_function(stream, "*** (%d/%d units active) ***\n", active_radios, globals.max_radios);
      } else if (argc == 2) {
         int radio = atoi(argv[1]);

	 if (radio < 0 || radio >= globals.max_radios) {
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
switch_status_t radio_load_configuration(switch_bool_t reload) {
   switch_status_t status = SWITCH_STATUS_FALSE;

   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[mod_hamradio] %sloading configuration from hamradio.conf\n", (reload ? "re" : ""));

   if (globals.mutex == NULL) { 
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "radio_load_configuration - mutex not initialized, failing!\n");
      return SWITCH_STATUS_FALSE;
   }

   switch_mutex_lock(globals.mutex);

   if (reload == true) {
      radio_gpio_fini();

      // is dictionary existing already? free it if so
      if (globals.cfg != NULL) {
         dict_free(globals.cfg);
         globals.cfg = NULL;
      }
   }

   // Set a default poll interval early...
   if (globals.poll_interval == 0)
      globals.poll_interval = 100;

   // load the dictionary configuration
   if (!(globals.cfg = dconf_load(HAMRADIO_CONF))) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[mod_hamradio] %sloading configuration from hamradio.conf failed. Please examine the DEBUG level log output from mod_hamradio to see why!\n", (reload ? "re" : ""));
      return SWITCH_STATUS_FALSE;
   }

   // Initialize GPIO chip(s)
   radio_gpiochip_init(dconf_get_str("gpiochip", NULL));

   // step through all the configured radios and initialize them
   for (int radio = 0; radio < globals.max_radios; radio++) {
      Radio_t *r = &Radios(radio);

      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Bringing up interface radio%d\n", radio);

      // Initialize our mutexes
      switch_mutex_init(&Radios(radio).mutex, SWITCH_MUTEX_UNNESTED, globals.pool);

      // initialize it's GPIO interfaces, if any
      radio_gpio_init(radio);

      // Show some userful information in the log
      radio_dump_state_var(radio, true);

      // Power it up and make it available for use, if enabled
      if (r->enabled)
         radio_enable(radio);

      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Interface radio%d successfully brought up.\n", radio);
   }
   switch_mutex_unlock(globals.mutex);
   return status;
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

   // Zero out the configuration structure, such as radio data
   memset(&globals, 0, sizeof(globals));

   globals.modname = strdup(modname);

   // Initialize our mutexes
   switch_mutex_init(&globals.mutex, SWITCH_MUTEX_UNNESTED, pool);

   // Update some essential pointers:
   globals.pool = pool;

   // Load config
   radio_load_configuration(0);

   // Add our event hooks
   radio_events_init();

#if	!defined(NO_HAMLIB)
   // Initialize hamlib interface
   radio_hamlib_init();
#endif

   radio_conference_init();

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
      Radios(radio).enabled = 0;
   }

#if	!defined(NO_LIBGPIOD)
   // close all GPIO interfaces
   radio_gpio_fini();
#endif

#if	!defined(NO_HAMLIB)
   // XXX: close all hamlib interfaces
   radio_hamlib_fini();
#endif
   // Free some memory
   radio_events_fini();

   // Clear our memory before it's returned to freeswitch for reuse...
   free(globals.modname);
   memset(&globals, 0, sizeof(globals));

   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "Goodbye from mod_hamradio, have a great day! We hope to see you back soon!\n");
   return SWITCH_STATUS_UNLOAD;
}
