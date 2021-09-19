Hardware Considerations
-----------------------


On Raspberry Pi, we have 40 GPIO and USB available.

For each radio you'll need to connect a few things:
    - Power
	Power control - Usually a relay on the IGNition SENse +12V circuit
    - PTT	(Push to Talk)
	You can use a relay or a a transistor for this switching.
    - Squelch Input
	Optional but avoids excessive CPU load from using VOX
    - Audio In/Out
	Use a USB sound card and connect the audio as fits your radio.
	You may need isolation transformers for some handhelds..
	