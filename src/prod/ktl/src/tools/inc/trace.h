/*++

Copyright (c) Microsoft Corporation

Module Name:

    trace.h

Abstract:

    WPP tracing definitions.

Environment:

    Kernel mode only.

--*/

#define WPP_CHECK_FOR_NULL_STRING  //to prevent exceptions due to NULL strings

#define WPP_CONTROL_GUIDS                                               \
    WPP_DEFINE_CONTROL_GUID( KmUnitGuid,                                \
                             (14B4D055,6F9B,4F4E,BAA9,F0257A8A6E7F),    \
                             WPP_DEFINE_BIT(DBG_INIT)     \
                             WPP_DEFINE_BIT(DBG_RW)       \
                             WPP_DEFINE_BIT(DBG_IOCTL)    \
                             WPP_DEFINE_BIT(DBG_TESTS)    \
                             )                                    

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) WPP_LEVEL_LOGGER(flags)
#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level  >= lvl)

#pragma warning(disable:4204) // C4204 nonstandard extension used : non-constant aggregate initializer

//
// Define the 'xstr' structure for logging buffer and length pairs
// and the 'log_xstr' function which returns it to create one in-place.
// this enables logging of complex data types.
//
typedef struct xstr { char * _buf; short  _len; } xstr_t;
__inline xstr_t log_xstr(void * p, short l) { xstr_t xs = {(char*)p,l}; return xs; }

#pragma warning(default:4204)

//
// Define the macro required for a hexdump use as:
//
//   Hexdump((FLAG,"%!HEXDUMP!\n", log_xstr(buffersize,(char *)buffer) ));
//
//
#define WPP_LOGHEXDUMP(x) WPP_LOGPAIR(2, &((x)._len)) WPP_LOGPAIR((x)._len, (x)._buf)



