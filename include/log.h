#pragma once

#include "prelude.h"

typedef enum LogLevel {
  LOG_LEVEL_NONE,
  LOG_LEVEL_VERBOSE,
  LOG_LEVEL_DEBUG,
  LOG_LEVEL_INFO,
  LOG_LEVEL_WARN,
  LOG_LEVEL_ERROR,
  LOG_LEVEL_CARDINAL,
} LogLevel;

Void platform_log(LogLevel level, const Char* fmt, ...);
Void platform_log_verbose   (const Char* message, ...);
Void platform_log_debug     (const Char* message, ...);
Void platform_log_info      (const Char* message, ...);
Void platform_log_warn      (const Char* message, ...);
Void platform_log_error     (const Char* message, ...);
