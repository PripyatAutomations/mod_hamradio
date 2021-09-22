#include <switch.h>
#include <gpiod.h>
#include "mod_hamradio.h"

// right now we only support one gpio chip, but this wrapper should ease transition
struct gpiod_chip *radio_find_gpiochip(const char *name) {
   return globals.gpiochip;
}

// Initialize a GPIO controller, so we can use it's pins
int radio_gpiochip_init(const char *chipname) {
   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[gpio] initializing gpio chip %s\n", chipname);
   
   if (radio_find_gpiochip(chipname) != NULL) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[gpio] chip %s is already open at [%p], leaving it alone\n", chipname, globals.gpiochip);
      return SWITCH_STATUS_SUCCESS;
   }

   // Locate the GPIO controller we want to talk to, by name
   if ((globals.gpiochip = gpiod_chip_open_by_name(chipname)) == NULL) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "[gpio] Error opening gpiochip %s, we cannot continue.\n", chipname);
   }
   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, "[gpio] connected chip %s to interface [%p]\n", chipname, globals.gpiochip);

   return SWITCH_STATUS_SUCCESS;
}

// XXX: Errors in here should be fatal if at startup (before globals.active == true)
int radio_gpio_init(const int radio) {
   Radio_t *r;

   if (radio < 0 || radio >= globals.max_radios) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "radio_gpio_init: (%d) is not a valid radio id\n", radio);
      err_invalid_radio(radio);
      return SWITCH_STATUS_FALSE;
   }

   // Shorthand
   r = &globals.Radios[radio];

   if (globals.gpiochip == NULL) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "radio_gpio_init(%d) failed - gpiochip unset! you MUST call radio_gpiochip_init first!\n", radio);
      return SWITCH_STATUS_FALSE;
   }

   // Attach the power GPIO line to our interface
   if (r->pin_power > 0) {
      if ((r->gpio_power = gpiod_chip_get_line(globals.gpiochip, r->pin_power)) == NULL) {
         switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "radio_gpio_init(%d)/power failed!\n", radio);
         return SWITCH_STATUS_FALSE;
      } else {
         gpiod_line_request_output(r->gpio_power, "power", 0);
      }
   }

   // Attach the ptt GPIO line to our interface
   if (r->pin_ptt > 0) {
      if ((r->gpio_ptt = gpiod_chip_get_line(globals.gpiochip, r->pin_ptt)) == NULL) {
         switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "radio_gpio_init(%d)/ptt failed!\n", radio);
         return SWITCH_STATUS_FALSE;
      }
      gpiod_line_request_output(r->gpio_ptt, "ptt", 0);
   }

   if (r->pin_squelch > 0) {
      if (r->RX_mode == SQUELCH_GPIO) {
         switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "radio%d initializing GPIO squelch input\n", radio);

         // connect to the GPIO line
         if ((r->gpio_squelch = gpiod_chip_get_line(globals.gpiochip, r->pin_squelch)) == NULL) {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "radio_gpio_init(%d)/squelch failed!\n", radio);
            return SWITCH_STATUS_FALSE;
         }
         // Set pin mode to INPUT
         gpiod_line_request_input(r->gpio_squelch, "squelch");
      }
   }
   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "[gpio] Attached GPIOs for radio%d: power=%p ptt=%p squelch=%p\n", radio,
                     r->gpio_power, r->gpio_ptt, r->gpio_squelch);

   return SWITCH_STATUS_SUCCESS;
}

switch_status_t radio_gpio_fini(void) {
   // if gpiochip is mapped, we need to unmap all the lines then the chip
   if (globals.gpiochip != NULL) {
      // Unmap the lines for each radio
      for (int i = 0; i < globals.max_radios; i++) {
         if (globals.Radios[i].gpio_power != NULL) {
            // Unmap the line
            gpiod_line_release(globals.Radios[i].gpio_power);
            globals.Radios[i].gpio_power = NULL;
         }

         if (globals.Radios[i].gpio_ptt != NULL) {
            // Unmap the line
            gpiod_line_release(globals.Radios[i].gpio_ptt);
            globals.Radios[i].gpio_ptt = NULL;
         }

         if (globals.Radios[i].gpio_squelch != NULL) {
            // Unmap the line
            gpiod_line_release(globals.Radios[i].gpio_squelch);
            globals.Radios[i].gpio_squelch = NULL;
         }
      }

      // Unmap the GPIO chip
      gpiod_chip_close(globals.gpiochip);
      globals.gpiochip = NULL;
   }
   return SWITCH_STATUS_SUCCESS;
}

switch_status_t radio_gpio_ptt_on(const int radio) {
   Radio_t *r;

   if (radio < 0 || radio >= globals.max_radios) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "%s: (%d) is not a valid radio id\n", __FUNCTION__, radio);
      err_invalid_radio(radio);
      return SWITCH_STATUS_FALSE;
   }

   // Shorthand
   r = &globals.Radios[radio];

   if (r == NULL || r->gpio_power == NULL)
      return SWITCH_STATUS_FALSE;

   gpiod_line_set_value(r->gpio_ptt, 1);
   return SWITCH_STATUS_SUCCESS;
}

switch_status_t radio_gpio_ptt_off(const int radio) {
   Radio_t *r;

   if (radio < 0 || radio >= globals.max_radios) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "%s: (%d) is not a valid radio id\n", __FUNCTION__, radio);
      err_invalid_radio(radio);
      return SWITCH_STATUS_FALSE;
   }

   // Shorthand
   r = &globals.Radios[radio];

   if (r == NULL || r->gpio_power == NULL)
      return SWITCH_STATUS_FALSE;

   gpiod_line_set_value(r->gpio_ptt, 0);
   return SWITCH_STATUS_SUCCESS;
}

switch_status_t radio_gpio_power_on(const int radio) {
   Radio_t *r;

   if (radio < 0 || radio >= globals.max_radios) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "%s: (%d) is not a valid radio id\n", __FUNCTION__, radio);
      err_invalid_radio(radio);
      return SWITCH_STATUS_FALSE;
   }

   // Shorthand
   r = &globals.Radios[radio];

   if (r == NULL || r->gpio_power == NULL)
      return SWITCH_STATUS_FALSE;

   gpiod_line_set_value(r->gpio_power, 1);
   return SWITCH_STATUS_SUCCESS;
}

switch_status_t radio_gpio_power_off(const int radio) {
   Radio_t *r;

   if (radio < 0 || radio >= globals.max_radios) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "%s: (%d) is not a valid radio id\n", __FUNCTION__, radio);
      err_invalid_radio(radio);
      return SWITCH_STATUS_FALSE;
   }

   // Shorthand
   r = &globals.Radios[radio];

   if (r == NULL || r->gpio_power == NULL)
      return SWITCH_STATUS_FALSE;

   gpiod_line_set_value(r->gpio_power, 0);
   return SWITCH_STATUS_SUCCESS;
}

int radio_gpio_read_squelch(const int radio) {
   Radio_t *r;
   int val = 0;

   if (radio < 0 || radio > globals.max_radios)
      return -1;

   r = &globals.Radios[radio];

   if (r == NULL || r->gpio_squelch == NULL)
      return -1;

   val = gpiod_line_get_value(r->gpio_squelch);
   return val;
}
