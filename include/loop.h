#pragma once

#include "prelude.h"
#include "event.h"

typedef struct SystemInfo {
  V2S display; // primary display resolution
} SystemInfo;

typedef struct ProgramConfig {
  Char* title;      // window title
  Char* caption;    // popup caption
  V2S resolution;   // window resolution
} ProgramConfig;

typedef enum ProgramStatus {
  PROGRAM_STATUS_LIVE,
  PROGRAM_STATUS_SUCCESS,
  PROGRAM_STATUS_FAILURE,
  PROGRAM_STATUS_CARDINAL,
} ProgramStatus;

ProgramStatus loop_config(ProgramConfig* config, const SystemInfo* system);
ProgramStatus loop_init();
ProgramStatus loop_video();
ProgramStatus loop_audio(F32* out, Index frames);
Void loop_event(const Event* event);
Void loop_terminate();
