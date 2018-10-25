// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"
#include "windef.h"
#include "winnt.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WINADVAPI extern "C"
#define WINBASEAPI extern "C"

#define SecureZeroMemory RtlSecureZeroMemory

#define FILE_FLAG_WRITE_THROUGH         0x80000000
#define FILE_FLAG_OVERLAPPED            0x40000000
#define FILE_FLAG_NO_BUFFERING          0x20000000
#define FILE_FLAG_RANDOM_ACCESS         0x10000000
#define FILE_FLAG_SEQUENTIAL_SCAN       0x08000000
#define FILE_FLAG_DELETE_ON_CLOSE       0x04000000
#define FILE_FLAG_BACKUP_SEMANTICS      0x02000000
#define FILE_FLAG_POSIX_SEMANTICS       0x01000000
#define FILE_FLAG_SESSION_AWARE         0x00800000
#define FILE_FLAG_OPEN_REPARSE_POINT    0x00200000
#define FILE_FLAG_OPEN_NO_RECALL        0x00100000
#define FILE_FLAG_FIRST_PIPE_INSTANCE   0x00080000

#define MOVEFILE_REPLACE_EXISTING       0x00000001
#define MOVEFILE_COPY_ALLOWED           0x00000002
#define MOVEFILE_DELAY_UNTIL_REBOOT     0x00000004
#define MOVEFILE_WRITE_THROUGH          0x00000008

// 108
//#define CaptureStackBackTrace RtlCaptureStackBackTrace
NTSYSAPI
WORD
NTAPI
CaptureStackBackTrace(
    __in DWORD FramesToSkip,
    __in DWORD FramesToCapture,
    __out_ecount(FramesToCapture) PVOID *BackTrace,
    __out_opt PDWORD BackTraceHash
   );

// 846
#define DETACHED_PROCESS                  0x00000008

#define CREATE_NEW_PROCESS_GROUP          0x00000200
#define CREATE_UNICODE_ENVIRONMENT        0x00000400

#define CREATE_BREAKAWAY_FROM_JOB         0x01000000

#define CREATE_NO_WINDOW                  0x08000000

// 1264
#define _InterlockedIncrement InterlockedIncrement
#define _InterlockedDecrement InterlockedDecrement
#define _InterlockedCompareExchange InterlockedCompareExchange

// 1029
LONG
InterlockedXor (
    _Inout_ LONG volatile *Destination,
    _In_ LONG Value
    );

LONG64
InterlockedAnd64 (
    _Inout_ LONG64 volatile *Destination,
    _In_ LONG64 Value
    );

LONG64
InterlockedOr64 (
    _Inout_ LONG64 volatile *Destination,
    _In_ LONG64 Value
    );

LONG64
InterlockedXor64 (
    _Inout_ LONG64 volatile *Destination,
    _In_ LONG64 Value
    );

FORCEINLINE
unsigned __int64
InterlockedExchangeSubtract(
    _Inout_ _Interlocked_operand_ unsigned __int64 volatile *Addend,
    _In_ unsigned __int64 Value
    )
{
    return (unsigned __int64) InterlockedExchangeAdd64((volatile __int64*) Addend,  - (__int64) Value);
}

// 2515
WINBASEAPI
VOID
WINAPI
RaiseFailFastException(
    _In_opt_ PEXCEPTION_RECORD pExceptionRecord,
    _In_opt_ PCONTEXT pContextRecord,
    _In_ DWORD dwFlags
    );

// 3666
WINBASEAPI
DWORD
WINAPI
GetProcessId(
    __in HANDLE Process
    );

// 5113
WINBASEAPI
BOOL
WINAPI
SystemTimeToFileTime(
    __in  CONST SYSTEMTIME *lpSystemTime,
    __out LPFILETIME lpFileTime
    );

WINBASEAPI
BOOL
WINAPI
FileTimeToLocalFileTime(
    __in  CONST FILETIME *lpFileTime,
    __out LPFILETIME lpLocalFileTime
    );

// 5206
WINBASEAPI
DWORD
WINAPI
FormatMessageA(
    __in     DWORD dwFlags,
    __in_opt LPCVOID lpSource,
    __in     DWORD dwMessageId,
    __in     DWORD dwLanguageId,
    __out    LPSTR lpBuffer,
    __in     DWORD nSize,
    __in_opt va_list *Arguments
    );
WINBASEAPI
DWORD
WINAPI
FormatMessageW(
    __in     DWORD dwFlags,
    __in_opt LPCVOID lpSource,
    __in     DWORD dwMessageId,
    __in     DWORD dwLanguageId,
    __out    LPWSTR lpBuffer,
    __in     DWORD nSize,
    __in_opt va_list *Arguments
    );

// 7105
WINBASEAPI
__success(return <= nSize)
__success(return != 0)
DWORD
WINAPI
ExpandEnvironmentStringsA(
    __in LPCSTR lpSrc,
    __out_ecount_part_opt(nSize, return) LPSTR lpDst,
    __in DWORD nSize
    );
WINBASEAPI
__success(return <= nSize)
__success(return != 0)
DWORD
WINAPI
ExpandEnvironmentStringsW(
    __in LPCWSTR lpSrc,
    __out_ecount_part_opt(nSize, return) LPWSTR lpDst,
    __in DWORD nSize
    );
#ifdef UNICODE
#define ExpandEnvironmentStrings  ExpandEnvironmentStringsW
#else
#define ExpandEnvironmentStrings  ExpandEnvironmentStringsA
#endif // !UNICODE

// 10559
WINADVAPI
BOOL
WINAPI
IsWellKnownSid (
    __in PSID pSid,
    __in WELL_KNOWN_SID_TYPE WellKnownSidType
    );

// 10567
WINADVAPI
BOOL
WINAPI
CreateWellKnownSid(
    __in     WELL_KNOWN_SID_TYPE WellKnownSidType,
    __in_opt PSID DomainSid,
    __out_bcount_part_opt(*cbSid, *cbSid) PSID pSid,
    __inout  DWORD *cbSid
    );

// 11780
typedef enum _COMPUTER_NAME_FORMAT {
    ComputerNameNetBIOS,
    ComputerNameDnsHostname,
    ComputerNameDnsDomain,
    ComputerNameDnsFullyQualified,
    ComputerNamePhysicalNetBIOS,
    ComputerNamePhysicalDnsHostname,
    ComputerNamePhysicalDnsDomain,
    ComputerNamePhysicalDnsFullyQualified,
    ComputerNameMax
} COMPUTER_NAME_FORMAT ;

// 11796
WINBASEAPI
__success(return != 0)
BOOL
WINAPI
GetComputerNameExA (
    __in    COMPUTER_NAME_FORMAT NameType,
    __out_ecount_part_opt(*nSize, *nSize + 1) LPSTR lpBuffer,
    __inout LPDWORD nSize
    );
WINBASEAPI
__success(return != 0)
BOOL
WINAPI
GetComputerNameExW (
    __in    COMPUTER_NAME_FORMAT NameType,
    __out_ecount_part_opt(*nSize, *nSize + 1) LPWSTR lpBuffer,
    __inout LPDWORD nSize
    );
#ifdef UNICODE
#define GetComputerNameEx  GetComputerNameExW
#else
#define GetComputerNameEx  GetComputerNameExA
#endif // !UNICODE

WINADVAPI
BOOL
WINAPI
GetUserNameA (
    __out_ecount_part_opt(*pcbBuffer, *pcbBuffer) LPSTR lpBuffer,
    __inout LPDWORD pcbBuffer
    );
WINADVAPI
BOOL
WINAPI
GetUserNameW (
    __out_ecount_part_opt(*pcbBuffer, *pcbBuffer) LPWSTR lpBuffer,
    __inout LPDWORD pcbBuffer
    );
#ifdef UNICODE
#define GetUserName  GetUserNameW
#else
#define GetUserName  GetUserNameA
#endif // !UNICODE

// 11893
#define LOGON32_LOGON_NETWORK_CLEARTEXT 8

#define LOGON32_PROVIDER_DEFAULT    0

// 12281
WINBASEAPI
VOID
WINAPI
SetThreadpoolThreadMaximum(
    __inout PTP_POOL ptpp,
    __in    DWORD    cthrdMost
    );

// 12298
WINBASEAPI
BOOL
WINAPI
SetThreadpoolThreadMinimum(
    __inout PTP_POOL ptpp,
    __in    DWORD    cthrdMic
    );

// 12306
WINBASEAPI
VOID
WINAPI
CloseThreadpool(
    __inout PTP_POOL ptpp
    );

// 12465
WINBASEAPI
__checkReturn
__out
PTP_WORK
WINAPI
CreateThreadpoolWork(
    __in        PTP_WORK_CALLBACK    pfnwk,
    __inout_opt PVOID                pv,
    __in_opt    PTP_CALLBACK_ENVIRON pcbe
    );

WINBASEAPI
VOID
WINAPI
SubmitThreadpoolWork(
    __inout PTP_WORK pwk
    );

WINBASEAPI
VOID
WINAPI
WaitForThreadpoolWorkCallbacks(
    __inout PTP_WORK pwk,
    __in    BOOL     fCancelPendingCallbacks
    );

WINBASEAPI
VOID
WINAPI
CloseThreadpoolWork(
    __inout PTP_WORK pwk
    );

// 13022
WINBASEAPI
BOOL
WINAPI
TerminateJobObject(
    __in HANDLE hJob,
    __in UINT uExitCode
    );

// 13939
#define SYMBOLIC_LINK_FLAG_DIRECTORY            (0x1)

// 13944
BOOLEAN
APIENTRY
CreateSymbolicLinkA (
    __in LPCSTR lpSymlinkFileName,
    __in LPCSTR lpTargetFileName,
    __in DWORD dwFlags
    );
BOOLEAN
APIENTRY
CreateSymbolicLinkW (
    __in LPCWSTR lpSymlinkFileName,
    __in LPCWSTR lpTargetFileName,
    __in DWORD dwFlags
    );
#ifdef UNICODE
#define CreateSymbolicLink  CreateSymbolicLinkW
#else
#define CreateSymbolicLink  CreateSymbolicLinkA
#endif // !UNICODE

#ifdef __cplusplus
}
#endif
