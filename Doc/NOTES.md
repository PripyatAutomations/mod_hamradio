DISABLED and OFF state are NOT the same.
	- RADIO_DISABLED is not a valid radio->status value! It *must* be >= 0
	  However, many places return RADIO_DISABLED (-1) to show the radio isn't available (radio->enabled == false)
	- When a radio is disabled, we try to ensure it remains powered off. This is for safety reasons.
	- Powering off a radio does NOT disable the radio.
