#ifndef crinha_common_h
#define crinha_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DEBUG_PRINT_CODE
#define DEBUG_TRACE_EXECUTION

#ifndef COMPUTED_GOTO
#ifdef _MSC_VER
  #define COMPUTED_GOTO 0
#else
  #define COMPUTED_GOTO 1
#endif
#endif

#define UINT8_COUNT (UINT8_MAX + 1)

#endif

#undef DEBUG_PRINT_CODE
#undef DEBUG_TRACE_EXECUTION