#include <stdlib.h>
#include "windows/wrapper.h"
#include "file.h"
#include "log.h"

Byte* platform_read_file(const Char* path, Index* size)
{
  // open file for reading
  const HANDLE handle = CreateFile(
      path,                   // file to open
      GENERIC_READ,           // open for reading
      FILE_SHARE_READ,        // share for reading
      NULL,                   // default security
      OPEN_EXISTING,          // existing file only
      FILE_ATTRIBUTE_NORMAL,  // normal file
      NULL);                  // no attr. template
  if (handle == INVALID_HANDLE_VALUE) {
    return NULL;
  }

  // get file size
  LARGE_INTEGER file_size;
  const BOOL file_size_status = GetFileSizeEx(handle, &file_size);
  ASSERT(file_size.HighPart == 0);
  if (file_size_status == FALSE) {
    CloseHandle(handle);
    return NULL;
  }

  // allocate memory
  Byte* const buffer = malloc(file_size.LowPart + 1);
  ASSERT(buffer);

  // read file
  DWORD bytes_read = 0;
  const BOOL read_file_status = ReadFile(
      handle,                 // file handle
      buffer,                 // out content
      file_size.LowPart,      // number of bytes to read
      &bytes_read,            // number of bytes read
      NULL);                  // overlapped
  ASSERT(bytes_read == file_size.LowPart);
  if (read_file_status == FALSE) {
    CloseHandle(handle);
    free(buffer);
    return NULL;
  }

  // close file
  CloseHandle(handle);

  // insert null terminator
  buffer[file_size.LowPart] = 0;

  // write out file size and return
  *size = (Index) bytes_read;
  return buffer;
}

Void platform_free_file(Byte* content)
{
  free(content);
}