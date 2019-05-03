// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


//    
// If software tracing is defined in the sources file..
// WPP_DEFINE_CONTROL_GUID specifies the GUID used for this driver.
// *** REPLACE THE GUID WITH YOUR OWN UNIQUE ID ***
// WPP_DEFINE_BIT allows setting debug bit masks to selectively print.   
// The names defined in the WPP_DEFINE_BIT call define the actual names 
// that are used to control the level of tracing for the control guid 
// specified. 
//
//   {71ae54db-0862-41bf-a24f-5330cec3c7f6}
//
#define WPP_CHECK_FOR_NULL_STRING  // to prevent exceptions due to NULL strings

#define WPP_CONTROL_GUIDS                                            \
    WPP_DEFINE_CONTROL_GUID(LeaseLayerTraceGuid,                        \
        (71ae54db,0862,41bf,a24f,5330cec3c7f6), \
        WPP_DEFINE_BIT(DBG_INIT)     \
        WPP_DEFINE_BIT(DBG_DRIVER)   \
        WPP_DEFINE_BIT(DBG_DEVICE)   \
        WPP_DEFINE_BIT(DBG_WMI)      \
        WPP_DEFINE_BIT(DBG_IO)       \
        WPP_DEFINE_BIT(DBG_IOCTL)    \
        WPP_DEFINE_BIT(DBG_LEASE)    \
        WPP_DEFINE_BIT(DBG_LEASE_AGENT)  \
        WPP_DEFINE_BIT(DBG_REMOTE_LEASE_AGENT)  \
        WPP_DEFINE_BIT(DBG_LEASE_SOCKET)  \
        WPP_DEFINE_BIT(DBG_LEASING_APPLICATION)  \
        WPP_DEFINE_BIT(DBG_LEASE_IDENTIFIER)  \
        WPP_DEFINE_BIT(DBG_LEASE_CHANNEL)  \
        WPP_DEFINE_BIT(DBG_LEASE_THREAD)  \
        WPP_DEFINE_BIT(DBG_LEASE_TRANSPORT) \
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



