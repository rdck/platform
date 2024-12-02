/*******************************************************************************
 * file.h - storage interface
 *
 * I'm not worrying about making this a complete interface, for now.
 ******************************************************************************/

#pragma once

#include "prelude.h"

Byte* platform_read_file(const Char* path, Index* size);
Void platform_free_file(Byte* content);

Index platform_write_file(const Char* path, const Byte* content, Index size);
