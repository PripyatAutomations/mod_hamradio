mod_hamradio
------------

A channel driver for radios attached to USB sound widgets.

Currently we only support GPIO based control but there are plans to
eventually support rigctld for controlling more advanced rigs.

You will need to create a dialplan leveraging mod_conference and
mod_portaudio to do much useful with this code.

There's a lot left to do before this is ready for every day use, but feature
requests, bug reports, and questions are always well!

Building your interface is up to you, but shouldn't require any exotic
hardware.

Feel free to add support for features or other platforms as you need, but i
will only be supporting Raspberry Pi 4 GPIOs officially.

We use libgpiod, however, so it shouldn't take much to make work elsewhere.

Audio is handled via portaudio channels, allowing you to use as many
channels as the machine can support.

No warranties of any kind! If it breaks, you get to keep the pieces!

Feel free to contribute bugfixes/issues via github at
   https://github.com/PripyatAutomations/mod_hamradio
