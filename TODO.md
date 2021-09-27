**** Make radios dynamically added and removed, to get rid of some bugs ****
- Clean up module unloading/reloading so they work without spewing a bunch
  of pointless error messages...
- Tighter integration with mod_portaudio 
- VAD/VOX
- Add support for capping averaging duty cycle & round-robin TX
  * Adds a delay before repeater will acknowledge RXes
  * Add "Over"/Reset tones?
- Cleanup and stability improvements
- Watchdog task that ensures PTT pins turned off, if freeswitch might crash.

* Abort start if gpiochip /dev entry inaccessible.
