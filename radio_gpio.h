#if	!defined(RADIO_GPIO_H)
#define	RADIO_GPIO_H

struct GPIO_pin {
    int	gpiochip;	// index into GPIOchips array
};
typedef struct GPIO_pin GPIOpin;

/////////////////
/// prototypes //
/////////////////
// Setup a GPIO controller chip
extern int radio_gpiochip_init(const char *chipname);

// Setup the needed libgpiod data for the radio line and set initial state (power on if enabled)
extern int radio_gpio_init(const int radio);

// Shut down gpio and free all resources (for unload or reload)
extern switch_status_t radio_gpio_fini(void);

// find an already initialized GPIO controller chip by name
struct gpiod_chip *radio_find_gpiochip(const char *name);

// PTT on
extern switch_status_t radio_gpio_ptt_on(const int radio);

// PTT off
extern switch_status_t radio_gpio_ptt_off(const int radio);

// Power on
extern switch_status_t radio_gpio_power_on(const int radio);

// Power off
extern switch_status_t radio_gpio_power_off(const int radio);

// Read squelch input
extern int radio_gpio_read_squelch(const int radio);
#endif	// !defined(RADIO_GPIO_H)
