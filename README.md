****
 Right now (9/19) there's a bug in the configuration stuff that mucks up
all the TOT and holdoff bits. I'll try to get it sorted out tomorrow.

Sorry!! :O At least i saved you finding out the hard way! Come back
tomorrow!
----


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

No warranties of any kind! If it breaks, you get to keep the pieces!

Feel free to contribute bugfixes/issues via github

https://github.com/PripyatAutomations/mod_hamradio
