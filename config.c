/*
 * Our terrible configuration parser.
 *
 * This needs a major overhaul, but works well enough so long as the config file is valid...
 *
 * The dictionary based (general) part wouldn't be possible without N. Devillard's dictionary.[ch]
 * See dict.[ch] for slightly modified version of his code or search google for original
 */
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <switch.h>
#include <strings.h>
#include <string.h>
#include "mod_hamradio.h"

dict *dconf_load(const char *file) {
   int line = 0, errors = 0, warnings = 0;
   int         in_comment = 0;
   char buf[768];
   FILE *fp;
   char *end, *skip,
        *key, *val,
        *section = NULL;
   dict *cp = dict_new();

   if (!(fp = fopen(file, "r"))) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "mod_hamdradio: %s: Failed loading '%s'\n", __FUNCTION__, file);
      fclose(fp);
      return false;
   }

   // We need to use safer string functions...
   do {
      memset(buf, 0, sizeof(buf));
      char *discard = fgets(buf, sizeof(buf) - 1, fp);
      line++;

      // delete prior whitespace...
      skip = buf;
      while(*skip == ' ')
        skip++;

      // Delete trailing newlines or white space
      end = buf + strlen(buf);
      do {
        *end = '\0';
        end--;
      } while(*end == '\r' || *end == '\n' || *end == ' ');

      // did we eat the whole line?
      if ((end - skip) <= 0)
         continue;

      // handle comments
      if (skip[0] == '*' && skip[1] == '/') {
         in_comment = 0;               /* end of block comment */
         continue;
      } else if (skip[0] == ';' || skip[0] == '#' || (skip[0] == '/' && skip[1] == '/')) {
         continue;                     /* line comment */
      } else if (skip[0] == '/' && skip[1] == '*')
         in_comment = 1;               /* start of block comment */

      if (in_comment)
         continue;                     /* ignored line, in block comment */

      if ((*skip == '/' && *skip+1 == '/') ||		// comments
           *skip == '#' || *skip == ';')
         continue;
      else if (*skip == '[' && *end == ']') {		// section
         // plug a memory leak
         if (section != NULL)
            free(section);

         section = strndup(skip + 1, strlen(skip) - 2);
         switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "cfg.section.open: '%s'\n", section);

         // Here we should initialize anything needed by a section
         if (strcasecmp(section, "tones")) {
            // Initialize the tone playback system
            radio_tones_init();
         }
         continue;
      }

      // Configuration data *MUST* be inside of a section, no exceptions.
      if (!section) {
         switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "config %s:%d: line outside of section: %s\n", file, line, buf);
         errors++;
         continue;
      }

      // @END exits a section early
      if (strcasecmp(skip, "@END") == 0) {
         section = NULL;
         continue;
      }

      ///////////////////////////////////
      // Handle configuration sections //
      ///////////////////////////////////

      ////////////////////////////////////
      // General Settings (dict backed) //
      ////////////////////////////////////
      if (strcasecmp(section, "general") == 0) {
         char *sep = strchr(skip, '=');

         if (sep == NULL) {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Radio configuration [%s] invalid key '%s' missing separator (=) (parsing %s:%d)\n", section, skip, file, line);
            continue;
         }

         // make sure you free this before returning/continuing!
         key = strndup(skip, (sep - skip));
         val = strndup(sep + 1, strlen(sep + 1));

         // Store value in the dictionary (globals.cfg)
         dict_add(cp, key, val);

         /////////////////////////////////////////
         // Scan dict config and update globals //
         /////////////////////////////////////////
         // Here we scan the dict for changed configurations that need refreshed in the globals struct (stuff that doesnt change except at reload but is polled often)
         int i;

         // XXX: This needs to be improved in a way that will reflect changes to the dict contents via api....
         if (strcasecmp(key, "max_radios") == 0) {
            // Define max radios
            if ((i = atoi(val)) > 0)
               globals.max_radios = i;
         } else if (strcasecmp(key, "max_conferences") == 0) {
            if ((i = atoi(val)) > 0)
               globals.max_conferences = i;
         } else if (strcasecmp(key, "poll_interval") == 0) {
            // Minimum poll time is 25ms
            if ((i = atoi(val)) >= 25) {
               globals.poll_interval = i;
            } else if (i == 0) {
               globals.poll_interval = 0;
               switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "poll_interval is disabled (0), set it to >= 25 to enable polling throttling, if you find CPU usage is too high when idle!\n");
            }
         } else if (strcasecmp(key, "id_timeout") == 0) {
            i = atoi(val);

            // ID timeout should always be set for ham usage, send a warning if not
            if (i <= 0) {
               switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "id_timeout should have non-zero value for ham usage!\n");
            } else {
               globals.timeout_id = i;
            }
         } else if (strcasecmp(key, "id_type") == 0) {
            if (strcasecmp(val, "none") == 0) {
               switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "id_type should be set to voice or cw for ham usage.\n");
               warnings++;
               continue;
            } else if (strcasecmp(val, "cw") == 0) {
               globals.id_type = ID_CW;
            } else if (strcasecmp(val, "voice") == 0) {
               globals.id_type = ID_VOICE;
            } else if (strcasecmp(val, "both") == 0) {
               globals.id_type = ID_BOTH;
            } else {
               switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "id_type '%s' is not valid while parsing configuration\n", val);
               errors++;
            }
         }

         // free some memory up
         free(key);
         free(val);

      /////////////////
      // Conferences //
      /////////////////
      } else if (strncasecmp(section, "conference", 10) == 0) {
         char *sep = strchr(skip, '=');
         // make sure you free this!
         key = strndup(skip, (sep - skip));
         val = strndup(sep + 1, strlen(sep + 1));
         // Conference config parser - XXX: Create a new conference struct and add to list...
         if (strcasecmp(key, "radios") == 0) {
         } else if (strcasecmp(key, "master_radio") == 0) {
           // XXX:
         } else if (strcasecmp(key, "admin_pin") == 0) {
           // XXX:
         } else if (strcasecmp(key, "listen_pin") == 0) {
           // XXX:
         }
         free(key);
         free(val);

      //////////////
      // Tonesets //
      //////////////
      } else if (strcasecmp(section, "tones") == 0) {
         char *sep = strchr(skip, '=');
         // make sure you free this!
         key = strndup(skip, (sep - skip));
         val = strndup(sep + 1, strlen(sep + 1));

         // Store value in the dictionary (globals.tones)
         dict_add(globals.radio_tones, key, val);

         // Add to dictionary
         free(key);
         free(val);

      //////////////////////
      // Radio Interfaces //
      //////////////////////
      } else if (strncasecmp(section, "radio", 5) == 0) {
         int radio = -1;
         char *radio_id_s = section + 5;
         Radio_t *r = NULL;
         char *sep = strchr(skip, '=');

         if (sep == NULL) {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Radio configuration [%s] invalid key '%s' missing separator (=) (parsing %s:%d)\n", section, skip, file, line);
            continue;
         }

         if (radio_id_s != NULL) {
            radio = atoi(radio_id_s);
         } else {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Radio configuration has invalid [radioX] section (parsing %s:%d)\n", file, line);
         }

         switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_INFO, "configuring radio%d\n", radio);

         if (radio < 0 || radio > globals.max_radios) {
           switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Radio configuration [%s] section ignored since general:radios is only set to %d! (parsing %s:%d)\n", section, globals.max_radios, file, line);
           continue;
         }

         // is this the first radio definition? if so, we must allocate the memory
         if (globals.Radios == NULL)
            globals.Radios = malloc(sizeof(Radio_t) * globals.max_radios);

         if ((r = &Radios(radio)) == NULL) {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "error bringing up radio%d - couldn't find memory structure!\n", radio);
            continue;
         }

         // make sure you free this!
         key = strndup(skip, (sep - skip));
         val = strndup(sep + 1, strlen(sep + 1));

         switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "[cfg:radio%d] setting %s - %s. (parsing '%s' at %s:%d)\n", radio, key, val, buf, file, line);

         if (strcasecmp(key, "enabled") == 0) {
           if (!strcasecmp(val, "true") || !strcasecmp(val, "yes") || !strcasecmp(val, "on")) {
              r->enabled = true;
           } else {
              r->enabled = false;
           }
         } else if (strcasecmp(key, "cat_type") == 0) {
            if (strcasecmp(val, "hamlib") == 0) {
            }
         } else if (strcasecmp(key, "cat_model") == 0) {
            if (strcasecmp(val, "probe") == 0) {
               // Set this to -1, so when we actually bring the radio up, we can see it's supposed tobe probed
               r->rig_model = -1;
            } else
               r->rig_model = atoi(val);
         } else if (strcasecmp(key, "cat_port") == 0) {
            memset(r->rig_path, 0, PATH_MAX);
            strncpy(r->rig_path, val, PATH_MAX);
         } else if (strcasecmp(key, "description") == 0) {
           char *qp = NULL, *ep = NULL;

           memset(r->description, 0, sizeof(r->description));
           if ((qp = strchr(val, '"')) != NULL) {
              // If we can't find a second ", it is an error...
              if ((ep = strrchr(qp, '"')) == NULL) {
                 // cry about missing end-quote
                 switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[config] Missing end-quote while trying to parse string in config at %s:%d\n", file, line);
                 // XXX: abort loading
              } else { // String is valid, copy it
                if ((ep - qp) < sizeof(r->description)) {
                   memcpy(r->description, qp + 1, (ep - qp) - 2);
                } else {
                   // cry that string is too big and truncate it...
                   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[radio%d] description too long (%lu bytes) and was truncated to %lu bytes!\n", radio, strlen(val), sizeof(r->description) - 1);
                   memcpy(r->description, qp + 1, sizeof(r->description) - 1);
                }
              }
           } else { // Not quoted
              if (strlen(val) <= sizeof(r->description) - 1) {
                 memcpy(r->description, val, strlen(val));
              } else {
                 // cry that the string is too big and truncate it...
                 switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[radio%d] description too long (%lu bytes) and was truncated to %lu bytes!\n", radio, strlen(val), sizeof(r->description) - 1);
                 memcpy(r->description, val, sizeof(r->description) - 1);
              }
           }
         } else if (strcasecmp(key, "ctcss_inband") == 0) {
           if (!strcasecmp(val, "true") || !strcasecmp(val, "yes") || !strcasecmp(val, "on")) {
              r->ctcss_inband = true;
           } else {
              r->ctcss_inband = false;
           }
         } else if (strcasecmp(key, "gpio_power_invert") == 0) {
           if (!strcasecmp(val, "true") || !strcasecmp(val, "yes") || !strcasecmp(val, "on")) {
              r->pin_power_invert = true;
           } else {
              r->pin_power_invert = false;
           }
         } else if (strcasecmp(key, "gpio_power") == 0) {
           int ival = atoi(val);

           // Some people don't use power control, -1 is a valid setting to indicate 'disabled'...
           if (ival == -1)
              switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[cfg:radio%d] gpio power control disabled.\n", radio);

           if (ival >= -1 && ival <= MAX_GPIO) {
              r->pin_power = ival;
           } else { 
              switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[cfg:radio%d] Key %s has invalid value '%s'. (parsing '%s' at %s:%d)\n", radio, key, val, buf, file, line);
           }
         } else if (strcasecmp(key, "gpio_ptt_invert") == 0) {
           if (!strcasecmp(val, "true") || !strcasecmp(val, "yes") || !strcasecmp(val, "on")) {
              r->pin_ptt_invert = true;
           } else {
              r->pin_ptt_invert = false;
           }
         } else if (strcasecmp(key, "gpio_ptt") == 0) {
           int ival = atoi(val);

           if (ival == -1)
              switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[cfg:radio%d] gpio ptt control disabled.\n", radio);

           // Receivers won't have a PTT pin, -1 is valid setting to indicate 'disabled'...
           if (ival >= -1 && ival <= MAX_GPIO) {
              r->pin_ptt = ival;
           } else { 
              switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[cfg:radio%d] Key %s has invalid value '%s'. (parsing '%s' at %s:%d)\n", radio, key, val, buf, file, line);
           }
         } else if (strcasecmp(key, "gpio_squelch") == 0) {
           int ival = atoi(val);

           // Some devices don't have squelch output, -1 is a valid setting to indicate 'disabled'...
           if (ival == -1) {
              switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[cfg:radio%d] gpio squlech input disabled.\n", radio);
           } else if (ival >= -1 && ival <= MAX_GPIO) {
              r->pin_squelch = ival;
           } else { 
              switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[cfg:radio%d] Key %s has invalid value '%s'. (parsing '%s' at %s:%d)\n", radio, key, val, buf, file, line);
           }
         } else if (strcasecmp(key, "pa_indev") == 0) {
           if (val != NULL) {
              // Zero out the buffer then copy our setting in
              memset(r->pa_indev, 0, sizeof(r->pa_indev));
              memcpy(r->pa_indev, val, (strlen(val) > (PATH_MAX - 1)) ? strlen(val) : PATH_MAX - 1);
           }
         } else if (strcasecmp(key, "pa_outdev") == 0) {
           if (val != NULL) {
              // Zero out the buffer then copy our setting in
              memset(r->pa_outdev, 0, sizeof(r->pa_outdev));
              memcpy(r->pa_outdev, val, (strlen(val) > (PATH_MAX - 1)) ? strlen(val) : PATH_MAX - 1);
           }
         } else if (strcasecmp(key, "squelch_mode") == 0) {
           if (!strcasecmp(val, "gpio") == 0) {
              r->RX_mode = SQUELCH_GPIO;
           } else if (!strcasecmp(val, "vox") == 0) {
              r->RX_mode = SQUELCH_VOX;
           } else
              r->RX_mode = SQUELCH_MANUAL;
         } else if (strcasecmp(key, "squelch_invert") == 0) {
           if (!strcasecmp(val, "true") || !strcasecmp(val, "yes") || !strcasecmp(val, "on")) {
              r->squelch_invert = true;
           } else {
              r->squelch_invert = false;
           }
         } else if (strcasecmp(key, "timeout_talk") == 0) {
           int new_tot = 0;
           new_tot = atoi(val);

           if (new_tot > 0) {
              switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "[%s] Set timeout_talk to %d\n", section, new_tot);
              r->timeout_talk = new_tot;
           } else {
              switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[%s] Invalid timeout_talk value '%s' parsing '%s' at %s:%d\n", section, val, buf, file, line);
              warnings++;
           }
         } else if (strcasecmp(key, "timeout_holdoff") == 0) {
           int new_holdoff = 0;
           new_holdoff = atoi(val);

           if (new_holdoff > 0) {
              switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "[%s] Set timeout_holdoff to %d\n", section, new_holdoff);
              r->timeout_holdoff = new_holdoff;
           } else {
              switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[%s] Invalid timeout_penalty value '%s' parsing '%s' at %s:%d\n", section, val, buf, file, line);
              warnings++;
           }
         }
         free(key);
         free(val);
      } else {
         switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "Unknown configuration section '%s' parsing '%s' at %s:%d\n", section, buf, file, line);
         warnings++;
      }
   } while (!feof(fp));

   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "configuration loaded with %d errors and %d warnings from %s (%d lines)\n", errors, warnings, file, line);
   fclose(fp);

   // avoid leaking memory from strdup above in section processing
   if (section != NULL)
      free(section);

   return cp;
}

///////////////////////////////////////////////////////////////////////////
// Functions for accessing dictionary contents, in the desired data type //
///////////////////////////////////////////////////////////////////////////
int dconf_get_bool(const char *key, const int def) {
   char       *tmp;
   int         rv = 0;

   if ((tmp = dconf_get_str(key, NULL)) == NULL)
      return def;
   else if (strcasecmp(tmp, "true") == 0 || strcasecmp(tmp, "on") == 0 ||
            strcasecmp(tmp, "yes") == 0 || (int)strtol(tmp, NULL, 0) == 1)
      rv = 1;
   else if (strcasecmp(tmp, "false") == 0 || strcasecmp(tmp, "off") == 0 ||
            strcasecmp(tmp, "no") == 0 || (int)strtol(tmp, NULL, 0) == 0)
      rv = 0;

   return rv;
}

double dconf_get_double(const char *key, const double def) {
   char       *tmp;

   if ((tmp = dconf_get_str(key, NULL)) == NULL)
      return def;

   return atof(tmp);
}

int dconf_get_int(const char *key, const int def) {
   char       *tmp;

   if ((tmp = dconf_get_str(key, NULL)) == NULL)
      return def;

   return (int)strtol(tmp, NULL, 0);
}

char       *dconf_get_str(const char *key, const char *def) {
   if (_CONF_DICT == NULL || key == NULL)
      return NULL;

   return (char *)dict_get(_CONF_DICT, key, def);
}

int dconf_set(const char *key, const char *val) {
   return dict_add(_CONF_DICT, key, val);
}

void dconf_unset(const char *key) {
   dict_del(_CONF_DICT, key);
}
