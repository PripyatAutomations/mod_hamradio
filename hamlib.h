#if	!defined(RADIO_RIGCTL_H)
#define	RADIO_RIGCTL_H

#include <switch.h>
#include <hamlib/rig.h>

extern switch_status_t radio_hamlib_init_radio(const int radio);
extern switch_status_t radio_hamlib_init(void);
extern switch_status_t radio_hamlib_fini(void);

///
extern switch_status_t	radio_hamlib_set_freq();
extern switch_status_t radio_hamlib_get_freq();
extern switch_status_t	radio_hamlib_set_mode();
extern switch_status_t radio_hamlib_get_mode();
extern switch_status_t radio_hamlib_set_ptt();
extern switch_status_t radio_hamlib_get_ptt();
extern switch_status_t radio_hamlib_set_ctcss();
extern switch_status_t radio_hamlib_get_ctcss();
extern switch_status_t radio_hamlib_set_ctcss_sql();
extern switch_status_t radio_hamlib_get_ctcss_sql();
extern switch_status_t radio_hamlib_set_dcs();
extern switch_status_t radio_hamlib_get_dcs();
extern switch_status_t radio_hamlib_set_dcs_sql();
extern switch_status_t radio_hamlib_get_dcs_sql();
extern switch_status_t radio_hamlib_set_vfo();
extern switch_status_t radio_hamlib_get_vfo();
extern switch_status_t radio_hamlib_get_vfo_mode();
extern switch_status_t radio_hamlib_set_rptr_shift();
extern switch_status_t radio_hamlib_get_rptr_shift();
extern switch_status_t radio_hamlib_set_rptr_offset();
extern switch_status_t radio_hamlib_get_rptr_offset();
extern switch_status_t radio_hamlib_set_split_freq();
extern switch_status_t radio_hamlib_get_split_freq();
extern switch_status_t radio_hamlib_set_split_freq_mode();
extern switch_status_t radio_hamlib_get_split_freq_mode();
extern switch_status_t radio_hamlib_set_split_vfo();
extern switch_status_t radio_hamlib_get_split_vfo();
extern switch_status_t radio_hamlib_set_rit();
extern switch_status_t radio_hamlib_get_rit();
extern switch_status_t radio_hamlib_set_xit();
extern switch_status_t radio_hamlib_get_xit();

#endif	// !defined(RADIO_RIGCTL_H)
