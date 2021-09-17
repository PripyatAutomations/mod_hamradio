/*
 * dictionary based configuration files (ini style) support
 *
 * This code wouldn't be possible without N. Devillard's dictionary.[ch]
 * See dict.[ch] for slightly modified version of his code or search google for original
 */
//#define	_BSD_SOURCE
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <switch.h>
#include <strings.h>
#include <string.h>
#include "mod_hamradio.h"
#include "dict.h"
#include "config.h"

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
         section = strndup(skip + 1, strlen(skip) - 2);
//         switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "cfg.section.open: '%s'\n", section);
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
         // Parse configuration line (XXX: GET RID OF STRTOK!)
         key = strtok(skip, "= \n");
         val = strtok(NULL, "= \n");

         // Store value
         dict_add(cp, key, val);
      } else if (strncasecmp(section, "conference", 10) == 0) {
         // Parse configuration line (XXX: GET RID OF STRTOK!)
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

         if (radio < 0 || radio > MAX_RADIOS) {
           switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Radio configuration [%s] ignored - we only support %d MAX_RADIOS. (parsing '%s' at %s:%d)\n", section, MAX_RADIOS, buf, file, line);
           continue;
         }

         r = &globals.Radios[radio];

         // Parse configuration line (XXX: GET RID OF STRTOK!)
         key = strtok(skip, "= \n");
         val = strtok(NULL, "= \n");

//         switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "[cfg:radio%d] setting %s - %s. (parsing '%s' at %s:%d)\n", radio, key, val, buf, file, line);

         if (strncasecmp(key, "enabled", 7) == 0) {
           if (!strncasecmp(val, "true", 4) || !strncasecmp(val, "yes", 3) || !strncasecmp(val, "on", 2)) {
              r->enabled = true;
           } else {
              r->enabled = false;
           }
         } else if (strncasecmp(key, "ctcss_inband", 12) == 0) {
           if (!strncasecmp(val, "true", 4) || !strncasecmp(val, "yes", 3) || !strncasecmp(val, "on", 2)) {
              r->ctcss_inband = true;
           } else {
              r->ctcss_inband = false;
           }
         } else if (strncasecmp(key, "gpio_power", 10) == 0) {
           int ival = atoi(val);

           if (ival == -1) {
              // Some people don't use power contrl...
           } else if (ival >= 0 && ival <= MAX_GPIO) {
              r->pin_power = ival;
           } else { 
              switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[cfg:radio%d] Key %s has invalid value '%s'. (parsing '%s' at %s:%d)\n", radio, key, val, buf, file, line);
           }
         } else if (strncasecmp(key, "gpio_ptt", 8) == 0) {
           int ival = atoi(val);

           if (ival >= 0 && ival <= MAX_GPIO) {
              r->pin_ptt = ival;
           } else { 
              switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[cfg:radio%d] Key %s has invalid value '%s'. (parsing '%s' at %s:%d)\n", radio, key, val, buf, file, line);
           }
         } else if (strncasecmp(key, "gpio_squelch", 12) == 0) {
           int ival = atoi(val);

           if (ival == -1) {
              // Some devices don't have squelch output...
           } else if (ival >= 0 && ival <= MAX_GPIO) {
              r->pin_squelch = ival;
           } else { 
              switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[cfg:radio%d] Key %s has invalid value '%s'. (parsing '%s' at %s:%d)\n", radio, key, val, buf, file, line);
           }
         } else if (strncasecmp(key, "pa_indev", 8) == 0) {
           if (val != NULL) {
              memcpy(r->pa_indev, val, (strlen(val) > (PATH_MAX - 1)) ? strlen(val) : PATH_MAX - 1);
           }
         } else if (strncasecmp(key, "pa_outdev", 9) == 0) {
           if (val != NULL) {
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
         } else if (strncasecmp(key, "timeout_talk", 12) == 0) {
           int new_tot = 0;
           new_tot = timestr_to_time(val, 0);

           if (new_tot > 0) {
              r->timeout_talk = new_tot;
           } else {
              switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[%s] Invalid timeout_talk value '%s' parsing '%s' at %s:%d\n", section, val, buf, file, line);
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
