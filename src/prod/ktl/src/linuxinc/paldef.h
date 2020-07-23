/*++

    (c) 2017 by Microsoft Corp. All Rights Reserved.

    paldef.h

    Description:
        Platform abstraction layer for linux

    History:

--*/

#pragma once

#include "strsafe.h"


//
// Map RTL functions to KTL equivalents
//
#define RtlCopyMemory KtlCopyMemory
#define RtlInitUnicodeString KtlInitUnicodeString
#define RtlCopyUnicodeString KtlCopyUnicodeString
#define RtlFreeUnicodeString KtlFreeUnicodeString
#define RtlCompareUnicodeStrings KtlCompareUnicodeStrings
#define RtlCompareUnicodeString KtlCompareUnicodeString
#define RtlAppendUnicodeStringToString KtlAppendUnicodeStringToString
#define RtlAnsiStringToUnicodeString   KtlAnsiStringToUnicodeString
#define RtlGUIDFromString KtlGUIDFromString
#define RtlStringFromGUIDEx  KtlStringFromGUIDEx
#define RtlExtendedLargeIntegerDivide KtlExtendedLargeIntegerDivide
#define RtlLargeIntegerToUnicode KtlLargeIntegerToUnicode
#define RtlLargeIntegerToChar KtlLargeIntegerToChar
#define RtlInt64ToUnicodeString KtlInt64ToUnicodeString
#define RtlStringFromGUID KtlStringFromGUID


//
// Map string safe functions to ktl
//
#define StringCchLengthW KtlStringCchLengthW
#define StringCchLengthA KtlStringCchLengthA
#define StringCopyWorkerW KtlStringCopyWorkerW
#define StringCopyWorkerA KtlStringCopyWorkerA
#define StringVPrintfWorkerW KtlStringVPrintfWorkerW
#define StringCbPrintfW KtlStringCbPrintfW
#define StringVPrintfWorkerA KtlStringVPrintfWorkerA
#define StringCchVPrintfA KtlStringCchVPrintfA
#define StringCchPrintfA KtlStringCchPrintfA
#define StringLengthWorkerW KtlStringLengthWorkerW
#define StringLengthWorkerA KtlStringLengthWorkerA
#define StringCbLengthA KtlStringCbLengthA
#define StringCbLengthW KtlStringCbLengthW
#define StringCchVPrintfW KtlStringCchVPrintfW
#define StringCchCopyW KtlStringCchCopyW
#define StringCchCopyA KtlStringCchCopyA


//#define __int2c __Ktlint2c
//#define SetEvent KtlSetEvent
//#define StringCchCopyW KtlStringCchCopyW

#define utf8to16 Ktlutf8to16
#define utf8to16 Ktlutf8to16
#define utf16to8 Ktlutf16to8
#define CreateEventW KtlCreateEventW
#define EventRegister KtlEventRegister
#define EventDataDescCreate KtlEventDataDescCreate
#define EventWrite KtlEventWrite
#define ResetEvent KtlResetEvent
#define WaitForSingleObject KtlWaitForSingleObject
#define NtQuerySystemInformation KtlNtQuerySystemInformation
#define CloseHandle KtlCloseHandle
#define HeapDestroy KtlHeapDestroy
#define VirtualAlloc KtlVirtualAlloc
#define VirtualFree KtlVirtualFree
#define RtlCompareMemory KtlRtlCompareMemory
#define RtlMoveMemory KtlRtlMoveMemory
#define RtlZeroMemory KtlRtlZeroMemory
#define InitializeSRWLock KtlInitializeSRWLock
#define AcquireSRWLockExclusive KtlAcquireSRWLockExclusive
#define TryAcquireSRWLockExclusive KtlTryAcquireSRWLockExclusive
#define AcquireSRWLockShared KtlAcquireSRWLockShared
#define TryAcquireSRWLockShared KtlTryAcquireSRWLockShared
#define ReleaseSRWLockExclusive KtlReleaseSRWLockExclusive
#define ReleaseSRWLockShared KtlReleaseSRWLockShared
#define GetProcessHeap KtlGetProcessHeap
#define SetLastError KtlSetLastError
#define GetCurrentThreadId KtlGetCurrentThreadId
#define HeapCreate KtlHeapCreate
#define DbgPrintEx KtlDbgPrintEx
#define HeapAlloc KtlHeapAlloc
#define HeapFree KtlHeapFree
#define EventEnabled KtlEventEnabled
#define memcpy_s Ktlmemcpy_s
#define GetModuleFileNameW KtlGetModuleFileNameW
#define CreateThread KtlCreateThread
#define InitializeThreadpoolEnvironment KtlInitializeThreadpoolEnvironment
#define DestroyThreadpoolEnvironment KtlDestroyThreadpoolEnvironment
#define SetThreadpoolCallbackPool KtlSetThreadpoolCallbackPool
#define CreateThreadpool KtlCreateThreadpool
#define CloseThreadpool KtlCloseThreadpool
#define SetThreadpoolThreadMinimum KtlSetThreadpoolThreadMinimum
#define SetThreadpoolThreadMaximum KtlSetThreadpoolThreadMaximum
#define CreateThreadpoolWork KtlCreateThreadpoolWork
#define CloseThreadpoolWork KtlCloseThreadpoolWork
#define SubmitThreadpoolWork KtlSubmitThreadpoolWork
#define WaitForThreadpoolWorkCallbacks KtlWaitForThreadpoolWorkCallbacks
#define GetLastError KtlGetLastError
#define DebugBreak KtlDebugBreak
#define GetCurrentProcess KtlGetCurrentProcess
#define GetCurrentProcessorNumber KtlGetCurrentProcessorNumber
#define GetProcessAffinityMask KtlGetProcessAffinityMask
#define OpenThread KtlOpenThread
#define SetThreadAffinityMask KtlSetThreadAffinityMask
#define SetThreadPriority KtlSetThreadPriority
#define SleepEx KtlSleepEx
#define ORClearBits KtlORClearBits
#define ORSetBits KtlORSetBits
#define RtlMultiByteToUnicodeSize KtlRtlMultiByteToUnicodeSize
#define RtlMultiByteToUnicodeN KtlRtlMultiByteToUnicodeN
#define UuidCreate KtlUuidCreate
#define RtlExtendedMagicDivide KtlRtlExtendedMagicDivide
#define RtlExtendedIntegerMultiply KtlRtlExtendedIntegerMultiply
#define ElapsedDaysToYears KtlElapsedDaysToYears
#define TimeToDaysAndFraction KtlTimeToDaysAndFraction
#define RtlTimeToTimeFields KtlRtlTimeToTimeFields
#define DaysAndFractionToTime KtlDaysAndFractionToTime
#define RtlTimeFieldsToTime KtlRtlTimeFieldsToTime
#define FileTimeToSystemTime KtlFileTimeToSystemTime
#define FILEUnixTimeToFileTime KtlFILEUnixTimeToFileTime
#define GetSystemTimeAsFileTime KtlGetSystemTimeAsFileTime
#define RtlMultiByteToUnicodeSize KtlRtlMultiByteToUnicodeSize
#define RtlMultiByteToUnicodeN KtlRtlMultiByteToUnicodeN
#define NtQueryVolumeInformationFile KtlNtQueryVolumeInformationFile
#define NtOpenDirectoryObject KtlNtOpenDirectoryObject
#define NtOpenSymbolicLinkObject KtlNtOpenSymbolicLinkObject
#define NtQuerySymbolicLinkObject KtlNtQuerySymbolicLinkObject
#define NtQueryDirectoryObject KtlNtQueryDirectoryObject
#define QueryPerformanceFrequency KtlQueryPerformanceFrequency
#define QueryPerformanceCounter KtlQueryPerformanceCounter
#define PerfCreateInstance KtlPerfCreateInstance
#define PerfSetCounterRefValue KtlPerfSetCounterRefValue
#define PerfDeleteInstance KtlPerfDeleteInstance
#define PerfSetCounterSetInfo KtlPerfSetCounterSetInfo
#define PerfStartProviderEx KtlPerfStartProviderEx
#define PerfStopProvider KtlPerfStopProvider
#define GetSystemInfo KtlGetSystemInfo
#define RtlCreateSecurityDescriptor KtlRtlCreateSecurityDescriptor
#define RtlLengthSid KtlRtlLengthSid
#define RtlCreateAcl KtlRtlCreateAcl
#define RtlAddAccessAllowedAce KtlRtlAddAccessAllowedAce
#define RtlSetDaclSecurityDescriptor KtlRtlSetDaclSecurityDescriptor
#define GetCurrentThread KtlGetCurrentThread
#define OpenThreadToken KtlOpenThreadToken
#define OpenProcessToken KtlOpenProcessToken
#define DuplicateTokenEx KtlDuplicateTokenEx
#define SetThreadToken KtlSetThreadToken
#define AdjustTokenPrivileges KtlAdjustTokenPrivileges
#define WriteFile KtlWriteFile
#define ReadFile KtlReadFile
#define RegOpenKeyExW KtlRegOpenKeyExW
#define RegCloseKey KtlRegCloseKey
#define RegQueryValueExW KtlRegQueryValueExW
#define RegSetValueExW KtlRegSetValueExW
#define RegDeleteValueW KtlRegDeleteValueW
#define SetThreadpoolTimer KtlSetThreadpoolTimer
#define CloseThreadpoolTimer KtlCloseThreadpoolTimer
#define CreateThreadpoolTimer KtlCreateThreadpoolTimer
#define WaitForThreadpoolTimerCallbacks KtlWaitForThreadpoolTimerCallbacks
#define RtlStringCchCatW KtlRtlStringCchCatW
#define RtlStringCchLengthW KtlRtlStringCchLengthW
#define RtlStringCchCopyW KtlRtlStringCchCopyW
#define GetEnvironmentVariableA KtlGetEnvironmentVariableA
#define GetEnvironmentVariableW KtlGetEnvironmentVariableW
#define GetEnvironmentVariableW KtlGetEnvironmentVariableW
#define GetEnvironmentVariableW KtlGetEnvironmentVariableW
#define ExpandEnvironmentStringsA KtlExpandEnvironmentStringsA
#define ExpandEnvironmentStringsW KtlExpandEnvironmentStringsW
#define RtlRandomEx KtlRtlRandomEx
#define VirtualProtect KtlVirtualProtect
#define WaitForMultipleObjects KtlWaitForMultipleObjects
#define SymInitialize KtlSymInitialize
#define CaptureStackBackTrace KtlCaptureStackBackTrace
#define SymFromAddr KtlSymFromAddr
#define SymGetLineFromAddr64 KtlSymGetLineFromAddr64
#define NtCreateFile KtlNtCreateFile
#define NtDeleteFile KtlNtDeleteFile
#define NtClose KtlNtClose
#define NtOpenFile KtlNtOpenFile
#define NtQueryInformationFile KtlNtQueryInformationFile
#define NtQueryFullAttributesFile KtlNtQueryFullAttributesFile
#define NtSetInformationFile KtlNtSetInformationFile
#define NtFsControlFile KtlNtFsControlFile
#define NtDeviceIoControlFile KtlNtDeviceIoControlFile
#define NtWriteFile KtlNtWriteFile
#define NtReadFile KtlNtReadFile
#define NtFlushBuffersFile KtlNtFlushBuffersFile
#define NtWaitForSingleObject KtlNtWaitForSingleObject
#define NtQueryDirectoryFile KtlNtQueryDirectoryFile
#define NtCreateEvent KtlNtCreateEvent
#define WriteFileGather KtlWriteFileGather
#define ReadFileScatter KtlReadFileScatter

WINBASEAPI VOID WINAPI __int2c(VOID);
HANDLE CreateEventW(
        IN LPSECURITY_ATTRIBUTES lpEventAttributes,
        IN BOOL bManualReset,
        IN BOOL bInitialState,
        IN LPCWSTR lpName);
BOOL
PALAPI
SetEvent(
        IN HANDLE hEvent);
BOOL
PALAPI
ResetEvent(
        IN HANDLE hEvent);
ULONG EVNTAPI EventRegister(
        IN LPCGUID ProviderId,
        PENABLECALLBACK EnableCallback,
        PVOID CallbackContext,
        OUT PREGHANDLE RegHandle
);
VOID EventDataDescCreate(
        PEVENT_DATA_DESCRIPTOR EventDataDescriptor,
        VOID const* DataPtr,
        ULONG DataSize
);
ULONG EVNTAPI EventWrite(
        _In_ REGHANDLE RegHandle,
        _In_ PCEVENT_DESCRIPTOR EventDescriptor,
        _In_ ULONG UserDataCount,
        _In_reads_opt_(UserDataCount) PEVENT_DATA_DESCRIPTOR UserData
);
PALIMPORT
DWORD
PALAPI
WaitForSingleObject(
        IN HANDLE hHandle,
        IN DWORD dwMilliseconds);
NTSTATUS
NTAPI
NtQuerySystemInformation(
        _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
        _Out_ PVOID SystemInformation,
        _In_ ULONG SystemInformationLength,
        _Out_ PULONG ReturnLength
);
PALIMPORT
BOOL
PALAPI
CloseHandle(
        IN OUT HANDLE hObject);
BOOL HeapDestroy(_In_ HANDLE hHeap);
PALIMPORT
LPVOID
PALAPI
VirtualAlloc(
        IN LPVOID lpAddress,
        IN SIZE_T dwSize,
        IN DWORD flAllocationType,
        IN DWORD flProtect);

PALIMPORT
BOOL
PALAPI
VirtualFree(
        IN LPVOID lpAddress,
        IN SIZE_T dwSize,
        IN DWORD dwFreeType);
SIZE_T
NTAPI
RtlCompareMemory(
        _In_ const VOID * Source1,
        _In_ const VOID * Source2,
        _In_ SIZE_T Length
);
PALIMPORT
VOID
PALAPI
RtlMoveMemory(
        IN PVOID Destination,
        IN CONST VOID *Source,
        IN SIZE_T Length);
PALIMPORT
VOID
PALAPI
RtlZeroMemory(
        IN PVOID Destination,
        IN SIZE_T Length);
WINBASEAPI
VOID
WINAPI
InitializeSRWLock(
        OUT PSRWLOCK SRWLock
);
WINBASEAPI
VOID
WINAPI
AcquireSRWLockExclusive(
        PSRWLOCK SRWLock
);
WINBASEAPI
BOOLEAN
WINAPI
TryAcquireSRWLockExclusive(
        PSRWLOCK SRWLock
);
WINBASEAPI
VOID
WINAPI
AcquireSRWLockShared(
        PSRWLOCK SRWLock
);
WINBASEAPI
BOOLEAN
WINAPI
TryAcquireSRWLockShared(
        PSRWLOCK SRWLock
);
WINBASEAPI
VOID
WINAPI
ReleaseSRWLockExclusive(
        IN OUT PSRWLOCK SRWLock
);
WINBASEAPI
VOID
WINAPI
ReleaseSRWLockShared(
        IN OUT PSRWLOCK SRWLock
);
PALIMPORT
HANDLE
PALAPI
GetProcessHeap(VOID);
PALIMPORT
VOID
PALAPI
SetLastError(
        IN DWORD dwErrCode);
PALIMPORT
DWORD
PALAPI
GetCurrentThreadId(
        VOID);
VOID WINAPI ExitProcess(
        _In_ UINT uExitCode
);
WINBASEAPI
HANDLE
WINAPI
HeapCreate(DWORD flOptions, SIZE_T dwInitialSize, SIZE_T dwMaximumSize);
ULONG
DbgPrintEx (
        _In_ ULONG ComponentId,
        _In_ ULONG Level,
        _In_ PCSTR Format,
        ...);
PALIMPORT
LPVOID
PALAPI
HeapAlloc(
        IN HANDLE hHeap,
        IN DWORD dwFlags,
        IN SIZE_T dwBytes);
PALIMPORT
BOOL
PALAPI
HeapFree(
        IN HANDLE hHeap,
        IN DWORD dwFlags,
        IN LPVOID lpMem);
extern "C"
BOOL EventEnabled(
        _In_ REGHANDLE RegHandle,
        _In_ PCEVENT_DESCRIPTOR EventDescriptor
);
WINBASEAPI HRESULT StringCchLengthW(
        _In_  LPCWSTR psz,
        _In_  size_t  cchMax,
        _Out_ size_t  *pcch
);
STRSAFEAPI
StringCchCopyW(
        _Out_writes_(cchDest) _Always_(_Post_z_) STRSAFE_LPWSTR pszDest,
        _In_ size_t cchDest,
        _In_ STRSAFE_LPCWSTR pszSrc);

WINBASEAPI HRESULT StringCchLengthA(
        _In_  LPCSTR psz,
        _In_  size_t  cchMax,
        _Out_ size_t  *pcch
);
STRSAFEAPI
StringCchCopyA(
        _Out_writes_(cchDest) _Always_(_Post_z_) STRSAFE_LPSTR pszDest,
        _In_ size_t cchDest,
        _In_ STRSAFE_LPCSTR pszSrc);


STRSAFEAPI
StringCbPrintfW(WCHAR* pszDest, size_t cbDest, const WCHAR* pszFormat, ...);
errno_t memcpy_s(void *dest, size_t destsz, const void *src, size_t count);
PALIMPORT
DWORD
PALAPI
GetModuleFileNameW(
        IN HMODULE hModule,
        OUT LPWSTR lpFileName,
        IN DWORD nSize);
PALIMPORT
HANDLE
PALAPI
CreateThread(
        IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
        IN DWORD dwStackSize,
        IN LPTHREAD_START_ROUTINE lpStartAddress,
        IN LPVOID lpParameter,
        IN DWORD dwCreationFlags,
        OUT LPDWORD lpThreadId);

PALIMPORT VOID PALAPI InitializeThreadpoolEnvironment(PTP_CALLBACK_ENVIRON pcbe);
PALIMPORT VOID PALAPI DestroyThreadpoolEnvironment(PTP_CALLBACK_ENVIRON pcbe);
PALIMPORT VOID PALAPI SetThreadpoolCallbackPool(PTP_CALLBACK_ENVIRON pcbe, PTP_POOL ptpp);
PALIMPORT PTP_POOL PALAPI CreateThreadpool(PVOID reserved);
PALIMPORT VOID PALAPI CloseThreadpool(PTP_POOL ptpp);
PALIMPORT BOOL PALAPI SetThreadpoolThreadMinimum(PTP_POOL ptpp, DWORD cthrdMic);
PALIMPORT VOID PALAPI SetThreadpoolThreadMaximum(PTP_POOL ptpp, DWORD cthrdMost);
PALIMPORT PTP_WORK PALAPI CreateThreadpoolWork(PTP_WORK_CALLBACK pfnwk, PVOID pv, PTP_CALLBACK_ENVIRON pcbe);
PALIMPORT VOID PALAPI CloseThreadpoolWork(PTP_WORK pwk);
PALIMPORT VOID PALAPI SubmitThreadpoolWork(PTP_WORK pwk);
PALIMPORT VOID PALAPI WaitForThreadpoolWorkCallbacks(PTP_WORK pwk, BOOL fCancelPendingCallbacks);

PALIMPORT
DWORD
PALAPI
GetLastError(
        VOID);
PALIMPORT
VOID
PALAPI
DebugBreak(
        VOID);

STRSAFEAPI
StringCchPrintfA(
            _Out_writes_(cchDest) _Always_(_Post_z_) STRSAFE_LPSTR pszDest,
            _In_ size_t cchDest,
            _In_ _Printf_format_string_ STRSAFE_LPCSTR pszFormat,
            ...);

STRSAFEAPI
        StringCchVPrintfA(
        _Out_writes_(cchDest) _Always_(_Post_z_) STRSAFE_LPSTR pszDest,
        _In_ size_t cchDest,
        _In_ _Printf_format_string_ STRSAFE_LPCSTR pszFormat,
        _In_ va_list argList);
HANDLE GetCurrentProcess();
ULONG
GetCurrentProcessorNumber (
        VOID
);
BOOL
WINAPI
GetProcessAffinityMask(
        HANDLE hProcess,
        PDWORD_PTR lpProcessAffinityMask,
        PDWORD_PTR lpSystemAffinityMask
);
HANDLE
WINAPI
OpenThread(
        __in DWORD dwDesiredAccess,
        __in BOOL bInheritHandle,
        __in DWORD dwThreadId
);
DWORD_PTR
WINAPI
SetThreadAffinityMask(
        __in HANDLE hThread,
        __in DWORD_PTR dwThreadAffinityMask
);
BOOL
__stdcall
SetThreadPriority(
        _In_ HANDLE hThread,
        _In_ int nPriority
);
DWORD
APIENTRY
SleepEx(
        __in DWORD dwMilliseconds,
        __in BOOL bAlertable
);
VOID
ORClearBits (
        __in PRTL_BITMAP BitMapHeader,
        __in BIT_NUMBER StartingIndex,
        __in BIT_NUMBER NumberToClear
);
VOID
ORSetBits (
        __in PRTL_BITMAP BitMapHeader,
        __in BIT_NUMBER StartingIndex,
        __in BIT_NUMBER NumberToSet
);
ULONG
RtlNumberOfSetBitsUlongPtr (
        __in ULONG_PTR Target
);
BIT_NUMBER
RtlNumberOfSetBits (
        _In_ PRTL_BITMAP BitMapHeader
);
BOOLEAN
RtlAreBitsClear (
        IN PRTL_BITMAP BitMapHeader,
        IN BIT_NUMBER StartingIndex,
        IN BIT_NUMBER Length
);
BOOLEAN
RtlAreBitsSet (
        IN PRTL_BITMAP BitMapHeader,
        IN BIT_NUMBER StartingIndex,
        IN BIT_NUMBER Length
);
RPC_STATUS
RPC_ENTRY
UuidCreate (
        OUT ::GUID __RPC_FAR * Uuid
);
LARGE_INTEGER
RtlExtendedMagicDivide (
        _In_ LARGE_INTEGER Dividend,
        _In_ LARGE_INTEGER MagicDivisor,
        _In_ CCHAR ShiftCount
);
LARGE_INTEGER
RtlExtendedIntegerMultiply (
        _In_ LARGE_INTEGER Multiplicand,
        _In_ LONG Multiplier
);
ULONG
ElapsedDaysToYears (
        __in ULONG ElapsedDays
);
VOID
TimeToDaysAndFraction (
        __in PLARGE_INTEGER Time,
        __out PULONG ElapsedDays,
        __out PULONG Milliseconds
);
VOID
RtlTimeToTimeFields (
        __in PLARGE_INTEGER Time,
        __out PTIME_FIELDS TimeFields
);
VOID
DaysAndFractionToTime (
        __in ULONG ElapsedDays,
        __in ULONG Milliseconds,
        __out PLARGE_INTEGER Time
);
BOOLEAN
RtlTimeFieldsToTime (
        __in PTIME_FIELDS TimeFields,
        __out PLARGE_INTEGER Time
);
BOOL
WINAPI
FileTimeToSystemTime(
        __in  CONST FILETIME *lpFileTime,
        __out LPSYSTEMTIME lpSystemTime
);
FILETIME FILEUnixTimeToFileTime( time_t sec, long nsec );
VOID
GetSystemTimeAsFileTime(
        OUT LPFILETIME lpSystemTimeAsFileTime);
STRSAFEAPI
StringCbLengthA(
        _In_reads_or_z_(cbMax) STRSAFE_PCNZCH psz,
        _In_ _In_range_(1, STRSAFE_MAX_CCH * sizeof(char);) size_t cbMax,
        _Out_opt_ _Deref_out_range_(<, cbMax) size_t* pcbLength);
STRSAFEAPI
StringCbLengthW(
        _In_reads_or_z_(cbMax / sizeof(wchar_t)) STRSAFE_PCNZWCH psz,
        _In_ _In_range_(1, STRSAFE_MAX_CCH * sizeof(wchar_t)) size_t cbMax,
        _Out_opt_ _Deref_out_range_(<, cbMax - 1) size_t* pcbLength);
STRSAFEAPI
        StringCchVPrintfW(
        __out_ecount(cchDest) STRSAFE_LPWSTR pszDest,
        __in size_t cchDest,
        __in __format_string STRSAFE_LPCWSTR pszFormat,
        __in va_list argList);
NTSTATUS RtlMultiByteToUnicodeSize(
        _Out_       PULONG BytesInUnicodeString,
        _In_  const CHAR   *MultiByteString,
        _In_        ULONG  BytesInMultiByteString
);
NTSTATUS
RtlMultiByteToUnicodeN(
        OUT PWCH UnicodeString,
        IN ULONG MaxBytesInUnicodeString,
        OUT PULONG BytesInUnicodeString OPTIONAL,
        IN PCSTR MultiByteString,
        IN ULONG BytesInMultiByteString);
NTSTATUS NtQueryVolumeInformationFile(
        _In_  HANDLE               FileHandle,
        _Out_ PIO_STATUS_BLOCK     IoStatusBlock,
        _Out_ PVOID                FsInformation,
        _In_  ULONG                Length,
        _In_  FS_INFORMATION_CLASS FsInformationClass
);
NTSTATUS WINAPI NtOpenDirectoryObject(
        _Out_ PHANDLE            DirectoryHandle,
        _In_  ACCESS_MASK        DesiredAccess,
        _In_  POBJECT_ATTRIBUTES ObjectAttributes
);
NTSTATUS WINAPI NtOpenSymbolicLinkObject(
        _Out_ PHANDLE            LinkHandle,
        _In_  ACCESS_MASK        DesiredAccess,
        _In_  POBJECT_ATTRIBUTES ObjectAttributes
);
NTSTATUS WINAPI NtQuerySymbolicLinkObject(
        _In_      HANDLE          LinkHandle,
        _Inout_   PUNICODE_STRING LinkTarget,
        _Out_opt_ PULONG          ReturnedLength
);
NTSTATUS WINAPI NtQueryDirectoryObject(
        _In_      HANDLE  DirectoryHandle,
        _Out_opt_ PVOID   Buffer,
        _In_      ULONG   Length,
        _In_      BOOLEAN ReturnSingleEntry,
        _In_      BOOLEAN RestartScan,
        _Inout_   PULONG  Context,
        _Out_opt_ PULONG  ReturnLength
);
BOOL WINAPI QueryPerformanceFrequency(
        _Out_ LARGE_INTEGER *lpFrequency
);
BOOL WINAPI QueryPerformanceCounter(
        _Out_ LARGE_INTEGER *lpPerformanceCount
);
PPERF_COUNTERSET_INSTANCE PerfCreateInstance(
        _In_ HANDLE  hProvider,
        _In_ LPCGUID CounterSetGuid,
        _In_ LPCWSTR szInstanceName,
        _In_ ULONG   dwInstance
);
ULONG PerfSetCounterRefValue(
        _In_ HANDLE                    hProvider,
        _In_ PPERF_COUNTERSET_INSTANCE pInstance,
        _In_ ULONG                     CounterId,
        _In_ PVOID                     lpAddr
);
ULONG PerfSetCounterRefValue(
        _In_ HANDLE                    hProvider,
        _In_ PPERF_COUNTERSET_INSTANCE pInstance,
        _In_ ULONG                     CounterId,
        _In_ PVOID                     lpAddr
);
ULONG PerfDeleteInstance(
        _In_ HANDLE                    hProvider,
        _In_ PPERF_COUNTERSET_INSTANCE InstanceBlock
);
ULONG PerfSetCounterSetInfo(
        _In_ HANDLE                hProvider,
        _In_ PPERF_COUNTERSET_INFO pTemplate,
        _In_ ULONG                 dwTemplateSize
);
ULONG PerfStartProviderEx(
        _In_     LPGUID                 ProviderGuid,
        _In_opt_ PPERF_PROVIDER_CONTEXT ProviderContext,
        _Out_    HANDLE                 *phProvider
);
ULONG PerfStopProvider(
        _In_ HANDLE hProvider
);
void WINAPI GetSystemInfo(
        _Out_ LPSYSTEM_INFO lpSystemInfo
);
NTSTATUS RtlCreateSecurityDescriptor(
        _Out_ PSECURITY_DESCRIPTOR SecurityDescriptor,
        _In_  ULONG                Revision
);
ULONG RtlLengthSid(
        _In_ PSID Sid
);
NTSTATUS RtlCreateAcl(
        _Out_ PACL  Acl,
        _In_  ULONG AclLength,
        _In_  ULONG AceRevision
);
NTSTATUS RtlAddAccessAllowedAce(
        _Inout_ PACL        Acl,
        _In_    ULONG       AceRevision,
        _In_    ACCESS_MASK AccessMask,
        _In_    PSID        Sid
);
NTSTATUS RtlSetDaclSecurityDescriptor(
        _Inout_  PSECURITY_DESCRIPTOR SecurityDescriptor,
        _In_     BOOLEAN              DaclPresent,
        _In_opt_ PACL                 Dacl,
        _In_opt_ BOOLEAN              DaclDefaulted
);
HANDLE WINAPI GetCurrentThread(void);
BOOL WINAPI OpenThreadToken(
        _In_  HANDLE  ThreadHandle,
        _In_  DWORD   DesiredAccess,
        _In_  BOOL    OpenAsSelf,
        _Out_ PHANDLE TokenHandle
);
BOOL WINAPI OpenProcessToken(
        _In_  HANDLE  ProcessHandle,
        _In_  DWORD   DesiredAccess,
        _Out_ PHANDLE TokenHandle
);
BOOL WINAPI DuplicateTokenEx(
        _In_     HANDLE                       hExistingToken,
        _In_     DWORD                        dwDesiredAccess,
        _In_opt_ LPSECURITY_ATTRIBUTES        lpTokenAttributes,
        _In_     SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
        _In_     TOKEN_TYPE                   TokenType,
        _Out_    PHANDLE                      phNewToken
);
BOOL WINAPI SetThreadToken(
        _In_opt_ PHANDLE Thread,
        _In_opt_ HANDLE  Token
);
BOOL WINAPI AdjustTokenPrivileges(
        _In_      HANDLE            TokenHandle,
        _In_      BOOL              DisableAllPrivileges,
        _In_opt_  PTOKEN_PRIVILEGES NewState,
        _In_      DWORD             BufferLength,
        _Out_opt_ PTOKEN_PRIVILEGES PreviousState,
        _Out_opt_ PDWORD            ReturnLength
);
WINADVAPI
LSTATUS
APIENTRY
RegOpenKeyExW(
        HKEY hKey,
        LPCWSTR lpSubKey,
        DWORD ulOptions,
        REGSAM samDesired,
        PHKEY phkResult
);
WINADVAPI
LSTATUS
APIENTRY
RegCloseKey(
        _In_ HKEY hKey
);
WINADVAPI
LSTATUS
APIENTRY
RegQueryValueExW(
        _In_ HKEY hKey,
        _In_opt_ LPCWSTR lpValueName,
        _Reserved_ LPDWORD lpReserved,
        _Out_opt_ LPDWORD lpType,
        LPBYTE lpData,
        LPDWORD lpcbData
);
WINADVAPI
LSTATUS
APIENTRY
RegSetValueExW(
        HKEY hKey,
        LPCWSTR lpValueName,
        DWORD Reserved,
        DWORD dwType,
        const BYTE *lpData,
        DWORD cbData
);
WINADVAPI
LSTATUS
APIENTRY
RegDeleteValueW(
        _In_ HKEY hKey,
        _In_opt_ LPCWSTR lpValueName
);
WINBASEAPI
VOID
WINAPI
SetThreadpoolTimer(
        PTP_TIMER pti,
        PFILETIME pftDueTime,
        DWORD msPeriod,
        DWORD msWindowLength
);
WINBASEAPI
VOID
WINAPI
CloseThreadpoolTimer(
        PTP_TIMER pti
);
WINBASEAPI
PTP_TIMER
WINAPI
CreateThreadpoolTimer(
        PTP_TIMER_CALLBACK pfnti,
        PVOID pv,
        PTP_CALLBACK_ENVIRON pcbe
);
WINBASEAPI
VOID
WINAPI
WaitForThreadpoolTimerCallbacks(
        PTP_TIMER pti,
        BOOL fCancelPendingCallbacks
);
NTSTATUS
RtlStringCchCatW(
        __out_ecount(cchDest) STRSAFE_LPWSTR pszDest,
        __in size_t cchDest,
        __in STRSAFE_LPCWSTR pszSrc);
NTSTATUS
RtlStringCchLengthW(
        _In_      STRSAFE_PCNZWCH psz,
        _In_      size_t cchMax,
        _Out_opt_ size_t* pcchLength);
NTSTATUS
        RtlStringCchCopyW(
        _Out_writes_(cchDest) _Always_(_Post_z_) NTSTRSAFE_PWSTR pszDest,
        _In_ size_t cchDest,
        _In_ NTSTRSAFE_PCWSTR pszSrc);
DWORD GetEnvironmentVariableA(LPCSTR lpName, LPSTR lpBuffer, DWORD nSize);
DWORD GetEnvironmentVariableW(LPCWSTR lpName, LPWSTR lpBuffer, DWORD nSize);
WINBASEAPI
DWORD
WINAPI
ExpandEnvironmentStringsA(
        _In_ LPCSTR lpSrc,
        LPSTR lpDst,
        _In_ DWORD nSize);
WINBASEAPI
DWORD
WINAPI
ExpandEnvironmentStringsW(
        _In_ LPCWSTR lpSrc,
        LPWSTR lpDst,
        _In_ DWORD nSize);

ULONG RtlRandomEx(
        _Inout_ PULONG Seed
);
BOOL WINAPI VirtualProtect(
        _In_  LPVOID lpAddress,
        _In_  SIZE_T dwSize,
        _In_  DWORD  flNewProtect,
        _Out_ PDWORD lpflOldProtect
);
DWORD WINAPI WaitForMultipleObjects(
        _In_       DWORD  nCount,
        _In_ const HANDLE *lpHandles,
        _In_       BOOL   bWaitAll,
        _In_       DWORD  dwMilliseconds
);
BOOL
SymInitialize(
        _In_ HANDLE hProcess,
        _In_opt_ PCSTR UserSearchPath,
        _In_ BOOL fInvadeProcess
);
USHORT WINAPI CaptureStackBackTrace(
        _In_      ULONG  FramesToSkip,
        _In_      ULONG  FramesToCapture,
        _Out_     PVOID  *BackTrace,
        _Out_opt_ PULONG BackTraceHash
);
BOOL
SymFromAddr(
        __in HANDLE hProcess,
        __in DWORD64 Address,
        __out_opt PDWORD64 Displacement,
        __inout PSYMBOL_INFO Symbol
);
BOOL WINAPI SymGetLineFromAddr64(
        _In_  HANDLE           hProcess,
        _In_  DWORD64          dwAddr,
        _Out_ PDWORD           pdwDisplacement,
        _Out_ PIMAGEHLP_LINE64 Line
);

NTSTATUS NtCreateFile(
        _Out_    PHANDLE            FileHandle,
        _In_     ACCESS_MASK        DesiredAccess,
        _In_     POBJECT_ATTRIBUTES ObjectAttributes,
        _Out_    PIO_STATUS_BLOCK   IoStatusBlock,
        _In_opt_ PLARGE_INTEGER     AllocationSize,
        _In_     ULONG              FileAttributes,
        _In_     ULONG              ShareAccess,
        _In_     ULONG              CreateDisposition,
        _In_     ULONG              CreateOptions,
        _In_     PVOID              EaBuffer,
        _In_     ULONG              EaLength
);
NTSTATUS NtDeleteFile(
        _In_ POBJECT_ATTRIBUTES ObjectAttributes
);
NTSTATUS WINAPI NtClose(
        _In_ HANDLE Handle
);
NTSTATUS NtOpenFile(
        _Out_ PHANDLE            FileHandle,
        _In_  ACCESS_MASK        DesiredAccess,
        _In_  POBJECT_ATTRIBUTES ObjectAttributes,
        _Out_ PIO_STATUS_BLOCK   IoStatusBlock,
        _In_  ULONG              ShareAccess,
        _In_  ULONG              OpenOptions
);
NTSTATUS NtQueryInformationFile(
        _In_  HANDLE                 FileHandle,
        _Out_ PIO_STATUS_BLOCK       IoStatusBlock,
        _Out_ PVOID                  FileInformation,
        _In_  ULONG                  Length,
        _In_  FILE_INFORMATION_CLASS FileInformationClass
);
NTSTATUS NtQueryFullAttributesFile(
        _In_  POBJECT_ATTRIBUTES             ObjectAttributes,
        _Out_ PFILE_NETWORK_OPEN_INFORMATION FileInformation
);
NTSTATUS NtSetInformationFile(
        _In_  HANDLE                 FileHandle,
        _Out_ PIO_STATUS_BLOCK       IoStatusBlock,
        _In_  PVOID                  FileInformation,
        _In_  ULONG                  Length,
        _In_  FILE_INFORMATION_CLASS FileInformationClass
);
NTSTATUS NtFsControlFile(
        _In_      HANDLE           FileHandle,
        _In_opt_  HANDLE           Event,
        _In_opt_  PIO_APC_ROUTINE  ApcRoutine,
        _In_opt_  PVOID            ApcContext,
        _Out_     PIO_STATUS_BLOCK IoStatusBlock,
        _In_      ULONG            FsControlCode,
        _In_opt_  PVOID            InputBuffer,
        _In_      ULONG            InputBufferLength,
        _Out_opt_ PVOID            OutputBuffer,
        _In_      ULONG            OutputBufferLength
);
NTSTATUS WINAPI NtDeviceIoControlFile(
        _In_  HANDLE           FileHandle,
        _In_  HANDLE           Event,
        _In_  PIO_APC_ROUTINE  ApcRoutine,
        _In_  PVOID            ApcContext,
        _Out_ PIO_STATUS_BLOCK IoStatusBlock,
        _In_  ULONG            IoControlCode,
        _In_  PVOID            InputBuffer,
        _In_  ULONG            InputBufferLength,
        _Out_ PVOID            OutputBuffer,
        _In_  ULONG            OutputBufferLength
);
NTSTATUS NtWriteFile(
        _In_     HANDLE           FileHandle,
        _In_opt_ HANDLE           Event,
        _In_opt_ PIO_APC_ROUTINE  ApcRoutine,
        _In_opt_ PVOID            ApcContext,
        _Out_    PIO_STATUS_BLOCK IoStatusBlock,
        _In_     PVOID            Buffer,
        _In_     ULONG            Length,
        _In_opt_ PLARGE_INTEGER   ByteOffset,
        _In_opt_ PULONG           Key
);NTSTATUS NtReadFile(
        _In_     HANDLE           FileHandle,
        _In_opt_ HANDLE           Event,
        _In_opt_ PIO_APC_ROUTINE  ApcRoutine,
        _In_opt_ PVOID            ApcContext,
        _Out_    PIO_STATUS_BLOCK IoStatusBlock,
        _Out_    PVOID            Buffer,
        _In_     ULONG            Length,
        _In_opt_ PLARGE_INTEGER   ByteOffset,
        _In_opt_ PULONG           Key
);
NTSTATUS NtFlushBuffersFile(
        _In_  HANDLE           FileHandle,
        _Out_ PIO_STATUS_BLOCK IoStatusBlock
);
NTSTATUS NtWaitForSingleObject(
        _In_     HANDLE         Handle,
        _In_     BOOLEAN        Alertable,
        _In_opt_ PLARGE_INTEGER Timeout
);
NTSTATUS NtQueryDirectoryFile(
        _In_     HANDLE                 FileHandle,
        _In_opt_ HANDLE                 Event,
        _In_opt_ PIO_APC_ROUTINE        ApcRoutine,
        _In_opt_ PVOID                  ApcContext,
        _Out_    PIO_STATUS_BLOCK       IoStatusBlock,
        _Out_    PVOID                  FileInformation,
        _In_     ULONG                  Length,
        _In_     FILE_INFORMATION_CLASS FileInformationClass,
        _In_     BOOLEAN                ReturnSingleEntry,
        _In_opt_ PUNICODE_STRING        FileName,
        _In_     BOOLEAN                RestartScan
);
NTSTATUS NtCreateEvent(
        _Out_    PHANDLE            EventHandle,
        _In_     ACCESS_MASK        DesiredAccess,
        _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
        _In_     EVENT_TYPE         EventType,
        _In_     BOOLEAN            InitialState
);

#if 0  // TODO: Remove
typedef struct ALIGNEDBUFFER {
    void *buf;
    int npages;
} ALIGNED_BUFFER;

BOOL WINAPI WriteFileGather(
        _In_       HANDLE               hFile,
        _In_       ALIGNED_BUFFER       AlignedBuffers[],
        _In_       DWORD                nNumberOfBytesToWrite,
        _Reserved_ LPDWORD              lpReserved,
        _Inout_    LPOVERLAPPED         lpOverlapped
);
BOOL WINAPI ReadFileScatter(
        _In_       HANDLE               hFile,
        _In_       ALIGNED_BUFFER       AlignedBuffers[],
        _In_       DWORD                nNumberOfBytesToRead,
        _Reserved_ LPDWORD              lpReserved,
        _Inout_    LPOVERLAPPED         lpOverlapped
);
#endif


BOOL WINAPI WriteFile(
        _In_       HANDLE               hFile,
        _In_       LPVOID               lpBuffer,
        _In_       DWORD                nNumberOfBytesToWrite,
        _Out_opt_  LPDWORD              lpNumberBytesWriten,
        _Inout_    LPOVERLAPPED         lpOverlapped
);
BOOL WINAPI ReadFile(
        _In_       HANDLE               hFile,
        _In_       LPVOID               lpBuffer,
        _In_       DWORD                nNumberOfBytesToRead,
        _Out_opt_  LPDWORD              lpNumberBytesRead,
        _Inout_    LPOVERLAPPED         lpOverlapped
);


VOID
KtlInitUnicodeString (
        __out PUNICODE_STRING DestinationString,
        __in_opt PCWSTR SourceString
);
VOID
KtlCopyUnicodeString(
        __inout PUNICODE_STRING DestinationString,
        __in_opt PCUNICODE_STRING SourceString
);
VOID
KtlFreeUnicodeString (
        _Inout_ PUNICODE_STRING UnicodeString);
LONG
KtlCompareUnicodeStrings (
        __in_ecount(String1Length) PCWCH String1,
        __in SIZE_T String1Length,
        __in_ecount(String2Length) PCWCH String2,
        __in SIZE_T String2Length,
        __in BOOLEAN CaseInSensitive
);
LONG
KtlCompareUnicodeString (
        __in PCUNICODE_STRING String1,
        __in PCUNICODE_STRING String2,
        __in BOOLEAN CaseInSensitive
);
NTSTATUS
KtlAppendUnicodeStringToString (
        __inout PUNICODE_STRING Destination,
        __in PCUNICODE_STRING Source
);
NTSTATUS
KtlGUIDFromString(
        __in PCUNICODE_STRING GuidString,
        __out GUID* Guid
);

VOID
KtlCopyMemory (
        OUT VOID UNALIGNED *Destination,
        IN CONST VOID UNALIGNED * Sources,
        IN SIZE_T Length
);
NTSTATUS
KtlStringFromGUIDEx(
        __in REFGUID Guid,
        __inout PUNICODE_STRING GuidString,
        __in BOOLEAN Allocate
);
NTSTATUS
KtlStringFromGUID(
        __in REFGUID Guid,
        PUNICODE_STRING GuidString
);
NTSTATUS
KtlLargeIntegerToUnicode (
        IN PLARGE_INTEGER Value,
        IN ULONG Base OPTIONAL,
        IN LONG OutputLength,
        OUT PWSTR String
);
NTSTATUS
KtlLargeIntegerToChar (
        IN PLARGE_INTEGER Value,
        IN ULONG Base OPTIONAL,
        IN LONG OutputLength,
        OUT PSZ String
);
NTSTATUS
KtlAnsiStringToUnicodeString (
        __inout PUNICODE_STRING DestinationString,
        __in ANSI_STRING *SourceString,
        __in BOOLEAN AllocateDestinationString
);
NTSTATUS
KtlInt64ToUnicodeString (
        IN ULONGLONG Value,
        IN ULONG Base OPTIONAL,
        IN OUT PUNICODE_STRING String
);
