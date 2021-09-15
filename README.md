Sorry for the mess. This is some quick code i threw together in a couple of
long days trying to get my remote station up and going.


Here you will find a module to let one connect one or more radios to
freeswitch using cheap USB sound widgets and relays attached to GPIO pins.

Building your interface is up to you, but shouldn't require any exotic
hardware.


Feel free to add support for features or other platforms as you need, but i
will only be supporting Raspberry Pi 4 GPIOs officially.

We use libgpiod, however, so it shouldn't take much to make work elsewhere.


Audio is handled via portaudio channels, allowing you to use as many
channels as the machine can support.

Eventually it would be nice to split the GPIO based control into it's own
file, so that radios with USB/serial CAT based PTT could be supported.
