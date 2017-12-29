#pragma once

//service model template built on top of shared-memory

#include "shared-memory.hpp"

#if defined(API_POSIX)
  #include "posix/service.hpp"
#endif

#if defined(API_WINDOWS)
  #include "windows/service.hpp"
#endif
