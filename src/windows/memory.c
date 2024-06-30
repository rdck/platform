#include "windows/wrapper.h"
#include "memory.h"
#include "log.h"

Void* platform_virtual_alloc(Size size)
{
  Void* const pointer = VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (pointer == NULL) {
    platform_log_error("out of memory");
    ExitProcess(EXIT_CODE_FAILURE);
  }
  return pointer;
}

Void platform_virtual_free(Void* pointer)
{
  VirtualFree(pointer, 0, MEM_RELEASE);
}
