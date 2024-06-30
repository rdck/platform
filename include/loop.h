#pragma once

#include "linear_algebra.h"

typedef struct SystemInfo {
  V2S display; // primary display resolution
} SystemInfo;

typedef struct ProgramConfig {
  V2S window; // window resolution
} ProgramConfig;

typedef enum PlatformEventTag {
  PLATFORM_EVENT_NONE,
  PLATFORM_EVENT_KEY,
  PLATFORM_EVENT_CARDINAL,
} PlatformEventTag;

typedef struct PlatformEvent {
  PlatformEventTag tag;
} PlatformEvent;

typedef enum ProgramStatus {
  PROGRAM_STATUS_SUCCESS,
  PROGRAM_STATUS_FAILURE,
  PROGRAM_STATUS_LIVE,
  PROGRAM_STATUS_CARDINAL,
} ProgramStatus;

ProgramStatus loop_config(ProgramConfig* config, const SystemInfo* system);
ProgramStatus loop_init();
ProgramStatus loop_video();
ProgramStatus loop_audio(F32* out, Index frames);
ProgramStatus loop_event(const PlatformEvent* event);
ProgramStatus loop_terminate();
