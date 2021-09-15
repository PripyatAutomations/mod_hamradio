/////////////////
/// prototypes //
/////////////////
// Setup a GPIO controller chip
extern int radio_gpiochip_init(const char *chipname);

// Setup the needed libgpiod data for the radio line and set initial state (power on if enabled)
extern int radio_gpio_init(int radio);

// find an already initialized GPIO controller chip by name
struct gpiod_chip *radio_find_gpiochip(const char *name);
