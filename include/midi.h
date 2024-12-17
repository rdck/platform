#include "prelude.h"

Void platform_midi_init();
S32 platform_midi_device_count();
Void platform_midi_note_on(S32 device, U32 channel, U32 note, U32 velocity);
Void platform_midi_note_off(S32 device, U32 channel, U32 note, U32 velocity);
