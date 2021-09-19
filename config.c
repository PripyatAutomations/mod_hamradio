/*
 * dictionary based configuration files (ini style) support
 *
 * This code wouldn't be possible without N. Devillard's dictionary.[ch]
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

   // This could use some cleanup... But it does work.
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
         if (strncasecmp(section, "tones", 5)) {
            // Initialize the tone playback system
            radio_tones_init();
         }
         continue;
      } else if (*skip == '@') {			// preprocessor
         if (strncasecmp(skip + 1, "if ", 3) == 0) {
            /* XXX: finish this */
         } else if (strncasecmp(skip + 1, "endif", 5) == 0) {
            /* XXX: finish this */
         } else if (strncasecmp(skip + 1, "else ", 5) == 0) {
            /* XXX: finish this */
         } else if (strncasecmp(skip + 1, "include ", 8) == 0) {
            /* XXX: Add includes (non-recursive?) */
         }
      }

      // Configuration data *MUST* be inside of a section, no exceptions.
      if (!section) {
         switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "config %s:%d: line outside of section: %s\n", file, line, buf);
         errors++;
         continue;
      }

      // @END exits a section early
      if (strncasecmp(skip, "@END", 4) == 0) {
         section = NULL;
         continue;
      }

      // Handle configuration sections
      if (strncasecmp(section, "general", 7) == 0) {
         key = strtok(skip, "= \n");
         val = strtok(NULL, "= \n");

         // Store value in the dictionary (globals.cfg)
         dict_add(cp, key, val);

         // Here we scan the dict for changed configurations that need refreshed in the globals struct (stuff that doesnt change except at reload but is polled often)
         int i;

         // XXX: This needs to be improved in a way that will reflect changes to the dict contents via api....
         if (strncasecmp(key, "max_radios", 9) == 0) {
            // Define max radios
            if ((i = atoi(val)) > 0)
               globals.max_radios = i;
         } else if (strncasecmp(key, "max_conferences", 16) == 0) {
            if ((i = atoi(val)) > 0)
               globals.max_conferences = i;
         } else if (strncasecmp(key, "poll_interval", 14) == 0) {
            // Minimum poll time is 25ms
            if ((i = atoi(val)) >= 25) {
               globals.poll_interval = i;
            } else {
               globals.poll_interval = 25;
               switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "poll_interval (%d) is too small - using minimum value of 25 (ms)!\n", i);
            }
         }
      } else if (strncasecmp(section, "conference", 10) == 0) {
         key = strtok(skip, "= \n");
         val = strtok(NULL, "= \n");
      } else if (strncasecmp(section, "radio", 5) == 0) {
         int radio = -1;
         char *radio_id_s = section + 5;
         Radio_t *r = NULL;

         if (radio_id_s != NULL) {
//            radio = strtoi(radio_id_s, NULL, 0, 10, 0, NULL);
            radio = atoi(radio_id_s);
         }

         if (radio < 0 || radio > globals.max_radios) {
           switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Radio configuration [%s] section ignored since general:radios is only set to %d! (parsing %s:%d)\n", section, globals.max_radios, file, line);
           continue;
         }

         // is this the first radio definition? if so, we must allocate the memory
         if (globals.Radios == NULL)
            globals.Radios = malloc(sizeof(Radio_t) * globals.max_radios);

         r = &globals.Radios[radio];
         key = strtok(skip, "= \n");
         val = strtok(NULL, "= \n");

//         switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "[cfg:radio%d] setting %s - %s. (parsing '%s' at %s:%d)\n", radio, key, val, buf, file, line);

         if (strncasecmp(key, "enabled", 7) == 0) {
           if (!strncasecmp(val, "true", 4) || !strncasecmp(val, "yes", 3) || !strncasecmp(val, "on", 2)) {
              r->enabled = true;
           } else {
              r->enabled = false;
           }
         } else if (strncasecmp(key, "description", 11) == 0) {
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
                   memcpy(r->description, qp, (ep - qp));
                } else {
                   // cry that string is too big and truncate it...
                   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[radio%d] description too long (%lu bytes) and was truncated to %lu bytes!\n", radio, strlen(val), sizeof(r->description));
                   memcpy(r->description, qp, sizeof(r->description) - 1);
                }
              }
           } else { // Not quoted
              if (strlen(val) <= sizeof(r->description) - 1) {
                 memcpy(r->description, val, strlen(val));
              } else {
                 // cry that the string is too big and truncate it...
                 switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[radio%d] description too long (%lu bytes) and was truncated to %lu bytes!\n", radio, strlen(val), sizeof(r->description));
                 memcpy(r->description, val, sizeof(r->description) - 1);
              }
           }
         } else if (strncasecmp(key, "ctcss_inband", 12) == 0) {
           if (!strncasecmp(val, "true", 4) || !strncasecmp(val, "yes", 3) || !strncasecmp(val, "on", 2)) {
              r->ctcss_inband = true;
           } else {
              r->ctcss_inband = false;
           }
         } else if (strncasecmp(key, "gpio_power", 10) == 0) {
           int ival = atoi(val);

           // Some people don't use power control, -1 is a valid setting to indicate 'disabled'...
           if (ival == -1)
              switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[cfg:radio%d] gpio power control disabled.\n", radio);

           if (ival >= -1 && ival <= MAX_GPIO) {
              r->pin_power = ival;
           } else { 
              switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[cfg:radio%d] Key %s has invalid value '%s'. (parsing '%s' at %s:%d)\n", radio, key, val, buf, file, line);
           }
         } else if (strncasecmp(key, "gpio_ptt", 8) == 0) {
           int ival = atoi(val);

           if (ival == -1)
              switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[cfg:radio%d] gpio ptt control disabled.\n", radio);

           // Receivers won't have a PTT pin, -1 is valid setting to indicate 'disabled'...
           if (ival >= -1 && ival <= MAX_GPIO) {
              r->pin_ptt = ival;
           } else { 
              switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[cfg:radio%d] Key %s has invalid value '%s'. (parsing '%s' at %s:%d)\n", radio, key, val, buf, file, line);
           }
         } else if (strncasecmp(key, "gpio_squelch", 12) == 0) {
           int ival = atoi(val);

           // Some devices don't have squelch output, -1 is a valid setting to indicate 'disabled'...
           if (ival == -1)
              switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[cfg:radio%d] gpio squlech input disabled.\n", radio);
           if (ival >= -1 && ival <= MAX_GPIO) {
              r->pin_squelch = ival;
           } else { 
              switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[cfg:radio%d] Key %s has invalid value '%s'. (parsing '%s' at %s:%d)\n", radio, key, val, buf, file, line);
           }
         } else if (strncasecmp(key, "pa_indev", 8) == 0) {
           if (val != NULL) {
              // Zero out the buffer then copy our setting in
              memset(r->pa_indev, 0, sizeof(r->pa_indev));
              memcpy(r->pa_indev, val, (strlen(val) > (PATH_MAX - 1)) ? strlen(val) : PATH_MAX - 1);
           }
         } else if (strncasecmp(key, "pa_outdev", 9) == 0) {
           if (val != NULL) {
              // Zero out the buffer then copy our setting in
              memset(r->pa_outdev, 0, sizeof(r->pa_outdev));
              memcpy(r->pa_outdev, val, (strlen(val) > (PATH_MAX - 1)) ? strlen(val) : PATH_MAX - 1);
           }
         } else if (strncasecmp(key, "squelch_mode", 12) == 0) {
           if (!strncasecmp(val, "gpio", 4) == 0) {
              r->RX_mode = SQUELCH_GPIO;
           } else if (!strncasecmp(val, "vox", 3) == 0) {
              r->RX_mode = SQUELCH_VOX;
           } else
              r->RX_mode = SQUELCH_MANUAL;
         } else if (strncasecmp(key, "squelch_invert", 14) == 0) {
           if (!strncasecmp(val, "true", 4) || !strncasecmp(val, "yes", 3) || !strncasecmp(val, "on", 2)) {
              r->squelch_invert = true;
           } else {
              r->squelch_invert = false;
           }
         } else if (strncasecmp(key, "timeout_talk", 13) == 0) {
           int new_tot = 0;
           new_tot = timestr_to_time(val, 0);

           if (new_tot > 0) {
              r->timeout_talk = new_tot;
           } else {
              switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[%s] Invalid timeout_talk value '%s' parsing '%s' at %s:%d\n", section, val, buf, file, line);
              warnings++;
           }
         } else if (strncasecmp(key, "timeout_holdoff", 16) == 0) {
           int new_holdoff = 0;
           new_holdoff = timestr_to_time(val, 0);

           if (new_holdoff > 0) {
              r->timeout_holdoff = new_holdoff;
           } else {
              switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[%s] Invalid timeout_penalty value '%s' parsing '%s' at %s:%d\n", section, val, buf, file, line);
              warnings++;
           }
         }
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

time_t dconf_get_time(const char *key, const time_t def) {
   return (timestr_to_time(dconf_get_str(key, NULL), def));
}

int dconf_set(const char *key, const char *val) {
   return dict_add(_CONF_DICT, key, val);
}

void dconf_unset(const char *key) {
   dict_del(_CONF_DICT, key);
}

/* these two are used to convert from/to a formatted time string.  the string
 * should be of the format [Xd][Xh][Xm][X][s].  e.g. 3600 is 3600s, or 1h.
 * either lower or upper case is acceptable. */
time_t timestr_to_time(const char *str, const time_t def) {
   char       *s;
   char       *s2;
   time_t      ret = 0;

   if (str == NULL)
      return def;

   s = s2 = strdup(str);
   while (*s2 != '\0') {
      *s2 = tolower(*s2);
      s2++;
   }

   s2 = strchr(s, 'd');
   if (s2 != NULL) {
      *s2 = '\0';
      ret += strtol(s, NULL, 0) * 86400;  /* days */
      s = s2 + 1;
   }
   s2 = strchr(s, 'h');
   if (s2 != NULL) {
      *s2 = '\0';
      ret += strtol(s, NULL, 0) * 3600;   /* hours */
      s = s2 + 1;
   }
   s2 = strchr(s, 'm');
   if (s2 != NULL) {
      *s2 = '\0';
      ret += strtol(s, NULL, 0) * 60;  /* minutes */
      s = s2 + 1;
   }
   ret += strtol(s, NULL, 0);          /* seconds */

   return ret;
}

char       *time_to_timestr(time_t itime) {
   int         d, h, m, s;
   static char rbuf[64];
   int         rlen = 0;

   d = itime / 86400;                  /* days */
   itime %= 86400;
   h = itime / 3600;                   /* hours */
   itime %= 3600;
   m = itime / 60;                     /* minutes */
   itime %= 60;
   s = itime;

   if (d)
      rlen += snprintf(rbuf + rlen, 64, "%dd", d);
   if (h)
      rlen += snprintf(rbuf + rlen, 64, "%dh", h);
   if (m)
      rlen += snprintf(rbuf + rlen, 64, "%dm", m);
   if (s)
      rlen += snprintf(rbuf + rlen, 64, "%ds", s);
   if (!rlen) {                        /* if we haven't added anything, it's 0s */
      rbuf[0] = '0';
      rbuf[1] = 's';
      rbuf[2] = '\0';
   }

   return rbuf;
}
