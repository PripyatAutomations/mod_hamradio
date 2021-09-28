Here is some code I've put together during the fall to run a remote station using Freeswitch.


It's intended to be as generic as possible, allowing use for a repeater or remote station.

See the configuration example in conf/ and make your own as appropriate in /etc/freeswitch


Edit Makefile to configure things - you can disable hamlib or libgpiod if not applicable.

REQUIREMENTS
------------

HARDWARE
========
	* Raspberry Pi
	* Some kind of interface board to connect GPIOs to radio PTT/squelch pins
	* USB sound widgets
	* Appropriate cabling

SOFTWARE
========
	* Debian 11
	* portaudio, hamlib, and libgpiod dev packages
	* Freeswitch and it's development headers, either packaged or locally built
		- MUST have mod_portaudio built or come up with your own audio solution
