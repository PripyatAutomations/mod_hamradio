#include <switch.h>
#include <gpiod.h>
#include "mod_hamradio.h"
#include "radio_gpio.h"

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

   if (radio < 0 || radio > MAX_RADIOS) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "radio_gpio_init: (%d) is not a valid radio id\n", radio);
      err_invalid_radio(radio);
   }

   // Shorthand
   r = &globals.Radios[radio];

   if (globals.gpiochip == NULL) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "radio_gpio_init(%d) failed - gpiochip unset! you MUST call radio_gpiochip_init first!\n", radio);
      return SWITCH_STATUS_FALSE;
   }

   // Attach the power GPIO line to our interface
   if ((r->gpio_power = gpiod_chip_get_line(globals.gpiochip, r->pin_power)) == NULL) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "radio_gpio_init(%d)/power failed!\n", radio);
      return SWITCH_STATUS_FALSE;
   }
   gpiod_line_request_output(r->gpio_power, "power", 0);

   // Attach the ptt GPIO line to our interface
   if ((r->gpio_ptt = gpiod_chip_get_line(globals.gpiochip, r->pin_ptt)) == NULL) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "radio_gpio_init(%d)/ptt failed!\n", radio);
      return SWITCH_STATUS_FALSE;
   }
   gpiod_line_request_output(r->gpio_ptt, "ptt", 0);

   if (r->RX_mode == SQUELCH_MODE_GPIO) {
      switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "radio%d initializing GPIO squelch input", radio);

      // connect to the GPIO line
      if ((r->gpio_squelch = gpiod_chip_get_line(globals.gpiochip, r->pin_squelch)) == NULL) {
         switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "radio_gpio_init(%d)/squelch failed!\n", radio);
         return SWITCH_STATUS_FALSE;
      }
      // Set pin mode to INPUT
      gpiod_line_request_input(r->gpio_squelch, "squelch");
   }
   switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, "[gpio] Attached GPIOs for radio%d: power=%p ptt=%p squelch=%p\n", radio,
                     r->gpio_power, r->gpio_ptt, r->gpio_squelch);

   return SWITCH_STATUS_SUCCESS;
}
