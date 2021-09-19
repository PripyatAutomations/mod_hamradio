#if	!defined(TONES_H)
#define	TONES_H

extern dict *radio_tones;
extern int radio_tones_init(void);
extern int radio_send_tones(const int radio, const char *tone);
extern int radio_tone_store(const char *tone, const char *data);

#endif	// !defined(TONES_H)
