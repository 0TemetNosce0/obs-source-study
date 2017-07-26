#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

#define KSAUDIO_SPEAKER_4POINT1 (KSAUDIO_SPEAKER_QUAD|SPEAKER_LOW_FREQUENCY)
#define KSAUDIO_SPEAKER_2POINT1 (KSAUDIO_SPEAKER_STEREO|SPEAKER_LOW_FREQUENCY)

#define safe_release(ptr) \
	do { \
		if (ptr) { \
			ptr->lpVtbl->Release(ptr); \
		} \
	} while (false)
