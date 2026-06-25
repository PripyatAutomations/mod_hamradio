/*
 * libgpiod support for PTT, Power control, and Squelch inputs
 *
 * Here we try to provide support for multiple GPIO chips with lines attached
 * to them. We support this by using chip:pin syntax in the configuration.
 */
#include <switch.h>
#include <gpiod.h>
#include "mod_hamradio.h"

//////////////////////
// GPIO chip globals //
//////////////////////

// still single-chip for now
static struct gpiod_chip *gpiochip = NULL;

struct gpiod_chip *radio_find_gpiochip(const char *name) {
   (void)name;
   return gpiochip;
}

int radio_gpiochip_init(const char *chipname) {
   char path[64];

   if (gpiochip) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                        "[gpio] chip already initialized\n");
      return SWITCH_STATUS_SUCCESS;
   }

   // accept either "gpiochip0" or "/dev/gpiochip0"
   if (strncmp(chipname, "/dev/", 5) == 0) {
      snprintf(path, sizeof(path), "%s", chipname);
   } else {
      snprintf(path, sizeof(path), "/dev/%s", chipname);
   }

   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
                     "[gpio] opening chip %s\n", path);

   gpiochip = gpiod_chip_open(path);

   if (!gpiochip) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
                        "[gpio] failed to open %s\n", path);
      return SWITCH_STATUS_TERM;
   }

   return SWITCH_STATUS_SUCCESS;
}

//////////////////////
// helper requests   //
//////////////////////

static struct gpiod_line_request *
gpio_request_output(unsigned int offset,
                    const char *consumer,
                    int value) {
   struct gpiod_line_settings *st;
   struct gpiod_line_config *cfg;
   struct gpiod_request_config *rcfg;
   struct gpiod_line_request *req;
   unsigned int offs[1] = { offset };

   st = gpiod_line_settings_new();
   gpiod_line_settings_set_direction(st, GPIOD_LINE_DIRECTION_OUTPUT);

   gpiod_line_settings_set_output_value(
      st,
      value ? GPIOD_LINE_VALUE_ACTIVE
            : GPIOD_LINE_VALUE_INACTIVE);

   cfg = gpiod_line_config_new();
   gpiod_line_config_add_line_settings(cfg, offs, 1, st);

   rcfg = gpiod_request_config_new();
   gpiod_request_config_set_consumer(rcfg, consumer);

   req = gpiod_chip_request_lines(gpiochip, rcfg, cfg);

   gpiod_request_config_free(rcfg);
   gpiod_line_config_free(cfg);
   gpiod_line_settings_free(st);

   return req;
}

static struct gpiod_line_request *
gpio_request_input(unsigned int offset,
                   const char *consumer) {
   struct gpiod_line_settings *st;
   struct gpiod_line_config *cfg;
   struct gpiod_request_config *rcfg;
   struct gpiod_line_request *req;
   unsigned int offs[1] = { offset };

   st = gpiod_line_settings_new();
   gpiod_line_settings_set_direction(st, GPIOD_LINE_DIRECTION_INPUT);

   cfg = gpiod_line_config_new();
   gpiod_line_config_add_line_settings(cfg, offs, 1, st);

   rcfg = gpiod_request_config_new();
   gpiod_request_config_set_consumer(rcfg, consumer);

   req = gpiod_chip_request_lines(gpiochip, rcfg, cfg);

   gpiod_request_config_free(rcfg);
   gpiod_line_config_free(cfg);
   gpiod_line_settings_free(st);

   return req;
}

//////////////////////
// radio init        //
//////////////////////
int radio_gpio_init(const int radio) {
   Radio_t *r;

   if (radio < 0 || radio >= globals.max_radios) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
                        "[gpio] invalid radio %d\n", radio);
      return SWITCH_STATUS_FALSE;
   }

   r = &Radios(radio);

   if (!gpiochip) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
                        "[gpio] chip not initialized\n");
      return SWITCH_STATUS_FALSE;
   }

   //////////////////////
   // POWER GPIO       //
   //////////////////////
   if (r->pin_power >= 0) {
      r->gpio_power =
         gpio_request_output(r->pin_power,
                             "hamradio-power",
                             r->pin_power_invert ? 1 : 0);

      if (!r->gpio_power) {
         switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
                           "[gpio] radio %d power request failed\n", radio);
         return SWITCH_STATUS_FALSE;
      }
   }

   //////////////////////
   // PTT GPIO         //
   //////////////////////
   if (r->pin_ptt >= 0) {
      r->gpio_ptt =
         gpio_request_output(r->pin_ptt,
                             "hamradio-ptt",
                             r->pin_ptt_invert ? 1 : 0);

      if (!r->gpio_ptt) {
         switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
                           "[gpio] radio %d ptt request failed\n", radio);
         return SWITCH_STATUS_FALSE;
      }
   }

   //////////////////////
   // SQUELCH INPUT    //
   //////////////////////
   if (r->pin_squelch >= 0 && r->RX_mode == SQUELCH_GPIO) {
      r->gpio_squelch =
         gpio_request_input(r->pin_squelch,
                            "hamradio-squelch");

      if (!r->gpio_squelch) {
         switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
                           "[gpio] radio %d squelch request failed\n", radio);
         return SWITCH_STATUS_FALSE;
      }
   }

   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE,
                     "[gpio] radio %d init done (pwr=%p ptt=%p sq=%p)\n",
                     radio,
                     r->gpio_power,
                     r->gpio_ptt,
                     r->gpio_squelch);

   return SWITCH_STATUS_SUCCESS;
}

//////////////////////
// cleanup           //
//////////////////////

switch_status_t radio_gpio_fini(void) {
   if (!gpiochip) {
      return SWITCH_STATUS_SUCCESS;
   }

   for (int i = 0; i < globals.max_radios; i++) {
      Radio_t *r = &Radios(i);

      if (r->gpio_power) {
         gpiod_line_request_release(r->gpio_power);
         r->gpio_power = NULL;
      }

      if (r->gpio_ptt) {
         gpiod_line_request_release(r->gpio_ptt);
         r->gpio_ptt = NULL;
      }

      if (r->gpio_squelch) {
         gpiod_line_request_release(r->gpio_squelch);
         r->gpio_squelch = NULL;
      }
   }

   gpiod_chip_close(gpiochip);
   gpiochip = NULL;

   return SWITCH_STATUS_SUCCESS;
}

//////////////////////
// control helpers   //
//////////////////////

switch_status_t radio_gpio_ptt_on(const int radio) {
   Radio_t *r;

   if (radio < 0 || radio >= globals.max_radios) {
      return SWITCH_STATUS_FALSE;
   }

   r = &Radios(radio);

   if (!r->gpio_ptt) {
      return SWITCH_STATUS_FALSE;
   }

   gpiod_line_request_set_value(
      r->gpio_ptt,
      r->pin_ptt,
      r->pin_ptt_invert ? GPIOD_LINE_VALUE_INACTIVE
                        : GPIOD_LINE_VALUE_ACTIVE);

   return SWITCH_STATUS_SUCCESS;
}

switch_status_t radio_gpio_ptt_off(const int radio)
{
   Radio_t *r;

   if (radio < 0 || radio >= globals.max_radios) {
      return SWITCH_STATUS_FALSE;
   }

   r = &Radios(radio);

   if (!r->gpio_ptt) {
      return SWITCH_STATUS_FALSE;
   }

   gpiod_line_request_set_value(
      r->gpio_ptt,
      r->pin_ptt,
      r->pin_ptt_invert ? GPIOD_LINE_VALUE_ACTIVE
                        : GPIOD_LINE_VALUE_INACTIVE);

   return SWITCH_STATUS_SUCCESS;
}

switch_status_t radio_gpio_power_on(const int radio)
{
   Radio_t *r;

   if (radio < 0 || radio >= globals.max_radios) {
      return SWITCH_STATUS_FALSE;
   }

   r = &Radios(radio);

   if (!r->gpio_power) {
      return SWITCH_STATUS_FALSE;
   }

   gpiod_line_request_set_value(
      r->gpio_power,
      r->pin_power,
      r->pin_power_invert ? GPIOD_LINE_VALUE_INACTIVE
                          : GPIOD_LINE_VALUE_ACTIVE);

   return SWITCH_STATUS_SUCCESS;
}

switch_status_t radio_gpio_power_off(const int radio)
{
   Radio_t *r;

   if (radio < 0 || radio >= globals.max_radios) {
      return SWITCH_STATUS_FALSE;
   }

   r = &Radios(radio);

   if (!r->gpio_power) {
      return SWITCH_STATUS_FALSE;
   }

   gpiod_line_request_set_value(
      r->gpio_power,
      r->pin_power,
      r->pin_power_invert ? GPIOD_LINE_VALUE_ACTIVE
                          : GPIOD_LINE_VALUE_INACTIVE);

   return SWITCH_STATUS_SUCCESS;
}

//////////////////////
// squelch read     //
//////////////////////

int radio_gpio_read_squelch(const int radio)
{
   Radio_t *r;

   if (radio < 0 || radio >= globals.max_radios) {
      return -1;
   }

   r = &Radios(radio);

   if (!r->gpio_squelch) {
      return -1;
   }

   enum gpiod_line_value v =
      gpiod_line_request_get_value(
         r->gpio_squelch,
         r->pin_squelch);

   if (v < 0) {
      return -1;
   }

   if (r->squelch_invert) {
      return v == GPIOD_LINE_VALUE_INACTIVE;
   }

   return v == GPIOD_LINE_VALUE_ACTIVE;
}
