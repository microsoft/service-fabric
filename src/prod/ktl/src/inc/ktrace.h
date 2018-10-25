/*
   (c) 2010 by Microsoft Corp.  All Rights Reserved.

   ktrace.h

   Description:

     Kernel Template Library (KTL)
     Template classes & definitions for manifest-based ETW Tracing.

History:
     raymcc        10-Oct-2010        Initial version

--*/


#pragma once

#if KTL_USER_MODE
#include <stdarg.h>
#include <strsafe.h>
#include <ktlevents.um.h>
#if defined(PLATFORM_UNIX)
#include <ktlevents.um.linux.h>
#endif
#else
#include <ntstrsafe.h>
#include <ktlevents.km.h>
#endif

extern volatile BOOLEAN KDbgMirrorToDebugger;


#if KTL_USER_MODE
//
// This section describes a hook where KTL traces can be routed back up
// to another component. 
//
// Since KTL is architecturally under other components there is a special
// callback mechanism used so that KTL can call "up" to trace consumer.
// The trace consumer code would register the callback with KTL using the
// RegisterKtlTraceCallback() api. The registration is
// per process and independent of any KTL system.
//

//
// Function prototype for the trace callback into the trace provider
//
typedef VOID (*KtlTraceCallback)(
    __in LPCWSTR TypeText,
    __in USHORT Level,
    __in LPCSTR Text
   );

extern VOID RegisterKtlTraceCallback(
    __in KtlTraceCallback Callback
);  

extern KtlTraceCallback g_KtlTraceCallback;


inline BOOLEAN IsKtlTraceCallbackEnabled()
{
    return(g_KtlTraceCallback != nullptr);
}

inline VOID InvokeKtlTraceCallback(
    __in LPCWSTR TypeText,
    __in USHORT Level,
    __in LPCSTR Text
    )
{
    (*g_KtlTraceCallback)(TypeText, Level, Text);
}


// Types
#define typeCALLED L"CALLED"
#define typeWDATA L"WDATA"
#define typeWSTATUS L"WSTATUS"
#define typePRINTF L".PRINTF"
#define typeCHECKPOINT L".CHECKPOINT"
#define typeINFORMATIONAL L"INFORMATIONAL"
#define typeREQUEST L"REQUEST"
#define typeERROR L"ERROR"
#define typeCONFIG L"CONFIG"

// Levels - Copied from f:\gitroot\WindowsFabric\src\prod\src\Common\LogLevel.h
#define levelSilent   0
#define levelCritical 1
#define levelError    2
#define levelWarning  3
#define levelInfo     4
#define levelNoise    5
#define levelAll      99
#endif

class __KDbgFuncTracker
{
    NTSTATUS    _ExitCode;
    const char *_FuncName;
    const char *_Message;
public:
     __KDbgFuncTracker(const char* FuncName, int Line, const char* File)
     {
#if KTL_USER_MODE
         TRACE_EID_DBG_FUNCTION_ENTRY(FuncName, Line, File);
#else
         TRACE_EID_DBG_FUNCTION_ENTRY(NULL, FuncName, Line, File);
#endif

         _FuncName = FuncName;
         _Message = "";
     }

     void SetStatus(NTSTATUS Code, LPCSTR Message)
     {
          _ExitCode = Code;
          _Message = Message;
     }

     ~__KDbgFuncTracker()
     {
#if KTL_USER_MODE
         TRACE_EID_DBG_FUNCTION_EXIT(_FuncName, _ExitCode, _Message);
#else
         TRACE_EID_DBG_FUNCTION_EXIT(NULL, _FuncName, _ExitCode, _Message);
#endif
     }
};


inline NTSTATUS __KDbgPrintf(const char *Fmt, ...)
{
    char Dest[512];
    va_list args;
    va_start(args, Fmt);

#if KTL_USER_MODE
    NTSTATUS Res = StringCchVPrintfA(STRSAFE_LPSTR(Dest), 512, Fmt, args);
    TRACE_EID_DBG_PRINTF(Dest);

    if (IsKtlTraceCallbackEnabled())
    {
        InvokeKtlTraceCallback(typePRINTF, levelNoise, Dest);
    }

#else
    NTSTATUS Res = RtlStringCchVPrintfA(Dest, 512, Fmt, args);
    TRACE_EID_DBG_PRINTF(NULL, Dest);
#endif

    if (KDbgMirrorToDebugger)
    {
        DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "%s\n", Dest);
    }

    return Res;
}

inline NTSTATUS __KDbgPrintfVarArgs(const char *Fmt, va_list Args)
{
    char Dest[512];

#if KTL_USER_MODE
    NTSTATUS Res = StringCchVPrintfA(STRSAFE_LPSTR(Dest), 512, Fmt, Args);
    TRACE_EID_DBG_PRINTF(Dest);
    
    if (IsKtlTraceCallbackEnabled())
    {
        InvokeKtlTraceCallback(typePRINTF, levelNoise, Dest);
    }
#else
    NTSTATUS Res = RtlStringCchVPrintfA(Dest, 512, Fmt, Args);
    TRACE_EID_DBG_PRINTF(NULL, Dest);
#endif

    if (KDbgMirrorToDebugger)
    {
        DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "%s\n", Dest);
    }

    return Res;
}

inline NTSTATUS __KDbgPrintfInformational(const char *Fmt, ...)
{
    char Dest[512];
    va_list args;
    va_start(args, Fmt);

#if KTL_USER_MODE
    NTSTATUS Res = StringCchVPrintfA(STRSAFE_LPSTR(Dest), 512, Fmt, args);
    TRACE_EID_DBG_PRINTF_INFORMATIONAL(Dest);
    
    if (IsKtlTraceCallbackEnabled())
    {
        InvokeKtlTraceCallback(typeINFORMATIONAL, levelInfo, Dest);
    }
#else
    NTSTATUS Res = RtlStringCchVPrintfA(Dest, 512, Fmt, args);
    TRACE_EID_DBG_PRINTF_INFORMATIONAL(NULL, Dest);
#endif

    if (KDbgMirrorToDebugger)
    {
        DbgPrintEx(DPFLTR_DEFAULT_ID, DPFLTR_ERROR_LEVEL, "%s\n", Dest);
    }

    return Res;
}


inline void __KDbgCheckpoint(ULONG Id, LPCSTR Message, LPCSTR Func, int Line, LPCSTR File)
{
#if KTL_USER_MODE
    TRACE_EID_DBG_CHECKPOINT(Id, Message, Func, Line, File);

    if (IsKtlTraceCallbackEnabled())
    {
        char Dest[512];
        StringCchPrintfA(STRSAFE_LPSTR(Dest),
                512,
                "Checkpoint Id: %d Message: %s\n  Function: %s Line: %d\n  File: %s\n",
                Id,
                Message,
                Func,
                Line,
                File);
        InvokeKtlTraceCallback(typeCHECKPOINT, levelNoise, Dest);
    }
#else
    TRACE_EID_DBG_CHECKPOINT(NULL, Id, Message, Func, Line, File);
#endif
    if (KDbgMirrorToDebugger)
    {
        DbgPrintEx(DPFLTR_DEFAULT_ID,
                    DPFLTR_ERROR_LEVEL,
                    "Checkpoint Id: %d Message: %s\n\tFunction: %s Line: %d\n\tFile: %s\n",
                    Id,
                    Message,
                    Func,
                    Line,
                    File);
    }
}

inline void __KDbgCheckpoint(KActivityId Id, LPCSTR Message, LPCSTR Func, int Line, LPCSTR File)
{
#if KTL_USER_MODE
    TRACE_EID_DBG_CHECKPOINT_AI(Id.Get(), Message, Func, Line, File);

    if (IsKtlTraceCallbackEnabled())
    {
        char Dest[512];
        StringCchPrintfA(STRSAFE_LPSTR(Dest),
                         512,
                "Checkpoint ActivityId: %llx Message: %s\n  Function: %s Line: %d\n  File: %s\n",
                Id.Get(),
                Message,
                Func,
                Line,
                File);
        InvokeKtlTraceCallback(typeCHECKPOINT, levelNoise, Dest);
    }
    
#else
    TRACE_EID_DBG_CHECKPOINT_AI(NULL, Id.Get(), Message, Func, Line, File);
#endif
    if (KDbgMirrorToDebugger)
    {
        DbgPrintEx(DPFLTR_DEFAULT_ID,
                    DPFLTR_ERROR_LEVEL,
                    "Checkpoint ActivityId: %llx Message: %s\n\tFunction: %s Line: %d\n\tFile: %s\n",
                    Id.Get(),
                    Message,
                    Func,
                    Line,
                    File);
    }
}

inline void __KDbgCheckpointInformational(KActivityId Id, LPCSTR Message, LPCSTR Func, int Line, LPCSTR File)
{
#if KTL_USER_MODE
    TRACE_EID_DBG_CHECKPOINT_INFORMATIONAL(Id.Get(), Message, Func, Line, File);

    if (IsKtlTraceCallbackEnabled())
    {
        char Dest[512];
        StringCchPrintfA(STRSAFE_LPSTR(Dest),
                         512,
                "Checkpoint ActivityId: %llx Message: %s\n  Function: %s Line: %d\n  File: %s\n",
                Id.Get(),
                Message,
                Func,
                Line,
                File);
        InvokeKtlTraceCallback(typeINFORMATIONAL, levelInfo, Dest);
    }
#else
    TRACE_EID_DBG_CHECKPOINT_INFORMATIONAL(NULL, Id.Get(), Message, Func, Line, File);
#endif
    if (KDbgMirrorToDebugger)
    {
        DbgPrintEx(DPFLTR_DEFAULT_ID,
                    DPFLTR_ERROR_LEVEL,
                    "Checkpoint ActivityId: %llx Message: %s\n\tFunction: %s Line: %d\n\tFile: %s\n",
                    Id.Get(),
                    Message,
                    Func,
                    Line,
                    File
                        );
    }
}

inline void __KDbgCheckpointWData(
                    ULONG Id,
                    LPCSTR Message,
                    NTSTATUS Status,
                    ULONGLONG D1,
                    ULONGLONG D2,
                    ULONGLONG D3,
                    ULONGLONG D4,
                    LPCSTR Func,
                    int Line,
                    LPCSTR File)
{
#if KTL_USER_MODE
    TRACE_EID_DBG_CHECKPOINT_WDATA(Id, Message, Status, D1, D2, D3, D4, Func, Line, File);

    if (IsKtlTraceCallbackEnabled())
    {
        char Dest[512];
        StringCchPrintfA(STRSAFE_LPSTR(Dest),
                         512,
                "Checkpoint Id: %d Message: %s Status: %8.1X\n  D1: %16.1llX  D2: %16.1llX  D3: %16.1llX  D4: %16.1llX\n  Function: %s Line: %d\n  File: %s\n",

                Id,
                Message,
                Status,
                D1,
                D2,
                D3,
                D4,
                Func,
                Line,
                File);
        InvokeKtlTraceCallback(typeWDATA, levelNoise, Dest);
    }
#else
    TRACE_EID_DBG_CHECKPOINT_WDATA(NULL, Id, Message, Status, D1, D2, D3, D4, Func, Line, File);
#endif
    if (KDbgMirrorToDebugger)
    {
        DbgPrintEx(DPFLTR_DEFAULT_ID,
                    DPFLTR_ERROR_LEVEL,
                    "Checkpoint Id: %d Message: %s Status: %8.1X\n\tD1: %16.1llX  D2: %16.1llX  D3: %16.1llX  D4: %16.1llX\n\tFunction: %s Line: %d\n\tFile: %s\n",

                    Id,
                    Message,
                    Status,
                    D1,
                    D2,
                    D3,
                    D4,
                    Func,
                    Line,
                    File);
    }
}

inline void __KDbgCheckpointWData(
                    KActivityId Id,
                    LPCSTR Message,
                    NTSTATUS Status,
                    ULONGLONG D1,
                    ULONGLONG D2,
                    ULONGLONG D3,
                    ULONGLONG D4,
                    LPCSTR Func,
                    int Line,
                    LPCSTR File)
{
#if KTL_USER_MODE
    TRACE_EID_DBG_CHECKPOINT_WDATA_AI(Id.Get(), Message, Status, D1, D2, D3, D4, Func, Line, File);

    if (IsKtlTraceCallbackEnabled())
    {
        char Dest[512];
        StringCchPrintfA(STRSAFE_LPSTR(Dest),
                         512,
                "Checkpoint Activity Id: %llx Message: %s Status: %8.1X\n  D1: %16.1llX  D2: %16.1llX  D3: %16.1llX  D4: %16.1llX\n  Function: %s Line: %d\n  File: %s\n",

                Id.Get(),
                Message,
                Status,
                D1,
                D2,
                D3,
                D4,
                Func,
                Line,
                File);
        InvokeKtlTraceCallback(typeWDATA, levelNoise, Dest);
    }
#else
    TRACE_EID_DBG_CHECKPOINT_WDATA_AI(NULL, Id.Get(), Message, Status, D1, D2, D3, D4, Func, Line, File);
#endif
    if (KDbgMirrorToDebugger)
    {
        DbgPrintEx(DPFLTR_DEFAULT_ID,
                    DPFLTR_ERROR_LEVEL,
                    "Checkpoint Activity Id: %llx Message: %s Status: %8.1X\n\tD1: %16.1llX  D2: %16.1llX  D3: %16.1llX  D4: %16.1llX\n\tFunction: %s Line: %d\n\tFile: %s\n",

                    Id.Get(),
                    Message,
                    Status,
                    D1,
                    D2,
                    D3,
                    D4,
                    Func,
                    Line,
                    File);
    }
}

inline void __KDbgCheckpointWDataInformational(
                    KActivityId Id,
                    LPCSTR Message,
                    NTSTATUS Status,
                    ULONGLONG D1,
                    ULONGLONG D2,
                    ULONGLONG D3,
                    ULONGLONG D4,
                    LPCSTR Func,
                    int Line,
                    LPCSTR File)
{
#if KTL_USER_MODE
    TRACE_EID_DBG_CHECKPOINT_WDATA_INFORMATIONAL(Id.Get(), Message, Status, D1, D2, D3, D4, Func, Line, File);

    if (IsKtlTraceCallbackEnabled())
    {
        char Dest[512];
        StringCchPrintfA(STRSAFE_LPSTR(Dest),
                         512,
                "Checkpoint Activity Id: %llx Message: %s Status: %8.1X\n  D1: %16.1llX  D2: %16.1llX  D3: %16.1llX  D4: %16.1llX\n  Function: %s Line: %d\n  File: %s\n",

                Id.Get(),
                Message,
                Status,
                D1,
                D2,
                D3,
                D4,
                Func,
                Line,
                File);
        InvokeKtlTraceCallback(typeINFORMATIONAL, levelInfo, Dest);
    }
#else
    TRACE_EID_DBG_CHECKPOINT_WDATA_INFORMATIONAL(NULL, Id.Get(), Message, Status, D1, D2, D3, D4, Func, Line, File);
#endif
    if (KDbgMirrorToDebugger)
    {
        DbgPrintEx(DPFLTR_DEFAULT_ID,
                    DPFLTR_ERROR_LEVEL,
                    "Checkpoint Activity Id: %llx Message: %s Status: %8.1X\n\tD1: %16.1llX  D2: %16.1llX  D3: %16.1llX  D4: %16.1llX\n\tFunction: %s Line: %d\n\tFile: %s\n",

                    Id.Get(),
                    Message,
                    Status,
                    D1,
                    D2,
                    D3,
                    D4,
                    Func,
                    Line,
                    File);
    }
}

inline void __KDbgCheckpointWStatus(
                    ULONG Id,
                    LPCSTR Message,
                    NTSTATUS Status,
                    LPCSTR Func,
                    int Line,
                    LPCSTR File)
{
#if KTL_USER_MODE
    TRACE_EID_DBG_CHECKPOINT_WSTATUS(Id, Message, Status, Func, Line, File);

    if (IsKtlTraceCallbackEnabled())
    {
        char Dest[512];
        StringCchPrintfA(STRSAFE_LPSTR(Dest),
                         512,
                "Checkpoint Id: %d Message: %s Status: %8.1X\n  Function: %s Line: %d\n  File: %s\n",

                Id,
                Message,
                Status,
                Func,
                Line,
                File);
        InvokeKtlTraceCallback(typeWSTATUS, levelNoise, Dest);
    }
#else
    TRACE_EID_DBG_CHECKPOINT_WSTATUS(NULL, Id, Message, Status, Func, Line, File);
#endif
    if (KDbgMirrorToDebugger)
    {
        DbgPrintEx(DPFLTR_DEFAULT_ID,
                    DPFLTR_ERROR_LEVEL,
                    "Checkpoint Id: %d Message: %s Status: %8.1X\n\tFunction: %s Line: %d\n\tFile: %s\n",

                    Id,
                    Message,
                    Status,
                    Func,
                    Line,
                    File);
    }
}

inline void __KDbgCheckpointWStatus(
                    KActivityId Id,
                    LPCSTR Message,
                    NTSTATUS Status,
                    LPCSTR Func,
                    int Line,
                    LPCSTR File)
{
#if KTL_USER_MODE
    TRACE_EID_DBG_CHECKPOINT_WSTATUS_AI(Id.Get(), Message, Status, Func, Line, File);

    if (IsKtlTraceCallbackEnabled())
    {
        char Dest[512];
        StringCchPrintfA(STRSAFE_LPSTR(Dest),
                         512,
                "Checkpoint Activity Id: %llx Message: %s Status: %8.1X\n  Function: %s Line: %d\n  File: %s\n",

                Id.Get(),
                Message,
                Status,
                Func,
                Line,
                File
                    );
        InvokeKtlTraceCallback(typeWSTATUS, levelNoise, Dest);
    }
#else
    TRACE_EID_DBG_CHECKPOINT_WSTATUS_AI(NULL, Id.Get(), Message, Status, Func, Line, File);
#endif
    if (KDbgMirrorToDebugger)
    {
        DbgPrintEx(DPFLTR_DEFAULT_ID,
                    DPFLTR_ERROR_LEVEL,
                    "Checkpoint Activity Id: %llx Message: %s Status: %8.1X\n\tFunction: %s Line: %d\n\tFile: %s\n",

                    Id.Get(),
                    Message,
                    Status,
                    Func,
                    Line,
                    File);
    }
}

inline void __KDbgCheckpointWStatusInformational(
                    KActivityId Id,
                    LPCSTR Message,
                    NTSTATUS Status,
                    LPCSTR Func,
                    int Line,
                    LPCSTR File)
{
#if KTL_USER_MODE
    TRACE_EID_DBG_CHECKPOINT_WSTATUS_INFORMATIONAL(Id.Get(), Message, Status, Func, Line, File);

    if (IsKtlTraceCallbackEnabled())
    {
        char Dest[512];
        StringCchPrintfA(STRSAFE_LPSTR(Dest),
                         512,
                "Checkpoint Activity Id: %llx Message: %s Status: %8.1X\n  Function: %s Line: %d\n  File: %s\n",

                Id.Get(),
                Message,
                Status,
                Func,
                Line,
                File);
        InvokeKtlTraceCallback(typeINFORMATIONAL, levelInfo, Dest);
    }
#else
    TRACE_EID_DBG_CHECKPOINT_WSTATUS_INFORMATIONAL(NULL, Id.Get(), Message, Status, Func, Line, File);
#endif
    if (KDbgMirrorToDebugger)
    {
        DbgPrintEx(DPFLTR_DEFAULT_ID,
                    DPFLTR_ERROR_LEVEL,
                    "Checkpoint Activity Id: %llx Message: %s Status: %8.1X\n\tFunction: %s Line: %d\n\tFile: %s\n",

                    Id.Get(),
                    Message,
                    Status,
                    Func,
                    Line,
                    File);
    }
}


inline void __KDbgErrorWData(
                    KActivityId Id,
                    LPCSTR Message,
                    NTSTATUS Status,
                    ULONGLONG D1,
                    ULONGLONG D2,
                    ULONGLONG D3,
                    ULONGLONG D4,
                    LPCSTR Func,
                    int Line,
                    LPCSTR File)
{
#if KTL_USER_MODE
    TRACE_EID_DBG_ERROR_WDATA(Id.Get(), Message, Status, D1, D2, D3, D4, Func, Line, File);

    if (IsKtlTraceCallbackEnabled())
    {
        char Dest[512];
        StringCchPrintfA(STRSAFE_LPSTR(Dest),
                         512,
                "Error Activity Id: %16.1llX Message: %s Status: %8.1X\n  D1: %16.1llX  D2: %16.1llX  D3: %16.1llX  D4: %16.1llX\n  Function: %s Line: %d\n  File: %s\n",

                Id.Get(),
                Message,
                Status,
                D1,
                D2,
                D3,
                D4,
                Func,
                Line,
                File);
        InvokeKtlTraceCallback(typeERROR, levelError, Dest);
    }
#else
    TRACE_EID_DBG_ERROR_WDATA(NULL, Id.Get(), Message, Status, D1, D2, D3, D4, Func, Line, File);
#endif
    if (KDbgMirrorToDebugger)
    {
        DbgPrintEx(DPFLTR_DEFAULT_ID,
                    DPFLTR_ERROR_LEVEL,
                    "Error Activity Id: %16.1llX Message: %s Status: %8.1X\n\tD1: %16.1llX  D2: %16.1llX  D3: %16.1llX  D4: %16.1llX\n\tFunction: %s Line: %d\n\tFile: %s\n",

                    Id.Get(),
                    Message,
                    Status,
                    D1,
                    D2,
                    D3,
                    D4,
                    Func,
                    Line,
                    File);
    }
}

inline void __KDbgErrorWStatus(
                    KActivityId Id,
                    LPCSTR Message,
                    NTSTATUS Status,
                    LPCSTR Func,
                    int Line,
                    LPCSTR File)
{
#if KTL_USER_MODE
    TRACE_EID_DBG_ERROR_WSTATUS(Id.Get(), Message, Status, 0, 0, 0, 0, Func, Line, File);

    if (IsKtlTraceCallbackEnabled())
    {
        char Dest[512];
        StringCchPrintfA(STRSAFE_LPSTR(Dest),
                         512,
                "Error Activity Id: %16.1llX Message: %s Status: %8.1X\n  Function: %s Line: %d\n  File: %s\n",

                Id.Get(),
                Message,
                Status,
                Func,
                Line,
                File);
        InvokeKtlTraceCallback(typeERROR, levelError, Dest);
    }
#else
    TRACE_EID_DBG_ERROR_WSTATUS(NULL, Id.Get(), Message, Status, 0, 0, 0, 0, Func, Line, File);
#endif
    if (KDbgMirrorToDebugger)
    {
        DbgPrintEx(DPFLTR_DEFAULT_ID,
                    DPFLTR_ERROR_LEVEL,
                    "Error Activity Id: %16.1llX Message: %s Status: %8.1X\n\tFunction: %s Line: %d\n\tFile: %s\n",

                    Id.Get(),
                    Message,
                    Status,
                    Func,
                    Line,
                    File);
    }
}


#define KDbgPrintf(...)                             __KDbgPrintf(__VA_ARGS__)
#define KDbgPrintfVarArgs(Fmt, VaList)              __KDbgPrintfVarArgs(Fmt, VaList)
#define KDbgPrintfInformational(...)                __KDbgPrintfInformational(__VA_ARGS__)
#define KDbgCheckpoint(Id, Message)                 __KDbgCheckpoint(Id, Message, __FUNCTION__, __LINE__, __FILE__)
#define KDbgCheckpointInformational(Id, Message)    __KDbgCheckpointInformational(Id, Message, __FUNCTION__, __LINE__, __FILE__)
#define KDbgCheckpointWData(Id, Message, Status, D1, D2, D3, D4) \
          __KDbgCheckpointWData(Id, Message, Status, D1, D2, D3, D4, __FUNCTION__, __LINE__, __FILE__)
#define KDbgCheckpointWDataInformational(Id, Message, Status, D1, D2, D3, D4) \
          __KDbgCheckpointWDataInformational(Id, Message, Status, D1, D2, D3, D4, __FUNCTION__, __LINE__, __FILE__)
#define KDbgCheckpointWStatus(Id, Message, Status) \
          __KDbgCheckpointWStatus(Id, Message, Status, __FUNCTION__, __LINE__, __FILE__)
#define KDbgCheckpointWStatusInformational(Id, Message, Status) \
          __KDbgCheckpointWStatusInformational(Id, Message, Status, __FUNCTION__, __LINE__, __FILE__)

#define KDbgErrorWData(Id, Message, Status, D1, D2, D3, D4) \
          __KDbgErrorWData(Id, Message, Status, D1, D2, D3, D4, __FUNCTION__, __LINE__, __FILE__)
#define KDbgErrorWStatus(Id, Message, Status) \
          __KDbgErrorWStatus(Id, Message, Status, __FUNCTION__, __LINE__, __FILE__)

//
// These macros are designed to compile to nothing unless KTRACE_DEBUG is defined
//

#if DBG
__declspec(selectany) volatile BOOLEAN KDbgMirrorToDebugger = TRUE;
#else
__declspec(selectany) volatile BOOLEAN KDbgMirrorToDebugger = FALSE;
#endif

#if DBG
#define KTRACE_DEBUG
#endif
#ifdef KTRACE_DEBUG
#define KDbgEntry()                                 __KDbgFuncTracker __dbg_tracker(__FUNCTION__, __LINE__, __FILE__)
#define KDbgExit(StatusCode, Message)               __dbg_tracker.SetStatus(StatusCode, Message)
#else
#define KDbgEntry()
#define KDbgExit(StatusCode, Message)
#endif

//
// Error traces
//

inline void
__KTraceOutOfMemory(
    KActivityId ActivityId,
    NTSTATUS Status,
    PVOID ContextPointer,
    ULONGLONG D1,
    ULONGLONG D2,
    LPCSTR Function,
    int Line,
    LPCSTR File
)
{
#if KTL_USER_MODE
    TRACE_EID_ERR_OUT_OF_MEMORY(ActivityId.Get(), Status, ContextPointer, D1, D2, Function, Line, File);

    if (IsKtlTraceCallbackEnabled())
    {
        char Dest[512];
        StringCchPrintfA(STRSAFE_LPSTR(Dest),
                         512,
                "Out of memory. Activity Id: %16.1llX Status: %8.1X\n  Context: %16.1llX D1: %16.1llX D2: %16.1llX\n  Function: %s Line: %d\n  File: %s\n",
                ActivityId.Get(),
                Status,
                (ULONGLONG)ContextPointer,
                D1,
                D2,
                Function,
                Line,
                File );
        InvokeKtlTraceCallback(typeERROR, levelError, Dest);
    }
#else
    TRACE_EID_ERR_OUT_OF_MEMORY(NULL, ActivityId.Get(), Status, ContextPointer, D1, D2, Function, Line, File);
#endif

    if (KDbgMirrorToDebugger)
    {
        DbgPrintEx(DPFLTR_DEFAULT_ID,
                    DPFLTR_ERROR_LEVEL,
                    "Out of memory. Activity Id: %16.1llX Status: %8.1X\n\tContext: %16.1llX D1: %16.1llX D2: %16.1llX\n\tFunction: %s Line: %d\n\tFile: %s\n",
                    ActivityId.Get(),
                    Status,
                    (ULONGLONG)ContextPointer,
                    D1,
                    D2,
                    Function,
                    Line,
                    File );
    }
}

/*++
 *
 * Routine Description:
 *      This macros is used to report when a memory allocation failure occured.
 *      Normally this macro not used when there is a KAsyncContext or
 *      KAsyncParentContext
 *
 * Arguments:
 *      ActivityId - Supplies the activity id for the current activity if there is
 *          one.
 *
 *      Status - Supplies that status that will be returned to the caller typically
 *          STATUS_INSUFFICIENT_RESOURCES.
 *
 *      Context - Supplies a context pointer which is typically the this pointer for
 *          the member function doing the allocated but maybe anything pointer that
 *          helps the correlate the error with other events.
 *
 *      D1 - Supplies additional ULONGLONG of information
 *
 *      D2 - Supplies additional ULONGLONG of information
 *
-*/

#define KTraceOutOfMemory(ActivityId, Status, Context, D1, D2) \
    __KTraceOutOfMemory(ActivityId, Status, Context, D1, D2, __FUNCTION__, __LINE__, __FILE__)

inline void
__KTraceFailedAsyncRequest(
    NTSTATUS Status,
    KAsyncContextBase *AsyncContext,
    ULONGLONG D1,
    ULONGLONG D2,
    LPCSTR Function,
    int Line,
    LPCSTR File
)
{
    ULONGLONG ActivityId = AsyncContext == nullptr ? 0 : AsyncContext->GetActivityId().Get();

#if KTL_USER_MODE
    TRACE_EID_FAILED_ASYNC_REQUEST(ActivityId, Status, AsyncContext, D1, D2, Function, Line, File);

    if (IsKtlTraceCallbackEnabled())
    {
        char Dest[512];
        StringCchPrintfA(STRSAFE_LPSTR(Dest),
                         512,
                "An Async request was failed. Activity Id: %16.1llX Status: %8.1X\n  AsyncContext: %16.1llX D1: %16.1llX D2: %16.1llX\n  Function: %s Line: %d\n  File: %s\n",
                ActivityId,
                Status,
                (ULONGLONG)AsyncContext,
                D1,
                D2,
                Function,
                Line,
                File );
        InvokeKtlTraceCallback(typeREQUEST, levelWarning, Dest);
    }
#else
    TRACE_EID_FAILED_ASYNC_REQUEST(NULL, ActivityId, Status, AsyncContext, D1, D2, Function, Line, File);
#endif
    if (KDbgMirrorToDebugger)
    {
        DbgPrintEx(DPFLTR_DEFAULT_ID,
                    DPFLTR_TRACE_LEVEL,
                    "An Async request was failed. Activity Id: %16.1llX Status: %8.1X\n\tAsyncContext: %16.1llX D1: %16.1llX D2: %16.1llX\n\tFunction: %s Line: %d\n\tFile: %s\n",
                    ActivityId,
                    Status,
                    (ULONGLONG)AsyncContext,
                    D1,
                    D2,
                    Function,
                    Line,
                    File );
    }
}

/*++
 *
 * Routine Description:
 *      This macros is used to report when a error is returned for Async operaiton
 *      including memory allocation failures.
 *
 * Arguments:
 *      Status - Supplies that status of that will be set on the async operation.
 *
 *      AsyncContext - Supplies either the AsyncContext or the Parent context for the
 *          ill fated operaiton. The activity id is extracted from the AsyncContext
 *          and the AsyncContext value is also logged.
 *
 *      D1 - Supplies additional ULONGLONG of information
 *
 *      D2 - Supplies additional ULONGLONG of information
 *
-*/

#define KTraceFailedAsyncRequest(Status, AsyncContext, D1, D2) \
    __KTraceFailedAsyncRequest(Status, AsyncContext, D1, D2, __FUNCTION__, __LINE__, __FILE__)

inline void
__KTraceCancelCalled(
    KAsyncContextBase *AsyncContext,
    BOOLEAN Cancelable,
    BOOLEAN Completed,
    ULONGLONG D1,
    LPCSTR Function,
    int Line,
    LPCSTR File
)
{
    ULONGLONG ActivityId = AsyncContext == nullptr ? 0 : AsyncContext->GetActivityId().Get();
#if KTL_USER_MODE
    TRACE_EID_TRACE_CANCEL_CALLED(ActivityId, AsyncContext, Cancelable, Completed, D1, Function, Line, File);

    if (IsKtlTraceCallbackEnabled())
    {
        char Dest[512];
        StringCchPrintfA(STRSAFE_LPSTR(Dest),
                         512,
                "Cancel Called. Activity Id: %16.1llX Cancelable: %d Completed: %d\n  AsyncContext: %16.1llX D1: %16.1llX\n  Function: %s Line: %d\n  File: %s\n",
                ActivityId,
                Cancelable,
                Completed,
                (ULONGLONG)AsyncContext,
                D1,
                Function,
                Line,
                File );
        InvokeKtlTraceCallback(typeCALLED, levelNoise, Dest);
    }
#else
    TRACE_EID_TRACE_CANCEL_CALLED(NULL, ActivityId, AsyncContext, Cancelable, Completed, D1, Function, Line, File);
#endif
    if (KDbgMirrorToDebugger)
    {
        DbgPrintEx(DPFLTR_DEFAULT_ID,
                    DPFLTR_TRACE_LEVEL,
                    "Cancel Called. Activity Id: %16.1llX Cancelable: %d Completed: %d\n\tAsyncContext: %16.1llX D1: %16.1llX\n\tFunction: %s Line: %d\n\tFile: %s\n",
                    ActivityId,
                    Cancelable,
                    Completed,
                    (ULONGLONG)AsyncContext,
                    D1,
                    Function,
                    Line,
                    File );
    }
}

/*++
 *
 * Routine Description:
 *      This macros is used to report when an async cancel routine is called.
 *
 * Arguments:
 *      AsyncContext - Supplies the AsyncContext which is normally the this pointer
 *          of the cancel method. The ActivityId is extracted from the AsyncContext
 *          and logged.
 *
 *      Cancelable - Indiates the operaiton was in such a state that it could be
 *          canceled. Note that if this routine calls canncel on another async
 *          context then the return value of Cancel() should be used.
 *
 *      Completed - Indicates of the operaiton was actually completed.  This is
 *          typically the return value of the Complete() method.
 *
 *      D1 - Supplies additional ULONGLONG of information
 *
-*/

#define KTraceCancelCalled(AsyncContext, Cancelable, Completed, D1) \
    __KTraceCancelCalled(AsyncContext, Cancelable, Completed, D1, __FUNCTION__, __LINE__, __FILE__)


inline void
KTraceConfigurationChange(
    __in LPCSTR ClassName,
    __in PVOID Context,
    __in LPCSTR ParameterName,
    __in ULONGLONG OldValue,
    __in ULONGLONG NewValue
    )
/*++
 *
 * Routine Description:
 *      This routine is used to log changes to tunable configuration parameters for
 *      component.
 *
 * Arguments:
 *      ClassName - Supplies the class or component name.
 *
 *      Context - Supplies a pointer to indicate where the class is stored.  Provides
 *          context as to which instance was updated.
 *
 *       ParameterName - Supplies the name of the parameter that was changed.
 *
 *       OldValue - Supplies the old setting.
 *
 *       NewValue - Supplies the new setting.
 *
 * Return Value:
 *      None.
 *
 * Note:
 *
-*/
{
#if KTL_USER_MODE
    TRACE_EID_TRACE_CONFIG_CHANGE(ClassName, Context, ParameterName, OldValue, NewValue);

    if (IsKtlTraceCallbackEnabled())
    {
        char Dest[512];
        StringCchPrintfA(STRSAFE_LPSTR(Dest),
                         512,
                "Configuration parameter changed in class:= %s (Context:=%p) (Parameter:= %s) (OldValue:=%lld) (NewValue:=%lld)\n",
                ClassName,
                Context,
                ParameterName,
                OldValue,
                NewValue);
        InvokeKtlTraceCallback(typeCONFIG, levelInfo, Dest);
    }
#else
    TRACE_EID_TRACE_CONFIG_CHANGE(NULL, ClassName, Context, ParameterName, OldValue, NewValue);
#endif
}
