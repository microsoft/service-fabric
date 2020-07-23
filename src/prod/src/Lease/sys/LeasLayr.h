// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------



#if !defined(_LEASLAYR_H_)
#define _LEASLAYR_H_

#define RTL_USE_AVL_TABLES 0 // switches from splay trees to AVL trees for improved performance
#include <ntosp.h>
#include <zwapi.h>
#include <ntddk.h>
#include <wdf.h>
#include <wsk.h>
#include <limits.h>
#include <ip2string.h>
#include <windef.h>

#include "Microsoft-ServiceFabric-LeaseEvents.h"

#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include <ntintsafe.h>

#include <initguid.h>

#include "leaselayerio.h"
#include "leaselayertrace.h"
#include "leaselayerinc.h"

#ifdef KTL_TRANSPORT
#include "../ktransport/leasetransport.h"
#else
#include "transport.h"
#endif

//
// Lease Layer Trace Provider.
//
DEFINE_GUID(GUID_TRACE_PROVIDER_LEASELAYER,
        0x3f68b79e, 0xa1cf, 0x4b10, 0x8c, 0xfd, 0x3d, 0xfe, 0x32, 0x2f, 0x07, 0xcb);
// {3F68B79E-A1CF-4B10-8CFD-3DFE322F07CB}

//
// Tag used to mark all memory allocations.
//
#define LEASELAYER_POOL_TAG (ULONG) 'YLSL'
#define LEASE_RELATHIONSHIP_CTXT_TAG (ULONG) 'CSSL'
#define LEASE_RELATHIONSHIP_ID_TAG (ULONG) 'ISSL'
#define LEASE_TABLE_TAG (ULONG) 'BTSL'
#define LEASE_REMOTE_LEASE_AGENT_CTXT_TAG (ULONG) 'CRSL'
#define LEASE_REMOTE_LEASE_AGENT_ID_TAG (ULONG) 'IRSL'
#define LEASE_EVENT_TAG (ULONG) 'NESL'
#define LEASE_APPLICATION_CTXT_TAG (ULONG) 'CASL'
#define LEASE_APPLICATION_ID_TAG (ULONG) 'IASL'
#define LEASE_MESSAGE_TAG (ULONG) 'SMSL'

#define LEASELAYER_REMOTE_CERT_CONTEXT (ULONG) 'CCSL'
#define LEASE_TAG_REMOTE_CERT 'CRtl'
#define LEASE_TAG_LOCAL_CERT_STORENAME 'CLSL'
#define LEASE_LISTEN_ENDPOINT (ULONG) 'PESL'

#if DBG
#define LAssert(cond) ((!(cond)) ? (KdBreakPoint(), FALSE) : TRUE)
#else
#define LAssert(cond) ( 0 )
#endif

#define LEASLAYR_BUGCHECK_CODE (0xE0010002)
#define DRIVER_UNLOAD_TIMEOUT_CODE (258)
// 258 = WAIT_TIMEOUT
#define LEASLAYR_PROCESS_ASSERT_EXIT_TIMEOUT_BUGCHECK_CODE (258)

//
// Device root name.
//
#define LEASELAYER_DEVICE_NAME_STRING L"\\Device\\LeaseLayer"
#define LEASELAYER_SYMBOLIC_NAME_STRING L"\\DosDevices\\LeaseLayer"

//
// Common LARGE_INTEGER operations.
//
#define LARGE_INTEGER_ADD_LARGE_INTEGER_AND_ASSIGN(a,b,c) (a.QuadPart = b.QuadPart + c.QuadPart)
#define LARGE_INTEGER_ADD_INTEGER_AND_ASSIGN(a,b,c) (a.QuadPart = b.QuadPart + (LONGLONG)c)
#define LARGE_INTEGER_SUBTRACT_INTEGER_AND_ASSIGN(a,b,c) (a.QuadPart = b.QuadPart - (LONGLONG)c)
#define LARGE_INTEGER_SUBTRACT_LARGE_INTEGER_AND_ASSIGN(a,b,c) (a.QuadPart = b.QuadPart - c.QuadPart)
#define LARGE_INTEGER_ADD_INTEGER(a,b) (a.QuadPart = a.QuadPart + (LONGLONG)b)
#define LARGE_INTEGER_SUBTRACT_INTEGER(a,b) (a.QuadPart = a.QuadPart - (LONGLONG)b)
#define LARGE_INTEGER_ASSIGN_INTEGER(a,b) (a.QuadPart = (LONGLONG)b)
#define IS_LARGE_INTEGER_GREATER_THAN_OR_EQUAL_INTEGER(a,b) (a.QuadPart >= (LONGLONG)b)
#define IS_LARGE_INTEGER_GREATER_THAN_INTEGER(a,b) (a.QuadPart > (LONGLONG)b)
#define IS_LARGE_INTEGER_GREATER_THAN_OR_EQUAL_LARGE_INTEGER(a,b) (a.QuadPart >= b.QuadPart)
#define IS_LARGE_INTEGER_GREATER_THAN_LARGE_INTEGER(a,b) (a.QuadPart > b.QuadPart)
#define IS_LARGE_INTEGER_LESS_THAN_LARGE_INTEGER(a,b) (a.QuadPart < b.QuadPart)
#define IS_LARGE_INTEGER_LESS_THAN_OR_EQUAL_INTEGER(a,b) (a.QuadPart <= (LONGLONG)b)
#define IS_LARGE_INTEGER_LESS_THAN_INTEGER(a,b) (a.QuadPart < (LONGLONG)b)
#define IS_LARGE_INTEGER_EQUAL_TO_LARGE_INTEGER(a,b) (a.QuadPart == b.QuadPart)
#define IS_LARGE_INTEGER_EQUAL_TO_INTEGER(a,b) (a.QuadPart == (LONGLONG)b)

//
// All default time constants are in milliseconds.
//
#define LEASE_PROTOCOL_MAJOR_VERSION 2
#define LEASE_PROTOCOL_MINOR_VERSION_V1 0
#define LEASE_PROTOCOL_MINOR_VERSION 1
#define MILLISECOND_TO_NANOSECOND_FACTOR 10000
#define DURATION_MAX_VALUE (MAXLONG - 1)
#define ARBITRATION_TIMEOUT_PERCENTAGE 1
#define RENEW_RETRY_INTERVAL_PERCENTAGE 5
#define RENEW_RETRY_INTERVAL_PERCENTAGE_LOW 3
#define RENEW_RETRY_INTERVAL_PERCENTAGE_HIGH 2
// MAINTENANCE_INTERVAL = 15 seconds
#define MAINTENANCE_INTERVAL 15
// DRIVER_UNLOAD_TIMEOUT = 60000 milliseconds
#define DRIVER_UNLOAD_TIMEOUT 60000
// PROCESS_ASSERT_EXIT_TIMEOUT = 300000 milliseconds
#define PROCESS_ASSERT_EXIT_TIMEOUT 300000
// DELAY_LEASE_AGENT_CLOSE_INTERVAL = 1000 milliseconds
#define DELAY_LEASE_AGENT_CLOSE_INTERVAL 1000
#define PRE_ARBITRATION_TIME 2000 * MILLISECOND_TO_NANOSECOND_FACTOR //2 seconds before arbitration
#define PING_RETRY_INTERVAL 10000 * MILLISECOND_TO_NANOSECOND_FACTOR //10 seconds

//
// Cleanup thread (garbage collector).
//
KSTART_ROUTINE MaintenanceWorkerThread;

//
// Maintenance thread context.
//
typedef struct _WORKER_THREAD_CONTEXT {

    //
    // Worker thread handle.
    //
    HANDLE WorkerThreadHandle;
    //
    // Shutdown event. Signaled when the thread has to exit.
    //
    KEVENT Shutdown;

} WORKER_THREAD_CONTEXT, *PWORKER_THREAD_CONTEXT;

//
// The driver object context space. Extra, nonpageable, memory space
// that a driver can allocate to a device object. Globally allocated
// structures typically go in this context.
//
typedef struct _DRIVER_CONTEXT {

    //
    // Driver Object
    //
    PDRIVER_OBJECT DriverObject;
    //
    // Singleton state for KTL transport
    //
    PDRIVER_TRANSPORT DriverTransport;
    //
    // Lease agent hash table concurrency control. For performance
    // we coudl consider changing this later to a reader/writer lock.
    //
    FAST_MUTEX LeaseAgentContextHashTableAccessMutex;
    //
    // Map of lease agents.
    //
    RTL_GENERIC_TABLE LeaseAgentContextHashTable;
    //
    // Worker thread context.
    //
    WORKER_THREAD_CONTEXT MaintenanceWorkerThreadContext;
    //
    // Flag indicating we're unloading and can skip some work
    //
    BOOLEAN Unloading;
    //
    // Last instance requested
    //
    LARGE_INTEGER LastInstance;
    //
    // Instance concurrency control.
    //
    KSPIN_LOCK InstanceLock;
    //
    // Unresponsive duration
    //
    LONG UnresponsiveDuration;
    //
    // Last update time from user mode for heartbeat
    //
    LARGE_INTEGER LastHeartbeatUpdateTime;
    //
    // Last Heartbeat error code returned from user mode
    //
    ULONG LastHeartbeatErrorCode;
    //
    // Test only. Table for blocking behaviors
    //
    RTL_GENERIC_TABLE TransportBehaviorTable;
    //
    // Test only. SpinLock for accessing TransportBehaviorTable
    //
    KSPIN_LOCK TransportBehaviorTableAccessSpinLock;
    //
    // Maintenance interval for maintenance thread
    //
    LONG MaintenanceInterval;
    //
    // Process assert exit timeout
    //
    LONG ProcessAssertExitTimeout;
    //
    // Delay lease agent close interval
    //
    LONG DelayLeaseAgentCloseInterval;

#ifndef KTL_TRANSPORT
    //
    // WSK Registration (required).
    //
    WSK_REGISTRATION WskRegistration;
    //
    // Indicates success/failure of initializing the WSK subsystem.
    //
    BOOLEAN WskRegistrationSucceeded;
    //
    // Required for Wsk.
    //
    WSK_PROVIDER_NPI WskProviderNpi;
    //
    // Indicates success/failure of initializing the WSK subsystem.
    //
    BOOLEAN WskProviderNpiSucceeded;
    //
    // Concurrency control.
    //
    FAST_MUTEX WskNpiMutex;
    //
    // Event signalled when driver can unload
    //
    KEVENT CanUnloadEvent;
    //
    // Refcount driver load/unload
    //
    LONG UnloadCount;
#endif

} DRIVER_CONTEXT, *PDRIVER_CONTEXT;

VOID DriverUnloadAddref();
VOID DriverUnloadRelease();

//
// The device object context space. Extra, nonpageable, memory space
// that a driver can allocate to a device object.
//
typedef struct _DEVICE_CONTEXT {

    ULONG Reserved;
}  DEVICE_CONTEXT, *PDEVICE_CONTEXT;

//
// Request context space. Used to manage request completion/cacellation.
//
typedef struct _REQUEST_CONTEXT {

    //
    // TRUE if the request has been canceled.
    //
    BOOLEAN IsCanceled;
    //
    // TRUE if the request has been completed.
    //
    BOOLEAN IsCompleted;
    //
    // Request concurrency control.
    //
    KSPIN_LOCK Lock;

} REQUEST_CONTEXT, *PREQUEST_CONTEXT;

//
// Context routines (driver, device, and request).
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DRIVER_CONTEXT, LeaseLayerDriverGetContext)
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, LeaseLayerDeviceGetContext)
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(REQUEST_CONTEXT, LeaseLayerRequestGetContext)

//
// Hash table routines.
//

RTL_AVL_COMPARE_ROUTINE GenericTableCompareLeaseRelationshipContexts;
RTL_AVL_COMPARE_ROUTINE GenericTableCompareLeaseAgentContexts;
RTL_AVL_COMPARE_ROUTINE GenericTableCompareRemoteLeaseAgentContexts;
RTL_AVL_COMPARE_ROUTINE GenericTableCompareLeasingApplicationContexts;
RTL_AVL_COMPARE_ROUTINE GenericTableCompareLeaseEvents;
RTL_AVL_COMPARE_ROUTINE GenericTableCompareCertificates;
RTL_AVL_COMPARE_ROUTINE GenericTableCompareLeaseRemoteCertOp;
RTL_AVL_COMPARE_ROUTINE GenericTableCompareTransportBlocking;
RTL_AVL_ALLOCATE_ROUTINE GenericTableAllocate;
RTL_AVL_FREE_ROUTINE GenericTableDeallocate;

//
// Driver entry point.
//
DRIVER_INITIALIZE DriverEntry;

//
// Driver events.
//
EVT_WDF_DRIVER_UNLOAD LeaseLayerEvtDriverUnload;
EVT_WDF_OBJECT_CONTEXT_CLEANUP LeaseLayerEvtDriverContextCleanup;
EVT_WDF_DEVICE_SHUTDOWN_NOTIFICATION LeaseLayerEvtShutdown;
EVT_WDF_DRIVER_DEVICE_ADD LeaseLayerEvtDeviceAdd;
EVT_WDF_DEVICE_CONTEXT_CLEANUP LeaseLayerEvtDeviceContextCleanup;

//
// I/O events callbacks.
//
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL LeaseLayerEvtIoDeviceControl;

//
// Power events callbacks
//
EVT_WDF_DEVICE_D0_EXIT  LeaseLayerEvtDeviceD0Exit;

//
// Routine used to cancel a delayed request.
//
EVT_WDF_REQUEST_CANCEL DelayedRequestCancelation;

//
// Routine used to establish correct security descriptor
//
NTSTATUS LeaseLayerEstablishSecurity(
    __in WDFDEVICE ControlDevice
    );

//
// Message types supported by the lease layer.
//
typedef enum _LEASE_MESSAGE_TYPE {

    LEASE_REQUEST,
    LEASE_RESPONSE,
    PING_REQUEST,
    PING_RESPONSE,
    FORWARD_REQUEST,
    FORWARD_RESPONSE,
    RELAY_REQUEST,
    RELAY_RESPONSE

} LEASE_MESSAGE_TYPE, *PLEASE_MESSAGE_TYPE;

//
// Types of receive operations. Message header is received first.
//
typedef enum _RECEIVE_FRAGMENT {

    RECEIVE_HEADER,
    RECEIVE_BODY

} RECEIVE_FRAGMENT, *PRECEIVE_FRAGMENT;

//
// Message body list descriptor.
//
typedef struct _LEASE_MESSAGE_BODY_LIST_DESCRIPTOR {

    //
    // Number of elements in the list.
    //
    DWORD Count;
    //
    // Offset to the first element in the list.
    //
    DWORD StartOffset;
    //
    // The whole size in bytes.
    //
    ULONG Size;

} LEASE_MESSAGE_BODY_LIST_DESCRIPTOR, *PLEASE_MESSAGE_BODY_LIST_DESCRIPTOR;

#define LEASE_MESSAGE_BODY_LIST_DESCRIPTOR_COUNT 7

//
// Lease message header.
//
typedef struct _LEASE_MESSAGE_HEADER_V1 {

    //
    // Protocol versioning information (major version).
    // This represents a breaking change.
    //
    BYTE MajorVersion;
    //
    // Protocol versioning information (minor version).
    // This represents an upgradable change.
    //
    BYTE MinorVersion;
    //
    // Message header size.
    //
    ULONG MessageHeaderSize;
    //
    // Message body size.
    //
    ULONG MessageSize;
    //
    // Identifier of the message.
    //
    LARGE_INTEGER MessageIdentifier;
    //
    // Message type.
    //
    LEASE_MESSAGE_TYPE MessageType;
    //
    // Lease identifier.
    //
    LARGE_INTEGER LeaseInstance;
    //
    // Remote lease agent instance. Required by arbitration.
    //
    LARGE_INTEGER RemoteLeaseAgentInstance;
    //
    // Lease TTL.
    //
    LONG Duration;
    //
    // Requested lease expiration.
    //
    LARGE_INTEGER Expiration;
    //
    // Lease short arbitration timeout.
    //
    LONG LeaseSuspendDuration;
    //
    // Lease long arbitration timeout.
    //
    LONG ArbitrationDuration;
    //
    // Lease long arbitration timeout.
    //
    BOOLEAN IsTwoWayTermination;
    //
    // List of lease relationships in subject role that are not yet established.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectPendingListDescriptor;
    //
    // List of lease relationships in subject role that are now failed.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectFailedPendingListDescriptor;
    //
    // List of lease relationships in monitor role that are now failed.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR MonitorFailedPendingListDescriptor;
    //
    // List of lease relationships in subject role that are now accepted.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectPendingAcceptedListDescriptor;
    //
    // List of lease relationships in subject role that are now failed.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectFailedAcceptedListDescriptor;
    //
    // List of lease relationships in monitor role that are now failed.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR MonitorFailedAcceptedListDescriptor;
    //
    // List of lease relationships in subject role that are now rejected.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectPendingRejectedListDescriptor;
    //
    // List of lease relationships in subject role that are now terminated.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectTerminatePendingListDescriptor;
    //
    // List of lease relationships in subject role that are now accepected as being terminated.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectTerminateAcceptedListDescriptor;
    //
    // Listen end point for sender in message body
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR MessageListenEndPointDescriptor;

} LEASE_MESSAGE_HEADER_V1, *PLEASE_MESSAGE_HEADER_V1;

//
// Lease message header.
//
typedef struct _LEASE_MESSAGE_HEADER {

    //
    // Protocol versioning information (major version).
    // This represents a breaking change.
    //
    BYTE MajorVersion;
    //
    // Protocol versioning information (minor version).
    // This represents an upgradable change.
    //
    BYTE MinorVersion;
    //
    // Message header size.
    //
    ULONG MessageHeaderSize;
    //
    // Message body size.
    //
    ULONG MessageSize;
    //
    // Identifier of the message.
    //
    LARGE_INTEGER MessageIdentifier;
    //
    // Message type.
    //
    LEASE_MESSAGE_TYPE MessageType;
    //
    // Lease identifier.
    //
    LARGE_INTEGER LeaseInstance;
    //
    // Remote lease agent instance. Required by arbitration.
    //
    LARGE_INTEGER RemoteLeaseAgentInstance;
    //
    // Lease TTL.
    //
    LONG Duration;
    //
    // Requested lease expiration.
    //
    LARGE_INTEGER Expiration;
    //
    // Lease short arbitration timeout.
    //
    LONG LeaseSuspendDuration;
    //
    // Lease long arbitration timeout.
    //
    LONG ArbitrationDuration;
    //
    // Lease long arbitration timeout.
    //
    BOOLEAN IsTwoWayTermination;
    //
    // List of lease relationships in subject role that are not yet established.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectPendingListDescriptor;
    //
    // List of lease relationships in subject role that are now failed.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectFailedPendingListDescriptor;
    //
    // List of lease relationships in monitor role that are now failed.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR MonitorFailedPendingListDescriptor;
    //
    // List of lease relationships in subject role that are now accepted.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectPendingAcceptedListDescriptor;
    //
    // List of lease relationships in subject role that are now failed.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectFailedAcceptedListDescriptor;
    //
    // List of lease relationships in monitor role that are now failed.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR MonitorFailedAcceptedListDescriptor;
    //
    // List of lease relationships in subject role that are now rejected.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectPendingRejectedListDescriptor;
    //
    // List of lease relationships in subject role that are now terminated.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectTerminatePendingListDescriptor;
    //
    // List of lease relationships in subject role that are now accepected as being terminated.
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR SubjectTerminateAcceptedListDescriptor;
    //
    // Listen end point for sender in message body
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR MessageListenEndPointDescriptor;
    //
    // Listen end point for remote destination used in forward/relay messages
    //
    LEASE_MESSAGE_BODY_LIST_DESCRIPTOR LeaseListenEndPointDescriptor;

} LEASE_MESSAGE_HEADER, *PLEASE_MESSAGE_HEADER;

typedef struct _LEASE_MESSAGE_EXT
{
    //
    // Lease agent instance known by the remote partner node
    // It would be compared with local lease agent instance to check for stale message
    //
    LARGE_INTEGER MsgLeaseAgentInstance;

} LEASE_MESSAGE_EXT, *PLEASE_MESSAGE_EXT;

//
// Lease states.
//
typedef enum _ONE_WAY_LEASE_STATE {

    LEASE_STATE_INACTIVE,
    LEASE_STATE_ACTIVE,
    LEASE_STATE_EXPIRED,
    LEASE_STATE_FAILED,

} ONE_WAY_LEASE_STATE, *PONE_WAY_LEASE_STATE;

//
// Lease agent state.
//
typedef enum _LEASE_AGENT_STATE {

    OPEN,
    SUSPENDED,
    FAILED

} LEASE_AGENT_STATE, *PLEASE_AGENT_STATE;

//
// Lease agent data structure.
//
typedef struct _LEASE_AGENT_CONTEXT {

    //
    // Instance of this lease agent.
    //
    LARGE_INTEGER Instance;
    //
    // Transport used by this lease agent
    //
    PLEASE_TRANSPORT Transport;
    //
    // Address this agent listens on
    //
    TRANSPORT_LISTEN_ENDPOINT ListenAddress;
    //
    // Hash table concurrency control.
    //
    KSPIN_LOCK Lock;
    //
    // Remote lease agent context hash table.
    //
    RTL_GENERIC_TABLE RemoteLeaseAgentContextHashTable;
    //
    // Leasing application context hash table.
    //
    RTL_GENERIC_TABLE LeasingApplicationContextHashTable;
    //
    // Leasing application context unregister list.
    //
    RTL_GENERIC_TABLE LeasingApplicationContextUnregisterList;
    //
    // State of this lease agent.
    //
    LEASE_AGENT_STATE State;
    //
    // Indicates whether the maintenance worker will clean up this lease agent.
    //
    LARGE_INTEGER TimeToBeFailed;
    //
    // Lease Agent Configuration
    //
    LEASE_CONFIG_DURATIONS LeaseConfigDurations;
    //
    LONG LeaseSuspendDuration;
    LONG ArbitrationDuration;
    LONG SessionExpirationTimeout;
    //
    // Number of renew retries
    //
    LONG LeaseRetryCount;
    LONG LeaseRenewBeginRatio;
    //
    // Security
    //
    LONG SecurityType;
    BYTE CertHash[SHA1_LENGTH];
    _Null_terminated_ WCHAR CertHashStoreName[SCH_CRED_MAX_STORE_NAME_SIZE];
    PVOID RemoteCertVerifyCallback;
    //
    // Certificates that have positive verify result
    //
    RTL_GENERIC_TABLE TrustedCertificateCache;
    //
    // Certificates that have positive verify result
    //
    RTL_GENERIC_TABLE CertificatePendingList;
    //
    // Delayed termination timer routine
    // For lease failure, before sending out terminate all lease message
    // We need to delay for TTL to make sure the lease is actually expired from user process perspective
    //
    KTIMER DelayedLeaseFailureTimer;
    KDPC DelayedLeaseFailureTimerDpc;
    //
    // Set to true when set the OnLeaseFailed delay timer.
    // The flag can prevent the timer begin set twice, that can happen when there are multiple arbitration rejects from user mode
    // This flag will be set to true for both DelayLeaseFailure and DelayLeaseAgentClose stage.
    //
    BOOLEAN IsInDelayLeaseFailureLeaseAgentCloseTimer;
    //
    // leases renewed indirectly
    //
    LONG ConsecutiveIndirectLeaseLimit;
    //
    // Delayed termination timer routine
    // For lease failure, before sending out terminate all lease message
    // We need to delay for TTL to make sure the lease is actually expired from user process perspective
    //
    KTIMER DelayLeaseAgentCloseTimer;
    KDPC DelayLeaseAgentCloseTimerDpc;


} LEASE_AGENT_CONTEXT, *PLEASE_AGENT_CONTEXT;

//
// Lease Agent Constructor/Destructor.
//
NTSTATUS LeaseAgentConstructor(
    __in PTRANSPORT_LISTEN_ENDPOINT SocketAddress,
    __in PLEASE_CONFIG_DURATIONS LeaseDurations,
    __in LONG LeaseSuspendDuration,
    __in LONG ArbitrationDuration,
    __in LONG NumOfRetry,
    __in LONG StartOfRetry,
    __in LONG SecurityType,
    __in_bcount(SHA1_LENGTH)  PBYTE CertHash,
    __in LPCWSTR CertHashStoreName,
    __in LONG SessionExpirationTimeout,
    __out PLEASE_AGENT_CONTEXT * result
     );

VOID
LeaseAgentUninitialize(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in BOOLEAN IsDelayLeaseAgentClose
    );

VOID LeaseAgentDestructor(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext
    );

//
// Lease agent routines.
//
NTSTATUS LeaseAgentCreateListeningTransport(__in PLEASE_AGENT_CONTEXT LeaseAgentContext);

//
// Lease context information.
//
typedef struct _LEASE_RELATIONSHIP_CONTEXT {

    //
    // Subject information.
    //
    ONE_WAY_LEASE_STATE SubjectState;
    LARGE_INTEGER SubjectIdentifier;
    LARGE_INTEGER SubjectExpireTime;
    LARGE_INTEGER SubjectSuspendTime;
    LARGE_INTEGER SubjectFailTime;
    KTIMER SubjectTimer;
    KDPC SubjectTimerDpc;
    BOOL LeaseMessageSent;

    //
    // Monitor information.
    //
    ONE_WAY_LEASE_STATE MonitorState;
    LARGE_INTEGER MonitorIdentifier;
    LARGE_INTEGER MonitorExpireTime;
    KTIMER MonitorTimer;
    KDPC MonitorTimerDpc;

    //
    // Arbitration information.
    //
    KTIMER PreArbitrationSubjectTimer;
    KDPC PreArbitrationSubjectTimerDpc;
    KTIMER PreArbitrationMonitorTimer;
    KDPC PreArbitrationMonitorTimerDpc;
    KTIMER PostArbitrationTimer;
    KDPC PostArbitrationTimerDpc;

    //
    // Common subject and monitor information.
    //
    KTIMER RenewOrArbitrateTimer;
    KDPC RenewOrArbitrateTimerDpc;
    LONG Duration;
    LEASE_DURATION_TYPE EstablishDurationType;
    //
    // Set to true by UpdateDuration IOCTL call
    // Would be reset to false with a new renew request sent out
    //
    BOOLEAN IsDurationUpdated;
    LONG LeaseSuspendDuration;
    LONG ArbitrationDuration;

    // Durations requested by the lease partner; initialized to MAX
    LONG RemoteLeaseDuration;
    LONG RemoteSuspendDuration;
    LONG RemoteArbitrationDuration;

    BOOLEAN IsRenewRetry;
    LONG RenewRetryCount;

    LONG IndirectLeaseCount;

    //
    // Timer for ping retry.
    //
    KTIMER PingRetryTimer;
    KDPC PingRetryTimerDpc;

} LEASE_RELATIONSHIP_CONTEXT, *PLEASE_RELATIONSHIP_CONTEXT;

//
// Leasing application data structure.
//
typedef struct _LEASING_APPLICATION_CONTEXT {

    //
    // Identifier of this application for this lease agent.
    //
    LPWSTR LeasingApplicationIdentifier;
    //
    // Instance of this lease application.
    //
    HANDLE Instance;
    //
    // Count in bytes of the leasing application identifier.
    // Used in message serialization.
    //
    ULONG LeasingApplicationIdentifierByteCount;
    //
    // The process handle registering this application.
    // Used to detect application failure.
    //
    PEPROCESS ProcessHandle;
    //
    // Delayed request used for leasing application events.
    //
    WDFREQUEST DelayedRequest;
    //
    // Leasing application events.
    //
    RTL_GENERIC_TABLE LeasingApplicationEventQueue;
    //
    // Specifies if the leasing application is enabled for arbitration.
    //
    BOOLEAN IsArbitrationEnabled;
    //
    // Gloabl Lease Expire Time.
    //
    LARGE_INTEGER GlobalLeaseExpireTime;
    //
    // Delay unregister timer routine.
    //
    KTIMER UnregisterTimer;
    KDPC UnregisterTimerDpc;

    PLEASE_AGENT_CONTEXT LeaseAgentContext;
    //
    // Set to true when set the unregister timer; GetLeasingAPPTTL returns 0 when this flag is true.
    //
    BOOLEAN IsBeingUnregistered;
    //
    // leasing application expiry timeout defined in fed config
    //
    LONG LeasingApplicationExpiryTimeout;
    //
    // 
    LARGE_INTEGER LastGrantExpireTime;
    //
    // Process assert time.
    //
    LARGE_INTEGER ProcessAssertTime;

} LEASING_APPLICATION_CONTEXT, *PLEASING_APPLICATION_CONTEXT;

//
// Remote lease agent data structure. The lease layer will contain one of
// these structures for each remote machine it maintains a lease with.
//
typedef struct _REMOTE_LEASE_AGENT_CONTEXT {

    //
    // Lease agent context holding this remote lease agent.
    //
    PLEASE_AGENT_CONTEXT LeaseAgentContext;
    //
    // Instance of this remote lease agent.
    //
    LARGE_INTEGER Instance;
    //
    // Instance of this lease agent at the other end of the lease.
    // Required by arbitration.
    //
    LARGE_INTEGER RemoteLeaseAgentInstance;
    //
    // Remote lease agent identifier (tracing).
    //
    WCHAR RemoteLeaseAgentIdentifier[LEASE_AGENT_ID_CCH_MAX];
    //
    // The remote socket address of the remote lease agent.
    //
    TRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress;
    //
    // Subject list (contains PLEASE_RELATIONSHIP_IDENTIFIER).
    //
    RTL_GENERIC_TABLE SubjectHashTable;
    //
    // Subject failed pending list (contains PLEASE_RELATIONSHIP_IDENTIFIER).
    //
    RTL_GENERIC_TABLE SubjectFailedPendingHashTable;
    //
    // Subject terminate pending list (contains PLEASE_RELATIONSHIP_IDENTIFIER).
    //
    RTL_GENERIC_TABLE SubjectTerminatePendingHashTable;
    //
    // Subject establish pending list (contains PLEASE_RELATIONSHIP_IDENTIFIER).
    //
    RTL_GENERIC_TABLE SubjectEstablishPendingHashTable;
    //
    // Monitor list (contains PLEASE_RELATIONSHIP_IDENTIFIER).
    //
    RTL_GENERIC_TABLE MonitorHashTable;
    //
    // Monitor failed list (contains PLEASE_RELATIONSHIP_IDENTIFIER).
    //
    RTL_GENERIC_TABLE MonitorFailedPendingHashTable;
    //
    // Lease with remote lease agent.
    //
    PLEASE_RELATIONSHIP_CONTEXT LeaseRelationshipContext;
    //
    // Partner Channel context
    //
    PTRANSPORT_SENDTARGET PartnerTarget;
    //
    // State of this remote lease agent.
    //
    LEASE_AGENT_STATE State;
    //
    // Reference count used to safely deallocate this struct.
    //
    LONG RefCount;
    //
    // Event used for waiting for zero reference count.
    //
    KEVENT ZeroRefCountEvent;
    //
    // Indicates when the maintenance worker will clean up this lease agent.
    //
    LARGE_INTEGER TimeToBeFailed;
    //
    // If the arbitration result is neutral, set this flag to keep the remote lease agent from being destructing.
    //
    BOOLEAN IsInArbitrationNeutral;
    //
    // It is possible that there are multiple RLA in the hash table for one remote side lease agent
    // There should only be one active, the rest are obsolete and would be cleaned up during maintenance
    //
    BOOLEAN IsActive;
    //
    // Mark the flag when RLA sent out two way lease termination request
    //
    BOOLEAN IsInTwoWayTermination;
    //
    // In ping process
    //
    BOOLEAN InPing;
    //
    // Has renew ever happened before?
    //
    BOOLEAN RenewedBefore;
    //
    // The time ping request or ping response was sent
    //
    LARGE_INTEGER PingSendTime;
    //
    // The leasing application used for rasing arbitration request to user mode
    //
    PLEASING_APPLICATION_CONTEXT LeasingApplicationForArbitration;
    //
    // Lease protocol version of the remote side
    //
    USHORT RemoteVersion;
    //
    PVOID TransportBufferPending;
    //
    ULONG TransportBufferPendingSize;

} REMOTE_LEASE_AGENT_CONTEXT, *PREMOTE_LEASE_AGENT_CONTEXT;

//
// Lease relationship Constructor/Destructor.
//
NTSTATUS
LeaseRelationshipConstructor(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __out PLEASE_RELATIONSHIP_CONTEXT * result
    );

VOID
CancelRemoteLeaseAgentTimers(
__in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
);

VOID 
RemoteLeaseAgentUninitialize(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

VOID
LeaseRelationshipUninitialize(
    PLEASE_RELATIONSHIP_CONTEXT LeaseRelationshipContext
    );

VOID
LeaseRelationshipDestructor(
    PLEASE_RELATIONSHIP_CONTEXT Lease
    );

//
// Remote Lease Agent Constructor/Destructor.
//
NTSTATUS RemoteLeaseAgentConstructor(
    __in_opt PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress,
    __out PREMOTE_LEASE_AGENT_CONTEXT * result
    );

VOID
RemoteLeaseAgentDisconnect(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in BOOLEAN AbortConnections
    );

VOID
RemoteLeaseAgentDestructor(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

//
// Deallocation check routines for lease agent and remote lease agent.
//
BOOLEAN
IsRemoteLeaseAgentReadyForDeallocation(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in BOOLEAN WaitForZeroRefCount
    );

BOOLEAN
CanLeaseAgentBeFailed(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext
    );

BOOLEAN
IsLeaseAgentReadyForDeallocation(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext
    );

BOOLEAN
HasLeasingApplication(
__in PLEASE_AGENT_CONTEXT LeaseAgentContext
);

BOOLEAN
HasNonFailedRemoteLeaseAgent(
__in PLEASE_AGENT_CONTEXT LeaseAgentContext
);

PREMOTE_LEASE_AGENT_CONTEXT
GetActiveRemoteLeaseAgent(
__in PLEASE_AGENT_CONTEXT LeaseAgentContext
);

//
// Remote lease agent ref couting routines.
//
LONG
AddRef(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

LONG
Release(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

//
// Remote lease agent timer routines.
//
VOID
EnqueueTimer(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PKTIMER Timer,
    __in LARGE_INTEGER ExpireTime,
    __in PKDPC TimerDpc
    );

VOID
DequeueTimer(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PKTIMER Timer
    );

NTSTATUS
ProcessLeaseMessageBuffer(
    __in PLEASE_TRANSPORT Listener,
    __in PTRANSPORT_SENDTARGET Target,
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in PVOID Buffer,
    __in ULONG BufferSize
    );

//
// Leasing application Constructor/Destructor.
//
PLEASING_APPLICATION_CONTEXT
LeasingApplicationConstructor(
    __in LPCWSTR LeasingApplicationIdentifier,
    __in_opt PEPROCESS ProcessHandle
    );

VOID
LeasingApplicationDestructor(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    );

//
// Lease relationship identification.
//
typedef struct _LEASE_RELATIONSHIP_IDENTIFIER {

    //
    // Local leasing application part of this lease.
    //
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext;
    //
    // The remote leasing application used by this lease.
    //
    LPWSTR RemoteLeasingApplicationIdentifier;
    //
    // Count in bytes of the remote leasing application identifier.
    //
    ULONG RemoteLeasingApplicationIdentifierByteCount;

} LEASE_RELATIONSHIP_IDENTIFIER, *PLEASE_RELATIONSHIP_IDENTIFIER;

//
// Lease Relationship Constructor/Destructor.
//
PLEASE_RELATIONSHIP_IDENTIFIER
LeaseRelationshipIdentifierConstructor(
    __in LPCWSTR LeasingApplicationIdentifier,
    __in LPCWSTR RemoteLeasingApplicationIdentifier
    );

VOID LeaseRelationshipIdentifierDestructor(
    __in PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier
    );

VOID
SwitchLeaseRelationshipLeasingApplicationIdentifiers(
    __in PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier
    );

//
// Validation routines.
//
BOOLEAN
IsValidString(
    __in_ecount(StringWcharLengthIncludingNullTerminator) PWCHAR String,
    __in ULONG StringWcharLengthIncludingNullTerminator
    );

BOOLEAN
IsValidListenEndpoint(
    __in PTRANSPORT_LISTEN_ENDPOINT listenEndpoint
    );

NTSTATUS
IsValidMessageHeader(
    __in PVOID Buffer,
    __in ULONG BufferSize
    );

BOOLEAN
IsValidProcessHandle(
    __in HANDLE UserModeProcessHandle
    );

BOOLEAN
IsValidDuration(
    __in LONG Duration
    );

//
// Routines used to mark lease agents and remote lease agents as failed.
//
VOID
SetLeaseAgentFailed(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in BOOLEAN IsDelayLeaseAgentClose
    );

VOID
OnLeaseFailure(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext
    );

VOID
SetRemoteLeaseAgentState(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in LEASE_AGENT_STATE State
    );

BOOLEAN
IsRemoteLeaseAgentFailed(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

BOOLEAN
IsRemoteLeaseAgentSuspended(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

BOOLEAN
IsRemoteLeaseAgentOrphaned(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

BOOLEAN
IsRemoteLeaseAgentInPing(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

BOOLEAN
IsRemoteLeaseAgentOpen(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

BOOLEAN
IsRemoteLeaseAgentOpenTwoWay(
	__in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

BOOLEAN
IsLeaseAgentFailed(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext
    );

VOID
SetRemoteLeaseAgentFailed(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

//
// Message serialization/deserialization routines.
//
NTSTATUS
SerializeLeaseMessage(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in LEASE_MESSAGE_TYPE LeaseMessageType,
    __in_opt PRTL_GENERIC_TABLE SubjectEstablishPendingList,
    __in_opt PRTL_GENERIC_TABLE SubjectFailedPendingList,
    __in_opt PRTL_GENERIC_TABLE MonitorFailedPendingList,
    __in_opt PRTL_GENERIC_TABLE SubjectPendingAcceptedList,
    __in_opt PRTL_GENERIC_TABLE SubjectPendingRejectedList,
    __in_opt PRTL_GENERIC_TABLE SubjectFailedAcceptedList,
    __in_opt PRTL_GENERIC_TABLE MonitorFailedAcceptedList,
    __in_opt PRTL_GENERIC_TABLE SubjectTerminatePendingList,
    __in_opt PRTL_GENERIC_TABLE SubjectTerminateAcceptedList,
    __in LONG Duration,
    __in LARGE_INTEGER Expiration,
    __in LONG LeaseSuspendDuration,
    __in LONG ArbitrationDuration,
    __in BOOLEAN IsTwoWayTermination,
    __in PVOID* Buffer,
    __in PULONG BufferSize
    );

NTSTATUS
DeserializeLeaseMessage(
    __in PLEASE_MESSAGE_HEADER LeaseMessageHeader,
    __in PVOID Buffer,
    __in PRTL_GENERIC_TABLE SubjectPendingList,
    __in PRTL_GENERIC_TABLE SubjectFailedPendingList,
    __in PRTL_GENERIC_TABLE MonitorFailedPendingList,
    __in PRTL_GENERIC_TABLE SubjectPendingAcceptedList,
    __in PRTL_GENERIC_TABLE SubjectPendingRejectedList,
    __in PRTL_GENERIC_TABLE SubjectFailedAcceptedList,
    __in PRTL_GENERIC_TABLE MonitorFailedAcceptedList,
    __in PRTL_GENERIC_TABLE SubjectTerminatePendingList,
    __in PRTL_GENERIC_TABLE SubjectTerminateAcceptedList,
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteMessageSocketAddress,
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteLeaseSocketAddress,
    __in PLEASE_MESSAGE_EXT MessageExtension
    );

VOID
ClearRelationshipIdentifierList(
    __in PRTL_GENERIC_TABLE HashTable
    );

VOID
ClearLeasingApplicationList(
    __in PRTL_GENERIC_TABLE HashTable
    );

VOID CheckHangLeasingApplication(__in PRTL_GENERIC_TABLE HashTable);

VOID UpdateAssertLeasingApplication(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in PEPROCESS TerminatingProcessHandle,
    __in BOOLEAN IsProcessAssert);

PLEASE_RELATIONSHIP_IDENTIFIER
AddToLeaseRelationshipIdentifierList(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in LPCWSTR RemoteLeasingApplicationIdentifier,
    __in PRTL_GENERIC_TABLE HashTable
    );

//
// Lease related routines.
//

PREMOTE_LEASE_AGENT_CONTEXT
FindMostRecentRemoteLeaseAgent(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContextMatch
    );

VOID
EstablishLease(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

VOID
ArbitrateLease(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in LONG LocalTimeToLive,
    __in LONG RemoteTimeToLive,
    __in BOOLEAN IsFinal
    );

NTSTATUS
TerminateSubjectLease(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier,
    __in BOOLEAN IsSubjectFailed
    );

NTSTATUS
TerminateAllLeaseRelationships(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in BOOLEAN isSubjectFailed
    );

//
// Timer related routines.
//
VOID
ArmTimer(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PKTIMER Timer,
    __in LARGE_INTEGER ExpireTime,
    __in PKDPC TimerDpc
    );

VOID
SetSubjectExpireTime(
    __in PLEASE_RELATIONSHIP_CONTEXT LeaseRelationshipContext,
    __in LARGE_INTEGER Expiration,
    __in LONG LeaseSuspendDuration,
    __in LONG ArbitrationDuration
    );

VOID
SetRenewTimer(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in BOOLEAN ByRenewTimerDPC,
    __in LONG RenewDuration
    );

//
// Lease event buffer routines.
//
VOID
DelayedRequestCompletion(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in LEASE_LAYER_EVENT_TYPE EventType,
    __in_opt HANDLE LeaseHandle,
    __in HANDLE LeasingApplicationHandle,
    __in_opt LPCWSTR RemoteLeasingApplicationIdentifier,
    __in LONG MonitorTTL,
    __in LONG SubjectTTL,
    __in_opt PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress,
    __in LONGLONG LeaseAgentInstance,
    __in LONGLONG RemoteLeaseAgentInstance,
    __in NTSTATUS CompletionStatus,
    __in BOOLEAN IsHighPriority,
    __in LONG cbCertificate,
    __in_bcount(cbCertificate) PBYTE pbCertificate,
    __in PVOID remoteCertVerifyCompleteOp,
    __in LONG HealthReportReportCode,
    __in_opt LPCWSTR HealthReportDynamicProperty,
    __in_opt LPCWSTR HealthReportExtraDescription,
    __in USHORT remoteVersion,
    __in LONGLONG monitorLeaseInstance,
    __in LONGLONG subjectLeaseInstance,
    __in LONG remoteArbitrationDurationUpperBound
    );

VOID
TryDelayedRequestCompletion(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in LEASE_LAYER_EVENT_TYPE EventType,
    __in_opt HANDLE LeaseHandle,
    __in HANDLE LeasingApplicationHandle,
    __in_opt LPCWSTR RemoteLeasingApplicationIdentifier,
    __in LONG MonitorTTL,
    __in LONG SubjectTTL,
    __in_opt PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress,
    __in LONGLONG LeaseAgentInstance,
    __in LONGLONG RemoteLeaseAgentInstance,
    __in NTSTATUS CompletionStatus,
    __in BOOLEAN IsHighPriority,
    __in LONG cbCertificate,
    __in_bcount(cbCertificate) PBYTE pbCertificate,
    __in PVOID remoteCertVerifyCompleteOp,
    __in LONG HealthReportReportCode,
    __in_opt LPCWSTR HealthReportDynamicProperty,
    __in_opt LPCWSTR HealthReportExtraDescription,
    __in USHORT remoteVersion,
    __in LONGLONG monitorLeaseInstance,
    __in LONGLONG subjectLeaseInstance,
    __in LONG remoteArbitrationDurationUpperBound
    );

PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER
LeaseEventConstructor(
    );

VOID
LeaseEventDestructor(
    PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER LeaseEvent
    );

BOOLEAN
EnqueueLeasingApplicationLeaseEvent(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    );

BOOLEAN
EnqueueLeasingApplicationLocalExpirationAndArbitrationEvents(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    );

BOOLEAN
EnqueueLeasingApplicationRemoteExpirationEvent(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    );

BOOLEAN
EnqueueLeasingApplicationLeaseEstablishEvents(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    );

BOOLEAN
EnqueueLeasingApplicationAllLeaseEvents(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    );

VOID
DequeueLeasingApplicationLeaseEstablishEvent(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    );

VOID
TryDequeueLeasingApplicationLeaseEvent(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    );

//
// Tracing/Supportability.
//
const LPWSTR GetLeaseState(
    __in ONE_WAY_LEASE_STATE State
    );

const LPWSTR GetLeaseAgentState(
    __in LEASE_AGENT_STATE State
    );

const LPWSTR GetMessageType(
    __in LEASE_MESSAGE_TYPE MessageType
    );

const LPWSTR GetBlockMessageType(
    __in LEASE_BLOCKING_ACTION_TYPE BlockMessageType
    );

VOID GetCurrentTime(
    __out PLARGE_INTEGER CurrentTime
    );

NTSTATUS
UnregisterApplication(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in BOOLEAN isSubjectFailed
    );

VOID
MoveToUnregisterList(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    );

VOID
StartUnregisterApplication(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in LONG TTLInMilliseconds
    );

//
// Leasing application unregister timer routine
//
VOID
EnqueueUnregisterTimer(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in LONG TTLInMilliseconds
    );

VOID
DequeueUnregisterTimer(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    );

VOID
ArmUnregisterTimer(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in LONG TTLInMilliseconds
    );

//
// Lease agent delay timer routine
//
VOID
EnqueueDelayedLeaseFailureTimer(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in LONG TTLInMilliseconds
    );

VOID
ArmDelayedLeaseFailureTimer(
__in PLEASE_AGENT_CONTEXT LeaseAgentContext,
__in LONG TTLInMilliseconds
);

VOID
DequeueDelayedLeaseFailureTimer(
__in PLEASE_AGENT_CONTEXT LeaseAgentContext
);

//
// Delay lease agent close timer routine
//
VOID
EnqueueDelayLeaseAgentCloseTimer(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext
);

VOID
ArmDelayLeaseAgentCloseTimer(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext
);

VOID
DequeueDelayLeaseAgentCloseTimer(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext
);

VOID
SetLeaseRelationshipDuration(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in LONG Duration,
    __in LONG LeaseSuspendDuration,
    __in LONG ArbitrationDuration
    );

//
// Get a monotonically increasing unique value for instance
//
LARGE_INTEGER GetInstance( );

//
// update subject and renew timer when receive a termination message
//
VOID
UpdateSubjectTimerForTermination(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

//
// update monitor timer when receive a termination message
//
VOID
UpdateMonitorTimerForTermination(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

//
// Check if a remote lease agent is sending out failed termination request
//
BOOLEAN
SendingFailedTerminationRequest(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

//
// Remote certificate struct.
//
typedef struct _LEASE_REMOTE_CERTIFICATE
{
    //
    // The whole size in bytes.
    //
    ULONG cbCertificate;
    //
    // The whole size in bytes.
    //
    BYTE * pbCertificate;
    //
    // Verify operation handle
    //
    PVOID verifyOperation;
    //
    // Leasing application handle
    //
    PLEASING_APPLICATION_CONTEXT certLeasingAppHandle;

} LEASE_REMOTE_CERTIFICATE, *PLEASE_REMOTE_CERTIFICATE;

//
// CTor of remote certificate structure.
//
PLEASE_REMOTE_CERTIFICATE
LeaseRemoteCertificateConstructor(
    __in ULONG cbCert,
    __in PBYTE pbCert,
    __in PVOID verifyOp,
    __in PLEASING_APPLICATION_CONTEXT leasingAppHandle
    );

VOID
LeaseRemoteCertificateDestructor(
    __in PLEASE_REMOTE_CERTIFICATE remoteCert
    );

//
// CTor/DTor of listen endpoint structure.
//
PTRANSPORT_LISTEN_ENDPOINT
LeaseListenEndpointConstructor( );

VOID
LeaseListenEndpointDestructor( __in PTRANSPORT_LISTEN_ENDPOINT ListenEndPoint );

//
// Construcotr and destructor of lease message extension
PLEASE_MESSAGE_EXT
MessageExtensionConstructor( );

VOID
MessageExtensionDestructor( __in PLEASE_MESSAGE_EXT MsgExt );

//
// Delete all entries and deallocate the pCert memory
//
VOID
ClearCertificateList(
    __in PRTL_GENERIC_TABLE HashTable,
    __in BOOLEAN CompleteVerify,
    __in PLEASING_APPLICATION_CONTEXT unregisterAppHandle
    );

VOID
TerminateSubjectLeaseSendMessage(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in BOOLEAN IsTwoWayTermination
    );

VOID
EstablishReverseLease(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier
    );

NTSTATUS
TerminateReverseLease(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PRTL_GENERIC_TABLE SubjectTerminatePendingList
    );

VOID
EstablishLeaseSendPingRequestMessage(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

VOID
SendPingRequestMessage(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

VOID
AbortPingProcess(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

VOID
SendPingResponseMessage(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

__success(return == TRUE)
BOOLEAN UpdateLeaseAgentCertHash(
    PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in_bcount(SHA1_LENGTH) PBYTE CertHash,
    __in LPCWSTR CertHashStoreName,
    __out PULONG certStoreNameLength
    );

/*
NTSTATUS LeaseAgentAddressToStringW(
    PTRANSPORT_LISTEN_ENDPOINT ListenEndPoint,
    PWCHAR Buffer,
    ULONG BufferLength,
    BOOL usePort
    );
*/

NTSTATUS
DeserializeMessageListenEndPoint(
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress,
    __in PVOID Buffer,
    __in ULONG Size
    );


NTSTATUS
SendIndirectLeaseForwardMessages(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    );

PREMOTE_LEASE_AGENT_CONTEXT
GetCurrentActiveRemoteLeaseAgent(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress
    );

NTSTATUS
SerializeMessageListenEndPoint(
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress,
    __in_bcount(BufferSize) PBYTE StartBuffer,
    __in ULONG BufferSize
    );

NTSTATUS
ProcessForwardMessage(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in PVOID Buffer,
    __in ULONG BufferSize
    );

NTSTATUS
GetListenEndPointSize(
    __in PTRANSPORT_LISTEN_ENDPOINT ListenEndPoint,
    __in PULONGLONG ListenEndpointAddressSize,
    __in PULONG ListenEndpointSize
    );

NTSTATUS
DeserializeForwardMessage(
    __in PLEASE_MESSAGE_HEADER LeaseMessageHeader,
    __in PVOID Buffer,
    __in PTRANSPORT_LISTEN_ENDPOINT SourceSocketAddress,
    __in PTRANSPORT_LISTEN_ENDPOINT DestSocketAddress
    );

NTSTATUS
SerializeAndSendRelayMessages(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PLEASE_MESSAGE_HEADER LeaseMessageHeader,
    __in PTRANSPORT_LISTEN_ENDPOINT MsgSourceSocketAddress,
    __in PVOID ForwardMsgBuf,
    __in ULONG ForwardMsgSize
    );

BOOLEAN
IsReceivedResponse(__in LEASE_MESSAGE_TYPE LeaseMessageType);

BOOLEAN
IsReceivedRequest(__in LEASE_MESSAGE_TYPE LeaseMessageType);

BOOLEAN
IsSendingResponse(__in LEASE_MESSAGE_TYPE LeaseMessageType);

BOOLEAN
IsSendingRequest(__in LEASE_MESSAGE_TYPE LeaseMessageType);

BOOLEAN
IsIndirectLeaseMsg(__in LEASE_MESSAGE_TYPE LeaseMessageType);

LONG
GetConfigLeaseDuration(__in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext);

VOID
UpdateIndirectLeaseLimit(__in PLEASE_AGENT_CONTEXT LeaseAgentContext);

VOID
GetDurationsForRequest(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __inout PLONG PRequestLeaseDuration,
    __inout PLONG PRequestLeaseSuspendDuration,
    __inout PLONG PRequestArbitrationDuration,
    __in BOOLEAN UpdateLeaseRelationship
    );

BOOLEAN IsBlockTransport(
    PTRANSPORT_LISTEN_ENDPOINT from,
    PTRANSPORT_LISTEN_ENDPOINT to, 
    LEASE_BLOCKING_ACTION_TYPE blockingType);

USHORT GetLeaseAgentVersion(USHORT majorVersion, USHORT minorVersion);

//
// Transportation blocking descriptions (test used only)
//
typedef struct _TRANSPORT_BLOCKING_TEST_DESCRIPTOR {
    TRANSPORT_LISTEN_ENDPOINT LocalSocketAddress;
    BOOLEAN FromAny;
    TRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress;
    BOOLEAN ToAny;
    LEASE_BLOCKING_ACTION_TYPE BlockingType;
    LARGE_INTEGER Instance;
    WCHAR Alias[TEST_TRANSPORT_ALIAS_LENGTH];
    PEPROCESS Process;
} TRANSPORT_BLOCKING_TEST_DESCRIPTOR, *PTRANSPORT_BLOCKING_TEST_DESCRIPTOR;

    
VOID
GetLeasingApplicationTTL(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in LONG RequestTTL,
    __out PLONG TimeToLive
    );

void RemoveAllLeasingApplication();

VOID
CrashLeasingApplication(
    __in WDFREQUEST Request,
    __in size_t OutputBufferLength,
    __in size_t InputBufferLength
    );

VOID
UpdateLeaseGlobalConfig(
    __in WDFREQUEST Request,
    __in size_t OutputBufferLength,
    __in size_t InputBufferLength
);

const LPWSTR GetLeaseIoCode(
    __in ULONG IoControlCode
);

VOID DestructArbitrationNeutralRemoteLeaseAgents(__in PLEASE_AGENT_CONTEXT LeaseAgentContext);

VOID CheckHeartbeatUnresponsiveTime(PDRIVER_CONTEXT DriverContext, PREMOTE_LEASE_AGENT_CONTEXT ActiveRemoteLeaseAgentContext, LARGE_INTEGER Now);

VOID DoMaintenance(__in PDRIVER_CONTEXT DriverContext);

#endif  // _LEASLAYR_H_
