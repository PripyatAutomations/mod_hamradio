//////////////////////////
// channel driver stuff //
//////////////////////////
#include <switch.h>
#include "mod_hamradio.h"

switch_status_t chan_on_init(switch_core_session_t *session) {
    switch_channel_t *channel;

    if (session) {
       if ((channel = switch_core_session_get_channel(session))) {
          switch_channel_set_flag(channel, CF_AUDIO);
       }
    }
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t chan_on_hangup(switch_core_session_t *session) {
    return SWITCH_STATUS_SUCCESS;
}


switch_status_t chan_on_destroy(switch_core_session_t *session) {
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t chan_on_routing(switch_core_session_t *session) {
    switch_channel_t *channel = switch_core_session_get_channel(session);
//    private_t *pvt = switch_core_session_get_private(session);
    
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t chan_on_exchange_media(switch_core_session_t *session) {
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t chan_on_soft_execute(switch_core_session_t *session) {
    return SWITCH_STATUS_SUCCESS;
}

switch_call_cause_t chan_outgoing_call(switch_core_session_t *session, switch_event_t *var_event,
    switch_caller_profile_t *outbound_profile, switch_core_session_t **new_session,
    switch_memory_pool_t **pool, switch_originate_flag_t flags, switch_call_cause_t *cancel_cause) {
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t channel_read_frame(switch_core_session_t *session, switch_frame_t **frame, switch_io_flag_t flags, int stream_id) {
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t channel_write_frame(switch_core_session_t *session, switch_frame_t *frame, switch_io_flag_t flags, int stream_id) {
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t channel_kill_channel(switch_core_session_t *session, int sig) {
    return SWITCH_STATUS_SUCCESS;
}
