#pragma once

#include "memory.hpp"
#include "string.hpp"

#if defined(API_POSIX)
  #include "posix/shared-memory.hpp"
#endif

#if defined(API_WINDOWS)
  #include "windows/shared-memory.hpp"
#endif
