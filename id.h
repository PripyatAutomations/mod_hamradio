#if	!defined(ID_H)
#define	ID_H

// Auto-ID mode
typedef enum ID_Mode {
   ID_NONE = 0,
   ID_CW,
   ID_VOICE,
   ID_BOTH
} ID_Mode_t;

// Send identifications on a radio
extern switch_status_t send_ids_cw(const int radio);
extern switch_status_t send_ids_voice(const int radio);

#endif	// !defined(ID_H)
