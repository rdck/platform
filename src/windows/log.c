#include "windows/wrapper.h"
#include <stdarg.h>
#include <stdio.h>
#include "log.h"

#ifndef PLATFORM_LOG_BUFFER
#define PLATFORM_LOG_BUFFER 0x400
#endif

#define TIME_STRING_BUFFER 0x20

// We'll implement proper log levels when we actually need them.
static Void platform_logv(LogLevel level, const Char* fmt, va_list ap)
{
  UNUSED_PARAMETER(level);

  // write log message to buffer
  Char buffer[PLATFORM_LOG_BUFFER];
  const S32 written = vsnprintf(buffer, PLATFORM_LOG_BUFFER, fmt, ap);
  UNUSED_PARAMETER(written);

  // read local time
  SYSTEMTIME lt;
  GetLocalTime(&lt);

  // write local time prefix to buffer
  Char time[TIME_STRING_BUFFER];
  snprintf(time, TIME_STRING_BUFFER, "[ %02d:%02d:%02d ] ", lt.wHour, lt.wMinute, lt.wSecond);

  // It's inefficient to make a separate call just for the newline, but that's
  // fine for now.
  OutputDebugString(time);
  OutputDebugString(buffer);
  OutputDebugString("\n");
}

Void platform_log(LogLevel level, const Char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  platform_logv(level, fmt, ap);
  va_end(ap);
}

Void platform_log_verbose(const Char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  platform_logv(LOG_LEVEL_VERBOSE, fmt, ap);
  va_end(ap);
}

Void platform_log_debug(const Char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  platform_logv(LOG_LEVEL_DEBUG, fmt, ap);
  va_end(ap);
}

Void platform_log_info(const Char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  platform_logv(LOG_LEVEL_INFO, fmt, ap);
  va_end(ap);
}

Void platform_log_warn(const Char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  platform_logv(LOG_LEVEL_WARN, fmt, ap);
  va_end(ap);
}

Void platform_log_error(const Char* fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  platform_logv(LOG_LEVEL_ERROR, fmt, ap);
  va_end(ap);
}
