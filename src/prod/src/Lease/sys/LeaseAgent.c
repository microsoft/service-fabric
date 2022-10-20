// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "LeasLayr.h"
#include "LeaseAgent.tmh"

KDEFERRED_ROUTINE DelayedLeaseFailureTimer;

KDEFERRED_ROUTINE DelayLeaseAgentCloseTimer;

VOID
DelayedLeaseFailureTimer(
__in PKDPC Dpc,
__in_opt PVOID DeferredContext,
__in_opt PVOID SystemArgument1,
__in_opt PVOID SystemArgument2
)
/*++
Routine Description:
    Send failed termination to remote lease agents.

Parameters Description:
    Dpc - unregister timer Dpc routine.
    DeferredContext - Leasing application context.
    SystemArgument1 - n/a
    SystemArgument2 - n/a

--*/
{
    KIRQL OldIrql;

    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    PVOID IterLeasingApplication = NULL;

    PLEASE_AGENT_CONTEXT LeaseAgentContext = (PLEASE_AGENT_CONTEXT)DeferredContext;
    PVOID IterRemoteLeaseAgent = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = NULL;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    if (NULL == LeaseAgentContext)
    {
        return;
    }

    EventWriteDelayTimer(
        NULL,
        L" fired ",
        TransportIdentifier(LeaseAgentContext->Transport),
        LeaseAgentContext->Instance.QuadPart
        );

    KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

    // Fail all applications.
    //
    for (IterLeasingApplication = RtlEnumerateGenericTable(&LeaseAgentContext->LeasingApplicationContextHashTable, TRUE);
        NULL != IterLeasingApplication;
        IterLeasingApplication = RtlEnumerateGenericTable(&LeaseAgentContext->LeasingApplicationContextHashTable, FALSE))
    {
        // Retrieve leasing application.
        //
        LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);

        EventWriteLeaseAppFailed(
            NULL,
            LeasingApplicationContext->LeasingApplicationIdentifier,
            TransportIdentifier(LeaseAgentContext->Transport),
            LeaseAgentContext->Instance.QuadPart
            );

        //
        // Fire leasing application expired notification. This
        // may be queued instead of immediately completed.
        //
        TryDelayedRequestCompletion(
            LeasingApplicationContext,
            LEASING_APPLICATION_EXPIRED,
            NULL,
            LeasingApplicationContext->Instance,
            NULL,
            -1,
            -1,
            NULL,
            0,
            0,
            STATUS_SUCCESS,
            TRUE,
            0,
            NULL,
            NULL,
            0,
            NULL,
            NULL,
            0,
            0,
            0,
            0
            );

        // Terminate all leases related for this application.
        //
        for (IterRemoteLeaseAgent = RtlEnumerateGenericTable(&LeaseAgentContext->RemoteLeaseAgentContextHashTable, TRUE);
            NULL != IterRemoteLeaseAgent;
            IterRemoteLeaseAgent = RtlEnumerateGenericTable(&LeaseAgentContext->RemoteLeaseAgentContextHashTable, FALSE))
        {
            RemoteLeaseAgentContext = *((PREMOTE_LEASE_AGENT_CONTEXT*)IterRemoteLeaseAgent);

            TerminateAllLeaseRelationships(
                RemoteLeaseAgentContext,
                LeasingApplicationContext,
                TRUE
                );
        }
    }

    SetLeaseAgentFailed(LeaseAgentContext, TRUE);

    KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);

}

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
    )

/*++

Routine Description:

    Constructs a new lease agent structure.

Arguments:

    SocketAddress - Device IOCTL Input buffer.

Return Value:

    NULL if lease agent structure could not be allocated and/or initialized.

--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());
    ULONG certStoreNameLength = 0;
    ULONG listenEndPointSize = sizeof(TRANSPORT_LISTEN_ENDPOINT);
    ULONG configDurationsSize = sizeof(LEASE_CONFIG_DURATIONS);

    PLEASE_AGENT_CONTEXT LeaseAgentContext = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(LEASE_AGENT_CONTEXT),
        LEASELAYER_POOL_TAG
        );

    *result = LeaseAgentContext;
    if (NULL == LeaseAgentContext) {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        EventWriteAllocateFail17(
                NULL
                ); 
        goto Error;
    }

    RtlZeroMemory(LeaseAgentContext, sizeof(LEASE_AGENT_CONTEXT));
    if (memcpy_s(&LeaseAgentContext->ListenAddress, listenEndPointSize, SocketAddress, listenEndPointSize))
    {
        goto Error;
    }

    // Initialize the lease config durations in lease agent
    if (LeaseDurations != NULL)
    {
        if(memcpy_s(&LeaseAgentContext->LeaseConfigDurations, configDurationsSize, LeaseDurations, configDurationsSize))
        {
            goto Error;
        }
    }

    //
    // Set certificate hash related
    //
    LeaseAgentContext->SecurityType = SecurityType;
    LeaseAgentContext->SessionExpirationTimeout = SessionExpirationTimeout;

    if (IsSecurityProviderSsl(SecurityType))
    {
        if (!UpdateLeaseAgentCertHash(LeaseAgentContext, CertHash, CertHashStoreName, &certStoreNameLength))
        {
            goto Error;
        }
    }

    //
    // Create Transport Object
    //

    Status = TransportCreate(
        DriverContext->DriverTransport,
        SocketAddress,
        LeaseAgentContext->SecurityType,
        LeaseAgentContext->CertHash,
        LeaseAgentContext->CertHashStoreName,
        certStoreNameLength,
        LeaseAgentContext->SessionExpirationTimeout,
        (PVOID)LeaseAgentContext,
        &LeaseAgentContext->Transport);

    if (!NT_SUCCESS(Status)) 
    {
        EventWriteTransportCreateFailed(NULL, Status);

        goto Error;
    }

    //
    // Initialize hash table spin locks.
    //
    KeInitializeSpinLock(&LeaseAgentContext->Lock);

    //
    // Initialize hash tables.
    //
    RtlInitializeGenericTable(
        &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareRemoteLeaseAgentContexts,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL);
    RtlInitializeGenericTable(
        &LeaseAgentContext->LeasingApplicationContextHashTable,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareLeasingApplicationContexts,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL);
    RtlInitializeGenericTable(
        &LeaseAgentContext->LeasingApplicationContextUnregisterList,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareLeasingApplicationContexts,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL);
    RtlInitializeGenericTable(
        &LeaseAgentContext->TrustedCertificateCache,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareCertificates,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL);
    RtlInitializeGenericTable(
        &LeaseAgentContext->CertificatePendingList,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareLeaseRemoteCertOp,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL);

    //
    // Set the instance id.
    //
    LeaseAgentContext->Instance = GetInstance();

    EventWriteLeaseAgentCreated(
        NULL, 
        TransportIdentifier(LeaseAgentContext->Transport),
        LeaseAgentContext->Instance.QuadPart,
        GetLeaseAgentState(LeaseAgentContext->State)
        ); 

    LeaseAgentContext->LeaseSuspendDuration = LeaseSuspendDuration;
    LeaseAgentContext->ArbitrationDuration = ArbitrationDuration;

    LeaseAgentContext->LeaseRetryCount = NumOfRetry;
    LeaseAgentContext->LeaseRenewBeginRatio = StartOfRetry;

    UpdateIndirectLeaseLimit(LeaseAgentContext);
    //
    // Initialize delayed termination timer.
    //
    KeInitializeTimer(&LeaseAgentContext->DelayedLeaseFailureTimer);
    //
    // Initialize the delayed termination timer deferred procedure call.
    //
    KeInitializeDpc(
        &LeaseAgentContext->DelayedLeaseFailureTimerDpc,
        DelayedLeaseFailureTimer,
        LeaseAgentContext
        );

    //
    // Initialize delay lease agent close timer.
    //
    KeInitializeTimer(&LeaseAgentContext->DelayLeaseAgentCloseTimer);
    //
    // Initialize the delay lease agent close timer deferred procedure call.
    //
    KeInitializeDpc(
        &LeaseAgentContext->DelayLeaseAgentCloseTimerDpc,
        DelayLeaseAgentCloseTimer,
        LeaseAgentContext
        );

    LeaseAgentContext->IsInDelayLeaseFailureLeaseAgentCloseTimer = FALSE;

    return STATUS_SUCCESS;

Error:

    //
    // Perform cleanup if any.
    //
    if (NULL != LeaseAgentContext) 
    {
        if (NULL != LeaseAgentContext->Transport) 
        {
            TransportAbort(LeaseAgentContext->Transport);
            TransportRelease(LeaseAgentContext->Transport);
        }

        ExFreePool(LeaseAgentContext);
        *result = NULL;
    }

    return Status;
}

void LeaseAgentMessageCallback(PLEASE_TRANSPORT Listner, PTRANSPORT_SENDTARGET Target, PVOID Buffer, ULONG BufferSize, PVOID State)
{
    NTSTATUS Status;

    Status = ProcessLeaseMessageBuffer(Listner, Target, (PLEASE_AGENT_CONTEXT)State, Buffer, BufferSize);

    if (!NT_SUCCESS(Status))
    {
        TransportAbortConnections(Target);
    }
}

void HealthReportCallback(int healthReportReportCode, LPCWSTR healthReportDynamicProperty, LPCWSTR healthReportExtraDescription, PVOID state)
{
    PLEASE_AGENT_CONTEXT LeaseAgentContext = (PLEASE_AGENT_CONTEXT)state;
    PVOID IterLeasingApplication = NULL;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    KIRQL OldIrql;

    EventWriteHealthReport(
        NULL,
        healthReportReportCode,
        healthReportDynamicProperty,
        healthReportExtraDescription
    );

    KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

    //
    // Retrieve the first application
    //
    IterLeasingApplication = RtlEnumerateGenericTable(
        &LeaseAgentContext->LeasingApplicationContextHashTable,
        TRUE
    );

    if (IterLeasingApplication != NULL)
    {
        LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);
    }
    else
    {
        EventWriteHealthReportLeasingApplicationNotFound(
            NULL,
            healthReportReportCode,
            healthReportDynamicProperty,
            healthReportExtraDescription
        );
         
        goto Done;
    }

    TryDelayedRequestCompletion(
        LeasingApplicationContext,
        HEALTH_REPORT,
        NULL,
        LeasingApplicationContext->Instance,
        NULL,
        -1,
        -1,
        NULL,
        0,
        0,
        STATUS_SUCCESS,
        TRUE,
        0,
        NULL,
        NULL,
        healthReportReportCode,
        healthReportDynamicProperty,
        healthReportExtraDescription,
        0,
        0,
        0,
        0
        );

Done:

    KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);
}

/*
    Check to see if we want to block this message (test only)
*/
BOOLEAN IsBlockTransport(PTRANSPORT_LISTEN_ENDPOINT from, PTRANSPORT_LISTEN_ENDPOINT to, LEASE_BLOCKING_ACTION_TYPE blockingType)
{
    PVOID IterTransportBlockingBehavior = NULL;
    PTRANSPORT_BLOCKING_TEST_DESCRIPTOR descriptor = NULL;
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());
    KIRQL OldIrql;

    BOOLEAN toBlock = FALSE;

    KeAcquireSpinLock(&DriverContext->TransportBehaviorTableAccessSpinLock, &OldIrql);

    IterTransportBlockingBehavior = RtlEnumerateGenericTable(
        &DriverContext->TransportBehaviorTable,
        TRUE
        );

    while (NULL != IterTransportBlockingBehavior) {

        descriptor = *((PTRANSPORT_BLOCKING_TEST_DESCRIPTOR*)IterTransportBlockingBehavior);
        BOOLEAN fromAddressMatch = (from == NULL) || descriptor->FromAny || (wcscmp(descriptor->LocalSocketAddress.Address, from->Address) == 0
            &&
            descriptor->LocalSocketAddress.Port == from->Port);
        BOOLEAN toAddressMatch = (to == NULL) || descriptor->ToAny ||
            (wcscmp(descriptor->RemoteSocketAddress.Address, to->Address) == 0
            &&
            descriptor->RemoteSocketAddress.Port == to->Port);
        BOOLEAN addressMatch = fromAddressMatch && toAddressMatch;
        BOOLEAN messageTypeMatch = descriptor->BlockingType == blockingType || descriptor->BlockingType == LEASE_BLOCKING_ALL;

        if (messageTypeMatch && addressMatch)
        {
            toBlock = TRUE;
            break;
        }

        IterTransportBlockingBehavior = RtlEnumerateGenericTable(
            &DriverContext->TransportBehaviorTable,
            FALSE
            );
    }

    KeReleaseSpinLock(&DriverContext->TransportBehaviorTableAccessSpinLock, OldIrql);

    if (toBlock)
    {
        EventWriteTransportMessageBlocked(
            NULL,
            from->Address,
            from->Port,
            to->Address,
            to->Port,
            GetBlockMessageType(blockingType));
    }

    return toBlock;
}

/*
    Initializes a listening transport in a lease agent
    
*/
NTSTATUS LeaseAgentCreateListeningTransport(__in PLEASE_AGENT_CONTEXT LeaseAgentContext)
{
    return TransportListen(LeaseAgentContext->Transport, LeaseAgentMessageCallback, HealthReportCallback, LeaseAgentContext);
}

VOID 
LeaseAgentUninitialize(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in BOOLEAN IsDelayLeaseAgentClose
    )

/*++

Routine Description:

    Uninitializes a lease agent.

Arguments:

    LeaseAgent - lease agent structure to uninitialize.

Return Value:

    n/a

--*/

{
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = NULL;
    PVOID Iter = NULL;

    if (IsLeaseAgentFailed(LeaseAgentContext)) {

        //
        // We have been here before.
        //
        return;
    }
    
    // 
    // Close listening socket.
    //
    if (IsDelayLeaseAgentClose == TRUE)
    {
        //
        // Since Transport is not close, maintenance thread will not destruct lease agent.
        // We do have such detection in IsLeaseAgentReadyForDeallocation.
        //
        ArmDelayLeaseAgentCloseTimer(
            LeaseAgentContext
            );
    }
    else
    {
        TransportClose(LeaseAgentContext->Transport);
    }

    //
    // Uninitialize each remote lease agent from hash table.
    //
    for (Iter = RtlEnumerateGenericTable(
            &LeaseAgentContext->RemoteLeaseAgentContextHashTable, 
            TRUE);
         NULL != Iter;
         Iter = RtlEnumerateGenericTable(
            &LeaseAgentContext->RemoteLeaseAgentContextHashTable, 
            FALSE)) {
        
        //
        // Retrieve remote lease agent context.
        //
        RemoteLeaseAgentContext = *((PREMOTE_LEASE_AGENT_CONTEXT*)Iter);

        //
        // Uninitialize the remote lease agent context.
        //
        RemoteLeaseAgentUninitialize(RemoteLeaseAgentContext);
    }

    //
    // Mark lease agent as failed.
    //
    LeaseAgentContext->State = FAILED;

    EventWriteLeaseAgentInStateForRemote(
        NULL,
        TransportIdentifier(LeaseAgentContext->Transport),
        LeaseAgentContext->Instance.QuadPart,
        GetLeaseAgentState(LeaseAgentContext->State)
        ); 
}

VOID 
LeaseAgentDestructor(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext
    )

/*++

Routine Description:

    Deallocates a lease agent.

Arguments:

    LeaseAgent - lease agent structure to deallocate.

Return Value:

    Nothing.

--*/

{
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = NULL;
    PVOID Iter = NULL;
    BOOLEAN Deleted = FALSE;

    // When security is enabled, this function will call SSPI SSPI FreeCredentialHandle, which is pagable code
    LAssert(KeGetCurrentIrql() <= APC_LEVEL);

    EventWriteLeaseAgentDelete(
        NULL, 
        TransportIdentifier(LeaseAgentContext->Transport),
        LeaseAgentContext->Instance.QuadPart
        ); 

    //
    // Remove and delete each remote lease agent from hash table.
    //
    Iter = RtlEnumerateGenericTable(
        &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
        TRUE
        );
    
    while (NULL != Iter) {
        
        //
        // Retrieve remote lease agent context.
        //
        RemoteLeaseAgentContext = *((PREMOTE_LEASE_AGENT_CONTEXT*)Iter);

        //
        // Remove the remote lease agent from the hash table.
        //
        Deleted = RtlDeleteElementGenericTable(
            &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
            &RemoteLeaseAgentContext
            );
        LAssert(Deleted);

        //
        // Deallocate the remote lease agent context.
        //
        RemoteLeaseAgentDestructor(RemoteLeaseAgentContext);

        //
        // Move on to the next element.
        //
        Iter = RtlEnumerateGenericTable(
            &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
            TRUE
            );
    }

    //
    // Clear leasing application list.
    //
    ClearLeasingApplicationList(&LeaseAgentContext->LeasingApplicationContextHashTable);
    //
    // Clear unregistered leasing application list.
    //
    ClearLeasingApplicationList(&LeaseAgentContext->LeasingApplicationContextUnregisterList);

    //
    // Clear certificates related list
    // BOOLEAN indicate if to complete verify callback
    //
    ClearCertificateList(&LeaseAgentContext->CertificatePendingList, TRUE, NULL);
    ClearCertificateList(&LeaseAgentContext->TrustedCertificateCache, FALSE, NULL);
    // 
    // Deallocate transport
    //
    TransportAbort(LeaseAgentContext->Transport);
    TransportRelease(LeaseAgentContext->Transport);

    if (TRUE == LeaseAgentContext->IsInDelayLeaseFailureLeaseAgentCloseTimer)
    {
        DequeueDelayedLeaseFailureTimer(LeaseAgentContext);
    }

    ExFreePool(LeaseAgentContext);
}

VOID
DelayLeaseAgentCloseTimer(
    __in PKDPC Dpc,
    __in_opt PVOID DeferredContext,
    __in_opt PVOID SystemArgument1,
    __in_opt PVOID SystemArgument2
)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    KIRQL OldIrql;
    PLEASE_AGENT_CONTEXT LeaseAgentContext = (PLEASE_AGENT_CONTEXT)DeferredContext;

    EventWriteDelayLeaseAgentCloseTimerCall(
        NULL,
        TransportIdentifier(LeaseAgentContext->Transport),
        LeaseAgentContext->Instance.QuadPart
        );

    KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

    //
    // Close listening socket.
    //
    TransportClose(LeaseAgentContext->Transport);

    LeaseAgentContext->IsInDelayLeaseFailureLeaseAgentCloseTimer = FALSE;

    KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);
}

NTSTATUS RemoteLeaseAgentConstructor(
    __in_opt PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress,
    __out PREMOTE_LEASE_AGENT_CONTEXT * result
    )
    
/*++

Routine Description:

    Constructs a new lease agent structure.

Arguments:

    LeaseAgent - lease agent that holds this remote lease agent.
    
    SocketContext - Socket information associated with this remote lease agent.

Return Value:

    NULL if remote lease agent structure could not be allocated and/or initialized.

--*/

{
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(REMOTE_LEASE_AGENT_CONTEXT),
        LEASE_REMOTE_LEASE_AGENT_CTXT_TAG
        );

    NTSTATUS status = STATUS_UNSUCCESSFUL;

    *result = RemoteLeaseAgentContext;
    if (NULL == RemoteLeaseAgentContext) {

        EventWriteAllocateFail18(
                        NULL
                        );
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Error;
    }

    RtlZeroMemory(RemoteLeaseAgentContext, sizeof(REMOTE_LEASE_AGENT_CONTEXT));

    //
    // Construct lease socket context identifier.
    //
    status = ListenEndpointToWString(
        RemoteSocketAddress, 
        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier, 
        sizeof(RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier));

    if (!NT_SUCCESS(status))
    {
        goto Error;
    }

    //
    // Store the address of the remote lease agent.
    //
    if (memcpy_s(
        &RemoteLeaseAgentContext->RemoteSocketAddress,
        sizeof(TRANSPORT_LISTEN_ENDPOINT),
        RemoteSocketAddress,
        sizeof(TRANSPORT_LISTEN_ENDPOINT)
        ))
    {
        goto Error;
    }

    //
    // Construct lease context.
    //
    status = LeaseRelationshipConstructor(RemoteLeaseAgentContext, &RemoteLeaseAgentContext->LeaseRelationshipContext);
    if (!NT_SUCCESS(status)) 
    {
        goto Error;
    }

    //
    // Initialize hash tables.
    //
    RtlInitializeGenericTable(
        &RemoteLeaseAgentContext->SubjectHashTable,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareLeaseRelationshipContexts,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL
        );
    RtlInitializeGenericTable(
        &RemoteLeaseAgentContext->SubjectEstablishPendingHashTable,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareLeaseRelationshipContexts,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL
        );
    RtlInitializeGenericTable(
        &RemoteLeaseAgentContext->SubjectFailedPendingHashTable,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareLeaseRelationshipContexts,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL
        );
    RtlInitializeGenericTable(
        &RemoteLeaseAgentContext->MonitorHashTable,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareLeaseRelationshipContexts,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL
        );
    RtlInitializeGenericTable(
        &RemoteLeaseAgentContext->MonitorFailedPendingHashTable,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareLeaseRelationshipContexts,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL
        );
    RtlInitializeGenericTable(
        &RemoteLeaseAgentContext->SubjectTerminatePendingHashTable,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareLeaseRelationshipContexts,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL
        );

    //
    // Set the instance id.
    //
    RemoteLeaseAgentContext->Instance = GetInstance();

    //
    // Initialize the ref count zero event.
    //
    KeInitializeEvent(
        &RemoteLeaseAgentContext->ZeroRefCountEvent,
        NotificationEvent,
        TRUE
        );

    //
    // Set its local lease agent context.
    //
    RemoteLeaseAgentContext->LeaseAgentContext = LeaseAgentContext;

    RemoteLeaseAgentContext->IsActive = TRUE;

    RemoteLeaseAgentContext->IsInTwoWayTermination = FALSE;

    RemoteLeaseAgentContext->IsInArbitrationNeutral = FALSE;

    RemoteLeaseAgentContext->InPing = FALSE;

    RemoteLeaseAgentContext->RenewedBefore = FALSE;

    RemoteLeaseAgentContext->TransportBufferPending = NULL;

    RemoteLeaseAgentContext->TransportBufferPendingSize = 0;

    EventWriteRemoteLeaseAgentCreated(
        NULL,
        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
        RemoteLeaseAgentContext->Instance.QuadPart,
        GetLeaseAgentState(RemoteLeaseAgentContext->State)
        ); 
    return STATUS_SUCCESS; 

Error:

    //
    // Perform cleanup if any.
    //
    if (NULL != RemoteLeaseAgentContext) {
        
        if (NULL != RemoteLeaseAgentContext->LeaseRelationshipContext) {

            LeaseRelationshipDestructor(RemoteLeaseAgentContext->LeaseRelationshipContext);
        }        
        
        ExFreePool(RemoteLeaseAgentContext);
        *result = NULL;
    }
    
    return status;
}

VOID 
CancelRemoteLeaseAgentTimers(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
)

/*++

Routine Description:

    Cancel all registered timers for remote lease agent.

Arguments:

    RemoteLeaseAgentContext - remote lease agent structure to uninitialize.

Return Value:

n/a

--*/

{
    DequeueTimer(
        RemoteLeaseAgentContext,
        &RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectTimer
        );
    DequeueTimer(
        RemoteLeaseAgentContext,
        &RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorTimer
        );
    DequeueTimer(
        RemoteLeaseAgentContext,
        &RemoteLeaseAgentContext->LeaseRelationshipContext->RenewOrArbitrateTimer
        );
    DequeueTimer(
        RemoteLeaseAgentContext,
        &RemoteLeaseAgentContext->LeaseRelationshipContext->PreArbitrationSubjectTimer
        );
    DequeueTimer(
        RemoteLeaseAgentContext,
        &RemoteLeaseAgentContext->LeaseRelationshipContext->PreArbitrationMonitorTimer
        );
    DequeueTimer(
        RemoteLeaseAgentContext,
        &RemoteLeaseAgentContext->LeaseRelationshipContext->PostArbitrationTimer
        );
    DequeueTimer(
        RemoteLeaseAgentContext,
        &RemoteLeaseAgentContext->LeaseRelationshipContext->PingRetryTimer
        );
}

VOID 
RemoteLeaseAgentUninitialize(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    )

/*++

Routine Description:

    Uninitializes a remote lease agent.

Arguments:

    RemoteLeaseAgentContext - remote lease agent structure to uninitialize.

Return Value:

    n/a

--*/

{
    if (IsRemoteLeaseAgentFailed(RemoteLeaseAgentContext)) {

        //
        // We have been here before.
        //
        return;
    }

    //
    // Cancel all timers.
    //
    CancelRemoteLeaseAgentTimers(RemoteLeaseAgentContext);

    //
    // Set the state of the remote agent to failed.
    //
    SetRemoteLeaseAgentState(RemoteLeaseAgentContext, FAILED);
}

VOID
RemoteLeaseAgentDisconnect(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in BOOLEAN AbortConnections
    )
{
    SetRemoteLeaseAgentFailed(RemoteLeaseAgentContext);

    if (RemoteLeaseAgentContext->PartnerTarget != NULL) 
    {
        if (TRUE == AbortConnections)
        {
            TransportAbortConnections(RemoteLeaseAgentContext->PartnerTarget);
        }

        TransportForgetSendNotifications(RemoteLeaseAgentContext->PartnerTarget, RemoteLeaseAgentContext);
        TransportReleaseTarget(RemoteLeaseAgentContext->PartnerTarget);
        RemoteLeaseAgentContext->PartnerTarget = NULL;
    }
}

VOID 
RemoteLeaseAgentDestructor(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    )

/*++

Routine Description:

    Deallocates a remote lease agent.

Arguments:

    RemoteLeaseAgentContext - remote lease agent structure to deallocate.

Return Value:

    n/a

--*/

{
    EventWriteRemoteLeaseAgentDelete(
        NULL,
        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
        RemoteLeaseAgentContext->Instance.QuadPart
        ); 

    LAssert(0 == RemoteLeaseAgentContext->RefCount);

    //
    // Deref send target
    //
    RemoteLeaseAgentDisconnect(RemoteLeaseAgentContext, TRUE);

    //
    // Clear lists. 
    //
    ClearRelationshipIdentifierList(&RemoteLeaseAgentContext->SubjectHashTable);
    ClearRelationshipIdentifierList(&RemoteLeaseAgentContext->SubjectFailedPendingHashTable);
    ClearRelationshipIdentifierList(&RemoteLeaseAgentContext->SubjectEstablishPendingHashTable);
    ClearRelationshipIdentifierList(&RemoteLeaseAgentContext->MonitorHashTable);
    ClearRelationshipIdentifierList(&RemoteLeaseAgentContext->MonitorFailedPendingHashTable);
    ClearRelationshipIdentifierList(&RemoteLeaseAgentContext->SubjectTerminatePendingHashTable);

    if (RemoteLeaseAgentContext->TransportBufferPending)
    {
        ExFreePool(RemoteLeaseAgentContext->TransportBufferPending);
    }
    
    //
    // Perform cleanup.
    //

    if (RemoteLeaseAgentContext->RefCount != 0)
    {
        EventWriteLeaseDriverTextTraceError(
            NULL,
            L"RLA ref count is non-zero: ",
            RemoteLeaseAgentContext->RefCount
            );
    }

    LeaseRelationshipDestructor(RemoteLeaseAgentContext->LeaseRelationshipContext);
    ExFreePool(RemoteLeaseAgentContext);
}

LONG 
AddRef(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    )

/*++

Routine Description:

    Increments the ref count of a remote lease agent.

Arguments:

    RemoteLeaseAgentContext - remote lease agent.

Return Value:

    The method returns the new reference count.

--*/

{
    LONG RefCount = InterlockedIncrement(&RemoteLeaseAgentContext->RefCount);
    LAssert(!IsRemoteLeaseAgentFailed(RemoteLeaseAgentContext));

    KeClearEvent(&RemoteLeaseAgentContext->ZeroRefCountEvent);

    return RefCount;
}

LONG
Release(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    )

/*++

Routine Description:

    Decrements the ref count of a remote lease agent.

Arguments:

    RemoteLeaseAgentContext - remote lease agent.

Return Value:

    The method returns the new reference count.

--*/

{
    LONG RefCount = InterlockedDecrement(&RemoteLeaseAgentContext->RefCount);
    LAssert(0 <= RefCount);

    if (0 == RefCount) {

        KeSetEvent(
            &RemoteLeaseAgentContext->ZeroRefCountEvent, 
            0, 
            FALSE
            );
    }

    return RefCount;
}

BOOLEAN 
IsRemoteLeaseAgentReadyForDeallocation(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in BOOLEAN WaitForZeroRefCount
    )

/*++

Routine Description:

    Checks to see if a remote lease agent could be deallocated.

Arguments:

    RemoteLeaseAgentContext - remote lease agent.

    WaitForZeroRefCount - specifies if the check for ref count
        zero is a soft one or a hard one.

Return Value:

    TRUE if the remote lease agent reference count is zero and
    its channels are closed.

--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    LARGE_INTEGER Timeout = {0, 0};
    
    LAssert(IsRemoteLeaseAgentFailed(RemoteLeaseAgentContext));

    EventWriteWaitingOnRemoteLeaseAgent(
        NULL,
        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
        RemoteLeaseAgentContext->Instance.QuadPart,
        GetLeaseAgentState(RemoteLeaseAgentContext->State),
        RemoteLeaseAgentContext->RefCount
        );

    //
    // Check if the ref count is zero.
    //
    Status = KeWaitForSingleObject(
        &RemoteLeaseAgentContext->ZeroRefCountEvent,
        Executive,
        KernelMode,
        FALSE,
        WaitForZeroRefCount ? NULL : &Timeout
        );
        
    if (STATUS_SUCCESS == Status)
    {
        EventWriteRemoteLeaseAgentDeallocReady(
            NULL,
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart
            );
    }
    else
    {
        EventWriteRemoteLeaseAgentDeallocNotReady(
            NULL,
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart,
            RemoteLeaseAgentContext->RefCount
            );
    }

    return (STATUS_SUCCESS == Status);
}

VOID
ProcessSubjectTerminateFailedPendingList(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PRTL_GENERIC_TABLE SubjectTerminateFailedPendingList,
    __in BOOLEAN IsSubjectFailedList,
    __inout PRTL_GENERIC_TABLE OutgoingSubjectTerminateFailedAcceptedList
    )
    
/*++

Routine Description:

    Processes the subject failed pending list.

Arguments:

    RemoteLeaseAgentContext - remote lease agent structure to process the list.

    SubjectFailedPendingList - subject failed pending list.

    IsSubjectFailedList - is this a failed list of a temrinate list?

    OutgoingSubjectFailedAcceptedList - subject failed accepted output list.

Return Value:

    n/a.

--*/

{
    PVOID Iter = NULL;
    PVOID IterLeasingApplication = NULL;
    PVOID IterMonitor = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifierMonitor = NULL;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    BOOLEAN Inserted = TRUE;
    BOOLEAN Deleted = FALSE;

    //
    // Iterate over subject failed pending list.
    //
    Iter = RtlEnumerateGenericTable(SubjectTerminateFailedPendingList, TRUE);
        
    while (NULL != Iter) {

        //
        // Retrieve lease relationship identifier from the failed pending list.
        //
        LeaseRelationshipIdentifier = *((PLEASE_RELATIONSHIP_IDENTIFIER*)Iter);

        //
        // Remove the lease relationship identifier from the subject failed pending list.
        //
        Deleted = RtlDeleteElementGenericTable(
            SubjectTerminateFailedPendingList,
            &LeaseRelationshipIdentifier
            );
        LAssert(Deleted);

        //
        // Remove from the monitor list all elements from the subject failed accepted list.
        //
        IterMonitor = RtlLookupElementGenericTable(
            &RemoteLeaseAgentContext->MonitorHashTable,
            &LeaseRelationshipIdentifier
            );

        //
        // Check to see if the monitor is still around. It could happen that
        // the monitor has already been removed from a previous message that
        // arrived before this one.
        //
        if (NULL != IterMonitor) {

            //
            // Retrieve lease relationship identifier from the failed pending list.
            //
            LeaseRelationshipIdentifierMonitor = *((PLEASE_RELATIONSHIP_IDENTIFIER*)IterMonitor);

            //
            // Check to see if we need to invoke the callback.
            //
            if (IsSubjectFailedList)
            {
                //
                // Switch temporarily the leasing application identifier
                // with the remote leasing application identifier.
                //
                SwitchLeaseRelationshipLeasingApplicationIdentifiers(LeaseRelationshipIdentifierMonitor);
                
                //
                // Check to see if the leasing application is registered.
                //
                IterLeasingApplication = RtlLookupElementGenericTable(
                    &RemoteLeaseAgentContext->LeaseAgentContext->LeasingApplicationContextHashTable,
                    &LeaseRelationshipIdentifierMonitor->LeasingApplicationContext
                    );
                
                //
                // Switch back the leasing application identifier and the 
                // remote leasing application identifier to initial values.
                //
                SwitchLeaseRelationshipLeasingApplicationIdentifiers(LeaseRelationshipIdentifierMonitor);

                if (NULL != IterLeasingApplication)
                {
                    //
                    // Retrieve lease relationship identifier from the subject pending list.
                    //
                    LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);

                    //
                    // Fire remote leasing application failed notification.
                    //
                    TryDelayedRequestCompletion(
                        LeasingApplicationContext,
                        REMOTE_LEASING_APPLICATION_EXPIRED,
                        NULL,
                        LeasingApplicationContext->Instance,
                        LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier,
                        -1,
                        -1,
                        NULL,
                        0,
                        0,
                        STATUS_SUCCESS,
                        TRUE,
                        0,
                        NULL,
                        NULL,
                        0,
                        NULL,
                        NULL,
                        0,
                        0,
                        0,
                        0
                        );
                }

                EventWriteTerminateMonitorLeaseSubFailed(
                    NULL,
                    LeaseRelationshipIdentifierMonitor->LeasingApplicationContext->LeasingApplicationIdentifier,
                    LeaseRelationshipIdentifierMonitor->RemoteLeasingApplicationIdentifier,
                    RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                    RemoteLeaseAgentContext->Instance.QuadPart
                    ); 
            // end of 'Failed' termination check
            }
            else
            {
                EventWriteTerminateMonitorLeaseSubTerminated(
                    NULL,
                    LeaseRelationshipIdentifierMonitor->LeasingApplicationContext->LeasingApplicationIdentifier,
                    LeaseRelationshipIdentifierMonitor->RemoteLeasingApplicationIdentifier,
                    RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                    RemoteLeaseAgentContext->Instance.QuadPart
                    ); 
            }
            
            Deleted = RtlDeleteElementGenericTable(
                &RemoteLeaseAgentContext->MonitorHashTable,
                &LeaseRelationshipIdentifierMonitor
                );
            LAssert(Deleted);

            //
            // Deallocate lease identifier.
            //
            LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifierMonitor);
        }

        //
        // Opportunistically add the lease relationship identifier to the 
        // subject failed accepted list. If the entry cannot be added, 
        // the subject will resend it at a later time.
        //
        RtlInsertElementGenericTable(
            OutgoingSubjectTerminateFailedAcceptedList,
            &LeaseRelationshipIdentifier,
            sizeof(PLEASE_RELATIONSHIP_IDENTIFIER),
            &Inserted
            );

        if (!Inserted) {

            EventWriteInsertFail(
                NULL
                );

            //
            // Dellocate lease relationship identifier.
            //
            LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifier);
        }

        //
        // Move on to the next element.
        //
        Iter = RtlEnumerateGenericTable(SubjectTerminateFailedPendingList, TRUE);
    }
}

VOID
ProcessSubjectTerminateFailedAcceptedList(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PRTL_GENERIC_TABLE SubjectTerminateFailedAcceptedList,
    __in BOOLEAN IsSubjectFailedList
    )
    
/*++

Routine Description:

    Processes the subject failed/terminate accepted list.

Arguments:

    RemoteLeaseAgentContext - remote lease agent structure to process the list.

    SubjectTerminateFailedAcceptedList - subject failed accepted list.

    IsSubjectFailedList - is this the failed list or the terminate list?

Return Value:

    n/a.

--*/

{
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifierPending = NULL;
    PVOID Iter = NULL;
    PVOID IterPending = NULL;
    BOOLEAN Deleted = FALSE;
    PRTL_GENERIC_TABLE HashTable = IsSubjectFailedList ? 
        &RemoteLeaseAgentContext->SubjectFailedPendingHashTable :
        &RemoteLeaseAgentContext->SubjectTerminatePendingHashTable;

    //
    // Iterate over the subject failed accepted list.
    //
    for (Iter = RtlEnumerateGenericTable(SubjectTerminateFailedAcceptedList, TRUE);
         NULL != Iter;
         Iter = RtlEnumerateGenericTable(SubjectTerminateFailedAcceptedList, FALSE)) {
    
        //
        // Retrieve lease relationship identifier from the subject failed accepted list.
        //
        LeaseRelationshipIdentifier = *((PLEASE_RELATIONSHIP_IDENTIFIER*)Iter);

        //
        // Find the element in the subject failed pending list.
        //
        IterPending = RtlLookupElementGenericTable(
            HashTable,
            &LeaseRelationshipIdentifier
            );
        
        if (NULL != IterPending) {

            //
            // Retrieve lease relationship identifier from the subject pending list.
            //
            LeaseRelationshipIdentifierPending = *((PLEASE_RELATIONSHIP_IDENTIFIER*)IterPending);

            //
            // Remove from subject pending list.
            //
            Deleted = RtlDeleteElementGenericTable(
                HashTable,
                &LeaseRelationshipIdentifierPending
                );

            LAssert(Deleted);

            if (IsSubjectFailedList)
            {
                EventWriteSubLeaseFailureAcked(
                    NULL,
                    LeaseRelationshipIdentifierPending->LeasingApplicationContext->LeasingApplicationIdentifier,
                    LeaseRelationshipIdentifierPending->RemoteLeasingApplicationIdentifier,
                    RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                    RemoteLeaseAgentContext->Instance.QuadPart
                    ); 
            }
            else
            {
                EventWriteSubLeaseTerminationAcked(
                    NULL,
                    LeaseRelationshipIdentifierPending->LeasingApplicationContext->LeasingApplicationIdentifier,
                    LeaseRelationshipIdentifierPending->RemoteLeasingApplicationIdentifier,
                    RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                    RemoteLeaseAgentContext->Instance.QuadPart
                    ); 
            }
            

            //
            // Deallocate lease identifier.
            //
            LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifierPending);
        }
    }
}

PLEASE_RELATIONSHIP_IDENTIFIER
GetReverseLeaseRelationshipIdentifier(
    __in PRTL_GENERIC_TABLE SubjectPendingList
    )
{
    PVOID IterSubjectPending = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifierReverse = NULL;

    //
    // Iterate over the subject pending list.
    //
    IterSubjectPending = RtlEnumerateGenericTable(SubjectPendingList, TRUE);
        
    while (NULL != IterSubjectPending)
    {
        //
        // Retrieve lease relationship identifier from the subject pending list.
        //
        LeaseRelationshipIdentifier = *((PLEASE_RELATIONSHIP_IDENTIFIER*)IterSubjectPending);

        LeaseRelationshipIdentifierReverse = LeaseRelationshipIdentifierConstructor(
            LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
            LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier
            );

        break;
    }

    return LeaseRelationshipIdentifierReverse;
}


VOID
ProcessSubjectPendingList(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PRTL_GENERIC_TABLE SubjectPendingList,
    __inout PRTL_GENERIC_TABLE OutgoingSubjectPendingAcceptedList, 
    __inout PRTL_GENERIC_TABLE OutgoingSubjectPendingRejectedList
    )
    
/*++

Routine Description:

    Processes the subject pending list.

Arguments:

    RemoteLeaseAgentContext - remote lease agent structure to process the list.

    SubjectPendingList - subject pending list.

    OutgoingSubjectPendingAcceptedList - subbject pending accepted list.

    OutgoingSubjectPendingRejectedList - subject pending rejected list.

Return Value:

    n/a.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID IterLeasingApplication = NULL;
    PVOID IterSubjectPending = NULL;
    PVOID IterMonitor = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifierMonitor = NULL;
    BOOLEAN Inserted = TRUE;
    BOOLEAN Deleted = FALSE;

    //
    // Iterate over the subject pending list.
    //
    IterSubjectPending = RtlEnumerateGenericTable(SubjectPendingList, TRUE);
        
    while (NULL != IterSubjectPending) {

        //
        // Retrieve lease relationship identifier from the subject pending list.
        //
        LeaseRelationshipIdentifier = *((PLEASE_RELATIONSHIP_IDENTIFIER*)IterSubjectPending);

        //
        // Remove the element from the input subject pending list.
        //
        Deleted = RtlDeleteElementGenericTable(
            SubjectPendingList,
            &LeaseRelationshipIdentifier
            );
        LAssert(Deleted);

        //
        // Switch temporarily the leasing application identifier
        // with the remote leasing application identifier.
        //
        SwitchLeaseRelationshipLeasingApplicationIdentifiers(LeaseRelationshipIdentifier);

        //
        // Check to see if the leasing application is registered.
        //
        IterLeasingApplication = RtlLookupElementGenericTable(
            &RemoteLeaseAgentContext->LeaseAgentContext->LeasingApplicationContextHashTable,
            &LeaseRelationshipIdentifier->LeasingApplicationContext
            );

        //
        // Switch back the leasing application identifier and the 
        // remote leasing application identifier to initial values.
        //
        SwitchLeaseRelationshipLeasingApplicationIdentifiers(LeaseRelationshipIdentifier);

        //
        // Reset monitor lease relationship identifier.
        //
        LeaseRelationshipIdentifierMonitor = NULL;
        
        //
        // The application might not be registered yet or it is down.
        //
        if (NULL != IterLeasingApplication) {

            IterMonitor = RtlLookupElementGenericTable(
                &RemoteLeaseAgentContext->MonitorHashTable,
                &LeaseRelationshipIdentifier
                );

            //
            // It is possible that the monitor was added on a previous message
            // but the subject failed accepted was not populated properly. In
            // that case a new message will show up containing it again.
            //
            if (NULL != IterMonitor) {

                //
                // Monitor already created.
                //
                Status = STATUS_SUCCESS;
                
            } else {

                //
                // Create new monitor relationship.
                //
                LeaseRelationshipIdentifierMonitor = AddToLeaseRelationshipIdentifierList(
                    LeaseRelationshipIdentifier->LeasingApplicationContext,
                    LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
                    &RemoteLeaseAgentContext->MonitorHashTable
                    );
                
                if (NULL == LeaseRelationshipIdentifierMonitor) {

                    EventWriteInsertFail(
                        NULL
                        );

                    //
                    // Could not create lease relationship identifier.
                    //
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    
                } else {

                    //
                    // The new monitor is now correctly set up.
                    //
                    Status = STATUS_SUCCESS;

                    EventWriteEstabMonitorLease(
                        NULL,
                        LeaseRelationshipIdentifierMonitor->LeasingApplicationContext->LeasingApplicationIdentifier,
                        LeaseRelationshipIdentifierMonitor->RemoteLeasingApplicationIdentifier,
                        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                        RemoteLeaseAgentContext->Instance.QuadPart
                        );

                }
            }

        } else {

            //
            // The leasing application is not registered.
            //
            Status = STATUS_INVALID_PARAMETER;
        }

        //
        // Check status.
        //
        if (!NT_SUCCESS(Status)) {

            if (NULL != LeaseRelationshipIdentifierMonitor) {
                
                //
                // Remove monitor lease relationship identifier.
                //
                Deleted = RtlDeleteElementGenericTable(
                    &RemoteLeaseAgentContext->MonitorHashTable,
                    &LeaseRelationshipIdentifierMonitor
                    );
                LAssert(Deleted);
                
                //
                // Deallocate lease relationship identifier.
                //
                LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifierMonitor);
            }

            //
            // Opportunistically add the lease relationship identifier to the 
            // monitor pending rejected list. If the entry cannot be added, 
            // the subject will resend it at a later time.
            //
            RtlInsertElementGenericTable(
                OutgoingSubjectPendingRejectedList,
                &LeaseRelationshipIdentifier,
                sizeof(PLEASE_RELATIONSHIP_IDENTIFIER),
                &Inserted
                );

            if (!Inserted) {

                EventWriteAllocateFail7(
                        NULL
                        ); 

                //
                // Deallocate lease relationship identifier.
                //
                LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifier);
            }

        } else {
        
            //
            // Opportunistically add the lease relationship identifier to the 
            // monitor pending accepted list. If the entry cannot be added, 
            // the subject will resend it at a later time.
            //
            RtlInsertElementGenericTable(
                OutgoingSubjectPendingAcceptedList,
                &LeaseRelationshipIdentifier,
                sizeof(PLEASE_RELATIONSHIP_IDENTIFIER),
                &Inserted
                );
            
            if (!Inserted) {
            
                EventWriteAllocateFail8(
                        NULL
                        );
                //
                // Deallocate lease relationship identifier.
                //
                LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifier);
            }
        }

        //
        // Move on to the next element.
        //
        IterSubjectPending = RtlEnumerateGenericTable(SubjectPendingList, TRUE);
    }
}

VOID
ProcessMonitorFailedPendingList(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PRTL_GENERIC_TABLE MonitorFailedPendingList,
    __inout PRTL_GENERIC_TABLE OutgoingMonitorFailedAcceptedList
    )
    
/*++

Routine Description:

    Processes the monitor failed pending list.

Arguments:

    RemoteLeaseAgentContext - remote lease agent structure to process the list.

    MonitorFailedPendingList - monitor failed pending list.

    OutgoingMonitorFailedAcceptedList - monitor failed accepted list.

Return Value:

    n/a.

--*/

{
    PVOID IterLeasingApplication = NULL;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    PVOID Iter = NULL;
    PVOID IterSubject = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER* LeaseRelationshipIdentifierExistent = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifierSubject = NULL;
    BOOLEAN Inserted = TRUE;
    BOOLEAN Deleted = FALSE;

    //
    // Iterate over the monitor failed list.
    //
    Iter = RtlEnumerateGenericTable(MonitorFailedPendingList, TRUE);
        
    while (NULL != Iter) {

        //
        // Retrieve lease relationship identifier from the monitor failed pending list.
        //
        LeaseRelationshipIdentifier = *((PLEASE_RELATIONSHIP_IDENTIFIER*)Iter);

        //
        // Remove the element from the input monitor failed pending list.
        //
        Deleted = RtlDeleteElementGenericTable(
            MonitorFailedPendingList,
            &LeaseRelationshipIdentifier
            );
        LAssert(Deleted);

        //
        // Look up the lease relationship identifier in the subject list.
        //
        IterSubject = RtlLookupElementGenericTable(
            &RemoteLeaseAgentContext->SubjectHashTable,
            &LeaseRelationshipIdentifier
            );

        //
        // If the subject is in the list, it needs to be added to the subject failed pending list.
        // It might not be there snce this lease relationship identifier might have been sent
        // on a previous message.
        //
        if (NULL != IterSubject) {
            
            //
            // Retrieve lease relationship identifier from the subject list.
            //
            LeaseRelationshipIdentifierSubject = *((PLEASE_RELATIONSHIP_IDENTIFIER*)IterSubject);

            //
            // Move the lease relationship identifier from the subject list
            // to the subject failed pending list.
            //
            LeaseRelationshipIdentifierExistent = RtlInsertElementGenericTable(
                &RemoteLeaseAgentContext->SubjectFailedPendingHashTable,
                &LeaseRelationshipIdentifierSubject,
                sizeof(PLEASE_RELATIONSHIP_IDENTIFIER),
                &Inserted
                );
            
            //
            // Check insert result.
            //
            if (NULL == LeaseRelationshipIdentifierExistent) {
            
                LAssert(!Inserted);

                //
                // This should happen very rarely. Attempt to process more elements.
                //
                EventWriteInsertFail(
                    NULL
                    ); 

            } else {
            
                //
                // There should be only one copy of this element in this hash table.
                //
                LAssert(Inserted);
            
                //
                // Remove the lease relationship identifier from the subject list.
                //
                Deleted = RtlDeleteElementGenericTable(
                    &RemoteLeaseAgentContext->SubjectHashTable,
                    &LeaseRelationshipIdentifierSubject
                    );
                LAssert(Deleted);

                //
                // Retrieve the leasing application.
                //
                IterLeasingApplication = RtlLookupElementGenericTable(
                    &RemoteLeaseAgentContext->LeaseAgentContext->LeasingApplicationContextHashTable,
                    &LeaseRelationshipIdentifierSubject->LeasingApplicationContext
                    );
                
                if (NULL != IterLeasingApplication) {
                
                    //
                    // Retrieve lease relationship identifier from the subject pending list.
                    //
                    LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);
                
                    //
                    // Fire remote leasing application expired notification.
                    //
                    TryDelayedRequestCompletion(
                        LeasingApplicationContext,
                        REMOTE_LEASING_APPLICATION_EXPIRED,
                        NULL,
                        LeasingApplicationContext->Instance,
                        LeaseRelationshipIdentifierSubject->RemoteLeasingApplicationIdentifier,
                        -1,
                        -1,
                        NULL,
                        0,
                        0,
                        STATUS_SUCCESS,
                        TRUE,
                        0,
                        NULL,
                        NULL,
                        0,
                        NULL,
                        NULL,
                        0,
                        0,
                        0,
                        0
                        );
                }

                EventWriteTerminateSubLeaseMonitorFailed(
                    NULL,
                    LeaseRelationshipIdentifierSubject->LeasingApplicationContext->LeasingApplicationIdentifier,
                    LeaseRelationshipIdentifierSubject->RemoteLeasingApplicationIdentifier,
                    RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                    RemoteLeaseAgentContext->Instance.QuadPart
                    ); 
                    
                //
                // Opportunistically add the lease relationship identifier to the 
                // monitor pending accepted list. If the entry cannot be added, 
                // the subject will resend it at a later time.
                //
                RtlInsertElementGenericTable(
                    OutgoingMonitorFailedAcceptedList,
                    &LeaseRelationshipIdentifier,
                    sizeof(PLEASE_RELATIONSHIP_IDENTIFIER),
                    &Inserted
                    );

                if (!Inserted) {
                    
                    EventWriteInsertFail(
                        NULL
                        ); 

                    //
                    // Deallocate lease relationship identifier.
                    //
                    LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifier);
                }
            }
                
        } else {
            
            //
            // Deallocate lease relationship identifier.
            //
            LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifier);
        }

        //
        // Move on to the next element.
        //
        Iter = RtlEnumerateGenericTable(MonitorFailedPendingList, TRUE);
    }
}

VOID
ProcessSubjectPendingAcceptedList(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PRTL_GENERIC_TABLE SubjectPendingAcceptedList
    )
    
/*++

Routine Description:

    Processes the subject pending accepted list.

Arguments:

    RemoteLeaseAgentContext - remote lease agent structure to process the list.

    SubjectPendingList - subject pending accepted list.

Return Value:

    n/a.

--*/

{
    PVOID IterLeasingApplication = NULL;
    PVOID IterSubject = NULL;
    PVOID IterSubjectPending = NULL;
    PVOID Iter = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifierSubject = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifierSubjectPending = NULL;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    BOOLEAN Deleted = FALSE;

    //
    // Iterate over the subject pending accepted list.
    //
    for (Iter = RtlEnumerateGenericTable(SubjectPendingAcceptedList, TRUE);
         NULL != Iter;
         Iter = RtlEnumerateGenericTable(SubjectPendingAcceptedList, FALSE)) {

        //
        // Retrieve lease relationship identifier from the subject accepted list.
        //
        LeaseRelationshipIdentifier = *((PLEASE_RELATIONSHIP_IDENTIFIER*)Iter);

        //
        // Retrieve lease relationship identifier from the subject pending list.
        //
        IterSubjectPending = RtlLookupElementGenericTable(
            &RemoteLeaseAgentContext->SubjectEstablishPendingHashTable,
            &LeaseRelationshipIdentifier
            );

        if (NULL != IterSubjectPending) {

            LeaseRelationshipIdentifierSubjectPending = *((PLEASE_RELATIONSHIP_IDENTIFIER*)IterSubjectPending);

            //
            // Retrieve lease relationship identifier from the subject list.
            //
            IterSubject = RtlLookupElementGenericTable(
                &RemoteLeaseAgentContext->SubjectHashTable,
                &LeaseRelationshipIdentifier
                );

            if (NULL == IterSubject) {

                //
                // By this time the subject does not exist anymore.
                //
                continue;
            }

            LeaseRelationshipIdentifierSubject = *((PLEASE_RELATIONSHIP_IDENTIFIER*)IterSubject);

            //
            // Retrieve the leasing application.
            //
            IterLeasingApplication = RtlLookupElementGenericTable(
                &RemoteLeaseAgentContext->LeaseAgentContext->LeasingApplicationContextHashTable,
                &LeaseRelationshipIdentifier->LeasingApplicationContext
                );

            if (NULL != IterLeasingApplication) {

                //
                // Retrieve lease relationship identifier from the subject pending list.
                //
                LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);

                //
                // Fire lease establish notification.
                //
                TryDelayedRequestCompletion(
                    LeasingApplicationContext,
                    LEASING_APPLICATION_LEASE_ESTABLISHED,
                    LeaseRelationshipIdentifierSubject,
                    LeasingApplicationContext->Instance,
                    LeaseRelationshipIdentifierSubject->RemoteLeasingApplicationIdentifier,
                    -1,
                    -1,
                    NULL,
                    0,
                    0,
                    STATUS_SUCCESS,
                    FALSE,
                    0,
                    NULL,
                    NULL,
                    0,
                    NULL,
                    NULL,
                    0,
                    0,
                    0,
                    0
                    );
            }

            //
            // Remove from subject pending list.
            //
            Deleted = RtlDeleteElementGenericTable(
                &RemoteLeaseAgentContext->SubjectEstablishPendingHashTable,
                &LeaseRelationshipIdentifierSubjectPending
                );
            LAssert(Deleted);

            //
            // Deallocate lease identifier.
            //
            LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifierSubjectPending);
        }
    }
}

VOID
ProcessSubjectPendingRejectedList(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PRTL_GENERIC_TABLE SubjectPendingRejectedList
    )
    
/*++

Routine Description:

    Processes the subject pending rejected list.

Arguments:

    RemoteLeaseAgentContext - remote lease agent structure to process the list.

    SubjectPendingList - subject pending accepted list.

Return Value:

    n/a.

--*/

{
    PVOID Iter = NULL;
    PVOID IterLeasingApplication = NULL;
    PVOID IterSubject = NULL;
    PVOID IterSubjectPending = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifierSubject = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifierSubjectPending = NULL;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    BOOLEAN Deleted = FALSE;

    //
    // Iterate over the subject pending rejected list.
    //
    for (Iter = RtlEnumerateGenericTable(SubjectPendingRejectedList, TRUE);
         NULL != Iter;
         Iter = RtlEnumerateGenericTable(SubjectPendingRejectedList, FALSE)) {

        //
        // Retrieve lease relationship identifier from the subject accepted list.
        //
        LeaseRelationshipIdentifier = *((PLEASE_RELATIONSHIP_IDENTIFIER*)Iter);

        IterSubjectPending = RtlLookupElementGenericTable(
            &RemoteLeaseAgentContext->SubjectEstablishPendingHashTable,
            &LeaseRelationshipIdentifier
            );

        if (NULL != IterSubjectPending) {

            //
            // Retrieve lease relationship identifier from the subject pending list.
            //
            LeaseRelationshipIdentifierSubjectPending = *((PLEASE_RELATIONSHIP_IDENTIFIER*)IterSubjectPending);

            //
            // Retrieve subject lease relationship.
            //
            IterSubject = RtlLookupElementGenericTable(
                &RemoteLeaseAgentContext->SubjectHashTable,
                &LeaseRelationshipIdentifier
                );
            
            if (NULL == IterSubject) {

                //
                // By this time the subject does not exist anymore.
                //
                continue;
            }

            //
            // Retrieve lease relationship identifier from the subject list.
            //
            LeaseRelationshipIdentifierSubject = *((PLEASE_RELATIONSHIP_IDENTIFIER*)IterSubject);

            //
            // Retrieve the leasing application.
            //
            IterLeasingApplication = RtlLookupElementGenericTable(
                &RemoteLeaseAgentContext->LeaseAgentContext->LeasingApplicationContextHashTable,
                &LeaseRelationshipIdentifier->LeasingApplicationContext
                );

            if (NULL != IterLeasingApplication) {

                //
                // Retrieve lease relationship identifier from the subject pending list.
                //
                LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);

                //
                // Fire remote leasing application failed notification.
                //
                TryDelayedRequestCompletion(
                    LeasingApplicationContext,
                    REMOTE_LEASING_APPLICATION_EXPIRED,
                    NULL,
                    LeasingApplicationContext->Instance,
                    LeaseRelationshipIdentifierSubject->RemoteLeasingApplicationIdentifier,
                    -1,
                    -1,
                    NULL,
                    0,
                    0,
                    STATUS_SUCCESS,
                    TRUE,
                    0,
                    NULL,
                    NULL,
                    0,
                    NULL,
                    NULL,
                    0,
                    0,
                    0,
                    0
                    );

            }

            //
            // Remove from subject pending list.
            //
            Deleted = RtlDeleteElementGenericTable(
                &RemoteLeaseAgentContext->SubjectEstablishPendingHashTable,
                &LeaseRelationshipIdentifierSubjectPending
                );
            LAssert(Deleted);

            //
            // Deallocate lease identifier.
            //
            LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifierSubjectPending);

            //
            // Remove from subject list.
            //
            Deleted = RtlDeleteElementGenericTable(
                &RemoteLeaseAgentContext->SubjectHashTable,
                &LeaseRelationshipIdentifierSubject
                );
            LAssert(Deleted);
            
            //
            // Deallocate lease identifier.
            //
            LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifierSubject);
        }
    }
}

VOID
ProcessMonitorFailedAcceptedList(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PRTL_GENERIC_TABLE MonitorFailedAcceptedList
    )
    
/*++

Routine Description:

    Processes the monitor failed accepted list.

Arguments:

    RemoteLeaseAgentContext - remote lease agent structure to process the list.

    MonitorFailedAcceptedList - monitor failed accepted list.

Return Value:

    n/a.

--*/

{
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifierPending = NULL;
    PVOID Iter = NULL;
    PVOID IterPending = NULL;
    BOOLEAN Deleted = FALSE;

    //
    // Iterate over the subject failed accepted list.
    //
    for (Iter = RtlEnumerateGenericTable(MonitorFailedAcceptedList, TRUE);
         NULL != Iter;
         Iter = RtlEnumerateGenericTable(MonitorFailedAcceptedList, FALSE)) {
    
        //
        // Retrieve lease relationship identifier from the subject failed accepted list.
        //
        LeaseRelationshipIdentifier = *((PLEASE_RELATIONSHIP_IDENTIFIER*)Iter);

        //
        // Find the element in the monitor failed pending list.
        //
        IterPending = RtlLookupElementGenericTable(
            &RemoteLeaseAgentContext->MonitorFailedPendingHashTable,
            &LeaseRelationshipIdentifier
            );
        
        if (NULL != IterPending) {

            //
            // Retrieve lease relationship identifier from the subject pending list.
            //
            LeaseRelationshipIdentifierPending = *((PLEASE_RELATIONSHIP_IDENTIFIER*)IterPending);

            //
            // Remove from subject pending list.
            //
            Deleted = RtlDeleteElementGenericTable(
                &RemoteLeaseAgentContext->MonitorFailedPendingHashTable,
                &LeaseRelationshipIdentifierPending
                );
            LAssert(Deleted);
            
            EventWriteMonitorLeaseFailureAcked(
                NULL,
                LeaseRelationshipIdentifierPending->LeasingApplicationContext->LeasingApplicationIdentifier,
                LeaseRelationshipIdentifierPending->RemoteLeasingApplicationIdentifier,
                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                RemoteLeaseAgentContext->Instance.QuadPart
                ); 
            //
            // Deallocate lease identifier.
            //
            LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifierPending);
        }
    }
}

VOID
LeaseResponseChangeState(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PRTL_GENERIC_TABLE SubjectPendingAcceptedList,
    __in PRTL_GENERIC_TABLE SubjectPendingRejectedList,
    __in PRTL_GENERIC_TABLE SubjectFailedAcceptedList,
    __in PRTL_GENERIC_TABLE SubjectTerminateAcceptedList,
    __in PRTL_GENERIC_TABLE MonitorFailedAcceptedList,
    __in PRTL_GENERIC_TABLE MonitorFailedPendingList,
    __in PRTL_GENERIC_TABLE OutgoingMonitorFailedAcceptedList
    )
    
/*++

Routine Description:

    Processes the lease response message. We will attempt to
    process as much as we can from this message.

Arguments:

    RemoteLeaseAgentContext - remote lease agent structure to process the list.

    SubjectTerminatePendingList - subject terminate pending list.

    SubjectPendingAcceptedList - subject pending accepted list.

    SubjectFailedAcceptedList - received subject failed accepted list.

    SubjectPendingRejectedList - subject pending rejected list.

    MonitorFailedAcceptedList - monitor failed accepted list.

Return Value:

    n/a.

--*/

{
    ProcessSubjectPendingAcceptedList(
        RemoteLeaseAgentContext,
        SubjectPendingAcceptedList
        );
    
    ProcessSubjectPendingRejectedList(
        RemoteLeaseAgentContext,
        SubjectPendingRejectedList
        );

    ProcessSubjectTerminateFailedAcceptedList(
        RemoteLeaseAgentContext,
        SubjectFailedAcceptedList,
        TRUE
        );

    ProcessSubjectTerminateFailedAcceptedList(
        RemoteLeaseAgentContext,
        SubjectTerminateAcceptedList,
        FALSE
        );

    ProcessMonitorFailedAcceptedList(
        RemoteLeaseAgentContext,
        MonitorFailedAcceptedList
        );

    ProcessMonitorFailedPendingList(
        RemoteLeaseAgentContext,
        MonitorFailedPendingList,
        OutgoingMonitorFailedAcceptedList
        );
}

VOID
LeaseRequestChangeState(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PRTL_GENERIC_TABLE SubjectPendingList,
    __in PRTL_GENERIC_TABLE SubjectFailedPendingList,
    __in PRTL_GENERIC_TABLE MonitorFailedPendingList,
    __in PRTL_GENERIC_TABLE SubjectTerminatePendingList,
    __inout PRTL_GENERIC_TABLE OutgoingSubjectPendingAcceptedList,
    __inout PRTL_GENERIC_TABLE OutgoingSubjectPendingRejectedList,
    __inout PRTL_GENERIC_TABLE OutgoingSubjectFailedAcceptedList,
    __inout PRTL_GENERIC_TABLE OutgoingMonitorFailedAcceptedList,
    __inout PRTL_GENERIC_TABLE OutgoingSubjectTerminateAcceptedList
    )
    
/*++

Routine Description:

    Process a lease request message.

Arguments:

    RemoteLeaseAgentContext - remote lease agent structure to process the list.

    SubjectPendingAcceptedList - pending lease relationships that have been accepted.

    SubjectFailedPendingList - failed subject pending list.

    MonitorFailedPendingList - failed monitor pending list.

    SubjectTerminatePendingList - subject terminate pending list.

    OutgoingSubjectPendingAcceptedList - subject pending accepted list.

    OutgoingSubjectFailedAcceptedList - subject failed accepted list.

    OutgoingMonitorFailedAcceptedList - monitor failed accepted list.

    OutgoingSubjectPendingRejectedList - subject pending rejected list.

    OutgoingSubjectTerminateAcceptedList - subject terminate accepted list.

Return Value:

    n/a.

--*/

{
    ProcessSubjectPendingList(
        RemoteLeaseAgentContext,
        SubjectPendingList,
        OutgoingSubjectPendingAcceptedList,
        OutgoingSubjectPendingRejectedList
        );

    ProcessSubjectTerminateFailedPendingList(
        RemoteLeaseAgentContext, 
        SubjectTerminatePendingList,
        FALSE,
        OutgoingSubjectTerminateAcceptedList
        );    

    ProcessSubjectTerminateFailedPendingList(
        RemoteLeaseAgentContext, 
        SubjectFailedPendingList,
        TRUE,
        OutgoingSubjectFailedAcceptedList
        );

    ProcessMonitorFailedPendingList(
        RemoteLeaseAgentContext,
        MonitorFailedPendingList,
        OutgoingMonitorFailedAcceptedList
        );
}

const LPWSTR GetLeaseAgentState(
    __in LEASE_AGENT_STATE State
    )
    
/*++

Routine Description:

    Returns readable lease agent state.

Parameters Description:

    State - state for pretty printing.

Return Value:

    Stringyfied lease agent state.

--*/

{
    LPWSTR StateString = L"";

    switch (State) {
        
    case OPEN:
        StateString = L"OPEN";
        break;
    case SUSPENDED:
        StateString = L"SUSPENDED";
        break;
    case FAILED:
        StateString = L"FAILED";
        break;
    }

    return StateString;
}

const LPWSTR GetMessageType(
    __in LEASE_MESSAGE_TYPE MessageType
    )
    
/*++

Routine Description:

    Returns readable message type.

Parameters Description:

    MessageType - message type for pretty printing.

Return Value:

    Stringyfied message type.

--*/

{
    LPWSTR MessageTypeString = L"";

    switch (MessageType) {

    case LEASE_REQUEST:
        MessageTypeString = L"REQUEST";
        break;
    case LEASE_RESPONSE:
        MessageTypeString = L"RESPONSE";
        break;
    case PING_REQUEST:
        MessageTypeString = L"PING REQUEST";
        break;
    case PING_RESPONSE:
        MessageTypeString = L"PING RESPONSE";
        break;
    case FORWARD_REQUEST:
        MessageTypeString = L"FORWARD REQUEST";
        break;
    case FORWARD_RESPONSE:
        MessageTypeString = L"FORWARD RESPONSE";
        break;
    case RELAY_REQUEST:
        MessageTypeString = L"RELAY REQUEST";
        break;
    case RELAY_RESPONSE:
        MessageTypeString = L"RELAY RESPONSE";
        break;
    }

    return MessageTypeString;
}

const LPWSTR GetBlockMessageType(
    __in LEASE_BLOCKING_ACTION_TYPE BlockMessageType
)

/*++

Routine Description:

Returns readable block message type.

Parameters Description:

BlockMessageType - block message type for pretty printing.

Return Value:

Stringyfied block message type.

--*/

{
    LPWSTR BlockMessageTypeString = L"";

    switch (BlockMessageType) {

    case LEASE_ESTABLISH_ACTION:
        BlockMessageTypeString = L"LEASE ESTABLISH ACTION";
        break;
    case LEASE_ESTABLISH_RESPONSE:
        BlockMessageTypeString = L"LEASE ESTABLISH RESPONSE";
        break;
    case LEASE_PING_RESPONSE:
        BlockMessageTypeString = L"LEASE PING RESPONSE";
        break;
    case LEASE_PING_REQUEST:
        BlockMessageTypeString = L"LEASE PING REQUEST";
        break;
    case LEASE_INDIRECT:
        BlockMessageTypeString = L"LEASE INDIRECT";
        break;
    case LEASE_BLOCKING_ALL:
        BlockMessageTypeString = L"LEASE BLOCKING ALL";
        break;
    }

    return BlockMessageTypeString;
}

VOID
SetLeaseRelationshipDuration(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in LONG Duration,
    __in LONG LeaseSuspendDuration,
    __in LONG ArbitrationDuration
    )
/*++

Routine Description:

    Compare the duration in the coming in request with local lease agent
    Return the ones that are greater.

Arguments:

    Duration, LeaseSuspendDuration, ArbitrationDuration  - Incoming lease related timeout.

--*/
{
    if (DURATION_MAX_VALUE == Duration || DURATION_MAX_VALUE ==  LeaseSuspendDuration || DURATION_MAX_VALUE ==  ArbitrationDuration)
    {
        return;
    }

    RemoteLeaseAgentContext->LeaseRelationshipContext->Duration = Duration;
    RemoteLeaseAgentContext->LeaseRelationshipContext->LeaseSuspendDuration = LeaseSuspendDuration;
    RemoteLeaseAgentContext->LeaseRelationshipContext->ArbitrationDuration = ArbitrationDuration;
}

VOID
GrantLargerDurations(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __inout PLONG IncomingLeaseDuration,
    __inout PLONG IncomingLeaseSuspendDuration,
    __inout PLONG IncomingArbitrationDuration,
    __inout PLARGE_INTEGER IncomingExpiration
    )
/*++
    1. Compare and return the greater of the incoming durations and the local configuration durations.
    2. Save the incoming duration to the lease relationship context.
--*/
{
    LONG ConfigLeaseSuspendDuration = RemoteLeaseAgentContext->LeaseAgentContext->LeaseSuspendDuration;
    LONG ConfigArbitrationDuration = RemoteLeaseAgentContext->LeaseAgentContext->ArbitrationDuration;
    LONG ConfigLeaseDuration = GetConfigLeaseDuration(RemoteLeaseAgentContext);

    LAssert(
        IncomingLeaseDuration != NULL &&
        IncomingLeaseSuspendDuration != NULL &&
        IncomingArbitrationDuration != NULL);

    if (DURATION_MAX_VALUE == *IncomingLeaseDuration)
    {
        // This is a termination, do nothing
        return;
    }

    if(DURATION_MAX_VALUE == *IncomingLeaseSuspendDuration ||
        DURATION_MAX_VALUE == *IncomingArbitrationDuration)
    {
        EventWriteLeaseDriverTextTraceError(NULL, L"Invalid inputs when update durations",0);
        return;
    }

    if (*IncomingLeaseDuration != RemoteLeaseAgentContext->LeaseRelationshipContext->Duration ||
        *IncomingLeaseSuspendDuration != RemoteLeaseAgentContext->LeaseRelationshipContext->LeaseSuspendDuration ||
        *IncomingArbitrationDuration != RemoteLeaseAgentContext->LeaseRelationshipContext->ArbitrationDuration
        )
    {
        // Update the remote duration if the requested is different from the exisint lease relationship
        EventWriteUpdateRemoteLeaseDuration(
            NULL,
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart,
            *IncomingLeaseDuration,
            RemoteLeaseAgentContext->LeaseRelationshipContext->RemoteLeaseDuration,
            ConfigLeaseDuration,
            RemoteLeaseAgentContext->LeaseRelationshipContext->Duration
            );

        RemoteLeaseAgentContext->LeaseRelationshipContext->RemoteLeaseDuration = *IncomingLeaseDuration;
        RemoteLeaseAgentContext->LeaseRelationshipContext->RemoteSuspendDuration = *IncomingLeaseSuspendDuration;
        RemoteLeaseAgentContext->LeaseRelationshipContext->RemoteArbitrationDuration = *IncomingArbitrationDuration;

    }

    if (ConfigLeaseDuration > *IncomingLeaseDuration)
    {
        EventWriteMonitorGrantGreaterLeaseDuration(
            NULL,
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart,
            *IncomingLeaseDuration,
            ConfigLeaseDuration,
            RemoteLeaseAgentContext->LeaseRelationshipContext->EstablishDurationType,
            (IncomingExpiration != NULL) ? IncomingExpiration->QuadPart: 0
            );

        // Incoming expiration would be NULL for a reponse
        //
        if (IncomingExpiration != NULL)
        {
            IncomingExpiration->QuadPart += (LONGLONG) (ConfigLeaseDuration - *IncomingLeaseDuration) * MILLISECOND_TO_NANOSECOND_FACTOR;
        }

        // A greater duration and expiration would be granted for the incoming request
        //
        *IncomingLeaseDuration = ConfigLeaseDuration;

    }

    if (ConfigLeaseSuspendDuration > *IncomingLeaseSuspendDuration)
    {
        *IncomingLeaseSuspendDuration = ConfigLeaseSuspendDuration;
    }

    if (ConfigArbitrationDuration > *IncomingArbitrationDuration)
    {
        *IncomingArbitrationDuration = ConfigArbitrationDuration;
    }

}

VOID
ProcessLeaseMessage(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContextMessage,
    __in LARGE_INTEGER RemoteLeaseAgentInstance,
    __in LARGE_INTEGER MessageIdentifier,
    __in LEASE_MESSAGE_TYPE MessageType,
    __in LARGE_INTEGER LeaseInstance,
    __in LONG Duration,
    __in LARGE_INTEGER Expiration,
    __in LONG LeaseSuspendDuration,
    __in LONG ArbitrationDuration,
    __in BOOLEAN IsTwoWayTermination,
    __in PRTL_GENERIC_TABLE SubjectPendingList,
    __in PRTL_GENERIC_TABLE SubjectFailedPendingList,
    __in PRTL_GENERIC_TABLE MonitorFailedPendingList,
    __in PRTL_GENERIC_TABLE SubjectPendingAcceptedList,
    __in PRTL_GENERIC_TABLE SubjectPendingRejectedList,
    __in PRTL_GENERIC_TABLE SubjectFailedAcceptedList,
    __in PRTL_GENERIC_TABLE MonitorFailedAcceptedList,
    __in PRTL_GENERIC_TABLE SubjectTerminatePendingList,
    __in PRTL_GENERIC_TABLE SubjectTerminateAcceptedList
    )
    
/*++

Routine Description:

    Processes the lease layer message.

Arguments:

    RemoteLeaseAgentContext - RLA for the lease relationship.
    RemoteLeaseAgentContextMessage - RLA for the message sender (sendTarget the message is from)
    If it is not relay message, the two are same

    MessageIdentifier - unique message identifier.

    RemoteLeaseAgentInstance - remote lease agent instance.
    
    MessageType - message type received.
    
    Identifier - remote lease agent identifier.
    
    Duration - requested lease TTL.

    LeaseSuspendDuration - short arbitration timeout.
    
    ArbitrationDuration - long arbitration timeout.

    SubjectPendingList - subject pending list.
    
    SubjectFailedPendingList - subject failed pending list.
    
    MonitorFailedPendingList - monitor failed pending list.

    SubjectPendingAcceptedList - subject pending accepted list.

    SubjectPendingRejectedList - subject pending rejected list.

    SubjectFailedAcceptedList - subject failed accepted list.

    MonitorFailedAcceptedList - monitor failed accepted list.
    
    SubjectTerminatePendingList - subject terminate pending list.

    SubjectTerminateAcceptedList - subject terminate accepted list.

Return Value:

    STATUS_SUCCESS if the message could be processed without errors.

--*/

{
    PVOID ResponseBuffer = NULL;
    ULONG ResponseBufferSize = 0;

    RTL_GENERIC_TABLE OutgoingSubjectPendingAcceptedList;
    RTL_GENERIC_TABLE OutgoingSubjectPendingRejectedList;
    RTL_GENERIC_TABLE OutgoingSubjectFailedAcceptedList;
    RTL_GENERIC_TABLE OutgoingMonitorFailedAcceptedList;
    RTL_GENERIC_TABLE OutgoingSubjectTerminateAcceptedList;

    LARGE_INTEGER TerminateExpiration;
    LARGE_INTEGER OutgoingMsgId;
    LARGE_INTEGER PreArbitrationMonitorTime;
    BOOLEAN IsRenewRequest = FALSE;
    BOOLEAN TermReverseLease1 = FALSE;

    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifierReverse = NULL;

    NTSTATUS status = STATUS_SUCCESS;
    LEASE_MESSAGE_TYPE ResponseType = LEASE_RESPONSE;

    TerminateExpiration.QuadPart = MAXLONGLONG;
    OutgoingMsgId.QuadPart = 0;

    if (RELAY_REQUEST == MessageType)
    {
        ResponseType = FORWARD_RESPONSE;
    }
    //
    // Initialize lists.
    //
    RtlInitializeGenericTable(
        &OutgoingSubjectPendingAcceptedList,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareLeaseRelationshipContexts,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL
        );
    RtlInitializeGenericTable(
        &OutgoingSubjectPendingRejectedList,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareLeaseRelationshipContexts,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL
        );
    RtlInitializeGenericTable(
        &OutgoingSubjectFailedAcceptedList,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareLeaseRelationshipContexts,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL
        );
    RtlInitializeGenericTable(
        &OutgoingMonitorFailedAcceptedList,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareLeaseRelationshipContexts,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL
        );
    RtlInitializeGenericTable(
        &OutgoingSubjectTerminateAcceptedList,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareLeaseRelationshipContexts,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL
        );

    if (DURATION_MAX_VALUE == Duration)
    {
        EventWriteLeaseRelationRcvTerminationMsg(
            NULL,
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
            GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
            GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
            NULL,
            GetMessageType(MessageType),
            MessageIdentifier.QuadPart
            ); 
    }
    else
    {
        EventWriteLeaseRelationRcvMsg(
            NULL,
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
            GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
            GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
            NULL,
            GetMessageType(MessageType),
            MessageIdentifier.QuadPart
            ); 
    }

    //
    // Check the remote lease agent instance.
    //
    if (IS_LARGE_INTEGER_EQUAL_TO_INTEGER(RemoteLeaseAgentContext->RemoteLeaseAgentInstance, 0)) {

        EventWriteRemoteLeaseAgentChangeInstance(
            NULL,
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart,
            RemoteLeaseAgentContext->RemoteLeaseAgentInstance.QuadPart,
            RemoteLeaseAgentInstance.QuadPart
            ); 
        //
        // Set the remote lease agent instance for the first time.
        //
        RemoteLeaseAgentContext->RemoteLeaseAgentInstance = RemoteLeaseAgentInstance;
    } 

    if (PING_REQUEST == MessageType)
    {
        //
        // Skip Ping if it is expired or terminated on subject or monitor side
        //
        if (LEASE_STATE_EXPIRED <= RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState ||
            LEASE_STATE_EXPIRED <= RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
        {
            goto Done;
        }

        //
        // Some other lease agent ping, send the response back
        //
        SendPingResponseMessage(RemoteLeaseAgentContext);

    }
    else if (PING_RESPONSE == MessageType)
    {
        if (LEASE_STATE_ACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState)
        {
            //
            // Lease already established, must be stale ping response;
            //
            EventWriteProcessStalePingResponse(
                NULL,
                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                RemoteLeaseAgentContext->Instance.QuadPart,
                RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
                GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
                MessageIdentifier.QuadPart
                );

            if (LEASE_STATE_ACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
            {
                //
                // If the state is A-A, the subject timer may still be left in queue, which is enqueued for ping request
                // This may happen when recieve a est request and establish the reverse side lease
                // Dequeue the subject timer will not impact the subject side lease since it only runs on renew timer in (A-A)
                //
                DequeueTimer(
                    RemoteLeaseAgentContext,
                    &RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectTimer
                    );
                    
                RemoteLeaseAgentContext->InPing = FALSE;

            }

            goto Done;
        }

        AbortPingProcess(RemoteLeaseAgentContext);

        //
        // Check the subject table here since it may be deleted by termination request
        //
        if (0 != RtlNumberGenericTableElements(&RemoteLeaseAgentContext->SubjectHashTable))
        {
            //
            // Send the establish request now
            //
            EstablishLease(RemoteLeaseAgentContext);
        }
        else if (LEASE_STATE_INACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
        {
            //
            // Receive the ping response, but the ping process is already aborted possibly due to termination
            //
            SetRemoteLeaseAgentFailed(RemoteLeaseAgentContext);
        }
        else
        {
            // RLA is I-A, but the subject table is empty
            EventWriteLeaseDriverTextTraceInfo(
                NULL,
                L"Received ping response in I-A state, wait for remote side termination"
                );
        }

    }
    else if (IsReceivedRequest(MessageType))
    {
        //
        // Check if the lease has already expired on the monitor or 
        // was terminated or expired on the subject.
        //
        if (LEASE_STATE_EXPIRED <= RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState ||
            LEASE_STATE_EXPIRED <= RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
        {
            //
            // If one of the relationship is expired or failed, ignore the request
            // Nothing else to do.
            //
            goto Done;
        }
        
        //
        // When the remote lease agent is inPing process, ignore all relayed request
        //
        if (RELAY_REQUEST == MessageType &&
            TRUE == RemoteLeaseAgentContext->InPing)
        {
            goto Done;
        }

        //
        // Check to see if this is the first monitor relationship.
        //
        if (LEASE_STATE_INACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
        {
            //
            // Ignore lease request for invalid duration.
            //
            if (DURATION_MAX_VALUE == Duration)
            {
                // TODO - work around recreated relationship for termination message
                RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier = LeaseInstance;
                //
                // Create lease response reject message.
                //
                LAssert(NULL == ResponseBuffer);
                SerializeLeaseMessage(
                    RemoteLeaseAgentContext,
                    LEASE_RESPONSE,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    DURATION_MAX_VALUE,
                    TerminateExpiration,
                    DURATION_MAX_VALUE,
                    DURATION_MAX_VALUE,
                    FALSE,
                    &ResponseBuffer,
                    &ResponseBufferSize
                    );

                if (NULL != ResponseBuffer) 
                {
                    OutgoingMsgId.QuadPart = ((PLEASE_MESSAGE_HEADER) ResponseBuffer)->MessageIdentifier.QuadPart;
                }

                EventWriteLeaseRelationSendingTermination(
                    NULL,
                    RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                    RemoteLeaseAgentContext->Instance.QuadPart,
                    RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                    RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                    GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
                    GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
                    GetMessageType(LEASE_RESPONSE),
                    OutgoingMsgId.QuadPart
                    );

                if (NULL != ResponseBuffer) {
                
                    TransportSendBuffer(
                        RemoteLeaseAgentContext->PartnerTarget,
                        ResponseBuffer,
                        ResponseBufferSize,
                        FALSE);
                }

                //
                // This remote lease agent should now go away.
                //
                if (LEASE_STATE_ACTIVE != RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState) {
                    SetRemoteLeaseAgentFailed(RemoteLeaseAgentContext);
                    KeSetEvent(
                        &RemoteLeaseAgentContext->ZeroRefCountEvent, 
                        0, 
                        FALSE
                    );
                }     
                //
                // Nothing else to do.
                //
                goto Done;
            }

            if (0 == RtlNumberGenericTableElements(SubjectPendingList))
            {
                IsRenewRequest = TRUE;
            }

            //
            // Get the reverse lease relationship ID from the subject pending list
            // SubjectPendingList would be cleared in LeaseRequestChangeState(...)
            //

            if (LEASE_STATE_ACTIVE != RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState)
            {
                LeaseRelationshipIdentifierReverse = GetReverseLeaseRelationshipIdentifier( SubjectPendingList );
            }

            //
            // Process state change.
            //
            LeaseRequestChangeState(
                RemoteLeaseAgentContext,
                SubjectPendingList,
                SubjectFailedPendingList,
                MonitorFailedPendingList,
                SubjectTerminatePendingList,
                &OutgoingSubjectPendingAcceptedList,
                &OutgoingSubjectPendingRejectedList,
                &OutgoingSubjectFailedAcceptedList,
                &OutgoingMonitorFailedAcceptedList,
                &OutgoingSubjectTerminateAcceptedList
                );

            if (SUSPENDED == RemoteLeaseAgentContext->State && 
                TRUE == RemoteLeaseAgentContext->IsInTwoWayTermination &&
                TRUE == IsRenewRequest)
            {
                //
                // RLA has sent out failed termination request, ignore the incoming renew request
                // We still process the establish request in case that multiple leasing app share one kernel LA
                // 
                //
                EventWriteRLAIgnoreRenewRequestInTwoWayTermination(
                    NULL,
                    RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                    RemoteLeaseAgentContext->Instance.QuadPart,
                    MessageIdentifier.QuadPart
                    );

                goto Done;
            }

            //
            // This is a lease establish request since the Monitor is INACTIVE. Accept it.
            //
            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState = LEASE_STATE_ACTIVE;

            //
            // Set the monitor identifier.
            //
            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier = LeaseInstance;

            // RECEIVED a lease establish request
            // Compare the incoming duration with local lease agent config
            // Grant a larger TTL if local lease agent config is greater
            GrantLargerDurations(
                RemoteLeaseAgentContext,
                &Duration,
                &LeaseSuspendDuration,
                &ArbitrationDuration,
                &Expiration
                );

            //
            // Check if the subject is active in order to reset its expiration timer.
            //
            if (LEASE_STATE_ACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState)
            {
                //
                // Expire time does not change but fail time changed.
                //
                SetSubjectExpireTime(
                    RemoteLeaseAgentContext->LeaseRelationshipContext,
                    RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectExpireTime,
                    LeaseSuspendDuration,
                    ArbitrationDuration
                    );

                //
                // Cancel the subject timer.
                //
                DequeueTimer(
                    RemoteLeaseAgentContext,
                    &RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectTimer
                    );

                DequeueTimer(
                    RemoteLeaseAgentContext,
                    &RemoteLeaseAgentContext->LeaseRelationshipContext->PreArbitrationSubjectTimer
                    );
                    
                //
                // Set the renew timer.
                //
                SetRenewTimer(RemoteLeaseAgentContext, FALSE, 0);
            }

            //
            // Send lease response.
            //
            LAssert(NULL == ResponseBuffer);

            SerializeLeaseMessage(
                RemoteLeaseAgentContext,
                ResponseType,
                NULL,
                NULL,
                &RemoteLeaseAgentContext->MonitorFailedPendingHashTable,
                &OutgoingSubjectPendingAcceptedList,
                &OutgoingSubjectPendingRejectedList,
                &OutgoingSubjectFailedAcceptedList,
                &OutgoingMonitorFailedAcceptedList,
                NULL,
                &OutgoingSubjectTerminateAcceptedList,
                Duration,
                Expiration,
                LeaseSuspendDuration,
                ArbitrationDuration,
                FALSE,
                &ResponseBuffer,
                &ResponseBufferSize
                );

            if (NULL != ResponseBuffer) 
            {
                OutgoingMsgId.QuadPart = ((PLEASE_MESSAGE_HEADER) ResponseBuffer)->MessageIdentifier.QuadPart;
            }

            EventWriteLeaseRelationSendingLease(
                NULL,
                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                RemoteLeaseAgentContext->Instance.QuadPart,
                RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
                GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
                GetMessageType(ResponseType),
                OutgoingMsgId.QuadPart
                ); 

            if (NULL != ResponseBuffer)
            {
                //
                // Note, here we use RemoteLeaseAgentContextMessage instead of RemoteLeaseAgentContext
                // Because the RemoteLeaseAgentContextMessage is where the message from, 
                // but not for the lease logic processing, where RemoteLeaseAgentContext is used
                //
                TransportSendBuffer(
                    RemoteLeaseAgentContextMessage->PartnerTarget,
                    ResponseBuffer,
                    ResponseBufferSize,
                    FALSE);
            }

            //
            // The reverse lease is only tried to be established if the subject is Inactive
            //
            if (LEASE_STATE_INACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState &&
                LeaseRelationshipIdentifierReverse != NULL )
            {
                EstablishReverseLease(
                    RemoteLeaseAgentContext,
                    LeaseRelationshipIdentifierReverse
                    );
            }

             // end of Monitor INACTIVE check
        }
        else
        {
            // This is a lease renew request.

            // Check for stale request.
            if (IS_LARGE_INTEGER_LESS_THAN_LARGE_INTEGER(LeaseInstance, RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier))
            {

                EventWriteProcessStaleRequest(
                    NULL,
                    RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                    RemoteLeaseAgentContext->Instance.QuadPart,
                    RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                    RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                    GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
                    GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
                    MessageIdentifier.QuadPart,
                    LeaseInstance.QuadPart,
                    RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart
                    );

                goto Done;
            }

            if (LEASE_STATE_ACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState &&
                0 != RtlNumberGenericTableElements(SubjectTerminatePendingList) &&
                DURATION_MAX_VALUE == Duration
                )
            {
                // Initiate the reverse lease termination on the condition:
                // if the sub is Active and incoming is termination request (duration is MAX)
                status = TerminateReverseLease(
                    RemoteLeaseAgentContext,
                    SubjectTerminatePendingList
                    );

                if (!NT_SUCCESS(status))
                {
                    goto Done;
                }

                TermReverseLease1 = TRUE;
            }

            //
            // Process state change.
            //
            LeaseRequestChangeState(
                RemoteLeaseAgentContext,
                SubjectPendingList,
                SubjectFailedPendingList,
                MonitorFailedPendingList,
                SubjectTerminatePendingList,
                &OutgoingSubjectPendingAcceptedList,
                &OutgoingSubjectPendingRejectedList,
                &OutgoingSubjectFailedAcceptedList,
                &OutgoingMonitorFailedAcceptedList,
                &OutgoingSubjectTerminateAcceptedList
                );

            //
            // Set the monitor identifier. It is possible that the other node has requested
            // a new relationship with the termination request for the previous one lost/delayed.
            //
            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier = LeaseInstance;

            //
            // Received a lease renew request, compare the incoming request with the config duration
            // Update the lease relationship context and the incoming Durations with the greater one 
            //
            GrantLargerDurations(
                RemoteLeaseAgentContext,
                &Duration,
                &LeaseSuspendDuration,
                &ArbitrationDuration,
                &Expiration
                );

            //
            // Send lease response.
            //
            LAssert(NULL == ResponseBuffer);
            SerializeLeaseMessage(
                RemoteLeaseAgentContext,
                ResponseType,
                NULL,
                NULL,
                &RemoteLeaseAgentContext->MonitorFailedPendingHashTable,
                &OutgoingSubjectPendingAcceptedList,
                &OutgoingSubjectPendingRejectedList,
                &OutgoingSubjectFailedAcceptedList,
                &OutgoingMonitorFailedAcceptedList,
                NULL,
                &OutgoingSubjectTerminateAcceptedList,
                Duration,
                Expiration,
                LeaseSuspendDuration,
                ArbitrationDuration,
                FALSE,
                &ResponseBuffer,
                &ResponseBufferSize
                );

            if (NULL != ResponseBuffer) 
            {
                OutgoingMsgId.QuadPart = ((PLEASE_MESSAGE_HEADER) ResponseBuffer)->MessageIdentifier.QuadPart;
            }

            if (DURATION_MAX_VALUE == Duration)
            {
                EventWriteLeaseRelationSendingTermination(
                    NULL,
                    RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                    RemoteLeaseAgentContext->Instance.QuadPart,
                    RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                    RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                    GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
                    GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
                    GetMessageType(ResponseType),
                    OutgoingMsgId.QuadPart
                    ); 
            }
            else
            {
                EventWriteLeaseRelationSendingLease(
                    NULL,
                    RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                    RemoteLeaseAgentContext->Instance.QuadPart,
                    RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                    RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                    GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
                    GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
                    GetMessageType(ResponseType),
                    OutgoingMsgId.QuadPart
                    );

            }

            if (NULL != ResponseBuffer)
            {
                if (!TermReverseLease1)
                {
                    TransportSendBuffer(
                        RemoteLeaseAgentContextMessage->PartnerTarget,
                        ResponseBuffer,
                        ResponseBufferSize,
                        FALSE);
                }
                else
                {
                    if (RemoteLeaseAgentContext->TransportBufferPending)
                    {
                        EventWriteTransportSendMultipleBuffersDropBuffer(
                            NULL,
                            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                            RemoteLeaseAgentContext->Instance.QuadPart,
                            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                            ((PLEASE_MESSAGE_HEADER)RemoteLeaseAgentContext->TransportBufferPending)->MessageIdentifier.QuadPart
                            );

                        ExFreePool(RemoteLeaseAgentContext->TransportBufferPending);
                        RemoteLeaseAgentContext->TransportBufferPending = NULL;
                        RemoteLeaseAgentContext->TransportBufferPendingSize = 0;
                    }

                    // Place to pending to send together with the next buffer
                    RemoteLeaseAgentContext->TransportBufferPending = ResponseBuffer;
                    RemoteLeaseAgentContext->TransportBufferPendingSize = ResponseBufferSize;
                }
            }

            // Send out reverse termination request if needed
            if (TermReverseLease1)
            {
                TerminateSubjectLeaseSendMessage(RemoteLeaseAgentContext, FALSE);
            }

        }

        if (LEASE_STATE_ACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
        {
            if (DURATION_MAX_VALUE == Duration)
            {
                //
                // update subject/renew timer when terminate the subject lease
                //
                UpdateMonitorTimerForTermination(RemoteLeaseAgentContext);

                //
                // Check the subject state.
                //
                if (LEASE_STATE_ACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState)
                {
                    //
                    // check if the termination request is due to subject failed
                    //
                    if (TRUE == IsTwoWayTermination &&
                        0 == RtlNumberGenericTableElements(&RemoteLeaseAgentContext->SubjectHashTable))
                    {
                        EventWriteReceivedTwoWayTermination(
                            NULL,
                            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                            RemoteLeaseAgentContext->Instance.QuadPart);

                        //
                        // Since it is two way termination, there is no subject on the remote side
                        // after processing the subject failed list, there should be no monitor on local side
                        //
                        if (0 != RtlNumberGenericTableElements(&RemoteLeaseAgentContext->MonitorHashTable))
                        {
                            EventWriteLeaseDriverTextTraceError(
                                NULL,
                                L"Monitor hash table is not empty for two way termination",
                                0);
                        }

                        //
                        // Update subject/renew timer when terminate the subject lease
                        //
                        UpdateSubjectTimerForTermination(RemoteLeaseAgentContext);
                        //
                        // This remote lease agent can now be deallocated.
                        //
                        SetRemoteLeaseAgentFailed(RemoteLeaseAgentContext);
                    }
                    else
                    {
                        //
                        // Expire time does not change but fail time changed.
                        //
                        SetSubjectExpireTime(
                            RemoteLeaseAgentContext->LeaseRelationshipContext,
                            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectExpireTime,
                            RemoteLeaseAgentContext->LeaseRelationshipContext->LeaseSuspendDuration,
                            RemoteLeaseAgentContext->LeaseRelationshipContext->ArbitrationDuration
                            );
        
                        //
                        // Enqueue the subject timer.
                        //
                        ArmTimer(
                            RemoteLeaseAgentContext,
                            &RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectTimer,
                            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectExpireTime,
                            &RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectTimerDpc
                            );
                    }

                }
                else
                {
                    //
                    // This remote lease agent can now be deallocated.
                    //
                    SetRemoteLeaseAgentFailed(RemoteLeaseAgentContext);
                }

            }
            else // Received a renew request and the duration is not MAX
            {
                GetCurrentTime(&RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorExpireTime);

                //
                // Monitor expire time should be updated by the granted duration.
                //
                LARGE_INTEGER_ADD_INTEGER(
                    RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorExpireTime,
                    Duration * MILLISECOND_TO_NANOSECOND_FACTOR
                    );

                //
                // Enqueue the monitor timer.
                //
                ArmTimer(
                    RemoteLeaseAgentContext,
                    &RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorTimer,
                    RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorExpireTime,
                    &RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorTimerDpc
                    );
                                
                //
                // Enqueue the prearbitration timer.
                //
                LARGE_INTEGER_SUBTRACT_INTEGER_AND_ASSIGN(PreArbitrationMonitorTime, 
                    RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorExpireTime, 
                    PRE_ARBITRATION_TIME);

                ArmTimer(
                    RemoteLeaseAgentContext,
                    &RemoteLeaseAgentContext->LeaseRelationshipContext->PreArbitrationMonitorTimer,
                    PreArbitrationMonitorTime,
                    &RemoteLeaseAgentContext->LeaseRelationshipContext->PreArbitrationMonitorTimerDpc
                    );
            }
        }
        
    }
    else if (IsReceivedResponse(MessageType))
    {
        //
        // Ignore response message if the lease instance is not match
        //
        if (!IS_LARGE_INTEGER_EQUAL_TO_LARGE_INTEGER(LeaseInstance, RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier)
            || IS_LARGE_INTEGER_GREATER_THAN_LARGE_INTEGER(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectExpireTime, Expiration))
        {
            goto Done;
        }

        //
        // If any non-pinged response received, set to renewed to true.
        //
        RemoteLeaseAgentContext->RenewedBefore = TRUE;
        //
        // The subject state has to be ACTIVE; otherwise, assert.
        //
        if (LEASE_STATE_ACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState)
        {
            if (RemoteLeaseAgentContext->LeaseRelationshipContext->IndirectLeaseCount != 0 && LEASE_RESPONSE == MessageType)
            {
                // Received a direct RESPONSE, reset the indirect lease count
                EventWriteIndirectLeaseCountReset(
                    NULL,
                    RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                    RemoteLeaseAgentContext->Instance.QuadPart,
                    MessageIdentifier.QuadPart,
                    RemoteLeaseAgentContext->LeaseRelationshipContext->IndirectLeaseCount
                    );

                RemoteLeaseAgentContext->LeaseRelationshipContext->IndirectLeaseCount = 0;
            }

            // Process state change.
            //
            LeaseResponseChangeState(
                RemoteLeaseAgentContext,
                SubjectPendingAcceptedList,
                SubjectPendingRejectedList,
                SubjectFailedAcceptedList,
                SubjectTerminateAcceptedList,
                MonitorFailedAcceptedList,
                MonitorFailedPendingList,
                &OutgoingMonitorFailedAcceptedList
                );

            //
            // Does the response terminate the subject relationship?
            //
            if (DURATION_MAX_VALUE == Duration)
            {
                //
                // update monitor timer when receive the termination ack for the subject lease
                //
                UpdateSubjectTimerForTermination(RemoteLeaseAgentContext);

            }
            else
            {
                // Receive a reponse, the granted duration/expiration may be greater than requested.
                // Update the lease relationship with the granted durations (This is the only place to UPDATE)
                SetLeaseRelationshipDuration(
                    RemoteLeaseAgentContext,
                    Duration,
                    LeaseSuspendDuration,
                    ArbitrationDuration);

                //
                // Set the subject time.
                //
                SetSubjectExpireTime(
                    RemoteLeaseAgentContext->LeaseRelationshipContext,
                    Expiration,
                    LeaseSuspendDuration,
                    ArbitrationDuration
                    );

                //
                // Set monitor timer.
                //
                if (LEASE_STATE_INACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState) {
                    
                    //
                    // Enqueue the subject timer.
                    //
                    ArmTimer(
                        RemoteLeaseAgentContext,
                        &RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectTimer,
                        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectExpireTime,
                        &RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectTimerDpc
                        );
                }

                //
                // Set renew timer.
                //
                SetRenewTimer(RemoteLeaseAgentContext, FALSE, 0);

                //
                // For the scenario that renew was retried, the pre-arbitration subject timer might be set. If retry succeeded, clear the pre-arbitration subject timer.
                //
                DequeueTimer(
                    RemoteLeaseAgentContext,
                    &RemoteLeaseAgentContext->LeaseRelationshipContext->PreArbitrationSubjectTimer
                    );
            }
        }

    }
    else
    {
        LAssert(FALSE);
    }
    
Done:

    if (DURATION_MAX_VALUE == Duration)
    {
        EventWriteLeaseRelationInStateAfterLeaseTermMsg(
            NULL,
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
            GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
            GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
            NULL,
            GetMessageType(MessageType),
            MessageIdentifier.QuadPart
            ); 
    }
    else
    {
        EventWriteLeaseRelationInStateAfterLeaseMsg(
            NULL,
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
            GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
            GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
            NULL,
            GetMessageType(MessageType),
            MessageIdentifier.QuadPart
            ); 
    }

    //
    // Clear lists.
    //
    ClearRelationshipIdentifierList(&OutgoingSubjectFailedAcceptedList);
    ClearRelationshipIdentifierList(&OutgoingSubjectPendingRejectedList);
    ClearRelationshipIdentifierList(&OutgoingSubjectPendingAcceptedList);
    ClearRelationshipIdentifierList(&OutgoingMonitorFailedAcceptedList);
    ClearRelationshipIdentifierList(&OutgoingSubjectTerminateAcceptedList);
}

PREMOTE_LEASE_AGENT_CONTEXT
FindMostRecentRemoteLeaseAgent(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContextMatch
    )

/*++

Routine Description:

    Retrieves most recent remote lease agent on this lease agent.

Arguments:

    LeaseAgentContext - channel that requires the remote lease agent.

    RemoteLeaseAgentContextMatch - matching remote lease agent.

Return Value:

    NULL if no remote lease agent was found.

--*/

{
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = NULL;
    PVOID IterRemoteLeaseAgent = NULL;

    //
    // Check to see if an existent remote lease agent can be reused.
    //
    for (IterRemoteLeaseAgent = RtlEnumerateGenericTable(
            &LeaseAgentContext->RemoteLeaseAgentContextHashTable, 
            TRUE);
         NULL != IterRemoteLeaseAgent;
         IterRemoteLeaseAgent = RtlEnumerateGenericTable(
            &LeaseAgentContext->RemoteLeaseAgentContextHashTable, 
            FALSE)) {

        //
        // Retrieve remote lease agent.
        //
        RemoteLeaseAgentContext = *((PREMOTE_LEASE_AGENT_CONTEXT*)IterRemoteLeaseAgent);

        //
        // Compare the remote lease agent address.
        //
        if (0 == wcscmp(
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier, 
            RemoteLeaseAgentContextMatch->RemoteLeaseAgentIdentifier))
        {

            if (TRUE == RemoteLeaseAgentContext->IsActive)
            {
                EventWriteFindActiveRLAInTable(
                    NULL,
                    RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                    RemoteLeaseAgentContext->Instance.QuadPart,
                    RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                    RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                    GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
                    GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
                    RemoteLeaseAgentContext->RemoteLeaseAgentInstance.QuadPart,
                    GetLeaseAgentState(RemoteLeaseAgentContext->State)
                    );

                return RemoteLeaseAgentContext;
            }
        }
    }
    
    return NULL;
}

PREMOTE_LEASE_AGENT_CONTEXT
FindArbitrationNeutralRemoteLeaseAgent(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContextMatch
)

/*++

Routine Description:

Retrieves remote lease agent with arbitration neutral set on this lease agent.

Arguments:

LeaseAgentContext - channel that requires the remote lease agent.

RemoteLeaseAgentContextMatch - matching remote lease agent.

Return Value:

NULL if no remote lease agent was found.

--*/

{
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = NULL;
    PVOID IterRemoteLeaseAgent = NULL;

    //
    // Check to see if a remote lease agent with arbitration neutral set exist 
    //
    for (IterRemoteLeaseAgent = RtlEnumerateGenericTable(
        &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
        TRUE);
        NULL != IterRemoteLeaseAgent;
        IterRemoteLeaseAgent = RtlEnumerateGenericTable(
            &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
            FALSE)) {
        //
        // Retrieve remote lease agent.
        //
        RemoteLeaseAgentContext = *((PREMOTE_LEASE_AGENT_CONTEXT*)IterRemoteLeaseAgent);

        //
        // Compare the remote lease agent address.
        //
        if (0 == wcscmp(
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContextMatch->RemoteLeaseAgentIdentifier))
        {
            //
            // Search for arbitration neutral remote lease agent
            //
            if (RemoteLeaseAgentContext->IsInArbitrationNeutral == TRUE)
            {
                EventWriteArbitrationNeutralRemoteLeaseAgentFound(
                    NULL,
                    RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                    RemoteLeaseAgentContext->Instance.QuadPart,
                    RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                    RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                    GetLeaseAgentState(RemoteLeaseAgentContext->State),
                    GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
                    GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
                    RemoteLeaseAgentContext->IsActive,
                    RemoteLeaseAgentContext->RemoteLeaseAgentInstance.QuadPart
                    );

                return RemoteLeaseAgentContext;
            }
        }
    }

    return NULL;
}

BOOLEAN
CheckForStaleMessage(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PLEASE_MESSAGE_HEADER LeaseMessageHeader)
/*++

Routine Description:

    the remote lease agent is retrieved from RLA table
    it has the same address as the incoming lease message.

Arguments:

    RemoteLeaseAgentContext - current active RLA in the table

    LeaseMessageHeader - lease message header.

Return Value:

    TRUE if stale message.

--*/

{
    // First step: check the remote side lease agent instance
    LAssert(RemoteLeaseAgentContext->RemoteLeaseAgentInstance.QuadPart != 0);

    if (IS_LARGE_INTEGER_LESS_THAN_LARGE_INTEGER(
        LeaseMessageHeader->RemoteLeaseAgentInstance,
        RemoteLeaseAgentContext->RemoteLeaseAgentInstance))
    {
        return TRUE;
    }

    if (IS_LARGE_INTEGER_GREATER_THAN_LARGE_INTEGER(
        LeaseMessageHeader->RemoteLeaseAgentInstance,
        RemoteLeaseAgentContext->RemoteLeaseAgentInstance))
    {
        return FALSE;
    }

    //in case that remote side lease agent instance is equal, compare the lease instance
    if (IsReceivedRequest(LeaseMessageHeader->MessageType))
    {
        if (IS_LARGE_INTEGER_LESS_THAN_LARGE_INTEGER(
            LeaseMessageHeader->LeaseInstance,
            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier))
        {
            return TRUE;
        }

        if (IS_LARGE_INTEGER_GREATER_THAN_LARGE_INTEGER(
            LeaseMessageHeader->LeaseInstance,
            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier))
        {
            return FALSE;
        }

        if (LEASE_STATE_EXPIRED <= RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState ||
            LEASE_STATE_EXPIRED <= RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
        {
            return TRUE;
        }

        //
        // For stale termination request, it cannot be ignored since the previous termition response may be lost
        // Another termination response will be sent
        // For stale renew request, just ignore
        //
        if (DURATION_MAX_VALUE != LeaseMessageHeader->Duration &&
            (IsRemoteLeaseAgentOrphaned(RemoteLeaseAgentContext) ||
            IsRemoteLeaseAgentFailed(RemoteLeaseAgentContext))
            )
        {
            return TRUE;
        }
    }
    else if (IsReceivedResponse(LeaseMessageHeader->MessageType))
    {
        if (!IS_LARGE_INTEGER_EQUAL_TO_LARGE_INTEGER(
            LeaseMessageHeader->LeaseInstance,
            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier) ||
            LEASE_STATE_ACTIVE != RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState)
        {
            EventWriteProcessStaleResponse(
                NULL,
                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                RemoteLeaseAgentContext->Instance.QuadPart,
                RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
                GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
                LeaseMessageHeader->MessageIdentifier.QuadPart,
                LeaseMessageHeader->LeaseInstance.QuadPart,
                RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart
                );

            return TRUE;
        }
    }
    else
    {
        LAssert(PING_REQUEST == LeaseMessageHeader->MessageType || PING_RESPONSE == LeaseMessageHeader->MessageType);
    }

    return FALSE;

}
NTSTATUS
GetOrCreateRemoteLeaseAgent(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in PLEASE_MESSAGE_HEADER LeaseMessageHeader,
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress,
    __out PREMOTE_LEASE_AGENT_CONTEXT * result
    )
    
/*++

Routine Description:

    Retrieves an existent remote lease agent or creates a new one for
    the incoming lease message that needs to be processed.

Arguments:

    LeaseAgentContext - lease agent message was received by

    LeaseMessageHeader - lease message header.

Return Value:

    NULL if error or if remote lease agent is already failed.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContextIter = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT* RemoteLeaseAgentContextExistent = NULL;
    BOOLEAN Inserted = FALSE;
    BOOLEAN Found = FALSE;
    BOOLEAN IsStaleMessage = FALSE;

    //
    // Check the lease agent state.
    //
    if (IsLeaseAgentFailed(LeaseAgentContext))
    {
        EventWriteDropingLeaseMsgLAFailed(
            NULL,
            GetMessageType(LeaseMessageHeader->MessageType),
            TransportIdentifier(LeaseAgentContext->Transport),
            LeaseAgentContext->Instance.QuadPart
            ); 

        //
        // Nothing to do at this point.
        //
        *result = NULL;
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Attempt to find the remote lease agent for the lease agent address
    // that is found in the message header.
    //
    Status = RemoteLeaseAgentConstructor(
        LeaseAgentContext,
        RemoteSocketAddress,
        &RemoteLeaseAgentContext
        );

    if (NT_SUCCESS(Status))
    {
        //
        // Check to see if an existent remote lease agent can be reused.
        //
        RemoteLeaseAgentContextIter = FindMostRecentRemoteLeaseAgent(
            LeaseAgentContext,
            RemoteLeaseAgentContext
            );

        //
        // This block is to check if we have received an lease message from the old lease relationship.
        // When the lease relationship received an arbitration neutral result, and new lease was created,
        // we could receive an old lease message from the old lease relationship. We have to stale such
        // message, or there could be inconsistent issue, that old lease identifier being stored in new
        // lease relationship.
        //
        if (RemoteLeaseAgentContextIter != NULL &&
            IsReceivedRequest(LeaseMessageHeader->MessageType) &&
            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState == LEASE_STATE_INACTIVE)
        {
            PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContextArbitrationNeutral = NULL;

            RemoteLeaseAgentContextArbitrationNeutral = FindArbitrationNeutralRemoteLeaseAgent(
                LeaseAgentContext,
                RemoteLeaseAgentContext
                );

            if (RemoteLeaseAgentContextArbitrationNeutral != NULL)
            {
                if (IS_LARGE_INTEGER_EQUAL_TO_LARGE_INTEGER(
                        LeaseMessageHeader->LeaseInstance,
                        RemoteLeaseAgentContextArbitrationNeutral->LeaseRelationshipContext->MonitorIdentifier))
                {
                    //
                    // If the lease instance match the one from the arbitration neutral remote lease agent,
                    // we know the message was from the previous lease relationship.
                    //
                    EventWriteProcessArbitrationNeutralStaleMessage(
                        NULL,
                        RemoteLeaseAgentContextArbitrationNeutral->RemoteLeaseAgentIdentifier,
                        RemoteLeaseAgentContextArbitrationNeutral->Instance.QuadPart,
                        RemoteLeaseAgentContextArbitrationNeutral->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                        RemoteLeaseAgentContextArbitrationNeutral->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                        GetLeaseAgentState(RemoteLeaseAgentContextArbitrationNeutral->State),
                        GetLeaseState(RemoteLeaseAgentContextArbitrationNeutral->LeaseRelationshipContext->SubjectState),
                        GetLeaseState(RemoteLeaseAgentContextArbitrationNeutral->LeaseRelationshipContext->MonitorState),
                        RemoteLeaseAgentContextArbitrationNeutral->IsActive,
                        RemoteLeaseAgentContextArbitrationNeutral->RemoteLeaseAgentInstance.QuadPart,
                        LeaseMessageHeader->MessageIdentifier.QuadPart,
                        GetMessageType(LeaseMessageHeader->MessageType),
                        LeaseMessageHeader->LeaseInstance.QuadPart
                        );

                    //
                    // Remove the newly created RLA
                    //
                    RemoteLeaseAgentDestructor(RemoteLeaseAgentContext);

                    *result = NULL;
                    return STATUS_UNSUCCESSFUL;
                }
                else
                {
                    EventWriteProcessNonArbitrationNeutralStaleMessage(
                        NULL,
                        RemoteLeaseAgentContextArbitrationNeutral->RemoteLeaseAgentIdentifier,
                        RemoteLeaseAgentContextArbitrationNeutral->Instance.QuadPart,
                        RemoteLeaseAgentContextArbitrationNeutral->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                        RemoteLeaseAgentContextArbitrationNeutral->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                        GetLeaseAgentState(RemoteLeaseAgentContextArbitrationNeutral->State),
                        GetLeaseState(RemoteLeaseAgentContextArbitrationNeutral->LeaseRelationshipContext->SubjectState),
                        GetLeaseState(RemoteLeaseAgentContextArbitrationNeutral->LeaseRelationshipContext->MonitorState),
                        RemoteLeaseAgentContextArbitrationNeutral->IsActive,
                        RemoteLeaseAgentContextArbitrationNeutral->RemoteLeaseAgentInstance.QuadPart,
                        LeaseMessageHeader->MessageIdentifier.QuadPart,
                        GetMessageType(LeaseMessageHeader->MessageType),
                        LeaseMessageHeader->LeaseInstance.QuadPart
                        );
                }
            }
        }

        if (NULL != RemoteLeaseAgentContextIter && 0 == RemoteLeaseAgentContextIter->RemoteLeaseAgentInstance.QuadPart)
        {
            RemoteLeaseAgentContextIter->RemoteLeaseAgentInstance.QuadPart = LeaseMessageHeader->RemoteLeaseAgentInstance.QuadPart;
        }

        if (NULL != RemoteLeaseAgentContextIter)
        {
            IsStaleMessage = CheckForStaleMessage(
                RemoteLeaseAgentContextIter,
                LeaseMessageHeader);

            if (TRUE == IsStaleMessage)
            {
                // stale message trace
                if (LeaseMessageHeader->MessageType == PING_REQUEST || LeaseMessageHeader->MessageType == PING_RESPONSE)
                {
                    EventWriteProcessStalePing(
                        NULL,
                        RemoteLeaseAgentContextIter->RemoteLeaseAgentIdentifier,
                        RemoteLeaseAgentContextIter->Instance.QuadPart,
                        RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                        RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                        GetLeaseState(RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectState),
                        GetLeaseState(RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorState),
                        GetMessageType(LeaseMessageHeader->MessageType),
                        LeaseMessageHeader->MessageIdentifier.QuadPart,
                        LeaseMessageHeader->LeaseInstance.QuadPart
                        );
                }
                else if (IsReceivedRequest(LeaseMessageHeader->MessageType))
                {
                    EventWriteProcessStaleRequest(
                        NULL,
                        RemoteLeaseAgentContextIter->RemoteLeaseAgentIdentifier,
                        RemoteLeaseAgentContextIter->Instance.QuadPart,
                        RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                        RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                        GetLeaseState(RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectState),
                        GetLeaseState(RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorState),
                        LeaseMessageHeader->MessageIdentifier.QuadPart,
                        LeaseMessageHeader->LeaseInstance.QuadPart,
                        RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorIdentifier.QuadPart
                        );
                }
                else if (IsReceivedResponse(LeaseMessageHeader->MessageType))
                {
                    EventWriteProcessStaleResponse(
                        NULL,
                        RemoteLeaseAgentContextIter->RemoteLeaseAgentIdentifier,
                        RemoteLeaseAgentContextIter->Instance.QuadPart,
                        RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                        RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                        GetLeaseState(RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectState),
                        GetLeaseState(RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorState),
                        LeaseMessageHeader->MessageIdentifier.QuadPart,
                        LeaseMessageHeader->LeaseInstance.QuadPart,
                        RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectIdentifier.QuadPart
                        );
                }

                //
                // Remove the newly created RLA
                //
                RemoteLeaseAgentDestructor(RemoteLeaseAgentContext);

                *result = NULL;
                return STATUS_UNSUCCESSFUL;
            }
        }

        //
        // Decide if a new remote lease agent is to be created.
        //
        if (NULL != RemoteLeaseAgentContextIter)
        {
            if (IS_LARGE_INTEGER_GREATER_THAN_LARGE_INTEGER(
                LeaseMessageHeader->RemoteLeaseAgentInstance,
                RemoteLeaseAgentContextIter->RemoteLeaseAgentInstance))
            {
                // current Active RLA has a older lease agent remote instance
                // the Found status will be false
                EventWriteLeaseAgentRemoteInstanceGreater(
                    NULL,
                    RemoteLeaseAgentContextIter->RemoteLeaseAgentIdentifier,
                    RemoteLeaseAgentContextIter->Instance.QuadPart,
                    RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                    RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                    GetLeaseState(RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectState),
                    GetLeaseState(RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorState),
                    RemoteLeaseAgentContextIter->RemoteLeaseAgentInstance.QuadPart,
                    LeaseMessageHeader->RemoteLeaseAgentInstance.QuadPart
                    );

                // If the state = suspended, subject and monitor are expired, we know we were in arbitration state
                if (RemoteLeaseAgentContextIter->State != SUSPENDED ||
                    RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectState != LEASE_STATE_EXPIRED ||
                    RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorState != LEASE_STATE_EXPIRED)
                {
                    RemoteLeaseAgentDisconnect(RemoteLeaseAgentContextIter, TRUE);

                    EventWriteDisconnectFailedRemoteLA(
                        NULL,
                        RemoteLeaseAgentContextIter->RemoteLeaseAgentIdentifier,
                        RemoteLeaseAgentContextIter->Instance.QuadPart,
                        RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                        RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                        GetLeaseState(RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectState),
                        GetLeaseState(RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorState),
                        RemoteLeaseAgentContextIter->RemoteLeaseAgentInstance.QuadPart
                        );
                }
                else
                {
                    EventWriteRemoteLeaseAgentSuspendedDropMessage(
                        NULL,
                        RemoteLeaseAgentContextIter->RemoteLeaseAgentIdentifier,
                        RemoteLeaseAgentContextIter->Instance.QuadPart,
                        RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                        RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                        GetLeaseState(RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectState),
                        GetLeaseState(RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorState),
                        LeaseMessageHeader->MessageIdentifier.QuadPart
                        );

                    //
                    // Remove the newly created RLA
                    //
                    RemoteLeaseAgentDestructor(RemoteLeaseAgentContext);
                    RemoteLeaseAgentContext = NULL;

                    *result = NULL;
                    return STATUS_UNSUCCESSFUL;
                }
            }
            else if (IsRemoteLeaseAgentOrphaned(RemoteLeaseAgentContextIter))
            {
                RemoteLeaseAgentDisconnect(RemoteLeaseAgentContextIter, FALSE);

                EventWriteDisconnectFailedRemoteLA(
                    NULL,
                    RemoteLeaseAgentContextIter->RemoteLeaseAgentIdentifier,
                    RemoteLeaseAgentContextIter->Instance.QuadPart,
                    RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                    RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                    GetLeaseState(RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectState),
                    GetLeaseState(RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorState),
                    RemoteLeaseAgentContextIter->RemoteLeaseAgentInstance.QuadPart
                    );

                //
                // When lease received ping request without a target remote lease agent in the lease agent, lease creates a
                // remote lease agent that isn't active. This remote lease agent is considered orphaned. When the remote sends
                // the renew request, it would destruct the orphaned remtoe lease agent and recreate one. Lease needs to copy
                // over the ping send time from the orphaned remote lease agent to the new one. Lease needs this information
                // for the arbitration.
                //
                RemoteLeaseAgentContext->PingSendTime = RemoteLeaseAgentContextIter->PingSendTime;
            }
            else if (IsRemoteLeaseAgentFailed(RemoteLeaseAgentContextIter))
            {
                // current Active RLA is failed or orphaned
                // the Found status will be false
                RemoteLeaseAgentDisconnect(RemoteLeaseAgentContextIter, FALSE);

                EventWriteDisconnectFailedRemoteLA(
                    NULL,
                    RemoteLeaseAgentContextIter->RemoteLeaseAgentIdentifier,
                    RemoteLeaseAgentContextIter->Instance.QuadPart,
                    RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                    RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                    GetLeaseState(RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectState),
                    GetLeaseState(RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorState),
                    RemoteLeaseAgentContextIter->RemoteLeaseAgentInstance.QuadPart
                    );
            }
            else
            {
                // current Active RLA is usable
                Found = TRUE;
            } 
        }

        if (Found)
        {
            //
            // Deallocate newly created remote lease agent.
            //
            RemoteLeaseAgentDestructor(RemoteLeaseAgentContext);
            RemoteLeaseAgentContext = NULL;

            //
            // Check to see if the remote lease agent has had its incoming channel set.
            //
            if (NULL == RemoteLeaseAgentContextIter->PartnerTarget)
            {
                LAssert(RemoteLeaseAgentContextIter->LeaseAgentContext == LeaseAgentContext);

                TransportResolveTarget(
                    RemoteLeaseAgentContextIter->LeaseAgentContext->Transport,
                    &RemoteLeaseAgentContextIter->RemoteSocketAddress,
                    &RemoteLeaseAgentContextIter->PartnerTarget
                    );

                // trace only if resolve target is successful
                if (RemoteLeaseAgentContextIter->PartnerTarget)
                {
                    EventWriteIncomingChnlWithRemoteLA(
                        NULL,
                        TransportTargetIdentifier(RemoteLeaseAgentContextIter->PartnerTarget),
                        RemoteLeaseAgentContextIter->RemoteLeaseAgentIdentifier,
                        RemoteLeaseAgentContextIter->Instance.QuadPart
                        ); 
                }
            }
            
            //
            // We use the existent remote lease agent for processing.
            //
            RemoteLeaseAgentContext = RemoteLeaseAgentContextIter;

        }
        else
        {

            //
            // Did not find available RLA in the table
            // Insert the newly created remote lease agent in the local lease agent hash table.
            //
            RemoteLeaseAgentContextExistent = RtlInsertElementGenericTable(
                &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
                &RemoteLeaseAgentContext,
                sizeof(PREMOTE_LEASE_AGENT_CONTEXT),
                &Inserted
                );
            
            //
            // Check insert result.
            //
            if (NULL == RemoteLeaseAgentContextExistent) {
            
                LAssert(!Inserted);

                EventWriteRegisterRemoteLAFail(
                    NULL,
                    RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                    RemoteLeaseAgentContext->Instance.QuadPart,
                    TransportIdentifier(LeaseAgentContext->Transport),
                    LeaseAgentContext->Instance.QuadPart
                    ); 

                //
                // Destroy lease agent.
                //
                RemoteLeaseAgentDestructor(RemoteLeaseAgentContext);
                RemoteLeaseAgentContext = NULL;
            
                //
                // Hash table memory allocation failed.
                //
                Status = STATUS_INSUFFICIENT_RESOURCES;
                
            } else if (*RemoteLeaseAgentContextExistent != RemoteLeaseAgentContext) {

                //
                // This can only happen if the remote lease agents are created
                // so fast that their instance ids are the same. This should be 
                // a very rare situation.
                //
                LAssert(!Inserted);

                EventWriteRemoteLAExist(
                    NULL,
                    RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                    RemoteLeaseAgentContext->Instance.QuadPart,
                    TransportIdentifier(LeaseAgentContext->Transport),
                    LeaseAgentContext->Instance.QuadPart
                    ); 
                
                //
                // Destroy lease agent.
                //
                RemoteLeaseAgentDestructor(RemoteLeaseAgentContext);
                RemoteLeaseAgentContext = NULL;
                //
                // Instances not unique enough.
                //
                Status = STATUS_INVALID_PARAMETER;
                
            } else {

                //
                // A new remote lease agent was created.
                //
                LAssert(Inserted);

                EventWriteRegisterRemoteLA(
                    NULL,
                    RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                    RemoteLeaseAgentContext->Instance.QuadPart,
                    TransportIdentifier(LeaseAgentContext->Transport),
                    LeaseAgentContext->Instance.QuadPart
                    ); 

                //
                // Done.
                //
                Status = STATUS_SUCCESS;
            }
        }

        if (!Found && NT_SUCCESS(Status))
        {
            //
            // Set the lease agent for this newly created remote lease agent.
            //
            RemoteLeaseAgentContext->LeaseAgentContext = LeaseAgentContext;

            if (NULL != RemoteLeaseAgentContextIter)
            {
                RemoteLeaseAgentContextIter->IsActive = FALSE;

                SetRemoteLeaseAgentFailed(RemoteLeaseAgentContextIter);
            }

            //
            // Set the channel for this remote lease agent.
            //
            TransportResolveTarget(
                RemoteLeaseAgentContext->LeaseAgentContext->Transport,
                &RemoteLeaseAgentContext->RemoteSocketAddress,
                &RemoteLeaseAgentContext->PartnerTarget
                );

            // trace only if resolve target is successful
            if (RemoteLeaseAgentContext->PartnerTarget)
            {
                EventWriteIncomingChnlWithRemoteLA(
                    NULL,
                    TransportTargetIdentifier(RemoteLeaseAgentContext->PartnerTarget),
                    RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                    RemoteLeaseAgentContext->Instance.QuadPart
                    );
            }
        }
    }
    else
    {
        // remote lease agent contruction fail
        // NULL would be returned
        EventWriteCreateRemoteLAFail(NULL); 
        RemoteLeaseAgentContext = NULL;
    }

    if (NT_SUCCESS(Status) && NULL != RemoteLeaseAgentContext)
    {
        // Update the remote version 
        RemoteLeaseAgentContext->RemoteVersion = GetLeaseAgentVersion(LeaseMessageHeader->MajorVersion, LeaseMessageHeader->MinorVersion);
    }

    *result = RemoteLeaseAgentContext;
    return Status;
}

USHORT GetLeaseAgentVersion(USHORT majorVersion, USHORT minorVersion)
{
    USHORT leaseAgentVersion = majorVersion;
    leaseAgentVersion <<= 8;
    leaseAgentVersion |= minorVersion;
    return leaseAgentVersion;
}

NTSTATUS
ProcessLeaseMessageBuffer(
    __in PLEASE_TRANSPORT Listener,
    __in PTRANSPORT_SENDTARGET Target,
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in PVOID Buffer,
    __in ULONG BufferSize
    )
    
/*++

Routine Description:

    Processes the next buffer on the remote lease agent. It first expects
    the fixed size lease message header to be received, then the actual 
    lease message body. Both will be verified for correctness before any 
    action is to be taken in the lease layer.

Arguments:

    LeaseAgentContext - lease agent on which the message was received.

    Buffer - buffer received.

    BufferSize - buffer size received.

Return Value:

    n/a

--*/

{
    UNREFERENCED_PARAMETER(Target);

    NTSTATUS Status = STATUS_SUCCESS;
    RTL_GENERIC_TABLE SubjectPendingList;
    RTL_GENERIC_TABLE SubjectFailedPendingList;
    RTL_GENERIC_TABLE MonitorFailedPendingList;
    RTL_GENERIC_TABLE SubjectPendingAcceptedList;
    RTL_GENERIC_TABLE SubjectPendingRejectedList;
    RTL_GENERIC_TABLE SubjectFailedAcceptedList;
    RTL_GENERIC_TABLE MonitorFailedAcceptedList;
    RTL_GENERIC_TABLE SubjectTerminatePendingList;
    RTL_GENERIC_TABLE SubjectTerminateAcceptedList;

    PTRANSPORT_LISTEN_ENDPOINT RemoteMessageSocketAddress = NULL;
    PTRANSPORT_LISTEN_ENDPOINT RemoteLeaseSocketAddress = NULL;

    PLEASE_MESSAGE_EXT MessageExtension = NULL;

    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContextMessage = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContextLease = NULL;

    PLEASE_MESSAGE_HEADER LeaseMessageHeader = NULL;
    KIRQL OldIrql;

    //
    // Validate header. If incorrect, drop message
    //
    Status = IsValidMessageHeader(Buffer, BufferSize);
    if (!NT_SUCCESS(Status))
    {
        EventWriteInvalidMessage(NULL, 0);
        return Status;
    }

    LeaseMessageHeader = (PLEASE_MESSAGE_HEADER)Buffer;

    if (LeaseMessageHeader->MessageType == FORWARD_REQUEST ||
        LeaseMessageHeader->MessageType == FORWARD_RESPONSE)
    {
        Status = ProcessForwardMessage(
            LeaseAgentContext,
            Buffer,
            BufferSize);

        return Status;
    }

    //
    // Check message size.
    //
    if (LeaseMessageHeader->MessageSize != BufferSize)
    {
        EventWriteInvalidMessage(NULL, 1);
        return STATUS_INVALID_PARAMETER;
    }

    // Allocate memory for message/lease endpoint.
    RemoteMessageSocketAddress = LeaseListenEndpointConstructor();
    if (RemoteMessageSocketAddress == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RemoteLeaseSocketAddress = LeaseListenEndpointConstructor();
    if (RemoteLeaseSocketAddress == NULL)
    {
        LeaseListenEndpointDestructor(RemoteMessageSocketAddress);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    MessageExtension = MessageExtensionConstructor();
    if (MessageExtension == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize lists.
    //
    RtlInitializeGenericTable(
        &SubjectPendingList,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareLeaseRelationshipContexts,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL
        );
    RtlInitializeGenericTable(
        &SubjectFailedPendingList,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareLeaseRelationshipContexts,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL
        );
    RtlInitializeGenericTable(
        &MonitorFailedPendingList,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareLeaseRelationshipContexts,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL
        );
    RtlInitializeGenericTable(
        &SubjectPendingAcceptedList,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareLeaseRelationshipContexts,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL
        );
    RtlInitializeGenericTable(
        &SubjectPendingRejectedList,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareLeaseRelationshipContexts,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL
        );
    RtlInitializeGenericTable(
        &SubjectFailedAcceptedList,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareLeaseRelationshipContexts,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL
        );
    RtlInitializeGenericTable(
        &MonitorFailedAcceptedList,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareLeaseRelationshipContexts,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL
        );
    RtlInitializeGenericTable(
        &SubjectTerminatePendingList,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareLeaseRelationshipContexts,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL
        );
    RtlInitializeGenericTable(
        &SubjectTerminateAcceptedList,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareLeaseRelationshipContexts,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL
        );

    //
    // Get message body.
    //
    Status = DeserializeLeaseMessage(
        LeaseMessageHeader,
        (PBYTE)Buffer + LeaseMessageHeader->MessageHeaderSize,
        &SubjectPendingList,
        &SubjectFailedPendingList,
        &MonitorFailedPendingList,
        &SubjectPendingAcceptedList,
        &SubjectPendingRejectedList,
        &SubjectFailedAcceptedList,
        &MonitorFailedAcceptedList,
        &SubjectTerminatePendingList,
        &SubjectTerminateAcceptedList,
        RemoteMessageSocketAddress,
        RemoteLeaseSocketAddress,
        MessageExtension
        );

    if (!NT_SUCCESS(Status))
    {
        EventWriteDeserializationError(NULL,
            GetMessageType(LeaseMessageHeader->MessageType),
            LeaseMessageHeader->MessageIdentifier.QuadPart
            );

        // all the lists and listen endpoin were cleaned up in DeserializeLeaseMessage
        // just return here and no need to worry about the leak
        //
        return Status;
    }

    if ((LeaseMessageHeader->MessageType == RELAY_REQUEST || LeaseMessageHeader->MessageType == RELAY_RESPONSE) &&
        !IsValidListenEndpoint(RemoteLeaseSocketAddress))
    {
        EventWriteLeaseDriverTextTraceError(
            NULL,
            L"Relay Message: Deserialize lease listent endpoint error ",
            Status);
    }

    PTRANSPORT_LISTEN_ENDPOINT listenerEndpoint = (PTRANSPORT_LISTEN_ENDPOINT)TransportListenEndpoint(Listener);

    if (PING_REQUEST == LeaseMessageHeader->MessageType)
    {
        if (IsBlockTransport(RemoteMessageSocketAddress, listenerEndpoint, LEASE_PING_REQUEST))
        {
            goto Done;
        }
    }
    else if (PING_RESPONSE == LeaseMessageHeader->MessageType)
    {
        if (IsBlockTransport(RemoteMessageSocketAddress, listenerEndpoint, LEASE_PING_RESPONSE))
        {
            goto Done;
        }
    }
    else if (IsReceivedRequest(LeaseMessageHeader->MessageType))
    {
        if (LeaseMessageHeader->MessageType == RELAY_REQUEST && IsBlockTransport(RemoteMessageSocketAddress, listenerEndpoint, LEASE_INDIRECT))
        {
            goto Done;
        }

        if (LeaseMessageHeader->MessageType == LEASE_REQUEST && IsBlockTransport(RemoteMessageSocketAddress, listenerEndpoint, LEASE_ESTABLISH_ACTION))
        {
            goto Done;
        }
    }
    else if (IsReceivedResponse(LeaseMessageHeader->MessageType))
    {
        if (LeaseMessageHeader->MessageType == LEASE_RESPONSE && IsBlockTransport(RemoteMessageSocketAddress, listenerEndpoint, LEASE_ESTABLISH_RESPONSE))
        {
            goto Done;
        }
    }

    LAssert(NULL != LeaseAgentContext);
    KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

    //
    // Retrieve the remote lease agent context for this channel.
    // One is created if it does not exist. If a remote lease agent
    // could not be created (insufficient resorces for instance), 
    // then we can still continue to attempt to further receive
    // on this channel.
    //

    if (!IS_LARGE_INTEGER_EQUAL_TO_INTEGER(MessageExtension->MsgLeaseAgentInstance, 0) && 
        IS_LARGE_INTEGER_LESS_THAN_LARGE_INTEGER(MessageExtension->MsgLeaseAgentInstance, LeaseAgentContext->Instance)
        )
    {
        // Incoming message use a instance less than the current lease agent instance
        // This is a stale message

        Status = STATUS_UNSUCCESSFUL;

        EventWriteReceivedStaleMessage(
            NULL,
            TransportIdentifier(LeaseAgentContext->Transport),
            LeaseAgentContext->Instance.QuadPart,
            LeaseMessageHeader->MessageIdentifier.QuadPart,
            MessageExtension->MsgLeaseAgentInstance.QuadPart
            );

    }
    else if (LeaseMessageHeader->MessageType == RELAY_REQUEST ||
        LeaseMessageHeader->MessageType == RELAY_RESPONSE)
    {

        RemoteLeaseAgentContextMessage = GetCurrentActiveRemoteLeaseAgent(
            LeaseAgentContext,
            RemoteMessageSocketAddress
            );

        Status = GetOrCreateRemoteLeaseAgent(
            LeaseAgentContext,
            LeaseMessageHeader,
            RemoteLeaseSocketAddress,
            &RemoteLeaseAgentContextLease
            );

        if (!NT_SUCCESS(Status))
        {
            EventWriteLeaseDriverTextTraceError(
                NULL,
                L"Did not find Active RLA for Relay messages ",
                Status);
        }
    }
    else
    {
        Status = GetOrCreateRemoteLeaseAgent(
            LeaseAgentContext,
            LeaseMessageHeader,
            RemoteMessageSocketAddress,
            &RemoteLeaseAgentContextMessage
            );

        if (!NT_SUCCESS(Status))
        {
            EventWriteLeaseDriverTextTraceError(
                NULL,
                L"Did not find Active RLA for messages ",
                Status);
        }

        RemoteLeaseAgentContextLease = RemoteLeaseAgentContextMessage;
    }

    //
    // Process message if possible.
    //
    if (NT_SUCCESS(Status) &&
        NULL != RemoteLeaseAgentContextMessage && 
        NULL != RemoteLeaseAgentContextLease)
    {
        //
        // Process message body.
        //
        ProcessLeaseMessage(
            RemoteLeaseAgentContextLease,
            RemoteLeaseAgentContextMessage,
            LeaseMessageHeader->RemoteLeaseAgentInstance,
            LeaseMessageHeader->MessageIdentifier,
            LeaseMessageHeader->MessageType,
            LeaseMessageHeader->LeaseInstance,
            LeaseMessageHeader->Duration,
            LeaseMessageHeader->Expiration,
            LeaseMessageHeader->LeaseSuspendDuration,
            LeaseMessageHeader->ArbitrationDuration,
            LeaseMessageHeader->IsTwoWayTermination,
            &SubjectPendingList,
            &SubjectFailedPendingList,
            &MonitorFailedPendingList,
            &SubjectPendingAcceptedList,
            &SubjectPendingRejectedList,
            &SubjectFailedAcceptedList,
            &MonitorFailedAcceptedList,
            &SubjectTerminatePendingList,
            &SubjectTerminateAcceptedList
            );
    }

    KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);

Done:
    //
    // Clear lists.
    //
    ClearRelationshipIdentifierList(&SubjectPendingList);
    ClearRelationshipIdentifierList(&SubjectFailedPendingList);
    ClearRelationshipIdentifierList(&MonitorFailedPendingList);
    ClearRelationshipIdentifierList(&SubjectPendingAcceptedList);
    ClearRelationshipIdentifierList(&SubjectPendingRejectedList);
    ClearRelationshipIdentifierList(&SubjectFailedAcceptedList);
    ClearRelationshipIdentifierList(&MonitorFailedAcceptedList);
    ClearRelationshipIdentifierList(&SubjectTerminatePendingList);
    ClearRelationshipIdentifierList(&SubjectTerminateAcceptedList);

    LeaseListenEndpointDestructor(RemoteMessageSocketAddress);
    LeaseListenEndpointDestructor(RemoteLeaseSocketAddress);

    MessageExtensionDestructor(MessageExtension);

    return Status;
}

VOID
UpdateSubjectTimerForTermination(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    )
    
/*++

Routine Description:

    Update subject and renew timer when terminate a lease

Arguments:

    RemoteLeaseAgentContext - remote lease agent

Return Value:

    n/a

--*/

{
    LARGE_INTEGER TerminateExpiration;
    TerminateExpiration.QuadPart = MAXLONGLONG;

    //
    // the subject lease is terminated.
    //
    RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState = LEASE_STATE_INACTIVE;

    //
    // Set the subject timer.
    //
    SetSubjectExpireTime(
        RemoteLeaseAgentContext->LeaseRelationshipContext,
        TerminateExpiration,
        DURATION_MAX_VALUE,
        DURATION_MAX_VALUE
        );

    //
    // Cancel the renew timer.
    //
    DequeueTimer(
        RemoteLeaseAgentContext,
        &RemoteLeaseAgentContext->LeaseRelationshipContext->RenewOrArbitrateTimer
        );

    //
    // Cancel the prearbitration subject timer.
    //
    DequeueTimer(
        RemoteLeaseAgentContext,
        &RemoteLeaseAgentContext->LeaseRelationshipContext->PreArbitrationMonitorTimer
        );
        
    //
    // Cancel the prearbitration monitor timer.
    //
    DequeueTimer(
        RemoteLeaseAgentContext,
        &RemoteLeaseAgentContext->LeaseRelationshipContext->PreArbitrationSubjectTimer
        );

    //
    // Cancel the subject timer.
    //
    DequeueTimer(
        RemoteLeaseAgentContext,
        &RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectTimer
        );

    //
    // Cancel the ping retry timer.
    //
    DequeueTimer(RemoteLeaseAgentContext, &RemoteLeaseAgentContext->LeaseRelationshipContext->PingRetryTimer);


    if (LEASE_STATE_ACTIVE != RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
    {
        //
        // This remote lease agent can now be deallocated.
        //
        SetRemoteLeaseAgentFailed(RemoteLeaseAgentContext);
    }
    else
    {
        //
        // Reset the state of the remote lease agent to open.
        // At this time new subject relationships can be accepted.
        //
        if (IsRemoteLeaseAgentSuspended(RemoteLeaseAgentContext))
        {
            //
            // Set the state of the remote agent back to open.
            //
            SetRemoteLeaseAgentState(RemoteLeaseAgentContext, OPEN);

            if (TRUE == RemoteLeaseAgentContext->IsInTwoWayTermination)
            {
                RemoteLeaseAgentContext->IsInTwoWayTermination = FALSE;
            }
        }
    }

}

VOID
UpdateMonitorTimerForTermination(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext
    )
    
/*++

Routine Description:

    Update subject and renew timer when terminate a lease

Arguments:

    RemoteLeaseAgentContext - remote lease agent

Return Value:

    n/a

--*/

{

    //
    // the Monitor lease is terminated.
    //
    RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState = LEASE_STATE_INACTIVE;

    //
    // Cancel the monitor timer.
    //
    DequeueTimer(
        RemoteLeaseAgentContext,
        &RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorTimer
        );
}

NTSTATUS
TerminateReverseLease(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PRTL_GENERIC_TABLE SubjectTerminatePendingList
    )
{

    PVOID Iter = NULL;
    PVOID IterSubject = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifierIncoming = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifierSubject = NULL;

    EventWriteTermReverseLease(
        NULL,
        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
        RemoteLeaseAgentContext->Instance.QuadPart
        );

    //
    // Iterate over subject failed pending list.
    //
    for (Iter = RtlEnumerateGenericTable ( SubjectTerminatePendingList, TRUE );
        Iter != NULL;
        Iter = RtlEnumerateGenericTable ( SubjectTerminatePendingList, FALSE ))
    {
        //
        // Retrieve lease relationship identifier from the failed pending list.
        //
        LeaseRelationshipIdentifierIncoming = *((PLEASE_RELATIONSHIP_IDENTIFIER*)Iter);

        SwitchLeaseRelationshipLeasingApplicationIdentifiers(LeaseRelationshipIdentifierIncoming);

        //
        // Look up the lease relationship identifier in the subject list.
        //
        IterSubject = RtlLookupElementGenericTable(
            &RemoteLeaseAgentContext->SubjectHashTable,
            &LeaseRelationshipIdentifierIncoming
            );

        if (NULL != IterSubject)
        {
            //
            // Retrieve lease relationship identifier from the subject list.
            // The relationship will be remove from the subject list in function TerminateSubjectLease
            //
            LeaseRelationshipIdentifierSubject = *((PLEASE_RELATIONSHIP_IDENTIFIER*)IterSubject);

            Status = TerminateSubjectLease(RemoteLeaseAgentContext, LeaseRelationshipIdentifierSubject, FALSE);
        }

        SwitchLeaseRelationshipLeasingApplicationIdentifiers(LeaseRelationshipIdentifierIncoming);

        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

    }

    return Status;
}

__success(return == TRUE)
BOOLEAN UpdateLeaseAgentCertHash(
    PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in_bcount(SHA1_LENGTH) PBYTE CertHash,
    __in LPCWSTR CertHashStoreName,
    __out PULONG certStoreNameLength // todo, remove this output parameter and update related KTL routine
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    size_t WcharLength = 0;

    if (memcpy_s(LeaseAgentContext->CertHash, SHA1_LENGTH, CertHash, SHA1_LENGTH))
    {
        return FALSE;
    }

    RtlZeroMemory(LeaseAgentContext->CertHashStoreName, sizeof(LeaseAgentContext->CertHashStoreName));

    Status = RtlStringCbCopyW(
        LeaseAgentContext->CertHashStoreName,
        sizeof(LeaseAgentContext->CertHashStoreName),
        CertHashStoreName
        );
    LAssert(STATUS_SUCCESS == Status);

    Status = RtlStringCchLengthW(
        LeaseAgentContext->CertHashStoreName,
        SCH_CRED_MAX_STORE_NAME_SIZE,
        &WcharLength
        );
    LAssert(STATUS_SUCCESS == Status); // the length was checked before

    *certStoreNameLength = (ULONG)(WcharLength + 1) * sizeof(WCHAR);

    return TRUE;

}

/*
NTSTATUS LeaseAgentAddressToStringW(PTRANSPORT_LISTEN_ENDPOINT SocketAddress, PWCHAR Buffer, ULONG BufferLength, BOOL usePort)
{
    LAssert(SocketAddress);

    if (TRUE == usePort)
    {
        return RtlStringCchPrintfW(Buffer, BufferLength, L"%s:%d", SocketAddress->Address, SocketAddress->Port);
    }
    else
    {
        return RtlStringCchPrintfW(Buffer, BufferLength, L"%s", SocketAddress->Address);
    }

}
*/

NTSTATUS
ProcessForwardMessage(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in PVOID Buffer,
    __in ULONG BufferSize
    )
/*++
Routine Description:

    Function utility: Process the forward request/response messages and relay to destination
    - Call DeserialForwardMessages(...) to deserialize source node address and dest node address
    - Call GetActiveRemoteLeaseAgent(...) to check if we have an active and open RLA
    - Call SerialForwardMessages(...) to serialize the relay request/response message and send to the active RLA
    - Return status

--*/
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PTRANSPORT_LISTEN_ENDPOINT SourceSocketAddress;
    PTRANSPORT_LISTEN_ENDPOINT LeaseSocketAddress;

    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = NULL;
    PLEASE_MESSAGE_HEADER LeaseMessageHeader = NULL;
    KIRQL OldIrql;

    // Allocate memory for message/lease endpoint.
    SourceSocketAddress = LeaseListenEndpointConstructor();
    if (SourceSocketAddress == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    LeaseSocketAddress = LeaseListenEndpointConstructor();
    if (LeaseSocketAddress == NULL)
    {
        LeaseListenEndpointDestructor(SourceSocketAddress);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    LeaseMessageHeader = (PLEASE_MESSAGE_HEADER)Buffer;

    //
    // Get the src/dst addresses.
    //
    Status = DeserializeForwardMessage(
        LeaseMessageHeader,
        (PBYTE)Buffer + LeaseMessageHeader->MessageHeaderSize,
        SourceSocketAddress,
        LeaseSocketAddress
        );

    if (!NT_SUCCESS(Status))
    {
        // listen endpoint is de-allocated in DeserializeForwardMessage
        //
        return Status;
    }

    //
    // Take the lock before process the RLA table
    //
    KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

    //
    // Check if we have a active lease relationship with dest, so we can relay the forward message
    // The returned RLA pointer is from the table; no worry to de-allocate it
    //
    RemoteLeaseAgentContext = GetCurrentActiveRemoteLeaseAgent(
        LeaseAgentContext,
        LeaseSocketAddress);

    if (NULL == RemoteLeaseAgentContext)
    {
        EventWriteProcessForwardMsgNoExistingRLA(
            NULL,
            TransportIdentifier(LeaseAgentContext->Transport),
            LeaseAgentContext->Instance.QuadPart,
            GetMessageType(LeaseMessageHeader->MessageType),
            LeaseMessageHeader->MessageIdentifier.QuadPart
            );

        // We don't want to set the status to unsuccessful because
        // this is not a fatal situation and the caller function would abort the connection
        //
        goto End;
    }

    SerializeAndSendRelayMessages(
        RemoteLeaseAgentContext,
        LeaseMessageHeader,
        SourceSocketAddress,
        Buffer,
        BufferSize
        );

End:

    KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);

    LeaseListenEndpointDestructor(SourceSocketAddress);
    LeaseListenEndpointDestructor(LeaseSocketAddress);

    return Status;
}

NTSTATUS
DeserializeForwardMessage(
    __in PLEASE_MESSAGE_HEADER LeaseMessageHeader,
    __in PVOID Buffer,
    __in PTRANSPORT_LISTEN_ENDPOINT SourceSocketAddress,
    __in PTRANSPORT_LISTEN_ENDPOINT DestSocketAddress
    )
/*++

Routine Description:
    Reads in source and destination node address for the forward messages

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;

    PVOID SourceAddressStartBuffer = NULL;
    PVOID DestAddressStartBuffer = NULL;

    LAssert(LeaseMessageHeader->MessageListenEndPointDescriptor.Size != 0 && 
        LeaseMessageHeader->LeaseListenEndPointDescriptor.Size != 0);

    SourceAddressStartBuffer = 
        (PBYTE)Buffer + 
        LeaseMessageHeader->MessageListenEndPointDescriptor.StartOffset - 
        LeaseMessageHeader->MessageHeaderSize;

    DestAddressStartBuffer = 
        (PBYTE)Buffer + 
        LeaseMessageHeader->LeaseListenEndPointDescriptor.StartOffset - 
        LeaseMessageHeader->MessageHeaderSize;

    Status = DeserializeMessageListenEndPoint(
        SourceSocketAddress,
        SourceAddressStartBuffer,
        LeaseMessageHeader->MessageListenEndPointDescriptor.Size);

    if (!NT_SUCCESS(Status))
    {
        EventWriteDeserializeForwardMessageError(NULL, L" src ", Status);
        goto Error;
    }

    Status = DeserializeMessageListenEndPoint(
        DestSocketAddress,
        DestAddressStartBuffer,
        LeaseMessageHeader->LeaseListenEndPointDescriptor.Size);

    if (!NT_SUCCESS(Status))
    {
        EventWriteDeserializeForwardMessageError(NULL, L" dst ", Status);
        goto Error;
    }

    return Status;

Error:

    LeaseListenEndpointDestructor(SourceSocketAddress);
    LeaseListenEndpointDestructor(DestSocketAddress);

    return Status;

}

PREMOTE_LEASE_AGENT_CONTEXT
GetCurrentActiveRemoteLeaseAgent(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress
    )

/*++
    - First, get the current active RLA by calling FindMostRecentRemoteLeaseAgent
    - Check if the return RLA is in open state and return if so; otherwise, return null
--*/

{

    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContextLookup = NULL;

    NTSTATUS status = RemoteLeaseAgentConstructor(
        LeaseAgentContext,
        RemoteSocketAddress,
        &RemoteLeaseAgentContextLookup
        );

    if (!NT_SUCCESS(status))
    {
        EventWriteLeaseDriverTextTraceError(NULL, L"GetCurrentActiveRemoteLeaseAgent: RemoteLeaseAgentConstructor failed", status);
        return NULL;
    }

    //
    // Check to see if an existent remote lease agent can be reused.
    //
    RemoteLeaseAgentContext = FindMostRecentRemoteLeaseAgent(
        LeaseAgentContext,
        RemoteLeaseAgentContextLookup
        );

    //
    // Deallocate newly created remote lease agent.
    //
    RemoteLeaseAgentDestructor(RemoteLeaseAgentContextLookup);

    if (NULL != RemoteLeaseAgentContext &&
        IsRemoteLeaseAgentOpen(RemoteLeaseAgentContext))
    {
        EventWriteFindRLARelayForwardMsg(
            NULL,
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart,
            GetLeaseAgentState(RemoteLeaseAgentContext->State)
            );

        return RemoteLeaseAgentContext;
    }

    return NULL;
}

NTSTATUS
SerializeAndSendRelayMessages(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __in PLEASE_MESSAGE_HEADER LeaseMessageHeader,
    __in PTRANSPORT_LISTEN_ENDPOINT MsgSourceSocketAddress,
    __in PVOID ForwardMsgBuf,
    __in ULONG ForwardMsgSize
    )

/*++

Routine Description:

    - A -- B -- C
    - Allocate the memory for the relay message (B - C)
    - Inupt buffer is forward message and it has A|C in the address section
    - Compared to the Forward msg (A-B), the relay message has B|A in the address
    - The routine calculate the size first and it is equal to ForwardMsgBufSize - sizeof(C) + sizeof(B)
    - MsgSourceSocketAddress/LeaseSocketAddressSize is full listen end point size including type/port

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONGLONG LocalListenEndpointAddressSize = 0;
    ULONG LocalListenEndpointSize = 0;
    ULONG RelayMsgSize = 0;
    PVOID RelayMsgBuf = NULL;
    PBYTE RelayMsgStartBuffer = NULL;
    PLEASE_MESSAGE_HEADER RelayMsgHeader = NULL;

    ULONG MsgSourceSocketAddressSize = LeaseMessageHeader->MessageListenEndPointDescriptor.Size;
    ULONG LeaseSocketAddressSize = LeaseMessageHeader->LeaseListenEndPointDescriptor.Size;
    BOOLEAN IsRequest = (FORWARD_REQUEST == LeaseMessageHeader->MessageType) ;

    LAssert(FORWARD_REQUEST == LeaseMessageHeader->MessageType || FORWARD_RESPONSE == LeaseMessageHeader->MessageType);

    EventWriteSerializeAndSendRelayMsg(
        NULL,
        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
        RemoteLeaseAgentContext->Instance.QuadPart,
        GetMessageType(LeaseMessageHeader->MessageType),
        LeaseMessageHeader->MessageIdentifier.QuadPart
        );

    GetListenEndPointSize(
        &RemoteLeaseAgentContext->LeaseAgentContext->ListenAddress,
        &LocalListenEndpointAddressSize,
        &LocalListenEndpointSize
        );

    Status = RtlULongSub(
        ForwardMsgSize,
        LeaseSocketAddressSize,
        &RelayMsgSize);

    if (!NT_SUCCESS(Status))
    {
        EventWriteLeaseDriverTextTraceError(
            NULL,
            L"SerializeAndSendRelayMessages failed in RtlULongSub ",
            Status);

        return Status;
    }

    Status = RtlULongAdd(
        RelayMsgSize,
        LocalListenEndpointSize,
        &RelayMsgSize);

    if (!NT_SUCCESS(Status))
    {
        EventWriteLeaseDriverTextTraceError(
            NULL,
            L"SerializeAndSendRelayMessages failed in RtlULongAdd ",
            Status);

        return Status;
    }

    //
    // Allocate message buffer.
    //
    RelayMsgBuf = ExAllocatePoolWithTag(
        NonPagedPool,
        RelayMsgSize,
        LEASE_MESSAGE_TAG
        );

    if (NULL == RelayMsgBuf)
    {
        EventWriteAllocateFail26(
            NULL
            );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(RelayMsgBuf, RelayMsgSize);

    //
    // First copy the forward message without the address section
    //
    if (memcpy_s(
        RelayMsgBuf,
        ForwardMsgSize,
        ForwardMsgBuf,
        ForwardMsgSize - MsgSourceSocketAddressSize - LeaseSocketAddressSize))
    {
        Status = STATUS_DATA_ERROR;
        return Status;
    }

    // Changes the fields in the header: 1st, update the MsgType from FORWARD to RELAY
    //
    RelayMsgHeader = (PLEASE_MESSAGE_HEADER)RelayMsgBuf;

    if (IsRequest)
    {
        RelayMsgHeader->MessageType = RELAY_REQUEST;
    }
    else
    {
        RelayMsgHeader->MessageType = RELAY_RESPONSE;
    }

    RelayMsgHeader->MessageSize = RelayMsgSize;

    // 2nd, update the address offset and size in the header A|C to B|A
    RelayMsgHeader->MessageListenEndPointDescriptor.StartOffset = LeaseMessageHeader->MessageListenEndPointDescriptor.StartOffset;
    RelayMsgHeader->MessageListenEndPointDescriptor.Size = LocalListenEndpointSize;

    RelayMsgHeader->LeaseListenEndPointDescriptor.StartOffset = RelayMsgHeader->MessageListenEndPointDescriptor.StartOffset + LocalListenEndpointSize;
    RelayMsgHeader->LeaseListenEndPointDescriptor.Size = MsgSourceSocketAddressSize;

    // Update the Message Address and Lease Address in the RELAY message body
    //
    RelayMsgStartBuffer = (PBYTE)RelayMsgBuf;

    RelayMsgStartBuffer += RelayMsgHeader->MessageListenEndPointDescriptor.StartOffset;

    Status = SerializeMessageListenEndPoint(
        &RemoteLeaseAgentContext->LeaseAgentContext->ListenAddress,
        RelayMsgStartBuffer,
        RelayMsgHeader->MessageListenEndPointDescriptor.Size
        );

    if (!NT_SUCCESS(Status))
    {
        EventWriteLeaseDriverTextTraceError(
            NULL,
            L"Serialize MessageListenEndPoint failed",
            Status);

        goto Error;
    }

    RelayMsgStartBuffer += RelayMsgHeader->MessageListenEndPointDescriptor.Size;

    SerializeMessageListenEndPoint(
        MsgSourceSocketAddress,
        RelayMsgStartBuffer,
        MsgSourceSocketAddressSize
        );

    if (!NT_SUCCESS(Status))
    {
        EventWriteLeaseDriverTextTraceError(
            NULL,
            L"Serialize LeaseListenEndPoint failed",
            Status);

        goto Error;
    }

    EventWriteLeaseRelationSendingLease(
        NULL,
        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
        RemoteLeaseAgentContext->Instance.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
        RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
        GetMessageType(RelayMsgHeader->MessageType),
        RelayMsgHeader->MessageIdentifier.QuadPart
        ); 

    TransportSendBuffer(
        RemoteLeaseAgentContext->PartnerTarget,
        RelayMsgBuf,
        RelayMsgSize,
        FALSE);


    return STATUS_SUCCESS;

Error:

    //
    // Deallocate lease message. There was an error serializing the message.
    //
    ExFreePool(RelayMsgBuf);
    
    return Status;
}

BOOLEAN
IsReceivedResponse(__in LEASE_MESSAGE_TYPE LeaseMessageType)
{
    return (LEASE_RESPONSE == LeaseMessageType || RELAY_RESPONSE == LeaseMessageType );
}

BOOLEAN
IsReceivedRequest(__in LEASE_MESSAGE_TYPE LeaseMessageType)
{
    return (LEASE_REQUEST == LeaseMessageType || RELAY_REQUEST == LeaseMessageType);
}

BOOLEAN
IsSendingResponse(__in LEASE_MESSAGE_TYPE LeaseMessageType)
{
    return (LEASE_RESPONSE == LeaseMessageType || FORWARD_RESPONSE == LeaseMessageType || PING_RESPONSE == LeaseMessageType);
}

BOOLEAN
IsSendingRequest(__in LEASE_MESSAGE_TYPE LeaseMessageType)
{
    return (LEASE_REQUEST == LeaseMessageType || FORWARD_REQUEST == LeaseMessageType || PING_REQUEST == LeaseMessageType);
}

BOOLEAN
IsIndirectLeaseMsg(__in LEASE_MESSAGE_TYPE LeaseMessageType)
{
    return (FORWARD_REQUEST == LeaseMessageType ||
            FORWARD_RESPONSE == LeaseMessageType ||
            RELAY_REQUEST == LeaseMessageType ||
            RELAY_RESPONSE == LeaseMessageType);
}
