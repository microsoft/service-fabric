#pragma once

#include <cstdint>
#include <stdarg.h>
#include <stdio.h>

// Make sure __min()/__max() are defined
#ifndef __min
#define __min(a,b)  (((a) < (b)) ? (a) : (b))
#endif /*__min*/
#ifndef __max
#define __max(a,b)  (((a) > (b)) ? (a) : (b))
#endif /*__max*/

namespace KtlThreadpool{

    #define __int8              char
    #define __int16             short int
    #define __int32             int
    #define __int64             long

    // Data Types and Constants

    typedef const char          *LPCSTR;
    typedef char                CHAR,   *PCHAR, *LPSTR;
    typedef unsigned char       UCHAR,  *PUCHAR;
    typedef unsigned char       BYTE,   *PBYTE;
    typedef short               SHORT,  *PSHORT;
    typedef unsigned short      USHORT, *PUSHORT;
    typedef int                 INT,    *PINT;
    typedef unsigned int        UINT,   *PUINT;
    typedef int                 LONG,   *PLONG;
    typedef unsigned int        ULONG,  *PULONG;
    typedef unsigned short      WORD,   *PWORD;
    typedef unsigned int        DWORD,  *PDWORD, *LPDWORD;
    typedef __int64             LONGLONG,   *PLONGLONG;
    typedef unsigned __int64    ULONGLONG,  *PULONGLONG;
    typedef unsigned __int64    DWORD64;
    typedef float               FLOAT;
    typedef double              DOUBLE;

    typedef void                *PVOID, *LPVOID;
    typedef int                 BOOL, *PBOOL, *LPBOOL;
    typedef unsigned char       BOOLEAN, *PBOOLEAN;

    typedef signed __int8       INT8,   *PINT8;
    typedef unsigned __int8     UINT8,  *PUINT8;
    typedef signed __int16      INT16,  *PINT16;
    typedef unsigned __int16    UINT16, *PUINT16;
    typedef signed __int32      INT32,  *PINT32;
    typedef unsigned __int32    UINT32, *PUINT32;
    typedef unsigned __int32    ULONG32,*PULONG32;
    typedef signed __int64      LONG64, *PLONG64;
    typedef unsigned __int64    ULONG64,*PULONG64;
    typedef signed __int64      INT64,  *PINT64;
    typedef unsigned __int64    UINT64, *PUINT64;

    typedef DWORD (*PTHREAD_START_ROUTINE)(LPVOID lpThreadParameter);
    typedef PTHREAD_START_ROUTINE LPTHREAD_START_ROUTINE;

    typedef void                VOID;
    typedef VOID                *HANDLE;
    typedef HANDLE              *PHANDLE, *LPHANDLE;
    typedef LONG                HRESULT;
    typedef LONG                NTSTATUS;
    typedef size_t              SIZE_T;

    typedef union _LARGE_INTEGER {
        struct {
            DWORD LowPart;
            LONG  HighPart;
        } u;
        LONGLONG QuadPart;
    } LARGE_INTEGER, *PLARGE_INTEGER;

    typedef struct _FILETIME {
        DWORD dwLowDateTime;
        DWORD dwHighDateTime;
    } FILETIME, *PFILETIME, *LPFILETIME;

    typedef struct _TP_CPU_INFORMATION {
        union {
            FILETIME ftLastRecordedIdleTime;
            FILETIME ftLastRecordedCurrentTime;
        } LastRecordedTime;
        FILETIME ftLastRecordedKernelTime;
        FILETIME ftLastRecordedUserTime;
    } TP_CPU_INFORMATION;

    #define TRUE                    1
    #define FALSE                   0
    #define INVALID_HANDLE_VALUE    ((VOID *)(-1))
    #define  ULONGLONG_MAX          0xFFFFFFFFFFFFFFFFL

    typedef enum _TimeConversionConstants
    {
        tccSecondsToMillieSeconds       = 1000,         // 10^3
        tccSecondsToMicroSeconds        = 1000000,      // 10^6
        tccSecondsToNanoSeconds         = 1000000000,   // 10^9
        tccMillieSecondsToMicroSeconds  = 1000,         // 10^3
        tccMillieSecondsToNanoSeconds   = 1000000,      // 10^6
        tccMicroSecondsToNanoSeconds    = 1000,         // 10^3
        tccSecondsTo100NanoSeconds      = 10000000,     // 10^7
        tccMicroSecondsTo100NanoSeconds = 10            // 10^1
    } TimeConversionConstants;

    // Assertions and Debugging Support

    #define DBG_BUFFER_SIZE  512
    inline void DBG_printf(LPCSTR function, LPCSTR file, INT line, LPCSTR format, ...)
    {
        va_list args;
        CHAR *buffer[DBG_BUFFER_SIZE];
        va_start(args, format);

        vsnprintf((char*)buffer, DBG_BUFFER_SIZE, format, args);
        fprintf(stderr, "%s", (char*)buffer);
        va_end(args);
    }

#if __aarch64__
     inline VOID DBG_DebugBreak()
    {
        __builtin_trap();
    }
#else
    inline VOID DBG_DebugBreak()
    {
        __asm__ __volatile__("int $3");
    }
#endif
    
    #define ASSERT(args...)                                             \
    {                                                                   \
        DBG_printf(__FUNCTION__,__FILE__,__LINE__,args);                \
        DBG_DebugBreak();                                               \
    }

    #define _ASSERT(expr) do { if (!(expr)) { ASSERT(""); } } while(0)
    #define _ASSERTE(expr) do { if (!(expr)) { ASSERT("Expression: " #expr "\n"); } } while(0)
    #define _ASSERT_MSG(expr, args...)                                  \
        do {                                                            \
            if (!(expr))                                                \
            {                                                           \
                ASSERT("Expression: " #expr ", Description: " args);    \
            }                                                           \
        } while(0)


    // APIs Declaration

    #define FILETIME_TO_ULONGLONG(f) \
        (((ULONGLONG)(f).dwHighDateTime << 32) | ((ULONGLONG)(f).dwLowDateTime))

    BOOL QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount);

    BOOL QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency);

    ULONGLONG GetTickCount64();

    DWORD GetTickCount();

    DWORD GetCurrentThreadId();

    DWORD GetCurrentProcessId();

    VOID YieldProcessor();

    BOOL SwitchToThread();

    bool __SwitchToThread(uint32_t dwSleepMSec, uint32_t dwSwitchCount);

    VOID Sleep(uint32_t dwSleepMSec);

    DWORD GetCurrentProcessorNumber();

    DWORD GetNumActiveProcessors();

    VOID GetProcessDefaultStackSize(SIZE_T* stacksize);

    INT GetCPUBusyTime(TP_CPU_INFORMATION *lpPrevCPUInfo);

    //#define VERBOSE
}
