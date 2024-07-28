#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "timer.h"

static S64 timer_frequency = 0;

Void timer_init()
{
  LARGE_INTEGER frequency;
  QueryPerformanceFrequency(&frequency);
  timer_frequency = frequency.QuadPart;
}

S64 timer_get_frequency()
{
  return timer_frequency;
}

S64 timer_get_counter()
{
  LARGE_INTEGER pc;
  QueryPerformanceCounter(&pc);
  return pc.QuadPart;
}

S64 timer_get_time()
{
  const S64 counter = timer_get_counter();
  return (counter * GIGA) / timer_frequency;
}
