#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "timer.h"

S64 timer_get_frequency()
{
  LARGE_INTEGER frequency;
  QueryPerformanceFrequency(&frequency);
  return frequency.QuadPart;
}

S64 timer_get_counter()
{
  LARGE_INTEGER pc;
  QueryPerformanceCounter(&pc);
  return pc.QuadPart;
}
