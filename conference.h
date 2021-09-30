#if	!defined(CONFERENCE_H)
#define	CONFERENCE_H

struct	Conference {
    char id[32];
    char description[128];

    // Participants
    u_int8_t	radios;		// bitmask of radios in the conference
};

extern int radio_conference_init(void);

#endif	// !defined(CONFERENCE_H)
