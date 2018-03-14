// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#if !defined(_LEASE_LAYER_PUBLIC_H_)
#define _LEASE_LAYER_PUBLIC_H_

#include <windef.h>
#include <winbase.h>
#include <winsock2.h>

#include "leaselayerinc.h"

// 
// Lease Layer Callback Routines.
//

/*++
 
Routine Description:
 
    Callback routine used when a leasing application has expired.

Parameters Description:

    Context - context for this operation.

Return Value:
 
    Nothing.
 
--*/
typedef VOID
(WINAPI *LEASING_APPLICATION_EXPIRED_CALLBACK)(
    __in PVOID Context
    );

/*++
 
Routine Description:
 
    Callback routine used when a lease establishment has completed.

Parameters Description:
 
    RemoteLeasingApplicationIdentifier - remote application used in establish lease.

    Context - context for this operation.

Return Value:
 
    Nothing.
 
--*/
typedef VOID
(WINAPI *LEASING_APPLICATION_LEASE_ESTABLISHED_CALLBACK)(
    __in HANDLE Lease,
    __in LPCWSTR RemoteLeasingApplicationIdentifier,
    __in PVOID Context
    );

/*++
 
Routine Description:
 
    Callback routine used when a remote leasing application has expired.

Parameters Description:

    Lease - lease handle.

    RemoteLeasingApplicationIdentifier - remote application identifier that expired.

    Context - context for this operation.

Return Value:
 
    Nothing.
 
--*/
typedef VOID
(WINAPI *REMOTE_LEASING_APPLICATION_EXPIRED_CALLBACK)(
    __in LPCWSTR RemoteLeasingApplicationIdentifier,
    __in PVOID Context
    );

/*++
 
Routine Description:
 
    Callback routine used for lease failure arbitration.

Parameters Description:

    localApplicationHandle - handle of the application performing arbitration.

    localInstance - local lease agent instance.

    localTTL - remaining TTL (ms) for the local side, this value is not really used by the
               arbitration logic, only used for tracing

    remoteSocketAddress - remote address of lease agent to arbitrate with.
    
    remoteInstance - remote lease agent instance.

    remoteTTL - remaining TTL (ms) for the remote side, for one-way lease arbitration this value
                should be set to MAXLONG

    remoteVersion - Lease protocol version of the remote side

    monitorLeaseInstance - the instance for the lease where local node is the monitor, for one
                           way lease this value should be set to 0

    subjectLeaseInstance - the instance for the lease where local node is the subject

    remoteArbitrationDurationUpperBound - the upper bound of the duration (ms) that the remote side
                                        - could have started an arbitration

    context - context for this operation.

Return Value:
 
    Nothing.
 
--*/
typedef VOID
(WINAPI *LEASING_APPLICATION_ARBITRATE_CALLBACK)(
    __in HANDLE localApplicationHandle,
    __in LONGLONG localInstance,
    __in LONG localTTL,
    __in PTRANSPORT_LISTEN_ENDPOINT remoteSocketAddress,
    __in LONGLONG remoteInstance,
    __in LONG remoteTTL,
    __in USHORT remoteVersion,
    __in LONGLONG monitorLeaseInstance,
    __in LONGLONG subjectLeaseInstance,
    __in LONG remoteArbitrationDurationUpperBound,
    __in LPCWSTR RemoteLeasingApplicationIdentifier,
    __in PVOID context
    );


/*++
 
Routine Description:
 
    Callback routine used for remote certificate verification.

Parameters Description:

    cbCertificate - size of the certificate

    pbCertificate - pointer point to the certficate raw bytes

    remoteCertVerifyCallbackOp - operation handle for the complete the verify result

    Context - context for this operation.

Return Value:
 
    Nothing.
 
--*/
typedef VOID
(WINAPI *REMOTE_CERTIFICATE_VERIFY_CALLBACK)(
    __in DWORD cbCertificate,
    __in PBYTE pbCertificate,
    __in PVOID remoteCertVerifyCallbackOp,
    __in PVOID Context
    );


typedef VOID
(WINAPI *HEALTH_REPORT_CALLBACK)(
    __in int reportCode,
    __in LPCWSTR dynamicProperty,
    __in LPCWSTR extraDescription,
    __in PVOID Context
    );

// 
// Lease Layer API.
//

/*++
 
Routine Description:
 
    Registers a local leasing application with the lease layer.
 
Parameters Description:
 
    SocketAddress – socket address of the lease agent.

    LeasingApplicationIdentifier - unique identifier of the leasing application. This ideintifier 
        is unique to the lease agent (and not the machine or cluster).

    LeasingApplicationExpired - callback routine invoked when the lease has expired.

    RemoteLeasingApplicationExpired - callback routine invoked when the remote application has expired.

    LeasingApplicationArbitrate - callback routine used for lease arbitration.

    LeasingApplicationLeaseEstablished - callback routine used for lease establishment.

    RemoteCertificateVerify - callback routine used for remote certificate verification

    CallbackContext - context for the callback routines.

Return Value:
 
    HANDLE to a successfully created leasing application structure, NULL otherwise. Call GetLastError 
    to retrieve the actual error that occured.
 
--*/
#ifdef PLATFORM_UNIX
HANDLE WINAPI 
RegisterLeasingApplication(
    __in PTRANSPORT_LISTEN_ENDPOINT SocketAddress,
    __in LPCWSTR LeasingApplicationIdentifier,
    __in PLEASE_CONFIG_DURATIONS leaseConfigDurations,
    __in LONG LeaseSuspendDurationMilliseconds,
    __in LONG ArbitrationDurationMilliseconds,
    __in LONG NumOfLeaseRetry,
    __in LONG StartOfLeaseRetry,
    void const* securitySettings,
    __in LONG LeasingApplicationExpiryTimeoutMilliseconds,
    __in LEASING_APPLICATION_EXPIRED_CALLBACK LeasingApplicationExpired,
    __in REMOTE_LEASING_APPLICATION_EXPIRED_CALLBACK RemoteLeasingApplicationExpired,
    __in_opt LEASING_APPLICATION_ARBITRATE_CALLBACK LeasingApplicationArbitrate,
    __in_opt LEASING_APPLICATION_LEASE_ESTABLISHED_CALLBACK LeasingApplicationLeaseEstablished,
    __in_opt PVOID CallbackContext,
    __out_opt PLONGLONG Instance,
    __out_opt PHANDLE LeaseAgentHandle
    );
#else
HANDLE WINAPI 
RegisterLeasingApplication(
    __in PTRANSPORT_LISTEN_ENDPOINT SocketAddress,
    __in LPCWSTR LeasingApplicationIdentifier,
    __in PLEASE_CONFIG_DURATIONS leaseConfigDurations,
    __in LONG LeaseSuspendDurationMilliseconds,
    __in LONG ArbitrationDurationMilliseconds,
    __in LONG NumOfLeaseRetry,
    __in LONG StartOfLeaseRetry,
    __in LONG SecurityType,
    __in_bcount(SHA1_LENGTH) PBYTE CertHash,
    __in LPCWSTR CertHashStoreName,
    __in LONG SessionExpirationTimeoutMilliseconds,
    __in LONG LeasingApplicationExpiryTimeoutMilliseconds,
    __in LEASING_APPLICATION_EXPIRED_CALLBACK LeasingApplicationExpired,
    __in REMOTE_LEASING_APPLICATION_EXPIRED_CALLBACK RemoteLeasingApplicationExpired,
    __in_opt LEASING_APPLICATION_ARBITRATE_CALLBACK LeasingApplicationArbitrate,
    __in_opt LEASING_APPLICATION_LEASE_ESTABLISHED_CALLBACK LeasingApplicationLeaseEstablished,
    __in_opt REMOTE_CERTIFICATE_VERIFY_CALLBACK RemoteCertificateVerify,
    __in_opt HEALTH_REPORT_CALLBACK HealthReportCallback,
    __in_opt PVOID CallbackContext,
    __out_opt PLONGLONG Instance,
    __out_opt PHANDLE LeaseAgentHandle
    );
#endif

/*++
 
Routine Description:
 
    Unregisters a local application from the lease layer and frees all resources
    associated with it. All leases for this application are terminated.
 
Parameters Description:
 
    LeasingApplication - leasing application handle.
 
Return Value:
 
    TRUE if the local application was successfully unregistered. Call GetLastError 
    to retrieve the actual error that occured.
 
--*/
BOOL WINAPI 
UnregisterLeasingApplication(
    __in HANDLE LeasingApplication,
    __in BOOL IsDelayed
    );

/*++
 
Routine Description:
 
    Establishes a logical lease relationship between a local and a remote application.
 
Parameters Description:
 
    LeasingApplication - leasing application handle (subject).

    LeaseTimeToLiveMilliseconds - lease time to live expressed in milliseconds.

    RemoteApplicationIdentifier - unique identifier of the remote application (monitor).

    RemoteSocketAddress - network address information used to bind to the remote lease agent.

    LeasingApplicationLeaseEstablished - callback routine used to notify the local application 
        when the lease relationship has actually been established (lease response was processed).

    LeasingApplicationLeaseEstablishedContext - context for this callback routine.
 
Return Value:
 
    HANDLE to a successfully created lease, NULL otherwise. Call GetLastError 
    to retrieve the actual error that occured.
 
--*/
__success(return != NULL)
HANDLE WINAPI 
EstablishLease(
    __in HANDLE LeasingApplication,
    __in LPCWSTR RemoteApplicationIdentifier,
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress,
    __in LONGLONG RemoteLeaseAgentInstance,
    __in LEASE_DURATION_TYPE LeaseDurationType,
    __out PBOOL IsEstablished
    );
 
/*++
 
Routine Description:
 
    Terminates a lease relationship between a local and a remote application.
 
Parameters Description:
 
    Lease - lease handle.
 
Return Value:
 
    TRUE if the lease relationship has successfully been terminated. Call GetLastError 
    to retrieve the actual error that occured.
 
--*/
BOOL WINAPI 
TerminateLease(
    __in HANDLE LeasingApplication,
    __in HANDLE Lease,
    __in LPCWSTR RemoteApplicationIdentifier
    );
 
/*++
 
Routine Description:
 
    Gets the time for which the local application is guaranteed to have valid leases.
 
Parameters Description:
 
    LeaseApplication - lease application handle.

    RemainingTimeToLiveMilliseconds - on return, it contains the current lease expiration time in milliseconds.

    KernelCurrentTime - on return, it contains the current system time in kernel.
 
Return Value:
 
    TRUE if arguments are valid, FALSE otherwise.
 
--*/
__success(return == TRUE)
BOOL WINAPI 
GetLeasingApplicationExpirationTime(
    __in HANDLE LeasingApplication,
    __in LONG RequestTimeToLiveMilliseconds,
    __out PLONG RemainingTimeToLiveMilliseconds,
    __out PLONGLONG KernelCurrentTime
    );


/*++

Routine Description:

Get the expiration time for a remote application.  Note that either
the monitor or the subject side TTL can be negative if it has already
expired.

Parameters Description:

LeaseApplication - lease application handle.

RemoteApplicationIdentifier - remote application id.

MonitorExpireTTL - TTL for lease relationship where the local acts as monitor, 0 if not found

SubjectExpireTTL - TTL for lease relationship where the local acts as subject, 0 if not found

Return Value:

TRUE if arguments are valid, FALSE otherwise.

--*/
__success(return == TRUE)
BOOL WINAPI
GetRemoteLeaseExpirationTime(
    __in HANDLE LeasingApplication,
    __in LPCWSTR RemoteApplicationIdentifier,
    __out PLONG MonitorExpireTTL,
    __out PLONG SubjectExpireTTL
    );

/*++
Routine Description:

     Completes a successful arbitration with relation to the remote lease agent.

Parameters Description:

    localApplicationHandle - The handle of the lease application that completed aribitration successfully.

    localInstance - local lease agent instance involved in arbitration.

    remoteSocketAddress - remote socket address.

    remoteInstance - remote lease agent instance involved in arbitration.

    localTTL - time to live for the local lease agent, currently the value is either 0, in which case
               the local side has lost arbitration and must go down, or MAXLONG, in which case the
               local side can continue to live
    remoteTTL - time to live for the remote lease agent, if this value is MAXLONG, the remote side can
                not be declared as down
    isDelayed - whether this is a delayed report

Return Value:

    TRUE if arguments are valid, FALSE otherwise.   

--*/
BOOL WINAPI
CompleteArbitrationSuccessProcessing(
    __in HANDLE localApplicationHandle,
    __in LONGLONG localInstance,
    __in PTRANSPORT_LISTEN_ENDPOINT remoteSocketAddress,
    __in LONGLONG remoteInstance,
    __in LONG localTTL,
    __in LONG remoteTTL,
    __in BOOL isDelayed
    );

/*++
Routine Description:

    Completes a remote certification verification

Parameters Description:

    RemoteCertVerifyCallbackOperation - The operation handle in kernel to process the verify result

    verifyResult - remote certificate verify result from user mode

Return Value:

    TRUE if arguments are valid, FALSE otherwise.   

--*/
BOOL WINAPI
CompleteRemoteCertificateVerification(
    __in HANDLE LeasingApplication,
    __in PVOID RemoteCertVerifyCallbackOperation,
    __in NTSTATUS verifyResult
    );

/*++
Routine Description:

     This is a test function that allows the test to fault the lease agent.

Parameters Description:

    SocketAddress - The address of the listener of the lease agent that needs to be faulted.

Return Value:

    TRUE if arguments are valid, FALSE otherwise.

--*/
BOOL WINAPI
FaultLeaseAgent(
    __in PTRANSPORT_LISTEN_ENDPOINT SocketAddress
    );


/*++
Routine Description:

     This is a test function that allows the test to block  the lease agent.
     This is functional only in the debug build.

Parameters Description:

    SocketAddress - The address of the listener of the lease agent that needs to be faulted.

Return Value:

    TRUE if arguments are valid, FALSE otherwise.

--*/
BOOL WINAPI
BlockLeaseAgent(
    __in PTRANSPORT_LISTEN_ENDPOINT SocketAddress
    );

/*++
 
Routine Description:
 
    Set the time of global lease expiration.
 
Parameters Description:
 
    LeaseApplication - lease application handle.

    GlobalLeaseExpireTime - global lease expire time (kernel time).
 
Return Value:
 
    TRUE if arguments are valid, FALSE otherwise.
 
--*/
BOOL WINAPI 
SetGlobalLeaseExpirationTime(
    __in HANDLE LeasingApplication,
    __in LONGLONG GlobalLeaseExpireTime
    );

#ifdef PLATFORM_UNIX
BOOL WINAPI UpdateSecuritySettings(
    HANDLE LeasingApplication,
    Transport::SecuritySettings const* securitySettings); 
#else
/*++
 
Routine Description:
 
    Update the certificate.
 
Parameters Description:
 
    LeaseApplication - lease application handle.

Return Value:
 
    TRUE if arguments are valid, FALSE otherwise.
 
--*/
BOOL WINAPI 
UpdateCertificate(
    __in HANDLE LeasingApplication,
    __in_bcount(SHA1_LENGTH) PBYTE CertHash,
    __in LPCWSTR CertHashStoreName
    );
#endif

/*++
Routine Description:
    Pass self-relative security descriptor to driver for lease transport connection authorization
 
Parameters Description:
    LeaseApplication - kernel lease agent handle.

Return Value:
    TRUE if arguments are valid, FALSE otherwise.
--*/
BOOL WINAPI 
SetListenerSecurityDescriptor(
    _In_ HANDLE LeaseAgentHandle,
    _In_ PSECURITY_DESCRIPTOR selfRelativeSecurityDescriptor
    );

/*++
 
Routine Description:
     This is a test function that allows the test to
     block connections between local lease agent and remote lease agent.
 
Parameters Description:

    LocalSocketAddress - The address of the listener of the lease agent that needs to be faulted.
    RemoteSocketAddress - The address of the remote side for the faulting connections.
    IsBlocking - 'true' if blocking; 'false' if unblocking.

Return Value:

    TRUE if arguments are valid, FALSE otherwise.

--*/
BOOL WINAPI 
BlockLeaseConnection(
    __in PTRANSPORT_LISTEN_ENDPOINT LocalSocketAddress,
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress,
    __in BOOL IsBlocking
    );

BOOL WINAPI
ClearTransportBehavior(
__in std::wstring Alias
);

BOOL WINAPI
AddTransportBehavior(
__in PTRANSPORT_LISTEN_ENDPOINT LocalSocketAddress,
__in BOOL FromAny,
__in PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress,
__in BOOL ToAny,
__in LEASE_BLOCKING_ACTION_TYPE BlockingType,
__in std::wstring Alias
);

/*++
 
Routine Description:
     This is a test function that allows the test to
     block a specific type of lease message from source to destination
 
Parameters Description:
    LocalSocketAddress - The address of the listener of the lease agent that needs to be faulted.
    RemoteSocketAddress - The address of the remote side for the faulting connections.
    FromAny - if the flag is true, then any source address will be considerred;
    ToAny - if the flag is true, then any destination address will be considerred;
    BlockingType - The enum to specify which type of message to block.
    Alias - The alias for this blocking rule. (optional)

Return Value:
    TRUE if arguments are valid, FALSE otherwise.

--*/
BOOL WINAPI
addLeaseBehavior(
    __in PTRANSPORT_LISTEN_ENDPOINT LocalSocketAddress,
    __in BOOL FromAny,
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress,
    __in BOOL ToAny,
    __in LEASE_BLOCKING_ACTION_TYPE BlockingType,
    __in std::wstring Alias
);

/*++
 
Routine Description:
    This is a test function that removes the blocking rules set by the addLeaseBehavior.
 
Parameters Description:
    Alias - The alias when setting the addLeaseBehavior rule. When the alias is empty, all 
    blocking rules will be cleared.
    
Return Value:
    TRUE if arguments are valid, FALSE otherwise.

--*/
BOOL WINAPI
removeLeaseBehavior(
    __in std::wstring Alias
);

/*++
Routine Description:
    Pass lease duration with its index to driver lease agent when there is user mode federation configuration update
--*/
BOOL WINAPI 
UpdateLeaseDuration(
    __in HANDLE LeaseAgentHandle,
    __in PLEASE_CONFIG_DURATIONS LeaseDurations
    );

/*++
Routine Description: Retrieve the existing lease relationship duration from driver
--*/
LONG WINAPI 
QueryLeaseDuration(
    __in PTRANSPORT_LISTEN_ENDPOINT LocalSocketAddress,
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress
    );

/*++
Routine Description: Pass heartbeat error code (or success code) to lease driver
--*/
BOOL WINAPI
UpdateHeartbeatResult(
    __in ULONG ErrorCode
);

/*++
Routine Description: Pass lease global config to lease driver
--*/
BOOL WINAPI
UpdateLeaseGlobalConfig(
    __in PLEASE_GLOBAL_CONFIG LeaseGlobalConfig
    );

#endif  // _LEASE_LAYER_PUBLIC_H_
