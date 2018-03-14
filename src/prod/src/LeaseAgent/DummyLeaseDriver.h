// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

// 
// Lease Layer Callback Routines.
//

/*++
Routine Description: Callback routine used for when a leasing application has expired.

Parameters Description: 
Context - context for this operation.

Return Value: Nothing.
--*/
typedef VOID
    (WINAPI *Dummy_LEASING_APPLICATION_EXPIRED_CALLBACK)(
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
    (WINAPI *Dummy_LEASING_APPLICATION_LEASE_ESTABLISHED_CALLBACK)(
    __in HANDLE leaseHandle,
    __in LPCWSTR RemoteLeasingApplicationIdentifier,
    __in PVOID Context
    );

/*++

Routine Description:
Callback routine used for when a remote leasing application has expired.

Parameters Description:
LeasingApplicationHandle - handle to leasing application.
RemoteLeasingApplicationIdentifier - remote application identifier that expired.
Context - context for this operation.

Return Value: Nothing.
--*/

typedef VOID
    (WINAPI *Dummy_REMOTE_LEASING_APPLICATION_EXPIRED_CALLBACK)(
    __in LPCWSTR RemoteLeasingApplicationIdentifier,
    __in PVOID Context
    );

/*++

Routine Description:
    Callback routine used for lease failure arbitration.
Parameters Description:
    monitorTTL - remaining time to live for monitor.
    subjectTTL - remaining time to live for subject.
    RemoteSocketAddress - remote address of lease agent to arbitrate with.
    Instance - lease agent instance.
    RemoteInstance - remote lease agent instance.
    Context - context for this operation.
Return Value:
    Nothing.
--*/
typedef VOID
    (WINAPI *Dummy_LEASING_APPLICATION_ARBITRATE_CALLBACK)(
    __in HANDLE localApplicationHandle,
    __in LONGLONG localInstance,
    __in LONG localTTL,
    __in LPCWSTR remoteSocketAddress,
    __in LONGLONG remoteInstance,
    __in LONG remoteTTL,
    __in PVOID context
    );

// 
// Lease Layer API.
//

/*++
Routine Description:
Registers a local leasing application with the lease layer.

Parameters Description:
LeaseAgent - The lease agent.
LeasingApplicationIdentifier - unique identifier of the leasing application. This ideintifier 
is unique to the application domain.
LeasingApplicationExpired - callback routine invoked when the lease has expired.
RemoteLeasingApplicationExpired - callback routine invoked when the remote node has failed.
LeasingApplicationArbitrate - callback routine invoked when arbitration is required.
LeasingApplicationLeaseEstablished - callback routine invoked when the lease has been established.
Callback Context - context for the callback routine.

Return Value:
Return true if successfully created leasing application structure, false otherwise. Call GetLastError 
to retrieve the actual error that occured.
--*/
HANDLE WINAPI 
    Dummy_RegisterLeasingApplication(
    __in LPCWSTR SocketAddress,
    __in LPCWSTR LeasingApplicationIdentifier,
    __in PLEASE_CONFIG_DURATIONS leaseConfigDurations,
    __in LONG, // LeaseSuspendDurationMilliseconds
    __in LONG, // ArbitrationDurationMilliseconds
    __in LONG, // NumOfLeaseRetry
    __in Dummy_LEASING_APPLICATION_EXPIRED_CALLBACK LeasingApplicationExpired,
    __in Dummy_REMOTE_LEASING_APPLICATION_EXPIRED_CALLBACK RemoteLeasingApplicationExpired,
    __in Dummy_LEASING_APPLICATION_ARBITRATE_CALLBACK LeasingApplicationArbitrate,
    __in_opt Dummy_LEASING_APPLICATION_LEASE_ESTABLISHED_CALLBACK LeasingApplicationLeaseEstablished,
    __in_opt PVOID CallbackContext,
    __out_opt PLONGLONG Instance
    );


/*++
Routine Description:
Unregisters a local application from the lease layer and frees all resources associated with it.

Parameters Description:
LeasingApplication - The leasing application.

Return Value:
TRUE if the local application was successfully unregistered. Call GetLastError 
to retrieve the actual error that occured.
--*/
BOOL WINAPI 
    Dummy_UnregisterLeasingApplication(
    __in HANDLE LeasingApplication
    );

/*++

Routine Description:
Establishes a logical lease relationship between a local and a remote application.

Parameters Description:
LeasingApplication - The leasing application.
TimeToLiveMilliseconds - lease time to live expressed in milliseconds.
RemoteApplicationIdentifier - unique identifier of the remote application (monitor).
RemoteSocketAddress - network address information used to bind to the remote lease agent.
LeasingApplicationLeaseEstablished - callback routine used to notify the local application 
when the lease relationship has actually been established (lease response was processed).
LeasingApplicationLeaseEstablishedContext - context for this callback routine.

Return Value:
HANDLE to a successfully created lease, NULL otherwise. Call GetLastError 
to retrieve the actual error that occured.
--*/
HANDLE WINAPI 
    Dummy_EstablishLease(
    __in HANDLE LeasingApplication,
    __in LPCWSTR RemoteApplicationIdentifier,
    __in LPCWSTR RemoteSocketAddress,
    __in LONGLONG RemoteLeaseAgentInstanceId
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
    Dummy_TerminateLease(
    __in HANDLE Lease
    );

/*++

Routine Description:
Gets the time for which the local application is guaranteed to have valid leases.

Parameters Description:
LeaseApplication - lease application handle.
RemainingTimeToLiveMilliseconds - on return, it contains the current lease expiration time in milliseconds.

Return Value:
TRUE if arguments are valid, FALSE otherwise.
--*/
BOOL WINAPI 
    Dummy_GetLeasingApplicationExpirationTime(
    __in HANDLE LeasingApplication,
    __out PLONG RemainingTimeToLiveMilliseconds,
    __out PLONGLONG KernelCurrentTime
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
    Dummy_SetGlobalLeaseExpirationTime(
    __in HANDLE LeasingApplication,
    __in LONGLONG KernelCurrentTime
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
BOOL WINAPI
Dummy_GetRemoteLeaseExpirationTime(
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

Return Value:

TRUE if arguments are valid, FALSE otherwise.

--*/
BOOL WINAPI
    Dummy_CompleteArbitrationSuccessProcessing(
    __in HANDLE localApplicationHandle,
    LONGLONG localInstance,
    __in LPCWSTR remoteSocketAddress,
    LONGLONG remoteInstance,
    LONG localTTL,
    LONG remoteTTL
    );

/*++
Routine Description:
To delete a particular applicaiton. This is done by the watchdog which ensures that
the state of a stuck process is also cleaned up.
Parameters Description:
LeasingApplicationIdentifier - The applicaiton which needs to be deleted.
Return Value:
TRUE if arguments are valid, FALSE otherwise.
--*/
BOOL WINAPI
    Dummy_DeleteRegisteredApplication(
    __in LPCWSTR LeasingApplicationIdentifier
    );

/*++
Routine Description:
This is a test function that allows the test to fault the leaseagent.
Parameters Description:
LocalSocketAddress - The address of the listener of the lease agent that needs to be faulted.
Return Value:
TRUE if arguments are valid, FALSE otherwise.
--*/
BOOL WINAPI
    Dummy_FaultLeaseAgent(
    __in LPCWSTR LocalSocketAddress
    );

std::wstring Dummy_DumpLeases();
