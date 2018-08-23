// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "internal/pal_common.h"

#ifdef __cplusplus
extern "C" {
#endif

// 130
#ifndef DECLSPEC_ALIGN
#if (_MSC_VER >= 1300) && !defined(MIDL_PASS)
#define DECLSPEC_ALIGN(x)   __declspec(align(x))
#else
#define DECLSPEC_ALIGN(x)
#endif
#endif

#ifndef SYSTEM_CACHE_ALIGNMENT_SIZE
#if defined(_AMD64_) || defined(_X86_)
#define SYSTEM_CACHE_ALIGNMENT_SIZE 64
#else
#define SYSTEM_CACHE_ALIGNMENT_SIZE 128
#endif
#endif

#ifndef DECLSPEC_CACHEALIGN
#define DECLSPEC_CACHEALIGN DECLSPEC_ALIGN(SYSTEM_CACHE_ALIGNMENT_SIZE)
#endif

// 190
#ifndef DECLSPEC_NOINLINE
#if (_MSC_VER >= 1300)
#define DECLSPEC_NOINLINE  __declspec(noinline)
#else
#define DECLSPEC_NOINLINE
#endif
#endif

// 235
typedef void *PVOID;
typedef void *POINTER_64;
typedef void *PVOID64;

// 256
#if !defined(_NTSYSTEM_)
#define NTSYSAPI     DECLSPEC_IMPORT
#define NTSYSCALLAPI DECLSPEC_IMPORT
#else
#define NTSYSAPI
#if defined(_NTDLLBUILD_)
#define NTSYSCALLAPI
#else
#define NTSYSCALLAPI DECLSPEC_ADDRSAFE
#endif
#endif

#define __TEXT(quote) L##quote      // r_winnt
#define TEXT(quote) __TEXT(quote)   // r_winnt

// 486
typedef char CCHAR;
#define ERROR_SEVERITY_ERROR         0xC0000000

// 531
#define MAXLONGLONG                      (0x7fffffffffffffff)

// 600
typedef struct _LUID {
    DWORD LowPart;
    LONG HighPart;
} LUID, *PLUID;

// 631
#define Int32x32To64(a, b)  (((__int64)((long)(a))) * ((__int64)((long)(b))))

#define DEFINE_ENUM_FLAG_OPERATORS(ENUMTYPE) \
extern "C++" { \
inline ENUMTYPE operator | (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) | ((int)b)); } \
inline ENUMTYPE &operator |= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) |= ((int)b)); } \
inline ENUMTYPE operator & (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) & ((int)b)); } \
inline ENUMTYPE &operator &= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) &= ((int)b)); } \
inline ENUMTYPE operator ~ (ENUMTYPE a) { return ENUMTYPE(~((int)a)); } \
inline ENUMTYPE operator ^ (ENUMTYPE a, ENUMTYPE b) { return ENUMTYPE(((int)a) ^ ((int)b)); } \
inline ENUMTYPE &operator ^= (ENUMTYPE &a, ENUMTYPE b) { return (ENUMTYPE &)(((int &)a) ^= ((int)b)); } \
}

// 837
#define UNICODE_NULL ((WCHAR)0)

// 1997
SHORT
InterlockedCompareExchange16 (
    __inout SHORT volatile *Destination,
    __in SHORT ExChange,
    __in SHORT Comperand
    );

// 2403
VOID
__movsb (
    IN PBYTE  Destination,
    IN BYTE  const *Source,
    IN SIZE_T Count
    );

VOID
__stosq (
    IN PDWORD64 Destination,
    IN DWORD64 Value,
    IN SIZE_T Count
    );

// 2708
CHAR
InterlockedExchange8 (
    _Inout_ _Interlocked_operand_ CHAR volatile *Target,
    _In_ CHAR Value
    );

// 3991
LONGLONG
__cdecl
InterlockedAdd64 (
    LONGLONG volatile *Addend,
    LONGLONG Value
    );

// 5611
typedef DWORD ACCESS_MASK;
typedef ACCESS_MASK *PACCESS_MASK;

// 5626
#define DELETE                           (0x00010000L)
#define READ_CONTROL                     (0x00020000L)
#define WRITE_DAC                        (0x00040000L)
#define WRITE_OWNER                      (0x00080000L)
#define SYNCHRONIZE                      (0x00100000L)

#define STANDARD_RIGHTS_REQUIRED         (0x000F0000L)

#define STANDARD_RIGHTS_READ             (READ_CONTROL)
#define STANDARD_RIGHTS_WRITE            (READ_CONTROL)
#define STANDARD_RIGHTS_EXECUTE          (READ_CONTROL)

#define STANDARD_RIGHTS_ALL              (0x001F0000L)

#define SPECIFIC_RIGHTS_ALL              (0x0000FFFFL)

// 5658
#define GENERIC_READ                     (0x80000000L)
#define GENERIC_WRITE                    (0x40000000L)
#define GENERIC_EXECUTE                  (0x20000000L)
#define GENERIC_ALL                      (0x10000000L)

// 5690
typedef struct _LUID_AND_ATTRIBUTES {
    LUID Luid;
    DWORD Attributes;
} LUID_AND_ATTRIBUTES, * PLUID_AND_ATTRIBUTES;
typedef LUID_AND_ATTRIBUTES LUID_AND_ATTRIBUTES_ARRAY[ANYSIZE_ARRAY];
typedef LUID_AND_ATTRIBUTES_ARRAY *PLUID_AND_ATTRIBUTES_ARRAY;

// 5730
#ifndef SID_IDENTIFIER_AUTHORITY_DEFINED
#define SID_IDENTIFIER_AUTHORITY_DEFINED
typedef struct _SID_IDENTIFIER_AUTHORITY {
    BYTE  Value[6];
} SID_IDENTIFIER_AUTHORITY, *PSID_IDENTIFIER_AUTHORITY;
#endif

// Linux specific macros
#define SID_SUBAUTH_ARRAY_DOMAIN   0
#define SID_SUBAUTH_ARRAY_UID      1
#define SID_SUBAUTH_ARRAY_SIZE     2

typedef struct _LSID {
	UCHAR Revision;                                     // 1
	UCHAR SubAuthorityCount;                            // 2
	SID_IDENTIFIER_AUTHORITY IdentifierAuthority;       // LOCAL
	ULONG SubAuthority[SID_SUBAUTH_ARRAY_SIZE];         // domain, uid
} LSID, *PLSID;

// 5738
#ifndef SID_DEFINED
#define SID_DEFINED
typedef struct _SID {
   BYTE  Revision;
   BYTE  SubAuthorityCount;
   SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
#ifdef MIDL_PASS
   [size_is(SubAuthorityCount)] DWORD SubAuthority[*];
#else // MIDL_PASS
   DWORD SubAuthority[ANYSIZE_ARRAY];
#endif // MIDL_PASS
} SID, *PISID;
#endif

#define SID_REVISION                     (1)    // Current revision level
#define SID_MAX_SUB_AUTHORITIES          (15)
#define SID_RECOMMENDED_SUB_AUTHORITIES  (1)    // Will change to around 6

#define SECURITY_MAX_SID_SIZE  \
      (sizeof(SID) - sizeof(DWORD) + (SID_MAX_SUB_AUTHORITIES * sizeof(DWORD)))

// 5763
typedef enum _SID_NAME_USE {
    SidTypeUser = 1,
    SidTypeGroup,
    SidTypeDomain,
    SidTypeAlias,
    SidTypeWellKnownGroup,
    SidTypeDeletedAccount,
    SidTypeInvalid,
    SidTypeUnknown,
    SidTypeComputer,
    SidTypeLabel
} SID_NAME_USE, *PSID_NAME_USE;

// 5812
#define SECURITY_LOCAL_SID_AUTHORITY        {0,0,0,0,0,2}
// Linux specific macros
#define SECURITY_LINUX_DOMAIN_RID       (0x00000021L)
#define SECURITY_LINUX_DOMAIN_GROUP_RID (0x00000022L)

// 5948
#define DOMAIN_GROUP_RID_USERS         (0x00000201L)

// 6014
typedef enum  {
  WinNullSid                                   = 0,
  WinWorldSid                                  = 1,
  WinLocalSid                                  = 2,
  WinCreatorOwnerSid                           = 3,
  WinCreatorGroupSid                           = 4,
  WinCreatorOwnerServerSid                     = 5,
  WinCreatorGroupServerSid                     = 6,
  WinNtAuthoritySid                            = 7,
  WinDialupSid                                 = 8,
  WinNetworkSid                                = 9,
  WinBatchSid                                  = 10,
  WinInteractiveSid                            = 11,
  WinServiceSid                                = 12,
  WinAnonymousSid                              = 13,
  WinProxySid                                  = 14,
  WinEnterpriseControllersSid                  = 15,
  WinSelfSid                                   = 16,
  WinAuthenticatedUserSid                      = 17,
  WinRestrictedCodeSid                         = 18,
  WinTerminalServerSid                         = 19,
  WinRemoteLogonIdSid                          = 20,
  WinLogonIdsSid                               = 21,
  WinLocalSystemSid                            = 22,
  WinLocalServiceSid                           = 23,
  WinNetworkServiceSid                         = 24,
  WinBuiltinDomainSid                          = 25,
  WinBuiltinAdministratorsSid                  = 26,
  WinBuiltinUsersSid                           = 27,
  WinBuiltinGuestsSid                          = 28,
  WinBuiltinPowerUsersSid                      = 29,
  WinBuiltinAccountOperatorsSid                = 30,
  WinBuiltinSystemOperatorsSid                 = 31,
  WinBuiltinPrintOperatorsSid                  = 32,
  WinBuiltinBackupOperatorsSid                 = 33,
  WinBuiltinReplicatorSid                      = 34,
  WinBuiltinPreWindows2000CompatibleAccessSid  = 35,
  WinBuiltinRemoteDesktopUsersSid              = 36,
  WinBuiltinNetworkConfigurationOperatorsSid   = 37,
  WinAccountAdministratorSid                   = 38,
  WinAccountGuestSid                           = 39,
  WinAccountKrbtgtSid                          = 40,
  WinAccountDomainAdminsSid                    = 41,
  WinAccountDomainUsersSid                     = 42,
  WinAccountDomainGuestsSid                    = 43,
  WinAccountComputersSid                       = 44,
  WinAccountControllersSid                     = 45,
  WinAccountCertAdminsSid                      = 46,
  WinAccountSchemaAdminsSid                    = 47,
  WinAccountEnterpriseAdminsSid                = 48,
  WinAccountPolicyAdminsSid                    = 49,
  WinAccountRasAndIasServersSid                = 50,
  WinNTLMAuthenticationSid                     = 51,
  WinDigestAuthenticationSid                   = 52,
  WinSChannelAuthenticationSid                 = 53,
  WinThisOrganizationSid                       = 54,
  WinOtherOrganizationSid                      = 55,
  WinBuiltinIncomingForestTrustBuildersSid     = 56,
  WinBuiltinPerfMonitoringUsersSid             = 57,
  WinBuiltinPerfLoggingUsersSid                = 58,
  WinBuiltinAuthorizationAccessSid             = 59,
  WinBuiltinTerminalServerLicenseServersSid    = 60,
  WinBuiltinDCOMUsersSid                       = 61,
  WinBuiltinIUsersSid                          = 62,
  WinIUserSid                                  = 63,
  WinBuiltinCryptoOperatorsSid                 = 64,
  WinUntrustedLabelSid                         = 65,
  WinLowLabelSid                               = 66,
  WinMediumLabelSid                            = 67,
  WinHighLabelSid                              = 68,
  WinSystemLabelSid                            = 69,
  WinWriteRestrictedCodeSid                    = 70,
  WinCreatorOwnerRightsSid                     = 71,
  WinCacheablePrincipalsGroupSid               = 72,
  WinNonCacheablePrincipalsGroupSid            = 73,
  WinEnterpriseReadonlyControllersSid          = 74,
  WinAccountReadonlyControllersSid             = 75,
  WinBuiltinEventLogReadersGroup               = 76,
  WinNewEnterpriseReadonlyControllersSid       = 77,
  WinBuiltinCertSvcDComAccessGroup             = 78,
  WinMediumPlusLabelSid                        = 79,
  WinLocalLogonSid                             = 80,
  WinConsoleLogonSid                           = 81,
  WinThisOrganizationCertificateSid            = 82,
  WinApplicationPackageAuthoritySid            = 83,
  WinBuiltinAnyPackageSid                      = 84,
  WinCapabilityInternetClientSid               = 85,
  WinCapabilityInternetClientServerSid         = 86,
  WinCapabilityPrivateNetworkClientServerSid   = 87,
  WinCapabilityPicturesLibrarySid              = 88,
  WinCapabilityVideosLibrarySid                = 89,
  WinCapabilityMusicLibrarySid                 = 90,
  WinCapabilityDocumentsLibrarySid             = 91,
  WinCapabilitySharedUserCertificatesSid       = 92,
  WinCapabilityEnterpriseAuthenticationSid     = 93,
  WinCapabilityRemovableStorageSid             = 94
} WELL_KNOWN_SID_TYPE;

// 6191
typedef struct _ACL {
    BYTE  AclRevision;
    BYTE  Sbz1;
    WORD   AclSize;
    WORD   AceCount;
    WORD   Sbz2;
} ACL;
typedef ACL *PACL;

// 6651
typedef WORD   SECURITY_DESCRIPTOR_CONTROL, *PSECURITY_DESCRIPTOR_CONTROL;

typedef struct _SECURITY_DESCRIPTOR {
   BYTE  Revision;
   BYTE  Sbz1;
   SECURITY_DESCRIPTOR_CONTROL Control;
   PSID Owner;
   PSID Group;
   PACL Sacl;
   PACL Dacl;
} SECURITY_DESCRIPTOR, *PISECURITY_DESCRIPTOR;

#define SE_DEBUG_NAME                     TEXT("SeDebugPrivilege")

// 6853
typedef enum _SECURITY_IMPERSONATION_LEVEL {
    SecurityAnonymous,
    SecurityIdentification,
    SecurityImpersonation,
    SecurityDelegation
} SECURITY_IMPERSONATION_LEVEL, * PSECURITY_IMPERSONATION_LEVEL;

// 6878
#define TOKEN_DUPLICATE         (0x0002)

// 6920
typedef enum _TOKEN_TYPE {
    TokenPrimary = 1,
    TokenImpersonation
    } TOKEN_TYPE;
typedef TOKEN_TYPE *PTOKEN_TYPE;

// 7176
#define PROCESS_QUERY_LIMITED_INFORMATION  (0x1000)  

// 7493
typedef enum _JOBOBJECTINFOCLASS {
    JobObjectBasicAccountingInformation = 1,
    JobObjectBasicLimitInformation,
    JobObjectBasicProcessIdList,
    JobObjectBasicUIRestrictions,
    JobObjectSecurityLimitInformation,
    JobObjectEndOfJobTimeInformation,
    JobObjectAssociateCompletionPortInformation,
    JobObjectBasicAndIoAccountingInformation,
    JobObjectExtendedLimitInformation,
    JobObjectJobSetInformation,
    MaxJobObjectInfoClass
} JOBOBJECTINFOCLASS;

// 7727
#define FILE_READ_DATA            ( 0x0001 )    // file & pipe
#define FILE_LIST_DIRECTORY       ( 0x0001 )    // directory

#define FILE_WRITE_DATA           ( 0x0002 )    // file & pipe
#define FILE_ADD_FILE             ( 0x0002 )    // directory

#define FILE_APPEND_DATA          ( 0x0004 )    // file
#define FILE_ADD_SUBDIRECTORY     ( 0x0004 )    // directory
#define FILE_CREATE_PIPE_INSTANCE ( 0x0004 )    // named pipe


#define FILE_READ_EA              ( 0x0008 )    // file & directory

#define FILE_WRITE_EA             ( 0x0010 )    // file & directory

#define FILE_EXECUTE              ( 0x0020 )    // file
#define FILE_TRAVERSE             ( 0x0020 )    // directory

#define FILE_DELETE_CHILD         ( 0x0040 )    // directory

#define FILE_READ_ATTRIBUTES      ( 0x0080 )    // all

#define FILE_WRITE_ATTRIBUTES     ( 0x0100 )    // all

#define FILE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x1FF)

#define FILE_GENERIC_READ         (STANDARD_RIGHTS_READ     |\
                                   FILE_READ_DATA           |\
                                   FILE_READ_ATTRIBUTES     |\
                                   FILE_READ_EA             |\
                                   SYNCHRONIZE)


#define FILE_GENERIC_WRITE        (STANDARD_RIGHTS_WRITE    |\
                                   FILE_WRITE_DATA          |\
                                   FILE_WRITE_ATTRIBUTES    |\
                                   FILE_WRITE_EA            |\
                                   FILE_APPEND_DATA         |\
                                   SYNCHRONIZE)


#define FILE_GENERIC_EXECUTE      (STANDARD_RIGHTS_EXECUTE  |\
                                   FILE_READ_ATTRIBUTES     |\
                                   FILE_EXECUTE             |\
                                   SYNCHRONIZE)

#define FILE_SHARE_READ                 0x00000001  
#define FILE_SHARE_WRITE                0x00000002  
#define FILE_SHARE_DELETE               0x00000004  
#define FILE_ATTRIBUTE_READONLY             0x00000001  
#define FILE_ATTRIBUTE_HIDDEN               0x00000002  
#define FILE_ATTRIBUTE_SYSTEM               0x00000004  
#define FILE_ATTRIBUTE_DIRECTORY            0x00000010  
#define FILE_ATTRIBUTE_ARCHIVE              0x00000020  
#define FILE_ATTRIBUTE_DEVICE               0x00000040  
#define FILE_ATTRIBUTE_NORMAL               0x00000080  
#define FILE_ATTRIBUTE_TEMPORARY            0x00000100  
#define FILE_ATTRIBUTE_SPARSE_FILE          0x00000200  
#define FILE_ATTRIBUTE_REPARSE_POINT        0x00000400  
#define FILE_ATTRIBUTE_COMPRESSED           0x00000800  
#define FILE_ATTRIBUTE_OFFLINE              0x00001000  
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED  0x00002000  
#define FILE_ATTRIBUTE_ENCRYPTED            0x00004000  
#define FILE_ATTRIBUTE_VIRTUAL              0x00010000  
#define FILE_NOTIFY_CHANGE_FILE_NAME    0x00000001   
#define FILE_NOTIFY_CHANGE_DIR_NAME     0x00000002   
#define FILE_NOTIFY_CHANGE_ATTRIBUTES   0x00000004   
#define FILE_NOTIFY_CHANGE_SIZE         0x00000008   
#define FILE_NOTIFY_CHANGE_LAST_WRITE   0x00000010   
#define FILE_NOTIFY_CHANGE_LAST_ACCESS  0x00000020   
#define FILE_NOTIFY_CHANGE_CREATION     0x00000040   
#define FILE_NOTIFY_CHANGE_SECURITY     0x00000100   
#define FILE_ACTION_ADDED                   0x00000001   
#define FILE_ACTION_REMOVED                 0x00000002   
#define FILE_ACTION_MODIFIED                0x00000003   
#define FILE_ACTION_RENAMED_OLD_NAME        0x00000004   
#define FILE_ACTION_RENAMED_NEW_NAME        0x00000005   
#define MAILSLOT_NO_MESSAGE             ((DWORD)-1) 
#define MAILSLOT_WAIT_FOREVER           ((DWORD)-1) 
#define FILE_CASE_SENSITIVE_SEARCH      0x00000001  
#define FILE_CASE_PRESERVED_NAMES       0x00000002  
#define FILE_UNICODE_ON_DISK            0x00000004  
#define FILE_PERSISTENT_ACLS            0x00000008  
#define FILE_FILE_COMPRESSION           0x00000010  
#define FILE_VOLUME_QUOTAS              0x00000020  
#define FILE_SUPPORTS_SPARSE_FILES      0x00000040  
#define FILE_SUPPORTS_REPARSE_POINTS    0x00000080  
#define FILE_SUPPORTS_REMOTE_STORAGE    0x00000100  
#define FILE_VOLUME_IS_COMPRESSED       0x00008000  
#define FILE_SUPPORTS_OBJECT_IDS        0x00010000  
#define FILE_SUPPORTS_ENCRYPTION        0x00020000  
#define FILE_NAMED_STREAMS              0x00040000  
#define FILE_READ_ONLY_VOLUME           0x00080000  
#define FILE_SEQUENTIAL_WRITE_ONCE      0x00100000  
#define FILE_SUPPORTS_TRANSACTIONS      0x00200000  

// 7839
typedef union _FILE_SEGMENT_ELEMENT {
    PVOID64 Buffer;
    ULONGLONG Alignment;
}FILE_SEGMENT_ELEMENT, *PFILE_SEGMENT_ELEMENT;

// 10206
typedef struct _TOKEN_PRIVILEGES {
    DWORD PrivilegeCount;
    LUID_AND_ATTRIBUTES Privileges[ANYSIZE_ARRAY];
} TOKEN_PRIVILEGES, *PTOKEN_PRIVILEGES;

// 11181
#define RTL_RUN_ONCE_INIT {0}   // Static initializer

// 11198
typedef union _RTL_RUN_ONCE {       // winnt ntddk
    PVOID Ptr;                      // winnt ntddk
    ULONG_PTR Value;
    struct {
        ULONG_PTR State /*: RTL_RUN_ONCE_CTX_RESERVED_BITS*/;
    };// DUMMYSTRUCTNAME;
} RTL_RUN_ONCE, *PRTL_RUN_ONCE;     // winnt ntddk

// 11246
#define HEAP_GENERATE_EXCEPTIONS        0x00000004 

// 11273
NTSYSAPI
WORD  
NTAPI
RtlCaptureStackBackTrace(
    __in DWORD FramesToSkip,
    __in DWORD FramesToCapture,
    __out_ecount(FramesToCapture) PVOID *BackTrace,
    __out_opt PDWORD BackTraceHash
   );

// 11348
NTSYSAPI
SIZE_T
NTAPI
RtlCompareMemory (
    const VOID *Source1,
    const VOID *Source2,
    SIZE_T Length
    );

// 11368
PVOID
RtlSecureZeroMemory(
    __in_bcount(cnt) PVOID ptr,
    __in SIZE_T cnt
    );

// 11630
typedef struct _RTL_SRWLOCK {
    PVOID Ptr;
    int Padding[64]; // padding to accomodate pthread_mutex_t
} RTL_SRWLOCK, *PRTL_SRWLOCK;

// 12744
typedef DWORD TP_VERSION, *PTP_VERSION;

typedef struct _TP_CALLBACK_INSTANCE TP_CALLBACK_INSTANCE, *PTP_CALLBACK_INSTANCE;

typedef VOID (NTAPI *PTP_SIMPLE_CALLBACK)(
    __inout     PTP_CALLBACK_INSTANCE Instance,
    __inout_opt PVOID                 Context
    );

typedef struct _TP_POOL TP_POOL, *PTP_POOL;

typedef enum _TP_CALLBACK_PRIORITY {
    TP_CALLBACK_PRIORITY_HIGH,
    TP_CALLBACK_PRIORITY_NORMAL,
    TP_CALLBACK_PRIORITY_LOW,
    TP_CALLBACK_PRIORITY_INVALID,
    TP_CALLBACK_PRIORITY_COUNT = TP_CALLBACK_PRIORITY_INVALID
} TP_CALLBACK_PRIORITY;

typedef struct _TP_CLEANUP_GROUP TP_CLEANUP_GROUP, *PTP_CLEANUP_GROUP;

typedef VOID (NTAPI *PTP_CLEANUP_GROUP_CANCEL_CALLBACK)(
    __inout_opt PVOID ObjectContext,
    __inout_opt PVOID CleanupContext
    );

//
// Do not manipulate this structure directly!  Allocate space for it
// and use the inline interfaces below.
//

typedef struct _TP_CALLBACK_ENVIRON_V3 {
    TP_VERSION                         Version;
    PTP_POOL                           Pool;
    PTP_CLEANUP_GROUP                  CleanupGroup;
    PTP_CLEANUP_GROUP_CANCEL_CALLBACK  CleanupGroupCancelCallback;
    PVOID                              RaceDll;
    struct _ACTIVATION_CONTEXT        *ActivationContext;
    PTP_SIMPLE_CALLBACK                FinalizationCallback;
    union {
        DWORD                          Flags;
        struct {
            DWORD                      LongFunction :  1;
            DWORD                      Persistent   :  1;
            DWORD                      Private      : 30;
        } s;
    } u;
    TP_CALLBACK_PRIORITY               CallbackPriority;
    DWORD                              Size;
} TP_CALLBACK_ENVIRON_V3;

typedef TP_CALLBACK_ENVIRON_V3 TP_CALLBACK_ENVIRON, *PTP_CALLBACK_ENVIRON;

// 12889
typedef struct _TP_WORK TP_WORK, *PTP_WORK;

typedef VOID (NTAPI *PTP_WORK_CALLBACK)(
    __inout     PTP_CALLBACK_INSTANCE Instance,
    __inout_opt PVOID                 Context,
    __inout     PTP_WORK              Work
    );

typedef struct _TP_TIMER TP_TIMER, *PTP_TIMER;

typedef VOID (NTAPI *PTP_TIMER_CALLBACK)(
    __inout     PTP_CALLBACK_INSTANCE Instance,
    __inout_opt PVOID                 Context,
    __inout     PTP_TIMER             Timer
    );

typedef DWORD    TP_WAIT_RESULT;

typedef struct _TP_WAIT TP_WAIT, *PTP_WAIT;

typedef VOID (NTAPI *PTP_WAIT_CALLBACK)(
    __inout     PTP_CALLBACK_INSTANCE Instance,
    __inout_opt PVOID                 Context,
    __inout     PTP_WAIT              Wait,
    __in        TP_WAIT_RESULT        WaitResult
    );

typedef struct _TP_IO TP_IO, *PTP_IO;

FORCEINLINE
CHAR
ReadNoFence8 (
    _In_ _Interlocked_operand_ CHAR const volatile *Source
    )
{
    CHAR Value;

    Value = *Source;
    return Value;
}

FORCEINLINE
LONG
ReadNoFence (
    _In_ _Interlocked_operand_ LONG const volatile *Source
    )
{
    LONG Value;

    Value = *Source;
    return Value;
}

FORCEINLINE
DWORD
ReadULongNoFence (
    _In_ _Interlocked_operand_ DWORD const volatile *Source
    )
{
    return (DWORD)ReadNoFence((PLONG)Source);
}

FORCEINLINE
BYTE
ReadUCharNoFence (
    _In_ _Interlocked_operand_ BYTE  const volatile *Source
    )
{
    return (BYTE )ReadNoFence8((PCHAR)Source);
}

#ifdef __cplusplus
}
#endif
