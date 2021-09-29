#if	!defined(RADIO_RIGCTL_H)
#define	RADIO_RIGCTL_H

extern switch_status_t radio_hamlib_init_radio(const int radio);
extern switch_status_t radio_hamlib_init(void);
extern switch_status_t radio_hamlib_fini(void);

#endif	// !defined(RADIO_RIGCTL_H)
