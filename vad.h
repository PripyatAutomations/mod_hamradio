#if	!defined(VAD_H)
#define	VAD_H

// This will be used to keep track of the state of a channel's voice activity
typedef enum {
   CHAN_QUIET = 0,
   CHAN_NOISE,
   CHAN_VOICE,
   CHAN_ERROR
} chan_vad_activity;

#endif	// !defined(VAD_H)
