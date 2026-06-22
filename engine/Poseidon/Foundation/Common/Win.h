#pragma once

#if defined(_WIN32)
  #include <windows.h>
#else
  #undef Verify
  #define Verify( expr ) (expr)
#endif

#undef SearchPath
#undef DrawText
#undef GetObject
