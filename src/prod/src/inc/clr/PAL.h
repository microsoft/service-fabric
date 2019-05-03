// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

/*
 * Needs to be moved to their proper headers with implementations later
 */

#pragma once


#define WINBASEAPI extern "C"

#define WINADVAPI extern "C"

// Should have Sal annotations? (sal.h)
// #define _Null_terminated
//
#define __RPC__in_string
#define __RPC__in_opt_string

typedef LONG LSTATUS;

/* ----- Taken from evenprov.h from rtcpal */
typedef ULONGLONG REGHANDLE, *PREGHANDLE;

typedef LONG SECURITY_STATUS;

/* APIENTRY is just a definition of WINAPI */
#define APIENTRY WINAPI

#define E_NOT_VALID_STATE 0x8007139f
#define E_BOUNDS 0x8000000B

// From Accctrl.h
typedef enum _SE_OBJECT_TYPE {
  SE_UNKNOWN_OBJECT_TYPE = 0,
  SE_FILE_OBJECT,
  SE_SERVICE,
  SE_PRINTER,
  SE_REGISTRY_KEY,
  SE_LMSHARE,
  SE_KERNEL_OBJECT,
  SE_WINDOW_OBJECT,
  SE_DS_OBJECT,
  SE_DS_OBJECT_ALL,
  SE_PROVIDER_DEFINED_OBJECT,
  SE_WMIGUID_OBJECT,
  SE_REGISTRY_WOW64_32KEY
} SE_OBJECT_TYPE;

// end Accctrl.h

//
//
// EVENT_DATA_DESCRIPTOR is used to pass in user data items
// in events.
//
#if !defined(UNDER_RTCPAL)
typedef struct _EVENT_DATA_DESCRIPTOR {
    ULONGLONG   Ptr;        // Pointer to data
    ULONG       Size;       // Size of data in bytes
    ULONG       Reserved;
} EVENT_DATA_DESCRIPTOR, *PEVENT_DATA_DESCRIPTOR;


#ifndef EVENT_DESCRIPTOR_DEF
#define EVENT_DESCRIPTOR_DEF
#endif

//
// EVENT_DESCRIPTOR describes and categorizes an event.
//
typedef struct _EVENT_DESCRIPTOR {

    USHORT      Id;
    UCHAR       Version;
    UCHAR       Channel;
    UCHAR       Level;
    UCHAR       Opcode;
    USHORT      Task;
    ULONGLONG   Keyword;
} EVENT_DESCRIPTOR, *PEVENT_DESCRIPTOR;

typedef const EVENT_DESCRIPTOR *PCEVENT_DESCRIPTOR;

#endif

/* File, Dir Access Permissions. From winnt.h */

// These are the generic rights.
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

typedef struct _SID_IDENTIFIER_AUTHORITY {
  BYTE Value[6];
} SID_IDENTIFIER_AUTHORITY, *PSID_IDENTIFIER_AUTHORITY;

typedef struct _SID {
    BYTE Revision;
    BYTE SubAuthorityCount;
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
} SID;

typedef struct _SID_AND_ATTRIBUTES
{
    SID *Sid;
    DWORD Attributes;
} SID_AND_ATTRIBUTES;

typedef struct _SID_AND_ATTRIBUTES * PSID_AND_ATTRIBUTES;

typedef enum _TOKEN_INFORMATION_CLASS {
    TokenUser = 1,
    TokenGroups,
    TokenPrivileges,
    TokenOwner,
    TokenPrimaryGroup,
    TokenDefaultDacl,
    TokenSource,
    TokenType,
    TokenImpersonationLevel,
    TokenStatistics,
    TokenRestrictedSids,
    TokenSessionId,
    TokenGroupsAndPrivileges,
    TokenSessionReference,
    TokenSandBoxInert,
    TokenAuditPolicy,
    TokenOrigin,
    TokenElevationType,
    TokenLinkedToken,
    TokenElevation,
    TokenHasRestrictions,
    TokenAccessInformation,
    TokenVirtualizationAllowed,
    TokenVirtualizationEnabled,
    TokenIntegrityLevel,
    TokenUIAccess,
    TokenMandatoryPolicy,
    TokenLogonSid,
    TokenIsAppContainer,
    TokenCapabilities,
    TokenAppContainerSid,
    TokenAppContainerNumber,
    TokenUserClaimAttributes,
    TokenDeviceClaimAttributes,
    TokenRestrictedUserClaimAttributes,
    TokenRestrictedDeviceClaimAttributes,
    TokenDeviceGroups,
    TokenRestrictedDeviceGroups,
    TokenSecurityAttributes,
    TokenIsRestricted,
    TokenProcessTrustLevel,
    TokenPrivateNameSpace,
    MaxTokenInfoClass  // MaxTokenInfoClass should always be the last enum
} TOKEN_INFORMATION_CLASS, *PTOKEN_INFORMATION_CLASS;

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

BOOL WINAPI CreateWellKnownSid(
  _In_       WELL_KNOWN_SID_TYPE WellKnownSidType,
  _In_opt_   PSID DomainSid,
  _Out_opt_  PSID pSid,
  _Inout_    DWORD *cbSid
  );


/*
 * Needed data structure/macro for simulating SID on Linux, i.e. LSID.
 */
#define SID_REVISION               1
#define SID_SUBAUTH_ARRAY_DOMAIN   0
#define SID_SUBAUTH_ARRAY_UID      1
#define SID_SUBAUTH_ARRAY_SIZE     2

#define SECURITY_LOCAL_SID_AUTHORITY    {0,0,0,0,0,2}
#define SECURITY_LINUX_DOMAIN_RID       (0x00000021L)
#define SECURITY_LINUX_DOMAIN_GROUP_RID (0x00000022L)

typedef struct _LSID {
	UCHAR Revision;                                     // 1
	UCHAR SubAuthorityCount;                            // 2
	SID_IDENTIFIER_AUTHORITY IdentifierAuthority;       // LOCAL
	ULONG SubAuthority[SID_SUBAUTH_ARRAY_SIZE];         // domain, uid
} LSID, *PLSID;

#define SID_REVISION                     (1)    // Current revision level
#define SID_MAX_SUB_AUTHORITIES          (15)
#define SID_RECOMMENDED_SUB_AUTHORITIES  (1)    // Will change to around 6

                                                // in a future release.
#define SECURITY_MAX_SID_SIZE  \
      (sizeof(SID) - sizeof(ULONG) + (SID_MAX_SUB_AUTHORITIES * sizeof(ULONG)))
#define PROCESS_QUERY_LIMITED_INFORMATION	(0x1000)


typedef DWORD ACCESS_MASK;

typedef ACCESS_MASK REGSAM;

#define ACL_REVISION	(2)
#define ACL_REVISION_DS	(4)

typedef ULONGLONG REGHANDLE, *PREGHANDLE;
typedef HKEY *PHKEY;

typedef struct _ACL {
    BYTE AclRevision;
    BYTE Sbz1;
    WORD AclSize;
    WORD AceCount;
    WORD Sbz2;
}ACL;

typedef struct _ACE_HEADER {
    BYTE AceType;
    BYTE AceFlags;
    WORD AceSize;
} ACE_HEADER;
typedef ACE_HEADER * PACE_HEADER;

typedef struct _ACCESS_ALLOWED_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    DWORD SidStart;
} ACCESS_ALLOWED_ACE;

typedef ACCESS_ALLOWED_ACE *PACCESS_ALLOWED_ACE;

typedef struct _ACCESS_DENIED_ACE {
    ACE_HEADER Header;
    ACCESS_MASK Mask;
    DWORD SidStart;
} ACCESS_DENIED_ACE;

typedef ACCESS_DENIED_ACE * PACCESS_DENIED_ACE;


typedef struct _ACL_SIZE_INFORMATION {
    DWORD AceCount;
    DWORD AclBytesInUse;
    DWORD AclBytesFree;
} ACL_SIZE_INFORMATION;
typedef ACL_SIZE_INFORMATION *PACL_SIZE_INFORMATION;

//TODO - waa
// PACL is defined as typedef void * PACL in the palrt.h file
// the definition here should be the real one
//typedef ACL * PACL;

#define ACCESS_ALLOWED_ACE_TYPE		(0x0)
#define ACCESS_DENIED_ACE_TYPE		(0x1)

typedef enum _ACL_INFORMATION_CLASS {
    AclRevisionInformation = 1,
    AclSizeInformation
} ACL_INFORMATION_CLASS;

typedef enum _JOBOBJECTINFOCLASS {
// TODO -waa
// fill this in with all the values
    JobObjectBasicAccountingInformation = 1,
    JobObjectExtendedLimitInformation = 9,
    JobObjectReserved1Information = 18,
    MaxJobObjectInfoClass
} JOBOBJECTINFOCLASS;

//FORCEINLINE
PVOID
RtlSecureZeroMemory(
    PVOID ptr,
    SIZE_T cnt
    );

#define OBJECT_INHERIT_ACE                (0x1)
#define CONTAINER_INHERIT_ACE             (0x2)
#define NO_PROPAGATE_INHERIT_ACE          (0x4)
#define INHERIT_ONLY_ACE                  (0x8)
#define INHERITED_ACE                     (0x10)
#define VALID_INHERIT_FLAGS               (0x1F)


typedef DWORD SECURITY_INFORMATION, *PSECURITY_INFORMATION;

#define OWNER_SECURITY_INFORMATION       (0x00000001L)
#define GROUP_SECURITY_INFORMATION       (0x00000002L)
#define DACL_SECURITY_INFORMATION        (0x00000004L)
#define SACL_SECURITY_INFORMATION        (0x00000008L)
#define LABEL_SECURITY_INFORMATION       (0x00000010L)
#define ATTRIBUTE_SECURITY_INFORMATION   (0x00000020L)
#define SCOPE_SECURITY_INFORMATION       (0x00000040L)
#define PROCESS_TRUST_LABEL_SECURITY_INFORMATION (0x00000080L)
#define BACKUP_SECURITY_INFORMATION      (0x00010000L)

#define PROTECTED_DACL_SECURITY_INFORMATION     (0x80000000L)
#define PROTECTED_SACL_SECURITY_INFORMATION     (0x40000000L)
#define UNPROTECTED_DACL_SECURITY_INFORMATION   (0x20000000L)
#define UNPROTECTED_SACL_SECURITY_INFORMATION   (0x10000000L)


typedef WORD   SECURITY_DESCRIPTOR_CONTROL, *PSECURITY_DESCRIPTOR_CONTROL;

#define SE_OWNER_DEFAULTED               (0x0001)
#define SE_GROUP_DEFAULTED               (0x0002)
#define SE_DACL_PRESENT                  (0x0004)
#define SE_DACL_DEFAULTED                (0x0008)
#define SE_SACL_PRESENT                  (0x0010)
#define SE_SACL_DEFAULTED                (0x0020)
#define SE_DACL_AUTO_INHERIT_REQ         (0x0100)
#define SE_SACL_AUTO_INHERIT_REQ         (0x0200)
#define SE_DACL_AUTO_INHERITED           (0x0400)
#define SE_SACL_AUTO_INHERITED           (0x0800)
#define SE_DACL_PROTECTED                (0x1000)
#define SE_SACL_PROTECTED                (0x2000)
#define SE_RM_CONTROL_VALID              (0x4000)
#define SE_SELF_RELATIVE                 (0x8000)
//typedef USHORT SECURITY_DESCRIPTOR_CONTROL;
//typedef USHORT* PSECURITY_DESCRIPTOR_CONTROL;

BOOL WINAPI GetSecurityDescriptorControl(
  _In_   PSECURITY_DESCRIPTOR pSecurityDescriptor,
  _Out_  PSECURITY_DESCRIPTOR_CONTROL pControl,
  _Out_  LPDWORD lpdwRevision
  );

DWORD
WINAPI
GetNamedSecurityInfoA(
    _In_  LPCSTR               pObjectName,
    _In_  SE_OBJECT_TYPE         ObjectType,
    _In_  SECURITY_INFORMATION   SecurityInfo,
    _Out_opt_       PSID         * ppsidOwner,
    _Out_opt_       PSID         * ppsidGroup,
    _Out_opt_       PACL         * ppDacl,
    _Out_opt_       PACL         * ppSacl,
    _Out_ PSECURITY_DESCRIPTOR   * ppSecurityDescriptor
    );

DWORD
WINAPI
GetNamedSecurityInfoW(
    _In_  LPCWSTR               pObjectName,
    _In_  SE_OBJECT_TYPE         ObjectType,
    _In_  SECURITY_INFORMATION   SecurityInfo,
    _Out_opt_       PSID         * ppsidOwner,
    _Out_opt_       PSID         * ppsidGroup,
    _Out_opt_       PACL         * ppDacl,
    _Out_opt_       PACL         * ppSacl,
    _Out_ PSECURITY_DESCRIPTOR   * ppSecurityDescriptor
    );
#ifdef UNICODE
#define GetNamedSecurityInfo  GetNamedSecurityInfoW
#else
#define GetNamedSecurityInfo  GetNamedSecurityInfoA
#endif // !UNICODE

DWORD
WINAPI
SetNamedSecurityInfoA(
    _In_ LPSTR               pObjectName,
    _In_ SE_OBJECT_TYPE        ObjectType,
    _In_ SECURITY_INFORMATION  SecurityInfo,
    _In_opt_ PSID              psidOwner,
    _In_opt_ PSID              psidGroup,
    _In_opt_ PACL              pDacl,
    _In_opt_ PACL              pSacl
    );

DWORD
WINAPI
SetNamedSecurityInfoW(
    _In_ LPWSTR               pObjectName,
    _In_ SE_OBJECT_TYPE        ObjectType,
    _In_ SECURITY_INFORMATION  SecurityInfo,
    _In_opt_ PSID              psidOwner,
    _In_opt_ PSID              psidGroup,
    _In_opt_ PACL              pDacl,
    _In_opt_ PACL              pSacl
    );
#ifdef UNICODE
#define SetNamedSecurityInfo  SetNamedSecurityInfoW
#else
#define SetNamedSecurityInfo  SetNamedSecurityInfoA
#endif // !UNICODE

BOOL WINAPI GetSecurityDescriptorDacl(
  _In_   PSECURITY_DESCRIPTOR pSecurityDescriptor,
  _Out_  LPBOOL lpbDaclPresent,
  _Out_  PACL *pDacl,
  _Out_  LPBOOL lpbDaclDefaulted
  );

BOOL WINAPI SetSecurityDescriptorDacl(
  _Inout_   PSECURITY_DESCRIPTOR pSecurityDescriptor,
  _In_      BOOL bDaclPresent,
  _In_opt_  PACL pDacl,
  _In_      BOOL bDaclDefaulted
);

BOOL WINAPI SetKernelObjectSecurity(
  _In_  HANDLE Handle,
  _In_  SECURITY_INFORMATION SecurityInformation,
  _In_  PSECURITY_DESCRIPTOR SecurityDescriptor
  );

#define GENERIC_READ		(0x80000000L)
#define GENERIC_WRITE		(0x40000000L)
#define GENERIC_EXECUTE		(0x20000000L)
#define GENERIC_ALL		(0x10000000L)


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

//
//  The following are masks for the predefined standard access types
//  From winnt.h
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
#define FILE_CASE_SENSITIVE_SEARCH          0x00000001
#define FILE_CASE_PRESERVED_NAMES           0x00000002
#define FILE_UNICODE_ON_DISK                0x00000004
#define FILE_PERSISTENT_ACLS                0x00000008
#define FILE_FILE_COMPRESSION               0x00000010
#define FILE_VOLUME_QUOTAS                  0x00000020
#define FILE_SUPPORTS_SPARSE_FILES          0x00000040
#define FILE_SUPPORTS_REPARSE_POINTS        0x00000080
#define FILE_SUPPORTS_REMOTE_STORAGE        0x00000100
#define FILE_VOLUME_IS_COMPRESSED           0x00008000
#define FILE_SUPPORTS_OBJECT_IDS            0x00010000
#define FILE_SUPPORTS_ENCRYPTION            0x00020000
#define FILE_NAMED_STREAMS                  0x00040000
#define FILE_READ_ONLY_VOLUME               0x00080000
#define FILE_SEQUENTIAL_WRITE_ONCE          0x00100000
#define FILE_SUPPORTS_TRANSACTIONS          0x00200000
#define FILE_SUPPORTS_HARD_LINKS            0x00400000
#define FILE_SUPPORTS_EXTENDED_ATTRIBUTES   0x00800000
#define FILE_SUPPORTS_OPEN_BY_FILE_ID       0x01000000
#define FILE_SUPPORTS_USN_JOURNAL           0x02000000

#define HEAP_GENERATE_EXCEPTIONS        0x00000004      // winnt

// Locking
typedef struct _RTL_SRWLOCK {
    PVOID Ptr;
    int Padding[64];  // padding to accomodate pthread_mutex_t
} RTL_SRWLOCK, *PRTL_SRWLOCK;

typedef struct _RTL_CONDITION_VARIABLE {
    PVOID Ptr;
} RTL_CONDITION_VARIABLE, *PRTL_CONDITION_VARIABLE;


// Threadpool related definitions from WinNt.h from Azure Windows SDK 8.1
//
// Do not manipulate this structure directly!  Allocate space for it
// and use the inline interfaces below.
//
typedef DWORD TP_VERSION, *PTP_VERSION;
typedef struct _TP_CALLBACK_INSTANCE TP_CALLBACK_INSTANCE, *PTP_CALLBACK_INSTANCE;
typedef struct _TP_WORK TP_WORK, *PTP_WORK;
typedef struct _TP_POOL TP_POOL, *PTP_POOL;
typedef struct _TP_CLEANUP_GROUP TP_CLEANUP_GROUP, *PTP_CLEANUP_GROUP;
typedef struct _TP_CALLBACK_ENVIRON_V3 *PTP_CALLBACK_ENVIRON;

typedef VOID (NTAPI *PTP_WORK_CALLBACK)(
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context,
    _Inout_     PTP_WORK              Work
    );

typedef struct _TP_WORK
{
    PVOID CallbackParameter;
    PTP_WORK_CALLBACK WorkCallback;
    PTP_CALLBACK_ENVIRON CallbackEnvironment;
} TP_WORK, *PTP_WORK;

typedef struct _TP_CALLBACK_INSTANCE
{
    PTP_WORK Work;
} TP_CALLBACK_INSTANCE, *PTP_CALLBACK_INSTANCE;

typedef struct _wArrayList wArrayList;
typedef struct _wQueue wQueue;
typedef struct _wCountdownEvent wCountdownEvent;

typedef struct _TP_POOL
{
    DWORD Minimum;
    DWORD Maximum;
    wArrayList* Threads;
    wQueue* PendingQueue;
    HANDLE TerminateEvent;
    wCountdownEvent* WorkComplete;
} TP_POOL, *PTP_POOL;

typedef struct _TP_TIMER TP_TIMER, *PTP_TIMER;

typedef DWORD TP_WAIT_RESULT;

typedef struct _TP_WAIT TP_WAIT, *PTP_WAIT;

typedef VOID (NTAPI *PTP_WAIT_CALLBACK)(
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context,
    _Inout_     PTP_WAIT              Wait,
    _In_        TP_WAIT_RESULT        WaitResult
    );

typedef VOID (NTAPI *PTP_SIMPLE_CALLBACK)(
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context
    );

typedef enum _TP_CALLBACK_PRIORITY {
    TP_CALLBACK_PRIORITY_HIGH,
    TP_CALLBACK_PRIORITY_NORMAL,
    TP_CALLBACK_PRIORITY_LOW,
    TP_CALLBACK_PRIORITY_INVALID,
    TP_CALLBACK_PRIORITY_COUNT = TP_CALLBACK_PRIORITY_INVALID
} TP_CALLBACK_PRIORITY;

typedef VOID (NTAPI *PTP_CLEANUP_GROUP_CANCEL_CALLBACK)(
            _Inout_opt_ PVOID ObjectContext,
            _Inout_opt_ PVOID CleanupContext
    );

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

typedef VOID (NTAPI *PTP_TIMER_CALLBACK)(
    PTP_CALLBACK_INSTANCE Instance,
    PVOID Context,
    PTP_TIMER	Timer
    );

WINBASEAPI
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
CloseThreadpoolWork(
    __inout PTP_WORK pwk
    );

WINBASEAPI
VOID
WINAPI
CloseThreadpool(
    __inout PTP_POOL ptpp
    );

WINBASEAPI
PTP_POOL
WINAPI
CreateThreadpool(
    __reserved PVOID reserved
    );

WINBASEAPI
VOID
WINAPI
SetThreadpoolThreadMaximum(
    __inout PTP_POOL ptpp,
    __in    DWORD    cthrdMost
    );

WINBASEAPI
BOOL
WINAPI
SetThreadpoolThreadMinimum(
    __inout PTP_POOL ptpp,
    __in    DWORD    cthrdMic
    );

WINBASEAPI
VOID
WINAPI
SubmitThreadpoolWork(
    __inout PTP_WORK pwk
    );

VOID
WINAPI
WaitForThreadpoolWorkCallbacks(
    _Inout_ PTP_WORK pwk,
    _In_ BOOL fCancelPendingCallbacks
    );


/* winnt.h - END */

//
// From winbase.h
//
#define SecureZeroMemory RtlSecureZeroMemory

#define CREATE_SUSPENDED		0x00000004

#define CREATE_NEW_CONSOLE		0x00000010
#define CREATE_UNICODE_ENVIRONMENT	0x00000400
#define CREATE_NEW_PROCESS_GROUP	0x00000200
#define CREATE_BREAKAWAY_FROM_JOB	0x01000000
#define CREATE_NO_WINDOW		0x08000000

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
/* MOVEFILE_WRITE_THROUGH : The function does not return until the file has
 * actually been moved on the disk. Setting this flag guarantees that a move
 * perfomed as a copy and delete operation is flushed to disk  before the
 * function returns. */
#define MOVEFILE_WRITE_THROUGH          0x00000008

/* Avoiding redefining 'new' here since KTL in kallocator.h defines the same and
 * there is a race condition to get to it. This is yanked from pal.h. Maybe need
 * to be moved closer to top */
#ifdef __cplusplus
#if !defined(__PLACEMENT_NEW_INLINE)
// This is already defined in llvm libc++ /usr/includ/c++/v1/new no need to
// redefine again. But have to guard redefinition in KTL and elsewhere with the
// below define
#define __PLACEMENT_NEW_INLINE
#endif // __PLACEMENT_NEW_INLINE
#endif

BOOL WINAPI OpenProcessToken(
  _In_   HANDLE ProcessHandle,
  _In_   DWORD DesiredAccess,
  _Out_  PHANDLE TokenHandle
  );

BOOL WINAPI GetTokenInformation(
  _In_       HANDLE TokenHandle,
  _In_       TOKEN_INFORMATION_CLASS TokenInformationClass,
  _Out_opt_  LPVOID TokenInformation,
  _In_       DWORD TokenInformationLength,
  _Out_      PDWORD ReturnLength
  );

#ifndef NTSTATUS
typedef LONG NTSTATUS;
#endif

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

#ifndef NT_ERROR
#define NT_ERROR(Status) (((NTSTATUS)(Status)) < 0)
#endif

WINBASEAPI
//_Success_(return != 0)
BOOL
WINAPI
GetComputerNameA (
    LPSTR lpBuffer,
    LPDWORD nSize
    );

//_Success_(return != 0)
BOOL
WINAPI
GetComputerNameW (
   LPWSTR lpBuffer,
    LPDWORD nSize
    );

#ifdef UNICODE
#define GetComputerName  GetComputerNameW
#else
#define GetComputerName  GetComputerNameA
#endif // !UNICODE

/* WinBase.h - END */

#define _TRUNCATE ((size_t)-1)
#define STRUNCATE       80

extern "C" int wcsncpy_s(wchar_t* dst, size_t dsize, wchar_t const* src, size_t ssize);

// winerror.h
#define ERROR_FILE_NOT_FOUND             2L
#define ERROR_PATH_NOT_FOUND             3L
#define ERROR_ACCESS_DENIED              5L
#define ERROR_NOT_ENOUGH_MEMORY          8L
#define ERROR_OUTOFMEMORY                14L
#define ERROR_CURRENT_DIRECTORY          16L
#define ERROR_NETWORK_ACCESS_DENIED      65L
#define ERROR_INVALID_PASSWORD           86L
#define ERROR_OPEN_FAILED                110L
#define ERROR_DISK_FULL                  112L
#define ERROR_MR_MID_NOT_FOUND           317L
#define ERROR_ASSERTION_FAILURE          668L
#define ERROR_NONE_MAPPED                1332L
#define ERROR_RETRY                      1237L
#define ERROR_REVISION_MISMATCH          1306L
#define ERROR_NO_SUCH_USER               1317L
#define ERROR_LOGON_FAILURE              1326L
#define ERROR_PASSWORD_EXPIRED           1330L
#define ERROR_ABANDONED_WAIT_0           735L
#define ERROR_NET_OPEN_FAILED            570L
#define ERROR_ALREADY_ASSIGNED           85L
#define ERROR_SEEK                       25L


// Winnt.h // // Token Specific Access Rights.
//

#define TOKEN_ASSIGN_PRIMARY    (0x0001)
#define TOKEN_DUPLICATE         (0x0002)
#define TOKEN_IMPERSONATE       (0x0004)
#define TOKEN_QUERY             (0x0008)
#define TOKEN_QUERY_SOURCE      (0x0010)
#define TOKEN_ADJUST_PRIVILEGES (0x0020)
#define TOKEN_ADJUST_GROUPS     (0x0040)
#define TOKEN_ADJUST_DEFAULT    (0x0080)
#define TOKEN_ADJUST_SESSIONID  (0x0100)

#define TOKEN_ALL_ACCESS_P (STANDARD_RIGHTS_REQUIRED  |\
                          TOKEN_ASSIGN_PRIMARY      |\
                          TOKEN_DUPLICATE           |\
                          TOKEN_IMPERSONATE         |\
                          TOKEN_QUERY               |\
                          TOKEN_QUERY_SOURCE        |\
                          TOKEN_ADJUST_PRIVILEGES   |\
                          TOKEN_ADJUST_GROUPS       |\
                          TOKEN_ADJUST_DEFAULT )

#if ((defined(_WIN32_WINNT) && (_WIN32_WINNT > 0x0400)) || (!defined(_WIN32_WINNT)))
#define TOKEN_ALL_ACCESS  (TOKEN_ALL_ACCESS_P |\
                          TOKEN_ADJUST_SESSIONID )
#else
#define TOKEN_ALL_ACCESS  (TOKEN_ALL_ACCESS_P)
#endif

#define TOKEN_READ       (STANDARD_RIGHTS_READ      |\
                          TOKEN_QUERY)


#define TOKEN_WRITE      (STANDARD_RIGHTS_WRITE     |\
                          TOKEN_ADJUST_PRIVILEGES   |\
                          TOKEN_ADJUST_GROUPS       |\
                          TOKEN_ADJUST_DEFAULT)

#define TOKEN_EXECUTE    (STANDARD_RIGHTS_EXECUTE)

typedef struct _TOKEN_USER {
    SID_AND_ATTRIBUTES User;
} TOKEN_USER, *PTOKEN_USER;


#define RTLP_RUN_ONCE_START         0x00UL
#define RTLP_RUN_ONCE_INITIALIZING  0x01UL
#define RTLP_RUN_ONCE_COMPLETE      0x02UL
#define RTLP_RUN_ONCE_NONBLOCKING   0x03UL
#define RTL_RUN_ONCE_CTX_RESERVED_BITS 2

typedef union _RTL_RUN_ONCE {       // winnt ntddk
    PVOID Ptr;                      // winnt ntddk
    ULONG_PTR Value;
    struct {
        ULONG_PTR State /*: RTL_RUN_ONCE_CTX_RESERVED_BITS*/;
    };// DUMMYSTRUCTNAME;
} RTL_RUN_ONCE, *PRTL_RUN_ONCE;     // winnt ntddk

// Winbase.h One time initialization primitives
typedef RTL_RUN_ONCE INIT_ONCE; // pthread_once() may be the equivalent
typedef PRTL_RUN_ONCE PINIT_ONCE;
typedef BOOL (WINAPI *PINIT_ONCE_FN) (
        __inout PINIT_ONCE InitOnce,
        __inout_opt PVOID Parameter,
        __deref_opt_out_opt PVOID *Context);

WINBASEAPI
BOOL
WINAPI
InitOnceExecuteOnce(
    _Inout_      PINIT_ONCE InitOnce,
    _In_         PINIT_ONCE_FN InitFn,
    _Inout_opt_  PVOID Parameter,
    _Out_opt_    LPVOID *Context
);

WINBASEAPI
BOOL
WINAPI
RaiseFailFastException(
    _Inout_opt_ PEXCEPTION_RECORD pExceptionRecord,
    _Inout_opt_ PCONTEXT pContextRecord,
    _In_        DWORD dwFlags
);

// From Windows SDK subauth.h. Needed for ktl
#define STATUS_SUCCESS                   ((NTSTATUS)0x00000000L)
#define STATUS_INVALID_INFO_CLASS        ((NTSTATUS)0xC0000003L)
#define STATUS_NO_SUCH_USER              ((NTSTATUS)0xC0000064L)
#define STATUS_WRONG_PASSWORD            ((NTSTATUS)0xC000006AL)
#define STATUS_PASSWORD_RESTRICTION      ((NTSTATUS)0xC000006CL)
#define STATUS_LOGON_FAILURE             ((NTSTATUS)0xC000006DL)
#define STATUS_ACCOUNT_RESTRICTION       ((NTSTATUS)0xC000006EL)
#define STATUS_INVALID_LOGON_HOURS       ((NTSTATUS)0xC000006FL)
#define STATUS_INVALID_WORKSTATION       ((NTSTATUS)0xC0000070L)
#define STATUS_PASSWORD_EXPIRED          ((NTSTATUS)0xC0000071L)
#define STATUS_ACCOUNT_DISABLED          ((NTSTATUS)0xC0000072L)
#define STATUS_INSUFFICIENT_RESOURCES    ((NTSTATUS)0xC000009AL)
#define STATUS_ACCOUNT_EXPIRED           ((NTSTATUS)0xC0000193L)
#define STATUS_PASSWORD_MUST_CHANGE      ((NTSTATUS)0xC0000224L)
#define STATUS_ACCOUNT_LOCKED_OUT        ((NTSTATUS)0xC0000234L)
#define STATUS_UNSUCCESSFUL              ((NTSTATUS)0xC0000001L)
#define STATUS_INVALID_PARAMETER         ((NTSTATUS)0xC000000DL)
#define STATUS_NO_MATCH                  ((NTSTATUS)0xC0000272L)
#define STATUS_NO_MORE_MATCHES           ((NTSTATUS)0xC0000273L)

#define STATUS_DATA_ERROR                ((NTSTATUS)0xC000003EL)
#define STATUS_ABANDONED                 ((NTSTATUS)0x00000080L)
#define STATUS_OBJECT_NO_LONGER_EXISTS   ((NTSTATUS)0xC0190021L)

#define STATUS_NAME_TOO_LONG             ((NTSTATUS)0xC0000106L)
#define STATUS_LOG_FULL                  ((NTSTATUS)0xC01A001DL)
#define STATUS_FILE_CLOSED               ((NTSTATUS)0xC0000128L)

#define STATUS_QUOTA_EXCEEDED             ((NTSTATUS)0xC0000044L)
#define STATUS_FILE_IS_A_DIRECTORY        ((NTSTATUS)0xC00000BAL)
#define STATUS_REPARSE_POINT_NOT_RESOLVED ((NTSTATUS)0xC0000280L)
#define STATUS_NO_SUCH_FILE              ((NTSTATUS)0xC000000FL)
#define STATUS_PIPE_NOT_AVAILABLE        ((NTSTATUS)0xC00000ACL)
#define STATUS_INSTANCE_NOT_AVAILABLE    ((NTSTATUS)0xC00000ABL)
#define STATUS_INVALID_PIPE_STATE        ((NTSTATUS)0xC00000ADL)
#define STATUS_PIPE_BUSY                 ((NTSTATUS)0xC00000AEL)
#define STATUS_ILLEGAL_FUNCTION          ((NTSTATUS)0xC00000AFL)
#define STATUS_PIPE_DISCONNECTED         ((NTSTATUS)0xC00000B0L)
#define STATUS_PIPE_CLOSING              ((NTSTATUS)0xC00000B1L)
#define STATUS_PIPE_CONNECTED            ((NTSTATUS)0xC00000B2L)
#define STATUS_PIPE_LISTENING            ((NTSTATUS)0xC00000B3L)
#define STATUS_INVALID_READ_MODE         ((NTSTATUS)0xC00000B4L)
#define STATUS_PIPE_BROKEN               ((NTSTATUS)0xC000014BL)
#define STATUS_PIPE_EMPTY                ((NTSTATUS)0xC00000D9L)
#define STATUS_ENCOUNTERED_WRITE_IN_PROGRESS    ((NTSTATUS)0xC0000433)
#define STATUS_RESOURCE_IN_USE          ((NTSTATUS)0xC0000708)
#define STATUS_OPEN_FAILED              ((NTSTATUS)0xC0000136)
#define STATUS_UNEXPECTED_IO_ERROR       ((NTSTATUS)0xC00000E9L)
#define STATUS_NO_MORE_FILES             ((NTSTATUS)0x80000006L)
#define STATUS_DISK_FULL                 ((NTSTATUS)0xC000007FL)

// From Windows sdk ntstatus.h
// The object was not found.
#define STATUS_NOT_FOUND                 ((NTSTATUS)0xC0000225L)
// An attempt was made to create an object and the object name already existed.
#define STATUS_OBJECT_NAME_EXISTS        ((NTSTATUS)0x40000000L)
// Object Name already exists.
#define STATUS_OBJECT_NAME_COLLISION     ((NTSTATUS)0xC0000035L)
// No more entries are available from an enumeration operation.
#define STATUS_NO_MORE_ENTRIES           ((NTSTATUS)0x8000001AL)
// The end of the media was encountered.
#define STATUS_END_OF_MEDIA              ((NTSTATUS)0x8000001EL)
// Beginning of tape or partition has been detected.
#define STATUS_BEGINNING_OF_MEDIA        ((NTSTATUS)0x8000001FL)

#define STATUS_BUFFER_OVERFLOW           ((NTSTATUS)0x80000005L)

#define STATUS_INVALID_LOCK_SEQUENCE     ((NTSTATUS)0xC000001EL)
#define STATUS_FILE_FORCED_CLOSED        ((NTSTATUS)0xC00000B6L)
#define STATUS_INSUFF_SERVER_RESOURCES   ((NTSTATUS)0xC0000205L)

#define STATUS_NOT_IMPLEMENTED           ((NTSTATUS)0xC0000002L)
#define STATUS_CANCELLED                 ((NTSTATUS)0xC0000120L)
#define STATUS_TIMEOUT                   ((NTSTATUS)0x00000102L)
#define STATUS_ACCESS_DENIED             ((NTSTATUS)0xC0000022L)

#define STATUS_OBJECT_PATH_SYNTAX_BAD    ((NTSTATUS)0xC000003BL)
#define STATUS_INTERNAL_ERROR            ((NTSTATUS)0xC00000E5L)
#define STATUS_PENDING                   ((NTSTATUS)0x00000103L)    // winnt
#define STATUS_SHUTDOWN_IN_PROGRESS      ((NTSTATUS)0xC00002FEL)
#define STATUS_OBJECT_NAME_NOT_FOUND     ((NTSTATUS)0xC0000034L)
#define STATUS_OBJECT_TYPE_MISMATCH      ((NTSTATUS)0xC0000024L)
#define STATUS_NOT_SUPPORTED             ((NTSTATUS)0xC00000BBL)
#define STATUS_INFO_LENGTH_MISMATCH      ((NTSTATUS)0xC0000004L)

#define STATUS_BUFFER_TOO_SMALL          ((NTSTATUS)0xC0000023L)

#define STATUS_DISK_FULL                 ((NTSTATUS)0xC000007FL)
#define STATUS_OPEN_FAILED               ((NTSTATUS)0xC0000136L)
#define STATUS_DELETE_PENDING            ((NTSTATUS)0xC0000056L)
#define STATUS_SHARING_VIOLATION         ((NTSTATUS)0xC0000043L)
#define STATUS_NOT_IMPLEMENTED           ((NTSTATUS)0xC0000002L)
#define STATUS_DATA_ERROR                ((NTSTATUS)0xC000003EL)
#define STATUS_CRC_ERROR                 ((NTSTATUS)0xC000003FL)
#define STATUS_CANCELLED                 ((NTSTATUS)0xC0000120L)
#define STATUS_OBJECT_NAME_INVALID       ((NTSTATUS)0xC0000033L)
#define STATUS_BUFFER_TOO_SMALL          ((NTSTATUS)0xC0000023L)
#define STATUS_LOG_FULL                  ((NTSTATUS)0xC01A001DL)
#define STATUS_BUFFER_OVERFLOW           ((NTSTATUS)0x80000005L)
#define STATUS_DEVICE_CONFIGURATION_ERROR ((NTSTATUS)0xC0000182L)
#define STATUS_OBJECT_NO_LONGER_EXISTS   ((NTSTATUS)0xC0190021L)
#define STATUS_IO_TIMEOUT                ((NTSTATUS)0xC00000B5L)
#define STATUS_INVALID_BUFFER_SIZE       ((NTSTATUS)0xC0000206L)
#define STATUS_OBJECT_PATH_INVALID       ((NTSTATUS)0xC0000039L)
#define STATUS_NAME_TOO_LONG             ((NTSTATUS)0xC0000106L)
#define STATUS_REQUEST_OUT_OF_SEQUENCE   ((NTSTATUS)0xC000042AL)

#define STATUS_ALERTED                   ((NTSTATUS)0x00000101L)
#define STATUS_INVALID_DEVICE_REQUEST    ((NTSTATUS)0xC0000010L)
#define STATUS_FILE_CLOSED               ((NTSTATUS)0xC0000128L)
#define STATUS_RESOURCE_NOT_OWNED        ((NTSTATUS)0xC0000264L)
#define STATUS_NONEXISTENT_SECTOR        ((NTSTATUS)0xC0000015L)
#define STATUS_OBJECT_PATH_NOT_FOUND     ((NTSTATUS)0xC000003AL)
#define STATUS_INVALID_PARAMETER_MIX     ((NTSTATUS)0xC0000030L)
#define STATUS_END_OF_FILE               ((NTSTATUS)0xC0000011L)
#define STATUS_MORE_ENTRIES              ((NTSTATUS)0x00000105L)
#define STATUS_UNRECOGNIZED_VOLUME       ((NTSTATUS)0xC000014FL)
#define STATUS_NOT_A_DIRECTORY           ((NTSTATUS)0xC0000103L)
#define STATUS_SOURCE_ELEMENT_EMPTY      ((NTSTATUS)0xC0000283L)
#define STATUS_PRIVILEGE_NOT_HELD        ((NTSTATUS)0xC0000061L)
#define STATUS_NO_SUCH_DEVICE            ((NTSTATUS)0xC000000EL)
#define STATUS_FILE_TOO_LARGE            ((NTSTATUS)0xC0000904L)
#define STATUS_FILE_INVALID              ((NTSTATUS)0xC0000098L)
#define STATUS_NO_DATA_DETECTED          ((NTSTATUS)0x80000022L)
#define STATUS_RANGE_NOT_FOUND           ((NTSTATUS)0xC000028CL)
#define STATUS_REVISION_MISMATCH         ((NTSTATUS)0xC0000059L)

#define STATUS_ABANDONED_WAIT_0          ((NTSTATUS)0x00000080L)
#define STATUS_ABANDONED_WAIT_63         ((NTSTATUS)0x000000BFL)
#define STATUS_WAIT_63                   ((NTSTATUS)0x0000003FL)
#define STATUS_WORKING_SET_QUOTA         ((NTSTATUS)0xC00000A1L)
#define STATUS_IO_DEVICE_ERROR           ((NTSTATUS)0xC0000185L)
#define STATUS_DIRECTORY_NOT_EMPTY       ((NTSTATUS)0xC0000101L)
#define STATUS_ABIOS_INVALID_COMMAND     ((NTSTATUS)0xC0000113L)
#define STATUS_INTERNAL_DB_ERROR         ((NTSTATUS)0xC0000158L)

#define STATUS_OBJECT_PATH_SYNTAX_BAD    ((NTSTATUS)0xC000003BL)
#define STATUS_INTERNAL_ERROR            ((NTSTATUS)0xC00000E5L)
#define STATUS_PENDING                   ((NTSTATUS)0x00000103L)    // winnt
#define STATUS_SHUTDOWN_IN_PROGRESS      ((NTSTATUS)0xC00002FEL)
#define STATUS_OBJECT_NAME_NOT_FOUND     ((NTSTATUS)0xC0000034L)
#define STATUS_OBJECT_TYPE_MISMATCH      ((NTSTATUS)0xC0000024L)
#define STATUS_NOT_SUPPORTED             ((NTSTATUS)0xC00000BBL)
#define STATUS_INFO_LENGTH_MISMATCH      ((NTSTATUS)0xC0000004L)
#define STATUS_INVALID_STATE_TRANSITION  ((NTSTATUS)0xC000A003L)
#define STATUS_INSUFFICIENT_POWER        ((NTSTATUS)0xC00002DEL)
#define STATUS_INTERNAL_DB_CORRUPTION    ((NTSTATUS)0xC00000E4L)
#define STATUS_MEMORY_NOT_ALLOCATED      ((NTSTATUS)0xC00000A0L)
#define STATUS_INVALID_PARAMETER_1       ((NTSTATUS)0xC00000EFL)
#define STATUS_INVALID_PARAMETER_2       ((NTSTATUS)0xC00000F0L)
#define STATUS_INVALID_PARAMETER_3       ((NTSTATUS)0xC00000F1L)
#define STATUS_INVALID_PARAMETER_4       ((NTSTATUS)0xC00000F2L)
#define STATUS_INVALID_PARAMETER_5       ((NTSTATUS)0xC00000F3L)
#define STATUS_INVALID_PARAMETER_6       ((NTSTATUS)0xC00000F4L)
#define STATUS_INVALID_PARAMETER_7       ((NTSTATUS)0xC00000F5L)
#define STATUS_INVALID_PARAMETER_8       ((NTSTATUS)0xC00000F6L)
#define STATUS_INVALID_PARAMETER_9       ((NTSTATUS)0xC00000F7L)
#define STATUS_INVALID_PARAMETER_10      ((NTSTATUS)0xC00000F8L)
#define STATUS_INVALID_PARAMETER_11      ((NTSTATUS)0xC00000F9L)
#define STATUS_INVALID_PARAMETER_12      ((NTSTATUS)0xC00000FAL)
#define STATUS_DISK_CORRUPT_ERROR        ((NTSTATUS)0xC0000032L)

#define VER_PRODUCTBUILD            /* NT */   10011

typedef CONST UCHAR *PCUCHAR;
typedef CONST USHORT *PCUSHORT;

inline VOID __movsb (
        PUCHAR Destination,
        PCUCHAR Source,
        SIZE_T Count
)
{
    SIZE_T i;

    for (i = 0; i < Count; ++i) {
        Destination[i] = Source[i];
    }

    return;
}

void __stosq(
        unsigned __int64* Dest,
        unsigned __int64 Data,
        size_t Count
);

static __inline__ void
__stosq(unsigned __int64 *__dst, unsigned __int64 __x, size_t __n) {
  __asm__("rep stosq" : : "D"(__dst), "a"(__x), "c"(__n));
}

#define OBJ_INHERIT                         0x00000002L
#define OBJ_PERMANENT                       0x00000010L
#define OBJ_EXCLUSIVE                       0x00000020L
#define OBJ_CASE_INSENSITIVE                0x00000040L
#define OBJ_OPENIF                          0x00000080L
#define OBJ_OPENLINK                        0x00000100L
#define OBJ_KERNEL_HANDLE                   0x00000200L
#define OBJ_FORCE_ACCESS_CHECK              0x00000400L
#define OBJ_IGNORE_IMPERSONATED_DEVICEMAP   0x00000800L
#define OBJ_DONT_REPARSE                    0x00001000L
#define OBJ_VALID_ATTRIBUTES                0x00001FF2L

#define InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( OBJECT_ATTRIBUTES );          \
    (p)->RootDirectory = r;                             \
    (p)->Attributes = a;                                \
    (p)->ObjectName = n;                                \
    /*(p)->SecurityDescriptor = s;                        \*/\
    (p)->SecurityQualityOfService = NULL;               \
    }


ULONG RtlRandomEx(IN OUT PULONG Seed);
BOOL DeviceIoControl(_In_ HANDLE hDevice, _In_ DWORD dwIoControlCode, _In_reads_bytes_opt_(nInBufferSize) LPVOID lpInBuffer,
                     _In_ DWORD nInBufferSize, _Out_writes_bytes_to_opt_(nOutBufferSize, *lpBytesReturned) LPVOID lpOutBuffer, _In_ DWORD nOutBufferSize,
                     _Out_opt_ LPDWORD lpBytesReturned, _Inout_opt_ LPOVERLAPPED lpOverlapped);
#ifndef CTL_CODE
#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)
#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3
#endif

#define FILE_ANY_ACCESS                 0
#define FILE_READ_ACCESS          ( 0x0001 )    // file & pipe
#define FILE_WRITE_ACCESS         ( 0x0002 )    // file & pipe

typedef struct _TOKEN_GROUPS {
    ULONG GroupCount;
#ifdef MIDL_PASS
    [size_is(GroupCount)] SID_AND_ATTRIBUTES Groups[*];
#else // MIDL_PASS
    SID_AND_ATTRIBUTES Groups[ANYSIZE_ARRAY];
#endif // MIDL_PASS
} TOKEN_GROUPS, *PTOKEN_GROUPS;


WINBASEAPI VOID WINAPI __int2c(VOID);

// From shlwapip.h, adding only needed definitions
#define LWSTDAPI_(type) type

#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))

#ifndef ASSERT
#define ASSERT          _ASSERTE
#endif

// --- end shlwapip.h

// Definitions needed from clrnt.h. To be moved next to entries from clrnt in pal
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
#ifdef MIDL_PASS
    [size_is(MaximumLength / 2), length_is((Length) / 2) ] USHORT * Buffer;
#else // MIDL_PASS
    PWSTR  Buffer;
#endif // MIDL_PASS
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;
typedef const UNICODE_STRING *PCUNICODE_STRING;
#define UNICODE_NULL ((WCHAR)0) // winnt
// end clr.t.h

// WDK SAL 2.0 Annotations got no place outside windows/drivers
#define __drv_maxIRQL(x)

// The NET_LUID union is the locally unique identifier (LUID) for a network interface.
typedef union _NET_LUID {
  ULONG64 Value;
  struct {
    ULONG64 Reserved  :24;
    ULONG64 NetLuidIndex  :24;
    ULONG64 IfType  :16;
  } Info;
} NET_LUID, *PNET_LUID;

// winsock replacements
#include <sys/socket.h>
#include <netinet/in.h>

#define WSAAPI

typedef sockaddr SOCKADDR, *PSOCKADDR, FAR *LPSOCKADDR;
typedef sockaddr_in SOCKADDR_IN, *PSOCKADDR_IN, FAR *LPSOCKADDR_IN;
typedef sockaddr_in6 SOCKADDR_IN6, *PSOCKADDR_IN6, FAR *LPSOCKADDR_IN6;

#define IN6_ADDR sockaddr_in6

/* mstcpip.h - END */

/* winsock2.h */
#define IN6ADDR_IS6TO4(a) \
    ((a)->sin6_addr.s6_addr16[0] == 0x2002)
#define IN6ADDR_ISISATAP(a) \
    (((a)->sin6_addr.s6_addr16[0] == 0xfe80) && \
    (((a)->sin6_addr.s6_addr16[1] == 0x0000) || \
    ((a)->sin6_addr.s6_addr16[1] == 0x0200)) && \
    ((a)->sin6_addr.s6_addr16[2] == 0x5efe))

#define WSADESCRIPTION_LEN	256
#define WSASYS_STATUS_LEN	128

typedef struct WSAData {
    WORD	wVersion;
    WORD	wHighVersion;
#ifdef _WIN64
    unsigned short	iMaxSockets;
    unsigned short	iMaxUdpDg;
    char FAR *		lpVendorInfo;
    char		szDescription[WSADESCRIPTION_LEN+1];
    char		szSystemStatus[WSASYS_STATUS_LEN+1];
#else
    char		szDescription[WSADESCRIPTION_LEN+1];
    char		szSystemStatus[WSASYS_STATUS_LEN+1];
    unsigned short	iMaxSockets;
    unsigned short	iMaxUdpDg;
    char FAR *		lpVendorInfo;
#endif
} WSADATA, FAR * LPWSADATA;

#define INVALID_SOCKET	(SOCKET)(~0)
#define SOCKET_ERROR	(-1)

typedef sa_family_t ADDRESS_FAMILY;

typedef struct _WSABUF {
    ULONG len;
    PCHAR buf;
} WSABUF, *LPWSABUF;

#if !defined(UNDER_RTCPAL)
typedef struct _SOCKET_ADDRESS {
    LPSOCKADDR lpSockaddr;
    INT iSockaddrLength;
} SOCKET_ADDRESS, *PSOCKET_ADDRESS, *LPSOCKET_ADDRESS;

#endif

#define SOCKADDR_STORAGE sockaddr_storage

#define SD_RECEIVE      SHUT_RD
#define SD_SEND         SHUT_WR
#define SD_BOTH         SHUT_RDWR

typedef int SOCKET;

int
WSAAPI
WSAGetLastError(
    void
    );
///* winsock2.h - END */

// intsafe functions
#define S_OK ((HRESULT)0x00000000L)

#define INTSAFE_E_ARITHMETIC_OVERFLOW       ((HRESULT)0x80070216L)  // 0x216 = 534 = ERROR_ARITHMETIC_OVERFLOW

#if defined(MIDL_PASS) || defined(RC_INVOKED) || defined(_M_CEE_PURE) \
    || defined(_68K_) || defined(_MPPC_) || defined(_PPC_)            \
    || defined(_M_IA64) || defined(_M_AMD64) || defined(__ARM_ARCH)

#ifndef UInt32x32To64
#define UInt32x32To64(a, b) ((unsigned __int64)((ULONG)(a)) * (unsigned __int64)((ULONG)(b)))
#endif

#elif defined(_M_IX86)

#ifndef UInt32x32To64
#define UInt32x32To64(a, b) (unsigned __int64)((unsigned __int64)(ULONG)(a) * (ULONG)(b))
#endif

#else

#error Must define a target architecture

#endif

#define Int32x32To64(a, b)  (((__int64)((long)(a))) * ((__int64)((long)(b))))

#define BIT_NUMBER ULONG
#define BITMAP_ELEMENT ULONG
#define PBITMAP_ELEMENT PULONG
#define PSBITMAP_ELEMENT PLONG
#define BITMAP_ELEMENT_BITS     32
#define BITMAP_ELEMENT_SHIFT    5
#define BITMAP_ELEMENT_MASK     31
#define BITMAP_SHIFT_BASE       1

#define RtlFillMemory(Destination,Length,Fill) memset((Destination),(Fill),(Length))

typedef struct _RTL_BITMAP {
    ULONG SizeOfBitMap;                     // Number of bits in bit map
    PULONG Buffer;                          // Pointer to the bit map itself
} RTL_BITMAP;
typedef RTL_BITMAP *PRTL_BITMAP;


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

#define INT8_MAX        127i8
#define UINT8_MAX       0xffui8
#define BYTE_MAX        0xff
#define SHORT_MAX       32767
#define INT16_MAX       32767i16
#define USHORT_MAX      0xffff
#define UINT16_MAX      0xffffui16
#define WORD_MAX        0xffff
#define INT_MAX         2147483647
#define INT32_MAX       2147483647i32
#define UINT_MAX        0xffffffff
#define UINT32_MAX      0xffffffffui32
#define LONG_MAX        2147483647L
#define ULONG_MAX       0xffffffffUL
#define DWORD_MAX       0xffffffffUL
#define LONGLONG_MAX    9223372036854775807i64
#define LONG64_MAX      9223372036854775807i64
#define INT64_MAX       9223372036854775807i64
#define ULONGLONG_MAX   0xffffffffffffffffui64
#define DWORDLONG_MAX   0xffffffffffffffffui64
#define ULONG64_MAX     0xffffffffffffffffui64
#define DWORD64_MAX     0xffffffffffffffffui64
#define UINT64_MAX      0xffffffffffffffffui64
#define INT128_MAX      170141183460469231731687303715884105727i128
#define UINT128_MAX     0xffffffffffffffffffffffffffffffffui128

#define USHORT_ERROR    (0xffff)
#define ULONG_ERROR     (0xffffffffUL)
#define SIZET_ERROR     ULONG_ERROR
__inline
HRESULT
UShortAdd(
    IN USHORT usAugend,
    IN USHORT usAddend,
    OUT USHORT* pusResult)
{
    HRESULT hr = INTSAFE_E_ARITHMETIC_OVERFLOW;
    *pusResult = USHORT_ERROR;

    if (((USHORT)(usAugend + usAddend)) >= usAugend)
    {
        *pusResult = (usAugend + usAddend);
        hr = S_OK;
    }

    return hr;
}

__inline
HRESULT
ULongLongToULong(
    IN ULONGLONG ullOperand,
    OUT ULONG* pulResult)
{
    HRESULT hr = INTSAFE_E_ARITHMETIC_OVERFLOW;
    *pulResult = ULONG_ERROR;

    if (ullOperand <= ULONG_MAX)
    {
        *pulResult = (ULONG)ullOperand;
        hr = S_OK;
    }

    return hr;
}

HRESULT Int64ToDWord(
  _In_   INT64 i64Operand,
  _Out_  DWORD *pdwResult
);

inline
HRESULT
ULongMult(
    IN ULONG ulMultiplicand,
    IN ULONG ulMultiplier,
    OUT ULONG* pulResult)
{
    ULONGLONG ull64Result = UInt32x32To64(ulMultiplicand, ulMultiplier);

    return ULongLongToULong(ull64Result, pulResult);
}

inline
HRESULT
SizeTMult(
    size_t Multiplicand,
    size_t Multiplier,
    size_t* pResult)
{
    HRESULT hr = INTSAFE_E_ARITHMETIC_OVERFLOW;
    *pResult = SIZET_ERROR;

    ULONGLONG ull64Result = (ULONGLONG)((ULONGLONG)(ULONG)(Multiplicand) * (ULONG)(Multiplier));
    if (ull64Result <= SIZE_MAX)
    {
        *pResult = (SIZE_T)ull64Result;
        hr = S_OK;
    }

    return hr;
}

inline
HRESULT
SizeTAdd(
    size_t Augend,
    size_t Addend,
    size_t* pResult)
{
    HRESULT hr = INTSAFE_E_ARITHMETIC_OVERFLOW;
    *pResult = SIZET_ERROR;

    if ((Augend + Addend) >= Augend)
    {
        *pResult = (Augend + Addend);
        hr = S_OK;
    }

    return hr;
}

inline
HRESULT
ULongAdd(
    ULONG ulAugend,
    ULONG ulAddend,
    ULONG* pulResult)
{
    HRESULT hr = INTSAFE_E_ARITHMETIC_OVERFLOW;
    *pulResult = ULONG_ERROR;

    if ((ulAugend + ulAddend) >= ulAugend)
    {
        *pulResult = (ulAugend + ulAddend);
        hr = S_OK;
    }

    return hr;
}

inline
HRESULT
UIntAdd(
    uint Augend,
    uint Addend,
    uint* pResult)
{
    HRESULT hr = INTSAFE_E_ARITHMETIC_OVERFLOW;
    *pResult = SIZET_ERROR;

    if ((Augend + Addend) >= Augend)
    {
        *pResult = (Augend + Addend);
        hr = S_OK;
    }

    return hr;
}

inline
HRESULT
ULongSub(
    IN ULONG ulMinuend,
    IN ULONG ulSubtrahend,
    OUT ULONG* pulResult)
{
    HRESULT hr = INTSAFE_E_ARITHMETIC_OVERFLOW;
    *pulResult = ULONG_ERROR;

    if (ulMinuend >= ulSubtrahend)
    {
        *pulResult = (ulMinuend - ulSubtrahend);
        hr = S_OK;
    }

    return hr;
}

// end intsafe functions

// ETW proto
VOID EventDataDescCreate(
  PEVENT_DATA_DESCRIPTOR EventDataDescriptor,
  VOID const* DataPtr,
  ULONG DataSize
);

// end ETW proto

typedef ULONG (* PERFLIBREQUEST)(
        IN ULONG  RequestCode,
        IN PVOID  Buffer,
        IN ULONG  BufferSize
);

typedef LPVOID (* PERF_MEM_ALLOC)(IN SIZE_T AllocSize, IN LPVOID pContext);
typedef void (* PERF_MEM_FREE)(IN LPVOID pBuffer, IN LPVOID pContext);

typedef struct _PROVIDER_CONTEXT {
    DWORD          ContextSize; // should be sizeof(PERF_PROVIDER_CONTEXT)
    DWORD          Reserved;
    PERFLIBREQUEST ControlCallback;
    PERF_MEM_ALLOC MemAllocRoutine;
    PERF_MEM_FREE  MemFreeRoutine;
    LPVOID         pMemContext;
} PERF_PROVIDER_CONTEXT, * PPERF_PROVIDER_CONTEXT;

ULONG WINAPI
PerfStartProviderEx (
        __in LPGUID ProviderGuid,
        __in_opt PPERF_PROVIDER_CONTEXT ProviderContext,
        __out PHANDLE Provider
);

typedef struct _LUID
{
    DWORD LowPart;
    LONG HighPart;
} 	LUID;
typedef struct _LUID_AND_ATTRIBUTES {
    LUID Luid;
    ULONG Attributes;
} LUID_AND_ATTRIBUTES, * PLUID_AND_ATTRIBUTES;
typedef struct _TOKEN_PRIVILEGES {
    ULONG PrivilegeCount;
    LUID_AND_ATTRIBUTES Privileges[ANYSIZE_ARRAY];
} TOKEN_PRIVILEGES, *PTOKEN_PRIVILEGES;

typedef enum _SECURITY_IMPERSONATION_LEVEL {
    SecurityAnonymous,
    SecurityIdentification,
    SecurityImpersonation,
    SecurityDelegation
} SECURITY_IMPERSONATION_LEVEL, * PSECURITY_IMPERSONATION_LEVEL;

typedef enum _TOKEN_TYPE {
    TokenPrimary = 1,
    TokenImpersonation
} TOKEN_TYPE;
typedef TOKEN_TYPE *PTOKEN_TYPE;

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

// xmllite types
typedef enum XmlNodeType
{
XmlNodeType_None                    = 0,
XmlNodeType_Element                 = 1,
XmlNodeType_Attribute               = 2,
XmlNodeType_Text                    = 3,
XmlNodeType_CDATA                   = 4,
XmlNodeType_ProcessingInstruction   = 7,
XmlNodeType_Comment                 = 8,
XmlNodeType_DocumentType            = 10,
XmlNodeType_Whitespace              = 13,
XmlNodeType_EndElement              = 15,
XmlNodeType_XmlDeclaration          = 17,
_XmlNodeType_Last                   = 17
} XmlNodeType;

// end xmllite types
//

// From DCommon.h in Windows SDK
//These macros are defined in the Windows 7 SDK, however to enable development using the technical preview,
//they are included here temporarily.
//
#ifndef DEFINE_ENUM_FLAG_OPERATORS
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
#endif

// winperf.h performance counter types

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
//  PERF_COUNTER_DEFINITION.CounterType field values
//
//
//        Counter ID Field Definition:
//
//   3      2        2    2    2        1        1    1
//   1      8        4    2    0        6        2    0    8                0
//  +--------+--------+----+----+--------+--------+----+----+----------------+
//  |Display |Calculation  |Time|Counter |        |Ctr |Size|                |
//  |Flags   |Modifiers    |Base|SubType |Reserved|Type|Fld |   Reserved     |
//  +--------+--------+----+----+--------+--------+----+----+----------------+
//
//
//  The counter type is the "or" of the following values as described below
//
//  select one of the following to indicate the counter's data size
//
#define PERF_SIZE_DWORD         0x00000000  // 32 bit field
#define PERF_SIZE_LARGE         0x00000100  // 64 bit field
#define PERF_SIZE_ZERO          0x00000200  // for Zero Length fields
#define PERF_SIZE_VARIABLE_LEN  0x00000300  // length is in CounterLength field
                                            //  of Counter Definition struct
//
//  select one of the following values to indicate the counter field usage
//
#define PERF_TYPE_NUMBER        0x00000000  // a number (not a counter)
#define PERF_TYPE_COUNTER       0x00000400  // an increasing numeric value
#define PERF_TYPE_TEXT          0x00000800  // a text field
#define PERF_TYPE_ZERO          0x00000C00  // displays a zero
//
//  If the PERF_TYPE_NUMBER field was selected, then select one of the
//  following to describe the Number
//
#define PERF_NUMBER_HEX         0x00000000  // display as HEX value
#define PERF_NUMBER_DECIMAL     0x00010000  // display as a decimal integer
#define PERF_NUMBER_DEC_1000    0x00020000  // display as a decimal/1000
//
//  If the PERF_TYPE_COUNTER value was selected then select one of the
//  following to indicate the type of counter
//
#define PERF_COUNTER_VALUE      0x00000000  // display counter value
#define PERF_COUNTER_RATE       0x00010000  // divide ctr / delta time
#define PERF_COUNTER_FRACTION   0x00020000  // divide ctr / base
#define PERF_COUNTER_BASE       0x00030000  // base value used in fractions
#define PERF_COUNTER_ELAPSED    0x00040000  // subtract counter from current time
#define PERF_COUNTER_QUEUELEN   0x00050000  // Use Queuelen processing func.
#define PERF_COUNTER_HISTOGRAM  0x00060000  // Counter begins or ends a histogram
#define PERF_COUNTER_PRECISION  0x00070000  // divide ctr / private clock
//
//  If the PERF_TYPE_TEXT value was selected, then select one of the
//  following to indicate the type of TEXT data.
//
#define PERF_TEXT_UNICODE       0x00000000  // type of text in text field
#define PERF_TEXT_ASCII         0x00010000  // ASCII using the CodePage field
//
//  Timer SubTypes
//
#define PERF_TIMER_TICK         0x00000000  // use system perf. freq for base
#define PERF_TIMER_100NS        0x00100000  // use 100 NS timer time base units
#define PERF_OBJECT_TIMER       0x00200000  // use the object timer freq
//
//  Any types that have calculations performed can use one or more of
//  the following calculation modification flags listed here
//
#define PERF_DELTA_COUNTER      0x00400000  // compute difference first
#define PERF_DELTA_BASE         0x00800000  // compute base diff as well
#define PERF_INVERSE_COUNTER    0x01000000  // show as 1.00-value (assumes:
#define PERF_MULTI_COUNTER      0x02000000  // sum of multiple instances
//
//  Select one of the following values to indicate the display suffix (if any)
//
#define PERF_DISPLAY_NO_SUFFIX  0x00000000  // no suffix
#define PERF_DISPLAY_PER_SEC    0x10000000  // "/sec"
#define PERF_DISPLAY_PERCENT    0x20000000  // "%"
#define PERF_DISPLAY_SECONDS    0x30000000  // "secs"
#define PERF_DISPLAY_NOSHOW     0x40000000  // value is not displayed
//
//  Predefined counter types
//

// 32-bit Counter.  Divide delta by delta time.  Display suffix: "/sec"
#define PERF_COUNTER_COUNTER        \
    (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
     PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_DISPLAY_PER_SEC)

// 64-bit Timer.  Divide delta by delta time.  Display suffix: "%"
#define PERF_COUNTER_TIMER          \
    (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
     PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_DISPLAY_PERCENT)

// Queue Length Space-Time Product. Divide delta by delta time. No Display Suffix.
#define PERF_COUNTER_QUEUELEN_TYPE  \
    (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_QUEUELEN |\
     PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_DISPLAY_NO_SUFFIX)

// Queue Length Space-Time Product. Divide delta by delta time. No Display Suffix.
#define PERF_COUNTER_LARGE_QUEUELEN_TYPE  \
    (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_QUEUELEN |\
     PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_DISPLAY_NO_SUFFIX)


// Queue Length Space-Time Product using 100 Ns timebase.
// Divide delta by delta time. No Display Suffix.
#define PERF_COUNTER_100NS_QUEUELEN_TYPE  \
    (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_QUEUELEN |\
     PERF_TIMER_100NS | PERF_DELTA_COUNTER | PERF_DISPLAY_NO_SUFFIX)


// 64-bit Counter.  Divide delta by delta time. Display Suffix: "/sec"
#define PERF_COUNTER_BULK_COUNT     \
    (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
     PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_DISPLAY_PER_SEC)

// Indicates the data is a counter  which should not be
// time averaged on display (such as an error counter on a serial line)
// Display as is.  No Display Suffix.
#define PERF_COUNTER_RAWCOUNT       \
    (PERF_SIZE_DWORD | PERF_TYPE_NUMBER | PERF_NUMBER_DECIMAL |\
     PERF_DISPLAY_NO_SUFFIX)

// Same as PERF_COUNTER_RAWCOUNT except its size is a large integer
#define PERF_COUNTER_LARGE_RAWCOUNT       \
    (PERF_SIZE_LARGE | PERF_TYPE_NUMBER | PERF_NUMBER_DECIMAL |\
     PERF_DISPLAY_NO_SUFFIX)

// Special case for RAWCOUNT that want to be displayed in hex
// Indicates the data is a counter  which should not be
// time averaged on display (such as an error counter on a serial line)
// Display as is.  No Display Suffix.
#define PERF_COUNTER_RAWCOUNT_HEX       \
    (PERF_SIZE_DWORD | PERF_TYPE_NUMBER | PERF_NUMBER_HEX |\
     PERF_DISPLAY_NO_SUFFIX)

// Same as PERF_COUNTER_RAWCOUNT_HEX except its size is a large integer
#define PERF_COUNTER_LARGE_RAWCOUNT_HEX       \
    (PERF_SIZE_LARGE | PERF_TYPE_NUMBER | PERF_NUMBER_HEX |\
     PERF_DISPLAY_NO_SUFFIX)

// A count which is either 1 or 0 on each sampling interrupt (% busy)
// Divide delta by delta base. Display Suffix: "%"
#define PERF_SAMPLE_FRACTION        \
    (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_FRACTION |\
     PERF_DELTA_COUNTER | PERF_DELTA_BASE | PERF_DISPLAY_PERCENT)

// A count which is sampled on each sampling interrupt (queue length)
// Divide delta by delta time. No Display Suffix.
#define PERF_SAMPLE_COUNTER         \
    (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
     PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_DISPLAY_NO_SUFFIX)

// 64-bit Timer inverse (e.g., idle is measured, but display busy %)
// Display 100 - delta divided by delta time.  Display suffix: "%"
#define PERF_COUNTER_TIMER_INV      \
    (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
     PERF_TIMER_TICK | PERF_DELTA_COUNTER | PERF_INVERSE_COUNTER | \
     PERF_DISPLAY_PERCENT)

// The divisor for a sample, used with the previous counter to form a
// sampled %.  You must check for >0 before dividing by this!  This
// counter will directly follow the  numerator counter.  It should not
// be displayed to the user.
#define PERF_SAMPLE_BASE            \
    (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_BASE |\
     PERF_DISPLAY_NOSHOW |\
     0x00000001)  // for compatibility with pre-beta versions

// A timer which, when divided by an average base, produces a time
// in seconds which is the average time of some operation.  This
// timer times total operations, and  the base is the number of opera-
// tions.  Display Suffix: "sec"
#define PERF_AVERAGE_TIMER          \
    (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_FRACTION |\
     PERF_DISPLAY_SECONDS)

// Used as the denominator in the computation of time or count
// averages.  Must directly follow the numerator counter.  Not dis-
// played to the user.
#define PERF_AVERAGE_BASE           \
    (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_BASE |\
     PERF_DISPLAY_NOSHOW |\
     0x00000002)  // for compatibility with pre-beta versions

// A bulk count which, when divided (typically) by the number of
// operations, gives (typically) the number of bytes per operation.
// No Display Suffix.
#define PERF_AVERAGE_BULK           \
    (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_FRACTION  |\
     PERF_DISPLAY_NOSHOW)

// 64-bit Timer in object specific units. Display delta divided by
// delta time as returned in the object type header structure.  Display suffix: "%"
#define PERF_OBJ_TIME_TIMER    \
    (PERF_SIZE_LARGE   | PERF_TYPE_COUNTER  | PERF_COUNTER_RATE |\
     PERF_OBJECT_TIMER | PERF_DELTA_COUNTER | PERF_DISPLAY_PERCENT)

// 64-bit Timer in 100 nsec units. Display delta divided by
// delta time.  Display suffix: "%"
#define PERF_100NSEC_TIMER          \
    (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
     PERF_TIMER_100NS | PERF_DELTA_COUNTER | PERF_DISPLAY_PERCENT)

// 64-bit Timer inverse (e.g., idle is measured, but display busy %)
// Display 100 - delta divided by delta time.  Display suffix: "%"
#define PERF_100NSEC_TIMER_INV      \
    (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
     PERF_TIMER_100NS | PERF_DELTA_COUNTER | PERF_INVERSE_COUNTER  |\
     PERF_DISPLAY_PERCENT)

// 64-bit Timer.  Divide delta by delta time.  Display suffix: "%"
// Timer for multiple instances, so result can exceed 100%.
#define PERF_COUNTER_MULTI_TIMER    \
    (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
     PERF_DELTA_COUNTER | PERF_TIMER_TICK | PERF_MULTI_COUNTER |\
     PERF_DISPLAY_PERCENT)

// 64-bit Timer inverse (e.g., idle is measured, but display busy %)
// Display 100 * _MULTI_BASE - delta divided by delta time.
// Display suffix: "%" Timer for multiple instances, so result
// can exceed 100%.  Followed by a counter of type _MULTI_BASE.
#define PERF_COUNTER_MULTI_TIMER_INV \
    (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_RATE |\
     PERF_DELTA_COUNTER | PERF_MULTI_COUNTER | PERF_TIMER_TICK |\
     PERF_INVERSE_COUNTER | PERF_DISPLAY_PERCENT)

// Number of instances to which the preceding _MULTI_..._INV counter
// applies.  Used as a factor to get the percentage.
#define PERF_COUNTER_MULTI_BASE     \
    (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_BASE |\
     PERF_MULTI_COUNTER | PERF_DISPLAY_NOSHOW)

// 64-bit Timer in 100 nsec units. Display delta divided by delta time.
// Display suffix: "%" Timer for multiple instances, so result can exceed 100%.
#define PERF_100NSEC_MULTI_TIMER   \
    (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_DELTA_COUNTER  |\
     PERF_COUNTER_RATE | PERF_TIMER_100NS | PERF_MULTI_COUNTER |\
     PERF_DISPLAY_PERCENT)

// 64-bit Timer inverse (e.g., idle is measured, but display busy %)
// Display 100 * _MULTI_BASE - delta divided by delta time.
// Display suffix: "%" Timer for multiple instances, so result
// can exceed 100%.  Followed by a counter of type _MULTI_BASE.
#define PERF_100NSEC_MULTI_TIMER_INV \
    (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_DELTA_COUNTER  |\
     PERF_COUNTER_RATE | PERF_TIMER_100NS | PERF_MULTI_COUNTER |\
     PERF_INVERSE_COUNTER | PERF_DISPLAY_PERCENT)

// Indicates the data is a fraction of the following counter  which
// should not be time averaged on display (such as free space over
// total space.) Display as is.  Display the quotient as "%".
#define PERF_RAW_FRACTION           \
    (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_FRACTION |\
     PERF_DISPLAY_PERCENT)

#define PERF_LARGE_RAW_FRACTION           \
    (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_FRACTION |\
     PERF_DISPLAY_PERCENT)

// Indicates the data is a base for the preceding counter which should
// not be time averaged on display (such as free space over total space.)
#define PERF_RAW_BASE               \
    (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_BASE |\
     PERF_DISPLAY_NOSHOW |\
             0x00000003)  // for compatibility with pre-beta versions

#define PERF_LARGE_RAW_BASE               \
    (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_BASE |\
     PERF_DISPLAY_NOSHOW )

// The data collected in this counter is actually the start time of the
// item being measured. For display, this data is subtracted from the
// sample time to yield the elapsed time as the difference between the two.
// In the definition below, the PerfTime field of the Object contains
// the sample time as indicated by the PERF_OBJECT_TIMER bit and the
// difference is scaled by the PerfFreq of the Object to convert the time
// units into seconds.
#define PERF_ELAPSED_TIME           \
    (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_ELAPSED |\
     PERF_OBJECT_TIMER | PERF_DISPLAY_SECONDS)

//
//  This counter is used to display the difference from one sample
//  to the next. The counter value is a constantly increasing number
//  and the value displayed is the difference between the current
//  value and the previous value. Negative numbers are not allowed
//  which shouldn't be a problem as long as the counter value is
//  increasing or unchanged.
//
#define PERF_COUNTER_DELTA      \
    (PERF_SIZE_DWORD | PERF_TYPE_COUNTER | PERF_COUNTER_VALUE |\
     PERF_DELTA_COUNTER | PERF_DISPLAY_NO_SUFFIX)

#define PERF_COUNTER_LARGE_DELTA      \
    (PERF_SIZE_LARGE | PERF_TYPE_COUNTER | PERF_COUNTER_VALUE |\
     PERF_DELTA_COUNTER | PERF_DISPLAY_NO_SUFFIX)

//
//  The precision counters are timers that consist of two counter values:
//      1) the count of elapsed time of the event being monitored
//      2) the "clock" time in the same units
//
//  the precition timers are used where the standard system timers are not
//  precise enough for accurate readings. It's assumed that the service
//  providing the data is also providing a timestamp at the same time which
//  will eliminate any error that may occur since some small and variable
//  time elapses between the time the system timestamp is captured and when
//  the data is collected from the performance DLL. Only in extreme cases
//  has this been observed to be problematic.
//
//  when using this type of timer, the definition of the
//      PERF_PRECISION_TIMESTAMP counter must immediately follow the
//      definition of the PERF_PRECISION_*_TIMER in the Object header
//
// The timer used has the same frequency as the System Performance Timer
#define PERF_PRECISION_SYSTEM_TIMER \
   (PERF_SIZE_LARGE    | PERF_TYPE_COUNTER     | PERF_COUNTER_PRECISION    | \
     PERF_TIMER_TICK    | PERF_DELTA_COUNTER    | PERF_DISPLAY_PERCENT   )
//
// The timer used has the same frequency as the 100 NanoSecond Timer
#define PERF_PRECISION_100NS_TIMER  \
    (PERF_SIZE_LARGE    | PERF_TYPE_COUNTER     | PERF_COUNTER_PRECISION    | \
     PERF_TIMER_100NS   | PERF_DELTA_COUNTER    | PERF_DISPLAY_PERCENT   )
//
// The timer used is of the frequency specified in the Object header's
//  PerfFreq field (PerfTime is ignored)
#define PERF_PRECISION_OBJECT_TIMER \
    (PERF_SIZE_LARGE    | PERF_TYPE_COUNTER     | PERF_COUNTER_PRECISION    | \
     PERF_OBJECT_TIMER  | PERF_DELTA_COUNTER    | PERF_DISPLAY_PERCENT   )

// end winperf.h performance counter types
// perflib.h performance counter instance types

// These are used for PERF_COUNTERSET_INFO::InstanceType value. That is, whether the CounterSet
// allows multiple instances (for example, Process, PhysicalDisk, etc) or only single default instance
// (for example, Memory, TCP, etc).
//
#define PERF_COUNTERSET_FLAG_MULTIPLE             2  // 0010
#define PERF_COUNTERSET_FLAG_AGGREGATE            4  // 0100
#define PERF_COUNTERSET_FLAG_HISTORY              8  // 1000
#define PERF_COUNTERSET_FLAG_INSTANCE            16  // 00010000

#define PERF_COUNTERSET_SINGLE_INSTANCE          0
#define PERF_COUNTERSET_MULTI_INSTANCES          (PERF_COUNTERSET_FLAG_MULTIPLE)
#define PERF_COUNTERSET_SINGLE_AGGREGATE         (PERF_COUNTERSET_FLAG_AGGREGATE)
#define PERF_COUNTERSET_MULTI_AGGREGATE          (PERF_COUNTERSET_FLAG_AGGREGATE | PERF_COUNTERSET_FLAG_MULTIPLE)
#define PERF_COUNTERSET_SINGLE_AGGREGATE_HISTORY (PERF_COUNTERSET_FLAG_HISTORY | PERF_COUNTERSET_SINGLE_AGGREGATE)
#define PERF_COUNTERSET_INSTANCE_AGGREGATE       (PERF_COUNTERSET_MULTI_AGGREGATE | PERF_COUNTERSET_FLAG_INSTANCE)

// end perflib.h performance counter instance types

// From Objidl.h
typedef enum _APTTYPE {
  APTTYPE_CURRENT  = -1,
  APTTYPE_STA      = 0,
  APTTYPE_MTA      = 1,
  APTTYPE_NA       = 2,
  APTTYPE_MAINSTA  = 3
} APTTYPE;

typedef enum  {
  APTTYPEQUALIFIER_NONE                = 0,
  APTTYPEQUALIFIER_IMPLICIT_MTA        = 1,
  APTTYPEQUALIFIER_NA_ON_MTA           = 2,
  APTTYPEQUALIFIER_NA_ON_STA           = 3,
  APTTYPEQUALIFIER_NA_ON_IMPLICIT_MTA  = 4,
  APTTYPEQUALIFIER_NA_ON_MAINSTA       = 5
} APTTYPEQUALIFIER;

// end Objidl.h



// Doubly-linked list manipulation macros (from ntrtl.h)

#undef InitializeListHead
inline
VOID
InitializeListHead(
        PLIST_ENTRY ListHead
)
{
    ListHead->Flink = ListHead->Blink = ListHead;
}

#undef IsListEmpty
inline
BOOLEAN
IsListEmpty(
        PLIST_ENTRY ListHead
)
{
    return (BOOLEAN)(ListHead->Flink == ListHead);
}

#undef RemoveHeadList
inline
PLIST_ENTRY
RemoveHeadList(
        PLIST_ENTRY ListHead
)
{
    PLIST_ENTRY Flink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Flink;
    Flink = Entry->Flink;
    ListHead->Flink = Flink;
    Flink->Blink = ListHead;
    return Entry;
}

#undef RemoveTailList
inline
PLIST_ENTRY
RemoveTailList(
        PLIST_ENTRY ListHead
)
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Blink;
    Blink = Entry->Blink;
    ListHead->Blink = Blink;
    Blink->Flink = ListHead;
    return Entry;
}

#undef RemoveEntryList
inline
BOOLEAN
RemoveEntryList(
        PLIST_ENTRY Entry
)
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Flink;

    Flink = Entry->Flink;
    Blink = Entry->Blink;
    Blink->Flink = Flink;
    Flink->Blink = Blink;
    return (BOOLEAN)(Flink == Blink);
}

#undef InsertTailList
inline
VOID
InsertTailList(
        PLIST_ENTRY ListHead,
        PLIST_ENTRY Entry
)
{
    PLIST_ENTRY Blink;

    Blink = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = Blink;
    Blink->Flink = Entry;
    ListHead->Blink = Entry;
    return;
}

#undef InsertHeadList
inline
VOID
InsertHeadList(
        PLIST_ENTRY ListHead,
        PLIST_ENTRY Entry
)
{
    PLIST_ENTRY Flink;

    Flink = ListHead->Flink;
    Entry->Flink = Flink;
    Entry->Blink = ListHead;
    Flink->Blink = Entry;
    ListHead->Flink = Entry;
    return;
}

#define CONTAINING_RECORD(address, type, field) ((type *)( \
                                                  (PCHAR)(address) - \
                                                  (ULONG_PTR)(&((type *)0)->field)))


// sriramsh: NO implementation in PAL, needs to be implemented in fmtmessage.cpp and
// prototype moved into pal.h
WINBASEAPI
DWORD
FormatMessageA(
           IN DWORD dwFlags,
           IN LPCVOID lpSource,
           IN DWORD dwMessageId,
           IN DWORD dwLanguageId,
           OUT LPSTR lpBffer,
           IN DWORD nSize,
           IN va_list *Arguments);


// from timezoneapi.h
WINBASEAPI
//_Success_(return != FALSE)
BOOL
//WINAPI
SystemTimeToFileTime(
    IN CONST SYSTEMTIME *lpSystemTime,
    OUT LPFILETIME lpFileTime
    );


// from rpc.h (rpcdce.h, rpcnterr.h)
// note the use of ::GUID instead of UUID in function
// signatures.
#define RPCRTAPI
#define RPC_ENTRY
#define __RPC_FAR
#define RPC_S_OK	STATUS_SUCCESS

typedef long RPC_STATUS;
typedef _Null_terminated_ wchar_t __RPC_FAR * RPC_WSTR;
typedef _Null_terminated_ const wchar_t * RPC_CWSTR;


//RPCRTAPI
//_Must_inspect_result_
WINBASEAPI
RPC_STATUS
//RPC_ENTRY
UuidCreate (
    OUT ::GUID __RPC_FAR * Uuid
    );


// synchapi.h
typedef RTL_SRWLOCK SRWLOCK, *PSRWLOCK;
typedef RTL_CONDITION_VARIABLE  CONDITION_VARIABLE, *PCONDITION_VARIABLE;

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
VOID
WINAPI
AcquireSRWLockShared(
    IN OUT PSRWLOCK SRWLock
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


/* libloaderapi.h */
#define GET_MODULE_HANDLE_EX_FLAG_PIN			(0x00000001)
#define GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT	(0x00000002)
#define GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS		(0x00000004)


WINBASEAPI
BOOL
WINAPI
GetModuleHandleExW(
    IN DWORD dwFlags,
    LPCWSTR lpModuleName,
    OUT HMODULE *phModule
    );

#ifdef UNICODE
#define GetModuleHandleEx GetModuleHandleExW
#endif

/* evntprov.h */
//#define EVNTAPI DECLSPEC_IMPORT __stdcall
#define EVNTAPI

#if !defined(UNDER_RTCPAL)
typedef struct _EVENT_FILTER_DESCRIPTOR {
	ULONGLONG Ptr;
	ULONG Size;
	ULONG Type;
} EVENT_FILTER_DESCRIPTOR, *PEVENT_FILTER_DESCRIPTOR;
#endif

typedef
VOID
(NTAPI *PENABLECALLBACK)(
    LPCGUID SourceId,
    ULONG IsEnabled,
    UCHAR Level,
    ULONGLONG MatchAnyKeyword,
    ULONGLONG MatchAllKeyword,
    PEVENT_FILTER_DESCRIPTOR FilterData,
    PVOID CallbackContext
    );

extern "C" ULONG EVNTAPI EventRegister(
    IN LPCGUID ProviderId,
    PENABLECALLBACK EnableCallback,
    PVOID CallbackContext,
    OUT PREGHANDLE RegHandle
    );

//FORCEINLINE
extern "C" VOID EventDescCreate(
    _Out_ PEVENT_DESCRIPTOR EventDescriptor,
    _In_ USHORT Id,
    _In_ UCHAR Version,
    _In_ UCHAR Channel,
    _In_ UCHAR Level,
    _In_ USHORT Task,
    _In_ UCHAR Opcode,
    _In_ ULONGLONG Keyword
    );

extern "C" ULONG EVNTAPI EventWrite(
    _In_ REGHANDLE RegHandle,
    _In_ PCEVENT_DESCRIPTOR EventDescriptor,
    _In_ ULONG UserDataCount,
    _In_reads_opt_(UserDataCount) PEVENT_DATA_DESCRIPTOR UserData
    );

extern "C" BOOL EventEnabled(
  _In_ REGHANDLE          RegHandle,
  _In_ PCEVENT_DESCRIPTOR EventDescriptor
);

#if !defined(UNDER_RTCPAL) && !defined(NO_INLINE_EVENTDESCCREATE)
inline VOID EventDescCreate(
    _Out_ PEVENT_DESCRIPTOR EventDescriptor,
    _In_ USHORT Id,
    _In_ UCHAR Version,
    _In_ UCHAR Channel,
    _In_ UCHAR Level,
    _In_ USHORT Task,
    _In_ UCHAR Opcode,
    _In_ ULONGLONG Keyword
)
{
    EventDescriptor->Id = Id;
    EventDescriptor->Version = Version;
    EventDescriptor->Channel = Channel;
    EventDescriptor->Level = Level;
    EventDescriptor->Task = Task;
    EventDescriptor->Opcode = Opcode;
    EventDescriptor->Keyword = Keyword;
    return;
}
#endif

/* evntpro.h END */
/* perflib.h */
#define PERF_ATTRIB_BY_REFERENCE	0x0000000000000001
#define PERF_ATTRIB_NO_DISPLAYABLE	0x0000000000000002
#define PERF_ATTRIB_NO_GROUP_SEPARATOR	0x0000000000000004
#define PERF_ATTRIB_DISPLAY_AS_REAL	0x0000000000000008
#define PERF_ATTRIB_DISPLAY_AS_HEX	0x0000000000000010

typedef struct _PERF_COUNTERSET_INFO {
    GUID CounterSetGuid;
    GUID ProviderGuid;
    ULONG NumCounters;
    ULONG InstanceType;
} PERF_COUNTERSET_INFO, * PPERF_COUNTERSET_INFO;

typedef struct _PERF_COUNTER_INFO {
    ULONG CounterId;
    ULONG Type;
    ULONGLONG Attrib;
    ULONG Size;
    ULONG DetailLevel;
    LONG Scale;
    ULONG Offset;
} PERF_COUNTER_INFO, *PPERF_COUNTER_INFO;


typedef struct _PERF_COUNTERSET_INSTANCE {
    GUID CounterSetGuid;
    ULONG dwSize;
    ULONG InstanceId;
    ULONG InstanceNameOffset;
    ULONG InstanceNameSize;
} PERF_COUNTERSET_INSTANCE, *PPERF_COUNTERSET_INSTANCE;

typedef ULONG (
WINAPI
*PERFLIBREQUEST)(
    IN ULONG RequestCode,
    IN PVOID Buffer,
    IN ULONG BufferSize
);

ULONG WINAPI
PerfStartProvider (
    IN LPGUID ProviderGuid,
    PERFLIBREQUEST ControlCallback,
    OUT HANDLE *phProvider
);

ULONG WINAPI
PerfStopProvider(
    IN HANDLE ProviderHandle
    );

PPERF_COUNTERSET_INSTANCE
WINAPI
PerfCreateInstance(
    IN HANDLE ProviderHandle,
    IN LPCGUID CounterSetGuid,
    IN PCWSTR Name,
    IN ULONG Id
    );

ULONG
WINAPI
PerfDeleteInstance(
    IN HANDLE Provider,
    IN PPERF_COUNTERSET_INSTANCE InstanceBlock
    );

ULONG
WINAPI
PerfSetCounterRefValue(
    IN HANDLE Provider,
    PPERF_COUNTERSET_INSTANCE Instance,
    IN ULONG CounterId,
    IN PVOID Address
    );

ULONG
WINAPI
PerfSetCounterSetInfo(
    IN HANDLE ProviderHandle,
    PPERF_COUNTERSET_INFO Template,
    IN ULONG TemplateSize
    );

/* winperf.h */
#define PERF_DETAIL_NOVICE	100
#define PERF_DETAIL_ADVANCED	200
#define PERF_DETAIL_EXPERT	300
#define PERF_DETAIL_WIZARD	400

/* From wincon.h */
#define FOREGROUND_BLUE      0x0001 // text color contains blue.
#define FOREGROUND_GREEN     0x0002 // text color contains green.
#define FOREGROUND_RED       0x0004 // text color contains red.
#define FOREGROUND_INTENSITY 0x0008 // text color is intensified.
#define BACKGROUND_BLUE      0x0010 // background color contains blue.
#define BACKGROUND_GREEN     0x0020 // background color contains green.
#define BACKGROUND_RED       0x0040 // background color contains red.
#define BACKGROUND_INTENSITY 0x0080 // background color is intensified.

typedef struct _COORD {
    SHORT X;
    SHORT Y;
} COORD, *PCOORD;

typedef struct _SMALL_RECT {
    SHORT Left;
    SHORT Top;
    SHORT Right;
    SHORT Bottom;
} SMALL_RECT, *PSMALL_RECT;

typedef struct _CONSOLE_READCONSOLE_CONTROL {
    ULONG nLength;
    ULONG nInitialChars;
    ULONG dwCtrlWakeupMask;
    ULONG dwControlKeyState;
} CONSOLE_READCONSOLE_CONTROL, *PCONSOLE_READCONSOLE_CONTROL;

typedef struct _CONSOLE_SCREEN_BUFFER_INFO {
    COORD dwSize;
    COORD dwCursorPosition;
    WORD  wAttributes;
    SMALL_RECT srWindow;
    COORD dwMaximumWindowSize;
} CONSOLE_SCREEN_BUFFER_INFO, *PCONSOLE_SCREEN_BUFFER_INFO;


/* consoleapi.h */
#ifdef UNICODE
#define WriteConsole  WriteConsoleW
#else
#define WriteConsole  WriteConsoleA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
GetConsoleMode(
    _In_ HANDLE hConsoleHandle,
    _Out_ LPDWORD lpMode
    );

WINBASEAPI
_Success_(return != FALSE)
BOOL
WINAPI
ReadConsoleW(
    _In_ HANDLE hConsoleInput,
    _Out_writes_bytes_to_(nNumberOfCharsToRead * sizeof(WCHAR), *lpNumberOfCharsRead * sizeof(WCHAR)) LPVOID lpBuffer,
    _In_ DWORD nNumberOfCharsToRead,
    _Out_ _Deref_out_range_(<=, nNumberOfCharsToRead) LPDWORD lpNumberOfCharsRead,
    _In_opt_ PCONSOLE_READCONSOLE_CONTROL pInputControl
   );

WINBASEAPI
BOOL
WINAPI
WriteConsoleA(
    _In_ HANDLE hConsoleOutput,
    _In_reads_(nNumberOfCharsToWrite) CONST VOID * lpBuffer,
    _In_ DWORD nNumberOfCharsToWrite,
    _Out_opt_ LPDWORD lpNumberOfCharsWritten,
    _Reserved_ LPVOID lpReserved
    );

WINBASEAPI
BOOL
WINAPI
WriteConsoleW(
    _In_ HANDLE hConsoleOutput,
    _In_reads_(nNumberOfCharsToWrite) CONST VOID * lpBuffer,
    _In_ DWORD nNumberOfCharsToWrite,
    _Out_opt_ LPDWORD lpNumberOfCharsWritten,
    _Reserved_ LPVOID lpReserved
    );

/* consoleapi2.h */
WINBASEAPI
BOOL
WINAPI
SetConsoleTextAttribute(
    _In_ HANDLE hConsoleOutput,
    _In_ WORD wAttributes
    );

WINBASEAPI
BOOL
WINAPI
GetConsoleScreenBufferInfo(
    _In_ HANDLE hConsoleOutput,
    _Out_ PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo
    );
/* winperf.h - END */

/* WinUser.h - BEGIN */
#define WINUSERAPI


WINUSERAPI
BOOL
WINAPI
IsCharAlphaW(
    WCHAR ch);

WINUSERAPI
BOOL
WINAPI
IsCharAlphaA(
    CHAR ch);

#ifdef UNICODE
#define IsCharAlpha IsCharAlphaW
#else
#define IsCharAlpha IsCharAlphaA
#endif

WINUSERAPI
DWORD
WINAPI
CharLowerBuffA(
    LPSTR lpsz,
    DWORD cchLength);

WINUSERAPI
DWORD
WINAPI
CharLowerBuffW(
    LPWSTR lpsz,
    DWORD cchLength);

#ifdef UNICODE
#define CharLowerBuff CharLowerBuffW
#else
#define CharLowerBuff CharLowerBuffA
#endif

/* WinUser.h - END */

/* WinNLs.h - BEGIN */
#define HIGH_SURROGATE_START 	0xd800
#define HIGH_SURROGATE_END	0xdbff
#define LOW_SURROGATE_START	0xdc00
#define LOW_SURROGATE_END	0xdfff

#define IS_HIGH_SURROGATE(wch)	(((wch) >= HIGH_SURROGATE_START) && ((wch) <= HIGH_SURROGATE_END))
#define IS_LOW_SURROGATE(wch)	(((wch) >= LOW_SURROGATE_START) && ((wch) <= LOW_SURROGATE_END))
#define IS_SURROGATE_PAIR(hs, ls) (IS_HIGH_SURROGATE(hs) && IS_LOW_SURROGATE(ls))


#define WC_ERR_INVALID_CHARS      0x00000080  // error for invalid chars

/* WinNIs.h - END */


/* WinDNS.h - BEGIN */
//
//  DNS Names limited to 255, 63 in any one label
//

#define DNS_MAX_NAME_LENGTH             (255)
#define DNS_MAX_LABEL_LENGTH            (63)

#define DNS_MAX_NAME_BUFFER_LENGTH      (256)
#define DNS_MAX_LABEL_BUFFER_LENGTH     (64)


/* WinDNS.h - END */

/* sysinfoapi.h - BEGIN */
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


WINBASEAPI
//_Success_(return != FALSE)
BOOL
WINAPI
GetComputerNameExA(
    COMPUTER_NAME_FORMAT NameType,
    LPSTR lpBuffer,
    LPDWORD nSize
    );

WINBASEAPI
//_Success_(return != FALSE)
BOOL
WINAPI
GetComputerNameExW(
    COMPUTER_NAME_FORMAT NameType,
    LPWSTR lpBuffer,
    LPDWORD nSize
    );

#ifdef UNICODE
#define GetComputerNameEx  GetComputerNameExW
#else
#define GetComputerNameEx  GetComputerNameExA
#endif // !UNICODE

/* sysinfoapi.h - END */

/* processenv.h - BEGIN */
WINBASEAPI
//_Success_(return != 0 && return <= nSize)
DWORD
WINAPI
ExpandEnvironmentStringsA(
    _In_ LPCSTR lpSrc,
    LPSTR lpDst,
    _In_ DWORD nSize
    );

WINBASEAPI
//_Success_(return != 0 && return <= nSize)
DWORD
WINAPI
ExpandEnvironmentStringsW(
    _In_ LPCWSTR lpSrc,
    LPWSTR lpDst,
    _In_ DWORD nSize
    );

#ifdef UNICODE
#define ExpandEnvironmentStrings  ExpandEnvironmentStringsW
#else
#define ExpandEnvironmentStrings  ExpandEnvironmentStringsA
#endif // !UNICODE
/* processenv.h - END */

/* APIENTRY is just a definition of WINAPI */
#define APIENTRY WINAPI

/*  wincrypt.h - BEGIN */

// certenrolld_begin -- CERT_NAME_STR_*_FLAG
//+-------------------------------------------------------------------------
//  Certificate name string types
//--------------------------------------------------------------------------
#define CERT_SIMPLE_NAME_STR        1
#define CERT_OID_NAME_STR           2
#define CERT_X500_NAME_STR          3
#define CERT_XML_NAME_STR           4
#define WINCRYPT32API

#define CERT_CHAIN_REVOCATION_CHECK_END_CERT           0x10000000
#define CERT_CHAIN_REVOCATION_CHECK_CHAIN              0x20000000
#define CERT_CHAIN_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT 0x40000000 
#define CERT_CHAIN_REVOCATION_CHECK_CACHE_ONLY         0x80000000

typedef ULONG_PTR HCRYPTPROV_OR_NCRYPT_KEY_HANDLE;
typedef ULONG_PTR HCRYPTPROV_LEGACY;

#define PP_UNIQUE_CONTAINER		36

#define X509_ASN_ENCODING		0x00000001
#define PKCS_7_ASN_ENCODING		0x00010000

#define CERT_STORE_PROV_SYSTEM_W	((LPCSTR) 10)
#define CERT_STORE_PROV_SYSTEM		CERT_STORE_PROV_SYSTEM_W

#define CERT_STORE_READONLY_FLAG	0x00008000

#define szOID_RSA_SHA1RSA               "1.2.840.113549.1.1.5"
#define szOID_NIST_AES256_CBC           "2.16.840.1.101.3.4.1.42"
#define szOID_SUBJECT_ALT_NAME          "2.5.29.7"
#define szOID_SUBJECT_ALT_NAME2         "2.5.29.17"
#define szOID_PKIX_KP_SERVER_AUTH       "1.3.6.1.5.5.7.3.1"
#define szOID_PKIX_KP_CLIENT_AUTH       "1.3.6.1.5.5.7.3.2"
#define CERT_ALT_NAME_DNS_NAME          3

#if defined(UNDER_RTCPAL)
typedef struct _CRYPTOAPI_BLOB {
    DWORD cbData;
    BYTE  *pbData;
} CRYPT_INTEGER_BLOB, *PCRYPT_INTEGER_BLOB, CRYPT_UINT_BLOB, *PCRYPT_UINT_BLOB, CRYPT_OBJID_BLOB, *PCRYPT_OBJID_BLOB, CERT_NAME_BLOB, CERT_RDN_VALUE_BLOB, *PCERT_NAME_BLOB, *PCERT_RDN_VALUE_BLOB, CERT_BLOB, *PCERT_BLOB, CRL_BLOB, *PCRL_BLOB, DATA_BLOB, *PDATA_BLOB, CRYPT_DATA_BLOB, *PCRYPT_DATA_BLOB, CRYPT_HASH_BLOB, *PCRYPT_HASH_BLOB, CRYPT_DIGEST_BLOB, *PCRYPT_DIGEST_BLOB, CRYPT_DER_BLOB, PCRYPT_DER_BLOB, CRYPT_ATTR_BLOB, *PCRYPT_ATTR_BLOB;

typedef struct _CRYPT_BIT_BLOB {
    DWORD cbData;
    BYTE  *pbData;
    DWORD cUnusedBits;
} CRYPT_BIT_BLOB, *PCRYPT_BIT_BLOB;

typedef struct _CERT_EXTENSION {
    LPSTR            pszObjId;
    BOOL             fCritical;
    CRYPT_OBJID_BLOB Value;
} CERT_EXTENSION, *PCERT_EXTENSION;

typedef struct _CRYPT_ALGORITHM_IDENTIFIER {
    LPSTR            pszObjId;
    CRYPT_OBJID_BLOB Parameters;
} CRYPT_ALGORITHM_IDENTIFIER, *PCRYPT_ALGORITHM_IDENTIFIER;

typedef struct _CERT_PUBLIC_KEY_INFO {
    CRYPT_ALGORITHM_IDENTIFIER Algorithm;
    CRYPT_BIT_BLOB             PublicKey;
} CERT_PUBLIC_KEY_INFO, *PCERT_PUBLIC_KEY_INFO;

typedef struct _CERT_INFO {
    DWORD                      dwVersion;
    CRYPT_INTEGER_BLOB         SerialNumber;
    CRYPT_ALGORITHM_IDENTIFIER SignatureAlgorithm;
    CERT_NAME_BLOB             Issuer;
    FILETIME                   NotBefore;
    FILETIME                   NotAfter;
    CERT_NAME_BLOB             Subject;
    CERT_PUBLIC_KEY_INFO       SubjectPublicKeyInfo;
    CRYPT_BIT_BLOB             IssuerUniqueId;
    CRYPT_BIT_BLOB             SubjectUniqueId;
    DWORD                      cExtension;
    PCERT_EXTENSION            rgExtension;
} CERT_INFO, *PCERT_INFO;

typedef void * HCERTSTORE;

typedef struct _CERT_CONTEXT {
    DWORD      dwCertEncodingType;
    BYTE       *pbCertEncoded;
    DWORD      cbCertEncoded;
    PCERT_INFO pCertInfo;
    HCERTSTORE hCertStore;
} CERT_CONTEXT, *PCERT_CONTEXT;
typedef const CERT_CONTEXT *PCCERT_CONTEXT;

#endif

// certenrolls_begin -- CERT_ALT_NAME_INFO
typedef struct _CERT_OTHER_NAME {
    LPSTR               pszObjId;
    CRYPT_OBJID_BLOB    Value;
} CERT_OTHER_NAME, *PCERT_OTHER_NAME;

typedef struct _CERT_ALT_NAME_ENTRY {
    DWORD   dwAltNameChoice;
    union {                                             // certenrolls_skip
        PCERT_OTHER_NAME            pOtherName;         // 1
        LPWSTR                      pwszRfc822Name;     // 2  (encoded IA5)
        LPWSTR                      pwszDNSName;        // 3  (encoded IA5)
        // Not implemented          x400Address;        // 4
        CERT_NAME_BLOB              DirectoryName;      // 5
        // Not implemented          pEdiPartyName;      // 6
        LPWSTR                      pwszURL;            // 7  (encoded IA5)
        CRYPT_DATA_BLOB             IPAddress;          // 8  (Octet String)
        LPSTR                       pszRegisteredID;    // 9  (Object Identifer)
    }; //DUMMYUNIONNAME;                                // certenrolls_skip
} CERT_ALT_NAME_ENTRY, *PCERT_ALT_NAME_ENTRY;
// certenrolls_end
typedef struct _CERT_ALT_NAME_INFO {
    DWORD cAltEntry;
    PCERT_ALT_NAME_ENTRY rgAltEntry;
} CERT_ALT_NAME_INFO, *PCERT_ALT_NAME_INFO;

typedef struct _CTL_USAGE {
    DWORD               cUsageIdentifier;
    LPSTR               *rgpszUsageIdentifier;      // array of pszObjId
} CTL_USAGE, *PCTL_USAGE,
CERT_ENHKEY_USAGE, *PCERT_ENHKEY_USAGE;
typedef const CTL_USAGE* PCCTL_USAGE;
typedef const CERT_ENHKEY_USAGE* PCCERT_ENHKEY_USAGE;

#define CERT_FIND_OPTIONAL_ENHKEY_USAGE_FLAG  0x1
#define CRYPT_ENCODE_ALLOC_FLAG               0x8000

typedef LPVOID (WINAPI *PFN_CRYPT_ALLOC)(
    _In_ size_t cbSize
    );

typedef VOID (WINAPI *PFN_CRYPT_FREE)(
    _In_ LPVOID pv
    );

typedef struct _CRYPT_DECODE_PARA {
    DWORD                   cbSize;
    PFN_CRYPT_ALLOC         pfnAlloc;           // OPTIONAL
    PFN_CRYPT_FREE          pfnFree;            // OPTIONAL
} CRYPT_DECODE_PARA, *PCRYPT_DECODE_PARA;

typedef struct _CRYPT_ENCODE_PARA {
    DWORD                   cbSize;
    PFN_CRYPT_ALLOC         pfnAlloc;           // OPTIONAL
    PFN_CRYPT_FREE          pfnFree;            // OPTIONAL
} CRYPT_ENCODE_PARA, *PCRYPT_ENCODE_PARA;

typedef struct _CERT_EXTENSIONS {
    DWORD           cExtension;
    PCERT_EXTENSION rgExtension;
} CERT_EXTENSIONS, *PCERT_EXTENSIONS;

#define X509_ALTERNATE_NAME                 ((LPCSTR) 12)
#define CERT_STORE_ADD_ALWAYS               4
#define CERT_NAME_ATTR_TYPE                 3
#define AT_KEYEXCHANGE                      1


typedef struct _CRYPT_KEY_PROV_PARAM {
    DWORD           dwParam;
    BYTE            *pbData;
    DWORD           cbData;
    DWORD           dwFlags;
} CRYPT_KEY_PROV_PARAM, *PCRYPT_KEY_PROV_PARAM;

typedef struct _CRYPT_KEY_PROV_INFO {
    LPWSTR                  pwszContainerName;
    LPWSTR                  pwszProvName;
    DWORD                   dwProvType;
    DWORD                   dwFlags;
    DWORD                   cProvParam;
    PCRYPT_KEY_PROV_PARAM   rgProvParam;
    DWORD                   dwKeySpec;
} CRYPT_KEY_PROV_INFO, *PCRYPT_KEY_PROV_INFO;

#define MS_ENHANCED_PROV_A      "Microsoft Enhanced Cryptographic Provider v1.0"
#define MS_ENHANCED_PROV_W      L"Microsoft Enhanced Cryptographic Provider v1.0"

#define LANG_NEUTRAL            0x00
#define SUBLANG_NEUTRAL         0x00    // language neutral
#define SUBLANG_DEFAULT         0x01    // user default
#define LANG_USER_DEFAULT      (MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT))
#define LOCALE_USER_DEFAULT    (MAKELCID(LANG_USER_DEFAULT, SORT_DEFAULT))
#define SORT_DEFAULT           0x0     // sorting default

#define MAKELANGID(p, s)       ((((WORD  )(s)) << 10) | (WORD  )(p))
#define MAKELCID(lgid, srtid)  ((DWORD)((((DWORD)((WORD  )(srtid))) << 16) |  \
                                         ((DWORD)((WORD  )(lgid)))))
#define MAKESORTLCID(lgid, srtid, ver)                                        \
                               ((DWORD)((MAKELCID(lgid, srtid)) |             \
                                    (((DWORD)((WORD  )(ver))) << 20)))

//+-------------------------------------------------------------------------
//  Attributes making up a Relative Distinguished Name (CERT_RDN)
//
//  The interpretation of the Value depends on the dwValueType.
//  See below for a list of the types.
//--------------------------------------------------------------------------
typedef struct _CERT_RDN_ATTR {
    LPSTR                   pszObjId;
    DWORD                   dwValueType;
    CERT_RDN_VALUE_BLOB     Value;
} CERT_RDN_ATTR, *PCERT_RDN_ATTR;

//+-------------------------------------------------------------------------
//  A CERT_RDN consists of an array of the above attributes
//--------------------------------------------------------------------------
typedef struct _CERT_RDN {
    DWORD           cRDNAttr;
    PCERT_RDN_ATTR  rgRDNAttr;
} CERT_RDN, *PCERT_RDN;


typedef struct _CRYPT_ENCRYPT_MESSAGE_PARA {
    DWORD	cbSize;
    DWORD	dwMsgEncodingType;
    HCRYPTPROV_LEGACY	hCryptProv;
    CRYPT_ALGORITHM_IDENTIFIER	ContentEncryptionAlgorithm;
    void *			pvEncryptionAuxInfo;
    DWORD			dwFlags;
    DWORD			dwInnerContentType;
} CRYPT_ENCRYPT_MESSAGE_PARA, *PCRYPT_ENCRYPT_MESSAGE_PARA;

typedef struct _CRYPT_DECRYPT_MESSAGE_PARA {
    DWORD	cbSize;
    DWORD	dwMsgAndCertEncodingType;
    DWORD	cCertStore;
    HCERTSTORE	*rghCertStore;
#ifdef CRYPT_DECRYPT_MESSAGE_PARA_HAS_EXTRA_FIELDS
    DWORD	dwFlags;
#endif
} CRYPT_DECRYPT_MESSAGE_PARA, *PCRYPT_DECRYPT_MESSAGE_PARA;

// certenrolld_begin -- PROV_RSA_*
#define PROV_RSA_FULL           1
#define PROV_RSA_SIG            2
#define PROV_DSS                3
#define PROV_FORTEZZA           4
#define PROV_MS_EXCHANGE        5
#define PROV_SSL                6
#define PROV_RSA_SCHANNEL       12
#define PROV_DSS_DH             13
#define PROV_EC_ECDSA_SIG       14
#define PROV_EC_ECNRA_SIG       15
#define PROV_EC_ECDSA_FULL      16
#define PROV_EC_ECNRA_FULL      17
#define PROV_DH_SCHANNEL        18
#define PROV_SPYRUS_LYNKS       20
#define PROV_RNG                21
#define PROV_INTEL_SEC          22
#define PROV_RSA_AES            24

#define MS_DEF_PROV_A           "Microsoft Base Cryptographic Provider v1.0"
#define MS_DEF_PROV_W           L"Microsoft Base Cryptographic Provider v1.0"
#ifdef UNICODE
#define MS_DEF_PROV             MS_DEF_PROV_W
#else
#define MS_DEF_PROV             MS_DEF_PROV_A
#endif

// dwFlags definitions for CryptAcquireContext
#define CRYPT_VERIFYCONTEXT     0xF0000000
#define CRYPT_NEWKEYSET         0x00000008
#define CRYPT_DELETEKEYSET      0x00000010
#define CRYPT_MACHINE_KEYSET    0x00000020
#define CRYPT_SILENT            0x00000040

// Algorithm classes
// certenrolld_begin -- ALG_CLASS_*
#define ALG_CLASS_ANY                   (0)
#define ALG_CLASS_SIGNATURE             (1 << 13)
#define ALG_CLASS_MSG_ENCRYPT           (2 << 13)
#define ALG_CLASS_DATA_ENCRYPT          (3 << 13)
#define ALG_CLASS_HASH                  (4 << 13)
#define ALG_CLASS_KEY_EXCHANGE          (5 << 13)
#define ALG_CLASS_ALL                   (7 << 13)
// certenrolld_end

// Algorithm types
#define ALG_TYPE_ANY                    (0)
#define ALG_TYPE_DSS                    (1 << 9)
#define ALG_TYPE_RSA                    (2 << 9)
#define ALG_TYPE_BLOCK                  (3 << 9)
#define ALG_TYPE_STREAM                 (4 << 9)
#define ALG_TYPE_DH                     (5 << 9)
#define ALG_TYPE_SECURECHANNEL          (6 << 9)

// Hash sub ids
#define ALG_SID_MD2                     1
#define ALG_SID_MD4                     2
#define ALG_SID_MD5                     3
#define ALG_SID_SHA                     4
#define ALG_SID_SHA1                    4
#define ALG_SID_MAC                     5
#define ALG_SID_RIPEMD                  6
#define ALG_SID_RIPEMD160               7
#define ALG_SID_SSL3SHAMD5              8
#define ALG_SID_HMAC                    9
#define ALG_SID_TLS1PRF                 10
#define ALG_SID_SHA_256                 12
#define ALG_SID_SHA_384                 13
#define ALG_SID_SHA_512                 14

#define CALG_MD5                (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_MD5)
#define CALG_SHA_512            (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_SHA_512)

#define HP_ALGID                0x0001  // Hash algorithm
#define HP_HASHVAL              0x0002  // Hash value
#define HP_HASHSIZE             0x0004  // Hash value size
#define HP_HMAC_INFO            0x0005  // information for creating an HMAC
#define HP_TLS1PRF_LABEL        0x0006  // label for TLS1 PRF
#define HP_TLS1PRF_SEED         0x0007  // seed for TLS1 PRF

#define CRYPT_FAILED            FALSE
#define CRYPT_SUCCEED           TRUE

BOOL
WINAPI
CertStrToNameA(
    _In_ DWORD dwCertEncodingType,
    _In_ LPCSTR pszX500,
    _In_ DWORD dwStrType,
    _Reserved_ void *pvReserved,
    _Out_writes_bytes_to_opt_(*pcbEncoded, *pcbEncoded) BYTE *pbEncoded,
    _Inout_ DWORD *pcbEncoded,
    _Outptr_opt_result_maybenull_ LPCSTR *ppszError
    );
//+-------------------------------------------------------------------------
//--------------------------------------------------------------------------

#define CRYPT_ACQUIRE_CACHE_FLAG		0x00000001
#define CRYPT_ACQUIRE_COMPARE_KEY_FLAG		0x00000004

//+-------------------------------------------------------------------------
//  Certificate Store Provider Types
//--------------------------------------------------------------------------
#define CERT_STORE_PROV_MEMORY              ((LPCSTR) 2)
#define CERT_STORE_PROV_SYSTEM_W            ((LPCSTR) 10)
#define CERT_STORE_PROV_SYSTEM              CERT_STORE_PROV_SYSTEM_W

//+-------------------------------------------------------------------------
//  Certificate Store open/property flags
//--------------------------------------------------------------------------
#define CERT_STORE_DEFER_CLOSE_UNTIL_LAST_FREE_FLAG     0x00000004


// From schannel.h

//
//
// Security package names.
//

#define UNISP_NAME_A    "Microsoft Unified Security Protocol Provider"
#define UNISP_NAME_W    L"Microsoft Unified Security Protocol Provider"

#define SSL2SP_NAME_A    "Microsoft SSL 2.0"
#define SSL2SP_NAME_W    L"Microsoft SSL 2.0"

#define SSL3SP_NAME_A    "Microsoft SSL 3.0"
#define SSL3SP_NAME_W    L"Microsoft SSL 3.0"

#define TLS1SP_NAME_A    "Microsoft TLS 1.0"
#define TLS1SP_NAME_W    L"Microsoft TLS 1.0"

#define PCT1SP_NAME_A    "Microsoft PCT 1.0"
#define PCT1SP_NAME_W    L"Microsoft PCT 1.0"

#define SCHANNEL_NAME_A  "Schannel"
#define SCHANNEL_NAME_W  L"Schannel"


#ifdef UNICODE

#define UNISP_NAME  UNISP_NAME_W
#define PCT1SP_NAME  PCT1SP_NAME_W
#define SSL2SP_NAME  SSL2SP_NAME_W
#define SSL3SP_NAME  SSL3SP_NAME_W
#define TLS1SP_NAME  TLS1SP_NAME_W
#define SCHANNEL_NAME  SCHANNEL_NAME_W

#else

#define UNISP_NAME  UNISP_NAME_A
#define PCT1SP_NAME  PCT1SP_NAME_A
#define SSL2SP_NAME  SSL2SP_NAME_A
#define SSL3SP_NAME  SSL3SP_NAME_A
#define TLS1SP_NAME  TLS1SP_NAME_A
#define SCHANNEL_NAME  SCHANNEL_NAME_A

#endif
// End schannel.h

// Security.h

#ifndef MICROSOFT_KERBEROS_NAME_A
#define MICROSOFT_KERBEROS_NAME_A   "Kerberos"
#define MICROSOFT_KERBEROS_NAME_W   L"Kerberos"
#define MICROSOFT_KERBEROS_NAME MICROSOFT_KERBEROS_NAME_W
#endif  // MICROSOFT_KERBEROS_NAME_A

#ifndef NEGOSSP_NAME
#define NEGOSSP_NAME_W  L"Negotiate"
#define NEGOSSP_NAME_A  "Negotiate"

#ifdef UNICODE
#define NEGOSSP_NAME    NEGOSSP_NAME_W
#else
#define NEGOSSP_NAME    NEGOSSP_NAME_A
#endif
#endif


// From Miniport.h - TO go into pal_mstypes.h
typedef LONGLONG *PLONGLONG;
// End Miniport.h
//

// atldef.h

#if !defined(_countof)
#if !defined(__cplusplus) || defined(_PREFAST_)
#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#else
extern "C++"
{
    template <typename _CountofType, size_t _SizeOfArray>
    char (*__countof_helper(UNALIGNED _CountofType (&_Array)[_SizeOfArray]))[_SizeOfArray];
    #define _countof(_Array) sizeof(*__countof_helper(_Array))
}
#endif
#endif

// end atldef.h

#if _LP64
//typedef int64_t _int64;
#endif
//typedef int8_t  _int8;
#define _int64 int64_t
#define _int8 int8_t

// This should be replaced with wcstol or some such in the code base.
extern "C" int _wtoi(const wchar_t *);


SIZE_T
NTAPI
RtlCompareMemory(
    _In_ const VOID * Source1,
    _In_ const VOID * Source2,
    _In_ SIZE_T Length
    );

#define MAXULONG    0xffffffff  // winnt

#define DbgRaiseAssertionFailure() __int2c()

#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "Unknwn.h"

EXTERN_C const IID IID_IXmlReader;

MIDL_INTERFACE("7279FC81-709D-4095-B63D-69FE4B0D9030")
IXmlReader : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE SetInput(
		/* [annotation] */
		_In_opt_  IUnknown *pInput) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetProperty(
		/* [annotation] */
		_In_  UINT nProperty,
		/* [annotation] */
		_Out_  LONG_PTR *ppValue) = 0;

	virtual HRESULT STDMETHODCALLTYPE SetProperty(
		/* [annotation] */
		_In_  UINT nProperty,
		/* [annotation] */
		_In_opt_  LONG_PTR pValue) = 0;

	virtual HRESULT STDMETHODCALLTYPE Read(
		/* [annotation] */
		_Out_opt_  XmlNodeType *pNodeType) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetNodeType(
		/* [annotation] */
		_Out_  XmlNodeType *pNodeType) = 0;

	virtual HRESULT STDMETHODCALLTYPE MoveToFirstAttribute( void) = 0;

	virtual HRESULT STDMETHODCALLTYPE MoveToNextAttribute( void) = 0;

	virtual HRESULT STDMETHODCALLTYPE MoveToAttributeByName(
		/* [annotation] */
		_In_  LPCWSTR pwszLocalName,
		/* [annotation] */
		_In_opt_  LPCWSTR pwszNamespaceUri) = 0;

	virtual HRESULT STDMETHODCALLTYPE MoveToElement( void) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetQualifiedName(
		/* [annotation] */
		_Outptr_result_buffer_maybenull_(*pcwchQualifiedName+1)  LPCWSTR *ppwszQualifiedName,
		/* [annotation] */
		_Out_opt_  UINT *pcwchQualifiedName) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetNamespaceUri(
		/* [annotation] */
		_Outptr_result_buffer_maybenull_(*pcwchNamespaceUri+1)  LPCWSTR *ppwszNamespaceUri,
		/* [annotation] */
		_Out_opt_  UINT *pcwchNamespaceUri) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetLocalName(
		/* [annotation] */
		_Outptr_result_buffer_maybenull_(*pcwchLocalName+1)  LPCWSTR *ppwszLocalName,
		/* [annotation] */
		_Out_opt_  UINT *pcwchLocalName) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetPrefix(
		/* [annotation] */
		_Outptr_result_buffer_maybenull_(*pcwchPrefix+1)  LPCWSTR *ppwszPrefix,
		/* [annotation] */
		_Out_opt_  UINT *pcwchPrefix) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetValue(
		/* [annotation] */
		_Outptr_result_buffer_maybenull_(*pcwchValue+1)  LPCWSTR *ppwszValue,
		/* [annotation] */
		_Out_opt_  UINT *pcwchValue) = 0;

	virtual HRESULT STDMETHODCALLTYPE ReadValueChunk(
		/* [annotation] */
		_Out_writes_to_(cwchChunkSize, *pcwchRead)  WCHAR *pwchBuffer,
		/* [annotation] */
		_In_  UINT cwchChunkSize,
		/* [annotation] */
		_Inout_  UINT *pcwchRead) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetBaseUri(
		/* [annotation] */
		_Outptr_result_buffer_maybenull_(*pcwchBaseUri+1)  LPCWSTR *ppwszBaseUri,
		/* [annotation] */
		_Out_opt_  UINT *pcwchBaseUri) = 0;

	virtual BOOL STDMETHODCALLTYPE IsDefault( void) = 0;

	virtual BOOL STDMETHODCALLTYPE IsEmptyElement( void) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetLineNumber(
		/* [annotation] */
		_Out_  UINT *pnLineNumber) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetLinePosition(
		/* [annotation] */
		_Out_  UINT *pnLinePosition) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetAttributeCount(
		/* [annotation] */
		_Out_  UINT *pnAttributeCount) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetDepth(
		/* [annotation] */
		_Out_  UINT *pnDepth) = 0;

	virtual BOOL STDMETHODCALLTYPE IsEOF( void) = 0;

};

typedef VOID (NTAPI *PTP_TIMER_CALLBACK)(
    PTP_CALLBACK_INSTANCE Instance,
    PVOID Context,
    PTP_TIMER	Timer
    );

typedef
enum XmlStandalone
    {
        XmlStandalone_Omit	= 0,
        XmlStandalone_Yes	= 1,
        XmlStandalone_No	= 2,
        _XmlStandalone_Last	= 2
    } 	XmlStandalone;

EXTERN_C const IID IID_IXmlWriter;

MIDL_INTERFACE("7279FC88-709D-4095-B63D-69FE4B0D9030")
IXmlWriter : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE SetOutput(
		/* [annotation] */
		_In_opt_  IUnknown *pOutput) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetProperty(
		/* [annotation] */
		_In_  UINT nProperty,
		/* [annotation] */
		_Out_  LONG_PTR *ppValue) = 0;

	virtual HRESULT STDMETHODCALLTYPE SetProperty(
		/* [annotation] */
		_In_  UINT nProperty,
		/* [annotation] */
		_In_opt_  LONG_PTR pValue) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteAttributes(
		/* [annotation] */
		_In_  IXmlReader *pReader,
		/* [annotation] */
		_In_  BOOL fWriteDefaultAttributes) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteAttributeString(
		/* [annotation] */
		_In_opt_  LPCWSTR pwszPrefix,
		/* [annotation] */
		_In_opt_  LPCWSTR pwszLocalName,
		/* [annotation] */
		_In_opt_  LPCWSTR pwszNamespaceUri,
		/* [annotation] */
		_In_opt_  LPCWSTR pwszValue) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteCData(
		/* [annotation] */
		_In_opt_  LPCWSTR pwszText) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteCharEntity(
		/* [annotation] */
		_In_  WCHAR wch) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteChars(
		/* [annotation] */
		_In_reads_opt_(cwch)  const WCHAR *pwch,
		/* [annotation] */
		_In_  UINT cwch) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteComment(
		/* [annotation] */
		_In_opt_  LPCWSTR pwszComment) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteDocType(
		/* [annotation] */
		_In_opt_  LPCWSTR pwszName,
		/* [annotation] */
		_In_opt_  LPCWSTR pwszPublicId,
		/* [annotation] */
		_In_opt_  LPCWSTR pwszSystemId,
		/* [annotation] */
		_In_opt_  LPCWSTR pwszSubset) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteElementString(
		/* [annotation] */
		_In_opt_  LPCWSTR pwszPrefix,
		/* [annotation] */
		_In_  LPCWSTR pwszLocalName,
		/* [annotation] */
		_In_opt_  LPCWSTR pwszNamespaceUri,
		/* [annotation] */
		_In_opt_  LPCWSTR pwszValue) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteEndDocument( void) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteEndElement( void) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteEntityRef(
		/* [annotation] */
		_In_  LPCWSTR pwszName) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteFullEndElement( void) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteName(
		/* [annotation] */
		_In_  LPCWSTR pwszName) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteNmToken(
		/* [annotation] */
		_In_  LPCWSTR pwszNmToken) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteNode(
		/* [annotation] */
		_In_  IXmlReader *pReader,
		/* [annotation] */
		_In_  BOOL fWriteDefaultAttributes) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteNodeShallow(
		/* [annotation] */
		_In_  IXmlReader *pReader,
		/* [annotation] */
		_In_  BOOL fWriteDefaultAttributes) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteProcessingInstruction(
		/* [annotation] */
		_In_  LPCWSTR pwszName,
		/* [annotation] */
		_In_opt_  LPCWSTR pwszText) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteQualifiedName(
		/* [annotation] */
		_In_  LPCWSTR pwszLocalName,
		/* [annotation] */
		_In_opt_  LPCWSTR pwszNamespaceUri) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteRaw(
		/* [annotation] */
		_In_opt_  LPCWSTR pwszData) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteRawChars(
		/* [annotation] */
		_In_reads_opt_(cwch)  const WCHAR *pwch,
		/* [annotation] */
		_In_  UINT cwch) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteStartDocument(
		/* [annotation] */
		_In_  XmlStandalone standalone) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteStartElement(
		/* [annotation] */
		_In_opt_  LPCWSTR pwszPrefix,
		/* [annotation] */
		_In_  LPCWSTR pwszLocalName,
		/* [annotation] */
		_In_opt_  LPCWSTR pwszNamespaceUri) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteString(
		/* [annotation] */
		_In_opt_  LPCWSTR pwszText) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteSurrogateCharEntity(
		/* [annotation] */
		_In_  WCHAR wchLow,
		/* [annotation] */
		_In_  WCHAR wchHigh) = 0;

	virtual HRESULT STDMETHODCALLTYPE WriteWhitespace(
		/* [annotation] */
		_In_opt_  LPCWSTR pwszWhitespace) = 0;

	virtual HRESULT STDMETHODCALLTYPE Flush( void) = 0;

};

typedef struct tagSTATSTG
    {
    LPOLESTR pwcsName;
    DWORD type;
    ULARGE_INTEGER cbSize;
    FILETIME mtime;
    FILETIME ctime;
    FILETIME atime;
    DWORD grfMode;
    DWORD grfLocksSupported;
    CLSID clsid;
    DWORD grfStateBits;
    DWORD reserved;
    } 	STATSTG;

typedef
enum tagSTATFLAG
    {
        STATFLAG_DEFAULT	= 0,
        STATFLAG_NONAME	= 1,
        STATFLAG_NOOPEN	= 2
    } 	STATFLAG;

typedef
enum tagSTREAM_SEEK
    {
        STREAM_SEEK_SET	= 0,
        STREAM_SEEK_CUR	= 1,
        STREAM_SEEK_END	= 2
    } 	STREAM_SEEK;

EXTERN_C const IID IID_ISequentialStream;

#if !defined(UNDER_RTCPAL)
MIDL_INTERFACE("0c733a30-2a1c-11ce-ade5-00aa0044773d")
ISequentialStream : public IUnknown
{
public:
	virtual /* [local] */ HRESULT STDMETHODCALLTYPE Read(
		/* [annotation] */
		_Out_writes_bytes_to_(cb, *pcbRead)  void *pv,
		/* [annotation][in] */
		_In_  ULONG cb,
		/* [annotation] */
		_Out_opt_  ULONG *pcbRead) = 0;

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE Write(
		/* [annotation] */
		_In_reads_bytes_(cb)  const void *pv,
		/* [annotation][in] */
		_In_  ULONG cb,
		/* [annotation] */
		_Out_opt_  ULONG *pcbWritten) = 0;

};

EXTERN_C const IID IID_IStream;

MIDL_INTERFACE("0000000c-0000-0000-C000-000000000046")
IStream : public ISequentialStream
{
public:
	virtual /* [local] */ HRESULT STDMETHODCALLTYPE Seek(
		/* [in] */ LARGE_INTEGER dlibMove,
		/* [in] */ DWORD dwOrigin,
		/* [annotation] */
		_Out_opt_  ULARGE_INTEGER *plibNewPosition) = 0;

	virtual HRESULT STDMETHODCALLTYPE SetSize(
		/* [in] */ ULARGE_INTEGER libNewSize) = 0;

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE CopyTo(
		/* [annotation][unique][in] */
		_In_  IStream *pstm,
		/* [in] */ ULARGE_INTEGER cb,
		/* [annotation] */
		_Out_opt_  ULARGE_INTEGER *pcbRead,
		/* [annotation] */
		_Out_opt_  ULARGE_INTEGER *pcbWritten) = 0;

	virtual HRESULT STDMETHODCALLTYPE Commit(
		/* [in] */ DWORD grfCommitFlags) = 0;

	virtual HRESULT STDMETHODCALLTYPE Revert( void) = 0;

	virtual HRESULT STDMETHODCALLTYPE LockRegion(
		/* [in] */ ULARGE_INTEGER libOffset,
		/* [in] */ ULARGE_INTEGER cb,
		/* [in] */ DWORD dwLockType) = 0;

	virtual HRESULT STDMETHODCALLTYPE UnlockRegion(
		/* [in] */ ULARGE_INTEGER libOffset,
		/* [in] */ ULARGE_INTEGER cb,
		/* [in] */ DWORD dwLockType) = 0;

	virtual HRESULT STDMETHODCALLTYPE Stat(
		/* [out] */ __RPC__out STATSTG *pstatstg,
		/* [in] */ DWORD grfStatFlag) = 0;

	virtual HRESULT STDMETHODCALLTYPE Clone(
		/* [out] */ __RPC__deref_out_opt IStream **ppstm) = 0;

};
#endif

typedef IUnknown IXmlReaderInput;
typedef IUnknown IXmlWriterOutput;
MIDL_INTERFACE("00000002-0000-0000-C000-000000000046")
IMalloc : public IUnknown
{
public:
	virtual void *STDMETHODCALLTYPE Alloc(
		/* [annotation][in] */
		_In_  SIZE_T cb) = 0;

	virtual void *STDMETHODCALLTYPE Realloc(
		/* [annotation][in] */
		_In_opt_  void *pv,
		/* [annotation][in] */
		_In_  SIZE_T cb) = 0;

	virtual void STDMETHODCALLTYPE Free(
		/* [annotation][in] */
		_In_opt_  void *pv) = 0;

	virtual SIZE_T STDMETHODCALLTYPE GetSize(
		/* [annotation][in] */
		_In_opt_  void *pv) = 0;

	virtual int STDMETHODCALLTYPE DidAlloc(
		/* [annotation][in] */
		_In_opt_  void *pv) = 0;

	virtual void STDMETHODCALLTYPE HeapMinimize( void) = 0;

};

LWSTDAPI_(IStream *) SHCreateMemStream(_In_reads_bytes_opt_(cbInit) const BYTE *pInit, _In_ UINT cbInit);

STDAPI CreateXmlWriterOutputWithEncodingName(
    _In_ IUnknown *pOutputStream,
    _In_opt_ IMalloc *pMalloc,
    _In_ LPCWSTR pwszEncodingName,
    _Out_ IXmlWriterOutput **ppOutput);

// XmlReader Constructors
STDAPI CreateXmlReader(
    _In_ REFIID riid,
    _In_ LPCWSTR ppwszFileName,
    _Outptr_ void ** ppvObject,
    _In_opt_ IMalloc * pMalloc);

STDAPI CreateXmlWriter(
    _In_ REFIID riid,
    _In_ LPCWSTR ppwszFileName,
    _Out_ void ** ppvObject,
    _In_opt_ IMalloc * pMalloc);

STDAPI CreateMemoryXmlWriter(
    _In_ REFIID riid,
    IUnknown *output,
    _Outptr_ void **ppvObject,
    _In_opt_ IMalloc *pMalloc);

typedef
enum XmlWriterProperty
    {
        XmlWriterProperty_MultiLanguage	= 0,
        XmlWriterProperty_Indent	= ( XmlWriterProperty_MultiLanguage + 1 ) ,
        XmlWriterProperty_ByteOrderMark	= ( XmlWriterProperty_Indent + 1 ) ,
        XmlWriterProperty_OmitXmlDeclaration	= ( XmlWriterProperty_ByteOrderMark + 1 ) ,
        XmlWriterProperty_ConformanceLevel	= ( XmlWriterProperty_OmitXmlDeclaration + 1 ) ,
        _XmlWriterProperty_Last	= XmlWriterProperty_OmitXmlDeclaration
    } 	XmlWriterProperty;

// end_winnt

typedef struct _IO_STATUS_BLOCK {
    union {
        NTSTATUS Status;
        PVOID Pointer;
    } ;//DUMMYUNIONNAME;

    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define AI_CANONNAME                0x00000002  // Return canonical name in first ai_canonname

typedef addrinfo ADDRINFOW;
typedef ADDRINFOW * PADDRINFOW;

VOID
WSAAPI
FreeAddrInfoW(
    _In_opt_        PADDRINFOW      pAddrInfo
    );

INT
WSAAPI
GetAddrInfoW(
    _In_opt_        PCWSTR              pNodeName,
    _In_opt_        PCWSTR              pServiceName,
    _In_opt_        const ADDRINFOW *   pHints,
    _Outptr_        PADDRINFOW *        ppResult
    );

// KTL
typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
    PVOID SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;
typedef CONST OBJECT_ATTRIBUTES *PCOBJECT_ATTRIBUTES;

typedef CONST WCHAR *LPCWCHAR, *PCWCHAR;
typedef CONST LONG CLONG;

//
// Define the file information class values
//
// WARNING:  The order of the following values are assumed by the I/O system.
//           Any changes made here should be reflected there as well.
//

typedef enum _FILE_INFORMATION_CLASS {
    FileDirectoryInformation         = 1,
    FileFullDirectoryInformation,   // 2
    FileBothDirectoryInformation,   // 3
    FileBasicInformation,           // 4
    FileStandardInformation,        // 5
    FileInternalInformation,        // 6
    FileEaInformation,              // 7
    FileAccessInformation,          // 8
    FileNameInformation,            // 9
    FileRenameInformation,          // 10
    FileLinkInformation,            // 11
    FileNamesInformation,           // 12
    FileDispositionInformation,     // 13
    FilePositionInformation,        // 14
    FileFullEaInformation,          // 15
    FileModeInformation,            // 16
    FileAlignmentInformation,       // 17
    FileAllInformation,             // 18
    FileAllocationInformation,      // 19
    FileEndOfFileInformation,       // 20
    FileAlternateNameInformation,   // 21
    FileStreamInformation,          // 22
    FilePipeInformation,            // 23
    FilePipeLocalInformation,       // 24
    FilePipeRemoteInformation,      // 25
    FileMailslotQueryInformation,   // 26
    FileMailslotSetInformation,     // 27
    FileCompressionInformation,     // 28
    FileObjectIdInformation,        // 29
    FileCompletionInformation,      // 30
    FileMoveClusterInformation,     // 31
    FileQuotaInformation,           // 32
    FileReparsePointInformation,    // 33
    FileNetworkOpenInformation,     // 34
    FileAttributeTagInformation,    // 35
    FileTrackingInformation,        // 36
    FileIdBothDirectoryInformation, // 37
    FileIdFullDirectoryInformation, // 38
    FileValidDataLengthInformation, // 39
    FileShortNameInformation,       // 40
    FileIoCompletionNotificationInformation, // 41
    FileIoStatusBlockRangeInformation,       // 42
    FileIoPriorityHintInformation,           // 43
    FileSfioReserveInformation,              // 44
    FileSfioVolumeInformation,               // 45
    FileHardLinkInformation,                 // 46
    FileProcessIdsUsingFileInformation,      // 47
    FileNormalizedNameInformation,           // 48
    FileNetworkPhysicalNameInformation,      // 49
    FileIdGlobalTxDirectoryInformation,      // 50
    FileIsRemoteDeviceInformation,           // 51
    FileUnusedInformation,                   // 52
    FileNumaNodeInformation,                 // 53
    FileStandardLinkInformation,             // 54
    FileRemoteProtocolInformation,           // 55

        //
        //  These are special versions of these operations (defined earlier)
        //  which can be used by kernel mode drivers only to bypass security
        //  access checks for Rename and HardLink operations.  These operations
        //  are only recognized by the IOManager, a file system should never
        //  receive these.
        //
    FileRenameInformationBypassAccessCheck,  // 56
    FileLinkInformationBypassAccessCheck,    // 57
    FileVolumeNameInformation,               // 58
    FileIdInformation,                       // 59
    FileIdExtdDirectoryInformation,          // 60
    FileReplaceCompletionInformation,        // 61
    FileHardLinkFullIdInformation,           // 62
    FileMaximumInformation
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

//
// System Information Classes.
// Do not delete a value from this enum! If a value is to be removed please
// replace it with SystemSpare# where # is the first available number.
//

typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation,
    SystemProcessorInformation,             // obsolete...delete
    SystemPerformanceInformation,
    SystemTimeOfDayInformation,
    SystemPathInformation,
    SystemProcessInformation,
    SystemCallCountInformation,
    SystemDeviceInformation,
    SystemProcessorPerformanceInformation,
    SystemFlagsInformation,
    SystemCallTimeInformation,
    SystemModuleInformation,
    SystemLocksInformation,
    SystemStackTraceInformation,
    SystemPagedPoolInformation,
    SystemNonPagedPoolInformation,
    SystemHandleInformation,
    SystemObjectInformation,
    SystemPageFileInformation,
    SystemVdmInstemulInformation,
    SystemVdmBopInformation,
    SystemFileCacheInformation,
    SystemPoolTagInformation,
    SystemInterruptInformation,
    SystemDpcBehaviorInformation,
    SystemFullMemoryInformation,
    SystemLoadGdiDriverInformation,
    SystemUnloadGdiDriverInformation,
    SystemTimeAdjustmentInformation,
    SystemSummaryMemoryInformation,
    SystemMirrorMemoryInformation,
    SystemPerformanceTraceInformation,
    SystemObsolete0,
    SystemExceptionInformation,
    SystemCrashDumpStateInformation,
    SystemKernelDebuggerInformation,
    SystemContextSwitchInformation,
    SystemRegistryQuotaInformation,
    SystemExtendServiceTableInformation,
    SystemPrioritySeperation,
    SystemVerifierAddDriverInformation,
    SystemVerifierRemoveDriverInformation,
    SystemProcessorIdleInformation,
    SystemLegacyDriverInformation,
    SystemCurrentTimeZoneInformation,
    SystemLookasideInformation,
    SystemTimeSlipNotification,
    SystemSessionCreate,
    SystemSessionDetach,
    SystemSessionInformation,
    SystemRangeStartInformation,
    SystemVerifierInformation,
    SystemVerifierThunkExtend,
    SystemSessionProcessInformation,
    SystemLoadGdiDriverInSystemSpace,
    SystemNumaProcessorMap,
    SystemPrefetcherInformation,
    SystemExtendedProcessInformation,
    SystemRecommendedSharedDataAlignment,
    SystemComPlusPackage,
    SystemNumaAvailableMemory,
    SystemProcessorPowerInformation,
    SystemEmulationBasicInformation,
    SystemEmulationProcessorInformation,
    SystemExtendedHandleInformation,
    SystemLostDelayedWriteInformation,
    SystemBigPoolInformation,
    SystemSessionPoolTagInformation,
    SystemSessionMappedViewInformation,
    SystemHotpatchInformation,
    SystemObjectSecurityMode,
    SystemWatchdogTimerHandler,
    SystemWatchdogTimerInformation,
    SystemLogicalProcessorInformation,
    SystemWow64SharedInformationObsolete,
    SystemRegisterFirmwareTableInformationHandler,
    SystemFirmwareTableInformation,
    SystemModuleInformationEx,
    SystemVerifierTriageInformation,
    SystemSuperfetchInformation,
    SystemMemoryListInformation,
    SystemFileCacheInformationEx,
    SystemThreadPriorityClientIdInformation,
    SystemProcessorIdleCycleTimeInformation,
    SystemVerifierCancellationInformation,
    SystemProcessorPowerInformationEx,
    SystemRefTraceInformation,
    SystemSpecialPoolInformation,
    SystemProcessIdInformation,
    SystemErrorPortInformation,
    SystemBootEnvironmentInformation,
    SystemHypervisorInformation,
    SystemVerifierInformationEx,
    SystemTimeZoneInformation,
    SystemImageFileExecutionOptionsInformation,
    SystemCoverageInformation,
    SystemPrefetchPatchInformation,
    SystemVerifierFaultsInformation,
    SystemSystemPartitionInformation,
    SystemSystemDiskInformation,
    SystemProcessorPerformanceDistribution,
    SystemNumaProximityNodeInformation,
    SystemDynamicTimeZoneInformation,
    SystemCodeIntegrityInformation,
    SystemProcessorMicrocodeUpdateInformation,
    SystemProcessorBrandString,
    SystemVirtualAddressInformation,
    SystemLogicalProcessorAndGroupInformation,
    SystemProcessorCycleTimeInformation,
    SystemStoreInformation,
    SystemRegistryAppendString,
    SystemAitSamplingValue,
    SystemVhdBootInformation,
    SystemCpuQuotaInformation,
    SystemNativeBasicInformation,
    SystemErrorPortTimeouts,
    SystemLowPriorityIoInformation,
    SystemBootEntropyInformation,
    SystemVerifierCountersInformation,
    SystemPagedPoolInformationEx,
    SystemSystemPtesInformationEx,
    SystemNodeDistanceInformation,
    SystemAcpiAuditInformation,
    SystemBasicPerformanceInformation,
    SystemQueryPerformanceCounterInformation,
    SystemSessionBigPoolInformation,
    SystemBootGraphicsInformation,
    SystemScrubPhysicalMemoryInformation,
    SystemBadPageInformation,
    SystemProcessorProfileControlArea,
    SystemCombinePhysicalMemoryInformation,
    SystemEntropyInterruptTimingInformation,
    SystemConsoleInformation,
    SystemPlatformBinaryInformation,
    SystemPolicyInformation,
    SystemHypervisorProcessorCountInformation,
    SystemDeviceDataInformation,
    SystemDeviceDataEnumerationInformation,
    SystemMemoryTopologyInformation,
    SystemMemoryChannelInformation,
    SystemBootLogoInformation,
    SystemProcessorPerformanceInformationEx,
    SystemSpare0,
    SystemSecureBootPolicyInformation,
    SystemPageFileInformationEx,
    SystemSecureBootInformation,
    SystemEntropyInterruptTimingRawInformation,
    SystemPortableWorkspaceEfiLauncherInformation,
    SystemFullProcessInformation,
    SystemKernelDebuggerInformationEx,
    SystemBootMetadataInformation,
    SystemSoftRebootInformation,
    SystemElamCertificateInformation,
    SystemOfflineDumpConfigInformation,
    SystemProcessorFeaturesInformation,
    SystemRegistryReconciliationInformation,
    SystemEdidInformation,
    MaxSystemInfoClass  // MaxSystemInfoClass should always be the last enum
} SYSTEM_INFORMATION_CLASS;

typedef enum _EVENT_TYPE {
    NotificationEvent,
    SynchronizationEvent
} EVENT_TYPE;

//
// Define the file system information class values
//
// WARNING:  The order of the following values are assumed by the I/O system.
//           Any changes made here should be reflected there as well.

typedef enum _FSINFOCLASS {
    FileFsVolumeInformation       = 1,
    FileFsLabelInformation,      // 2
    FileFsSizeInformation,       // 3
    FileFsDeviceInformation,     // 4
    FileFsAttributeInformation,  // 5
    FileFsControlInformation,    // 6
    FileFsFullSizeInformation,   // 7
    FileFsObjectIdInformation,   // 8
    FileFsDriverPathInformation, // 9
    FileFsVolumeFlagsInformation,// 10
    FileFsSectorSizeInformation, // 11
    FileFsDataCopyInformation,   // 12
    FileFsMaximumInformation
} FS_INFORMATION_CLASS, *PFS_INFORMATION_CLASS;

typedef
VOID
(NTAPI *PIO_APC_ROUTINE) (
    _In_ PVOID ApcContext,
    _In_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG Reserved
    );

//#define PIO_APC_ROUTINE_DEFINED
typedef struct _FILE_NETWORK_OPEN_INFORMATION {
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG FileAttributes;
} FILE_NETWORK_OPEN_INFORMATION, *PFILE_NETWORK_OPEN_INFORMATION;

#define __drv_functionClass(x)
#define __drv_sameIRQL
#define _IRQL_requires_same_
#define __drv_allocatesMem(kind)
#define __drv_freesMem(kind)

typedef enum _RTL_GENERIC_COMPARE_RESULTS {
    GenericLessThan,
    GenericGreaterThan,
    GenericEqual
} RTL_GENERIC_COMPARE_RESULTS;

typedef struct _RTL_BALANCED_LINKS {
    struct _RTL_BALANCED_LINKS *Parent;
    struct _RTL_BALANCED_LINKS *LeftChild;
    struct _RTL_BALANCED_LINKS *RightChild;
    CHAR Balance;
    UCHAR Reserved[3];
} RTL_BALANCED_LINKS;
typedef RTL_BALANCED_LINKS *PRTL_BALANCED_LINKS;

typedef
_IRQL_requires_same_
_Function_class_(RTL_AVL_COMPARE_ROUTINE)
RTL_GENERIC_COMPARE_RESULTS
NTAPI
RTL_AVL_COMPARE_ROUTINE (
    _In_ struct _RTL_AVL_TABLE *Table,
    _In_ PVOID FirstStruct,
    _In_ PVOID SecondStruct
    );
typedef RTL_AVL_COMPARE_ROUTINE *PRTL_AVL_COMPARE_ROUTINE;
typedef
_IRQL_requires_same_
_Function_class_(RTL_AVL_ALLOCATE_ROUTINE)
__drv_allocatesMem(Mem)
PVOID
NTAPI
RTL_AVL_ALLOCATE_ROUTINE (
    _In_ struct _RTL_AVL_TABLE *Table,
    _In_ CLONG ByteSize
    );
typedef RTL_AVL_ALLOCATE_ROUTINE *PRTL_AVL_ALLOCATE_ROUTINE;

typedef
_IRQL_requires_same_
_Function_class_(RTL_AVL_FREE_ROUTINE)
VOID
NTAPI
RTL_AVL_FREE_ROUTINE (
    _In_ struct _RTL_AVL_TABLE *Table,
    _In_ __drv_freesMem(Mem) _Post_invalid_ PVOID Buffer
    );
typedef RTL_AVL_FREE_ROUTINE *PRTL_AVL_FREE_ROUTINE;


typedef struct _RTL_AVL_TABLE {
    RTL_BALANCED_LINKS BalancedRoot;
    PVOID OrderedPointer;
    ULONG WhichOrderedElement;
    ULONG NumberGenericTableElements;
    ULONG DepthOfTree;
    PRTL_BALANCED_LINKS RestartKey;
    ULONG DeleteCount;
    PRTL_AVL_COMPARE_ROUTINE CompareRoutine;
    PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine;
    PRTL_AVL_FREE_ROUTINE FreeRoutine;
    PVOID TableContext;
} RTL_AVL_TABLE;
typedef RTL_AVL_TABLE *PRTL_AVL_TABLE;

typedef struct _STRING {
	USHORT Length;
	USHORT MaximumLength;
#ifdef MIDL_PASS
	[size_is(MaximumLength), length_is(Length) ]
#endif // MIDL_PASS
	PCHAR Buffer;
} STRING;
typedef STRING *PSTRING;
typedef STRING ANSI_STRING;

typedef char CCHAR;
typedef short CSHORT;

typedef CCHAR *PCCHAR;

typedef struct _TIME_FIELDS {
    CSHORT Year;        // range [1601...]
    CSHORT Month;       // range [1..12]
    CSHORT Day;         // range [1..31]
    CSHORT Hour;        // range [0..23]
    CSHORT Minute;      // range [0..59]
    CSHORT Second;      // range [0..59]
    CSHORT Milliseconds;// range [0..999]
    CSHORT Weekday;     // range [0..6] == [Sunday..Saturday]
} TIME_FIELDS;
typedef TIME_FIELDS *PTIME_FIELDS;


#define MAXUSHORT   0xffff      // winnt

#define SYNCHRONIZE                      (0x00100000L)
#define STANDARD_RIGHTS_REQUIRED         (0x000F0000L)
#define THREAD_ALL_ACCESS         (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | \
                                   0xFFFF)

LONG
RtlCompareUnicodeString(
    _In_ PCUNICODE_STRING String1,
    _In_ PCUNICODE_STRING String2,
    _In_ BOOLEAN CaseInSensitive
    );

typedef ULONG64 TRACEHANDLE, *PTRACEHANDLE;
typedef ULONGLONG REGHANDLE, *PREGHANDLE;

#define FORCEINLINE __inline
#define DECLSPEC_NOINLINE
#define SYSTEM_CACHE_ALIGNMENT_SIZE 64
#define DECLSPEC_CACHEALIGN DECLSPEC_ALIGN(SYSTEM_CACHE_ALIGNMENT_SIZE)

ULONG
EVNTAPI
EventUnregister(
    _In_ REGHANDLE RegHandle
    );
#define EVENT_CONTROL_CODE_DISABLE_PROVIDER 0
#define EVENT_CONTROL_CODE_ENABLE_PROVIDER  1
#define EVENT_CONTROL_CODE_CAPTURE_STATE    2

#define _Null_terminated_
typedef _Null_terminated_ char* STRSAFE_LPSTR;

typedef __nullterminated const char* STRSAFE_LPCSTR;
typedef __nullterminated wchar_t* STRSAFE_LPWSTR;
typedef __nullterminated const wchar_t* STRSAFE_LPCWSTR;

typedef _Null_terminated_ char* NTSTRSAFE_PSTR;
typedef _Null_terminated_ const char* NTSTRSAFE_PCSTR;
typedef _Null_terminated_ wchar_t* NTSTRSAFE_PWSTR;
typedef _Null_terminated_ const wchar_t* NTSTRSAFE_PCWSTR;
typedef _Null_terminated_ const wchar_t UNALIGNED* NTSTRSAFE_PCUWSTR;

typedef  const char* STRSAFE_PCNZCH;
typedef  const wchar_t* STRSAFE_PCNZWCH;
typedef  const wchar_t UNALIGNED* STRSAFE_PCUNZWCH;


// STRSAFE error return codes
//
#define STRSAFE_E_INSUFFICIENT_BUFFER       ((HRESULT)0x8007007AL)  // 0x7A = 122L = ERROR_INSUFFICIENT_BUFFER
#define STRSAFE_E_INVALID_PARAMETER         ((HRESULT)0x80070057L)  // 0x57 =  87L = ERROR_INVALID_PARAMETER
#define STRSAFE_E_END_OF_FILE               ((HRESULT)0x80070026L)  // 0x26 =  38L = ERROR_HANDLE_EOF

#define MAXULONGLONG ((ULONGLONG)~((ULONGLONG)0))
#define MAXLONGLONG  (0x7fffffffffffffff)

BOOL
WINAPI
HeapDestroy(
    _In_ HANDLE hHeap
    );

#define NULL 0

#define SYNCHRONIZE                      (0x00100000L)
#define FILE_EXECUTE                     ( 0x0020 )    // file
#define READ_CONTROL                     (0x00020000L)
#define STANDARD_RIGHTS_EXECUTE          (READ_CONTROL)
#define FILE_GENERIC_EXECUTE      (STANDARD_RIGHTS_EXECUTE  |\
                                   FILE_READ_ATTRIBUTES     |\
                                   FILE_EXECUTE             |\
                                   SYNCHRONIZE)
#define FILE_GENERIC_WRITE        (STANDARD_RIGHTS_WRITE    |\
                                   FILE_WRITE_DATA          |\
                                   FILE_WRITE_ATTRIBUTES    |\
                                   FILE_WRITE_EA            |\
                                   FILE_APPEND_DATA         |\
                                   SYNCHRONIZE)

#if !defined(UNDER_RTCPAL)
typedef unsigned long TimeStamp;
typedef unsigned long * PTimeStamp;
#endif


#define RTL_RUN_ONCE_INIT {0}   // Static initializer
#define INIT_ONCE_STATIC_INIT   RTL_RUN_ONCE_INIT

typedef struct in_addr IN_ADDR, *PIN_ADDR, FAR *LPIN_ADDR;

extern CONST IN_ADDR in4addr_any;

#define ERROR_NETNAME_DELETED            64L

#define __deref_in_ecount(A)

#define __in_

__inline
HRESULT
SizeTToULong(
    _In_ ULONGLONG ullOperand,
    _Out_ _Deref_out_range_(== , ullOperand) ULONG* pulResult)
{
    HRESULT hr;

    if (ullOperand <= ULONG_MAX)
    {
        *pulResult = (ULONG)ullOperand;
        hr = S_OK;
    }
    else
    {
        *pulResult = ULONG_ERROR;
        hr = INTSAFE_E_ARITHMETIC_OVERFLOW;
    }

    return hr;
}

#define LONG_ERROR      (-1L)
#ifndef INTSAFE_E_ARITHMETIC_OVERFLOW
#define INTSAFE_E_ARITHMETIC_OVERFLOW   ((HRESULT)0x80070216L)  // 0x216 = 534 = ERROR_ARITHMETIC_OVERFLOW
#endif

__inline
HRESULT
SizeTToLong(
    _In_ UINT_PTR uOperand,
    _Out_ _Deref_out_range_(== , uOperand) LONG* plResult)
{
    HRESULT hr;

    if (uOperand <= LONG_MAX)
    {
        *plResult = (LONG)uOperand;
        hr = S_OK;
    }
    else
    {
        *plResult = LONG_ERROR;
        hr = INTSAFE_E_ARITHMETIC_OVERFLOW;
    }

    return hr;
}

BOOL
WINAPI
CancelIoEx(
    _In_ HANDLE hFile,
    _In_opt_ LPOVERLAPPED lpOverlapped
    );

typedef struct _SYMBOL_INFO {
    ULONG       SizeOfStruct;
    ULONG       TypeIndex;        // Type Index of symbol
    ULONG64     Reserved[2];
    ULONG       Index;
    ULONG       Size;
    ULONG64     ModBase;          // Base Address of module comtaining this symbol
    ULONG       Flags;
    ULONG64     Value;            // Value of symbol, ValuePresent should be 1
    ULONG64     Address;          // Address of symbol including base address of module
    ULONG       Register;         // register holding value or pointer to value
    ULONG       Scope;            // scope of the symbol
    ULONG       Tag;              // pdb classification
    ULONG       NameLen;          // Actual length of name
    ULONG       MaxNameLen;
    CHAR        Name[1];          // Name of symbol
} SYMBOL_INFO, *PSYMBOL_INFO;

BOOL
SymFromAddr(
    __in HANDLE hProcess,
    __in DWORD64 Address,
    __out_opt PDWORD64 Displacement,
    __inout PSYMBOL_INFO Symbol
);
typedef struct _IMAGEHLP_LINE64 {
    DWORD    SizeOfStruct;           // set to sizeof(IMAGEHLP_LINE64)
    PVOID    Key;                    // internal
    DWORD    LineNumber;             // line number in file
    PCHAR    FileName;               // full filename
    DWORD64  Address;                // first instruction of line
} IMAGEHLP_LINE64, *PIMAGEHLP_LINE64;

typedef void * PVOID64;

typedef union _FILE_SEGMENT_ELEMENT {
    PVOID64 Buffer;
    ULONGLONG Alignment;
}FILE_SEGMENT_ELEMENT, *PFILE_SEGMENT_ELEMENT;


NTSTATUS
NTAPI
NtQuerySystemInformation(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _Out_ PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_ PULONG ReturnLength
);

#define szOID_COMMON_NAME                   "2.5.4.3"  // case-ignore string

#define CERT_RDN_ANY_TYPE                0
#define CERT_UNICODE_IS_RDN_ATTRS_FLAG              0x1

#define CRYPT_E_NOT_FOUND _HRESULT_TYPEDEF_(0x80092004)

int _kbhit();

#define MAXUINT64   ((UINT64)~((UINT64)0))
#define MAXINT64    ((INT64)(MAXUINT64 >> 1))
#define MININT64    ((INT64)~MAXINT64)

typedef enum {
    ScopeLevelInterface    = 1,
    ScopeLevelLink         = 2,
    ScopeLevelSubnet       = 3,
    ScopeLevelAdmin        = 4,
    ScopeLevelSite         = 5,
    ScopeLevelOrganization = 8,
    ScopeLevelGlobal       = 14,
    ScopeLevelCount        = 16
} SCOPE_LEVEL;

#define IN6ADDR_LINKLOCALPREFIX_LENGTH 64

extern const in6_addr in6addr_linklocalprefix;

typedef void *HWINSTA;
typedef void *HDESK;
HWINSTA WINAPI GetProcessWindowStation(void);

typedef HANDLE SC_HANDLE;
typedef HANDLE SERVICE_STATUS_HANDLE;
#define SERVICE_STOPPED                        0x00000001
#define SERVICE_START_PENDING                  0x00000002
#define SERVICE_STOP_PENDING                   0x00000003
#define SERVICE_RUNNING                        0x00000004
#define SERVICE_CONTINUE_PENDING               0x00000005
#define SERVICE_PAUSE_PENDING                  0x00000006
#define SERVICE_PAUSED                         0x00000007


typedef struct _SERVICE_SID_INFO {
    DWORD       dwServiceSidType;     // Service SID type
} SERVICE_SID_INFO, *LPSERVICE_SID_INFO;

typedef struct _WNODE_HEADER {
  ULONG BufferSize;
  ULONG ProviderId;
  union {
    ULONG64 HistoricalContext;
    struct {
      ULONG Version;
      ULONG Linkage;
    };
  };
  union {
    HANDLE        KernelHandle;
    LARGE_INTEGER TimeStamp;
  };
  GUID  Guid;
  ULONG ClientContext;
  ULONG Flags;
} WNODE_HEADER, *PWNODE_HEADER;

typedef struct _EVENT_TRACE_PROPERTIES {
  WNODE_HEADER Wnode;
  ULONG        BufferSize;
  ULONG        MinimumBuffers;
  ULONG        MaximumBuffers;
  ULONG        MaximumFileSize;
  ULONG        LogFileMode;
  ULONG        FlushTimer;
  ULONG        EnableFlags;
  LONG         AgeLimit;
  ULONG        NumberOfBuffers;
  ULONG        FreeBuffers;
  ULONG        EventsLost;
  ULONG        BuffersWritten;
  ULONG        LogBuffersLost;
  ULONG        RealTimeBuffersLost;
  HANDLE       LoggerThreadId;
  ULONG        LogFileNameOffset;
  ULONG        LoggerNameOffset;
} EVENT_TRACE_PROPERTIES, *PEVENT_TRACE_PROPERTIES;

#define SE_DEBUG_NAME L"SeDebugPrivilege"

#define SYMBOLIC_LINK_FLAG_DIRECTORY (0x1)
#define DETACHED_PROCESS 0x00000008

#define ERROR_SERVICE_DOES_NOT_EXIST    1060L
#define ERROR_CONTROL_C_EXIT            572L
#define ERROR_SERVICE_ALREADY_RUNNING   1056L
#define ERROR_SERVICE_EXISTS            1073L

#define SERVICE_AUTO_START              0x00000002L

DWORD WINAPI GetProcessId(
  _In_ HANDLE Process
);

BOOLEAN WINAPI CreateSymbolicLink(
  _In_ LPCTSTR lpSymlinkFileName,
  _In_ LPCTSTR lpTargetFileName,
  _In_ DWORD  dwFlags
);

#define LOGON32_LOGON_INTERACTIVE        2
#define LOGON32_LOGON_NETWORK_CLEARTEXT  8
#define LOGON32_PROVIDER_DEFAULT         0

#define SYNCHRONIZE                      (0x00100000L)
#define READ_CONTROL                     (0x00020000L)
#define STANDARD_RIGHTS_READ             (READ_CONTROL)
#define STANDARD_RIGHTS_WRITE            (READ_CONTROL)
#define STANDARD_RIGHTS_EXECUTE          (READ_CONTROL)
#define STANDARD_RIGHTS_REQUIRED         (0x000F0000L)

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

#define WNODE_FLAG_TRACED_GUID   0x00020000 // denotes a trace
#define EVENT_TRACE_FILE_MODE_NEWFILE       0x00000008  // Auto-switch log file
#define EVENT_TRACE_CONTROL_QUERY           0
#define EVENT_TRACE_CONTROL_STOP            1
#define EVENT_TRACE_CONTROL_UPDATE          2

#define TRACE_LEVEL_NONE        0   // Tracing is not on
#define TRACE_LEVEL_CRITICAL    1   // Abnormal exit or termination
#define TRACE_LEVEL_FATAL       1   // Deprecated name for Abnormal exit or termination
#define TRACE_LEVEL_ERROR       2   // Severe errors that need logging
#define TRACE_LEVEL_WARNING     3   // Warnings such as allocation failure
#define TRACE_LEVEL_INFORMATION 4   // Includes non-error cases(e.g.,Entry-Exit)
#define TRACE_LEVEL_VERBOSE     5   // Detailed traces from intermediate steps
#define TRACE_LEVEL_RESERVED6   6
#define TRACE_LEVEL_RESERVED7   7
#define TRACE_LEVEL_RESERVED8   8
#define TRACE_LEVEL_RESERVED9   9

#define ERROR_WMI_INSTANCE_NOT_FOUND     4201L

#define COINIT_MULTITHREADED     0
#define COINIT_SINGLETHREADED    1
#define COINIT_APARTMENTTHREADED 2

typedef long DISPID;

typedef enum NET_FW_PROFILE_TYPE2_
{
    NET_FW_PROFILE2_DOMAIN    = 0x1,
    NET_FW_PROFILE2_PRIVATE    = 0x2,
    NET_FW_PROFILE2_PUBLIC    = 0x4,
    NET_FW_PROFILE2_ALL    = 0x7fffffff
} NET_FW_PROFILE_TYPE2;

typedef interface INetFwPolicy2 INetFwPolicy2;
typedef class NetFwPolicy2 NetFwPolicy2;

typedef struct tagDISPPARAMS
{
/* [size_is] */ VARIANTARG *rgvarg;
/* [size_is] */ DISPID *rgdispidNamedArgs;
    UINT cArgs;
    UINT cNamedArgs;
} DISPPARAMS;

typedef struct tagEXCEPINFO {
    WORD  wCode;
    WORD  wReserved;
    BSTR  bstrSource;
    BSTR  bstrDescription;
    BSTR  bstrHelpFile;
    DWORD dwHelpContext;
    PVOID pvReserved;
    HRESULT (__stdcall *pfnDeferredFillIn)(struct tagEXCEPINFO *);
    SCODE scode;
} EXCEPINFO, * LPEXCEPINFO;

typedef enum NET_FW_RULE_DIRECTION_
{
    NET_FW_RULE_DIR_IN	= 1,
    NET_FW_RULE_DIR_OUT	= ( NET_FW_RULE_DIR_IN + 1 ) ,
    NET_FW_RULE_DIR_MAX	= ( NET_FW_RULE_DIR_OUT + 1 )
} NET_FW_RULE_DIRECTION;

typedef enum NET_FW_ACTION_
{
    NET_FW_ACTION_BLOCK	= 0,
    NET_FW_ACTION_ALLOW	= ( NET_FW_ACTION_BLOCK + 1 ) ,
    NET_FW_ACTION_MAX	= ( NET_FW_ACTION_ALLOW + 1 )
}NET_FW_ACTION;

MIDL_INTERFACE("00020400-0000-0000-C000-000000000046")
IDispatch : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(
        /* [out] */ __RPC__out UINT *pctinfo) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(
        /* [in] */ UINT iTInfo,
        /* [in] */ LCID lcid,
        /* [out] */ __RPC__deref_out_opt ITypeInfo **ppTInfo) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(
        /* [in] */ __RPC__in REFIID riid,
        /* [size_is][in] */ __RPC__in_ecount_full(cNames) LPOLESTR *rgszNames,
        /* [range][in] */ UINT cNames,
        /* [in] */ LCID lcid,
        /* [size_is][out] */DISPID *rgDispId) = 0;
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Invoke(
        /* [annotation][in] */
        _In_  DISPID dispIdMember,
        /* [annotation][in] */
        _In_  REFIID riid,
        /* [annotation][in] */
        _In_  LCID lcid,
        /* [annotation][in] */
        _In_  WORD wFlags,
        /* [annotation][out][in] */
        _In_  DISPPARAMS *pDispParams,
        /* [annotation][out] */
        _Out_opt_  VARIANT *pVarResult,
        /* [annotation][out] */
        _Out_opt_  EXCEPINFO *pExcepInfo,
        /* [annotation][out] */
        _Out_opt_  UINT *puArgErr) = 0;
};

MIDL_INTERFACE("AF230D27-BABA-4E42-ACED-F524F22CFCE2")
INetFwRule : public IDispatch
{
public:
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Name(
        /* [retval][out] */ BSTR *name) = 0;
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Name(
        /* [in] */ BSTR name) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Description(
        /* [retval][out] */ BSTR *desc) = 0;
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Description(
        /* [in] */ BSTR desc) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ApplicationName(
        /* [retval][out] */ BSTR *imageFileName) = 0;
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_ApplicationName(
        /* [in] */ BSTR imageFileName) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ServiceName(
        /* [retval][out] */ BSTR *serviceName) = 0;
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_ServiceName(
        /* [in] */ BSTR serviceName) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Protocol(
        /* [retval][out] */ LONG *protocol) = 0;
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Protocol(
        /* [in] */ LONG protocol) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_LocalPorts(
        /* [retval][out] */ BSTR *portNumbers) = 0;
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_LocalPorts(
        /* [in] */ BSTR portNumbers) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_RemotePorts(
        /* [retval][out] */ BSTR *portNumbers) = 0;
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_RemotePorts(
        /* [in] */ BSTR portNumbers) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_LocalAddresses(
        /* [retval][out] */ BSTR *localAddrs) = 0;
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_LocalAddresses(
        /* [in] */ BSTR localAddrs) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_RemoteAddresses(
        /* [retval][out] */ BSTR *remoteAddrs) = 0;
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_RemoteAddresses(
        /* [in] */ BSTR remoteAddrs) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_IcmpTypesAndCodes(
        /* [retval][out] */ BSTR *icmpTypesAndCodes) = 0;
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_IcmpTypesAndCodes(
        /* [in] */ BSTR icmpTypesAndCodes) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Direction(
        /* [retval][out] */ NET_FW_RULE_DIRECTION *dir) = 0;
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Direction(
        /* [in] */ NET_FW_RULE_DIRECTION dir) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Interfaces(
        /* [retval][out] */ VARIANT *interfaces) = 0;
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Interfaces(
        /* [in] */ VARIANT interfaces) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_InterfaceTypes(
        /* [retval][out] */ BSTR *interfaceTypes) = 0;
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_InterfaceTypes(
        /* [in] */ BSTR interfaceTypes) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Enabled(
        /* [retval][out] */ VARIANT_BOOL *enabled) = 0;
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Enabled(
        /* [in] */ VARIANT_BOOL enabled) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Grouping(
        /* [retval][out] */ BSTR *context) = 0;
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Grouping(
        /* [in] */ BSTR context) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Profiles(
        /* [retval][out] */ long *profileTypesBitmask) = 0;
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Profiles(
        /* [in] */ long profileTypesBitmask) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_EdgeTraversal(
        /* [retval][out] */ VARIANT_BOOL *enabled) = 0;
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_EdgeTraversal(
        /* [in] */ VARIANT_BOOL enabled) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Action(
        /* [retval][out] */ NET_FW_ACTION *action) = 0;
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_Action(
        /* [in] */ NET_FW_ACTION action) = 0;
};

MIDL_INTERFACE("9C4C6277-5027-441E-AFAE-CA1F542DA009")
INetFwRules : public IDispatch
{
public:
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Count(
        /* [retval][out] */ long *count) = 0;
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE Add(
        /* [in] */ INetFwRule *rule) = 0;
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE Remove(
        /* [in] */ BSTR name) = 0;
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE Item(
        /* [in] */ BSTR name,
        /* [retval][out] */ INetFwRule **rule) = 0;
    virtual /* [restricted][propget][id] */ HRESULT STDMETHODCALLTYPE get__NewEnum(
        /* [retval][out] */ IUnknown **newEnum) = 0;
};

typedef enum NET_FW_IP_PROTOCOL_
{
    NET_FW_IP_PROTOCOL_TCP    = 6,
    NET_FW_IP_PROTOCOL_UDP    = 17,
    NET_FW_IP_PROTOCOL_ANY    = 256
} NET_FW_IP_PROTOCOL;


typedef interface INetFwServiceRestriction INetFwServiceRestriction;
MIDL_INTERFACE("8267BBE3-F890-491C-B7B6-2DB1EF0E5D2B")
INetFwServiceRestriction : public IDispatch
{
public:
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE RestrictService(
        /* [in] */ BSTR serviceName,
        /* [in] */ BSTR appName,
        /* [in] */ VARIANT_BOOL restrictService,
        /* [in] */ VARIANT_BOOL serviceSidRestricted) = 0;
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE ServiceRestricted(
        /* [in] */ BSTR serviceName,
        /* [in] */ BSTR appName,
        /* [retval][out] */ VARIANT_BOOL *serviceRestricted) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Rules(
        /* [retval][out] */ INetFwRules **rules) = 0;
};

typedef
enum NET_FW_MODIFY_STATE_
{
    NET_FW_MODIFY_STATE_OK	= 0,
    NET_FW_MODIFY_STATE_GP_OVERRIDE	= ( NET_FW_MODIFY_STATE_OK + 1 ) ,
    NET_FW_MODIFY_STATE_INBOUND_BLOCKED	= ( NET_FW_MODIFY_STATE_GP_OVERRIDE + 1 )
} NET_FW_MODIFY_STATE;

typedef interface INetFwPolicy2 INetFwPolicy2;

MIDL_INTERFACE("98325047-C671-4174-8D81-DEFCD3F03186")
INetFwPolicy2 : public IDispatch
{
public:
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_CurrentProfileTypes(
        /* [retval][out] */ LONG *profileTypesBitmask) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_FirewallEnabled(
        /* [in] */ NET_FW_PROFILE_TYPE2 profileType,
        /* [retval][out] */ VARIANT_BOOL *enabled) = 0;
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_FirewallEnabled(
        /* [in] */ NET_FW_PROFILE_TYPE2 profileType,
        /* [in] */ VARIANT_BOOL enabled) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ExcludedInterfaces(
        /* [in] */ NET_FW_PROFILE_TYPE2 profileType,
        /* [retval][out] */ VARIANT *interfaces) = 0;
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_ExcludedInterfaces(
        /* [in] */ NET_FW_PROFILE_TYPE2 profileType,
        /* [in] */ VARIANT interfaces) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_BlockAllInboundTraffic(
        /* [in] */ NET_FW_PROFILE_TYPE2 profileType,
        /* [retval][out] */ VARIANT_BOOL *Block) = 0;
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_BlockAllInboundTraffic(
        /* [in] */ NET_FW_PROFILE_TYPE2 profileType,
        /* [in] */ VARIANT_BOOL Block) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_NotificationsDisabled(
        /* [in] */ NET_FW_PROFILE_TYPE2 profileType,
        /* [retval][out] */ VARIANT_BOOL *disabled) = 0;
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_NotificationsDisabled(
        /* [in] */ NET_FW_PROFILE_TYPE2 profileType,
        /* [in] */ VARIANT_BOOL disabled) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_UnicastResponsesToMulticastBroadcastDisabled(
        /* [in] */ NET_FW_PROFILE_TYPE2 profileType,
        /* [retval][out] */ VARIANT_BOOL *disabled) = 0;
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_UnicastResponsesToMulticastBroadcastDisabled(
        /* [in] */ NET_FW_PROFILE_TYPE2 profileType,
        /* [in] */ VARIANT_BOOL disabled) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_Rules(
        /* [retval][out] */ INetFwRules **rules) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_ServiceRestriction(
        /* [retval][out] */ INetFwServiceRestriction **ServiceRestriction) = 0;
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE EnableRuleGroup(
        /* [in] */ long profileTypesBitmask,
        /* [in] */ BSTR group,
        /* [in] */ VARIANT_BOOL enable) = 0;
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE IsRuleGroupEnabled(
        /* [in] */ long profileTypesBitmask,
        /* [in] */ BSTR group,
        /* [retval][out] */ VARIANT_BOOL *enabled) = 0;
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE RestoreLocalFirewallDefaults( void) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_DefaultInboundAction(
        /* [in] */ NET_FW_PROFILE_TYPE2 profileType,
        /* [retval][out] */ NET_FW_ACTION *action) = 0;
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_DefaultInboundAction(
        /* [in] */ NET_FW_PROFILE_TYPE2 profileType,
        /* [in] */ NET_FW_ACTION action) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_DefaultOutboundAction(
        /* [in] */ NET_FW_PROFILE_TYPE2 profileType,
        /* [retval][out] */ NET_FW_ACTION *action) = 0;
    virtual /* [propput][id] */ HRESULT STDMETHODCALLTYPE put_DefaultOutboundAction(
        /* [in] */ NET_FW_PROFILE_TYPE2 profileType,
        /* [in] */ NET_FW_ACTION action) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_IsRuleGroupCurrentlyEnabled(
        /* [in] */ BSTR group,
        /* [retval][out] */ VARIANT_BOOL *enabled) = 0;
    virtual /* [propget][id] */ HRESULT STDMETHODCALLTYPE get_LocalPolicyModifyState(
        /* [retval][out] */ NET_FW_MODIFY_STATE *modifyState) = 0;
};

class DECLSPEC_UUID("2C5BC43E-3369-4C33-AB0C-BE9469677AF4")
NetFwRule;

class DECLSPEC_UUID("E2B3C97F-6AE1-41AC-817A-F6F92166D7DD")
NetFwPolicy2;


#define WINSTA_ENUMDESKTOPS         0x0001L
#define WINSTA_READATTRIBUTES       0x0002L
#define WINSTA_ACCESSCLIPBOARD      0x0004L
#define WINSTA_CREATEDESKTOP        0x0008L
#define WINSTA_WRITEATTRIBUTES      0x0010L
#define WINSTA_ACCESSGLOBALATOMS    0x0020L
#define WINSTA_EXITWINDOWS          0x0040L
#define WINSTA_ENUMERATE            0x0100L
#define WINSTA_READSCREEN           0x0200L

#define DESKTOP_READOBJECTS         0x0001L
#define DESKTOP_CREATEWINDOW        0x0002L
#define DESKTOP_CREATEMENU          0x0004L
#define DESKTOP_HOOKCONTROL         0x0008L
#define DESKTOP_JOURNALRECORD       0x0010L
#define DESKTOP_JOURNALPLAYBACK     0x0020L
#define DESKTOP_ENUMERATE           0x0040L
#define DESKTOP_WRITEOBJECTS        0x0080L
#define DESKTOP_SWITCHDESKTOP       0x0100L

#define MAXIMUM_ALLOWED                  (0x02000000L)

#define UOI_FLAGS       1
#define UOI_NAME        2
#define UOI_TYPE        3
#define UOI_USER_SID    4
#define UOI_HEAPSIZE    5
#define UOI_IO          6

#define PIPE_ACCESS_INBOUND         0x00000001
#define PIPE_ACCESS_OUTBOUND        0x00000002
#define PIPE_ACCESS_DUPLEX          0x00000003

#define PIPE_WAIT                   0x00000000
#define PIPE_NOWAIT                 0x00000001
#define PIPE_READMODE_BYTE          0x00000000
#define PIPE_READMODE_MESSAGE       0x00000002
#define PIPE_TYPE_BYTE              0x00000000
#define PIPE_TYPE_MESSAGE           0x00000004
#define PIPE_ACCEPT_REMOTE_CLIENTS  0x00000000
#define PIPE_REJECT_REMOTE_CLIENTS  0x00000008

#define TerminateJobObject TerminateJobObjectMw
BOOL WINAPI TerminateJobObjectMw(
    __in  HANDLE hJob,
    __in  UINT uExitCode
    );

#define LOGON_WITH_PROFILE              0x00000001
#define LOGON_NETCREDENTIALS_ONLY       0x00000002
#define LOGON_ZERO_PASSWORD_BUFFER      0x80000000

#define JOB_OBJECT_LIMIT_PROCESS_MEMORY             0x00000100
#define JOB_OBJECT_LIMIT_JOB_MEMORY                 0x00000200
#define JOB_OBJECT_LIMIT_JOB_MEMORY_HIGH            JOB_OBJECT_LIMIT_JOB_MEMORY
#define JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION 0x00000400
#define JOB_OBJECT_LIMIT_BREAKAWAY_OK               0x00000800
#define JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK        0x00001000
#define JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE          0x00002000
#define JOB_OBJECT_LIMIT_SUBSET_AFFINITY            0x00004000
#define JOB_OBJECT_LIMIT_JOB_MEMORY_LOW             0x00008000

typedef STARTUPINFOW STARTUPINFO;
typedef LPSTARTUPINFOW LPSTARTUPINFO;

#define SW_HIDE             0
#define SW_SHOWNORMAL       1
#define SW_NORMAL           1
#define SW_SHOWMINIMIZED    2
#define SW_SHOWMAXIMIZED    3
#define SW_MAXIMIZE         3
#define SW_SHOWNOACTIVATE   4
#define SW_SHOW             5
#define SW_MINIMIZE         6
#define SW_SHOWMINNOACTIVE  7
#define SW_SHOWNA           8
#define SW_RESTORE          9
#define SW_SHOWDEFAULT      10
#define SW_FORCEMINIMIZE    11
#define SW_MAX              11

#define STARTF_USESHOWWINDOW       0x00000001
#define STARTF_USESIZE             0x00000002
#define STARTF_USEPOSITION         0x00000004
#define STARTF_USECOUNTCHARS       0x00000008
#define STARTF_USEFILLATTRIBUTE    0x00000010
#define STARTF_RUNFULLSCREEN       0x00000020  // ignored for non-x86 platforms
#define STARTF_FORCEONFEEDBACK     0x00000040
#define STARTF_FORCEOFFFEEDBACK    0x00000080
#define STARTF_USESTDHANDLES       0x00000100

#define SE_ASSIGNPRIMARYTOKEN_NAME        L"SeAssignPrimaryTokenPrivilege"

////////////////////////////////////////////////////////////////

typedef struct _LOCALGROUP_INFO_1 {
    LPWSTR   lgrpi1_name;
    LPWSTR   lgrpi1_comment;
}LOCALGROUP_INFO_1, *PLOCALGROUP_INFO_1, *LPLOCALGROUP_INFO_1;

typedef struct _LOCALGROUP_MEMBERS_INFO_3 {
     LPWSTR       lgrmi3_domainandname;
} LOCALGROUP_MEMBERS_INFO_3, *PLOCALGROUP_MEMBERS_INFO_3,
  *LPLOCALGROUP_MEMBERS_INFO_3;

typedef struct _LOCALGROUP_USERS_INFO_0 {
     LPWSTR  lgrui0_name;
} LOCALGROUP_USERS_INFO_0, *PLOCALGROUP_USERS_INFO_0,
  *LPLOCALGROUP_USERS_INFO_0;

typedef struct _LOCALGROUP_MEMBERS_INFO_1 {
     PSID         lgrmi1_sid;
     SID_NAME_USE lgrmi1_sidusage;
     LPWSTR       lgrmi1_name;
} LOCALGROUP_MEMBERS_INFO_1, *PLOCALGROUP_MEMBERS_INFO_1,
  *LPLOCALGROUP_MEMBERS_INFO_1;

typedef struct _LOCALGROUP_MEMBERS_INFO_0 {
	PSID    lgrmi0_sid;
} LOCALGROUP_MEMBERS_INFO_0, *PLOCALGROUP_MEMBERS_INFO_0, *LPLOCALGROUP_MEMBERS_INFO_0;

typedef struct _USER_INFO_1007 {
     LPWSTR  usri1007_comment;
} USER_INFO_1007, *PUSER_INFO_1007, *LPUSER_INFO_1007;

typedef struct _LOCALGROUP_INFO_1002 {
     LPWSTR  lgrpi1002_comment;
}LOCALGROUP_INFO_1002, *PLOCALGROUP_INFO_1002, *LPLOCALGROUP_INFO_1002;

#define LG_INCLUDE_INDIRECT         (0x0001)
#define MAX_PREFERRED_LENGTH    ((DWORD) -1)
#define NET_API_STATUS          DWORD

NET_API_STATUS NetLocalGroupSetMembers (
    IN LPCWSTR servername OPTIONAL,
    IN LPCWSTR groupname,
    IN DWORD level,
    IN LPBYTE buf,
    IN DWORD totalentries
);

NET_API_STATUS NetLocalGroupAddMembers (
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR LocalGroupName,
    IN DWORD Level,
    IN LPBYTE Buffer,
    IN DWORD NewMemberCount
    );

typedef struct _USER_INFO_1 {
    LPWSTR   usri1_name;
    LPWSTR   usri1_password;
    DWORD    usri1_password_age;
    DWORD    usri1_priv;
    LPWSTR   usri1_home_dir;
    LPWSTR   usri1_comment;
    DWORD    usri1_flags;
    LPWSTR   usri1_script_path;
}USER_INFO_1, *PUSER_INFO_1, *LPUSER_INFO_1;

typedef struct _USER_INFO_4 {
    LPWSTR   usri4_name;
    LPWSTR   usri4_password;
    DWORD    usri4_password_age;
    DWORD    usri4_priv;
    LPWSTR   usri4_home_dir;
    LPWSTR   usri4_comment;
    DWORD    usri4_flags;
    LPWSTR   usri4_script_path;
    DWORD    usri4_auth_flags;
    LPWSTR   usri4_full_name;
    LPWSTR   usri4_usr_comment;
    LPWSTR   usri4_parms;
    LPWSTR   usri4_workstations;
    DWORD    usri4_last_logon;
    DWORD    usri4_last_logoff;
    DWORD    usri4_acct_expires;
    DWORD    usri4_max_storage;
    DWORD    usri4_units_per_week;
    PBYTE    usri4_logon_hours;
    DWORD    usri4_bad_pw_count;
    DWORD    usri4_num_logons;
    LPWSTR   usri4_logon_server;
    DWORD    usri4_country_code;
    DWORD    usri4_code_page;
    PSID     usri4_user_sid;
    DWORD    usri4_primary_group_id;
    LPWSTR   usri4_profile;
    LPWSTR   usri4_home_dir_drive;
    DWORD    usri4_password_expired;
}USER_INFO_4, *PUSER_INFO_4, *LPUSER_INFO_4;

#define NERR_Success    0       /* Success */
#define NERR_BASE       2100

#define NERR_ACFNotFound        (NERR_BASE+119) /* The security database could not be found. */
#define NERR_GroupNotFound      (NERR_BASE+120) /* The group name could not be found. */
#define NERR_UserNotFound       (NERR_BASE+121) /* The user name could not be found. */
#define NERR_ResourceNotFound   (NERR_BASE+122) /* The resource name could not be found.  */
#define NERR_GroupExists        (NERR_BASE+123) /* The group already exists. */
#define NERR_UserExists         (NERR_BASE+124) /* The account already exists. */
#define NERR_ResourceExists     (NERR_BASE+125) /* The resource permission list already exists. */
#define NERR_NotPrimary         (NERR_BASE+126) /* This operation is only allowed on the primary domain controller of the domain. */
#define NERR_ACFNotLoaded       (NERR_BASE+127) /* The security database has not been started. */
#define NERR_ACFNoRoom          (NERR_BASE+128) /* There are too many names in the user accounts database. */
#define NERR_ACFFileIOFail      (NERR_BASE+129) /* A disk I/O failure occurred.*/
#define NERR_ACFTooManyLists    (NERR_BASE+130) /* The limit of 64 entries per resource was exceeded. */
#define NERR_UserLogon          (NERR_BASE+131) /* Deleting a user with a session is not allowed. */
#define NERR_ACFNoParent        (NERR_BASE+132) /* The parent directory could not be located. */
#define NERR_CanNotGrowSegment  (NERR_BASE+133) /* Unable to add to the security database session cache segment. */
#define NERR_SpeGroupOp         (NERR_BASE+134) /* This operation is not allowed on this special group. */
#define NERR_NotInCache         (NERR_BASE+135) /* This user is not cached in user accounts database session cache. */
#define NERR_UserInGroup        (NERR_BASE+136) /* The user already belongs to this group. */
#define NERR_UserNotInGroup     (NERR_BASE+137) /* The user does not belong to this group. */
#define NERR_AccountUndefined   (NERR_BASE+138) /* This user account is undefined. */
#define NERR_AccountExpired     (NERR_BASE+139) /* This user account has expired. */
#define NERR_InvalidWorkstation (NERR_BASE+140) /* The user is not allowed to log on from this workstation. */
#define NERR_InvalidLogonHours  (NERR_BASE+141) /* The user is not allowed to log on at this time.  */
#define NERR_PasswordExpired    (NERR_BASE+142) /* The password of this user has expired. */
#define NERR_PasswordCantChange (NERR_BASE+143) /* The password of this user cannot change. */
#define NERR_PasswordHistConflict (NERR_BASE+144) /* This password cannot be used now. */
#define NERR_PasswordTooShort   (NERR_BASE+145) /* The password does not meet the password policy requirements. Check the minimum password length, password complexity and password history requirements. */
#define NERR_PasswordTooRecent  (NERR_BASE+146) /* The password of this user is too recent to change.  */
#define NERR_InvalidDatabase    (NERR_BASE+147) /* The security database is corrupted. */
#define NERR_DatabaseUpToDate   (NERR_BASE+148) /* No updates are necessary to this replicant network/local security database. */
#define NERR_SyncRequired       (NERR_BASE+149) /* This replicant database is outdated; synchronization is required. */

#define ERROR_MEMBER_IN_ALIAS            1378L
#define ERROR_ALIAS_EXISTS               1379L
#define ERROR_NO_SUCH_ALIAS              1376L

#define UF_TEMP_DUPLICATE_ACCOUNT       0x0100
#define UF_NORMAL_ACCOUNT               0x0200
#define UF_INTERDOMAIN_TRUST_ACCOUNT    0x0800
#define UF_WORKSTATION_TRUST_ACCOUNT    0x1000
#define UF_SERVER_TRUST_ACCOUNT         0x2000

#define UF_SCRIPT                          0x0001
#define UF_ACCOUNTDISABLE                  0x0002
#define UF_HOMEDIR_REQUIRED                0x0008
#define UF_LOCKOUT                         0x0010
#define UF_PASSWD_NOTREQD                  0x0020
#define UF_PASSWD_CANT_CHANGE              0x0040
#define UF_ENCRYPTED_TEXT_PASSWORD_ALLOWED 0x0080

#define UF_DONT_EXPIRE_PASSWD                         0x10000
#define UF_MNS_LOGON_ACCOUNT                          0x20000
#define UF_SMARTCARD_REQUIRED                         0x40000
#define UF_TRUSTED_FOR_DELEGATION                     0x80000
#define UF_NOT_DELEGATED                             0x100000
#define UF_USE_DES_KEY_ONLY                          0x200000
#define UF_DONT_REQUIRE_PREAUTH                      0x400000
#define UF_PASSWORD_EXPIRED                          0x800000
#define UF_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION   0x1000000
#define UF_NO_AUTH_DATA_REQUIRED                    0x2000000
#define UF_PARTIAL_SECRETS_ACCOUNT                  0x4000000

#define TIMEQ_FOREVER               ((unsigned long) -1L)

#define DOMAIN_GROUP_RID_ADMINS        (0x00000200L)
#define DOMAIN_GROUP_RID_USERS         (0x00000201L)
#define DOMAIN_GROUP_RID_GUESTS        (0x00000202L)
#define DOMAIN_GROUP_RID_COMPUTERS     (0x00000203L)
#define DOMAIN_GROUP_RID_CONTROLLERS   (0x00000204L)
#define DOMAIN_GROUP_RID_CERT_ADMINS   (0x00000205L)
#define DOMAIN_GROUP_RID_SCHEMA_ADMINS (0x00000206L)
#define DOMAIN_GROUP_RID_ENTERPRISE_ADMINS (0x00000207L)
#define DOMAIN_GROUP_RID_POLICY_ADMINS (0x00000208L)
#define DOMAIN_GROUP_RID_READONLY_CONTROLLERS (0x00000209L)
#define DOMAIN_GROUP_RID_CLONEABLE_CONTROLLERS (0x0000020AL)
#define DOMAIN_GROUP_RID_CDC_RESERVED    (0x0000020CL)
#define DOMAIN_GROUP_RID_PROTECTED_USERS (0x0000020DL)
#define DOMAIN_GROUP_RID_KEY_ADMINS      (0x0000020EL)
#define DOMAIN_GROUP_RID_ENTERPRISE_KEY_ADMINS (0x0000020FL)

#define SERVICE_ACCOUNT_FLAG_LINK_TO_HOST_ONLY    0x00000001L

BOOL WINAPI DeleteProfileW (
    _In_        LPCWSTR    lpSidString,
    _In_opt_    LPCWSTR    lpProfilePath,
    _In_opt_    LPCWSTR    lpComputerName);
#define DeleteProfile  DeleteProfileW

NET_API_STATUS NetUserAdd(
    IN LPCWSTR ServerName OPTIONAL,
    IN DWORD Level,
    IN LPBYTE Buffer,
    OUT LPDWORD ParmError OPTIONAL // Name required by NetpSetParmError
    );
NET_API_STATUS NetUserDel(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR UserName
    );

NET_API_STATUS NetLocalGroupDel(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR LocalGroupName
    );

NET_API_STATUS NetLocalGroupGetInfo(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR LocalGroupName,
    IN DWORD Level,
    OUT LPBYTE *Buffer
    );

NET_API_STATUS NetUserGetInfo(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR UserName,
    IN DWORD Level,
    OUT LPBYTE *Buffer
    );

#define SERVICE_ACCOUNT_PASSWORD L"_SA_{262E99C9-6160-4871-ACEC-4E61736B6F21}"

NET_API_STATUS NetLocalGroupAdd(
    IN LPCWSTR ServerName OPTIONAL,
    IN DWORD Level,
    IN LPBYTE Buffer,
    OUT LPDWORD ParmError OPTIONAL // Name required by NetpSetParmError
    );

NET_API_STATUS NetLocalGroupGetMembers(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR LocalGroupName,
    IN DWORD Level,
    OUT LPBYTE *Buffer,
    IN DWORD PrefMaxLen,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD EntriesLeft,
    IN OUT PDWORD_PTR ResumeHandle
    );

NET_API_STATUS NetUserGetLocalGroups(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR UserName,
    IN DWORD Level,
    IN DWORD Flags,
    OUT LPBYTE *Buffer,
    IN DWORD PrefMaxLen,
    OUT LPDWORD EntriesRead,
    OUT LPDWORD EntriesLeft
    );

NET_API_STATUS NetUserSetInfo(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR UserName,
    IN DWORD Level,
    IN LPBYTE Buffer,
    OUT LPDWORD ParmError OPTIONAL  // Name required by NetpSetParmError
    );

NET_API_STATUS NetLocalGroupSetInfo(
    IN LPCWSTR ServerName OPTIONAL,
    IN LPCWSTR LocalGroupName,
    IN DWORD Level,
    IN LPBYTE Buffer,
    OUT LPDWORD ParmError OPTIONAL // Name required by NetpSetParmError
    );

NET_API_STATUS NetApiBufferFree(void *a_bufptr);

//////////////////////////////////////////////////////////////////
// interlock functions
//////////////////////////////////////////////////////////////////
inline CHAR InterlockedExchange8(CHAR volatile *Target, CHAR Value)
{
    return __sync_swap(Target, Value);
}

inline SHORT InterlockedCompareExchange16(SHORT volatile *Destination, SHORT Exchange, SHORT Comperand)
{
    return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

// LONG
inline LONG InterlockedXor(LONG volatile *Destination,LONG Value)
{
    return __sync_fetch_and_xor(Destination, Value);
}

// ULONG
inline ULONG InterlockedIncrement(ULONG volatile *lpAddend)
{
    return __sync_add_and_fetch(lpAddend, (ULONG)1);
}

inline ULONG InterlockedDecrement(ULONG volatile *lpAddend)
{
    return __sync_sub_and_fetch(lpAddend, (ULONG)1);
}

inline ULONG InterlockedCompareExchange(ULONG volatile *Destination, ULONG Exchange, ULONG Comperand)
{
    return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

// LONGLONG
inline LONGLONG InterlockedIncrement(LONGLONG volatile *lpAddend)
{
    return __sync_add_and_fetch(lpAddend, (LONGLONG)1);
}

inline LONGLONG InterlockedDecrement(LONGLONG volatile *lpAddend)
{
    return __sync_sub_and_fetch(lpAddend, (LONGLONG)1);
}

inline LONGLONG InterlockedAdd64(LONGLONG volatile *lpAddend, LONGLONG addent)
{
   return __sync_add_and_fetch(lpAddend, (LONGLONG)addent);
}

// ULONGLONG
inline ULONGLONG InterlockedIncrement(ULONGLONG volatile *lpAddend)
{
    return __sync_add_and_fetch(lpAddend, (ULONGLONG)1);
}

inline ULONGLONG InterlockedDecrement(ULONGLONG volatile *lpAddend)
{
    return __sync_sub_and_fetch(lpAddend, (ULONGLONG)1);
}

inline ULONGLONG InterlockedExchange(ULONGLONG volatile *Target, ULONGLONG Value)
{
    return __sync_swap(Target, Value);
}

inline ULONGLONG InterlockedCompareExchange(ULONGLONG volatile *Destination, ULONGLONG Exchange, ULONGLONG Comperand)
{
    return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

inline ULONGLONG InterlockedCompareExchangeAcquire(ULONGLONG volatile *Destination, ULONGLONG Exchange, ULONGLONG Comperand)
{
    return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

inline ULONGLONG InterlockedCompareExchangeRelease(ULONGLONG volatile *Destination, ULONGLONG Exchange, ULONGLONG Comperand)
{
    return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

inline ULONGLONG InterlockedExchangeAdd(ULONGLONG volatile *Addend, ULONGLONG Value)
{
    return __sync_fetch_and_add(Addend, Value);
}

inline ULONGLONG InterlockedExchangeSubtract(ULONGLONG volatile *Addend, ULONGLONG Value)
{
    return __sync_fetch_and_sub(Addend, Value);
}

inline ULONGLONG InterlockedAnd(ULONGLONG volatile *Destination, ULONGLONG Value)
{
    return __sync_fetch_and_and(Destination, Value);
}

inline ULONGLONG InterlockedOr(ULONGLONG volatile *Destination, ULONGLONG Value)
{
    return __sync_fetch_and_or(Destination, Value);
}

inline ULONGLONG InterlockedXor(ULONGLONG volatile *Destination, ULONGLONG Value)
{
    return __sync_fetch_and_xor(Destination, Value);
}

#if 0

PALIMPORT
ULONGLONG
PALAPI
InterlockedIncrement(
             IN OUT ULONGLONG volatile *lpAddend);

PALIMPORT
ULONG
PALAPI
InterlockedIncrement(
        IN OUT ULONG volatile *lpAddend);

PALIMPORT
ULONGLONG
PALAPI
InterlockedDecrement(
             IN OUT ULONGLONG volatile *lpAddend);

PALIMPORT
ULONG
PALAPI
InterlockedDecrement(
        IN OUT ULONG volatile *lpAddend);

PALIMPORT
ULONGLONG
PALAPI
InterlockedExchange(
            IN OUT ULONGLONG volatile *Target,
            IN ULONGLONG Value);

PALIMPORT
CHAR
PALAPI
InterlockedExchange8(
        IN OUT CHAR volatile *Target,
        IN CHAR Value);

PALIMPORT
SHORT
PALAPI
InterlockedExchange16(
        IN OUT SHORT volatile *Target,
        IN SHORT Value);

PALIMPORT
ULONGLONG
PALAPI
InterlockedCompareExchange(
               IN OUT ULONGLONG volatile *Destination,
               IN ULONGLONG Exchange,
               IN ULONGLONG Comperand);

PALIMPORT
LONG
PALAPI
InterlockedCompareExchange(
               IN OUT LONG volatile *Destination, 
               IN LONG Exchange, 
               IN LONG Comperand);

PALIMPORT
ULONG
PALAPI
InterlockedCompareExchange(
               IN OUT ULONG volatile *Destination, 
               IN ULONG Exchange, 
               IN ULONG Comperand);

PALIMPORT
SHORT
PALAPI InterlockedCompareExchange16(
               IN OUT SHORT volatile *Destination, 
               IN SHORT Exchange, 
               IN SHORT Comperand);

PALIMPORT
ULONGLONG
PALAPI
InterlockedCompareExchangeAcquire(
               IN OUT ULONGLONG volatile *Destination,
               IN ULONGLONG Exchange,
               IN ULONGLONG Comperand);

PALIMPORT
ULONGLONG
PALAPI
InterlockedCompareExchangeRelease(
               IN OUT ULONGLONG volatile *Destination,
               IN ULONGLONG Exchange,
               IN ULONGLONG Comperand);

PALIMPORT
ULONGLONG
PALAPI
InterlockedExchangeAdd(
               IN OUT ULONGLONG volatile *Addend,
               IN ULONGLONG Value);

PALIMPORT
ULONGLONG
PALAPI
InterlockedExchangeSubtract(
               IN OUT ULONGLONG volatile *Addend,
               IN ULONGLONG Value);

PALIMPORT
ULONGLONG
PALAPI
InterlockedAnd(
               IN OUT ULONGLONG volatile *Destination,
               IN ULONGLONG Value);

PALIMPORT
ULONGLONG
PALAPI
InterlockedOr(
               IN OUT ULONGLONG volatile *Destination,
               IN ULONGLONG Value);

PALIMPORT
LONG
PALAPI
InterlockedXor(
               IN OUT LONG volatile *Destination,
               IN LONG Value);

PALIMPORT
ULONGLONG
PALAPI
InterlockedXor(
               IN OUT ULONGLONG volatile *Destination,
               IN ULONGLONG Value);
/*
PALIMPORT
LONGLONG
PALAPI
InterlockedAdd64(
               IN OUT LONGLONG volatile *lpAddend,
               IN LONGLONG addent);
*/
inline LONGLONG InterlockedAdd64(LONGLONG volatile *lpAddend, LONGLONG addent)
{
    return __sync_add_and_fetch(lpAddend, (LONGLONG)addent);
}
#endif

#if !defined(CLANG_5_0_1_PLUS)
inline LONG _InterlockedIncrement(LONG volatile *lpAddend)
{
    return __sync_add_and_fetch(lpAddend, (LONG)1);
}
#endif

#if !defined(CLANG_5_0_1_PLUS)
inline LONG _InterlockedDecrement(LONG volatile *lpAddend)
{
	return __sync_sub_and_fetch(lpAddend, (LONG)1);
}
#endif

inline ULONGLONG _InterlockedIncrement(ULONGLONG volatile *lpAddend)
{
    return __sync_add_and_fetch(lpAddend, (ULONGLONG)1);
}

inline ULONGLONG _InterlockedDecrement(ULONGLONG volatile *lpAddend)
{
    return __sync_sub_and_fetch(lpAddend, (ULONGLONG)1);
}

typedef USHORT SECURITY_DESCRIPTOR_CONTROL, *PSECURITY_DESCRIPTOR_CONTROL;

typedef struct _SECURITY_DESCRIPTOR {
    UCHAR Revision;
    UCHAR Sbz1;
    SECURITY_DESCRIPTOR_CONTROL Control;
    PSID Owner;
    PSID Group;
    PACL Sacl;
    PACL Dacl;

} SECURITY_DESCRIPTOR, *PISECURITY_DESCRIPTOR;

// This is also defined in pal.h, but is hidden behind __APPLE__ define. Need
// to find out why.
WINBASEAPI
HANDLE
WINAPI
HeapCreate(
	       IN DWORD flOptions,
	       IN SIZE_T dwInitialSize,
	       IN SIZE_T dwMaximumSize);

WINBASEAPI
BOOL
WINAPI
HeapValidate(
        _In_ HANDLE hHEAP,
        _In_ DWORD dwFlags,
        _In_opt_ LPCVOID lpMem
        );

FORCEINLINE
CHAR
ReadNoFence8 (
    _In_ CHAR const volatile *Source
    )

{

    CHAR Value;

    Value = *Source;
    return Value;
}

FORCEINLINE
LONG
ReadNoFence (
    _In_ LONG const volatile *Source
    )
{
    LONG Value;

    Value = *Source;
    return Value;
}

FORCEINLINE
ULONG
ReadULongNoFence (
    _In_ ULONG const volatile *Source
    )

{
    return (ULONG)ReadNoFence((PLONG)Source);
}

FORCEINLINE
UCHAR
ReadUCharNoFence (
    _In_ UCHAR const volatile *Source
    )

{
    return (UCHAR)ReadNoFence8((PCHAR)Source);
}



///////////////////////////////////////////////////////////////////////////////
// Fabric.exe
///////////////////////////////////////////////////////////////////////////////
#define ERROR_UNHANDLED_EXCEPTION        574L
#define ERROR_CLUSTER_INCOMPATIBLE_VERSIONS 5075L
#define LOWORD(_dw)     ((WORD)(((DWORD_PTR)(_dw)) & 0xffff))
#define HIWORD(_dw)     ((WORD)((((DWORD_PTR)(_dw)) >> 16) & 0xffff))
#define LODWORD(_qw)    ((DWORD)(_qw))
#define HIDWORD(_qw)    ((DWORD)(((_qw) >> 32) & 0xffffffff))

//
// Re-implementations of various PAL methods
//

// memcpy_s exists as part of the c11 standard on compliant implementations
// This method should thus be removed once such an implementation is used
// Currently the required method is not present within <string.h> as used by SF
errno_t memcpy_s(void *dest, size_t destsz, const void *src, size_t count);

// Utility function used by Common::File and Common::Directory
extern FILETIME UnixTimeToFILETIME(time_t sec, long nsec);
extern bool GlobMatch(const char *str, const char *pat);

// Utility function used by ProcessUtility.cpp
int ForkExec(const char* appName, const char* cmdline, const char* workdir, const char* environment, int *fdin, int* fdout);

typedef enum _SECTION_INHERIT {
    ViewShared = 1,
    ViewUnmap  = 2
} SECTION_INHERIT;


#define LONG64_MIN      (-9223372036854775807i64 - 1)
#define MAXULONG64      ((ULONG64)~((ULONG64)0))
#define MAXLONG64       ((LONG64)(MAXULONG64 >> 1))
#define MINLONG64       ((LONG64)~MAXLONG64)
#define MAXULONG32  ((ULONG32)~((ULONG32)0))
#define MAXLONG32   ((LONG32)(MAXULONG32 >> 1))
#define MINLONG32   ((LONG32)~MAXLONG32)

#define CONVERT_TO_ARGS(argc, cargs) \
    std::vector<WCHAR*> args_vec(argc);\
    WCHAR** args = (WCHAR**)args_vec.data();\
    std::vector<std::wstring> wargs(argc);\
    for (int iter = 0; iter < argc; iter++)\
    {\
        wargs[iter] = Utf8To16(cargs[iter]);\
        args[iter] = (WCHAR*)(wargs[iter].data());\
    }\

typedef struct _RTL_SPLAY_LINKS {
    struct _RTL_SPLAY_LINKS *Parent;
    struct _RTL_SPLAY_LINKS *LeftChild;
    struct _RTL_SPLAY_LINKS *RightChild;
} RTL_SPLAY_LINKS;
typedef RTL_SPLAY_LINKS *PRTL_SPLAY_LINKS;

typedef
_IRQL_requires_same_
_Function_class_(RTL_GENERIC_COMPARE_ROUTINE)
RTL_GENERIC_COMPARE_RESULTS
NTAPI
RTL_GENERIC_COMPARE_ROUTINE (
    _In_ struct _RTL_GENERIC_TABLE *Table,
    _In_ PVOID FirstStruct,
    _In_ PVOID SecondStruct
    );
typedef RTL_GENERIC_COMPARE_ROUTINE *PRTL_GENERIC_COMPARE_ROUTINE;

typedef
_IRQL_requires_same_
_Function_class_(RTL_GENERIC_ALLOCATE_ROUTINE)
__drv_allocatesMem(Mem)
PVOID
NTAPI
RTL_GENERIC_ALLOCATE_ROUTINE (
    _In_ struct _RTL_GENERIC_TABLE *Table,
    _In_ CLONG ByteSize
    );
typedef RTL_GENERIC_ALLOCATE_ROUTINE *PRTL_GENERIC_ALLOCATE_ROUTINE;

typedef
_IRQL_requires_same_
_Function_class_(RTL_GENERIC_FREE_ROUTINE)
VOID
NTAPI
RTL_GENERIC_FREE_ROUTINE (
    _In_ struct _RTL_GENERIC_TABLE *Table,
    _In_ __drv_freesMem(Mem) _Post_invalid_ PVOID Buffer
    );
typedef RTL_GENERIC_FREE_ROUTINE *PRTL_GENERIC_FREE_ROUTINE;

typedef
_IRQL_requires_same_
_Function_class_(RTL_AVL_MATCH_FUNCTION)
NTSTATUS
NTAPI
RTL_AVL_MATCH_FUNCTION (
    _In_ struct _RTL_AVL_TABLE *Table,
    _In_ PVOID UserData,
    _In_ PVOID MatchData
    );
typedef RTL_AVL_MATCH_FUNCTION *PRTL_AVL_MATCH_FUNCTION;

typedef struct _RTL_GENERIC_TABLE {
    PRTL_SPLAY_LINKS TableRoot;
    LIST_ENTRY InsertOrderList;
    PLIST_ENTRY OrderedPointer;
    ULONG WhichOrderedElement;
    ULONG NumberGenericTableElements;
    PRTL_GENERIC_COMPARE_ROUTINE CompareRoutine;
    PRTL_GENERIC_ALLOCATE_ROUTINE AllocateRoutine;
    PRTL_GENERIC_FREE_ROUTINE FreeRoutine;
    PVOID TableContext;
} RTL_GENERIC_TABLE;
typedef RTL_GENERIC_TABLE *PRTL_GENERIC_TABLE;

typedef enum _TABLE_SEARCH_RESULT{
    TableEmptyTree,
    TableFoundNode,
    TableInsertAsLeft,
    TableInsertAsRight
} TABLE_SEARCH_RESULT;

#define RtlLeftChild(Links) (           \
    (PRTL_SPLAY_LINKS)(Links)->LeftChild \
    )

#define RtlRightChild(Links) (           \
    (PRTL_SPLAY_LINKS)(Links)->RightChild \
    )

#define RtlIsRoot(Links) (                          \
    (RtlParent(Links) == (PRTL_SPLAY_LINKS)(Links)) \
    )
#define RtlIsLeftChild(Links) (                                   \
    (RtlLeftChild(RtlParent(Links)) == (PRTL_SPLAY_LINKS)(Links)) \
    )

#define RtlIsRightChild(Links) (                                   \
    (RtlRightChild(RtlParent(Links)) == (PRTL_SPLAY_LINKS)(Links)) \
    )

#define RtlParent(Links) (           \
    (PRTL_SPLAY_LINKS)(Links)->Parent \
    )

VOID RtlInitializeGenericTableAvl(
        _Out_    PRTL_AVL_TABLE            Table,
        _In_     PRTL_AVL_COMPARE_ROUTINE  CompareRoutine,
        _In_     PRTL_AVL_ALLOCATE_ROUTINE AllocateRoutine,
        _In_     PRTL_AVL_FREE_ROUTINE     FreeRoutine,
        _In_opt_ PVOID                     TableContext
);

PVOID RtlEnumerateGenericTableAvl(
        _In_ PRTL_AVL_TABLE Table,
        _In_ BOOLEAN        Restart
);

BOOLEAN RtlDeleteElementGenericTableAvl(
        _In_ PRTL_AVL_TABLE Table,
        _In_ PVOID          Buffer
);

PVOID RtlLookupElementGenericTableAvl(
        _In_ PRTL_AVL_TABLE Table,
        _In_ PVOID          Buffer
);

PVOID RtlInsertElementGenericTableAvl(
        _In_      PRTL_AVL_TABLE Table,
        _In_      PVOID          Buffer,
        _In_      CLONG          BufferSize,
        _Out_opt_ PBOOLEAN       NewElement
);

ULONG RtlNumberGenericTableElementsAvl(
        _In_ PRTL_AVL_TABLE Table
);

PVOID RtlEnumerateGenericTableLikeADirectory(
        _In_     PRTL_AVL_TABLE          Table,
        _In_opt_ PRTL_AVL_MATCH_FUNCTION MatchFunction,
        _In_opt_ PVOID                   MatchData,
        _In_     ULONG                   NextFlag,
        _Inout_  PVOID                   *RestartKey,
        _Inout_  PULONG                  DeleteCount,
        _In_     PVOID                   Buffer
);

PVOID RtlEnumerateGenericTableWithoutSplayingAvl(
        _In_    PRTL_AVL_TABLE     Table,
        _Inout_ PVOID              *RestartKey
);

BOOLEAN
RtlIsGenericTableEmptyAvl (
    __in PRTL_AVL_TABLE Table
    );


#define ERROR_SEVERITY_SUCCESS       0x00000000
#define ERROR_SEVERITY_INFORMATIONAL 0x40000000
#define ERROR_SEVERITY_WARNING       0x80000000
#define ERROR_SEVERITY_ERROR         0xC0000000

#define MAXUINT64   ((UINT64)~((UINT64)0))
#define MAXINT64    ((INT64)(MAXUINT64 >> 1))
#define MININT64    ((INT64)~MAXINT64)
#define MAXUINT     ((UINT)~((UINT)0))
#define MAXINT      ((INT)(MAXUINT >> 1))
#define MININT      ((INT)~MAXINT)

USHORT WINAPI CaptureStackBackTrace(
        _In_      ULONG  FramesToSkip,
        _In_      ULONG  FramesToCapture,
        _Out_     PVOID  *BackTrace,
        _Out_opt_ PULONG BackTraceHash
);

#define DPFLTR_ERROR_LEVEL 0
#define DPFLTR_WARNING_LEVEL 1
#define DPFLTR_TRACE_LEVEL 2
#define DPFLTR_INFO_LEVEL 3
#define DPFLTR_MASK 0x80000000

typedef enum _DPFLTR_TYPE {
    DPFLTR_SYSTEM_ID = 0,
    DPFLTR_SMSS_ID = 1,
    DPFLTR_SETUP_ID = 2,
    DPFLTR_NTFS_ID = 3,
    DPFLTR_FSTUB_ID = 4,
    DPFLTR_CRASHDUMP_ID = 5,
    DPFLTR_CDAUDIO_ID = 6,
    DPFLTR_CDROM_ID = 7,
    DPFLTR_CLASSPNP_ID = 8,
    DPFLTR_DISK_ID = 9,
    DPFLTR_REDBOOK_ID = 10,
    DPFLTR_STORPROP_ID = 11,
    DPFLTR_SCSIPORT_ID = 12,
    DPFLTR_SCSIMINIPORT_ID = 13,
    DPFLTR_CONFIG_ID = 14,
    DPFLTR_I8042PRT_ID = 15,
    DPFLTR_SERMOUSE_ID = 16,
    DPFLTR_LSERMOUS_ID = 17,
    DPFLTR_KBDHID_ID = 18,
    DPFLTR_MOUHID_ID = 19,
    DPFLTR_KBDCLASS_ID = 20,
    DPFLTR_MOUCLASS_ID = 21,
    DPFLTR_TWOTRACK_ID = 22,
    DPFLTR_WMILIB_ID = 23,
    DPFLTR_ACPI_ID = 24,
    DPFLTR_AMLI_ID = 25,
    DPFLTR_HALIA64_ID = 26,
    DPFLTR_VIDEO_ID = 27,
    DPFLTR_SVCHOST_ID = 28,
    DPFLTR_VIDEOPRT_ID = 29,
    DPFLTR_TCPIP_ID = 30,
    DPFLTR_DMSYNTH_ID = 31,
    DPFLTR_NTOSPNP_ID = 32,
    DPFLTR_FASTFAT_ID = 33,
    DPFLTR_SAMSS_ID = 34,
    DPFLTR_PNPMGR_ID = 35,
    DPFLTR_NETAPI_ID = 36,
    DPFLTR_SCSERVER_ID = 37,
    DPFLTR_SCCLIENT_ID = 38,
    DPFLTR_SERIAL_ID = 39,
    DPFLTR_SERENUM_ID = 40,
    DPFLTR_UHCD_ID = 41,
    DPFLTR_RPCPROXY_ID = 42,
    DPFLTR_AUTOCHK_ID = 43,
    DPFLTR_DCOMSS_ID = 44,
    DPFLTR_UNIMODEM_ID = 45,
    DPFLTR_SIS_ID = 46,
    DPFLTR_FLTMGR_ID = 47,
    DPFLTR_WMICORE_ID = 48,
    DPFLTR_BURNENG_ID = 49,
    DPFLTR_IMAPI_ID = 50,
    DPFLTR_SXS_ID = 51,
    DPFLTR_FUSION_ID = 52,
    DPFLTR_IDLETASK_ID = 53,
    DPFLTR_SOFTPCI_ID = 54,
    DPFLTR_TAPE_ID = 55,
    DPFLTR_MCHGR_ID = 56,
    DPFLTR_IDEP_ID = 57,
    DPFLTR_PCIIDE_ID = 58,
    DPFLTR_FLOPPY_ID = 59,
    DPFLTR_FDC_ID = 60,
    DPFLTR_TERMSRV_ID = 61,
    DPFLTR_W32TIME_ID = 62,
    DPFLTR_PREFETCHER_ID = 63,
    DPFLTR_RSFILTER_ID = 64,
    DPFLTR_FCPORT_ID = 65,
    DPFLTR_PCI_ID = 66,
    DPFLTR_DMIO_ID = 67,
    DPFLTR_DMCONFIG_ID = 68,
    DPFLTR_DMADMIN_ID = 69,
    DPFLTR_WSOCKTRANSPORT_ID = 70,
    DPFLTR_VSS_ID = 71,
    DPFLTR_PNPMEM_ID = 72,
    DPFLTR_PROCESSOR_ID = 73,
    DPFLTR_DMSERVER_ID = 74,
    DPFLTR_SR_ID = 75,
    DPFLTR_INFINIBAND_ID = 76,
    DPFLTR_IHVDRIVER_ID = 77,
    DPFLTR_IHVVIDEO_ID = 78,
    DPFLTR_IHVAUDIO_ID = 79,
    DPFLTR_IHVNETWORK_ID = 80,
    DPFLTR_IHVSTREAMING_ID = 81,
    DPFLTR_IHVBUS_ID = 82,
    DPFLTR_HPS_ID = 83,
    DPFLTR_RTLTHREADPOOL_ID = 84,
    DPFLTR_LDR_ID = 85,
    DPFLTR_TCPIP6_ID = 86,
    DPFLTR_ISAPNP_ID = 87,
    DPFLTR_SHPC_ID = 88,
    DPFLTR_STORPORT_ID = 89,
    DPFLTR_STORMINIPORT_ID = 90,
    DPFLTR_PRINTSPOOLER_ID = 91,
    DPFLTR_VSSDYNDISK_ID = 92,
    DPFLTR_VERIFIER_ID = 93,
    DPFLTR_VDS_ID = 94,
    DPFLTR_VDSBAS_ID = 95,
    DPFLTR_VDSDYN_ID = 96,
    DPFLTR_VDSDYNDR_ID = 97,
    DPFLTR_VDSLDR_ID = 98,
    DPFLTR_VDSUTIL_ID = 99,
    DPFLTR_DFRGIFC_ID = 100,
    DPFLTR_DEFAULT_ID = 101,
    DPFLTR_MM_ID = 102,
    DPFLTR_DFSC_ID = 103,
    DPFLTR_WOW64_ID = 104,
    DPFLTR_ALPC_ID = 105,
    DPFLTR_WDI_ID = 106,
    DPFLTR_PERFLIB_ID = 107,
    DPFLTR_KTM_ID = 108,
    DPFLTR_IOSTRESS_ID = 109,
    DPFLTR_HEAP_ID = 110,
    DPFLTR_WHEA_ID = 111,
    DPFLTR_USERGDI_ID = 112,
    DPFLTR_MMCSS_ID = 113,
    DPFLTR_TPM_ID = 114,
    DPFLTR_THREADORDER_ID = 115,
    DPFLTR_ENVIRON_ID = 116,
    DPFLTR_EMS_ID = 117,
    DPFLTR_WDT_ID = 118,
    DPFLTR_FVEVOL_ID = 119,
    DPFLTR_NDIS_ID = 120,
    DPFLTR_NVCTRACE_ID = 121,
    DPFLTR_LUAFV_ID = 122,
    DPFLTR_APPCOMPAT_ID = 123,
    DPFLTR_USBSTOR_ID = 124,
    DPFLTR_SBP2PORT_ID = 125,
    DPFLTR_COVERAGE_ID = 126,
    DPFLTR_CACHEMGR_ID = 127,
    DPFLTR_MOUNTMGR_ID = 128,
    DPFLTR_CFR_ID = 129,
    DPFLTR_TXF_ID = 130,
    DPFLTR_KSECDD_ID = 131,
    DPFLTR_FLTREGRESS_ID = 132,
    DPFLTR_MPIO_ID = 133,
    DPFLTR_MSDSM_ID = 134,
    DPFLTR_UDFS_ID = 135,
    DPFLTR_PSHED_ID = 136,
    DPFLTR_STORVSP_ID = 137,
    DPFLTR_LSASS_ID = 138,
    DPFLTR_SSPICLI_ID = 139,
    DPFLTR_CNG_ID = 140,
    DPFLTR_EXFAT_ID = 141,
    DPFLTR_FILETRACE_ID = 142,
    DPFLTR_XSAVE_ID = 143,
    DPFLTR_SE_ID = 144,
    DPFLTR_DRIVEEXTENDER_ID = 145,
    DPFLTR_POWER_ID = 146,
    DPFLTR_CRASHDUMPXHCI_ID = 147,
    DPFLTR_GPIO_ID = 148,
    DPFLTR_REFS_ID = 149,
    DPFLTR_WER_ID = 150,
    DPFLTR_CAPIMG_ID = 151,
    DPFLTR_VPCI_ID = 152,
    DPFLTR_STORAGECLASSMEMORY_ID = 153,
    DPFLTR_FSLIB_ID = 154,
    DPFLTR_ENDOFTABLE_ID
} DPFLTR_TYPE;

ULONG
DbgPrintEx (
    _In_ ULONG ComponentId,
    _In_ ULONG Level,
    _In_ PCSTR Format,
    ...
    );

inline void MemoryBarrier(VOID)
{
	__sync_synchronize();
}

inline ULONG RtlNtStatusToDosError( __in  NTSTATUS Status )
{
    return (ULONG) Status;
}



//
// symbol data structure
//

typedef struct _IMAGEHLP_SYMBOL64 {
    DWORD   SizeOfStruct;           // set to sizeof(IMAGEHLP_SYMBOL64)
    DWORD64 Address;                // virtual address including dll base address
    DWORD   Size;                   // estimated size of symbol, can be zero
    DWORD   Flags;                  // info about the symbols, see the SYMF defines
    DWORD   MaxNameLength;          // maximum size of symbol name in 'Name'
    CHAR    Name[1];                // symbol name (null terminated string)
} IMAGEHLP_SYMBOL64, *PIMAGEHLP_SYMBOL64;


#define PMDL PVOID

