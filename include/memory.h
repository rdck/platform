/*******************************************************************************
 * memory.h - allocation interface
 ******************************************************************************/

#pragma once

#include "prelude.h"

Void* platform_virtual_alloc(Size size);
Void platform_virtual_free(Void* pointer);
