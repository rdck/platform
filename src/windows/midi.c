#include <windows.h>
#include <stdlib.h>
#include "prelude.h"
#include "log.h"

static HMIDIOUT* midi_devices = NULL;
static S32 midi_device_count = 0;

Void platform_midi_init()
{
  // count available midi devices
  midi_device_count = midiOutGetNumDevs();

  // allocate global midi device array
  midi_devices = calloc(midi_device_count, sizeof(*midi_devices));
  ASSERT(midi_devices);

  // open all devices
  for (Index i = 0; i < midi_device_count; i++) {
    const MMRESULT open_status = midiOutOpen(&midi_devices[i], (UINT) i, 0, 0, CALLBACK_NULL);
    if (open_status != MMSYSERR_NOERROR) {
      platform_log(LOG_LEVEL_WARN, "error opening midi device %lld\n", i);
    }
  }
}

S32 platform_midi_device_count()
{
  return midi_device_count;
}

Void platform_midi_note_on(S32 device, U32 channel, U32 note, U32 velocity)
{
  const HMIDIOUT out = midi_devices[device];
  if (out) {
    const U32 shifted_channel = 0x90 + channel;
    const U32 shifted_note = note << 8;
    const U32 shifted_velocity = velocity << 16;
    midiOutShortMsg(out, shifted_channel | shifted_note | shifted_velocity);
  }
}

Void platform_midi_note_off(S32 device, U32 channel, U32 note, U32 velocity)
{
  const HMIDIOUT out = midi_devices[device];
  if (out) {
    const U32 shifted_channel = 0x80 + channel;
    const U32 shifted_note = note << 8;
    const U32 shifted_velocity = velocity << 16;
    midiOutShortMsg(out, shifted_channel | shifted_note | shifted_velocity);
  }
}
