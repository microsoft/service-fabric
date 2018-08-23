// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

static Global<DRIVER_CONTEXT> driverContext = make_global<DRIVER_CONTEXT>();
static const StringLiteral TraceType = "LeasLayr";

bool LEASE_RELATIONSHIP_IDENTIFIER_COMPARATOR::operator()(_LEASE_RELATIONSHIP_IDENTIFIER* left, _LEASE_RELATIONSHIP_IDENTIFIER* right){
    auto res1 = wcscmp(left->RemoteLeasingApplicationIdentifier, right->RemoteLeasingApplicationIdentifier);
    auto res2 = wcscmp(left->LeasingApplicationContext->LeasingApplicationIdentifier, right->LeasingApplicationContext->LeasingApplicationIdentifier);
    if(res1 <0) {
        return TRUE;
    }
    else if(res1>0){
        return FALSE;
    }
    else{
        return res2<0;
    }
}

bool REMOTE_LEASE_AGENT_CONTEXT_COMPARATOR::operator()(_REMOTE_LEASE_AGENT_CONTEXT* left, _REMOTE_LEASE_AGENT_CONTEXT* right){
    return IS_LARGE_INTEGER_LESS_THAN_LARGE_INTEGER(left->Instance, right->Instance);
}

bool LEASE_AGENT_CONTEXT_COMPARATOR::operator()(_LEASE_AGENT_CONTEXT* left, _LEASE_AGENT_CONTEXT* right){
    return IS_LARGE_INTEGER_LESS_THAN_LARGE_INTEGER(left->Instance, right->Instance);
}

bool LEASING_APPLICATION_CONTEXT_COMPARATOR::operator()(_LEASING_APPLICATION_CONTEXT* left, _LEASING_APPLICATION_CONTEXT* right){
    return wcscmp(left->LeasingApplicationIdentifier, right->LeasingApplicationIdentifier) < 0;
}

bool TRANSPORT_BLOCKING_TEST_DESCRIPTOR_COMPARATOR::operator()(_TRANSPORT_BLOCKING_TEST_DESCRIPTOR* left, _TRANSPORT_BLOCKING_TEST_DESCRIPTOR* right){
    wstringstream ss1, ss2, ss3, ss4;
    ss1 << std::wstring(left->LocalSocketAddress.Address);
    ss2 << std::wstring(right->LocalSocketAddress.Address);
    ss3 << std::wstring(left->RemoteSocketAddress.Address);
    ss4 << std::wstring(right->RemoteSocketAddress.Address);
    return ss1.str() < ss2.str() ||
        left->LocalSocketAddress.Port < right->LocalSocketAddress.Port ||
        left->FromAny < right->FromAny ||
        ss3.str() < ss4.str() ||
        left->RemoteSocketAddress.Port < right->RemoteSocketAddress.Port ||
        left->ToAny < right->ToAny ||
        left->BlockingType < right->BlockingType;
}

PDRIVER_CONTEXT LeaseLayerDriverGetContext()
{
    return &(*driverContext);
}

VOID
DelayedRequestCancelation(
    __in WDFREQUEST & Request
    )

/*++

Routine Description:

    Routine to cancel a delayed I/O.

Arguments:

    Request - request that is pending.

Return Value:

    n/a

--*/

{
    BOOLEAN CompleteRequest = FALSE;
    KIRQL OldIrql;

    //
    // Retrieve the request's context area.
    //
    PREQUEST_CONTEXT ReqContext = &Request->requestContext;

    //
    // Acquire the request spinlock.
    //
    KeAcquireSpinLock(&ReqContext->Lock, &OldIrql);

    //
    // Check to see if the request has benn completed already.
    //
    if (!ReqContext->IsCompleted) {
        
        //
        // Mark the fact that the request is cancelled.
        //
        ReqContext->IsCanceled = TRUE;

        //
        // This request will be cancelled now.
        //
        CompleteRequest = TRUE;
    }
    
    //
    // Release request spinlock.
    //
    KeReleaseSpinLock(&ReqContext->Lock, OldIrql);
//
//    //
//    // Complete request as canceled if possible.
//    //
//    if (CompleteRequest) {
//
//        //
//        // Complete request with cancellation code.
//        //
//        WdfRequestComplete(Request, STATUS_CANCELLED);
//    }
}

VOID
TerminateSystemThread(
    __in PDRIVER_CONTEXT DriverContext
    )

/*++

Routine Description:

    Terminates the system maintenance thread created by DriverEntry.

Parameters Description:

    DriverContext - driver context area.

Return Value:

    n/a

--*/

{
    //
    // Make the system thread exit.
    //
    KeSetEvent(
        &DriverContext->MaintenanceWorkerThreadContext.Shutdown,
        0,
        FALSE
        );
    KeWaitForSingleObject(
        DriverContext->MaintenanceWorkerThreadContext.WorkerThreadHandle,
        Executive,
        KernelMode,
        FALSE,
        NULL 
        );
    //
    // Release the ref count on the thread handle.
    //
    ObDereferenceObject(DriverContext->MaintenanceWorkerThreadContext.WorkerThreadHandle);
}

//__drv_dispatchType(IRP_MJ_CLEANUP)
DRIVER_DISPATCH LeaseLayerEvtCleanup;

VOID
DeallocateFailedRemoteLeaseAgents(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in BOOLEAN WaitForDeallocation,
    __in_opt LARGE_INTEGER CurrentTime
    )

/*++
Routine Description:

    Unloads remote lease agents from a given lease agent.

Arguments:

    LeaseAgentContext - lease agent to work on.

    WaitForDeallocation - specifies if a remote lease agent 
        that is not yet ready to deallocate can be skipped.
    
Return Value:

    n/a

--*/

{
    BOOLEAN RestartRemoteLeaseAgent = TRUE;
    //
    // Iterate over remote lease agents.
    //
    for(auto iter = LeaseAgentContext->RemoteLeaseAgentContextHashTable.begin(); iter != LeaseAgentContext->RemoteLeaseAgentContextHashTable.end();){
        PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = *iter;
        //
        // TODO check why it is nullptr.
        //
        if(!RemoteLeaseAgentContext) {
            iter++;
            continue;
        }
        //
        // Check for orphan remote lease agents.
        //
        if (IsRemoteLeaseAgentOrphaned(RemoteLeaseAgentContext)) {

            //
            // We can now fail this remote lease agent. It was previously orphaned.
            // When the channel layer is built, this problem should go away.
            //
            SetRemoteLeaseAgentFailed(RemoteLeaseAgentContext);
        }
        
        //
        // Check to see if we can get rid of this remote lease agent.
        // The remote lease agent has to be in a failed state and queiesced.
        //
        if (WaitForDeallocation)
        {
            // Only in unload rouinte; 
            // The disconnect call would de-reference the SendTarget and cause it to destruct;
            // The SendTarget would destruct MessageQueue and and remaining messages;
            // The message destruction would de-reference RLA refCount and unblock the KeWaitForSingleObject call below

            RemoteLeaseAgentDisconnect(RemoteLeaseAgentContext, TRUE);
        }

        RestartRemoteLeaseAgent = 
            IsRemoteLeaseAgentFailed(RemoteLeaseAgentContext) &&
            IsRemoteLeaseAgentReadyForDeallocation(RemoteLeaseAgentContext, WaitForDeallocation);

        //
        // We want to delay the destruction of RLA under the following conditions
        // 1. Not in the driver unloading routine, i.e, WaitForDeallocation is FALSE
        // 2. The timeToBeFailed filed of RLA has not been initialized
        // 3. The ToBeFailed filed is initialized, but it was done in the same thread (by checking currentTime)
        //
        if (IsRemoteLeaseAgentFailed(RemoteLeaseAgentContext) && FALSE == WaitForDeallocation)
        {
            if (0 == RemoteLeaseAgentContext->TimeToBeFailed.QuadPart)
            {
                RemoteLeaseAgentContext->TimeToBeFailed.QuadPart = CurrentTime.QuadPart;
            }

            if (CurrentTime.QuadPart == RemoteLeaseAgentContext->TimeToBeFailed.QuadPart || RemoteLeaseAgentContext->IsInArbitrationNeutral == TRUE)
            {
                EventWriteRemoteLeaseAgentSetNotDestruct(
                    NULL,
                    RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                    RemoteLeaseAgentContext->Instance.QuadPart,
                    RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                    RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                    GetLeaseAgentState(RemoteLeaseAgentContext->State),
                    GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
                    GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
                    RemoteLeaseAgentContext->IsActive,
                    RemoteLeaseAgentContext->TimeToBeFailed.QuadPart,
                    RemoteLeaseAgentContext->IsInArbitrationNeutral
                    );
                RestartRemoteLeaseAgent = FALSE;
            }
            else
            {
                //
                // TimeToBeFailed has been set and not equal to current time
                // It must be set in the previous maintenance thread
                // The RLADisconnect need to be called to abort connection and release target to release RLA ref count

                RemoteLeaseAgentDisconnect(RemoteLeaseAgentContext, TRUE);
            }

        }

        if (RestartRemoteLeaseAgent) {

             EventWriteCleanUpRemoteLeaseAgent(
                NULL,
                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                RemoteLeaseAgentContext->Instance.QuadPart
                ); 

            //
            // Remove remote lease agent from the lease agent hash table.
            //
            LeaseAgentContext->RemoteLeaseAgentContextHashTable.erase(LeaseAgentContext->RemoteLeaseAgentContextHashTable.find(RemoteLeaseAgentContext));

            //
            // Deallocate remote lease agent.
            //
            RemoteLeaseAgentDestructor(RemoteLeaseAgentContext);
            iter =  LeaseAgentContext->RemoteLeaseAgentContextHashTable.begin();
        }
        else{
            iter ++;
        }
    }
}

VOID
DestructArbitrationNeutralRemoteLeaseAgents(__in PLEASE_AGENT_CONTEXT LeaseAgentContext)
{
    PREMOTE_LEASE_AGENT_CONTEXT FailedRemoteLeaseAgentContext = NULL;

    for(auto IterFailedRemoteLeaseAgent = LeaseAgentContext->RemoteLeaseAgentContextHashTable.begin();
        IterFailedRemoteLeaseAgent != LeaseAgentContext->RemoteLeaseAgentContextHashTable.end();
        IterFailedRemoteLeaseAgent++)
    {
        FailedRemoteLeaseAgentContext = *IterFailedRemoteLeaseAgent;

        if (IsRemoteLeaseAgentFailed(FailedRemoteLeaseAgentContext) && FailedRemoteLeaseAgentContext->IsInArbitrationNeutral == TRUE)
        {
            //
            // Found a candidate of remote lease agent to search. We try to search for the remote lease agent that has the same remote identity.
            //
            EventWriteRemoteLeaseAgentSetDestructSearch(
                NULL,
                FailedRemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                FailedRemoteLeaseAgentContext->Instance.QuadPart,
                FailedRemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                FailedRemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                GetLeaseAgentState(FailedRemoteLeaseAgentContext->State),
                GetLeaseState(FailedRemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
                GetLeaseState(FailedRemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
                FailedRemoteLeaseAgentContext->IsActive,
                FailedRemoteLeaseAgentContext->TimeToBeFailed.QuadPart,
                FailedRemoteLeaseAgentContext->IsInArbitrationNeutral
                );

            PREMOTE_LEASE_AGENT_CONTEXT OpenRemoteLeaseAgentContext = NULL;
            BOOLEAN FoundOpenRemoteLeaseAgent = FALSE;

            for (auto IterOpenRemoteLeaseAgent = LeaseAgentContext->RemoteLeaseAgentContextHashTable.begin();
                IterOpenRemoteLeaseAgent != LeaseAgentContext->RemoteLeaseAgentContextHashTable.end();
                IterOpenRemoteLeaseAgent++)
            {
                OpenRemoteLeaseAgentContext = *IterOpenRemoteLeaseAgent;

                if (0 == wcscmp(FailedRemoteLeaseAgentContext->RemoteLeaseAgentIdentifier, OpenRemoteLeaseAgentContext->RemoteLeaseAgentIdentifier))
                {
                    if (IsRemoteLeaseAgentOpenTwoWay(OpenRemoteLeaseAgentContext))
                    {
                        //
                        // Found an open remote lease agent, we could destruct the failed one.
                        // Set the arbitration neutral flag back.
                        //
                        EventWriteRemoteLeaseAgentSetDestructFound(
                            NULL,
                            OpenRemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                            OpenRemoteLeaseAgentContext->Instance.QuadPart,
                            OpenRemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                            OpenRemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                            GetLeaseAgentState(OpenRemoteLeaseAgentContext->State),
                            GetLeaseState(OpenRemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
                            GetLeaseState(OpenRemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
                            OpenRemoteLeaseAgentContext->IsActive,
                            OpenRemoteLeaseAgentContext->TimeToBeFailed.QuadPart,
                            OpenRemoteLeaseAgentContext->IsInArbitrationNeutral
                            );
                        FailedRemoteLeaseAgentContext->IsInArbitrationNeutral = FALSE;

                        FoundOpenRemoteLeaseAgent = TRUE;
                        break;
                    }
                }
            }

            if (FoundOpenRemoteLeaseAgent == FALSE)
            {
                EventWriteRemoteLeaseAgentSetDestructNotFound(
                    NULL,
                    FailedRemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                    FailedRemoteLeaseAgentContext->Instance.QuadPart,
                    FailedRemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                    FailedRemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                    GetLeaseAgentState(FailedRemoteLeaseAgentContext->State),
                    GetLeaseState(FailedRemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
                    GetLeaseState(FailedRemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
                    FailedRemoteLeaseAgentContext->IsActive,
                    FailedRemoteLeaseAgentContext->TimeToBeFailed.QuadPart,
                    FailedRemoteLeaseAgentContext->IsInArbitrationNeutral
                    );
            }
        }
    }
}

LONG
GetConfigLeaseDuration(__in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext)
{
    if (LEASE_DURATION_ACROSS_FAULT_DOMAIN == RemoteLeaseAgentContext->LeaseRelationshipContext->EstablishDurationType)
    {
        return RemoteLeaseAgentContext->LeaseAgentContext->LeaseConfigDurations.LeaseDurationAcrossFD;
    }

    return RemoteLeaseAgentContext->LeaseAgentContext->LeaseConfigDurations.LeaseDuration;
}

//
// Update the indirectLeaseLimit based on the lease duration
// Caller should hold lock
//
VOID
UpdateIndirectLeaseLimit(__in PLEASE_AGENT_CONTEXT LeaseAgentContext)
{
    if (LeaseAgentContext->LeaseConfigDurations.LeaseDuration != 0)
    {
        LeaseAgentContext->ConsecutiveIndirectLeaseLimit =
            (LONG)(LeaseAgentContext->LeaseConfigDurations.MaxIndirectLeaseTimeout / LeaseAgentContext->LeaseConfigDurations.LeaseDuration);
    }
    else
    {
        LeaseAgentContext->ConsecutiveIndirectLeaseLimit = 0;
    }
}

VOID
GetDurationsForRequest(
    __in PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext,
    __inout PLONG PRequestLeaseDuration,
    __inout PLONG PRequestLeaseSuspendDuration,
    __inout PLONG PRequestArbitrationDuration,
    __in BOOLEAN UpdateLeaseRelationship
    )
{
    LONG ConfigLeaseDuration;
    LONG ConfigLeaseSuspendDuration;
    LONG ConfigArbitrationDuration;

    LONG RemoteLeaseDuration;
    LONG RemoteLeaseSuspendDuration;
    LONG RemoteArbitrationDuration;

    LAssert(PRequestLeaseDuration != NULL && PRequestLeaseSuspendDuration != NULL && PRequestArbitrationDuration != NULL);

    ConfigLeaseDuration = GetConfigLeaseDuration(RemoteLeaseAgentContext);
    ConfigLeaseSuspendDuration = RemoteLeaseAgentContext->LeaseAgentContext->LeaseSuspendDuration;
    ConfigArbitrationDuration = RemoteLeaseAgentContext->LeaseAgentContext->ArbitrationDuration;

    if (RemoteLeaseAgentContext->LeaseRelationshipContext->IsDurationUpdated ||
        LEASE_STATE_INACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState)
    {
        // Always use the latest config duration if the lease duration is updated OR it is a establish request
        // The lease partner may grant a greater duration when send back the RESPONSE
        //
        *PRequestLeaseDuration = ConfigLeaseDuration;
        *PRequestLeaseSuspendDuration = ConfigLeaseSuspendDuration;
        *PRequestArbitrationDuration = ConfigArbitrationDuration;

        // Reset to false; Otherwise, the granted lease duration would be different from the requested for each lease exchange
        RemoteLeaseAgentContext->LeaseRelationshipContext->IsDurationUpdated = FALSE;
    }
    else
    {
        // Otherwise, chosse the greater one between local config and remote partner config duration
        // Don't use the lease relationship duration because the partner may just reduce its requested duration, 
        // which may not be reflect on lease relationship duration yet
        //

        RemoteLeaseDuration = RemoteLeaseAgentContext->LeaseRelationshipContext->RemoteLeaseDuration;
        RemoteLeaseSuspendDuration = RemoteLeaseAgentContext->LeaseRelationshipContext->RemoteSuspendDuration;
        RemoteArbitrationDuration = RemoteLeaseAgentContext->LeaseRelationshipContext->RemoteArbitrationDuration;

        *PRequestLeaseDuration = (RemoteLeaseDuration != DURATION_MAX_VALUE && RemoteLeaseDuration > ConfigLeaseDuration)
            ? RemoteLeaseDuration : ConfigLeaseDuration;

        *PRequestLeaseSuspendDuration = (RemoteLeaseSuspendDuration != DURATION_MAX_VALUE && RemoteLeaseSuspendDuration > ConfigLeaseSuspendDuration)
            ? RemoteLeaseSuspendDuration : ConfigLeaseSuspendDuration;

        *PRequestArbitrationDuration = (RemoteArbitrationDuration != DURATION_MAX_VALUE && RemoteArbitrationDuration > ConfigArbitrationDuration)
            ? RemoteArbitrationDuration : ConfigArbitrationDuration;
    }

    // Trace out if the requested duration is different from the lease relationship
    //
    if (RemoteLeaseAgentContext->LeaseRelationshipContext->Duration != *PRequestLeaseDuration ||
        RemoteLeaseAgentContext->LeaseRelationshipContext->LeaseSuspendDuration != *PRequestLeaseSuspendDuration ||
        RemoteLeaseAgentContext->LeaseRelationshipContext->ArbitrationDuration != *PRequestArbitrationDuration
        )
    {
        EventWriteSubjectRequestedLeaseDuration(
            NULL,
            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContext->Instance.QuadPart,
            *PRequestLeaseDuration,
            ConfigLeaseDuration,
            RemoteLeaseAgentContext->LeaseRelationshipContext->RemoteLeaseDuration,
            RemoteLeaseAgentContext->LeaseRelationshipContext->Duration,
            UpdateLeaseRelationship
            );
    }

    if (UpdateLeaseRelationship)
    {
        RemoteLeaseAgentContext->LeaseRelationshipContext->Duration = *PRequestLeaseDuration;
        RemoteLeaseAgentContext->LeaseRelationshipContext->LeaseSuspendDuration = *PRequestLeaseSuspendDuration;
        RemoteLeaseAgentContext->LeaseRelationshipContext->ArbitrationDuration = *PRequestArbitrationDuration;
    }

}

NTSTATUS
CheckAndTraceDurationList(PLEASE_CONFIG_DURATIONS LeaseConfigDurations)
{
    EventWriteCheckLeaseDurationList(NULL, 0, LeaseConfigDurations->LeaseDuration);
    EventWriteCheckLeaseDurationList(NULL, 1, LeaseConfigDurations->LeaseDurationAcrossFD);

    EventWriteLeaseDurationConfigs(NULL, L"Unresponsive duration: ", LeaseConfigDurations->UnresponsiveDuration);
    EventWriteLeaseDurationConfigs(NULL, L"MaxIndirectLease duration: ", LeaseConfigDurations->MaxIndirectLeaseTimeout);

    if (!IsValidDuration(LeaseConfigDurations->LeaseDuration) ||
        DURATION_MAX_VALUE == LeaseConfigDurations->LeaseDuration ||
        DURATION_MAX_VALUE == LeaseConfigDurations->LeaseDurationAcrossFD)
    {
        EventWriteInvalidReq10(NULL);
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}
VOID MaintenanceWorkerThread();
static INIT_ONCE initOnceMaintenanceEventSource;
static BOOL CALLBACK InitOnceMaintenanceEventSourceFunction(PINIT_ONCE, PVOID, PVOID *)
{
    Common::Threadpool::Post([] { MaintenanceWorkerThread(); });

    return TRUE;
}

NTSTATUS
CreateLeaseAgent(
    WDFREQUEST & Request,
    __in size_t OutputBufferLength,
    __in size_t InputBufferLength
    )

/*++

Routine Description:

    Creates a new lease agent. 

Arguments:
        
    Request - Handle to a framework request object.

    OutputBufferLength - The size of the output buffer, in bytes.
    
    InputBufferLength - The size of the input buffer, in bytes.

Return Value:

    n/a

--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PVOID InputBuffer = NULL;
    PVOID OutputBuffer = NULL;
    PVOID IterLeaseAgent = NULL;
    ULONG BytesTransferred = 0;
    size_t Length = 0;
    BOOLEAN Inserted = FALSE;
    BOOLEAN Found = FALSE;
    BOOLEAN SecuritySettingMatch = TRUE;
    KIRQL OldIrql;
    
    PLEASE_AGENT_CONTEXT LeaseAgentContext = NULL;
    PLEASE_AGENT_CONTEXT leaseAgentToDestruct = NULL;

    PCREATE_LEASE_AGENT_INPUT_BUFFER_DEVICE_IOCTL CreateLeaseAgentInputBuffer = NULL;
    PLEASE_AGENT_BUFFER_DEVICE_IOCTL CreateLeaseAgentOutputBuffer = NULL;

    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext();

    PVOID lpContext = NULL;
    //
    // Retrieve the input buffer from the request.
    //
    if (sizeof(CREATE_LEASE_AGENT_INPUT_BUFFER_DEVICE_IOCTL) == InputBufferLength) {
        
        Status = WdfRequestRetrieveInputBuffer(
            Request, 
            sizeof(CREATE_LEASE_AGENT_INPUT_BUFFER_DEVICE_IOCTL),
            &InputBuffer,
            &Length
            );
        
        if (!NT_SUCCESS(Status)) {
        
            EventWriteRetrieveFail1(
                NULL
                );
    
            goto End;
        }
        
        LAssert(NULL != InputBuffer);
        LAssert(Length == InputBufferLength);
        
    } else {
    
        EventWriteInvalidReq1(
            NULL
            );

        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;

        goto End;
    }

    //
    // Retrieve the output buffer from the request.
    //
    if (sizeof(LEASE_AGENT_BUFFER_DEVICE_IOCTL) == OutputBufferLength)
    {
        Status = WdfRequestRetrieveOutputBuffer(
            Request, 
            sizeof(LEASE_AGENT_BUFFER_DEVICE_IOCTL),
            &OutputBuffer,
            &Length
            );
        
        if (!NT_SUCCESS(Status))
        {
            EventWriteRetrieveFail2(NULL);
            goto End;
        }

        LAssert(NULL != OutputBuffer);
        LAssert(Length == OutputBufferLength);
        
    }
    else
    {
        EventWriteInvalidReq5(NULL);
        Status = STATUS_INVALID_PARAMETER;

        goto End;
    }

    //
    // Cast input and output buffers to their types.
    // The input and output buffers are overlapping.
    // Take caution when setting data in the output 
    // buffer, it should be done only at the end after
    // the data from the input buffer has been processed.
    //
    CreateLeaseAgentInputBuffer = (PCREATE_LEASE_AGENT_INPUT_BUFFER_DEVICE_IOCTL)InputBuffer;
    CreateLeaseAgentOutputBuffer = (PLEASE_AGENT_BUFFER_DEVICE_IOCTL)OutputBuffer;
    
    if (CreateLeaseAgentInputBuffer->UserModeVersion != LEASE_DRIVER_VERSION)
    {
        CreateLeaseAgentOutputBuffer->KernelModeVersion = LEASE_DRIVER_VERSION;

        //
        // Set the number of bytes transferred.
        //
        BytesTransferred = sizeof(LEASE_AGENT_BUFFER_DEVICE_IOCTL);

        Status = STATUS_REVISION_MISMATCH;

        EventWriteKernelUserVersionMismatch(
            NULL,
            CreateLeaseAgentInputBuffer->UserModeVersion,
            CreateLeaseAgentOutputBuffer->KernelModeVersion
            );

        goto End;
    }

    //
    // Check input buffer - socket address.
    //
    if (!IsValidListenEndpoint(&CreateLeaseAgentInputBuffer->SocketAddress)) {
    
        EventWriteInvalidReq6(
            NULL
            );    

        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        
        goto End;
    }

    // Check and trace out the lease duration list
    Status = CheckAndTraceDurationList(&CreateLeaseAgentInputBuffer->LeaseConfigDurations);

    if (!NT_SUCCESS(Status))
    {
        EventWriteLeaseDriverTextTraceError(
            NULL,
            L"Invalid user mode lease durations inputs",
            Status);

        goto End;
    }

    if (!IsValidDuration(CreateLeaseAgentInputBuffer->LeaseSuspendDurationMilliseconds) ||
        !IsValidDuration(CreateLeaseAgentInputBuffer->ArbitrationDurationMilliseconds) ||
        DURATION_MAX_VALUE == CreateLeaseAgentInputBuffer->LeaseSuspendDurationMilliseconds ||
        DURATION_MAX_VALUE == CreateLeaseAgentInputBuffer->ArbitrationDurationMilliseconds)
    {
        Status = STATUS_INVALID_PARAMETER;
        EventWriteLeaseDriverTextTraceError(
            NULL,
            L"Invalid user mode arbitration durations inputs",
            Status);

        goto End;
    }

    if (CreateLeaseAgentInputBuffer->NumberOfLeaseRetry <= 0 || CreateLeaseAgentInputBuffer->StartOfLeaseRetry <= 0 )
    {
        EventWriteInvalidReq12(NULL);
        Status = STATUS_INVALID_PARAMETER;
        goto End;
    }

    //
    // Construct lease agent structure.
    //
    Status = LeaseAgentConstructor(
        &CreateLeaseAgentInputBuffer->SocketAddress,
        &CreateLeaseAgentInputBuffer->LeaseConfigDurations,
        CreateLeaseAgentInputBuffer->LeaseSuspendDurationMilliseconds,
        CreateLeaseAgentInputBuffer->ArbitrationDurationMilliseconds,
        CreateLeaseAgentInputBuffer->NumberOfLeaseRetry,
        CreateLeaseAgentInputBuffer->StartOfLeaseRetry,
        CreateLeaseAgentInputBuffer->SecuritySettings,
        &LeaseAgentContext
        );

    if (!NT_SUCCESS(Status)) {

        //
        // Request cannot be completed.
        //
        goto End;
    }

    ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    //
    // Check to see if this lease agent contains already the remote lease agent.
    // If a remote lease agent of the same remote address exists and is not 
    // terminating, then reuse it.
    //
    for(PLEASE_AGENT_CONTEXT LeaseAgentContextIter: DriverContext->LeaseAgentContextHashTable)
    {
        if(Found) break;

        KeAcquireSpinLock(&LeaseAgentContextIter->Lock, &OldIrql);
    
        //
        // Compare the lease agent address and check lease agent failed state.
        //
        if (!IsLeaseAgentFailed(LeaseAgentContextIter) &&
            0 == wcscmp(
                TransportIdentifier(LeaseAgentContext->Transport), 
                TransportIdentifier(LeaseAgentContextIter->Transport)
                )
            ) {

            //
            // Lease agent already registered.
            //
            Found = TRUE;

            if (LeaseAgentContextIter->Transport->Security()->SecurityProvider!= LeaseAgentContext->Transport->Security()->SecurityProvider)
            {
                SecuritySettingMatch = FALSE;
            }

            if (FALSE == SecuritySettingMatch)
            {
                EventWriteLeaseSecuritySettingMismatch(
                    NULL,
                    TransportIdentifier(LeaseAgentContextIter->Transport),
                    LeaseAgentContextIter->Instance.QuadPart,
                    LeaseAgentContextIter->Transport->Security()->SecurityProvider,
                    TransportIdentifier(LeaseAgentContext->Transport),
                    LeaseAgentContext->Instance.QuadPart,
                    LeaseAgentContext->Transport->Security()->SecurityProvider
                    );

                leaseAgentToDestruct = LeaseAgentContext;
                //
                // Found lease agent, but cert hash does not match.
                //
                Status = STATUS_RETRY;
            }
            else
            {
                if (CanLeaseAgentBeFailed(LeaseAgentContextIter))
                {
                    // reset this in case the maintenance thread has seen it
                    LeaseAgentContextIter->TimeToBeFailed.QuadPart = 0;

                    EventWriteLeaseAgentActionStatus2(
                        NULL,
                        TransportIdentifier(LeaseAgentContextIter->Transport),
                        LeaseAgentContextIter->Instance.QuadPart
                        ); 
                }
                else
                {
                    EventWriteLeaseAgentActionStatus3(
                        NULL,
                        TransportIdentifier(LeaseAgentContextIter->Transport),
                        LeaseAgentContextIter->Instance.QuadPart
                        ); 
                }

                //
                // Update the Lease Duration before destory the temporary lease agent
                //
                if (memcpy_s(&LeaseAgentContextIter->LeaseConfigDurations, sizeof(LEASE_CONFIG_DURATIONS), &LeaseAgentContext->LeaseConfigDurations, sizeof(LEASE_CONFIG_DURATIONS)))
                {
                    Status = STATUS_DATA_ERROR;
                    goto End;
                }
           
                LeaseAgentContextIter->LeaseSuspendDuration = LeaseAgentContext->LeaseSuspendDuration;
                LeaseAgentContextIter->ArbitrationDuration = LeaseAgentContext->ArbitrationDuration;
                LeaseAgentContextIter->LeaseRetryCount = LeaseAgentContext->LeaseRetryCount;
                LeaseAgentContextIter->LeaseRenewBeginRatio = LeaseAgentContext->LeaseRenewBeginRatio;
                LeaseAgentContextIter->SessionExpirationTimeout = LeaseAgentContext->SessionExpirationTimeout;

                LeaseAgentContextIter->ConsecutiveIndirectLeaseLimit = LeaseAgentContext->ConsecutiveIndirectLeaseLimit;

                //
                // Destroy newly created lease agent.
                //
                leaseAgentToDestruct = LeaseAgentContext;

                //
                // Update the reference.
                //
                LeaseAgentContext = LeaseAgentContextIter;

                //
                // Found lease agent.
                //
                Status = STATUS_SUCCESS;
            }
        }

        KeReleaseSpinLock(&LeaseAgentContextIter->Lock, OldIrql);
    }

    if (leaseAgentToDestruct != NULL)
    {
        LeaseAgentDestructor(leaseAgentToDestruct);
    }

    if (!Found)
    {
        //
        // There is no available lease agent in the table
        // Attempt to insert the new lease agent context in the 
        // lease agent context hash table. 
        //
        DriverContext->LeaseAgentContextHashTable.insert(LeaseAgentContext);
        EventWriteLeaseAgentActionStatus4(
                NULL,
                TransportIdentifier(LeaseAgentContext->Transport),
                LeaseAgentContext->Instance.QuadPart
        );

        //
        // Done. Successfully created a new lease agent.
        //
        Status = STATUS_SUCCESS;
    }

    ExReleaseFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    if (NT_SUCCESS(Status))
    {

        if (!Found)
        {
            //
            // Not found in the table, this newly create LeaseAgentContext and it is already inserted into the table
            // 
            // Create the listening socket for this lease agent.
            //
            Status = LeaseAgentCreateListeningTransport(LeaseAgentContext);

            EventWriteTransportListenStatus(
                NULL,
                Status,
                TransportIdentifier(LeaseAgentContext->Transport),
                LeaseAgentContext->Instance.QuadPart);
    
            if (!NT_SUCCESS(Status))
            {

                EventWriteLeaseAgentBindingFailedWith(
                    NULL,
                    TransportIdentifier(LeaseAgentContext->Transport),
                    LeaseAgentContext->Instance.QuadPart,
                    Status
                    );

                //
                // Binding failed, set the lease agent to failed
                // Maintenance will take care of the cleanup
                //
                SetLeaseAgentFailed(LeaseAgentContext);

                //
                // The caller should retry this call. The socket binding failed
                // but there is no lease agent in the hash table yet. This is a
                // known race condition in this routine.
                //

                if (STATUS_SHARING_VIOLATION == Status)
                {
                    Status = STATUS_RETRY;
                }

                goto End;

            }
        }

        //
        // Populate the output buffer with new lease agent.
        //
        CreateLeaseAgentOutputBuffer->LeaseAgentHandle = LeaseAgentContext;
        CreateLeaseAgentOutputBuffer->LeaseAgentInstance = LeaseAgentContext->Instance.QuadPart;

        //
        // Set the number of bytes transferred.
        //
        BytesTransferred = sizeof(LEASE_AGENT_BUFFER_DEVICE_IOCTL);
    }

End:
    Request->outputLength = BytesTransferred;

    ::InitOnceExecuteOnce(
            &initOnceMaintenanceEventSource,
            InitOnceMaintenanceEventSourceFunction,
            NULL,
            &lpContext);
    return Status;
}

NTSTATUS
UnregisterApplication(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in BOOLEAN isSubjectFailed
    )

/*++

Routine Description:

    Unregisters a leasing application from a lease agent. Remote 
    lease agents in state failed or suspended are skipped.

Arguments:
        
    LeaseAgentContext - lease agent containing the leasing application.

    LeasingApplicationContext - leasing application to unregister.

Return Value:

    STATUS_SUCCESS if all lease relationships of the leasing application 
    are successfuly terminated.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    EventWriteTerminateAllLeaseRelation(
        NULL,
        LeasingApplicationContext->LeasingApplicationIdentifier,
        TransportIdentifier(LeaseAgentContext->Transport),
        LeaseAgentContext->Instance.QuadPart
        );

    //
    // Terminate all leases related for this application.
    //
    for(PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext : LeaseAgentContext->RemoteLeaseAgentContextHashTable){
        //
        // Retrieve remote lease agent.
        //
        if ( IsRemoteLeaseAgentSuspended(RemoteLeaseAgentContext) &&
            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState == LEASE_STATE_EXPIRED &&
            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState == LEASE_STATE_EXPIRED &&
            RemoteLeaseAgentContext->LeasingApplicationForArbitration == LeasingApplicationContext &&
            isSubjectFailed == TRUE)
        {
            //
            // The leasing application used for application is unregistering
            // This case is treated as if the remote lease agent has gone into arbitration but has lost it. 
            // Fail the lease agent and all its remote lease agents.
            //
            OnLeaseFailure(RemoteLeaseAgentContext->LeaseAgentContext);
            return Status;
        }

        if (IsRemoteLeaseAgentFailed(RemoteLeaseAgentContext) || 
            IsRemoteLeaseAgentSuspended(RemoteLeaseAgentContext)) {
        
            EventWriteTerminateLeaseRelationSkipped(
                NULL,
                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                RemoteLeaseAgentContext->Instance.QuadPart,
                GetLeaseAgentState(RemoteLeaseAgentContext->State)
                ); 
        
            //
            // This remote lease agent is failed or suspended already.
            // Since this application is going away, the fact that the remote 
            // lease agent is not available is irrelevant. The arbitration result,
            // or the fact that the last lease terminate has been sent but not
            // acked yet, won't matter for this leasing application.
            //
            //
            continue;
        }
    
        //
        // Terminate all lease relationships for this application on this
        // remote lease agent.
        //
        Status = TerminateAllLeaseRelationships(
            RemoteLeaseAgentContext,
            LeasingApplicationContext,
            isSubjectFailed
            );
    
        if (!NT_SUCCESS(Status)) {
    
            //
            // Cannot unregister this application.
            //
            break;
        }
    }

    if (NT_SUCCESS(Status)) {
        
        EventWriteLeaseAppUnregister(
            NULL,
            LeasingApplicationContext->LeasingApplicationIdentifier,
            TransportIdentifier(LeaseAgentContext->Transport),
            LeaseAgentContext->Instance.QuadPart
            ); 

    } else {
    
        EventWriteLeaseAppUnregisterFailed(
            NULL,
            LeasingApplicationContext->LeasingApplicationIdentifier,
            TransportIdentifier(LeaseAgentContext->Transport),
            LeaseAgentContext->Instance.QuadPart
            ); 
    }

    return Status;
}

VOID
MoveToUnregisterList(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    )
{
    //
    // Add to the unregister list may fail;
    // The unregister will retry through the timer with UnregisterRetryInterval in milliseconds
    //
    PLEASE_AGENT_CONTEXT LeaseAgentContext = LeasingApplicationContext->LeaseAgentContext;
    //
    // Insert the leasing application to the un-registered list.
    //
    LeaseAgentContext->LeasingApplicationContextUnregisterList.insert(LeasingApplicationContext);
    LeaseAgentContext->LeasingApplicationContextHashTable.erase(LeaseAgentContext->LeasingApplicationContextHashTable.find(LeasingApplicationContext));
}

VOID 
BlockLeaseAgent(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext
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
    if (IsLeaseAgentFailed(LeaseAgentContext)) {

        //
        // We have been here before.
        //
        return;
    }

    EventWriteLeaseAgentBlocked(
        NULL,
        TransportIdentifier(LeaseAgentContext->Transport),
        LeaseAgentContext->Instance.QuadPart);

    //
    // Close listening socket.
    //
    TransportAbort(LeaseAgentContext);

    EventWriteLeaseAgentNowInState(
        NULL,
        TransportIdentifier(LeaseAgentContext->Transport),
        LeaseAgentContext->Instance.QuadPart,
        GetLeaseAgentState(LeaseAgentContext->State)
        ); 
}

NTSTATUS
CloseLeaseAgent(
    WDFREQUEST & Request,
    __in size_t OutputBufferLength,
    __in size_t InputBufferLength,
    __in BOOL FaultLeases
    )

/*++

Routine Description:

    Closes an existent lease agent. Its state is set to failed.

Arguments:
        
    Request - Handle to a framework request object.

    OutputBufferLength - The size of the output buffer, in bytes.

    
    InputBufferLength - The size of the input buffer, in bytes.

Return Value:

    n/a

--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PVOID InputBuffer = NULL;
    BOOLEAN Found = FALSE;
    size_t Length = 0;
    KIRQL OldIrql;
    
    PLEASE_AGENT_CONTEXT LeaseAgentContextLookup = NULL;
    PCLOSE_LEASE_AGENT_BUFFER_DEVICE_IOCTL LeaseAgentInputBuffer = NULL;

    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext();

    //
    // Check request output size.
    //
    if (0 != OutputBufferLength) {

        EventWriteInvalidReq5(
            NULL
            );  

        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;

        goto End;
    }

    //
    // Retrieve the input buffer from the request.
    //
    if (sizeof(CLOSE_LEASE_AGENT_BUFFER_DEVICE_IOCTL) == InputBufferLength) {
        
        Status = WdfRequestRetrieveInputBuffer(
            Request, 
            sizeof(CLOSE_LEASE_AGENT_BUFFER_DEVICE_IOCTL),
            &InputBuffer,
            &Length
            );
        
        if (!NT_SUCCESS(Status)) {
        
            EventWriteRetrieveFail1(
                NULL
                );

            //
            // This is an invalid request.
            //
            goto End;
        }

        LAssert(NULL != InputBuffer);
        LAssert(Length == InputBufferLength);
        
    } else {

        EventWriteInvalidReq1(
            NULL
            );       

        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;

        goto End;
    }

    //
    // Cast input buffer to its type.
    //
    LeaseAgentInputBuffer = (PCLOSE_LEASE_AGENT_BUFFER_DEVICE_IOCTL)InputBuffer;

    //
    // Check input buffer - socket address.
    //
    if (!IsValidListenEndpoint(&LeaseAgentInputBuffer->SocketAddress)) {
    
         EventWriteInvalidReq6(
            NULL
            );   

        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        
        goto End;
    }

    //
    // Construct lease agent for comparison.
    //
    Status = LeaseAgentConstructor(
        &LeaseAgentInputBuffer->SocketAddress,
        NULL, // lease durations
        0, // lease suspend duration
        0, // arbitration duration
        0, // number of lease retry
        0, // start of lease retry
        nullptr, // SecuritySettings
        &LeaseAgentContextLookup
        );

    if (!NT_SUCCESS(Status)) {

        //
        // Cannot complete request.
        //
        goto End;
    }
    
    ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    //
    // Look up the lease agent in the hash table.
    //
    for(PLEASE_AGENT_CONTEXT LeaseAgentContext: DriverContext->LeaseAgentContextHashTable){
        KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

        //
        // Skip over existent lease agents that are failed.
        //
        if (!IsLeaseAgentFailed(LeaseAgentContext)) {

            if (0 == wcscmp(TransportIdentifier(LeaseAgentContext->Transport), TransportIdentifier(LeaseAgentContextLookup->Transport))) {

                //
                // The lease agent is found.
                //
                Found = TRUE;


                //
                // Mark the lease agent as failed and fail/expire all applications.
                //
                if (FaultLeases) {
                    
                    OnLeaseFailure(LeaseAgentContext);
                    
                } else {
                    BlockLeaseAgent(LeaseAgentContext);
                }
            }
        }
        
        KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);
    }
            
    ExReleaseFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    //
    // Deallocate temporary lease agent.
    //
    LeaseAgentDestructor(LeaseAgentContextLookup);
    
    //
    // Set status based on success of the deletion operation.
    //
    if (!Found) {
        
        EventWriteInvalidReq7(
            NULL
            );

        Status = STATUS_INVALID_PARAMETER;
    }

End:
    return Status;
}

NTSTATUS
BlockLeaseConnection(
    WDFREQUEST & Request,
    __in size_t OutputBufferLength,
    __in size_t InputBufferLength
    )

/*++

Routine Description:

    Closes an existent connection; Messages would be dropped.

Arguments:
        
    Request - Handle to a framework request object.

    OutputBufferLength - The size of the output buffer, in bytes.
    
    InputBufferLength - The size of the input buffer, in bytes.

Return Value:

    n/a

--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PVOID InputBuffer = NULL;
    BOOLEAN Found = FALSE;
    size_t Length = 0;
    KIRQL OldIrql;
    
    PLEASE_AGENT_CONTEXT LeaseAgentContextLookup = NULL;

    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContextLookup = NULL;

    PBLOCK_LEASE_CONNECTION_BUFFER_DEVICE_IOCTL LeaseAgentInputBuffer = NULL;

    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext();

    //
    // Check request output size.
    //
    if (0 != OutputBufferLength)
    {
        EventWriteInvalidReq5( NULL );
        Status = STATUS_INVALID_PARAMETER;

        goto End;
    }

    //
    // Retrieve the input buffer from the request.
    //
    if (sizeof(BLOCK_LEASE_CONNECTION_BUFFER_DEVICE_IOCTL) == InputBufferLength)
    {
        Status = WdfRequestRetrieveInputBuffer(
            Request, 
            sizeof(BLOCK_LEASE_CONNECTION_BUFFER_DEVICE_IOCTL),
            &InputBuffer,
            &Length
            );

        if (!NT_SUCCESS(Status))
        {
            EventWriteRetrieveFail1( NULL );
            goto End;
        }

        LAssert(NULL != InputBuffer);
        LAssert(Length == InputBufferLength);
    }
    else
    {
        EventWriteInvalidReq1( NULL );
        Status = STATUS_INVALID_PARAMETER;

        goto End;
    }

    //
    // Cast input buffer to its type.
    //
    LeaseAgentInputBuffer = (PBLOCK_LEASE_CONNECTION_BUFFER_DEVICE_IOCTL)InputBuffer;

    //
    // Check input buffer - socket address.
    //
    if (!IsValidListenEndpoint(&LeaseAgentInputBuffer->LocalSocketAddress) ||
        !IsValidListenEndpoint(&LeaseAgentInputBuffer->RemoteSocketAddress))
    {
         EventWriteInvalidReq6( NULL );
         Status = STATUS_INVALID_PARAMETER;

        goto End;
    }

    //
    // Construct lease agent for comparison.
    //
    Status = LeaseAgentConstructor(
        &LeaseAgentInputBuffer->LocalSocketAddress,
        NULL, // lease durations
        0, // lease suspend duration
        0, // arbitration duration
        0, // number of lease retry
        0, // start of lease retry
        nullptr, // SecuritySettings 
        &LeaseAgentContextLookup
        );

    if (!NT_SUCCESS(Status))
    {
        goto End;
    }

    //
    // Create remote lease agent for comparison.
    //
    Status = RemoteLeaseAgentConstructor(
        NULL,
        &LeaseAgentInputBuffer->RemoteSocketAddress,
        &RemoteLeaseAgentContextLookup
        );

    if (!NT_SUCCESS(Status))
    {
        goto End;
    }

    //EventWriteLeaseDriverTextTraceError(NULL, L"Blocking Lease Connection:", 0);

    ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    //
    // Look up the lease agent in the hash table.
    //
    for(PLEASE_AGENT_CONTEXT LeaseAgentContext: DriverContext->LeaseAgentContextHashTable){

        KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

        if (!IsLeaseAgentFailed(LeaseAgentContext) && 
            (0 == wcscmp(TransportIdentifier(LeaseAgentContext->Transport), TransportIdentifier(LeaseAgentContextLookup->Transport))))
        {
            //
            // Check to see if an existent remote lease agent can be reused.
            //
            RemoteLeaseAgentContext = FindMostRecentRemoteLeaseAgent(
                LeaseAgentContext,
                RemoteLeaseAgentContextLookup
                );

            if (NULL != RemoteLeaseAgentContext)
            {
                Found = TRUE;

                //
                // Check to see if the remote lease agent has had its incoming channel set.
                //
                if (NULL == RemoteLeaseAgentContext->PartnerTarget)
                {

                    TransportResolveTarget(
                        RemoteLeaseAgentContext->LeaseAgentContext->Transport,
                        &RemoteLeaseAgentContext->RemoteSocketAddress,
                        RemoteLeaseAgentContext->PartnerTarget
                        );

                }

                if (RemoteLeaseAgentContext->PartnerTarget)
                {
                    EventWriteBlockingConnection(
                        NULL,
                        TransportIdentifier(LeaseAgentContext->Transport),
                        LeaseAgentContext->Instance.QuadPart,
                        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                        RemoteLeaseAgentContext->Instance.QuadPart,
                        RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                        RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
                        GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
                        LeaseAgentInputBuffer->IsBlocking
                        );

                    TransportBlockConnection(
                        RemoteLeaseAgentContext->PartnerTarget,
                        LeaseAgentInputBuffer->IsBlocking
                        );

                }
                else
                {
                    // trace target null error message
                    EventWriteLeaseDriverTextTraceError(
                        NULL,
                        L"BlockLeaseConnection: target is NULL. ",
                        0
                        );
                }
            }
        }

        KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);
    }

    ExReleaseFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    //
    // Set status based on success of the deletion operation.
    //
    if (!Found)
    {
        EventWriteInvalidReq7( NULL );
        Status = STATUS_INVALID_PARAMETER;
    }

End:

    if (LeaseAgentContextLookup != NULL)
    {
        //
        // Deallocate temporary lease agent.
        //
        LeaseAgentDestructor(LeaseAgentContextLookup);
    }

    if (RemoteLeaseAgentContextLookup != NULL)
    {
        //
        // Deallocate temporary remote lease agent.
        //
        RemoteLeaseAgentDestructor(RemoteLeaseAgentContextLookup);
    }

    return Status;
}

NTSTATUS
SetListenerSecurityDescriptor(
    WDFREQUEST & Request,
    __in size_t OutputBufferLength,
    __in size_t InputBufferLength
    )
/*++
Routine Description:
    - Find the lease agent by the hanlde in the input buffer
    - Pass the security descriptor to the lease agent listener
--*/
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PVOID InputBuffer = NULL;
    BOOLEAN Found = FALSE;
    size_t Length = 0;
    KIRQL OldIrql;

    PSET_SECURITY_DESCRIPTOR_INPUT_BUFFER setSecurityDescriptorBuffer = NULL;
    PTRANSPORT_SECURITY_DESCRIPTOR transportSecurityDescriptor = NULL;

    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext();

    if (0 != OutputBufferLength)
    {
        EventWriteInvalidReq5( NULL );  
        Status = STATUS_INVALID_PARAMETER;
        goto End;
    }

    if (InputBufferLength >= sizeof(SET_SECURITY_DESCRIPTOR_INPUT_BUFFER))
    {        
        Status = WdfRequestRetrieveInputBuffer(
            Request, 
            InputBufferLength,
            &InputBuffer,
            &Length
            );
        
        if (!NT_SUCCESS(Status))
        {        
            EventWriteRetrieveFail1( NULL );
            goto End;
        }

        LAssert(NULL != InputBuffer);
        LAssert(Length == InputBufferLength);
        
    }
    else
    {
        EventWriteInvalidReq1( NULL );       
        Status = STATUS_INVALID_PARAMETER;
        goto End;
    }

    setSecurityDescriptorBuffer = (PSET_SECURITY_DESCRIPTOR_INPUT_BUFFER)InputBuffer;

    Status = TransportCreateSecurityDescriptor(
        &setSecurityDescriptorBuffer->SecurityDescriptor,
        setSecurityDescriptorBuffer->SecurityDescriptorSize,
        &transportSecurityDescriptor);

    if (!NT_SUCCESS(Status))
    {
        EventWriteLeaseDriverTextTraceError(NULL, L"TransportCreateSecurityDescriptor failed ", Status);
        goto End;
    }

    ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    for(PLEASE_AGENT_CONTEXT LeaseAgentContext: DriverContext->LeaseAgentContextHashTable){
        KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

        if (LeaseAgentContext == setSecurityDescriptorBuffer->LeaseAgentHandle && !IsLeaseAgentFailed(LeaseAgentContext))
        {
            Found = TRUE;
            TransportSetSecurityDescriptor(LeaseAgentContext->Transport, transportSecurityDescriptor);
        }
        
        KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);
    }

    ExReleaseFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    if (!Found)
    {
        TransportSecurityDescriptorRelease(transportSecurityDescriptor);
        EventWriteInvalidReq7( NULL );
        Status = STATUS_INVALID_PARAMETER;
    }

End:
    return Status;
}


NTSTATUS
UpdateConfigLeaseDuration
(
     WDFREQUEST & Request,
    __in size_t OutputBufferLength,
    __in size_t InputBufferLength
    )
/*++
Routine Description:
    - Find the lease agent by the hanlde in the input buffer
    - Update the lease duration in the lease agent context
--*/
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PVOID InputBuffer = NULL;
    BOOLEAN Found = FALSE;
    size_t Length = 0;
    KIRQL OldIrql;
    BOOLEAN IsDurationChanged = FALSE;
    BOOLEAN IsDurationAcrossFDChanged = FALSE;

    PUPDATE_LEASE_DURATION_INPUT_BUFFER UpdateLeaseDurationBuffer = NULL;

    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext();

    if (0 != OutputBufferLength)
    {
        EventWriteInvalidReq5( NULL );  
        Status = STATUS_INVALID_PARAMETER;
        goto End;
    }

    if (InputBufferLength >= sizeof(UPDATE_LEASE_DURATION_INPUT_BUFFER))
    {
        Status = WdfRequestRetrieveInputBuffer(
            Request, 
            InputBufferLength,
            &InputBuffer,
            &Length
            );

        if (!NT_SUCCESS(Status))
        {
            EventWriteRetrieveFail1( NULL );
            goto End;
        }

        LAssert(NULL != InputBuffer);
        LAssert(Length == InputBufferLength);
    }
    else
    {
        EventWriteInvalidReq1( NULL );
        Status = STATUS_INVALID_PARAMETER;
        goto End;
    }

    UpdateLeaseDurationBuffer = (PUPDATE_LEASE_DURATION_INPUT_BUFFER)InputBuffer;

    ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    for(PLEASE_AGENT_CONTEXT LeaseAgentContext: DriverContext->LeaseAgentContextHashTable){
        if(Found) break;

        KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

        if (LeaseAgentContext == UpdateLeaseDurationBuffer->LeaseAgentHandle && !IsLeaseAgentFailed(LeaseAgentContext))
        {
            Found = TRUE;
            // Update the durations list in lease agent context
            if (LeaseAgentContext->LeaseConfigDurations.LeaseDuration != UpdateLeaseDurationBuffer->UpdatedLeaseDurations.LeaseDuration)
            {
                IsDurationChanged = TRUE;

                EventWriteUpdateLeaseAgentDuration(
                    NULL,
                    L"Regular",
                    LeaseAgentContext->LeaseConfigDurations.LeaseDuration,
                    UpdateLeaseDurationBuffer->UpdatedLeaseDurations.LeaseDuration);

                LeaseAgentContext->LeaseConfigDurations.LeaseDuration = UpdateLeaseDurationBuffer->UpdatedLeaseDurations.LeaseDuration;
                
                // update the indirect lease limist if lease duration is changed.
                UpdateIndirectLeaseLimit(LeaseAgentContext);
            }

            if (LeaseAgentContext->LeaseConfigDurations.LeaseDurationAcrossFD != UpdateLeaseDurationBuffer->UpdatedLeaseDurations.LeaseDurationAcrossFD)
            {
                IsDurationAcrossFDChanged = TRUE;

                EventWriteUpdateLeaseAgentDuration(
                    NULL,
                    L"AcrossFD",
                    LeaseAgentContext->LeaseConfigDurations.LeaseDurationAcrossFD,
                    UpdateLeaseDurationBuffer->UpdatedLeaseDurations.LeaseDurationAcrossFD);

                LeaseAgentContext->LeaseConfigDurations.LeaseDurationAcrossFD = UpdateLeaseDurationBuffer->UpdatedLeaseDurations.LeaseDurationAcrossFD;
            }

            if (IsDurationChanged || IsDurationAcrossFDChanged)
            {
                // Reset the lease relationship UPDATE flag so that the new duration would be used in the next renew cycle
                for(PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext: LeaseAgentContext->RemoteLeaseAgentContextHashTable){

                    // Only reset the UPDATE flag is the index is equal
                    if (!IsRemoteLeaseAgentFailed(RemoteLeaseAgentContext) &&
                        ((IsDurationChanged && LEASE_DURATION == RemoteLeaseAgentContext->LeaseRelationshipContext->EstablishDurationType) ||
                        (IsDurationAcrossFDChanged && LEASE_DURATION_ACROSS_FAULT_DOMAIN == RemoteLeaseAgentContext->LeaseRelationshipContext->EstablishDurationType))
                        )
                    {
                        EventWriteUpdateRemoteLeaseAgentDurationFlag(
                            NULL,
                            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                            RemoteLeaseAgentContext->Instance.QuadPart,
                            (int)RemoteLeaseAgentContext->LeaseRelationshipContext->EstablishDurationType,
                            RemoteLeaseAgentContext->LeaseRelationshipContext->Duration
                            );

                        RemoteLeaseAgentContext->LeaseRelationshipContext->IsDurationUpdated = TRUE;
                    }
                }
            } // if the input duration is same as lease agent context, do nothing
        }

        KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);
    }

    ExReleaseFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    if (!Found)
    {
        EventWriteInvalidReq7( NULL );
        EventWriteLeaseDriverTextTraceInfo(NULL, L"Updating lease duration didn't find the lease agent handle!");

        Status = STATUS_INVALID_PARAMETER;
    }

End:
    return Status;
}

NTSTATUS
QueryLeaseDuration(
    WDFREQUEST & Request,
    __in size_t OutputBufferLength,
    __in size_t InputBufferLength
    )
/*++
Routine Description:
    Return the current lease relationship duration based on the input local/remote address
--*/
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PVOID InputBuffer = NULL;
    PVOID OutputBuffer = NULL;
    BOOLEAN Found = FALSE;
    size_t Length = 0;
    ULONG BytesTransferred = 0;
    KIRQL OldIrql;

    PLEASE_AGENT_CONTEXT LeaseAgentContextLookup = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContextLookup = NULL;

    PBLOCK_LEASE_CONNECTION_BUFFER_DEVICE_IOCTL QueryDurationInputBuffer = NULL;
    PGET_LEASING_APPLICATION_EXPIRATION_OUTPUT_BUFFER QueryDurationOutputBuffer = NULL;
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext();

    //
    // Retrieve the input buffer from the request.
    //
    if (sizeof(BLOCK_LEASE_CONNECTION_BUFFER_DEVICE_IOCTL) == InputBufferLength)
    {
        Status = WdfRequestRetrieveInputBuffer(
            Request, 
            sizeof(BLOCK_LEASE_CONNECTION_BUFFER_DEVICE_IOCTL),
            &InputBuffer,
            &Length
            );

        if (!NT_SUCCESS(Status))
        {
            EventWriteRetrieveFail1( NULL );
            goto End;
        }

        LAssert(NULL != InputBuffer);
        LAssert(Length == InputBufferLength);
    }
    else
    {
        EventWriteInvalidReq1( NULL );
        Status = STATUS_INVALID_PARAMETER;

        goto End;
    }

    //
    // Retrieve the output buffer from the request.
    //
    if (sizeof(GET_LEASING_APPLICATION_EXPIRATION_OUTPUT_BUFFER) == OutputBufferLength)
    {
        Status = WdfRequestRetrieveOutputBuffer(
            Request, 
            sizeof(GET_LEASING_APPLICATION_EXPIRATION_OUTPUT_BUFFER),
            &OutputBuffer,
            &Length);

        if (!NT_SUCCESS(Status))
        {
            EventWriteRetrieveFail2(NULL);
            goto End;
        }
        LAssert(NULL != OutputBuffer);
        LAssert(Length == OutputBufferLength);
    }
    else
    {
        EventWriteInvalidReq5(NULL);
        Status = STATUS_INVALID_PARAMETER;
        goto End;
    }

    //
    // Cast input and output buffers to their types.
    //
    QueryDurationInputBuffer = (PBLOCK_LEASE_CONNECTION_BUFFER_DEVICE_IOCTL)InputBuffer;
    QueryDurationOutputBuffer = (PGET_LEASING_APPLICATION_EXPIRATION_OUTPUT_BUFFER)OutputBuffer;
    //
    // Check input buffer - socket address.
    //
    if (!IsValidListenEndpoint(&QueryDurationInputBuffer->LocalSocketAddress) ||
        !IsValidListenEndpoint(&QueryDurationInputBuffer->RemoteSocketAddress))
    {
         EventWriteInvalidReq6( NULL );
         Status = STATUS_INVALID_PARAMETER;

        goto End;
    }

    //
    // Construct lease agent for comparison.
    //
    Status = LeaseAgentConstructor(
        &QueryDurationInputBuffer->LocalSocketAddress,
        NULL, // lease durations
        0, // lease suspend duration
        0, // arbitration duration
        0, // number of lease retry
        0, // start of lease retry
        nullptr, // SecuritySettings
        &LeaseAgentContextLookup
        );

    if (!NT_SUCCESS(Status))
    {
        goto End;
    }

    //
    // Create remote lease agent for comparison.
    //
    Status = RemoteLeaseAgentConstructor(
        NULL,
        &QueryDurationInputBuffer->RemoteSocketAddress,
        &RemoteLeaseAgentContextLookup
        );

    if (!NT_SUCCESS(Status))
    {
        goto End;
    }

    ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    for(PLEASE_AGENT_CONTEXT LeaseAgentContext: DriverContext->LeaseAgentContextHashTable){
        if(Found) {
            break;
        }
        KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);
        
        if (!IsLeaseAgentFailed(LeaseAgentContext) && 
            (0 == wcscmp(TransportIdentifier(LeaseAgentContext->Transport), TransportIdentifier(LeaseAgentContextLookup->Transport))))
        {
            //
            // Check to see if an existent remote lease agent can be reused.
            //
            RemoteLeaseAgentContext = FindMostRecentRemoteLeaseAgent(
                LeaseAgentContext,
                RemoteLeaseAgentContextLookup
                );

            if (NULL != RemoteLeaseAgentContext)
            {
                Found = TRUE;

                QueryDurationOutputBuffer->KernelSystemTime = 0;
                BytesTransferred = sizeof(GET_LEASING_APPLICATION_EXPIRATION_OUTPUT_BUFFER);

                EventWriteLeaseDriverTextTraceInfo(NULL, L"QueryLeaseDuration: Found the RLA and return the current lease duration to user mode.");

                QueryDurationOutputBuffer->TimeToLive = RemoteLeaseAgentContext->LeaseRelationshipContext->Duration;
            }
            else
            {
                EventWriteLeaseDriverTextTraceError(
                    NULL,
                    L"QueryLeaseDuration: Couldn't find the requested lease relationship. ",
                    0);
            }
        }

        KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);
    }

    ExReleaseFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    //
    // Set status based on success of the deletion operation.
    //
    if (!Found)
    {
        EventWriteInvalidReq7( NULL );
        Status = STATUS_INVALID_PARAMETER;
    }

End:

    if (LeaseAgentContextLookup != NULL)
    {
        LeaseAgentDestructor(LeaseAgentContextLookup);
    }

    if (RemoteLeaseAgentContextLookup != NULL)
    {
        RemoteLeaseAgentDestructor(RemoteLeaseAgentContextLookup);
    }

    Request->outputLength = BytesTransferred;
    return Status;
}

NTSTATUS
GetRemoteLeaseExpirationTime(
__in WDFREQUEST Request,
__in size_t OutputBufferLength,
__in size_t InputBufferLength
)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG BytesTransferred = 0;
    PVOID InputBuffer = NULL;
    PVOID OutputBuffer = NULL;
    size_t Length = 0;

    PREMOTE_LEASE_EXPIRATION_RESULT_INPUT_BUFFER GetRemoteLeaseExpirationTimeInputBuffer = NULL;
    PREMOTE_LEASE_EXPIRATION_RESULT_OUTPUT_BUFFER GetRemoteLeaseExpirationTimeOutputBuffer = NULL;

    PVOID IterLeaseAgent = NULL;
    PVOID IterLeasingApplication = NULL;
    PVOID IterRemoteLeaseAgent = NULL;
    PVOID IterRemoteLeaseIdentifier = NULL;
    BOOLEAN FoundLeasingApplication = FALSE;
    BOOLEAN FoundRemoteLeasingApplicationIdentifier = FALSE;
    KIRQL OldIrql;
    
    PLEASE_AGENT_CONTEXT LeaseAgentContext = NULL;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContextIter = NULL;

    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier = NULL;

    LARGE_INTEGER Now;
    LARGE_INTEGER TTL;

    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext();
    LONG MonitorTTL = -MAXLONG;
    LONG SubjectTTL = -MAXLONG;
    //
    // Retrieve the input buffer from the request.
    //
    if (sizeof(REMOTE_LEASE_EXPIRATION_RESULT_INPUT_BUFFER) == InputBufferLength) {

        Status = WdfRequestRetrieveInputBuffer(
            Request,
            sizeof(REMOTE_LEASE_EXPIRATION_RESULT_INPUT_BUFFER),
            &InputBuffer,
            &Length
            );

        if (!NT_SUCCESS(Status)) {

            EventWriteRetrieveFail1(
                NULL
                );
            goto End;
        }

        LAssert(NULL != InputBuffer);
        LAssert(Length == InputBufferLength);

    }
    else {

        EventWriteInvalidReq1(
            NULL
            );
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;

        goto End;
    }

    //
    // Retrieve the output buffer from the request.
    //
    if (sizeof(REMOTE_LEASE_EXPIRATION_RESULT_OUTPUT_BUFFER) == OutputBufferLength) {

        Status = WdfRequestRetrieveOutputBuffer(
            Request,
            sizeof(REMOTE_LEASE_EXPIRATION_RESULT_OUTPUT_BUFFER),
            &OutputBuffer,
            &Length
            );

        if (!NT_SUCCESS(Status)) {

            EventWriteRetrieveFail2(
                NULL
                );
            goto End;
        }

        LAssert(NULL != OutputBuffer);
        LAssert(Length == OutputBufferLength);

    }
    else {

        EventWriteInvalidReq5(
            NULL
            );
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;

        goto End;
    }


    GetRemoteLeaseExpirationTimeInputBuffer = (PREMOTE_LEASE_EXPIRATION_RESULT_INPUT_BUFFER)InputBuffer;
    GetRemoteLeaseExpirationTimeOutputBuffer = (PREMOTE_LEASE_EXPIRATION_RESULT_OUTPUT_BUFFER)OutputBuffer;
    //
    // Check arguments.
    //
    if (NULL == GetRemoteLeaseExpirationTimeInputBuffer->LeasingApplicationHandle) {

        EventWriteInvalidReq9(
            NULL
            );
        Status = STATUS_INVALID_PARAMETER;

        goto End;
    }
   
    //
    // Check input buffer - leasing application identifier.
    //
    if (!IsValidString(GetRemoteLeaseExpirationTimeInputBuffer->RemoteLeasingApplicationIdentifier, MAX_PATH + 1)) {

        EventWriteInvalidReq11(
            NULL
            );
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;

        goto End;
    }

    GetCurrentTime(&Now);

    ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);
    //
    // Iterate over the lease agent in the hash table.
    //
    for(auto leaseAgentContextIter = DriverContext->LeaseAgentContextHashTable.begin(); leaseAgentContextIter != DriverContext->LeaseAgentContextHashTable.end() && !FoundLeasingApplication; leaseAgentContextIter++)
    {
        //
        // Retrieve lease agent context.
        //
        LeaseAgentContext = *leaseAgentContextIter;

        KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

        if (IsLeaseAgentFailed(LeaseAgentContext)) {

            //
            // Skip over the previously closed lease agents.
            //
            KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);

            continue;
        }

        //
        // Check to see if this lease agent contains the leasing application.
        //
        for(auto leaseApplicationContextIter = LeaseAgentContext->LeasingApplicationContextHashTable.begin();
            leaseApplicationContextIter != LeaseAgentContext->LeasingApplicationContextHashTable.end() && !FoundLeasingApplication;
            leaseApplicationContextIter++)
        {
            //
            // Retrieve leasing application context.
            //
            LeasingApplicationContext = *leaseApplicationContextIter;

            if (LeasingApplicationContext->Instance == GetRemoteLeaseExpirationTimeInputBuffer->LeasingApplicationHandle)
            {
                //
                // Found the leasing application.
                //
                FoundLeasingApplication = TRUE;

                for(auto remoteLeaseAgentIter = LeaseAgentContext->RemoteLeaseAgentContextHashTable.begin();
                    remoteLeaseAgentIter != LeaseAgentContext->RemoteLeaseAgentContextHashTable.end();
                    remoteLeaseAgentIter++)
                {
                    //
                    // Retrieve remote lease agent.
                    //
                    RemoteLeaseAgentContextIter = *remoteLeaseAgentIter;

                    //
                    // Find the lease identifier within this remote lease agent.
                    //
                    for(auto subjectIter = RemoteLeaseAgentContextIter->SubjectHashTable.begin();
                        subjectIter != RemoteLeaseAgentContextIter->SubjectHashTable.end();
                        subjectIter++)
                    {
                        LeaseRelationshipIdentifier = *subjectIter;
                        GetRemoteLeaseExpirationTimeInputBuffer->RemoteLeasingApplicationIdentifier[MAX_PATH] = L'\0';

                        if (!IsRemoteLeaseAgentInPing(RemoteLeaseAgentContextIter) && 0 == wcscmp(
                            LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
                            GetRemoteLeaseExpirationTimeInputBuffer->RemoteLeasingApplicationIdentifier
                            ))
                        {
                            //
                            // As long as the expired time value is not max, it is valid and ttl is calculated and returned. 
                            // There could be a scenario that 2 remote lease agents exist. This is due to the neutral arbitration result failed
                            // the previous remote lease agent. A new remote lease agent was constructed. The lease may not have been fully
                            // established yet. In this case, we return the previously failed expiration time. We only return the new expiration
                            // time, when there is a new and valid expiration time.
                            //
                            if (RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectExpireTime.QuadPart != MAXLONGLONG)
                            {
                                LARGE_INTEGER_SUBTRACT_LARGE_INTEGER_AND_ASSIGN(TTL, RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectExpireTime, Now);
                                LONG LocalSubjectTTL = (LONG)(TTL.QuadPart / MILLISECOND_TO_NANOSECOND_FACTOR);
                                if (LocalSubjectTTL > SubjectTTL)
                                {
                                    SubjectTTL = LocalSubjectTTL;
                                }
                            }

                            if (RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorExpireTime.QuadPart != MAXLONGLONG)
                            {
                                LARGE_INTEGER_SUBTRACT_LARGE_INTEGER_AND_ASSIGN(TTL, RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorExpireTime, Now);
                                LONG LocalMonitorTTL = (LONG)(TTL.QuadPart / MILLISECOND_TO_NANOSECOND_FACTOR);
                                if (LocalMonitorTTL > MonitorTTL)
                                {
                                    MonitorTTL = LocalMonitorTTL;
                                }
                            }

                            //
                            // It is possible to find multiple leasing application identifiers in multiple remote lease agents. If we received arbitration
                            // neutral for the previous remote lease agent, we could have a period of time to have multiple remote lease agents.
                            //
                            FoundRemoteLeasingApplicationIdentifier = TRUE;
                            EventWriteRemoteLeaseExpirationTimeFound(
                                NULL,
                                TransportIdentifier(LeaseAgentContext->Transport),
                                LeaseAgentContext->Instance.QuadPart,
                                RemoteLeaseAgentContextIter->RemoteLeaseAgentIdentifier,
                                RemoteLeaseAgentContextIter->Instance.QuadPart,
                                RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                                RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                                GetLeaseState(RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectState),
                                GetLeaseState(RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorState),
                                GetRemoteLeaseExpirationTimeInputBuffer->RemoteLeasingApplicationIdentifier,
                                SubjectTTL,
                                MonitorTTL
                                );
                            
                            //
                            // Break remote lease identifier, but continue to loop remote lease agent.
                            //
                            break;
                        }
                    }
                }
            }
        }
        KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);
    }
    ExReleaseFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

End:
    if (FoundRemoteLeasingApplicationIdentifier == TRUE)
    {
        //
        // Since we could find multiple leasing application identifiers in multiple remote lease agents. We trace the final result.
        //
        EventWriteRemoteLeaseExpirationTimeFinal(
            NULL,
            TransportIdentifier(LeaseAgentContext->Transport),
            LeaseAgentContext->Instance.QuadPart,
            RemoteLeaseAgentContextIter->RemoteLeaseAgentIdentifier,
            RemoteLeaseAgentContextIter->Instance.QuadPart,
            RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
            RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
            GetLeaseState(RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectState),
            GetLeaseState(RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorState),
            GetRemoteLeaseExpirationTimeInputBuffer->RemoteLeasingApplicationIdentifier,
            SubjectTTL,
            MonitorTTL
            );
    }
    else
    {
        if (LeaseAgentContext == NULL)
        {
            EventWriteLeaseDriverTextTraceInfo(NULL, L"GetRemoteLeaseExpirationTime LeaseAgentContext is null");
            EventWriteLeaseDriverTextTraceInfo(NULL, GetRemoteLeaseExpirationTimeInputBuffer->RemoteLeasingApplicationIdentifier);
        }
        else
        {
            EventWriteRemoteLeaseExpirationTimeNotFound(
                NULL,
                TransportIdentifier(LeaseAgentContext->Transport),
                LeaseAgentContext->Instance.QuadPart,
                L"",
                0,
                0,
                0,
                L"",
                L"",
                GetRemoteLeaseExpirationTimeInputBuffer->RemoteLeasingApplicationIdentifier,
                SubjectTTL,
                MonitorTTL
                );
        }
    }
    
    //
    // Complete the request.
    //

    GetRemoteLeaseExpirationTimeOutputBuffer->MonitorExpireTTL = MonitorTTL;
    GetRemoteLeaseExpirationTimeOutputBuffer->SubjectExpireTTL = SubjectTTL;

    if (NT_SUCCESS(Status))
    {
        //
        // Set the number of bytes transferred.
        //
        BytesTransferred = sizeof(REMOTE_LEASE_EXPIRATION_RESULT_OUTPUT_BUFFER);
    }

    return Status;
}

NTSTATUS
removeLeaseBehavior(
    __in WDFREQUEST Request,
    __in size_t OutputBufferLength,
    __in size_t InputBufferLength
)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PVOID InputBuffer = NULL;
    size_t Length = 0;

    PTRANSPORT_BEHAVIOR_BUFFER LeaseAgentInputBuffer = NULL;
    PVOID IterTransportBlockingBehavior = NULL;
    PTRANSPORT_BLOCKING_TEST_DESCRIPTOR descriptor = NULL;
    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext();
    BOOLEAN ClearAll = FALSE;
    BOOLEAN Deleted = FALSE;
    //
    // Check request output size.
    //
    if (0 != OutputBufferLength)
    {
        EventWriteInvalidReq5(NULL);
        Status = STATUS_INVALID_PARAMETER;

        return Status;
    }

    //
    // Retrieve the input buffer from the request.
    //
    if (sizeof(TRANSPORT_BEHAVIOR_BUFFER) == InputBufferLength)
    {
        Status = WdfRequestRetrieveInputBuffer(
            Request,
            sizeof(TRANSPORT_BEHAVIOR_BUFFER),
            &InputBuffer,
            &Length
            );

        if (!NT_SUCCESS(Status))
        {
            EventWriteRetrieveFail1(NULL);
            return Status;
        }

        LAssert(NULL != InputBuffer);
        LAssert(Length == InputBufferLength);
    }
    else
    {
        EventWriteInvalidReq1(NULL);
        Status = STATUS_INVALID_PARAMETER;

        return Status;
    }

    //
    // Cast input buffer to its type.
    //
    LeaseAgentInputBuffer = (PTRANSPORT_BEHAVIOR_BUFFER)InputBuffer;

    if (L'\0' == LeaseAgentInputBuffer->Alias[0])
    {
        EventWriteRemoveLeaseBehavior(NULL, L"*");
    }
    else
    {
        EventWriteRemoveLeaseBehavior(NULL, LeaseAgentInputBuffer->Alias);
    }

    if (L'\0' == LeaseAgentInputBuffer->Alias[0])
    {
        ClearAll = TRUE;
    }

    KIRQL OldIrql;
    KeAcquireSpinLock(&DriverContext->TransportBehaviorTableAccessSpinLock, &OldIrql);
    for(auto iter = DriverContext->TransportBehaviorTable.begin(); iter != DriverContext->TransportBehaviorTable.end();)
    {
        descriptor = *iter;

        if (ClearAll || (0 == wcscmp(LeaseAgentInputBuffer->Alias, descriptor->Alias)))
        {
            iter = DriverContext->TransportBehaviorTable.erase(iter);
        }
        else
        {
            iter++;
        }
    }

    KeReleaseSpinLock(&DriverContext->TransportBehaviorTableAccessSpinLock, OldIrql);

    return Status;
}

NTSTATUS
addLeaseBehavior(
    __in WDFREQUEST Request,
    __in size_t OutputBufferLength,
    __in size_t InputBufferLength
)

/*++

    Routine Description:

    Add a behavior in the lease transportation layer for testing purpose

    Arguments:

    Request - Handle to a framework request object.

    OutputBufferLength - The size of the output buffer, in bytes.

    InputBufferLength - The size of the input buffer, in bytes.

    Return Value:

    n/a

--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PVOID InputBuffer = NULL;
    size_t Length = 0;

    PTRANSPORT_BEHAVIOR_BUFFER LeaseAgentInputBuffer = NULL;

    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext();

    //
    // Check request output size.
    //
    if (0 != OutputBufferLength)
    {
        EventWriteInvalidReq5(NULL);
        Status = STATUS_INVALID_PARAMETER;

        return Status;
    }

    //
    // Retrieve the input buffer from the request.
    //
    if (sizeof(TRANSPORT_BEHAVIOR_BUFFER) == InputBufferLength)
    {
        Status = WdfRequestRetrieveInputBuffer(
            Request,
            sizeof(TRANSPORT_BEHAVIOR_BUFFER),
            &InputBuffer,
            &Length
            );

        if (!NT_SUCCESS(Status))
        {
            EventWriteRetrieveFail1(NULL);
            return Status;
        }

        LAssert(NULL != InputBuffer);
        LAssert(Length == InputBufferLength);
    }
    else
    {
        EventWriteInvalidReq1(NULL);
        Status = STATUS_INVALID_PARAMETER;

        return Status;
    }

    //
    // Cast input buffer to its type.
    //
    LeaseAgentInputBuffer = (PTRANSPORT_BEHAVIOR_BUFFER)InputBuffer;

    EventWriteAddLeaseBehavior(
        NULL,
        LeaseAgentInputBuffer->FromAny,
        LeaseAgentInputBuffer->LocalSocketAddress.Address,
        LeaseAgentInputBuffer->LocalSocketAddress.Port,
        LeaseAgentInputBuffer->ToAny,
        LeaseAgentInputBuffer->RemoteSocketAddress.Address,
        LeaseAgentInputBuffer->RemoteSocketAddress.Port,
        GetBlockMessageType(LeaseAgentInputBuffer->BlockingType),
        LeaseAgentInputBuffer->Alias);

    PTRANSPORT_BLOCKING_TEST_DESCRIPTOR desc = new TRANSPORT_BLOCKING_TEST_DESCRIPTOR();

    LAssert(NULL != desc);
    
    desc->FromAny = LeaseAgentInputBuffer->FromAny;
    desc->ToAny = LeaseAgentInputBuffer->ToAny;
    desc->LocalSocketAddress = LeaseAgentInputBuffer->LocalSocketAddress;
    desc->RemoteSocketAddress = LeaseAgentInputBuffer->RemoteSocketAddress;
    desc->BlockingType = LeaseAgentInputBuffer->BlockingType;
    
    Status = memcpy_s(&desc->Alias, TEST_TRANSPORT_ALIAS_LENGTH, &LeaseAgentInputBuffer->Alias, TEST_TRANSPORT_ALIAS_LENGTH);
    LAssert(STATUS_SUCCESS == Status);

    desc->Instance = GetInstance();

    KIRQL OldIrql;
    KeAcquireSpinLock(&DriverContext->TransportBehaviorTableAccessSpinLock, &OldIrql);
    DriverContext->TransportBehaviorTable.insert(desc);
    KeReleaseSpinLock(&DriverContext->TransportBehaviorTableAccessSpinLock, OldIrql);

    return Status;
}

NTSTATUS
CreateLeasingApplication(
    WDFREQUEST & Request,
    __in size_t OutputBufferLength,
    __in size_t InputBufferLength
    )

/*++

Routine Description:

    Creates a leasing application on a lease agent. 

Arguments:
        
    Request - Handle to a framework request object.

    OutputBufferLength - The size of the output buffer, in bytes.
    
    InputBufferLength - The size of the input buffer, in bytes.

Return Value:

    n/a

--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PVOID InputBuffer = NULL;
    PVOID OutputBuffer = NULL;
    ULONG BytesTransferred = 0;
    size_t Length = 0;
    BOOLEAN Inserted = FALSE;
    BOOLEAN Deleted = FALSE;
    KIRQL OldIrql;
    BOOLEAN Found = FALSE;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    PCREATE_LEASING_APPLICATION_INPUT_BUFFER_DEVICE_IOCTL CreateLeasingApplicationInputBuffer = NULL;
    PLEASING_APPLICATION_BUFFER_DEVICE_IOCTL LeasingApplicationOutputBuffer = NULL;
    PEPROCESS UserModeCallingProcessHandle = NULL;

    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext();

    //
    // Retrieve the calling process handle.
    //
    UserModeCallingProcessHandle = PsGetCurrentProcess();
    //LAssert(NULL != UserModeCallingProcessHandle);

    //
    // Retrieve the input buffer from the request.
    //
    if (sizeof(CREATE_LEASING_APPLICATION_INPUT_BUFFER_DEVICE_IOCTL) == InputBufferLength) {
        
        Status = WdfRequestRetrieveInputBuffer(
            Request, 
            sizeof(CREATE_LEASING_APPLICATION_INPUT_BUFFER_DEVICE_IOCTL),
            &InputBuffer,
            &Length
            );
        
        if (!NT_SUCCESS(Status)) {
        
            EventWriteRetrieveFail1(
                NULL
                );
            goto End;
        }
        
        LAssert(NULL != InputBuffer);
        LAssert(Length == InputBufferLength);
        
    } else {

        EventWriteInvalidReq1(
            NULL
            );        
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        
        goto End;
    }

    //
    // Retrieve the output buffer from the request.
    //
    if (sizeof(LEASING_APPLICATION_BUFFER_DEVICE_IOCTL) == OutputBufferLength) {
        
        Status = WdfRequestRetrieveOutputBuffer(
            Request, 
            sizeof(LEASING_APPLICATION_BUFFER_DEVICE_IOCTL),
            &OutputBuffer,
            &Length
            );
        
        if (!NT_SUCCESS(Status)) {
        
            EventWriteRetrieveFail2(
                NULL
                ); 
            goto End;
        }
        
        LAssert(NULL != OutputBuffer);
        LAssert(Length == OutputBufferLength);
        
    } else {

         EventWriteInvalidReq5(
            NULL
            );       
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        
        goto End;
    }

    //
    // Cast input and output buffers to their types.
    // The input and output buffers are identical.
    // Take caution when setting data in the output 
    // buffer, it should be done only at the end after
    // the data from the input buffer has been processed.
    //
    CreateLeasingApplicationInputBuffer = (PCREATE_LEASING_APPLICATION_INPUT_BUFFER_DEVICE_IOCTL)InputBuffer;
    LeasingApplicationOutputBuffer = (PLEASING_APPLICATION_BUFFER_DEVICE_IOCTL)OutputBuffer;

    //
    // Check input buffer - lease agent handle.
    //
    if (NULL == CreateLeasingApplicationInputBuffer->LeaseAgentHandle) {

        EventWriteInvalidReq7(
            NULL
            );
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        
        goto End;
    }

    //
    // Check input buffer - leasing application identifier.
    //
    if (!IsValidString(CreateLeasingApplicationInputBuffer->LeasingApplicationIdentifier, MAX_PATH + 1)) {

        EventWriteInvalidReq8(
            NULL
            );
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        
        goto End;
    }

    //
    // Construct leasing application structure.
    //
    CreateLeasingApplicationInputBuffer->LeasingApplicationIdentifier[MAX_PATH] = L'\0';
    LeasingApplicationContext = LeasingApplicationConstructor(
        CreateLeasingApplicationInputBuffer->LeasingApplicationIdentifier,
        UserModeCallingProcessHandle
        );

    if (NULL == LeasingApplicationContext) {

        //
        // Request cannot be completed.
        //
        Status = STATUS_INSUFFICIENT_RESOURCES;
        
        goto End;
    }

    memcpy(&LeasingApplicationContext->Callbacks, &CreateLeasingApplicationInputBuffer->Callbacks, sizeof(CreateLeasingApplicationInputBuffer->Callbacks));

    ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    //
    // Look up the lease agent in the hash table.
    //
    for(PLEASE_AGENT_CONTEXT LeaseAgentContext: DriverContext->LeaseAgentContextHashTable){
        KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);
        //
        // Check the lease agent handle against the lease agent address.
        //
        if (LeaseAgentContext == CreateLeasingApplicationInputBuffer->LeaseAgentHandle) {

            //
            // Lease Agent was found (TODO:linux specific logic)
            //
            Found = TRUE;
            
            if (IsLeaseAgentFailed(LeaseAgentContext))
            {
                //
                // If the lease agent is already closed, let user mode retry.
                // This could happen when the maintenance thread close the lease agent between
                // the CreateLeaseAgent and CreateLeasingApplication calls
                //
                LeasingApplicationDestructor(LeasingApplicationContext);

                // Trave invalid lease agent handle
                EventWriteInvalidReq7(NULL);

                // Invalid request, but not fatal; let user mode retry
                Status = STATUS_RETRY;

                KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);
                
                break;
            }
            auto size = LeaseAgentContext->LeasingApplicationContextHashTable.size();
            //
            // Attempt to add the leasing application.
            //
            LeaseAgentContext->LeasingApplicationContextHashTable.insert(LeasingApplicationContext);
            //
            // Check insert result.
            //
            //
            // Pre-allocate all leasing application event callback.
            //
            if (EnqueueLeasingApplicationAllLeaseEvents(LeasingApplicationContext)) {
                EventWriteLeaseAppCreatedOnLeaseAgent3(
                        NULL,
                        LeasingApplicationContext->LeasingApplicationIdentifier,
                        TransportIdentifier(LeaseAgentContext->Transport),
                        LeaseAgentContext->Instance.QuadPart,
                        NULL
                );
                //
                // Set the arbitration enabled flag.
                //
                LeasingApplicationContext->IsArbitrationEnabled = CreateLeasingApplicationInputBuffer->IsArbitrationEnabled;
                LeasingApplicationContext->LeasingApplicationExpiryTimeout = CreateLeasingApplicationInputBuffer->LeasingAppExpiryTimeoutMilliseconds;

                //
                // Set the lease agent that this application registered on.
                // When the unregister timer fires, this lease agent context would be used to terminate all lease relationships
                //
                LeasingApplicationContext->LeaseAgentContext = LeaseAgentContext;
                //
                // Populate the output buffer with new leasing application handle and the instance to avoid duplicate 
                // for stale and this newly created application.
                //
                LeasingApplicationOutputBuffer->LeasingApplicationHandle = LeasingApplicationContext->Instance;
                //
                // Set the number of bytes transferred.
                //
                BytesTransferred = sizeof(LEASING_APPLICATION_BUFFER_DEVICE_IOCTL);

                //
                // Leasing application created successfully.
                //
                Status = STATUS_SUCCESS;
            }
            else {
                LeaseAgentContext->LeasingApplicationContextHashTable.erase(LeaseAgentContext->LeasingApplicationContextHashTable.find(LeasingApplicationContext));
                //
                // Deallocate leasing application.
                //
                LeasingApplicationDestructor(LeasingApplicationContext);

                //
                // Pre-allocation events failed.
                //
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }

            KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);
            
            break; // the loop
        }

        KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);
    }
            
    ExReleaseFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    //
    // Check to see if the lease agent was found.
    //
    if (!Found) {

        //
        // Deallocate leasing application.
        //
        LeasingApplicationDestructor(LeasingApplicationContext);

        EventWriteInvalidReq3(
            NULL
            );
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
    }

End:
    Request->outputLength = BytesTransferred;
    return Status;
}

NTSTATUS
RegisterLeasingApplication(
    WDFREQUEST & Request,
    __in size_t OutputBufferLength,
    __in size_t InputBufferLength
    )

/*++

Routine Description:

    Registers a leasing application with a lease agent. The leasing 
    application must have been created already on the lease agent.

Arguments:
        
    Request - Handle to a framework request object.

    OutputBufferLength - The size of the output buffer, in bytes.
    
    InputBufferLength - The size of the input buffer, in bytes.

Return Value:

    n/a

--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PVOID InputBuffer = NULL;
    PVOID OutputBuffer = NULL;
    PVOID IterLeaseAgent = NULL;
    LEASING_APPLICATION_CONTEXT_SET::iterator IterLeasingApplication;
    size_t Length = 0;
    BOOLEAN Found = FALSE;
    BOOLEAN Delayed = FALSE;
    KIRQL OldIrql;
    
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER RegisterLeasingApplicationInputBuffer = NULL;

    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext();

    //
    // Retrieve the input buffer from the request.
    //
    if (sizeof(LEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER) == InputBufferLength) {
        
        Status = WdfRequestRetrieveInputBuffer(
            Request, 
            sizeof(LEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER),
            &InputBuffer,
            &Length
            );
        
        if (!NT_SUCCESS(Status)) {

            EventWriteRetrieveFail1(
                NULL
                );
            goto End;
        }
        
        LAssert(NULL != InputBuffer);
        LAssert(Length == InputBufferLength);
        
    } else {

        EventWriteInvalidReq1(
            NULL
            );        
        goto End;
    }

    //
    // Retrieve the output buffer from the request.
    //
    if (sizeof(LEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER) == OutputBufferLength) {
        
        Status = WdfRequestRetrieveOutputBuffer(
            Request, 
            sizeof(LEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER),
            &OutputBuffer,
            &Length
            );
        
        if (!NT_SUCCESS(Status)) {
        
            EventWriteRetrieveFail2(
                NULL
                ); 
            goto End;
        }
        
        LAssert(NULL != OutputBuffer);
        LAssert(Length == OutputBufferLength);
        
    } else {

        EventWriteInvalidReq5(
            NULL
            );        
        goto End;
    }

    //
    // Cast input and output buffers to their types.
    // The input and output buffers are identical.
    // Take caution when setting data in the output 
    // buffer, it should be done only at the end after
    // the data from the input buffer has been processed.
    //
    RegisterLeasingApplicationInputBuffer = (PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER)InputBuffer;

    ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    //
    // Look up the lease agent in the hash table that contains 
    // this leasing application.
    //
    for(PLEASE_AGENT_CONTEXT LeaseAgentContext: DriverContext->LeaseAgentContextHashTable){
        if(Found) break;
        KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

        //
        // In this routine we cannot skip failed lease agents. The reason
        // is that they may have become failed while the user mode was
        // in the process of consuming the latest delayed I/O. If we were
        // to skip the failed lease agents, they won't be able to receive
        // the leasing application expired event which has bene posted 
        // in the meantime.
        //
        
        //
        // Check input handle to see if it matches.
        //
        if (LeaseAgentContext == RegisterLeasingApplicationInputBuffer->LeaseAgentHandle) {
            
            //
            // Check to see if this lease agent contains the leasing application.
            //
            for(IterLeasingApplication = LeaseAgentContext->LeasingApplicationContextHashTable.begin();
                    IterLeasingApplication != LeaseAgentContext->LeasingApplicationContextHashTable.end();
                    IterLeasingApplication++)
            {
                LeasingApplicationContext = *IterLeasingApplication;
                if (LeasingApplicationContext->Instance == RegisterLeasingApplicationInputBuffer->LeasingApplicationHandle &&
                    LeasingApplicationContext->ProcessHandle == PsGetCurrentProcess() &&
                    FALSE == LeasingApplicationContext->IsBeingUnregistered) {

                    //
                    // Leasing application found.
                    //
                    Found = TRUE;

                    //
                    // Check to see if there isn't a pending request already.
                    //
                    if (NULL == LeasingApplicationContext->DelayedRequest) {

                        EventWriteLeaseAppOnLeaseAgentInState4(
                            NULL,
                            LeasingApplicationContext->LeasingApplicationIdentifier,
                            TransportIdentifier(LeaseAgentContext->Transport),
                            LeaseAgentContext->Instance.QuadPart,
                            GetLeaseAgentState(LeaseAgentContext->State)
                            );

                        //
                        // This request will now be delayed. It will be completed
                        // at a later time when the leasing application expires.
                        //
                        Delayed = TRUE;

                        //
                        // Delay the request's completion.
                        //
                        LeasingApplicationContext->DelayedRequest = Request;

                        //
                        // Set the cancel routine for the request. This is required if the app decides to
                        // exit, cancel the event prematurely, or he driver to unload.
                        //
                        WdfRequestMarkCancelable(Request, DelayedRequestCancelation);

                        //
                        // Check to see if new lease events have been completed.
                        //
                        TryDequeueLeasingApplicationLeaseEvent(LeasingApplicationContext);

                        EventWriteLeaseAppOnLeaseAgentInState6(
                            NULL,
                            LeasingApplicationContext->LeasingApplicationIdentifier,
                            TransportIdentifier(LeaseAgentContext->Transport),
                            LeaseAgentContext->Instance.QuadPart,
                            GetLeaseAgentState(LeaseAgentContext->State)
                            );

                    } else {

                        EventWriteLeaseAppOnLeaseAgentInState5(
                            NULL,
                            LeasingApplicationContext->LeasingApplicationIdentifier,
                            TransportIdentifier(LeaseAgentContext->Transport),
                            LeaseAgentContext->Instance.QuadPart,
                            GetLeaseAgentState(LeaseAgentContext->State)
                            );
                    }

                    break;
                }
            }
        }

        KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);
    }

    ExReleaseFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

End:

    //
    // Check to see if the request was delayed. If not, 
    // complete it not with failure code.
    //
    return Delayed? STATUS_PENDING : Status;
}

VOID
GetLeasingApplicationTTL(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in LONG RequestTTL,
    __out PLONG TimeToLive
    )
{

    PVOID IterRemoteLeaseAgent = NULL;
    LARGE_INTEGER Now;
    LARGE_INTEGER Min;
    PLEASE_AGENT_CONTEXT LeaseAgentContext = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContextForDebug = NULL;
    LONG ttl = 0;
    LARGE_INTEGER lastGrantTime;
    LARGE_INTEGER grantExpireTime;

    *TimeToLive = 0;

    LAssert(NULL != LeasingApplicationContext);

    LeaseAgentContext = LeasingApplicationContext->LeaseAgentContext;
    //
    // Compute remaining TTL.
    //
    if (IsLeaseAgentFailed(LeaseAgentContext) || 
        TRUE == LeaseAgentContext->IsInDelayTimer ||
        TRUE == LeasingApplicationContext->IsBeingUnregistered)
    {
        LeaseTrace::WriteInfo("GetLeasingApplicationTTL", "Lease Agent Failed or Is Being Unregistered.");
        return;
    }

    //
    // Get current time
    //
    GetCurrentTime(&Now);

    if (RequestTTL <= 0)
    {
        // Being called for failure report, use last grant time
        lastGrantTime = LeasingApplicationContext->LastGrantExpireTime;

        if (MAXLONGLONG != lastGrantTime.QuadPart && IS_LARGE_INTEGER_GREATER_THAN_OR_EQUAL_LARGE_INTEGER(lastGrantTime, Now))
        {
            // If already passed the last grant time, ttl of 0 is used since there's no need to delay
            LARGE_INTEGER_SUBTRACT_LARGE_INTEGER_AND_ASSIGN(lastGrantTime, lastGrantTime, Now);

            ttl = (LONG)(lastGrantTime.QuadPart / MILLISECOND_TO_NANOSECOND_FACTOR);

            EventWriteGetExpireTimeForFailureReport(
                NULL,
                LeasingApplicationContext->LeasingApplicationIdentifier,
                ttl,
                RequestTTL
                );
        }

        *TimeToLive = ttl;

        return;
    }

    //
    // Find minimum of subject suspend time amongst all remote lease agents.
    //
    Min.QuadPart = MAXLONGLONG;
    for(PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext: LeaseAgentContext->RemoteLeaseAgentContextHashTable){

        if (IsRemoteLeaseAgentFailed(RemoteLeaseAgentContext))
        {
            //
            // Retrieve the time to live from the remaining alive remote lease agents.
            //
            continue;
        }

        if (IS_LARGE_INTEGER_GREATER_THAN_OR_EQUAL_LARGE_INTEGER(
            Min,
            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectSuspendTime))
        {
            Min = RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectSuspendTime;
            RemoteLeaseAgentContextForDebug = RemoteLeaseAgentContext;
        }
    }
    //
    // (MAXLONGLONG == Min.QuadPart) means there no machine lease
    // Global lease should be ignored in this case; 
    // otherwise, the user mode may cache a long expiration TTL
    //
    if (MAXLONGLONG != Min.QuadPart && IS_LARGE_INTEGER_GREATER_THAN_OR_EQUAL_LARGE_INTEGER(
        Min,
        LeasingApplicationContext->GlobalLeaseExpireTime))
    {
        Min = LeasingApplicationContext->GlobalLeaseExpireTime;
    }

    if (MAXLONGLONG == Min.QuadPart) 
    {
        //
        // This lease agent only has monitor relationships.
        //
        *TimeToLive = LeaseAgentContext->LeaseConfigDurations.LeaseDuration;
        
        // TODO
        EventWriteGetLeasingApplicationTTLMax(
            NULL,
            LeasingApplicationContext->LeasingApplicationIdentifier,
            Now.QuadPart,
            RequestTTL
            );
    }
    else
    {
        if (IS_LARGE_INTEGER_GREATER_THAN_OR_EQUAL_LARGE_INTEGER(Now, Min))
        {
            
            if(RemoteLeaseAgentContextForDebug == NULL)
            {
                LeaseTrace::WriteWarning("GetLeasingApplicationTTL", "Now: {0}, Min: {1}, GlobalLeaseExpiredTime: {2} ", 
                    Now.QuadPart, Min.QuadPart, LeasingApplicationContext->GlobalLeaseExpireTime.QuadPart);
            }
            else
            {
                LeaseTrace::WriteWarning(
                    "GetLeasingApplicationTTL", "Now: {0}, Min: {1}, GlobalLeaseExpiredTime: {2}, RemoteLeaseAgent {3}, SubjectSuspendTime {4}", 
                    Now.QuadPart, Min.QuadPart, LeasingApplicationContext->GlobalLeaseExpireTime.QuadPart, 
                    RemoteLeaseAgentContextForDebug->RemoteLeaseAgentIdentifier, RemoteLeaseAgentContextForDebug->LeaseRelationshipContext->SubjectSuspendTime.QuadPart);
            }
            
            *TimeToLive = 0;

            EventWriteGetLeasingApplicationTTLZero(
                NULL,
                LeasingApplicationContext->LeasingApplicationIdentifier,
                Now.QuadPart,
                RequestTTL
                );
        }
        else
        {
            LARGE_INTEGER_SUBTRACT_LARGE_INTEGER_AND_ASSIGN(Min, Min, Now);
            ttl = (LONG) (Min.QuadPart / MILLISECOND_TO_NANOSECOND_FACTOR);

            EventWriteGetExpireTimeWithRequestTTL(
                NULL,
                LeasingApplicationContext->LeasingApplicationIdentifier,
                ttl,
                RequestTTL
                );

            if (RequestTTL > 0)
            {
                // TODO linux only logic, the real ttl is returned and out put to a 
                // memory-mapped file
                // ttl = LeasingApplicationContext->LeasingApplicationExpiryTimeout;

                if (ttl < LeasingApplicationContext->LeasingApplicationExpiryTimeout)
                {
                    // Warning trace when the ttl is less than the configed timeout
                    EventWriteRemainingLeaseTimeoutLessThanConfigExpiryTimeout(
                        NULL,
                        LeasingApplicationContext->LeasingApplicationIdentifier,
                        ttl,
                        LeasingApplicationContext->LeasingApplicationExpiryTimeout,
                        RequestTTL
                        );
                }

                // if there is passed-in TTL, use it only if less than actual TTL;
                // otherwise, actual TTL is used
                if (RequestTTL < ttl)
                {
                    ttl = RequestTTL;
                }

                // Update the granted expire time for delayed failure report
                //LeasingApplicationContext->LastGrantExpireTime.QuadPart = Now.QuadPart + (LONGLONG)(ttl * MILLISECOND_TO_NANOSECOND_FACTOR);
                grantExpireTime.QuadPart = Now.QuadPart + (LONGLONG)(ttl * MILLISECOND_TO_NANOSECOND_FACTOR);

                if (MAXLONGLONG == LeasingApplicationContext->LastGrantExpireTime.QuadPart ||
                    IS_LARGE_INTEGER_GREATER_THAN_LARGE_INTEGER(grantExpireTime, LeasingApplicationContext->LastGrantExpireTime))
                {
                    // Only update the LastGrantExpireTime if the current one is larger
                    LeasingApplicationContext->LastGrantExpireTime.QuadPart = grantExpireTime.QuadPart;
                }
            }

            *TimeToLive = ttl;
        }
        LAssert(0 <= *TimeToLive);
    }

    return;
}

NTSTATUS
CleanLeasingApplication(
    __in PLEASE_AGENT_CONTEXT LeaseAgentContext,
    __in PEPROCESS TerminatingProcessHandle,
    __in HANDLE TerminatingProcessId,
    __in_opt LPCWSTR LeasingApplicationIdentifier,
    __in_opt HANDLE LeasingApplicationHandle,
    __in_opt BOOLEAN IsDelayed
    )

/*++

Routine Description:

    Removes a leasing application from a lease agent. 

Arguments:
        
    Request - Handle to a framework request object.

    TerminatingProcessHandle - process handle that registered the leasing aplication.
    
    LeasingApplicationIdentifier - leasing application identifier.

    LeasingApplicationHandle - leasing application handle.

Return Value:

    NTSTATUS of UnregisterApplication or STATUS_INVALID_PARAMETER.

--*/

{
    NTSTATUS Status = STATUS_INVALID_PARAMETER;
    BOOLEAN Compare = FALSE;

    LONG LeasingApplicationTTL;
    
    //
    // Check to see if this lease agent contains the leasing application.
    //
    for(PLEASING_APPLICATION_CONTEXT LeasingApplicationContext: LeaseAgentContext->LeasingApplicationContextHashTable){
        //
        // Retrieve leasing application context.
        //
        if (NULL != LeasingApplicationIdentifier) {

            Compare = (0 == wcscmp(
                LeasingApplicationContext->LeasingApplicationIdentifier,
                LeasingApplicationIdentifier
                ) );
            Compare = Compare && (TerminatingProcessHandle == LeasingApplicationContext->ProcessHandle);

        } else if (NULL != LeasingApplicationHandle) {

            Compare = (LeasingApplicationHandle == LeasingApplicationContext->Instance);
            Compare = Compare && (TerminatingProcessHandle == LeasingApplicationContext->ProcessHandle);
            
        } else {

            Compare = (TerminatingProcessHandle == LeasingApplicationContext->ProcessHandle);
        }
        
        if (Compare) {
            EventWriteCleanupApplication(
                NULL,
                (UINT64)TerminatingProcessId,
                TransportIdentifier(LeaseAgentContext->Transport),
                LeasingApplicationContext->LeasingApplicationIdentifier);
    
            //
            // Unregister this leasing application from this lease agent.
            //
            if (FALSE == LeasingApplicationContext->IsBeingUnregistered)
            {

                if (TRUE == IsDelayed)
                {
                    GetLeasingApplicationTTL(
                        LeasingApplicationContext,
                        0,
                        &LeasingApplicationTTL
                        );
                }
                else
                {
                    LeasingApplicationTTL = 0;
                }

                StartUnregisterApplication(
                    LeasingApplicationContext,
                    LeasingApplicationTTL
                    );

                Status = STATUS_SUCCESS;

                if (NULL != LeasingApplicationIdentifier || 
                    NULL != LeasingApplicationHandle ||
                    IsLeaseAgentFailed(LeaseAgentContext)) {
                
                    //
                    // Done with this application on this lease agent.
                    //
                    break;
                }
            }
            else
            {
                EventWriteLeasingAppIsBeingUnregistered(
                    NULL,
                    LeasingApplicationContext->LeasingApplicationIdentifier,
                    LeasingApplicationContext->IsBeingUnregistered
                    );
            }

        }
    }

    return Status;
}

NTSTATUS
UnregisterLeasingApplication(
    WDFREQUEST & Request,
    __in size_t OutputBufferLength,
    __in size_t InputBufferLength
    )

/*++

Routine Description:

    Unregisters a leasing application from a lease agent. 

Arguments:
        
    Request - Handle to a framework request object.

    OutputBufferLength - The size of the output buffer, in bytes.
    
    InputBufferLength - The size of the input buffer, in bytes.

Return Value:

    n/a

--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PVOID InputBuffer = NULL;
    size_t Length = 0;
    BOOLEAN Found = FALSE;
    KIRQL OldIrql;

    PUNREGISTER_LEASING_APPLICATION_INPUT_BUFFER_DEVICE_IOCTL LeasingApplicationInputBuffer = NULL;

    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext();

    //
    // Check request output size.
    //
    if (0 != OutputBufferLength) {
    
        EventWriteInvalidReq5(
            NULL
            );        
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        
        goto End;
    }
    
    //
    // Retrieve the input buffer from the request.
    //
    if (sizeof(UNREGISTER_LEASING_APPLICATION_INPUT_BUFFER_DEVICE_IOCTL) == InputBufferLength) {
        
        Status = WdfRequestRetrieveInputBuffer(
            Request, 
            sizeof(UNREGISTER_LEASING_APPLICATION_INPUT_BUFFER_DEVICE_IOCTL),
            &InputBuffer,
            &Length
            );
        
        if (!NT_SUCCESS(Status)) {
        
            EventWriteRetrieveFail1(
                NULL
                );    
            goto End;
        }
    
        LAssert(NULL != InputBuffer);
        LAssert(Length == InputBufferLength);
        
    } else {
    
        EventWriteInvalidReq1(
            NULL
            );        
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        
        goto End;
    }
    
    //
    // Cast input buffer to its type.
    //
    LeasingApplicationInputBuffer = (PUNREGISTER_LEASING_APPLICATION_INPUT_BUFFER_DEVICE_IOCTL)InputBuffer;
    
    //
    // Check input buffer - leasing application handle.
    //
    if (NULL == LeasingApplicationInputBuffer->LeasingApplicationHandle) {
    
        EventWriteInvalidReq9(
            NULL
            );
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        
        goto End;
    }

    ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);
    
    //
    // Look up the lease agent in the hash table that contains 
    // this leasing application. Skip the failed lease agents.
    //
    for(PLEASE_AGENT_CONTEXT LeaseAgentContext: DriverContext->LeaseAgentContextHashTable){

        KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

        //
        // Skip over already failed lease agents.
        //
        if (!IsLeaseAgentFailed(LeaseAgentContext)) {
            
            //
            // Take this application out of this lease agent.
            //
            Status = CleanLeasingApplication(
                LeaseAgentContext,
                PsGetCurrentProcess(),
                PsGetCurrentProcessId(),
                NULL,
                LeasingApplicationInputBuffer->LeasingApplicationHandle,
                LeasingApplicationInputBuffer->IsDelayed
                );

            if (STATUS_INVALID_PARAMETER != Status) {

                //
                // The request was not invalid.
                //
                Found = TRUE;
            }
        } 

        KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);

        if (Found) {

            break;
        }
    }
         
    ExReleaseFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    //
    // Set status based on success of the unregister operation.
    //
    if (!Found) {
        
        EventWriteInvalidReq9(
            NULL
            );    
        Status = STATUS_INVALID_PARAMETER;
    }

End:

    //
    // Complete request.
    //
    return Status;
}

NTSTATUS
CrashLeasingApplication(
__in WDFREQUEST Request,
__in size_t OutputBufferLength,
__in size_t InputBufferLength
)
{
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    EventWriteAssertCrashLeasingApplication(NULL);

    NTSTATUS Status = STATUS_SUCCESS;

    RemoveAllLeasingApplication();

    return Status;
}

NTSTATUS
EstablishLeaseRelationship(
    WDFREQUEST & Request,
    __in size_t OutputBufferLength,
    __in size_t InputBufferLength
    )

/*++

Routine Description:

    Creates all structures necessary for a new lease relationship. If that
    is successful, then a request lease message is sent.

Arguments:
        
    Request - Handle to a framework request object.

    OutputBufferLength - The size of the output buffer, in bytes.
    
    InputBufferLength - The size of the input buffer, in bytes.

Return Value:

    n/a

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID InputBuffer = NULL;
    PVOID OutputBuffer = NULL;
    PVOID IterLeaseAgent = NULL;
    ULONG BytesTransferred = 0;
    size_t Length = 0;
    BOOLEAN InsertedRemoteLeaseAgent = FALSE;
    BOOLEAN InsertedLeaseRelationship = FALSE;
    BOOLEAN Deleted = FALSE;
    BOOLEAN Found = FALSE;
    NTSTATUS ConnectionStatus = STATUS_SUCCESS;
    KIRQL OldIrql;
    
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContextIter = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContextActive = NULL;

    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContextExistent = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifierExistent = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifierPending = NULL;

    PESTABLISH_LEASE_INPUT_BUFFER_DEVICE_IOCTL EstablishLeaseInputBuffer = NULL;
    PESTABLISH_LEASE_OUTPUT_BUFFER_DEVICE_IOCTL EstablishLeaseOutputBuffer = NULL;

    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext();

    //
    // Retrieve the input buffer from the request.
    //
    if (sizeof(ESTABLISH_LEASE_INPUT_BUFFER_DEVICE_IOCTL) == InputBufferLength) {
        
        Status = WdfRequestRetrieveInputBuffer(
            Request, 
            sizeof(ESTABLISH_LEASE_INPUT_BUFFER_DEVICE_IOCTL),
            &InputBuffer,
            &Length
            );
        
        if (!NT_SUCCESS(Status)) {
        
            EventWriteRetrieveFail1(
                NULL
                );
            goto End;
        }
        
        LAssert(NULL != InputBuffer);
        LAssert(Length == InputBufferLength);
        
    } else {

        EventWriteInvalidReq1(
            NULL
            );        
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        
        goto End;
    }

    //
    // Retrieve the output buffer from the request.
    //
    if (sizeof(ESTABLISH_LEASE_OUTPUT_BUFFER_DEVICE_IOCTL) == OutputBufferLength) {
        
        Status = WdfRequestRetrieveOutputBuffer(
            Request, 
            sizeof(ESTABLISH_LEASE_OUTPUT_BUFFER_DEVICE_IOCTL),
            &OutputBuffer,
            &Length
            );
        
        if (!NT_SUCCESS(Status)) {
        
            EventWriteRetrieveFail2(
                NULL
                ); 
            goto End;
        }
        
        LAssert(NULL != OutputBuffer);
        LAssert(Length == OutputBufferLength);
        
    } else {

        EventWriteInvalidReq5(
            NULL
            );        
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        
        goto End;
    }        

    //
    // Cast input and output buffers to their types.
    // The input and output buffers are identical.
    // Take caution when setting data in the output 
    // buffer, it should be done only at the end after
    // the data from the input buffer has been processed.
    //
    EstablishLeaseInputBuffer = (PESTABLISH_LEASE_INPUT_BUFFER_DEVICE_IOCTL)InputBuffer;
    EstablishLeaseOutputBuffer = (PESTABLISH_LEASE_OUTPUT_BUFFER_DEVICE_IOCTL)OutputBuffer;

    //
    // Check arguments.
    //
    if (NULL == EstablishLeaseInputBuffer->LeasingApplicationHandle) {

        EventWriteInvalidReq9(
            NULL
            );
        Status = STATUS_INVALID_PARAMETER;

        goto End;
    }

    if (!IsValidListenEndpoint(&EstablishLeaseInputBuffer->RemoteSocketAddress)) {

        EventWriteInvalidReq6(
            NULL
            );
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        
        goto End;
    }

    //
    // Check input buffer - leasing application identifier.
    //
    if (!IsValidString(EstablishLeaseInputBuffer->RemoteLeasingApplicationIdentifier, MAX_PATH + 1)) {

        EventWriteInvalidReq11(
            NULL
            );
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        
        goto End;
    }

    //
    // Construct remote lease agent structure. Do not know the 
    // lease agent context at this point or if the remote lease
    // agent already exists.
    //
    Status = RemoteLeaseAgentConstructor(
        NULL,
        &EstablishLeaseInputBuffer->RemoteSocketAddress,
        &RemoteLeaseAgentContext
        );

    if (!NT_SUCCESS(Status)) {

        //
        // Request cannot be completed.
        //
        goto End;
    }

    //
    // Set the expected instance
    //
    RemoteLeaseAgentContext->RemoteLeaseAgentInstance.QuadPart = EstablishLeaseInputBuffer->RemoteLeaseAgentInstance;

    //
    // Check if the remote lease agent has not yet been created before.
    // If it has, reuse it, otherwise store the remote lease agent in
    // the lease agent hash table.
    //

    ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    //
    // Iterate over the lease agent in the hash table.
    //
    for(PLEASE_AGENT_CONTEXT LeaseAgentContext: DriverContext->LeaseAgentContextHashTable){
        if(Found) break;

        KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

        if (IsLeaseAgentFailed(LeaseAgentContext)) {

            //
            // Skip over the previously closed lease agents.
            //
            KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);

            continue;
        }

        //
        // Check to see if this lease agent contains the leasing application.
        //
        for(PLEASING_APPLICATION_CONTEXT LeasingApplicationContext: LeaseAgentContext->LeasingApplicationContextHashTable){
            if(Found) break;
            //
            // Retrieve leasing application context.
            //
            if (LeasingApplicationContext->Instance == EstablishLeaseInputBuffer->LeasingApplicationHandle) {

                //
                // Found the leasing application.
                //
                Found = TRUE;

                //
                // Construct new lease relationship identifier.
                //
                EstablishLeaseInputBuffer->RemoteLeasingApplicationIdentifier[MAX_PATH] = L'\0';
                LeaseRelationshipIdentifier = LeaseRelationshipIdentifierConstructor(
                    LeasingApplicationContext->LeasingApplicationIdentifier,
                    EstablishLeaseInputBuffer->RemoteLeasingApplicationIdentifier
                    );
                
                if (NULL == LeaseRelationshipIdentifier) {

                    //
                    // Deallocate remote lease agent.
                    //
                    RemoteLeaseAgentDestructor(RemoteLeaseAgentContext);

                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }

                if (!NT_SUCCESS(Status))
                {
                    // Terminate loop. The success status will be checked outside the loop.
                    break;
                }

                // Assign the index by checking the input establish duration
                LAssert(LEASE_DURATION == EstablishLeaseInputBuffer->LeaseDurationType ||
                    LEASE_DURATION_ACROSS_FAULT_DOMAIN == EstablishLeaseInputBuffer->LeaseDurationType);

                RemoteLeaseAgentContext->LeaseRelationshipContext->EstablishDurationType = EstablishLeaseInputBuffer->LeaseDurationType;

                BOOLEAN RemoteLeaseAgentFound = FALSE;
                //
                // Check to see if this lease agent contains already the remote lease agent.
                // If a remote lease agent of the same remote address exists and is still  
                // functional, then reuse it.
                //
                for(PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContextItem: LeaseAgentContext->RemoteLeaseAgentContextHashTable){

                    //
                    // Retrieve remote lease agent.
                    //
                    RemoteLeaseAgentContextIter = RemoteLeaseAgentContextItem;

                    if (0 != wcscmp(
                            RemoteLeaseAgentContextIter->RemoteLeaseAgentIdentifier, 
                            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier
                            ))
                    {
                        continue;
                    }

                    if (TRUE == RemoteLeaseAgentContextIter->IsActive)
                    {
                        RemoteLeaseAgentContextActive = RemoteLeaseAgentContextIter;
                    }

                    //
                    // Determine whether this is a newer instance
                    //
                    if (RemoteLeaseAgentContext->RemoteLeaseAgentInstance.QuadPart != 0 && RemoteLeaseAgentContextIter->RemoteLeaseAgentInstance.QuadPart != 0)
                    {
                        if (RemoteLeaseAgentContext->RemoteLeaseAgentInstance.QuadPart > RemoteLeaseAgentContextIter->RemoteLeaseAgentInstance.QuadPart) 
                        {
                            EventWriteRemoteLeaseAgentInstanceNewer(
                                NULL,
                                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                                RemoteLeaseAgentContext->Instance.QuadPart,
                                RemoteLeaseAgentContext->RemoteLeaseAgentInstance.QuadPart,
                                RemoteLeaseAgentContextIter->RemoteLeaseAgentIdentifier,
                                RemoteLeaseAgentContextIter->Instance.QuadPart,
                                RemoteLeaseAgentContextIter->RemoteLeaseAgentInstance.QuadPart);

                           RemoteLeaseAgentDisconnect(RemoteLeaseAgentContextIter, TRUE);
                           continue;
                        }
                    }

                    if (IsRemoteLeaseAgentFailed(RemoteLeaseAgentContextIter)) 
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

                        continue;
                    }
                    else if (IsRemoteLeaseAgentOrphaned(RemoteLeaseAgentContextIter))
                    {
                        RemoteLeaseAgentDisconnect(RemoteLeaseAgentContextIter, FALSE);

                        EventWriteDisconnectOrphanedRemoteLA(
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
                        // remote lease agent that isn't active. This remote lease agent is considered orphaned. When the federation
                        // asks to establish lease with this remote, it would destruct the orphaned remote lease agent and recreate one.
                        // Lease needs to copy over the ping send time from the orphaned remote lease agent to the new one. Lease needs
                        // this information for the arbitration.
                        //
                        RemoteLeaseAgentContext->PingSendTime = RemoteLeaseAgentContextIter->PingSendTime;

                        continue;
                    }

                    //
                    // If the remote lease agent has sent terminate request but did not get 
                    // the terminate ack back, then we cannot complete this call.
                    //
                    if (IsRemoteLeaseAgentSuspended(RemoteLeaseAgentContextIter)) {

                        EventWriteEstLeaseRelationFail(
                            NULL,
                            LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier,
                            LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
                            RemoteLeaseAgentContextIter->RemoteLeaseAgentIdentifier,
                            RemoteLeaseAgentContextIter->Instance.QuadPart,
                            GetLeaseAgentState(RemoteLeaseAgentContextIter->State)
                            ); 
                        //
                        // Deallocate remote lease agent.
                        //
                        RemoteLeaseAgentDestructor(RemoteLeaseAgentContext);

                        //
                        // Deallocate lease identifier.
                        //
                        LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifier);

                        //
                        // The remote lease agent cannot be used a new one cannot be created
                        // at this time. The caller needs to retry later.
                        //
                        Status = STATUS_RETRY;
                        RemoteLeaseAgentFound = TRUE;
                        break;
                    }

                    //
                    // Save the establish duration type from input buffer
                    //
                    RemoteLeaseAgentContextIter->LeaseRelationshipContext->EstablishDurationType = 
                        RemoteLeaseAgentContext->LeaseRelationshipContext->EstablishDurationType;

                    //
                    // Destroy the newly created remote lease agent.
                    //
                    RemoteLeaseAgentDestructor(RemoteLeaseAgentContext);

                    //
                    // Update the reference.
                    //
                    RemoteLeaseAgentContext = RemoteLeaseAgentContextIter;

                    //
                    // An existent remote lease agent was found.
                    //
                    RemoteLeaseAgentFound = TRUE;
                    break;
                }

                if (!NT_SUCCESS(Status)) {

                    //
                    // Terminate loop. The success status will be checked outside the loop.
                    //
                    break;
                }

                //
                // Check to see if we have found an existent remote lease agent. If not,
                // add the new one to the lease agent remote lease agent hash table.
                //
                if (!RemoteLeaseAgentFound) {

                    //
                    // Check connection status for the newly created remote lease agent.
                    // If the conneciton failed, there is nothing to add.
                    //
                    if (NT_SUCCESS(ConnectionStatus)) {
                        
                        //
                        // The remote lease agent was not found. Insert the new one in the hash table.
                        //
                        LeaseAgentContext->RemoteLeaseAgentContextHashTable.insert(RemoteLeaseAgentContext);
                        InsertedRemoteLeaseAgent = TRUE;
                        RemoteLeaseAgentContextExistent = RemoteLeaseAgentContext;
                        if (NULL == RemoteLeaseAgentContextExistent) {

                            EventWriteRegisterRemoteLAFail(
                                NULL,
                                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                                RemoteLeaseAgentContext->Instance.QuadPart,
                                TransportIdentifier(LeaseAgentContext->Transport),
                                LeaseAgentContext->Instance.QuadPart
                                ); 

                            //
                            // Deallocate remote lease agent. This will deallocate
                            // the lease relationship identifier also.
                            //
                            RemoteLeaseAgentDestructor(RemoteLeaseAgentContext);

                            //
                            // Deallocate lease relationship identifier.
                            //
                            LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifier);

                            //
                            // The hash table allocation has failed.
                            //
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            
                        } else {

                            //
                            // The remote lease agent cannot already exist at this point.
                            //
                            LAssert(InsertedRemoteLeaseAgent);
                            LAssert(RemoteLeaseAgentContextExistent == RemoteLeaseAgentContext);

                            EventWriteRegisterRemoteLA(
                                NULL,
                                RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                                RemoteLeaseAgentContext->Instance.QuadPart,
                                TransportIdentifier(LeaseAgentContext->Transport),
                                LeaseAgentContext->Instance.QuadPart
                                ); 

                            //
                            // Set the lease agent context for this new remote lease agent
                            // and its outgoing channel.
                            //
                            RemoteLeaseAgentContext->LeaseAgentContext = LeaseAgentContext;

                            //
                            // Resolve the SendTarget for this lease agent
                            //
                            TransportResolveTarget(LeaseAgentContext->Transport, 
                                                   &RemoteLeaseAgentContext->RemoteSocketAddress, 
                                                   RemoteLeaseAgentContext->PartnerTarget);

                            //
                            // The remote lease agent has been registered successfully.
                            //
                            Status = STATUS_SUCCESS;
                            //
                            // New RLA is inserted in the table and active
                            // Set the previous active one to inactive
                            //
                            if (NULL != RemoteLeaseAgentContextActive)
                            {
                                if (RemoteLeaseAgentContextActive->IsActive == TRUE) {
                                                                      LeaseTrace::WriteInfo(
                                                                                        "RemoteLeaseAgentDeactivate2",
                                                                                        "{0}, instance={1}, subject={2}, monitor={3}",
                                                                                        RemoteLeaseAgentContextActive->RemoteLeaseAgentIdentifier,
                                                                                        RemoteLeaseAgentContextActive->Instance.QuadPart,
                                                                                        RemoteLeaseAgentContextActive->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                                                                                        RemoteLeaseAgentContextActive->LeaseRelationshipContext->MonitorIdentifier.QuadPart);

                                                                                RemoteLeaseAgentContextActive->IsActive = FALSE;
                                                                    }
                                SetRemoteLeaseAgentFailed(RemoteLeaseAgentContextActive);
                            }
                        }
                        
                    } else {
                    
                        EventWriteRemoteLeaseAgentConnectionFail(
                            NULL,
                            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                            RemoteLeaseAgentContext->Instance.QuadPart,
                            ConnectionStatus
                            ); 

                        //
                        // Destroy the newly created remote lease agent.
                        //
                        RemoteLeaseAgentDestructor(RemoteLeaseAgentContext);

                        //
                        // Deallocate lease identifier.
                        //
                        LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifier);

                        //
                        // Fail with the connection error status.
                        //
                        Status = ConnectionStatus;
                    }
                }

                if (!NT_SUCCESS(Status)) {

                    //
                    // Terminate loop. The success status will be checked outside the loop.
                    //
                    break;
                }

                // If the lease relationship identifier is already in the subject terminate/failed pending failed list,
                // Previous lease is still being terminated, return STATUS_OBJECTID_EXISTS and let user mode retry

                if (RemoteLeaseAgentContext->SubjectFailedPendingHashTable.end() != RemoteLeaseAgentContext->SubjectFailedPendingHashTable.find(LeaseRelationshipIdentifier)
                        ||
                    RemoteLeaseAgentContext->SubjectTerminatePendingHashTable.end() != RemoteLeaseAgentContext->SubjectTerminatePendingHashTable.find(LeaseRelationshipIdentifier)
                    )
                {

                   EventWriteLeaseRelationExistRemoteLA(
                        NULL,
                        LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier,
                        LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
                        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                        RemoteLeaseAgentContext->Instance.QuadPart
                        ); 

                    //
                    // Destroy lease relationship identifier.
                    //
                    LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifier);

                    Status = STATUS_OBJECTID_EXISTS;

                    break;
                }


                // If the lease relationship identifier is already in the subject list,
                // Previous lease is  established, return existing lease handle
                auto LeaseRelationshipIdentifierInTable = RemoteLeaseAgentContext->SubjectHashTable.find(LeaseRelationshipIdentifier);

                if (RemoteLeaseAgentContext->SubjectHashTable.end() != LeaseRelationshipIdentifierInTable)
                {
                    //
                    // Populate the output buffer with existing lease.
                    //
                    EstablishLeaseOutputBuffer->LeaseHandle = *LeaseRelationshipIdentifierInTable;

                    // If the lease relationship identifier is already in the subject pending list,
                    // Previous lease is still being established, return existing lease handle in SubjectList
                    // OnLeaseEstablish callback will be called when the RESPONSE is received.


                    if (RemoteLeaseAgentContext->SubjectEstablishPendingHashTable.end() != RemoteLeaseAgentContext->SubjectEstablishPendingHashTable.find(LeaseRelationshipIdentifier))
                    {
                        EventWriteSubLeaseEstablishingOnRemote(
                            NULL,
                            LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier,
                            LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
                            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                            RemoteLeaseAgentContext->Instance.QuadPart,
                            RemoteLeaseAgentContext->RemoteLeaseAgentInstance.QuadPart
                            );

                        EstablishLeaseOutputBuffer->IsEstablished = FALSE;
                        LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifier);

                        break;

                    }

                    EventWriteSubLeaseEstablishedOnRemote(
                        NULL,
                        LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier,
                        LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
                        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                        RemoteLeaseAgentContext->Instance.QuadPart,
                        RemoteLeaseAgentContext->RemoteLeaseAgentInstance.QuadPart
                        );

                    EstablishLeaseOutputBuffer->IsEstablished = TRUE;
                    LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifier);

                    break;

                }

                //
                // Add the lease relationship identifier to the subject list and 
                // subject pending list of the found or newly created remote agent.
                //
                Invariant(LeaseRelationshipIdentifier);

                RemoteLeaseAgentContext->SubjectHashTable.insert(LeaseRelationshipIdentifier);
                InsertedLeaseRelationship = TRUE;
                LeaseRelationshipIdentifierExistent= LeaseRelationshipIdentifier;
                //
                // Check insert result.
                //
                if (NULL == LeaseRelationshipIdentifierExistent)
                {
                    LAssert(!InsertedLeaseRelationship);

                    EventWriteRegisterLRIDSubListFail(
                        NULL,
                        LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier,
                        LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
                        RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                        RemoteLeaseAgentContext->Instance.QuadPart
                        );                
                    //
                    // Destroy lease relationship identifier.
                    //
                    LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifier);
                
                    //
                    // Hash table memory allocation failed.
                    //
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
                else
                {
                    //
                    // New lease relationship identifier was added.
                    //
                    LAssert(InsertedLeaseRelationship);

                    //
                    // Add the lease identifier to the subject pending list of the remote lease agent.
                    //
                    //TODO shuxu
                    LeaseRelationshipIdentifierPending = AddToLeaseRelationshipIdentifierList(
                        LeasingApplicationContext,
                        LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
                        RemoteLeaseAgentContext->SubjectEstablishPendingHashTable
                        );
                    
                    if (NULL == LeaseRelationshipIdentifierPending)
                    {
                        EventWriteRegisterLRIDSubPendingListFail(
                            NULL,
                            LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier,
                            LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
                            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                            RemoteLeaseAgentContext->Instance.QuadPart
                            );
                        //
                        // Remove lease identifier from subject list.
                        //
                        RemoteLeaseAgentContext->SubjectHashTable.erase(RemoteLeaseAgentContext->SubjectHashTable.find(LeaseRelationshipIdentifier));

                        //
                        // Deallocate lease relationship identifier.
                        //
                        LeaseRelationshipIdentifierDestructor(LeaseRelationshipIdentifier);
                
                        //
                        // Hash table memory allocation failed.
                        //
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        
                    }
                    else
                    {
                        EventWriteLeaseRelationRegisteredRemoteLA(
                            NULL,
                            LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier,
                            LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
                            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                            RemoteLeaseAgentContext->Instance.QuadPart
                            ); 
                    }
                }

                if (!NT_SUCCESS(Status))
                {
                    //
                    // Check to see if we need to deallocate the remote lease agent.
                    // We cannot leave the remote lease agent in this state since
                    // we will not be able to later unload it.
                    //
                    if (InsertedRemoteLeaseAgent)
                    {
                        //
                        // Remove the remote lease agent from the hash table.
                        //
                        LeaseAgentContext->RemoteLeaseAgentContextHashTable.erase(LeaseAgentContext->RemoteLeaseAgentContextHashTable.find(RemoteLeaseAgentContext));

                        //
                        // Deallocate remote lease agent.
                        //
                        RemoteLeaseAgentDestructor(RemoteLeaseAgentContext);
                        //
                        // the previous active was set to inactive
                        // reset it active
                        //
                        if (NULL != RemoteLeaseAgentContextActive)
                        {
                            RemoteLeaseAgentContextActive->IsActive = TRUE;
                        }
                    }
                    //
                    // Terminate loop. The success status will be checked outside the loop.
                    //
                    break;
                }

                EventWriteEstSubLeaseRelationOnRemote(
                    NULL,
                    LeaseRelationshipIdentifier->LeasingApplicationContext->LeasingApplicationIdentifier,
                    LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
                    GetConfigLeaseDuration(RemoteLeaseAgentContext),
                    RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                    RemoteLeaseAgentContext->Instance.QuadPart,
                    RemoteLeaseAgentContext->RemoteLeaseAgentInstance.QuadPart
                    );
                //
                // Establish new lease relationship.
                //

                if ( LEASE_STATE_INACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState && 
                    LEASE_STATE_INACTIVE == RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState)
                {
                    //
                    // this is the first lease establish request, send the ping first
                    //
                    EstablishLeaseSendPingRequestMessage( RemoteLeaseAgentContext );
                }
                else
                {
                    EstablishLease( RemoteLeaseAgentContext );
                }

                //
                // Populate the output buffer with new lease.
                //
                EstablishLeaseOutputBuffer->LeaseHandle = LeaseRelationshipIdentifier;
                EstablishLeaseOutputBuffer->IsEstablished = FALSE;
                break;
            }
        }
             
        KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);
    }

    ExReleaseFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    //
    // Check to see if the lease agent was found.
    //
    if (!Found) {

        //
        // Deallocate remote lease agent.
        //
        RemoteLeaseAgentDestructor(RemoteLeaseAgentContext);

        EventWriteInvalidReq9(
            NULL
            );
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
    }

End:
    
    //
    // Complete the request.
    //

    if (NT_SUCCESS(Status))
    {
        //
        // Set the number of bytes transferred.
        //
        BytesTransferred = sizeof(ESTABLISH_LEASE_OUTPUT_BUFFER_DEVICE_IOCTL);
    }

    Request->outputLength = BytesTransferred;
    return Status;
}

NTSTATUS
TerminateLeaseRelationship(
    WDFREQUEST & Request,
    __in size_t OutputBufferLength,
    __in size_t InputBufferLength
    )

/*++

Routine Description:

    Terminates an existent lease relationship. 

Arguments:
        
    Request - Handle to a framework request object.

    OutputBufferLength - The size of the output buffer, in bytes.
    
    InputBufferLength - The size of the input buffer, in bytes.

Return Value:

    n/a

--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PVOID InputBuffer = NULL;
    size_t Length = 0;
    BOOLEAN FoundLeaseRelationshipIdentifier = FALSE;
    KIRQL OldIrql;
    
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier = NULL;

    PTERMINATE_LEASE_BUFFER_DEVICE_IOCTL LeaseInputBuffer = NULL;
    
    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext();

    //
    // Check request output size.
    //
    if (0 != OutputBufferLength) {
    
         EventWriteInvalidReq5(
            NULL
            );

        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        
        goto End;
    }
    
    //
    // Retrieve the input buffer from the request.
    //
    if (sizeof(TERMINATE_LEASE_BUFFER_DEVICE_IOCTL) == InputBufferLength) {
        
        Status = WdfRequestRetrieveInputBuffer(
            Request, 
            sizeof(TERMINATE_LEASE_BUFFER_DEVICE_IOCTL),
            &InputBuffer,
            &Length
            );
        
        if (!NT_SUCCESS(Status)) {
        
            EventWriteRetrieveFail1(
                NULL
                );    
            goto End;
        }
    
        LAssert(NULL != InputBuffer);
        LAssert(Length == InputBufferLength);
        
    } else {
    
         EventWriteInvalidReq1(
            NULL
            );       
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        
        goto End;
    }
    
    //
    // Cast input buffer to its type.
    //
    LeaseInputBuffer = (PTERMINATE_LEASE_BUFFER_DEVICE_IOCTL)InputBuffer;
    
    //
    // Check arguments.
    //
    if (NULL == LeaseInputBuffer->LeasingApplicationHandle) {

        EventWriteInvalidReq9(NULL);

        Status = STATUS_INVALID_PARAMETER;

        goto End;
    }

    if (NULL == LeaseInputBuffer->LeaseHandle) {
    
        Status = STATUS_INVALID_PARAMETER;

        goto End;
    }
    
    //
    // Find the remote lease agent for the given lease handle.
    //
    
    ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);
    
    //
    // Look up the lease agent in the hash table that contains 
    // this leasing application.
    //
    for(PLEASE_AGENT_CONTEXT LeaseAgentContext: DriverContext->LeaseAgentContextHashTable){

        if (FoundLeaseRelationshipIdentifier) break;

        KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

        if (IsLeaseAgentFailed(LeaseAgentContext)) {

            //
            // Skip over the previously closed lease agents.
            //
            KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);

            continue;
        }

        //
        // Check to see if this lease agent contains the leasing application.
        //
        for (PLEASING_APPLICATION_CONTEXT LeasingApplicationContext : LeaseAgentContext->LeasingApplicationContextHashTable)
        {
            if (FoundLeaseRelationshipIdentifier) break;

            if (LeasingApplicationContext->Instance == LeaseInputBuffer->LeasingApplicationHandle)
            {
                //
                // Find the remote lease agent that will initiate the lease termination.
                //
                for(PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext: LeaseAgentContext->RemoteLeaseAgentContextHashTable){
                    if (FoundLeaseRelationshipIdentifier) break;

                    for(PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier: RemoteLeaseAgentContext->SubjectHashTable){

                        //
                        // Match the lease identifier with the provided input handle.
                        //
                        LeaseInputBuffer->RemoteLeasingApplicationIdentifier[MAX_PATH] = L'\0';
                        if (LeaseRelationshipIdentifier == LeaseInputBuffer->LeaseHandle &&
                            0 == wcscmp(LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
                                        LeaseInputBuffer->RemoteLeasingApplicationIdentifier)
                        )
                        {
                            //
                            // Found lease handle.
                            //
                            FoundLeaseRelationshipIdentifier = TRUE;

                            if (IsRemoteLeaseAgentFailed(RemoteLeaseAgentContext) || 
                                IsRemoteLeaseAgentSuspended(RemoteLeaseAgentContext)) {

                                EventWriteTerminateLeaseRelationSkipped(
                                    NULL,
                                    RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                                    RemoteLeaseAgentContext->Instance.QuadPart,
                                    GetLeaseAgentState(RemoteLeaseAgentContext->State)
                                    ); 

                                Status = STATUS_INVALID_PARAMETER;

                                //
                                // This remote lease agent is failed or suspended already.
                                //
                                break;
                            }

                            //
                            // Initiate terminate lease relationship.
                            //
                            Status = TerminateSubjectLease(
                                RemoteLeaseAgentContext,
                                LeaseRelationshipIdentifier,
                                FALSE
                                );

                            if (!NT_SUCCESS(Status))
                            {
                                break;
                            }

                            //
                            // TerminateSubjectLease add the identifier to the terminate pending list
                            // The following function send the message out
                            //
                            TerminateSubjectLeaseSendMessage(RemoteLeaseAgentContext, FALSE);

                            //
                            // Done.
                            //
                            break;
                        }
                    }
                }
            }
        }
                
        KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);    
    }

    ExReleaseFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    if (!FoundLeaseRelationshipIdentifier) {
        EventWriteTerminateLeaseRemoteLeaseAgentNotFound(NULL,
            (LONGLONG)LeaseInputBuffer->LeasingApplicationHandle,
            (LONGLONG)LeaseInputBuffer->LeaseHandle,
            LeaseInputBuffer->RemoteLeasingApplicationIdentifier
            );
        
        //
        // Request is invalid.
        //
        Status = STATUS_INVALID_PARAMETER;
    }

End:

    //
    // Complete request.
    //
    return Status;
}

NTSTATUS
GetLeasingApplicationExpirationTime(
    WDFREQUEST & Request,
    __in size_t OutputBufferLength,
    __in size_t InputBufferLength
    )

/*++

Routine Description:

    Returns the leasing application time to live. Its value is the smallest
    of subject suspend times of all remote lease agents on this lease agent.

Arguments:
        
    Request - Handle to a framework request object.

    OutputBufferLength - The size of the output buffer, in bytes.
    
    InputBufferLength - The size of the input buffer, in bytes.

Return Value:

    n/a

--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PVOID InputBuffer = NULL;
    PVOID OutputBuffer = NULL;
    PVOID IterLeaseAgent = NULL;
    ULONG BytesTransferred = 0;
    size_t Length = 0;
    LARGE_INTEGER Now;
    BOOLEAN Found = FALSE;
    KIRQL OldIrql;

    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    PGET_LEASING_APPLICATION_EXPIRATION_INPUT_BUFFER GetLeasingApplicationExpirationTimeInputBuffer = NULL;
    PGET_LEASING_APPLICATION_EXPIRATION_OUTPUT_BUFFER GetLeasingApplicationExpirationTimeOutputBuffer = NULL;

    LONG InputRequestTTL = 0;

    //
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext();

    //
    // Retrieve the input buffer from the request.
    //
    if (sizeof(GET_LEASING_APPLICATION_EXPIRATION_INPUT_BUFFER) == InputBufferLength) {
        
        Status = WdfRequestRetrieveInputBuffer(
            Request, 
            sizeof(GET_LEASING_APPLICATION_EXPIRATION_INPUT_BUFFER),
            &InputBuffer,
            &Length
            );
        
        if (!NT_SUCCESS(Status)) {
        
            EventWriteRetrieveFail1(
                NULL
                );
            goto End;
        }
        
        LAssert(NULL != InputBuffer);
        LAssert(Length == InputBufferLength);
        
    } else {

        EventWriteInvalidReq1(
            NULL
            );        
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        
        goto End;
    }

    //
    // Retrieve the output buffer from the request.
    //
    if (sizeof(GET_LEASING_APPLICATION_EXPIRATION_OUTPUT_BUFFER) == OutputBufferLength) {
        
        Status = WdfRequestRetrieveOutputBuffer(
            Request, 
            sizeof(GET_LEASING_APPLICATION_EXPIRATION_OUTPUT_BUFFER),
            &OutputBuffer,
            &Length
            );
        
        if (!NT_SUCCESS(Status)) {
        
            EventWriteRetrieveFail2(
                NULL
                ); 
            goto End;
        }
        
        LAssert(NULL != OutputBuffer);
        LAssert(Length == OutputBufferLength);
        
    } else {

        EventWriteInvalidReq5(
            NULL
            );        
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        
        goto End;
    }

    //
    // Cast input and output buffers to their types.
    //
    GetLeasingApplicationExpirationTimeInputBuffer = (PGET_LEASING_APPLICATION_EXPIRATION_INPUT_BUFFER)InputBuffer;
    GetLeasingApplicationExpirationTimeOutputBuffer = (PGET_LEASING_APPLICATION_EXPIRATION_OUTPUT_BUFFER)OutputBuffer;

    InputRequestTTL = GetLeasingApplicationExpirationTimeInputBuffer->RequestTimeToLive;

    //
    // Check input buffer - lease agent handle.
    //
    if (NULL == GetLeasingApplicationExpirationTimeInputBuffer->LeasingApplicationHandle) {

        EventWriteInvalidReq9(
            NULL
            );
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        
        goto End;
    }

    ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    //
    // Look up the lease agent in the hash table.
    //
    for(PLEASE_AGENT_CONTEXT LeaseAgentContext: DriverContext->LeaseAgentContextHashTable){

        KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

        //
        // Check to see if this lease agent contains the leasing application.
        //
        for(PLEASING_APPLICATION_CONTEXT LeasingApplicationContextItem: LeaseAgentContext->LeasingApplicationContextHashTable){
            //
            // Retrieve leasing application context.
            //
            LeasingApplicationContext = LeasingApplicationContextItem;
            if (LeasingApplicationContext->Instance == GetLeasingApplicationExpirationTimeInputBuffer->LeasingApplicationHandle &&
                FALSE == LeasingApplicationContext->IsBeingUnregistered) {

                //
                // Leasing application found.
                //
                Found = TRUE;

                break;
            }
        }

        if (Found) {

            //
            // Populate with no time to live.
            //
            GetLeasingApplicationExpirationTimeOutputBuffer->TimeToLive = 0;
            GetLeasingApplicationExpirationTimeOutputBuffer->KernelSystemTime = 0;
            BytesTransferred = sizeof(GET_LEASING_APPLICATION_EXPIRATION_OUTPUT_BUFFER);

            GetLeasingApplicationTTL(
                LeasingApplicationContext,
                InputRequestTTL,
                &GetLeasingApplicationExpirationTimeOutputBuffer->TimeToLive
                );

            GetCurrentTime(&Now);
            //
            // Set the output buffer kernel time in milliseconds
            // 
            GetLeasingApplicationExpirationTimeOutputBuffer->KernelSystemTime = 
                Now.QuadPart / MILLISECOND_TO_NANOSECOND_FACTOR;

            EventWriteGetLeasingAPPTTL(
                NULL,
                LeasingApplicationContext->LeasingApplicationIdentifier,
                GetLeasingApplicationExpirationTimeOutputBuffer->TimeToLive,
                FALSE,
                GetLeasingApplicationExpirationTimeOutputBuffer->KernelSystemTime,
                InputRequestTTL
                );

        }

        KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);

        if(Found){
            break;
        }
    }

    ExReleaseFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    //
    // Check to see if the leasing application has been added.
    //
    if (!Found) {

        EventWriteInvalidReq9(
            NULL
            );
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
    }

End:

    //
    // Complete the request.
    //
    Request->outputLength = BytesTransferred;
    return Status;
}

NTSTATUS
SetGlobalLeaseExpirationTime(
    WDFREQUEST & Request,
    __in size_t OutputBufferLength,
    __in size_t InputBufferLength
    )

/*++

Routine Description:

    Set the global lease expiration time based on the leasing application handle.

Arguments:
        
    Request - Handle to a framework request object.

    OutputBufferLength - The size of the output buffer, in bytes.
    
    InputBufferLength - The size of the input buffer, in bytes.

Return Value:

    n/a

--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PVOID InputBuffer = NULL;
    PVOID IterLeaseAgent = NULL;
    PVOID IterLeasingApplication = NULL;
    size_t Length = 0;
    BOOLEAN Found = FALSE;
    KIRQL OldIrql;

    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    PGLOBAL_LEASE_EXPIRATION_INPUT_BUFFER SetLeaseExpirationTimeInputBuffer = NULL;

    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext();

    //
    // Retrieve the input buffer from the request.
    //
    if (sizeof(GLOBAL_LEASE_EXPIRATION_INPUT_BUFFER) == InputBufferLength) {
        
        Status = WdfRequestRetrieveInputBuffer(
            Request, 
            sizeof(GLOBAL_LEASE_EXPIRATION_INPUT_BUFFER),
            &InputBuffer,
            &Length
            );
        
        if (!NT_SUCCESS(Status)) {
        
            EventWriteRetrieveFail1(
                NULL
                );
            goto End;
        }
        
        LAssert(NULL != InputBuffer);
        LAssert(Length == InputBufferLength);
        
    } else {

        EventWriteInvalidReq1(
            NULL
            );        
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        
        goto End;
    }

    //
    // Check request output size.
    //
    if (0 != OutputBufferLength) {
    
         EventWriteInvalidReq5(
            NULL
            );

        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        
        goto End;
    }

    //
    // Cast input buffers to their types.
    //
    SetLeaseExpirationTimeInputBuffer = (PGLOBAL_LEASE_EXPIRATION_INPUT_BUFFER)InputBuffer;

    //
    // Check input buffer - lease agent handle.
    //
    if (NULL == SetLeaseExpirationTimeInputBuffer->LeasingApplicationHandle) {

        EventWriteInvalidReq6(
            NULL
            );
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        
        goto End;
    }

    ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    //
    // Look up the lease agent in the hash table.
    //
    for(PLEASE_AGENT_CONTEXT LeaseAgentContext: DriverContext->LeaseAgentContextHashTable){
        if(Found) break;
        KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

        //
        // Check to see if this lease agent contains the leasing application.
        //
        for(PLEASING_APPLICATION_CONTEXT LeasingApplicationContextItem: LeaseAgentContext->LeasingApplicationContextHashTable){
            if(Found) break;
            //
            // Retrieve leasing application context.
            //
            LeasingApplicationContext = LeasingApplicationContextItem;
            if (LeasingApplicationContext->Instance == SetLeaseExpirationTimeInputBuffer->LeasingApplicationHandle &&
                PsGetCurrentProcess() == LeasingApplicationContext->ProcessHandle) {

                //
                // Leasing application found.
                //
                Found = TRUE;

                break;
            }
        }

        if (Found)
        {
            LeasingApplicationContext->GlobalLeaseExpireTime.QuadPart = 
                SetLeaseExpirationTimeInputBuffer->LeaseExpireTime * MILLISECOND_TO_NANOSECOND_FACTOR;
        }

        KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);
    }
            
    ExReleaseFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    //
    // Check to see if the leasing application has been added.
    //
    if (!Found) {

        EventWriteInvalidReq9(
            NULL
            );
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
    }

End:

    //
    // Complete the request.
    //
    return Status;
}

NTSTATUS
ProcessArbitrationResult(
    WDFREQUEST & Request,
    __in size_t OutputBufferLength,
    __in size_t InputBufferLength
    )

/*++

Routine Description:

    Processes an arbitration result for a given remote lease agent.

Arguments:
        
    Request - Handle to a framework request object.

    OutputBufferLength - The size of the output buffer, in bytes.
    
    InputBufferLength - The size of the input buffer, in bytes.

Return Value:

    n/a

--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PVOID InputBuffer = NULL;
    PVOID IterLeaseAgent = NULL;
    PVOID IterRemoteLeaseAgent = NULL;
    PVOID IterLeasingApplication = NULL;
    size_t Length = 0;
    BOOLEAN Found = FALSE;
    KIRQL OldIrql;

    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContextIter = NULL;
    PARBITRATION_RESULT_INPUT_BUFFER ArbitrationResultInputBuffer = NULL;

    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext();

    //
    // Check request output size.
    //
    if (0 != OutputBufferLength) {

         EventWriteInvalidReq5(
            NULL
            );       
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;

        goto End;
    }

    //
    // Retrieve the input buffer from the request.
    //
    if (sizeof(ARBITRATION_RESULT_INPUT_BUFFER) == InputBufferLength) {
        
        Status = WdfRequestRetrieveInputBuffer(
            Request, 
            sizeof(ARBITRATION_RESULT_INPUT_BUFFER),
            &InputBuffer,
            &Length
            );
        
        if (!NT_SUCCESS(Status)) {
        
            EventWriteRetrieveFail1(
                NULL
                );
            //
            // This is an invalid request.
            //
            goto End;
        }

        LAssert(NULL != InputBuffer);
        LAssert(Length == InputBufferLength);
        
    } else {

        EventWriteInvalidReq1(
            NULL
            );       
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;

        goto End;
    }

    //
    // Cast input buffer to its type.
    //
    ArbitrationResultInputBuffer = (PARBITRATION_RESULT_INPUT_BUFFER)InputBuffer;

    //
    // Check input buffer - time to live for both local and remote
    //
    if (0 > ArbitrationResultInputBuffer->RemoteTimeToLive || 0 > ArbitrationResultInputBuffer->LocalTimeToLive) {

        EventWriteInvalidReq12(
            NULL
            );
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;

        goto End;
    }

    //
    // Check input buffer - leasing application handle.
    //
    if (NULL == ArbitrationResultInputBuffer->LeasingApplicationHandle) {
    
        EventWriteInvalidReq9(
            NULL
            );    
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        
        goto End;
    }


    //
    // Check input buffer - lease agent instance.
    //
    if (ArbitrationResultInputBuffer->LeaseAgentInstance < 0) {
    
        EventWriteInvalidReq3(
            NULL
            );    
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        
        goto End;
    }

    //
    // Check input buffer - remote lease agent instance.
    //
    if (ArbitrationResultInputBuffer->RemoteLeaseAgentInstance < 0) {
    
        EventWriteInvalidReq4(
            NULL
            );    
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        
        goto End;
    }

    //
    // Check input buffer - remote socket address.
    //
    if (!IsValidListenEndpoint(&ArbitrationResultInputBuffer->RemoteSocketAddress)) {
    
        EventWriteInvalidReq6(
            NULL
            );    
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        
        goto End;
    }

    //
    // Create remote lease agent for comparison.
    //
    Status = RemoteLeaseAgentConstructor(
        NULL,
        &ArbitrationResultInputBuffer->RemoteSocketAddress,
        &RemoteLeaseAgentContext
        );

    if (!NT_SUCCESS(Status)) {
    
        //
        // Request cannot be completed.
        //
        goto End;
    }

    ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    //
    // Look up the lease agent in the hash table.
    //
    for(PLEASE_AGENT_CONTEXT LeaseAgentContext: DriverContext->LeaseAgentContextHashTable){

        if(Found) break;

        KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

        if (IsLeaseAgentFailed(LeaseAgentContext) || 
            LeaseAgentContext->Instance.QuadPart != ArbitrationResultInputBuffer->LeaseAgentInstance) {

            //
            // Skip over the previously closed lease agents or lease agents with different instance.
            //            
            KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);

            continue;
        }

        //
        // Check to see if this lease agent contains the leasing application.
        //
        for(PLEASING_APPLICATION_CONTEXT LeasingApplicationContextItem: LeaseAgentContext->LeasingApplicationContextHashTable){
            if(Found) break;
            //
            // Retrieve leasing application context.
            //
            LeasingApplicationContext = LeasingApplicationContextItem;
        
            if (LeasingApplicationContext->Instance == ArbitrationResultInputBuffer->LeasingApplicationHandle) {
                IterLeasingApplication = LeasingApplicationContextItem;
                //
                // Found the leasing application.
                //
                break;
            }
        }

        if (NULL == IterLeasingApplication) {

            //
            // Move on to the next lease agent. The leasing application
            // is not on this lease agent.
            //
            KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);

            continue;
        }
             
        //
        // Find the specified remote lease agent.
        //
        for(PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContextItem: LeaseAgentContext->RemoteLeaseAgentContextHashTable){
        
            //
            // Retrieve remote lease agent.
            //
            RemoteLeaseAgentContextIter = RemoteLeaseAgentContextItem;
        
            //
            // Compare the remote lease agent instance and check remote lease agent state.
            //
            if (RemoteLeaseAgentContextIter->RemoteLeaseAgentInstance.QuadPart == ArbitrationResultInputBuffer->RemoteLeaseAgentInstance &&
                0 == wcscmp(RemoteLeaseAgentContextIter->RemoteLeaseAgentIdentifier, RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier)) {

                if (IsRemoteLeaseAgentFailed(RemoteLeaseAgentContextIter)) {
        
                    //
                    // This remote lease agent is not usable anymore.
                    //
                    continue;
                }

                LARGE_INTEGER Now;
                GetCurrentTime(&Now);

                //
                // Check to see if arbitration result was delayed. The arbitration result callback could be blocked. We double check the time here.
                // In case the arbitration result was delayed, we fail the lease.
                //
                if (IS_LARGE_INTEGER_GREATER_THAN_OR_EQUAL_LARGE_INTEGER(Now, RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectFailTime))
                {
                    EventWriteArbitrationResultReceiveTimeout(NULL,
                        RemoteLeaseAgentContextIter->RemoteLeaseAgentIdentifier,
                        RemoteLeaseAgentContextIter->Instance.QuadPart,
                        RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                        RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                        GetLeaseState(RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectState),
                        GetLeaseState(RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorState)
                        );

                    //
                    // Fail lease since the arbitration result timeout
                    //
                    RemoteLeaseAgentContextIter->LeaseRelationshipContext->SubjectState = LEASE_STATE_FAILED;
                    RemoteLeaseAgentContextIter->LeaseRelationshipContext->MonitorState = LEASE_STATE_INACTIVE;

                    OnLeaseFailure(RemoteLeaseAgentContextIter->LeaseAgentContext);
                }
                else
                {
                
                    //
                    // Process arbitration result.
                    //
                    ArbitrateLease(
                        LeaseAgentContext,
                        RemoteLeaseAgentContextIter,
                        ArbitrationResultInputBuffer->LocalTimeToLive,
                        ArbitrationResultInputBuffer->RemoteTimeToLive,
                        ArbitrationResultInputBuffer->IsDelayed
                        );
                }
                
                //
                // Found the remote lease agent.
                //
                Found = TRUE;
                
                //
                // Done.
                //
                break;
            }
        }

        KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);
    }
            
    ExReleaseFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    //
    // Deallocate the remote lease agent.
    //
    RemoteLeaseAgentDestructor(RemoteLeaseAgentContext);
    
    //
    // Set status based on success of the deletion operation.
    //
    if (!Found) {
        
        EventWriteInvalidReq11(
            NULL
            );
        Status = STATUS_INVALID_PARAMETER;
    }

End:

    //
    // Complete request.
    //
    return Status;
}

NTSTATUS
UpdateSecuritySettings(
    WDFREQUEST & Request,
    __in size_t OutputBufferLength,
    __in size_t InputBufferLength
    )

/*++

Routine Description:

    Update the certificate based on the leasing application handle.

Arguments:
        
    Request - Handle to a framework request object.

    OutputBufferLength - The size of the output buffer, in bytes.
    
    InputBufferLength - The size of the input buffer, in bytes.

Return Value:

    n/a

--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PVOID InputBuffer = NULL;
    PVOID IterLeaseAgent = NULL;
    PVOID IterLeasingApplication = NULL;
    size_t Length = 0;
    BOOLEAN Found = FALSE;
    KIRQL OldIrql;
    ULONG certStoreNameLength = 0;

    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    PUPDATE_SECURITY_SETTINGS_INPUT_BUFFER securitySettingsInputBuffer = NULL;

    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext();

    //
    // Retrieve the input buffer from the request.
    //
    if (sizeof(UPDATE_SECURITY_SETTINGS_INPUT_BUFFER) == InputBufferLength) {
        
        Status = WdfRequestRetrieveInputBuffer(
            Request, 
            sizeof(UPDATE_SECURITY_SETTINGS_INPUT_BUFFER),
            &InputBuffer,
            &Length
            );
        
        if (!NT_SUCCESS(Status))
        {
            EventWriteRetrieveFail1(
                NULL
                );
            goto End;
        }
        
        LAssert(NULL != InputBuffer);
        LAssert(Length == InputBufferLength);
        
    }
    else
    {
        EventWriteInvalidReq1(NULL);
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        goto End;
    }

    //
    // Check request output size.
    //
    if (0 != OutputBufferLength)
    {
         EventWriteInvalidReq5(NULL);
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        goto End;
    }

    //
    // Cast input buffers to their types.
    //
    securitySettingsInputBuffer = (PUPDATE_SECURITY_SETTINGS_INPUT_BUFFER)InputBuffer;

    //
    // Check input buffer - lease agent handle.
    //
    if (NULL == securitySettingsInputBuffer->LeasingApplicationHandle)
    {
        EventWriteInvalidReq9(
            NULL
            );
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        goto End;
    }

    ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    //
    // Look up the lease agent in the hash table.
    //
    for(PLEASE_AGENT_CONTEXT LeaseAgentContext: DriverContext->LeaseAgentContextHashTable){

        KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

        //
        // Check to see if this lease agent contains the leasing application.
        //
        for(PLEASING_APPLICATION_CONTEXT LeasingApplicationContextItem: LeaseAgentContext->LeasingApplicationContextHashTable){
            //
            // Retrieve leasing application context.
            //
            LeasingApplicationContext = LeasingApplicationContextItem;
        
            if (LeasingApplicationContext->Instance == securitySettingsInputBuffer->LeasingApplicationHandle &&
                PsGetCurrentProcess() == LeasingApplicationContext->ProcessHandle)
            {
                // Leasing application found.
                Found = TRUE;
                break;
            }
        }

        if (Found)
        {
            auto err = LeaseAgentContext->Transport->SetSecurity(*(securitySettingsInputBuffer->SecuritySettings));
            if (!err.IsSuccess())
            {
                Status = STATUS_UNSUCCESSFUL;
                EventWriteUpdateSecuritySettingsFailed(
                    NULL,
                    LeasingApplicationContext->LeasingApplicationIdentifier,
                    TransportIdentifier(LeaseAgentContext->Transport),
                    LeaseAgentContext->Instance.QuadPart);

                break;
            }
        }

        KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);
    }

    ExReleaseFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    //
    // Check to see if the leasing application has been added.
    //
    if (!Found) {

        EventWriteInvalidReq9(
            NULL
            );
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
    }

End:

    //
    // Complete the request.
    //
    return Status;
}

void RemoveAllLeasingApplication()
{
    PVOID IterLeaseAgent = NULL;
    KIRQL OldIrql;

    PLEASE_AGENT_CONTEXT LeaseAgentContext = NULL;
    PEPROCESS UserModeCallingProcessHandle = NULL;
    HANDLE UserModeCallingProcessId = NULL;

    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext();

    //
    // Retrieve the calling process handle.
    //
    UserModeCallingProcessHandle = PsGetCurrentProcess();
    UserModeCallingProcessId = PsGetCurrentProcessId();

    EventWriteProcessCleanup(
        NULL,
        (UINT64)UserModeCallingProcessId);

    ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    //
    // Look up the lease agent in the hash table that contains 
    // this leasing application.
    //
    for (PLEASE_AGENT_CONTEXT LeaseAgentContextIter: DriverContext->LeaseAgentContextHashTable) {

        //
        // Retrieve lease agent context.
        //
        LeaseAgentContext = LeaseAgentContextIter;

        KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

        //
        // Skip over already failed lease agents.
        //
        if (!IsLeaseAgentFailed(LeaseAgentContext)) {

            //
            // Take this application out of this lease agent.
            //
            CleanLeasingApplication(
                LeaseAgentContext,
                UserModeCallingProcessHandle,
                UserModeCallingProcessId,
                NULL,
                NULL,
                TRUE
            );
        }
        KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);

    }

    ExReleaseFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);
}

NTSTATUS
LeaseLayerEvtIoDeviceControl (
    WDFREQUEST & Request,
    __in size_t OutputBufferLength,
    __in size_t InputBufferLength,
    __in ULONG IoControlCode
    )
    
/*++

Routine Description:

    Performs device I/O to the Lease Layer device. 

Arguments:
        
    Request - Handle to a framework request object.

    OutputBufferLength - The size of the output buffer, in bytes.
    
    InputBufferLength - The size of the input buffer, in bytes.

    IoControlCode - The control code for the operation. This value identifies 
        the specific operation to be performed and the type of device on which 
        to perform it. 

Return Value:

    n/a

--*/

{
    NTSTATUS status;

    EventWriteApiStart(
        NULL,
        GetLeaseIoCode(IoControlCode)
        );
    
    //
    // Process the incoming IOCTL code.
    //
    switch (IoControlCode) {

    //
    // Create or reuse a new lease agent.
    //
    case IOCTL_LEASING_CREATE_LEASE_AGENT:

        status = CreateLeaseAgent(
            Request,
            OutputBufferLength,
            InputBufferLength
            );
        break;

    //
    // Close an existent lease agent.
    //
    case IOCTL_LEASING_CLOSE_LEASE_AGENT:

        status = CloseLeaseAgent(
            Request,
            OutputBufferLength,
            InputBufferLength,
            TRUE
            );
        break;

    //
    // Block an existent lease agent.
    //
    case IOCTL_LEASING_BLOCK_LEASE_AGENT:

        status = CloseLeaseAgent(
            Request,
            OutputBufferLength,
            InputBufferLength,
            FALSE
            );
        break;

    //
    // Create leasing application.
    //
    case IOCTL_CREATE_LEASING_APPLICATION:

        status = CreateLeasingApplication(
            Request,
            OutputBufferLength,
            InputBufferLength
            );
        break;

    //
    // Register leasing application.
    //
    case IOCTL_REGISTER_LEASING_APPLICATION:

        status = RegisterLeasingApplication(
            Request,
            OutputBufferLength,
            InputBufferLength
            );
        break;

    //
    // Unregister leasing application.
    //
    case IOCTL_UNREGISTER_LEASING_APPLICATION:
        
        status = UnregisterLeasingApplication(
            Request,
            OutputBufferLength,
            InputBufferLength
            );
        break;

    //
    // Establish a lease.
    //
    case IOCTL_ESTABLISH_LEASE:

        status = EstablishLeaseRelationship(
            Request,
            OutputBufferLength,
            InputBufferLength
            );
        break;

    //
    // Terminate a lease.
    //
    case IOCTL_TERMINATE_LEASE:

        status = TerminateLeaseRelationship(
            Request,
            OutputBufferLength,
            InputBufferLength
            );
        break;

    //
    // Return application expiration time.
    //
    case IOCTL_GET_LEASING_APPLICATION_EXPIRATION_TIME:

        status = GetLeasingApplicationExpirationTime(
            Request,
            OutputBufferLength,
            InputBufferLength
            );
        break;

    //
    // Set global lease expiration time.
    //

    case IOCTL_SET_GLOBAL_LEASE_EXPIRATION_TIME:

        status = SetGlobalLeaseExpirationTime(
            Request,
            OutputBufferLength,
            InputBufferLength
            );
        break;

    //
    // Process arbitration result.
    //
    case IOCTL_ARBITRATION_RESULT:

        status = ProcessArbitrationResult(
            Request,
            OutputBufferLength,
            InputBufferLength
            );
        break;

    //
    // Update the certificate.
    //
    case IOCTL_UPDATE_CERTIFICATE:

        status = UpdateSecuritySettings(
            Request,
            OutputBufferLength,
            InputBufferLength
            );
        break;

    //
    // Block a connection by dropping all messages.
    //
    case IOCTL_BLOCK_LEASE_CONNECTION:

        status = BlockLeaseConnection(
            Request,
            OutputBufferLength,
            InputBufferLength
            );
        break;

    // Set security descriptor in the listener.
    case IOCTL_SET_SECURITY_DESCRIPTOR:

        status = SetListenerSecurityDescriptor(
            Request,
            OutputBufferLength,
            InputBufferLength
            );
        break;

    // Update lease duration from config update
    case IOCTL_UPDATE_LEASE_DURATION:

        status = UpdateConfigLeaseDuration(
            Request,
            OutputBufferLength,
            InputBufferLength
            );
        break;

    case IOCTL_QUERY_LEASE_DURATION:

        status = QueryLeaseDuration(
            Request,
            OutputBufferLength,
            InputBufferLength
            );
        break;
    
    case IOCTL_CRASH_LEASING_APPLICATION:

        status = CrashLeasingApplication(
            Request,
            OutputBufferLength,
            InputBufferLength
            );
        break;

    case IOCTL_GET_REMOTE_LEASE_EXPIRATION_TIME:

        status = GetRemoteLeaseExpirationTime(
            Request,
            OutputBufferLength,
            InputBufferLength
            );
        break;

    case IOCTL_ADD_TRANSPORT_BEHAVIOR:

        status = addLeaseBehavior(
            Request,
            OutputBufferLength,
            InputBufferLength
            );
        break;

    case IOCTL_CLEAR_TRANSPORT_BEHAVIOR:

        status = removeLeaseBehavior(
            Request,
            OutputBufferLength,
            InputBufferLength
            );
        break;

    default:
        
        EventWriteInvalidIOCTL(
            NULL,
            IoControlCode
            );
        //
        // Reject processing of this control code.
        //
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    EventWriteApiEnd(
        NULL,
        GetLeaseIoCode(IoControlCode)
        );

    return status;
}

const LPWSTR GetLeaseIoCode(
    __in ULONG IoControlCode
)
{
    LPWSTR IoCodeString = L"";

    switch (IoControlCode) {
    case IOCTL_LEASING_CREATE_LEASE_AGENT:
        IoCodeString = L"LEASING_CREATE_LEASE_AGENT";
        break;

    case IOCTL_LEASING_CLOSE_LEASE_AGENT:
        IoCodeString = L"LEASING_CLOSE_LEASE_AGENT";
        break;

    case IOCTL_LEASING_BLOCK_LEASE_AGENT:
        IoCodeString = L"LEASING_BLOCK_LEASE_AGENT";
        break;

    case IOCTL_CREATE_LEASING_APPLICATION:
        IoCodeString = L"CREATE_LEASING_APPLICATION";
        break;

    case IOCTL_REGISTER_LEASING_APPLICATION:
        IoCodeString = L"REGISTER_LEASING_APPLICATION";
        break;

    case IOCTL_UNREGISTER_LEASING_APPLICATION:
        IoCodeString = L"UNREGISTER_LEASING_APPLICATION";
        break;

    case IOCTL_ESTABLISH_LEASE:
        IoCodeString = L"ESTABLISH_LEASE";
        break;

    case IOCTL_TERMINATE_LEASE:
        IoCodeString = L"TERMINATE_LEASE";
        break;

    case IOCTL_GET_LEASING_APPLICATION_EXPIRATION_TIME:
        IoCodeString = L"GET_LEASING_APPLICATION_EXPIRATION_TIME";
        break;

    case IOCTL_SET_GLOBAL_LEASE_EXPIRATION_TIME:
        IoCodeString = L"SET_GLOBAL_LEASE_EXPIRATION_TIME";
        break;

    case IOCTL_ARBITRATION_RESULT:
        IoCodeString = L"ARBITRATION_RESULT";
        break;

    case IOCTL_REMOTE_CERT_VERIFY_RESULT:
        IoCodeString = L"REMOTE_CERT_VERIFY_RESULT";
        break;

    case IOCTL_UPDATE_CERTIFICATE:
        IoCodeString = L"UPDATE_CERTIFICATE";
        break;

    case IOCTL_BLOCK_LEASE_CONNECTION:
        IoCodeString = L"BLOCK_LEASE_CONNECTION";
        break;

    case IOCTL_SET_SECURITY_DESCRIPTOR:
        IoCodeString = L"SET_SECURITY_DESCRIPTOR";
        break;

    case IOCTL_UPDATE_LEASE_DURATION:
        IoCodeString = L"UPDATE_LEASE_DURATION";
        break;

    case IOCTL_QUERY_LEASE_DURATION:
        IoCodeString = L"QUERY_LEASE_DURATION";
        break;

    case IOCTL_GET_REMOTE_LEASE_EXPIRATION_TIME:
        IoCodeString = L"GET_REMOTE_LEASE_EXPIRATION_TIME";
        break;

    case IOCTL_ADD_TRANSPORT_BEHAVIOR:
        IoCodeString = L"ADD_TRANSPORT_BEHAVIOR";
        break;

    case IOCTL_CLEAR_TRANSPORT_BEHAVIOR:
        IoCodeString = L"CLEAR_TRANSPORT_BEHAVIOR";
        break;

    case IOCTL_CRASH_LEASING_APPLICATION:
        IoCodeString = L"CRASH_LEASING_APPLICATION";
        break;

    default:
        IoCodeString = L"UNKNOWN";
        break;
    }

    return IoCodeString;
}

const LPWSTR GetLeaseState(
    __in ONE_WAY_LEASE_STATE State
    )
    
/*++

Routine Description:

    Returns readable lease state.

Parameters Description:

    State - state for pretty printing.

Return Value:

    Stringyfied lease state.

--*/

{
    LPWSTR StateString = L"";

    switch (State) {
        
    case LEASE_STATE_INACTIVE:
        StateString = L"INACTIVE";
        break;
    case LEASE_STATE_ACTIVE:
        StateString = L"ACTIVE";
        break;
    case LEASE_STATE_EXPIRED:
        StateString = L"EXPIRED";
        break;
    case LEASE_STATE_FAILED:
        StateString = L"FAILED";
        break;
    }

    return StateString;
}

VOID
DoMaintenance(
    __in PDRIVER_CONTEXT DriverContext
    )
{
    BOOLEAN Deleted = FALSE;
    BOOLEAN RestartLeaseAgent = TRUE;
    KIRQL OldIrql;

    LARGE_INTEGER Now;
    GetCurrentTime(&Now);

    EventWritePerformMaintenance(NULL);

    //
    // Run cleanup of lease agents and remote lease agents.
    //
    ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);
    for(auto iter = DriverContext->LeaseAgentContextHashTable.begin(); iter != DriverContext->LeaseAgentContextHashTable.end(); )
    {
        //
        // Retrieve lease agent context.
        //
        PLEASE_AGENT_CONTEXT LeaseAgentContext = *iter;

        KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

        //
        // Deallocate unregistered leasing application from this lease agent, if any.
        //
        ClearLeasingApplicationList(LeaseAgentContext->LeasingApplicationContextUnregisterList);
        //
        // Destruct the remote lease agents that had neutral arbitration earlier, but new lease has been established.
        //
        DestructArbitrationNeutralRemoteLeaseAgents(LeaseAgentContext);
        //
        // Deallocate remote lease agents from this lease agent, if any.
        //
        DeallocateFailedRemoteLeaseAgents(LeaseAgentContext, FALSE, Now);

        //
        // Check state of lease agent. If the lease agent is not failed,
        // see if it can be failed right now.
        //
        if (!IsLeaseAgentFailed(LeaseAgentContext)) {

            if (CanLeaseAgentBeFailed(LeaseAgentContext)) {

                if (0 == LeaseAgentContext->TimeToBeFailed.QuadPart)
                {
                    LeaseAgentContext->TimeToBeFailed.QuadPart = Now.QuadPart + (LONGLONG) (MAINTENANCE_INTERVAL - 1) * 1000 * MILLISECOND_TO_NANOSECOND_FACTOR;
                }
                else if (LeaseAgentContext->TimeToBeFailed.QuadPart < Now.QuadPart)
                {
                    SetLeaseAgentFailed(LeaseAgentContext);
                }
            }
            else if (0 != LeaseAgentContext->TimeToBeFailed.QuadPart)
            {
                // reset fail time in case previous maintenance already change it
                LeaseAgentContext->TimeToBeFailed.QuadPart = 0;
            }
        }

        //
        // In order to remove this lease agent, it has to be failed and ready
        // to deallocate, including all its remote lease agents.
        //
        RestartLeaseAgent =
                IsLeaseAgentFailed(LeaseAgentContext) &&
                IsLeaseAgentReadyForDeallocation(LeaseAgentContext);

        if (RestartLeaseAgent) {

//            EventWriteLeaseAgentCleanup(
//                    NULL,
//                    TransportIdentifier(LeaseAgentContext->Transport),
//                    LeaseAgentContext->Instance.QuadPart
//            );

            //
            // Remote remote lease agent from the lease agent hash table.
            //
            DriverContext->LeaseAgentContextHashTable.erase(iter);
            Deleted = TRUE;
        }

        KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);

        if (RestartLeaseAgent) {

            //
            // Deallocate lease agent.
            //
            LeaseAgentDestructor(LeaseAgentContext);
            iter = DriverContext->LeaseAgentContextHashTable.begin();
        }
        else{
            iter++;
        }
    }

    ExReleaseFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);
}

VOID
MaintenanceWorkerThread(
  //  __in PVOID Context
    )
/*++

Routine Description:

     Performs one task: regularly cleans up failed lease agents and remote lease agents.

Parameters Description:

    Context - execution control context.

Return Value:

    n/a.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN StopThread = FALSE;
    LARGE_INTEGER Timeout;
   // PWORKER_THREAD_CONTEXT WorkerThreadContext = (PWORKER_THREAD_CONTEXT)Context;

    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext();

    //
    // This thread will wake up every 15s to perform cleanup.
    //
    Timeout.QuadPart = -MAINTENANCE_INTERVAL * 1000 * MILLISECOND_TO_NANOSECOND_FACTOR;

    EventWriteWorkerThread1(
        NULL
        ); 

    while (!StopThread) {
    // Sleep instead of sleep, which can return due to signal
    Sleep(MAINTENANCE_INTERVAL * 1000);
        //
        // Wait for the shutdown event to be signaled.
        //
        //TODO shuxu
//        Status = KeWaitForSingleObject(
//            &WorkerThreadContext->Shutdown,
//            Executive,
//            KernelMode,
//            FALSE,
//            &Timeout
//            );

//        if (STATUS_SUCCESS == Status) {
//
//            EventWriteWorkerThread2(
//                NULL
//                );
//
//            //
//            // The driver is unloading.
//            //
//            StopThread = TRUE;
//
//        } else {
//            DoMaintenance(DriverContext);
//        }

        DoMaintenance(DriverContext);
        //
        // Go back to wait/sleep.
        //
    }

    PsTerminateSystemThread(STATUS_SUCCESS);
}

LARGE_INTEGER
    GetInstance( )
/*++

Routine Description:
     Wrap the performance counter inside and ensure a monotonically increasing unique value returned for instance.

Parameters Description:
    n/a

Return Value:
    n/a.

--*/
{
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext();
    KIRQL OldIrql;
    LARGE_INTEGER instance;

    // The instance has to be a persisted counter. We need to ensure when the machine restart, the next instance will be greater than
    // the previous instance. We used to use QueryPerformanceCounter, which caused some issues.
    instance.QuadPart = DateTime::Now().Ticks;

    //
    // Acquire the instance spinlock.
    //
    KeAcquireSpinLock(&DriverContext->InstanceLock, &OldIrql);
    //
    // Check to see whether the newly created instance is greater than the last instance saved in driver context.
    //
    if (IS_LARGE_INTEGER_GREATER_THAN_OR_EQUAL_LARGE_INTEGER(DriverContext->LastInstance, instance))
    {
        LARGE_INTEGER_ADD_INTEGER(DriverContext->LastInstance, 1);
        instance.QuadPart = DriverContext->LastInstance.QuadPart;
    }
    else
    {
        DriverContext->LastInstance.QuadPart = instance.QuadPart;
    }

    //
    // Release instance spinlock.
    //
    KeReleaseSpinLock(&DriverContext->InstanceLock, OldIrql);

    return instance;
}

