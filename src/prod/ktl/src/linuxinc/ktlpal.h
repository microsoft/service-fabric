/*++

    (c) 2017 by Microsoft Corp. All Rights Reserved.

    ktlpal.h

    Description:
        Platform abstraction layer for linux

    History:

--*/

/*
 * Needs to be moved to their proper headers with implementations later
 */

#pragma once

#include <PAL.h> // from prod/src/inc/clr

#define SIZETMult		SizeTMult

/* ---- From WTypes.h, or sspicli.h, sspi.h ----*/
#define SEC_FAR
typedef WCHAR SEC_WCHAR;
typedef CHAR SEC_CHAR;

#if !defined(UNDER_RTCPAL)
typedef struct _SecHandle {
	ULONG_PTR dwLower;
	ULONG_PTR dwUpper;
} SecHandle, * PSecHandle;

typedef struct _SecBuffer {
	unsigned long cbBuffer;		// Size of the buffer, in bytes
	unsigned long BufferType;	// Type of the buffer (below)
	void * pvBuffer;		// Pointer to the buffer
} SecBuffer, * PSecBuffer;

typedef struct _SecBufferDesc {
	unsigned long ulVersion;
	unsigned long cBuffers;
	PSecBuffer pBuffers;
} SecBufferDesc, *PSecBufferDesc;

typedef struct _SecPkgInfoW {
	unsigned long fCapabilities;
	unsigned short wVersion;
	unsigned short wRPCID;
	unsigned long cbMaxToken;
	SEC_WCHAR * Name;
	SEC_WCHAR * Comment;
} SecPkgInfoW, * PSecPkgInfoW;

typedef struct _SecPkgInfoA {
	unsigned long fCapabilities;
	unsigned short wVersion;
	unsigned short wRPCID;
	unsigned long cbMaxToken;

	SEC_CHAR * Name;
	SEC_CHAR * Comment;
} SecPkgInfoA, * PSecPkgInfoA;

#ifdef UNICODE
typedef SecPkgInfoW SecPkgInfo;
typedef PSecPkgInfoW PSecPkgInfo;
#else
typedef SecPkgInfoA SecPkgInfo;
typedef PSecPkgInfoA PSecPkgInfo;
#endif

#endif

BOOL
WINAPI
IsValidSid(
    _In_ PSID pSid
    );

DWORD WINAPI GetSidLengthRequired(
  _In_  UCHAR nSubAuthorityCount
  );

_Success_(return != FALSE) BOOL
WINAPI
LookupAccountSidW(
    _In_opt_ LPCWSTR lpSystemName,
    _In_ PSID Sid,
    _Out_writes_to_opt_(*cchName, *cchName + 1) LPWSTR Name,
    _Inout_  LPDWORD cchName,
    _Out_writes_to_opt_(*cchReferencedDomainName, *cchReferencedDomainName + 1) LPWSTR ReferencedDomainName,
    _Inout_ LPDWORD cchReferencedDomainName,
    _Out_ PSID_NAME_USE peUse
    );
#ifdef UNICODE
#define LookupAccountSid  LookupAccountSidW
#else
#define LookupAccountSid  LookupAccountSidA
#endif // !UNICODE

BOOL
WINAPI
ConvertSidToStringSidA(
    _In_  PSID     Sid,
    _Outptr_ LPSTR  *StringSid
    );

BOOL
WINAPI
ConvertSidToStringSidW(
    _In_ PSID Sid,
    _Outptr_ LPWSTR * StringSid
    );

#ifdef UNICODE
#define ConvertSidToStringSid  ConvertSidToStringSidW
#else
#define ConvertSidToStringSid  ConvertSidToStringSidA
#endif // !UNICODE

BOOL
WINAPI
ConvertStringSidToSidA(
    _In_ LPCSTR   StringSid,
    _Outptr_ PSID   *Sid
    );

BOOL
WINAPI
ConvertStringSidToSidW(
    _In_ LPCWSTR StringSid,
    _Outptr_ PSID * Sid
    );

#ifdef UNICODE
#define ConvertStringSidToSid  ConvertStringSidToSidW
#else
#define ConvertStringSidToSid  ConvertStringSidToSidA
#endif // !UNICODE

BOOL WINAPI IsWellKnownSid(
  _In_  PSID pSid,
  _In_  WELL_KNOWN_SID_TYPE WellKnownSidType
  );

PSID_IDENTIFIER_AUTHORITY WINAPI GetSidIdentifierAuthority(
  _In_  PSID pSid
  );

PUCHAR WINAPI GetSidSubAuthorityCount(
  _In_  PSID pSid
);

PDWORD WINAPI GetSidSubAuthority(
  _In_  PSID pSid,
  _In_  DWORD nSubAuthority
  );

BOOL ConvertSidToStringSid(
  _In_   PSID Sid,
  _Out_  LPTSTR *StringSid
  );

BOOL WINAPI CopySid(
  _In_   DWORD nDestinationSidLength,
  _Out_  PSID pDestinationSid,
  _In_   PSID pSourceSid
  );

BOOL WINAPI LookupAccountName(
  _In_opt_   LPCTSTR lpSystemName,
  _In_       LPCTSTR lpAccountName,
  _Out_opt_  PSID Sid,
  _Inout_    LPDWORD cbSid,
  _Out_opt_  LPTSTR ReferencedDomainName,
  _Inout_    LPDWORD cchReferencedDomainName,
  _Out_      PSID_NAME_USE peUse
  );


#define JOB_OBJECT_LIMIT_BREAKAWAY_OK		0x00000800
#define JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE	0x00002000

typedef struct _IO_COUNTERS {
    ULONGLONG ReadOperationCount;
    ULONGLONG WriteOperationCount;
    ULONGLONG OtherOperationCount;
    ULONGLONG ReadTransferCount;
    ULONGLONG WriteTransferCount;
    ULONGLONG OtherTransferCount;
} IO_COUNTERS;
typedef IO_COUNTERS * PIO_COUNTERS;

typedef struct _JOBOBJECT_BASIC_LIMIT_INFORMATION {
    LARGE_INTEGER PerProcessUserTimeLimit;
    LARGE_INTEGER PerJobUserTimeLimit;
    DWORD LimitFlags;
    SIZE_T MinimumWorkingSetSize;
    SIZE_T MaximumWorkingSetSize;
    DWORD ActiveProcessLimit;
    ULONG_PTR Affinity;
    DWORD PriorityClass;
    DWORD SchedulingClass;
} JOBOBJECT_BASIC_LIMIT_INFORMATION, *PJOBOBJECT_BASIC_LIMIT_INFORMATION;

typedef struct _JOBOBJECT_EXTENDED_LIMIT_INFORMATION {
    JOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimitInformation;
    IO_COUNTERS IoInfo;
    SIZE_T ProcessMemoryLimit;
    SIZE_T JobMemoryLimit;
    SIZE_T PeakProcessMemoryUsed;
    SIZE_T PeakJobMemoryUsed;
} JOBOBJECT_EXTENDED_LIMIT_INFORMATION, *PJOBOBJECT_EXTENDED_LIMIT_INFORMATION;

WINBASEAPI
BOOL
WINAPI
SetInformationJobObject(
    HANDLE hJob,
    JOBOBJECTINFOCLASS JobObjectInformationClass,
    LPVOID lpJobObjectInformation,
    DWORD cbJobObjectInformationLength
    );

WINBASEAPI
BOOL
WINAPI
AssignProcessToJobObject(
    HANDLE hJob,
    HANDLE hProcess
    );

WINBASEAPI
VOID
WINAPI
CloseThreadpoolWait(
    __inout PTP_WAIT pwa
    );

WINBASEAPI
VOID
WINAPI
SetThreadpoolWait(
    __inout  PTP_WAIT  pwa,
    __in_opt HANDLE    h,
    __in_opt PFILETIME pftTimeout
    );

WINBASEAPI
VOID
WINAPI
WaitForThreadpoolWaitCallbacks(
    __inout PTP_WAIT pwa,
    __in    BOOL     fCancelPendingCallbacks
    );

WINBASEAPI
VOID
WINAPI
DestroyThreadpoolEnvironment(
    __inout PTP_CALLBACK_ENVIRON pcbe
    );

WINBASEAPI
VOID
WINAPI
InitializeThreadpoolEnvironment(
    __out PTP_CALLBACK_ENVIRON pcbe
    );

WINBASEAPI
VOID
WINAPI
SetThreadpoolCallbackPool(
    __inout PTP_CALLBACK_ENVIRON pcbe,
    __in    PTP_POOL             ptpp
    );

// Registry
//
// Registry Specific Access Rights.
//

#define KEY_QUERY_VALUE         (0x0001)
#define KEY_SET_VALUE           (0x0002)
#define KEY_CREATE_SUB_KEY      (0x0004)
#define KEY_ENUMERATE_SUB_KEYS  (0x0008)
#define KEY_NOTIFY              (0x0010)
#define KEY_CREATE_LINK         (0x0020)
#define KEY_WOW64_32KEY         (0x0200)
#define KEY_WOW64_64KEY         (0x0100)
#define KEY_WOW64_RES           (0x0300)

#define KEY_READ                ((STANDARD_RIGHTS_READ       |\
                                  KEY_QUERY_VALUE            |\
                                  KEY_ENUMERATE_SUB_KEYS     |\
                                  KEY_NOTIFY)                 \
                                  &                           \
                                 (~SYNCHRONIZE))


#define KEY_WRITE               ((STANDARD_RIGHTS_WRITE      |\
                                  KEY_SET_VALUE              |\
                                  KEY_CREATE_SUB_KEY)         \
                                  &                           \
                                 (~SYNCHRONIZE))

#define KEY_EXECUTE             ((KEY_READ)                   \
                                  &                           \
                                 (~SYNCHRONIZE))

#define KEY_ALL_ACCESS          ((STANDARD_RIGHTS_ALL        |\
                                  KEY_QUERY_VALUE            |\
                                  KEY_SET_VALUE              |\
                                  KEY_CREATE_SUB_KEY         |\
                                  KEY_ENUMERATE_SUB_KEYS     |\
                                  KEY_NOTIFY                 |\
                                  KEY_CREATE_LINK)            \
                                  &                           \
                                 (~SYNCHRONIZE))



//
// Open/Create Options
//

#define REG_OPTION_RESERVED         (0x00000000L)   // Parameter is reserved

#define REG_OPTION_NON_VOLATILE     (0x00000000L)   // Key is preserved
                                                    // when system is rebooted

#define REG_OPTION_VOLATILE         (0x00000001L)   // Key is not preserved
                                                    // when system is rebooted

#define REG_OPTION_CREATE_LINK      (0x00000002L)   // Created key is a
                                                    // symbolic link

#define REG_OPTION_BACKUP_RESTORE   (0x00000004L)   // open for backup or restore
                                                    // special access rules
                                                    // privilege required

#define REG_OPTION_OPEN_LINK        (0x00000008L)   // Open symbolic link

#define REG_LEGAL_OPTION            \
                (REG_OPTION_RESERVED            |\
                 REG_OPTION_NON_VOLATILE        |\
                 REG_OPTION_VOLATILE            |\
                 REG_OPTION_CREATE_LINK         |\
                 REG_OPTION_BACKUP_RESTORE      |\
                 REG_OPTION_OPEN_LINK)

#define REG_OPEN_LEGAL_OPTION       \
                (REG_OPTION_RESERVED            |\
                 REG_OPTION_BACKUP_RESTORE      |\
                 REG_OPTION_OPEN_LINK)


#define REG_CREATED_NEW_KEY         (0x00000001L)   // New Registry Key created
#define REG_OPENED_EXISTING_KEY     (0x00000002L)   // Existing Key opened

//
// hive format to be used by Reg(Nt)SaveKeyEx
//
#define REG_STANDARD_FORMAT     1
#define REG_LATEST_FORMAT       2
#define REG_NO_COMPRESSION      4

//
// Key restore & hive load flags
//

#define REG_WHOLE_HIVE_VOLATILE         (0x00000001L)   // Restore whole hive volatile
#define REG_REFRESH_HIVE                (0x00000002L)   // Unwind changes to last flush
#define REG_NO_LAZY_FLUSH               (0x00000004L)   // Never lazy flush this hive
#define REG_FORCE_RESTORE               (0x00000008L)   // Force the restore process even when we have open handles on subkeys
#define REG_APP_HIVE                    (0x00000010L)   // Loads the hive visible to the calling process
#define REG_PROCESS_PRIVATE             (0x00000020L)   // Hive cannot be mounted by any other process while in use
#define REG_START_JOURNAL               (0x00000040L)   // Starts Hive Journal
#define REG_HIVE_EXACT_FILE_GROWTH      (0x00000080L)   // Grow hive file in exact 4k increments
#define REG_HIVE_NO_RM                  (0x00000100L)   // No RM is started for this hive (no transactions)
#define REG_HIVE_SINGLE_LOG             (0x00000200L)   // Legacy single logging is used for this hive
#define REG_BOOT_HIVE                   (0x00000400L)   // This hive might be used by the OS loader
#define REG_LOAD_HIVE_OPEN_HANDLE       (0x00000800L)   // Load the hive and return a handle to its root kcb
#define REG_FLUSH_HIVE_FILE_GROWTH      (0x00001000L)   // Flush changes to primary hive file size as part of all flushes

//
// Unload Flags
//
#define REG_FORCE_UNLOAD            1

//
// Notify filter values
//

#define REG_NOTIFY_CHANGE_NAME          (0x00000001L) // Create or delete (child)
#define REG_NOTIFY_CHANGE_ATTRIBUTES    (0x00000002L)
#define REG_NOTIFY_CHANGE_LAST_SET      (0x00000004L) // time stamp
#define REG_NOTIFY_CHANGE_SECURITY      (0x00000008L)
#define REG_NOTIFY_THREAD_AGNOSTIC      (0x10000000L) // Not associated with a calling thread, can only be used
                                                      // for async user event based notification

#define REG_LEGAL_CHANGE_FILTER                 \
                (REG_NOTIFY_CHANGE_NAME          |\
                 REG_NOTIFY_CHANGE_ATTRIBUTES    |\
                 REG_NOTIFY_CHANGE_LAST_SET      |\
                 REG_NOTIFY_CHANGE_SECURITY      |\
                 REG_NOTIFY_THREAD_AGNOSTIC)

// end_wdm

//
//
// Predefined Value Types.
//

#define REG_NONE                    ( 0 )   // No value type
#define REG_SZ                      ( 1 )   // Unicode nul terminated string
#define REG_EXPAND_SZ               ( 2 )   // Unicode nul terminated string
                                            // (with environment variable references)
#define REG_BINARY                  ( 3 )   // Free form binary
#define REG_DWORD                   ( 4 )   // 32-bit number
#define REG_DWORD_LITTLE_ENDIAN     ( 4 )   // 32-bit number (same as REG_DWORD)
#define REG_DWORD_BIG_ENDIAN        ( 5 )   // 32-bit number
#define REG_LINK                    ( 6 )   // Symbolic Link (unicode)
#define REG_MULTI_SZ                ( 7 )   // Multiple Unicode strings
#define REG_RESOURCE_LIST           ( 8 )   // Resource list in the resource map
#define REG_FULL_RESOURCE_DESCRIPTOR ( 9 )  // Resource list in the hardware description
#define REG_RESOURCE_REQUIREMENTS_LIST ( 10 )
#define REG_QWORD                   ( 11 )  // 64-bit number
#define REG_QWORD_LITTLE_ENDIAN     ( 11 )  // 64-bit number (same as REG_QWORD)

WINBASEAPI
HANDLE
WINAPI
FindFirstChangeNotificationA(
    _In_ LPCSTR lpPathName,
    _In_ BOOL bWatchSubtree,
    _In_ DWORD dwNotifyFilter
    );

WINBASEAPI
HANDLE
WINAPI
FindFirstChangeNotificationW(
    _In_ LPCWSTR lpPathName,
    _In_ BOOL bWatchSubtree,
    _In_ DWORD dwNotifyFilter
    );

#ifdef UNICODE
#define FindFirstChangeNotification FindFirstChangeNotificationW
#else
#define FindFirstChangeNotification FindFirstChangeNotificationA
#endif

WINBASEAPI
BOOL
WINAPI
FindNextChangeNotification(
    _In_ HANDLE hChangeHandle
    );

WINBASEAPI
BOOL
WINAPI
FindCloseChangeNotification(
    _In_ HANDLE hChangeHandle
    );


WINBASEAPI
HANDLE
WINAPI
CreateJobObjectW(
    LPSECURITY_ATTRIBUTES lpJobAttributes,
    LPCWSTR lpName
    );

WINBASEAPI
PTP_WAIT
WINAPI
CreateThreadpoolWait(
    __in        PTP_WAIT_CALLBACK    pfnwa,
    __inout_opt PVOID                pv,
    __in_opt    PTP_CALLBACK_ENVIRON pcbe
    );

LWSTDAPI_(BOOL)
PathSearchAndQualifyW(LPCWSTR pszPath, LPWSTR pszBuf, UINT cchBuf);

#ifdef UNICODE
#define PathSearchAndQualify PathSearchAndQualifyW
#else
#define PathSearchAndQualify PathSearchAndQualifyA
#endif

HRESULT FileExists(
        __in PCWSTR filename,
        __out bool * exists);

typedef struct _MOUNTDEV_STABLE_GUID {
    GUID    StableGuid;
} MOUNTDEV_STABLE_GUID, *PMOUNTDEV_STABLE_GUID;

#define IOCTL_MOUNTDEV_QUERY_STABLE_GUID            CTL_CODE(MOUNTDEVCONTROLTYPE, 6, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define MOUNTMGRCONTROLTYPE                         0x0000006D // 'm'
#define MOUNTDEVCONTROLTYPE                         0x0000004D // 'M'


#define DIRECTORY_QUERY                 (0x0001)
#define DIRECTORY_TRAVERSE              (0x0002)
#define DIRECTORY_CREATE_OBJECT         (0x0004)
#define DIRECTORY_CREATE_SUBDIRECTORY   (0x0008)

#define DIRECTORY_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0xF)

#define SYMBOLIC_LINK_QUERY    (0x0001)
#define SYMBOLIC_LINK_SET      (0x0002)

#define SYMBOLIC_LINK_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | 0x1)
#define SYMBOLIC_LINK_ALL_ACCESS_EX (STANDARD_RIGHTS_REQUIRED | 0xFFFF)

typedef struct _OBJECT_DIRECTORY_INFORMATION {
    UNICODE_STRING Name;
    UNICODE_STRING TypeName;
} OBJECT_DIRECTORY_INFORMATION, *POBJECT_DIRECTORY_INFORMATION;

#define DEVICE_TYPE ULONG

typedef struct _FILE_FS_DEVICE_INFORMATION {
    DEVICE_TYPE DeviceType;
    ULONG Characteristics;
} FILE_FS_DEVICE_INFORMATION, *PFILE_FS_DEVICE_INFORMATION;

#define FILE_REMOVABLE_MEDIA                        0x00000001

typedef struct _MOUNTMGR_MOUNT_POINT {
	ULONG   SymbolicLinkNameOffset;
	USHORT  SymbolicLinkNameLength;
	ULONG   UniqueIdOffset;
	USHORT  UniqueIdLength;
	ULONG   DeviceNameOffset;
	USHORT  DeviceNameLength;
} MOUNTMGR_MOUNT_POINT, *PMOUNTMGR_MOUNT_POINT;

#define MOUNTMGR_DEVICE_NAME                        L"\\Device\\MountPointManager"

#define IOCTL_MOUNTMGR_QUERY_POINTS                 CTL_CODE(MOUNTMGRCONTROLTYPE, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct _MOUNTMGR_MOUNT_POINTS {
	ULONG                   Size;
	ULONG                   NumberOfMountPoints;
	MOUNTMGR_MOUNT_POINT    MountPoints[1];
} MOUNTMGR_MOUNT_POINTS, *PMOUNTMGR_MOUNT_POINTS;

#define MOUNTMGR_IS_DRIVE_LETTER(s) (   \
    (s)->Length == 28 &&                \
    (s)->Buffer[0] == '\\' &&           \
    (s)->Buffer[1] == 'D' &&            \
    (s)->Buffer[2] == 'o' &&            \
    (s)->Buffer[3] == 's' &&            \
    (s)->Buffer[4] == 'D' &&            \
    (s)->Buffer[5] == 'e' &&            \
    (s)->Buffer[6] == 'v' &&            \
    (s)->Buffer[7] == 'i' &&            \
    (s)->Buffer[8] == 'c' &&            \
    (s)->Buffer[9] == 'e' &&            \
    (s)->Buffer[10] == 's' &&           \
    (s)->Buffer[11] == '\\' &&          \
    (s)->Buffer[12] >= 'A' &&           \
    (s)->Buffer[12] <= 'Z' &&           \
    (s)->Buffer[13] == ':')

#define MOUNTMGR_IS_VOLUME_NAME(s) (                                          \
     ((s)->Length == 96 || ((s)->Length == 98 && (s)->Buffer[48] == '\\')) && \
     (s)->Buffer[0] == '\\' &&                                                \
     ((s)->Buffer[1] == '?' || (s)->Buffer[1] == '\\') &&                     \
     (s)->Buffer[2] == '?' &&                                                 \
     (s)->Buffer[3] == '\\' &&                                                \
     (s)->Buffer[4] == 'V' &&                                                 \
     (s)->Buffer[5] == 'o' &&                                                 \
     (s)->Buffer[6] == 'l' &&                                                 \
     (s)->Buffer[7] == 'u' &&                                                 \
     (s)->Buffer[8] == 'm' &&                                                 \
     (s)->Buffer[9] == 'e' &&                                                 \
     (s)->Buffer[10] == '{' &&                                                \
     (s)->Buffer[19] == '-' &&                                                \
     (s)->Buffer[24] == '-' &&                                                \
     (s)->Buffer[29] == '-' &&                                                \
     (s)->Buffer[34] == '-' &&                                                \
     (s)->Buffer[47] == '}'                                                   \
    )

#define FILE_SUPERSEDE                  0x00000000
#define FILE_OPEN                       0x00000001
#define FILE_CREATE                     0x00000002
#define FILE_OPEN_IF                    0x00000003
#define FILE_OVERWRITE                  0x00000004
#define FILE_OVERWRITE_IF               0x00000005
#define FILE_MAXIMUM_DISPOSITION        0x00000005

//
// Define the create/open option flags
//

#define FILE_DIRECTORY_FILE                     0x00000001
#define FILE_WRITE_THROUGH                      0x00000002
#define FILE_SEQUENTIAL_ONLY                    0x00000004
#define FILE_NO_INTERMEDIATE_BUFFERING          0x00000008

#define FILE_SYNCHRONOUS_IO_ALERT               0x00000010
#define FILE_SYNCHRONOUS_IO_NONALERT            0x00000020
#define FILE_NON_DIRECTORY_FILE                 0x00000040
#define FILE_CREATE_TREE_CONNECTION             0x00000080

#define FILE_COMPLETE_IF_OPLOCKED               0x00000100
#define FILE_NO_EA_KNOWLEDGE                    0x00000200
#define FILE_OPEN_REMOTE_INSTANCE               0x00000400
#define FILE_RANDOM_ACCESS                      0x00000800

#define FILE_DELETE_ON_CLOSE                    0x00001000
#define FILE_OPEN_BY_FILE_ID                    0x00002000
#define FILE_OPEN_FOR_BACKUP_INTENT             0x00004000
#define FILE_NO_COMPRESSION                     0x00008000

#define FILE_RESERVE_OPFILTER                   0x00100000
#define FILE_OPEN_REPARSE_POINT                 0x00200000
#define FILE_OPEN_NO_RECALL                     0x00400000
#define FILE_OPEN_FOR_FREE_SPACE_QUERY          0x00800000

#undef DeleteFile
typedef struct _FILE_DISPOSITION_INFORMATION {
    BOOLEAN DeleteFile;
} FILE_DISPOSITION_INFORMATION, *PFILE_DISPOSITION_INFORMATION;

typedef struct _FILE_DIRECTORY_INFORMATION {
    ULONG NextEntryOffset;
    ULONG FileIndex;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER EndOfFile;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_DIRECTORY_INFORMATION, *PFILE_DIRECTORY_INFORMATION;

typedef struct _FILE_LINK_ENTRY_INFORMATION {
    ULONG NextEntryOffset;
    LONGLONG ParentFileId;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_LINK_ENTRY_INFORMATION, *PFILE_LINK_ENTRY_INFORMATION;

typedef struct _FILE_LINKS_INFORMATION {
    ULONG BytesNeeded;
    ULONG EntriesReturned;
    FILE_LINK_ENTRY_INFORMATION Entry;
} FILE_LINKS_INFORMATION, *PFILE_LINKS_INFORMATION;

typedef struct _FILE_NAME_INFORMATION {
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;

typedef struct _FILE_LINK_INFORMATION {
    BOOLEAN ReplaceIfExists;
    HANDLE RootDirectory;
    ULONG FileNameLength;
    WCHAR FileName[1];
} FILE_LINK_INFORMATION, *PFILE_LINK_INFORMATION;


typedef enum _IO_PRIORITY_HINT {
    IoPriorityVeryLow = 0,          // Defragging, content indexing and other background I/Os
    IoPriorityLow,                  // Prefetching for applications.
    IoPriorityNormal,               // Normal I/Os
    IoPriorityHigh,                 // Used by filesystems for checkpoint I/O
    IoPriorityCritical,             // Used by memory manager. Not available for applications.
    MaxIoPriorityTypes
} IO_PRIORITY_HINT;

typedef void * PVOID64;

#ifdef __aarch64__ 
inline __uint128_t InterlockedCompareExchange128(__uint128_t * src, __uint128_t cmp, __uint128_t with)
{
    return __sync_val_compare_and_swap(src, cmp, with);
}

inline unsigned char InterlockedCompareExchange128(
    _Inout_ LONGLONG volatile *Destination,
    _In_    LONGLONG          ExchangeHigh,
    _In_    LONGLONG          ExchangeLow,
    _Inout_ LONGLONG          *ComparandResult
    )
{
    LONGLONG exchange[2]= {ExchangeHigh, ExchangeLow};  
    return __sync_bool_compare_and_swap((__uint128_t*)Destination, *(__uint128_t*)exchange, *(__uint128_t*)ComparandResult);
}
#else
inline __uint128_t InterlockedCompareExchange128(volatile __uint128_t * src, __uint128_t cmp, __uint128_t with)
{
    __asm__ __volatile__
    (
        "lock cmpxchg16b %1"
        : "+A" (cmp)
        , "+m" (*src)
        : "b" ((long long)with)
        , "c" ((long long)(with >> 64))
        : "cc"
    );
    return cmp;
}

inline unsigned char InterlockedCompareExchange128(
    _Inout_ LONGLONG volatile *Destination,
    _In_    LONGLONG          ExchangeHigh,
    _In_    LONGLONG          ExchangeLow,
    _Inout_ LONGLONG          *ComparandResult
    )
{
    unsigned char result = 0;
    __asm__ __volatile__
    (
    "lock cmpxchg16b %1\n\t"
    "setz %0"
    : "=q" ( result )
    , "+m" ( *Destination )
    , "+d" ( ComparandResult[1] )
    , "+a" ( ComparandResult[0] )
    : "c" ( ExchangeHigh )
    , "b" ( ExchangeLow )
    : "cc"
    );
    return result;
}
#endif


typedef struct _FILE_STANDARD_INFORMATION {
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG NumberOfLinks;
    BOOLEAN DeletePending;
    BOOLEAN Directory;
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;

typedef struct _FILE_VALID_DATA_LENGTH_INFORMATION {
    LARGE_INTEGER ValidDataLength;
} FILE_VALID_DATA_LENGTH_INFORMATION, *PFILE_VALID_DATA_LENGTH_INFORMATION;

typedef struct _GET_LENGTH_INFORMATION {
    LARGE_INTEGER   Length;
} GET_LENGTH_INFORMATION, *PGET_LENGTH_INFORMATION;

typedef struct _FILE_SET_SPARSE_BUFFER {
    BOOLEAN SetSparse;
} FILE_SET_SPARSE_BUFFER, *PFILE_SET_SPARSE_BUFFER;

#define FSCTL_SET_SPARSE                CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 49, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#define FSCTL_QUERY_ALLOCATED_RANGES    CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 51,  METHOD_NEITHER, FILE_READ_DATA)  // FILE_ALLOCATED_RANGE_BUFFER, FILE_ALLOCATED_RANGE_BUFFER
#define FSCTL_SET_ZERO_DATA             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 50, METHOD_BUFFERED, FILE_WRITE_DATA) // FILE_ZERO_DATA_INFORMATION,

#define IOCTL_DISK_GET_LENGTH_INFO          CTL_CODE(IOCTL_DISK_BASE, 0x0017, METHOD_BUFFERED, FILE_READ_ACCESS)

#define FILE_DEVICE_FILE_SYSTEM         0x00000009

#define FILE_ANY_ACCESS                 0
#define FILE_SPECIAL_ACCESS    (FILE_ANY_ACCESS)
#define FILE_READ_ACCESS          ( 0x0001 )    // file & pipe
#define FILE_WRITE_ACCESS         ( 0x0002 )    // file & pipe

typedef struct _FILE_BASIC_INFORMATION {
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    ULONG FileAttributes;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

#define FILE_DEVICE_DISK                0x00000007
#define IOCTL_DISK_BASE                 FILE_DEVICE_DISK

#define SECURITY_NT_AUTHORITY           {0,0,0,0,0,5}   // ntifs

#define SECURITY_DESCRIPTOR_REVISION     (1)
#define SECURITY_DESCRIPTOR_REVISION1    (1)

#define SECURITY_BUILTIN_DOMAIN_RID     (0x00000020L)
#define DOMAIN_ALIAS_RID_ADMINS                         (0x00000220L)

ULONG RtlLengthSid (PSID Sid);

NTSTATUS
RtlCreateAcl (
        __in  PACL Acl,
        __in  ULONG AclLength,
        __in  ULONG AclRevision
);

NTSTATUS
RtlAddAccessAllowedAce (
        __in  __out PACL Acl,
        __in  ULONG AceRevision,
        __in  ACCESS_MASK AccessMask,
        __in  PSID Sid
);

NTSTATUS
RtlSetDaclSecurityDescriptor (
        __in  PSECURITY_DESCRIPTOR SecurityDescriptor,
        __in  BOOLEAN DaclPresent,
        __in  PACL Dacl OPTIONAL,
        __in  BOOLEAN DaclDefaulted OPTIONAL
);

typedef struct _FILE_ALLOCATED_RANGE_BUFFER {

    LARGE_INTEGER FileOffset;
    LARGE_INTEGER Length;

} FILE_ALLOCATED_RANGE_BUFFER, *PFILE_ALLOCATED_RANGE_BUFFER;

typedef struct _FILE_ZERO_DATA_INFORMATION {

    LARGE_INTEGER FileOffset;
    LARGE_INTEGER BeyondFinalZero;

} FILE_ZERO_DATA_INFORMATION, *PFILE_ZERO_DATA_INFORMATION;

NTSTATUS
RtlCreateSecurityDescriptor (
        __in  PSECURITY_DESCRIPTOR SecurityDescriptor,
        __in  ULONG Revision
);

BOOL
WINAPI
WriteFileGather(
        __in       HANDLE hFile,
        __in       FILE_SEGMENT_ELEMENT aSegementArray[],
        __in       DWORD nNumberOfBytesToWrite,
        __reserved LPDWORD lpReserved,
        __inout    LPOVERLAPPED lpOverlapped
);

BOOL
WINAPI
ReadFileScatter(
        __in       HANDLE hFile,
        __in       FILE_SEGMENT_ELEMENT aSegementArray[],
        __in       DWORD nNumberOfBytesToRead,
        __reserved LPDWORD lpReserved,
        __inout    LPOVERLAPPED lpOverlapped
);

BOOL WINAPI WriteFile(
  _In_       HANDLE               hFile,
  _In_       LPVOID               lpBuffer,
  _In_       DWORD                nNumberOfBytesToWrite,
  _Out_opt_  LPDWORD              lpNumberBytesWriten,
  _Inout_    LPOVERLAPPED         lpOverlapped);

BOOL WINAPI ReadFile(
  _In_       HANDLE               hFile,
  _In_       LPVOID               lpBuffer,
  _In_       DWORD                nNumberOfBytesToRead,
  _Out_opt_  LPDWORD              lpNumberBytesRead,
  _Inout_    LPOVERLAPPED         lpOverlapped);

BOOL
WINAPI
DuplicateTokenEx(
        IN HANDLE hExistingToken,
        IN DWORD dwDesiredAccess,
        IN LPSECURITY_ATTRIBUTES lpTokenAttributes,
        IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
        IN TOKEN_TYPE TokenType,
        OUT PHANDLE phNewToken);

BOOL
APIENTRY
SetThreadToken (
        __in_opt PHANDLE Thread,
        __in_opt HANDLE Token
);

#define SE_MANAGE_VOLUME_PRIVILEGE          (28L)
#define SE_PRIVILEGE_ENABLED            (0x00000002L)

BOOL
APIENTRY
AdjustTokenPrivileges (
        __in      HANDLE TokenHandle,
        __in      BOOL DisableAllPrivileges,
        __in_opt  PTOKEN_PRIVILEGES NewState,
        __in      DWORD BufferLength,
        __out_bcount_part_opt(BufferLength, *ReturnLength) PTOKEN_PRIVILEGES PreviousState,
        __out_opt PDWORD ReturnLength
);

typedef struct _FILE_ALLOCATION_INFORMATION {
    LARGE_INTEGER AllocationSize;
} FILE_ALLOCATION_INFORMATION, *PFILE_ALLOCATION_INFORMATION;

typedef struct _FILE_END_OF_FILE_INFORMATION {
    LARGE_INTEGER EndOfFile;
} FILE_END_OF_FILE_INFORMATION, *PFILE_END_OF_FILE_INFORMATION;

typedef struct _FILE_IO_PRIORITY_HINT_INFORMATION {
    IO_PRIORITY_HINT   PriorityHint;
} FILE_IO_PRIORITY_HINT_INFORMATION, *PFILE_IO_PRIORITY_HINT_INFORMATION;

#define BIT_NUMBER ULONG
#define BITMAP_ELEMENT ULONG
#define PBITMAP_ELEMENT PULONG
#define PSBITMAP_ELEMENT PLONG
#define BITMAP_ELEMENT_BITS     32
#define BITMAP_ELEMENT_SHIFT    5
#define BITMAP_ELEMENT_MASK     31
#define BITMAP_SHIFT_BASE       1

#define RtlFillMemory(Destination,Length,Fill) memset((Destination),(Fill),(Length))

VOID
ORClearBits (
    __in PRTL_BITMAP BitMapHeader,
    __in BIT_NUMBER StartingIndex,
    __in BIT_NUMBER NumberToClear
);

VOID
ORSetBits (
        __in PRTL_BITMAP BitMapHeader,
        __in UINT32 StartingIndex,
        __in UINT32 NumberToSet
);

#define RtlClearBits         ORClearBits
#define RtlSetBits           ORSetBits

#define RtlCheckBit(BMH,BP) (((((PLONG)(BMH)->Buffer)[(BP) / 32]) >> ((BP) % 32)) & 0x1)

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

NTSTATUS
RtlMultiByteToUnicodeSize(
        OUT PULONG BytesInUnicodeString,
        IN PCSTR MultiByteString,
        IN ULONG BytesInMultiByteString);

NTSTATUS
RtlMultiByteToUnicodeN(
        OUT PWCH UnicodeString,
        IN ULONG MaxBytesInUnicodeString,
        OUT PULONG BytesInUnicodeString OPTIONAL,
        IN PCSTR MultiByteString,
        IN ULONG BytesInMultiByteString);

#define RPC_NT_STRING_TOO_LONG           ((NTSTATUS)0xC002002CL)

#define ULONGLONG_ERROR 0xffffffffffffffffui64

#define ULongPtrToULong ULongLongToULong

#define ULONGLONG_MAX   0xffffffffffffffffui64

#define STATUS_DUPLICATE_OBJECTID        ((NTSTATUS)0xC000022AL)
#define STATUS_BUFFER_ALL_ZEROS          ((NTSTATUS)0x00000117L)

//#define GetCurrentTime()                GetTickCount()

void GetSystemTimeAsFileTime(
  _Out_ LPFILETIME lpSystemTimeAsFileTime
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
);

NTSTATUS NtReadFile(
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

void WINAPI GetSystemInfo(
  _Out_ LPSYSTEM_INFO lpSystemInfo
);

HANDLE WINAPI GetCurrentThread(void);

#define LONGLONG_MAX    9223372036854775807i64

HANDLE
CreateIoCompletionPort(
    _In_ HANDLE FileHandle,
    _In_opt_ HANDLE ExistingCompletionPort,
    _In_ ULONG_PTR CompletionKey,
    _In_ DWORD NumberOfConcurrentThreads
    );

HANDLE
WINAPI
OpenThread(
    _In_ DWORD dwDesiredAccess,
    _In_ BOOL bInheritHandle,
    _In_ DWORD dwThreadId
    );

DWORD_PTR
WINAPI
SetThreadAffinityMask(
    _In_ HANDLE hThread,
    _In_ DWORD_PTR dwThreadAffinityMask
    );

BOOL
WINAPI
GetProcessAffinityMask(
    _In_  HANDLE hProcess,
    _Out_ PDWORD_PTR lpProcessAffinityMask,
    _Out_ PDWORD_PTR lpSystemAffinityMask
    );

BOOL
WINAPI
PostQueuedCompletionStatus(
    _In_ HANDLE CompletionPort,
    _In_ DWORD dwNumberOfBytesTransferred,
    _In_ ULONG_PTR dwCompletionKey,
    _In_opt_ LPOVERLAPPED lpOverlapped
    );

BOOL
WINAPI
GetQueuedCompletionStatus(
    _In_ HANDLE CompletionPort,
    _Out_ LPDWORD lpNumberOfBytesTransferred,
    _Out_ PULONG_PTR lpCompletionKey,
    _Out_ LPOVERLAPPED * lpOverlapped,
    _In_ DWORD dwMilliseconds
    );

/* winreg.h - BEGIN */
#define RRF_RT_REG_SZ			0x00000002
#define RRF_RT_REG_EXPAND_SZ		0x00000004
#define RRF_RT_REG_DWORD		0x00000010


#define RRF_NOEXPAND			0x10000000
#define RRF_ZEROONFAILURE		0x20000000

//
// Reserved Key Handles.
//

#define HKEY_CLASSES_ROOT                   (( HKEY ) (ULONG_PTR)((LONG)0x80000000) )
#define HKEY_CURRENT_USER                   (( HKEY ) (ULONG_PTR)((LONG)0x80000001) )
#define HKEY_LOCAL_MACHINE                  (( HKEY ) (ULONG_PTR)((LONG)0x80000002) )
#define HKEY_USERS                          (( HKEY ) (ULONG_PTR)((LONG)0x80000003) )
#define HKEY_PERFORMANCE_DATA               (( HKEY ) (ULONG_PTR)((LONG)0x80000004) )
#define HKEY_PERFORMANCE_TEXT               (( HKEY ) (ULONG_PTR)((LONG)0x80000050) )
#define HKEY_PERFORMANCE_NLSTEXT            (( HKEY ) (ULONG_PTR)((LONG)0x80000060) )

#if (WINVER >= 0x0400)
#define HKEY_CURRENT_CONFIG                 (( HKEY ) (ULONG_PTR)((LONG)0x80000005) )
#define HKEY_DYN_DATA                       (( HKEY ) (ULONG_PTR)((LONG)0x80000006) )
#define HKEY_CURRENT_USER_LOCAL_SETTINGS    (( HKEY ) (ULONG_PTR)((LONG)0x80000007) )
#endif

WINADVAPI
LSTATUS
APIENTRY
RegQueryValueExA(
    _In_ HKEY hKey,
    _In_opt_ LPCSTR lpValueName,
    _Reserved_ LPDWORD lpReserved,
    _Out_opt_ LPDWORD lpType,
     LPBYTE lpData,
     LPDWORD lpcbData
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

#ifdef UNICODE
#define RegQueryValueEx  RegQueryValueExW
#else
#define RegQueryValueEx  RegQueryValueExA
#endif // !UNICODE

WINADVAPI
LSTATUS
APIENTRY
RegDeleteValueA(
    _In_ HKEY hKey,
    _In_opt_ LPCSTR lpValueName
    );

WINADVAPI
LSTATUS
APIENTRY
RegDeleteValueW(
    _In_ HKEY hKey,
    _In_opt_ LPCWSTR lpValueName
    );

#ifdef UNICODE
#define RegDeleteValue  RegDeleteValueW
#else
#define RegDeleteValue  RegDeleteValueA
#endif // !UNICODE

WINADVAPI
LSTATUS
APIENTRY
RegDeleteKeyExA(
    _In_ HKEY hKey,
    _In_ LPCSTR lpSubKey,
    _In_ REGSAM samDesired,
    _Reserved_ DWORD Reserved
    );

WINADVAPI
LSTATUS
APIENTRY
RegDeleteKeyExW(
    _In_ HKEY hKey,
    _In_ LPCWSTR lpSubKey,
    _In_ REGSAM samDesired,
    _Reserved_ DWORD Reserved
    );

#ifdef UNICODE
#define RegDeleteKeyEx  RegDeleteKeyExW
#else
#define RegDeleteKeyEx  RegDeleteKeyExA
#endif // !UNICODE

WINADVAPI
LSTATUS
APIENTRY
RegSetValueExA(
    HKEY hKey,
    LPCSTR lpValueName,
    DWORD Reserved,
    DWORD dwType,
    const BYTE *lpData,
    DWORD cbData
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

#ifdef UNICODE
#define RegSetValueEx RegSetValueExW
#else
#define RegSetValueEx RegSetValueExA
#endif

WINADVAPI
LSTATUS
APIENTRY
RegOpenKeyExA(
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
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

#ifdef UNICODE
#define RegOpenKeyEx RegOpenKeyExW
#else
#define RegOpenKeyEx RegOpenKeyExA
#endif

WINADVAPI
LSTATUS
APIENTRY
RegCreateKeyExA(
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD Reserved,
    LPSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    const LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition
    );

WINADVAPI
LSTATUS
APIENTRY
RegCreateKeyExW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD Reserved,
    LPWSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    const LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition
    );

#ifdef UNICODE
#define RegCreateKeyEx RegCreateKeyExW
#else
#define RegCreateKeyEx RegCreateKeyExA
#endif

WINADVAPI
LSTATUS
APIENTRY
RegGetValueA(
    HKEY hKey,
    LPCSTR lpSubKey,
    LPCSTR lpValue,
    DWORD dwFlags,
    LPDWORD pdwType,
    PVOID pvData,
    LPDWORD pcbData
    );

WINADVAPI
LSTATUS
APIENTRY
RegGetValueW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    LPCWSTR lpValue,
    DWORD dwFlags,
    LPDWORD pdwType,
    PVOID pvData,
    LPDWORD pcbData
    );


#ifdef UNICODE
#define RegGetValue RegGetValueW
#else
#define RegGetValue RegGetValueA
#endif

WINADVAPI
LSTATUS
APIENTRY
RegCloseKey(
    _In_ HKEY hKey
    );

LONG WINAPI RegConnectRegistry(
  _In_opt_ LPCTSTR lpMachineName,
  _In_     HKEY    hKey,
  _Out_    PHKEY   phkResult
);
