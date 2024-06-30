#include <math.h>
#include "glad/gl.h"
#include "loop.h"
#include "timer.h"
#include "audio_format.h"
#include "log.h"
#include "display.h"

#define LOOP_PI 3.141592653589793238f
#define TONE 440.f

static V2S render_resolution = { 32, 18 };
static V2S window_resolution = { 960, 540 };

static S64 frequency = 0;
static S64 frame = 0;
static TextureID texture_white = 0;

ProgramStatus loop_config(ProgramConfig* config, const SystemInfo* system)
{
  UNUSED_PARAMETER(system);
  config->resolution = window_resolution;
  config->title = "Example Program";
  config->caption = "Example Program";
  return PROGRAM_STATUS_LIVE;
}

ProgramStatus loop_init()
{
  display_init(window_resolution, render_resolution);
  const Byte white[] = { 0xFF, 0xFF, 0xFF, 0xFF };
  texture_white = display_load_image(white, v2s(1, 1));
  frequency = timer_get_frequency();
  return PROGRAM_STATUS_LIVE;
}

Void loop_event(const Event* event)
{
  UNUSED_PARAMETER(event);
}

ProgramStatus loop_video()
{
  display_begin_frame();
  display_begin_draw(texture_white);

  const V2S cursor = v2s_inv_scale(read_cursor(), 30);
  const V2F root = v2f_of_v2s(cursor);
  const V2F size = v2f(4.f, 4.f);
  display_draw_texture(root, size, COLOR_WHITE);

  display_end_draw();
  display_end_frame();

  return PROGRAM_STATUS_LIVE;
}

ProgramStatus loop_audio(F32* out, Index frames)
{
  for (Index i = 0; i < frames; i++) {
    const Index index = (frame + i) % PLATFORM_SAMPLE_RATE;
    const F32 r = (F32) index / PLATFORM_SAMPLE_RATE;
    const F32 v = sinf(2.f * TONE * LOOP_PI * r) * 0.2f;
    out[STEREO * i + 0] = v;
    out[STEREO * i + 1] = v;
  }
  frame += frames;
  return PROGRAM_STATUS_LIVE;
}

Void loop_terminate()
{
}
