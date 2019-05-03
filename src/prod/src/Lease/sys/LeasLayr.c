// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "LeasLayr.h"
#include "LeasLayr.tmh"

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif

VOID
CompleteIoRequest(
    WDFREQUEST Request,
    NTSTATUS Status,
    ULONG BytesTransferred
    )

/*++

Routine Description:

    Request completion routine.

Arguments:

    Request - request to complete.

    Status - completion status.

    BytesTransferred - bytes transferred for this request.

Return Value:

    n/a

--*/

{
    //
    // Complete the request.
    //
    if (0 != BytesTransferred) {
        
        WdfRequestCompleteWithInformation(
            Request, 
            Status,
            (ULONG_PTR)BytesTransferred
            );
        
    } else {

        WdfRequestComplete(Request, Status);
    }
}

VOID
DelayedRequestCancelation(
    __in WDFREQUEST Request
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
    PREQUEST_CONTEXT ReqContext = LeaseLayerRequestGetContext(Request);

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
    // Complete request as canceled if possible.
    //
    if (CompleteRequest) {

        //
        // Complete request with cancellation code.
        //
        WdfRequestComplete(Request, STATUS_CANCELLED);
    }
}

//
// Signals load/unload of driver
//

VOID
DriverUnloadAddref()
{
#ifndef KTL_TRANSPORT
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());
    KeClearEvent(&DriverContext->CanUnloadEvent);
    InterlockedIncrement(&DriverContext->UnloadCount);
#endif
}

VOID
DriverUnloadRelease()
{
#ifndef KTL_TRANSPORT
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());
    if (InterlockedDecrement(&DriverContext->UnloadCount) == 0)
    {
        KeSetEvent(&DriverContext->CanUnloadEvent, 0, FALSE);
    }
#endif
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

__drv_dispatchType(IRP_MJ_CLEANUP)
DRIVER_DISPATCH LeaseLayerEvtCleanup;

NTSTATUS
DriverEntry(
    __in PDRIVER_OBJECT DriverObject,
    __in PUNICODE_STRING RegistryPath
    )
    
/*++

Routine Description:

    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded. DriverEntry specifies the other entry
    points in the function driver: LeaseLayerAddDevice and LeaseLayerUnload.

Parameters Description:

    DriverObject - represents the instance of the function driver that is loaded
        into memory. DriverEntry must initialize members of DriverObject before it
        returns to the caller. DriverObject is allocated by the system before the
        driver is loaded, and it is released by the system after the system unloads
        the function driver from memory.

    RegistryPath - represents the driver specific path in the Registry.
        The function driver can use the path to store driver related data between
        reboots. The path does not store hardware instance specific data.

Return Value:

    STATUS_SUCCESS if the driver can be loaded.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    WDF_DRIVER_CONFIG Config;
    WDF_OBJECT_ATTRIBUTES Attributes;
    PDRIVER_CONTEXT DriverContext = NULL;
    HANDLE ThreadHandle = NULL;
    PWDFDEVICE_INIT pInit = NULL;
    WDFDRIVER hDriver;

    //
    // Initialize driver config to control the attributes that
    // are global to the driver. This is a software-only driver
    // creating a single control device object.
    //
    WDF_DRIVER_CONFIG_INIT(&Config,
                           WDF_NO_EVENT_CALLBACK // This is a non-pnp driver.
                          );
    //
    // Set the pool tags associated with this driver. This tag will
    // be assigned to all driver's pool allocations.
    //
    Config.DriverPoolTag = LEASELAYER_POOL_TAG;

    // Tell the framework that this is non-pnp driver so that it doesn't
    // set the default AddDevice routine.
    Config.DriverInitFlags |= WdfDriverInitNonPnpDriver;
    
    //
    // Explicitly register an unload routine for the driver. This routine 
    // will not be called in the case of DriverEntry returning an error.
    //
    Config.EvtDriverUnload = LeaseLayerEvtDriverUnload;

    //
    // Register a cleanup callback so that we can call WPP_CLEANUP when
    // the framework driver object is deleted during driver unload.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, DRIVER_CONTEXT);

    //
    // Register a driver context area cleanup callback.
    //
    Attributes.EvtCleanupCallback = LeaseLayerEvtDriverContextCleanup;

    //
    // Create a framework driver object to represent our driver.
    //
    Status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        &Attributes,
        &Config,
        &hDriver
        );

    if (!NT_SUCCESS(Status)) {

        //
        // There is no tracing at this point except the debugger output.
        //
        KdPrintEx((
            DPFLTR_DEFAULT_ID, 
            DPFLTR_ERROR_LEVEL, 
            "WdfDriverCreate failed with status 0x%x\n", 
            Status
            ));

        return Status;
    }

    
    //
    // Since we are calling WPP_CLEANUP in the DriverContextCleanup
    // callback we should initialize WPP Tracing after WDFDRIVER
    // object is created to ensure that we cleanup WPP properly
    // if we return failure status from DriverEntry. This
    // eliminates the need to call WPP_CLEANUP in every path
    // of DriverEntry.
    //
    EventRegisterMicrosoft_ServiceFabric_Lease();
#if KTL_USER_MODE == 0
    EventRegisterMicrosoft_Windows_KTL();
#endif
    WPP_INIT_TRACING(DriverObject, RegistryPath);

    EventWriteLeaseLayerTrace1(
        NULL
        ); 
                
    //
    // Retrieve the driver object context area.
    //
    DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());
    RtlZeroMemory(DriverContext, sizeof(DRIVER_CONTEXT));

    //
    // Initialize driver context area.
    //
    ExInitializeFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);
    KeInitializeSpinLock(&DriverContext->TransportBehaviorTableAccessSpinLock);

    DriverContext->DriverObject = DriverObject;

    //
    //  Initialize the lease agent hash table.
    //
    RtlInitializeGenericTable(
        &DriverContext->LeaseAgentContextHashTable,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareLeaseAgentContexts,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL
        );

    RtlInitializeGenericTable(
        &DriverContext->TransportBehaviorTable,
        (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareTransportBlocking,
        (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
        (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
        NULL
        );

    //
    // Set the dispatch cleanup routine. This will be called
    // when user mode processes that use the driver are being
    // terminated (normally or abnormally).
    //
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = LeaseLayerEvtCleanup;

    // Initialize the transport layer
    Status = TransportInitialize(&DriverContext->DriverTransport);
    if (!NT_SUCCESS(Status)) 
    {
        EventWriteDriverFailedWith2(
            NULL,
            Status
            );     
        return Status;
    }

    //
    // Set the initial value of unresponsive duration and lastheartbeat time.
    //
    DriverContext->UnresponsiveDuration = 0;
    DriverContext->LastHeartbeatUpdateTime.QuadPart = 0;

    //
    // Create maintenance system thread.
    //
    DriverContext->MaintenanceInterval = MAINTENANCE_INTERVAL * 1000;

    DriverContext->ProcessAssertExitTimeout = PROCESS_ASSERT_EXIT_TIMEOUT;

    DriverContext->DelayLeaseAgentCloseInterval = DELAY_LEASE_AGENT_CLOSE_INTERVAL;

    KeInitializeEvent(
        &DriverContext->MaintenanceWorkerThreadContext.Shutdown, 
        SynchronizationEvent,
        FALSE
        );

    Status = PsCreateSystemThread(
        &ThreadHandle, 
        THREAD_ALL_ACCESS, 
        NULL, 
        NULL, 
        NULL,
        MaintenanceWorkerThread,
        &DriverContext->MaintenanceWorkerThreadContext
        );
    
    if (!NT_SUCCESS(Status)) {
        
        EventWriteDriverFailedWith1(
            NULL,
            Status
            );

        return Status;
    }

    //
    // Convert the thread object handle into a pointer to the 
    // thread object itself. Then close the thread object handle.
    //
    Status = ObReferenceObjectByHandle(
        ThreadHandle,
        THREAD_ALL_ACCESS,
        NULL,
        KernelMode,
        &DriverContext->MaintenanceWorkerThreadContext.WorkerThreadHandle,
        NULL
        );
    ZwClose(ThreadHandle);

    if (!NT_SUCCESS(Status)) {

        //
        // Set the event so the system thread exits.
        //
        KeSetEvent(
            &DriverContext->MaintenanceWorkerThreadContext.Shutdown,
            0,
            FALSE
            );
        TransportShutdownWait(DriverContext->DriverTransport);
        
        EventWriteDriverFailedWith4(
            NULL,
            Status
            );     
        return Status;
    }

    //
    // In order to create a control device, we first need to allocate a
    // WDFDEVICE_INIT structure and set all properties.
    //
    pInit = WdfControlDeviceInitAllocate(
                            hDriver,
                            &SDDL_DEVOBJ_SYS_ALL_ADM_ALL
                            );

    if (pInit == NULL)
    {
        TerminateSystemThread(DriverContext);
        TransportShutdownWait(DriverContext->DriverTransport);

        Status = STATUS_INSUFFICIENT_RESOURCES;
        return Status;
    }

    //
    // Call NonPnpDeviceAdd to create a deviceobject to represent our
    // software device.
    //
    Status = LeaseLayerEvtDeviceAdd(hDriver, pInit);

    if (!NT_SUCCESS(Status))
    {
        TerminateSystemThread(DriverContext);
        TransportShutdownWait(DriverContext->DriverTransport);
    }
    
    return Status;
}

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
    PVOID IterRemoteLeaseAgent = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = NULL;
    BOOLEAN RestartRemoteLeaseAgent = TRUE;
    BOOLEAN Deleted = TRUE;
    
    //
    // Iterate over remote lease agents.
    //
    IterRemoteLeaseAgent = RtlEnumerateGenericTable(
        &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
        TRUE
        );
    
    while (NULL != IterRemoteLeaseAgent) {

        //
        // Retrieve remote lease agent context.
        //
        RemoteLeaseAgentContext = *((PREMOTE_LEASE_AGENT_CONTEXT*)IterRemoteLeaseAgent);

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
                LONG ArbitrationDuration = RemoteLeaseAgentContext->LeaseRelationshipContext->ArbitrationDuration == DURATION_MAX_VALUE ?
                    RemoteLeaseAgentContext->LeaseAgentContext->ArbitrationDuration : RemoteLeaseAgentContext->LeaseRelationshipContext->ArbitrationDuration;

                // Wait for another full arbitration duration cycle so that user mode can still query expired date for this RLA
                RemoteLeaseAgentContext->TimeToBeFailed.QuadPart = CurrentTime.QuadPart 
                    + ((LONGLONG)ArbitrationDuration * MILLISECOND_TO_NANOSECOND_FACTOR);

                EventWriteSetRemoteLeaseAgentArbitrationDuration(
                    NULL,
                    RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                    RemoteLeaseAgentContext->Instance.QuadPart,
                    RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectIdentifier.QuadPart,
                    RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorIdentifier.QuadPart,
                    GetLeaseAgentState(RemoteLeaseAgentContext->State),
                    GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState),
                    GetLeaseState(RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState),
                    RemoteLeaseAgentContext->IsActive,
                    RemoteLeaseAgentContext->TimeToBeFailed.QuadPart
                    );
            }

            if (CurrentTime.QuadPart <= RemoteLeaseAgentContext->TimeToBeFailed.QuadPart || RemoteLeaseAgentContext->IsInArbitrationNeutral == TRUE)
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
            Deleted = RtlDeleteElementGenericTable(
                &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
                &RemoteLeaseAgentContext
                );
            LAssert(Deleted);
            
            //
            // Deallocate remote lease agent.
            //
            RemoteLeaseAgentDestructor(RemoteLeaseAgentContext);
        }

        //
        // Move on to the next element.
        //
        IterRemoteLeaseAgent = RtlEnumerateGenericTable(
            &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
            RestartRemoteLeaseAgent
            );
    }
}

VOID
DestructArbitrationNeutralRemoteLeaseAgents(__in PLEASE_AGENT_CONTEXT LeaseAgentContext)
{
    PVOID IterFailedRemoteLeaseAgent = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT FailedRemoteLeaseAgentContext = NULL;

    IterFailedRemoteLeaseAgent = RtlEnumerateGenericTable(
        &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
        TRUE
        );

    while (IterFailedRemoteLeaseAgent != NULL)
    {
        FailedRemoteLeaseAgentContext = *((PREMOTE_LEASE_AGENT_CONTEXT*) IterFailedRemoteLeaseAgent);

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

            PVOID IterOpenRemoteLeaseAgent = NULL;
            PREMOTE_LEASE_AGENT_CONTEXT OpenRemoteLeaseAgentContext = NULL;
            BOOLEAN FoundOpenRemoteLeaseAgent = FALSE;

            for (IterOpenRemoteLeaseAgent = RtlEnumerateGenericTable(
                &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
                TRUE);
                NULL != IterOpenRemoteLeaseAgent;
                IterOpenRemoteLeaseAgent = RtlEnumerateGenericTable(
                    &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
                    FALSE))
            {
                OpenRemoteLeaseAgentContext = *((PREMOTE_LEASE_AGENT_CONTEXT*) IterOpenRemoteLeaseAgent);

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

            //
            // Enumerate the table back to the original position of the first pointer.
            //
            PVOID IterBackRemoteLeaseAgent = NULL;
            PREMOTE_LEASE_AGENT_CONTEXT BackRemoteLeaseAgentContext = NULL;

            IterBackRemoteLeaseAgent = RtlEnumerateGenericTable(
                &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
                TRUE
                );

            for (IterBackRemoteLeaseAgent = RtlEnumerateGenericTable(
                &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
                TRUE);
                NULL != IterBackRemoteLeaseAgent;
                IterBackRemoteLeaseAgent = RtlEnumerateGenericTable(
                    &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
                    FALSE))
            {
                BackRemoteLeaseAgentContext = *((PREMOTE_LEASE_AGENT_CONTEXT*) IterBackRemoteLeaseAgent);

                if (FailedRemoteLeaseAgentContext == BackRemoteLeaseAgentContext)
                {
                    //
                    // Found the original position of the first pointer.
                    //
                    IterBackRemoteLeaseAgent = RtlEnumerateGenericTable(
                        &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
                        FALSE);
                    IterFailedRemoteLeaseAgent = IterBackRemoteLeaseAgent;
                    break;
                }
            }
        }
        else
        {
            IterFailedRemoteLeaseAgent = RtlEnumerateGenericTable(
                &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
                FALSE
                );
        }
    }
}

KDEFERRED_ROUTINE DriverUnloadExpiredTimerCallback;

VOID
DriverUnloadExpiredTimerCallback(
    __in PKDPC Dpc,
    __in_opt PVOID DeferredContext,
    __in_opt PVOID SystemArgument1,
    __in_opt PVOID SystemArgument2
)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    EventWriteDriverUnloadTimeout(NULL);

    KeBugCheckEx(
        LEASLAYR_BUGCHECK_CODE,
        DRIVER_UNLOAD_TIMEOUT_CODE,
        0,
        0,
        0);
}

VOID
LeaseLayerEvtDriverUnload(
    __in WDFDRIVER Driver
    )
    
/*++
Routine Description:

    Called by the I/O subsystem just before unloading the driver.
    You can free the resources created in the DriverEntry.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry.

Return Value:

    n/a

--*/

{
    PLEASE_AGENT_CONTEXT LeaseAgentContext = NULL;
    PVOID Iter = NULL;
    BOOLEAN Deleted = FALSE;
    KIRQL OldIrql;
    LARGE_INTEGER Now;
    KTIMER DriverUnloadExpiredTimer;
    KDPC DriverUnloadExpiredTimerDpc;

    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(Driver);

    //
    // Initialize driver unload expired timer.
    //
    KeInitializeTimer(&DriverUnloadExpiredTimer);
    //
    // Initialize the driver unload expired timer deferred procedure call.
    //
    KeInitializeDpc(
        &DriverUnloadExpiredTimerDpc,
        DriverUnloadExpiredTimerCallback,
        0
        );

    LARGE_INTEGER TimerTTL;
    TimerTTL.QuadPart = -((LONGLONG)DRIVER_UNLOAD_TIMEOUT) * MILLISECOND_TO_NANOSECOND_FACTOR;

    BOOLEAN DriverUnloadExpiredTimerRet = FALSE;
    DriverUnloadExpiredTimerRet = KeSetTimer(
        &DriverUnloadExpiredTimer,
        TimerTTL,
        &DriverUnloadExpiredTimerDpc
        );

    //
    // 1. Close all listening sockets. This way nothing comes into the system.
    // 2. Terminate all 
    //    - sockets
    //    - timers 
    //    - queued dpcs
    //    - kernel threads
    //    - work items
    //    so no new pending operations exist or are accepted in the system.
    // 3. Close all remaining sockets, so all existent pending operations fail.
    // 4. Unregister WSK, so all existent pending operations terminate.
    // 5. Release allocated memory.
    //

    EventWriteDriverUnloading(NULL, (LONGLONG)DriverContext);

    DriverContext->Unloading = TRUE;

    //
    // Stop the maintenance thread.
    //
    TerminateSystemThread(DriverContext);
    
    EventWriteMaintenanceThreadStopped(NULL);

    //
    // Uninitialize each lease agent from the hash table.
    //

    ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    for (Iter = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable, 
            TRUE);
         NULL != Iter;
         Iter = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable, 
            TRUE)) {

        //
        // Retrieve lease agent context.
        //
        LeaseAgentContext = *((PLEASE_AGENT_CONTEXT*)Iter);

        KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);
        
        //
        // Remove the remote lease agent from the hash table.
        //
        Deleted = RtlDeleteElementGenericTable(
            &DriverContext->LeaseAgentContextHashTable,
            &LeaseAgentContext
            );
        LAssert(Deleted);
        
        //
        // Uninitialize the lease agent context.
        //
        LeaseAgentUninitialize(LeaseAgentContext, FALSE);

        KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);

        Now.QuadPart = 0;
        DeallocateFailedRemoteLeaseAgents(LeaseAgentContext, TRUE, Now);

        // 
        // Now destroy the LA
        //
        LeaseAgentDestructor(LeaseAgentContext);
    }

    ExReleaseFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    TransportShutdownWait(DriverContext->DriverTransport);

    EventWriteDeallocateLeaseAgents(NULL);

    EventWriteUnloadComplete(NULL);

    KeCancelTimer(&DriverUnloadExpiredTimer);
}

NTSTATUS
LeaseLayerEvtDeviceAdd(
    __in WDFDRIVER Driver,
    __inout PWDFDEVICE_INIT DeviceInit
    )
    
/*++
Routine Description:

    LeaseLayerEvtDeviceAdd is called by DriverEntry. We create and initialize 
    a device object to represent a new instance of lease layer device.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    STATUS_SUCCESS if the device can be created and initialized.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    WDF_OBJECT_ATTRIBUTES FdoAttributes;
    WDF_OBJECT_ATTRIBUTES RequestAttributes;
    WDFDEVICE Device;
    WDF_IO_QUEUE_CONFIG QueueConfig;
    PDEVICE_CONTEXT DeviceContext;
    WDFQUEUE Queue;
    DECLARE_CONST_UNICODE_STRING(DeviceName, LEASELAYER_DEVICE_NAME_STRING);
    DECLARE_CONST_UNICODE_STRING(SymbolicLinkName, LEASELAYER_SYMBOLIC_NAME_STRING);

    UNREFERENCED_PARAMETER(Driver);

    //
    // This implementation uses buffered I/O to access data buffers.
    //
    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered);

    //
    // Assign the device name to the device object.
    //
    Status = WdfDeviceInitAssignName(DeviceInit, &DeviceName);

    if (!NT_SUCCESS(Status)) {

        EventWriteDeviceFailedWith4(
             NULL,
             Status
             );     
        goto End;
    }

    //
    // Now specify the size of device extension where we track per device context.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&FdoAttributes, DEVICE_CONTEXT);

    //
    // We handle all synchronization.
    //
    FdoAttributes.SynchronizationScope = WdfSynchronizationScopeNone;

    //
    // Set a context cleanup routine to cleanup any resources that are not
    // parent to this device. This cleanup will be called in the context of
    // remove-device when the framework deletes the device object.
    //
    FdoAttributes.EvtCleanupCallback = LeaseLayerEvtDeviceContextCleanup;

    //
    // Now specify the size of request extension where we track per request context.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&RequestAttributes, REQUEST_CONTEXT);
    WdfDeviceInitSetRequestAttributes(DeviceInit, &RequestAttributes);

    //
    // DeviceInit is completely initialized. So call the framework to create the
    // device and attach it to the device stack.
    //
    Status = WdfDeviceCreate(
        &DeviceInit, 
        &FdoAttributes, 
        &Device
        );
    
    if (!NT_SUCCESS(Status)) {

        EventWriteDeviceFailedWith5(
            NULL,
            Status
            );     
        goto End;
    }

    //
    // Get the device context by using accessor function.
    //
    DeviceContext = LeaseLayerDeviceGetContext(Device);
    RtlZeroMemory(DeviceContext, sizeof(DEVICE_CONTEXT));

    //
    // Create a symbolic link for the control object so that usermode can open
    // the device.
    //
    Status = WdfDeviceCreateSymbolicLink(Device, &SymbolicLinkName);

    if (!NT_SUCCESS(Status)) {

        //
        // Control device will be deleted automatically by the framework.
        //
        EventWriteDeviceFailedWith6(
            NULL,
            Status
            );     

        goto End;
    }

    //
    // Register I/O callbacks to tell the framework that you are interested
    // in handling only IRP_MJ_DEVICE_CONTROL requests.
    // In case a specific handler is not specified for one of these,
    // the request will be dispatched to the EvtIoDefault handler, if any.
    // If there is no EvtIoDefault handler, the request will be failed with
    // STATUS_INVALID_DEVICE_REQUEST.
    // WdfIoQueueDispatchParallel means that we are capable of handling
    // all the I/O request simultaneously and we are responsible for protecting
    // data that could be accessed by these callbacks simultaneously.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
        &QueueConfig,
        WdfIoQueueDispatchParallel
        );
    QueueConfig.EvtIoDeviceControl = LeaseLayerEvtIoDeviceControl;

    //
    // Create the queue for this device.
    // 
    Status = WdfIoQueueCreate(
        Device,
        &QueueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &Queue
        );
    
    if (!NT_SUCCESS(Status)) {

        EventWriteDeviceFailedWith7(
            NULL,
            Status
            );     
        goto End;
    }

    //
    // Control devices must notify WDF when they are done initializing.   I/O is
    // rejected until this call is made.
    //
    WdfControlFinishInitializing(Device);

    //
    // Establish security descriptor for device object. There is a case
    // where the security descriptor is not yet set up (first device
    // start) so do not fail in this case.
    //
    NTSTATUS statusDontCare;
    statusDontCare = LeaseLayerEstablishSecurity(Device);
    
End:
    //
    // If the device is created successfully, framework would clear the
    // DeviceInit value. Otherwise device create must have failed so we
    // should free the memory ourself.
    //
    if (DeviceInit != NULL) {
        WdfDeviceInitFree(DeviceInit);
    }
    
    return Status;  
}

NTSTATUS LeaseLayerEstablishSecurity(
    __in WDFDEVICE ControlDevice
    )
{
    NTSTATUS status;
    WDFDRIVER driver;
    WDFKEY key;
    WDFMEMORY memory;
    size_t size;
    DECLARE_CONST_UNICODE_STRING(securityDescriptorName, L"SecurityDescriptor");
    PSECURITY_DESCRIPTOR securityDescriptor = NULL;
    PDEVICE_OBJECT deviceObject;
    HANDLE fileHandle;

    driver = WdfDeviceGetDriver(ControlDevice);
    if (driver == NULL)
    {
        return(STATUS_NOT_FOUND);
    }

    deviceObject = WdfDeviceWdmGetDeviceObject(ControlDevice);
    if (deviceObject == NULL)
    {
        return(STATUS_NOT_FOUND);       
    }
        
    status = WdfDriverOpenParametersRegistryKey(driver,
                                                KEY_READ,
                                                NULL,         // KeyAttributes
                                                &key);

    if (NT_SUCCESS(status))
    {
        status = WdfRegistryQueryMemory(key,
                                        &securityDescriptorName,
                                        PagedPool,
                                        NULL,       // MemoryAttributes
                                        &memory,
                                        NULL);      // ValueType
        if (NT_SUCCESS(status))
        {
            securityDescriptor = WdfMemoryGetBuffer(memory,
                                                    &size);
            if (securityDescriptor != NULL)
            {
                status = ObOpenObjectByPointer(deviceObject,
                                                   OBJ_KERNEL_HANDLE,
                                                   NULL,
                                                   WRITE_DAC,
                                                   0,
                                                   KernelMode,
                                                   &fileHandle);

                if (NT_SUCCESS(status))
                {
                    status = ZwSetSecurityObject(fileHandle, DACL_SECURITY_INFORMATION, securityDescriptor);
                    
                    ZwClose(fileHandle);
                }
            }

            WdfObjectDelete(memory);
        }
                                        
        WdfRegistryClose(key);
    }

    return(status);
}


VOID
LeaseLayerEvtShutdown(
    __in WDFDEVICE Device
    )
    
/*++

Routine Description:

    Callback invoked when the machine is shutting down.  If you register for
    a last chance shutdown notification you cannot do the following:
        - Call any pageable routines
        - Access pageable memory
        - Perform any file I/O operations

    If you register for a normal shutdown notification, all of these are
    available to you.

    This function implementation does nothing, but if you had any outstanding
    file handles open, this is where you would close them.

Arguments:

    Device - The device which registered the notification during init

Return Value:

    n/a

  --*/

{
    UNREFERENCED_PARAMETER(Device);
}

VOID
LeaseLayerEvtDriverContextCleanup(
    __in WDFOBJECT Object
    )
    
/*++
Routine Description:

    Called when the driver object is deleted during driver unload.
    You can free all the resources created in DriverEntry that are
    not automatically freed by the framework.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

Return Value:

    n/a

--*/

{
    UNREFERENCED_PARAMETER(Object);
    
    EventWriteLeaseLayerTrace2(
        NULL
        ); 

    //
    // Cleanup WPP tracing.
    //
    WPP_CLEANUP(WdfGetDriver());
#if KTL_USER_MODE == 0
    EventUnregisterMicrosoft_Windows_KTL();
#endif
    EventUnregisterMicrosoft_ServiceFabric_Lease();
}

VOID
LeaseLayerEvtDeviceContextCleanup(
    WDFOBJECT Device
    )
    
/*++

Routine Description:

    EvtDeviceContextCleanup event callback must perform any operations that are
    necessary before the specified device is removed. The framework calls
    the driver's EvtDeviceContextCleanup callback when the device is deleted in response
    to IRP_MN_REMOVE_DEVICE request.

Arguments:

    Device - Handle to a framework device object.

Return Value:

    n/a

--*/

{
    UNREFERENCED_PARAMETER(Device);    
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

VOID
CreateLeaseAgent(
    __in WDFREQUEST Request,
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
    PLEASE_AGENT_CONTEXT LeaseAgentContextIter = NULL;
    PLEASE_AGENT_CONTEXT* LeaseAgentContextExistent = NULL;

    PCREATE_LEASE_AGENT_INPUT_BUFFER_DEVICE_IOCTL CreateLeaseAgentInputBuffer = NULL;
    PLEASE_AGENT_BUFFER_DEVICE_IOCTL CreateLeaseAgentOutputBuffer = NULL;

    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());

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
    // Check input buffer - hash store name.
    //
    if (IsSecurityProviderSsl(CreateLeaseAgentInputBuffer->SecurityType) &&
        !IsValidString(CreateLeaseAgentInputBuffer->CertHashStoreName, SCH_CRED_MAX_STORE_NAME_SIZE))
    {
        EventWriteInvalidReq2(
            NULL
            );
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;
        goto End;
    }

    //
    // Construct lease agent structure.
    //
    CreateLeaseAgentInputBuffer->CertHashStoreName[SCH_CRED_MAX_STORE_NAME_SIZE - 1] = L'\0';
    Status = LeaseAgentConstructor(
        &CreateLeaseAgentInputBuffer->SocketAddress,
        &CreateLeaseAgentInputBuffer->LeaseConfigDurations,
        CreateLeaseAgentInputBuffer->LeaseSuspendDurationMilliseconds,
        CreateLeaseAgentInputBuffer->ArbitrationDurationMilliseconds,
        CreateLeaseAgentInputBuffer->NumberOfLeaseRetry,
        CreateLeaseAgentInputBuffer->StartOfLeaseRetry,
        CreateLeaseAgentInputBuffer->SecurityType,
        CreateLeaseAgentInputBuffer->CertHash,
        CreateLeaseAgentInputBuffer->CertHashStoreName,
        CreateLeaseAgentInputBuffer->SessionExpirationTimeout,
        &LeaseAgentContext
        );

    if (!NT_SUCCESS(Status)) {

        //
        // Request cannot be completed.
        //
        goto End;
    }

    ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    // Under mutext, check and update unresponsive duration in driver context
    if ( (NULL != LeaseAgentContext) && 
        (DriverContext->UnresponsiveDuration != LeaseAgentContext->LeaseConfigDurations.UnresponsiveDuration) )
    {
        EventWriteUpdateLeaseAgentDuration(
            NULL,
            L"Unresponsive ",
            DriverContext->UnresponsiveDuration,
            LeaseAgentContext->LeaseConfigDurations.UnresponsiveDuration);

        DriverContext->UnresponsiveDuration = LeaseAgentContext->LeaseConfigDurations.UnresponsiveDuration;
    }

    //
    // Check to see if this lease agent contains already the remote lease agent.
    // If a remote lease agent of the same remote address exists and is not 
    // terminating, then reuse it.
    //
    for (IterLeaseAgent = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable, 
            TRUE);
         NULL != IterLeaseAgent && !Found;
         IterLeaseAgent = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable, 
            FALSE)) {
    
        //
        // Retrieve lease agent.
        //
        LeaseAgentContextIter = *((PLEASE_AGENT_CONTEXT*)IterLeaseAgent);

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

            if (LeaseAgentContextIter->SecurityType != LeaseAgentContext->SecurityType)
            {
                SecuritySettingMatch = FALSE;
            }

            if (IsSecurityProviderSsl(LeaseAgentContextIter->SecurityType) && IsSecurityProviderSsl(LeaseAgentContext->SecurityType) &&
                memcmp(LeaseAgentContextIter->CertHash, LeaseAgentContext->CertHash, SHA1_LENGTH) != 0)
            {
                SecuritySettingMatch = FALSE;
            }

            if (FALSE == SecuritySettingMatch)
            {
                EventWriteLeaseSecuritySettingMismatch(
                    NULL,
                    TransportIdentifier(LeaseAgentContextIter->Transport),
                    LeaseAgentContextIter->Instance.QuadPart,
                    LeaseAgentContextIter->SecurityType,
                    TransportIdentifier(LeaseAgentContext->Transport),
                    LeaseAgentContext->Instance.QuadPart,
                    LeaseAgentContext->SecurityType
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
        LeaseAgentContextExistent = RtlInsertElementGenericTable(
            &DriverContext->LeaseAgentContextHashTable,
            &LeaseAgentContext,
            sizeof(PLEASE_AGENT_CONTEXT),
            &Inserted
            );
            
        //
        // Check insert result.
        //
        if (NULL == LeaseAgentContextExistent)
        {
            LAssert(!Inserted);

            EventWriteLeaseAgentActionError6(
                NULL,
                TransportIdentifier(LeaseAgentContext->Transport),
                LeaseAgentContext->Instance.QuadPart
                );

            //
            // Destroy lease agent.
            //
            LeaseAgentDestructor(LeaseAgentContext);
            
            //
            // Hash table memory allocation failed.
            //
            Status = STATUS_INSUFFICIENT_RESOURCES;

        }
        else if (*LeaseAgentContextExistent != LeaseAgentContext)
        {
            //
            // This can happen if creation of lease agents is too quick in a loop.
            // The lease agent comparison is performed on its instance,
            // and not its socket address.
            //
            LAssert(!Inserted);

            EventWriteLeaseAgentActionError5(
                NULL,
                TransportIdentifier((*LeaseAgentContextExistent)->Transport),
                (*LeaseAgentContextExistent)->Instance.QuadPart
                );

            //
            // Destroy lease agent.
            //
            LeaseAgentDestructor(LeaseAgentContext);

            //
            // Hash table memory allocation failed.
            //
            Status = STATUS_RETRY;

        }
        else
        {
            LAssert(Inserted);
            LAssert(*LeaseAgentContextExistent == LeaseAgentContext);

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
                SetLeaseAgentFailed(LeaseAgentContext, FALSE);

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
    
    //
    // Complete the request.
    //
    CompleteIoRequest(
        Request,
        Status,
        BytesTransferred
        );
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
    PVOID IterRemoteLeaseAgent = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = NULL;

    EventWriteTerminateAllLeaseRelation(
        NULL,
        LeasingApplicationContext->LeasingApplicationIdentifier,
        TransportIdentifier(LeaseAgentContext->Transport),
        LeaseAgentContext->Instance.QuadPart
        );

    //
    // Terminate all leases related for this application.
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

        if ( IsRemoteLeaseAgentSuspended(RemoteLeaseAgentContext) && 
            RemoteLeaseAgentContext->LeaseRelationshipContext->SubjectState == LEASE_STATE_EXPIRED &&
            RemoteLeaseAgentContext->LeaseRelationshipContext->MonitorState == LEASE_STATE_EXPIRED &&
            RemoteLeaseAgentContext->LeasingApplicationForArbitration->Instance == LeasingApplicationContext->Instance &&
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
    PLEASING_APPLICATION_CONTEXT* LeasingApplicationContextExistent = NULL;
    BOOLEAN Deleted = FALSE;
    BOOLEAN Inserted = FALSE;
    //
    // Add to the unregister list may fail;
    // The unregister will retry through the timer with UnregisterRetryInterval in milliseconds
    //
    LONG UnregisterRetryInterval = 0;

    PLEASE_AGENT_CONTEXT LeaseAgentContext = LeasingApplicationContext->LeaseAgentContext;
    //
    // Insert the leasing application to the un-registered list.
    //
    LeasingApplicationContextExistent = RtlInsertElementGenericTable(
        &LeaseAgentContext->LeasingApplicationContextUnregisterList,
        &LeasingApplicationContext,
        sizeof(PLEASING_APPLICATION_CONTEXT),
        &Inserted
        );

    if (NULL == LeasingApplicationContextExistent)
    {
        LAssert(!Inserted);

        UnregisterRetryInterval = 1000; // using 1000ms for now
        EnqueueUnregisterTimer(LeasingApplicationContext, UnregisterRetryInterval);

        EventWriteInsertUnregisterListFail(
            NULL,
            LeasingApplicationContext->LeasingApplicationIdentifier,
            Inserted);

        //
        // Hash table memory allocation failed.
        //
        return;
    }
    //
    // Remove the leasing application from the registered list.
    //
    Deleted = RtlDeleteElementGenericTable(
        &LeaseAgentContext->LeasingApplicationContextHashTable,
        &LeasingApplicationContext
        );

    LAssert(Deleted);
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
    TransportAbort(LeaseAgentContext->Transport);

    EventWriteLeaseAgentNowInState(
        NULL,
        TransportIdentifier(LeaseAgentContext->Transport),
        LeaseAgentContext->Instance.QuadPart,
        GetLeaseAgentState(LeaseAgentContext->State)
        ); 
}

VOID
CloseLeaseAgent(
    __in WDFREQUEST Request,
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
    PVOID IterLeaseAgent = NULL;
    BOOLEAN Found = FALSE;
    size_t Length = 0;
    KIRQL OldIrql;
    
    PLEASE_AGENT_CONTEXT LeaseAgentContext = NULL;
    PLEASE_AGENT_CONTEXT LeaseAgentContextLookup = NULL;
    PCLOSE_LEASE_AGENT_BUFFER_DEVICE_IOCTL LeaseAgentInputBuffer = NULL;

    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());

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
        0, // security type
        NULL, // cert hash
        NULL, // cert hash store name
        0, // session timeout
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
    for (IterLeaseAgent = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable, 
            TRUE);
         NULL != IterLeaseAgent && !Found;
         IterLeaseAgent = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable, 
            FALSE)) {

        //
        // Retrieve lease agent context.
        //
        LeaseAgentContext = *((PLEASE_AGENT_CONTEXT*)IterLeaseAgent);

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

    //
    // Complete request.
    //
    CompleteIoRequest(Request, Status, 0);
}

VOID
BlockLeaseConnection(
    __in WDFREQUEST Request,
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
    PVOID IterLeaseAgent = NULL;
    BOOLEAN Found = FALSE;
    size_t Length = 0;
    KIRQL OldIrql;
    
    PLEASE_AGENT_CONTEXT LeaseAgentContext = NULL;
    PLEASE_AGENT_CONTEXT LeaseAgentContextLookup = NULL;

    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContextLookup = NULL;

    PBLOCK_LEASE_CONNECTION_BUFFER_DEVICE_IOCTL LeaseAgentInputBuffer = NULL;

    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());

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
        0, // security type
        NULL, // cert hash
        NULL, // cert hash store name
        0, // session timeout
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
    for (IterLeaseAgent = RtlEnumerateGenericTable(&DriverContext->LeaseAgentContextHashTable, TRUE);
         NULL != IterLeaseAgent && !Found;
         IterLeaseAgent = RtlEnumerateGenericTable(&DriverContext->LeaseAgentContextHashTable, FALSE))
    {

        LeaseAgentContext = *((PLEASE_AGENT_CONTEXT*)IterLeaseAgent);
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
                        &RemoteLeaseAgentContext->PartnerTarget
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

    //
    // Complete request.
    //
    CompleteIoRequest(Request, Status, 0);
}

VOID
SetListenerSecurityDescriptor(
    __in WDFREQUEST Request,
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
    PVOID IterLeaseAgent = NULL;
    BOOLEAN Found = FALSE;
    size_t Length = 0;
    KIRQL OldIrql;

    PLEASE_AGENT_CONTEXT LeaseAgentContext = NULL;
    PSET_SECURITY_DESCRIPTOR_INPUT_BUFFER setSecurityDescriptorBuffer = NULL;
    PTRANSPORT_SECURITY_DESCRIPTOR transportSecurityDescriptor = NULL;

    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());

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

    for (IterLeaseAgent = RtlEnumerateGenericTable(&DriverContext->LeaseAgentContextHashTable, TRUE);
         NULL != IterLeaseAgent && !Found;
         IterLeaseAgent = RtlEnumerateGenericTable(&DriverContext->LeaseAgentContextHashTable, FALSE))
    {
        LeaseAgentContext = *((PLEASE_AGENT_CONTEXT*)IterLeaseAgent);

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

    CompleteIoRequest(Request, Status, 0);
}

VOID
UpdateLeaseGlobalConfig(
    __in WDFREQUEST Request,
    __in size_t OutputBufferLength,
    __in size_t InputBufferLength)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PVOID InputBuffer = NULL;
    size_t Length = 0;
    PUPDATE_LEASE_GLOBAL_CONFIG_INPUT_BUFFER UpdateLeaseGlobalConfigInputBuffer = NULL;
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());

    EventWriteLeaseDriverTextTraceInfo(NULL, L"UpdateLeaseGlobalConfig");

    if (0 != OutputBufferLength)
    {
        EventWriteInvalidReq5(NULL);
        Status = STATUS_INVALID_PARAMETER;
        goto End;
    }

    if (InputBufferLength >= sizeof(UPDATE_LEASE_GLOBAL_CONFIG_INPUT_BUFFER))
    {
        Status = WdfRequestRetrieveInputBuffer(
            Request,
            InputBufferLength,
            &InputBuffer,
            &Length
        );

        if (!NT_SUCCESS(Status))
        {
            EventWriteRetrieveFail1(NULL);
            goto End;
        }

        LAssert(NULL != InputBuffer);
        LAssert(Length == InputBufferLength);
    }
    else
    {
        EventWriteInvalidReq1(NULL);
        Status = STATUS_INVALID_PARAMETER;
        goto End;
    }

    UpdateLeaseGlobalConfigInputBuffer = (PUPDATE_LEASE_GLOBAL_CONFIG_INPUT_BUFFER)InputBuffer;

    EventWriteLeaseGlobalConfigUpdate(
        NULL,
        DriverContext->MaintenanceInterval,
        UpdateLeaseGlobalConfigInputBuffer->LeaseGlobalConfig.MaintenanceInterval,
        DriverContext->ProcessAssertExitTimeout,
        UpdateLeaseGlobalConfigInputBuffer->LeaseGlobalConfig.ProcessAssertExitTimeout,
        DriverContext->DelayLeaseAgentCloseInterval,
        UpdateLeaseGlobalConfigInputBuffer->LeaseGlobalConfig.DelayLeaseAgentCloseInterval);

    DriverContext->MaintenanceInterval = UpdateLeaseGlobalConfigInputBuffer->LeaseGlobalConfig.MaintenanceInterval;
    DriverContext->ProcessAssertExitTimeout = UpdateLeaseGlobalConfigInputBuffer->LeaseGlobalConfig.ProcessAssertExitTimeout;
    DriverContext->DelayLeaseAgentCloseInterval = UpdateLeaseGlobalConfigInputBuffer->LeaseGlobalConfig.DelayLeaseAgentCloseInterval;

End:
    CompleteIoRequest(Request, Status, 0);
}

VOID
UpdateConfigLeaseDuration
(
    __in WDFREQUEST Request,
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
    PVOID IterLeaseAgent = NULL;
    BOOLEAN Found = FALSE;
    size_t Length = 0;
    KIRQL OldIrql;
    BOOLEAN IsDurationChanged = FALSE;
    BOOLEAN IsDurationAcrossFDChanged = FALSE;

    PLEASE_AGENT_CONTEXT LeaseAgentContext = NULL;
    PVOID IterRemoteLeaseAgent = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = NULL;
    PUPDATE_LEASE_DURATION_INPUT_BUFFER UpdateLeaseDurationBuffer = NULL;

    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());

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

    // Under mutext, check and update unresponsive duration in driver context
    if (DriverContext->UnresponsiveDuration != UpdateLeaseDurationBuffer->UpdatedLeaseDurations.UnresponsiveDuration)
    {
        EventWriteUpdateLeaseAgentDuration(
            NULL,
            L"Unresponsive ",
            DriverContext->UnresponsiveDuration,
            UpdateLeaseDurationBuffer->UpdatedLeaseDurations.UnresponsiveDuration);

        DriverContext->UnresponsiveDuration = UpdateLeaseDurationBuffer->UpdatedLeaseDurations.UnresponsiveDuration;
    }

    for (IterLeaseAgent = RtlEnumerateGenericTable(&DriverContext->LeaseAgentContextHashTable, TRUE);
         NULL != IterLeaseAgent && !Found;
         IterLeaseAgent = RtlEnumerateGenericTable(&DriverContext->LeaseAgentContextHashTable, FALSE))
    {
        LeaseAgentContext = *((PLEASE_AGENT_CONTEXT*)IterLeaseAgent);

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
                for (IterRemoteLeaseAgent = RtlEnumerateGenericTable(&LeaseAgentContext->RemoteLeaseAgentContextHashTable, TRUE);
                    NULL != IterRemoteLeaseAgent;
                    IterRemoteLeaseAgent = RtlEnumerateGenericTable(&LeaseAgentContext->RemoteLeaseAgentContextHashTable, FALSE))
                {
                    RemoteLeaseAgentContext = *((PREMOTE_LEASE_AGENT_CONTEXT*)IterRemoteLeaseAgent);

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
                            RemoteLeaseAgentContext->LeaseRelationshipContext->EstablishDurationType,
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

    CompleteIoRequest(Request, Status, 0);
}

VOID
UpdateLastHeartbeatTime(ULONG LastHeartbeatErrorCode)
/*++
Routine Description:
- Update the last success heartbeat time
- Update the last failed heartbeat error code
--*/
{
    LARGE_INTEGER Now;
    LONG UpdateTimePassedMilliseconds = 0;

    GetCurrentTime(&Now);
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());

    // winerror ERROR_SUCCESS
    if (0 == LastHeartbeatErrorCode)
    {
        ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

        // Skip trace warning for the very first time when last update time is 0.
        if ((DriverContext->UnresponsiveDuration != 0) && (DriverContext->LastHeartbeatUpdateTime.QuadPart != 0))
        {
            UpdateTimePassedMilliseconds = (LONG)((Now.QuadPart - DriverContext->LastHeartbeatUpdateTime.QuadPart) / MILLISECOND_TO_NANOSECOND_FACTOR);

            // By default unresponsive duration is 60 seconds, trace warning if 30 seconds has passed.
            if (UpdateTimePassedMilliseconds > DriverContext->UnresponsiveDuration / 2) 
            {
                EventWriteHeartbeatTooLong(NULL, UpdateTimePassedMilliseconds);
            }
        }

        DriverContext->LastHeartbeatUpdateTime.QuadPart = Now.QuadPart;

        ExReleaseFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

        EventWriteHeartbeatSucceeded(NULL);
    }
    else
    {
        DriverContext->LastHeartbeatErrorCode = LastHeartbeatErrorCode;

        EventWriteHeartbeatFailed(NULL, LastHeartbeatErrorCode);
    }
}

VOID
Heartbeat
(
__in WDFREQUEST Request,
__in size_t OutputBufferLength,
__in size_t InputBufferLength
)
/*++
Routine Description:
- Find the lease agent by the handle in the input buffer
- Update the lease heart beat time and last error code.
--*/
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PVOID InputBuffer = NULL;
    size_t Length = 0;

    if (0 != OutputBufferLength)
    {
        EventWriteInvalidReq5(NULL);
        Status = STATUS_INVALID_PARAMETER;
        goto End;
    }

    if (InputBufferLength >= sizeof(HEARTBEAT_INPUT_BUFFER))
    {
        Status = WdfRequestRetrieveInputBuffer(
            Request,
            InputBufferLength,
            &InputBuffer,
            &Length
            );

        if (!NT_SUCCESS(Status))
        {
            EventWriteRetrieveFail1(NULL);
            goto End;
        }

        LAssert(NULL != InputBuffer);
        LAssert(Length == InputBufferLength);
    }
    else
    {
        EventWriteInvalidReq1(NULL);
        Status = STATUS_INVALID_PARAMETER;
        goto End;
    }

    UpdateLastHeartbeatTime(((PHEARTBEAT_INPUT_BUFFER)InputBuffer)->ErrorCode);

End:

    CompleteIoRequest(Request, Status, 0);
}

VOID
QueryLeaseDuration(
    __in WDFREQUEST Request,
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
    PVOID IterLeaseAgent = NULL;
    BOOLEAN Found = FALSE;
    size_t Length = 0;
    ULONG BytesTransferred = 0;
    KIRQL OldIrql;

    PLEASE_AGENT_CONTEXT LeaseAgentContext = NULL;
    PLEASE_AGENT_CONTEXT LeaseAgentContextLookup = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContextLookup = NULL;

    PBLOCK_LEASE_CONNECTION_BUFFER_DEVICE_IOCTL QueryDurationInputBuffer = NULL;
    PGET_LEASING_APPLICATION_EXPIRATION_OUTPUT_BUFFER QueryDurationOutputBuffer = NULL;
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());

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
        0, // security type
        NULL, // cert hash
        NULL, // cert hash store name
        0, // session timeout
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

    for (IterLeaseAgent = RtlEnumerateGenericTable(&DriverContext->LeaseAgentContextHashTable, TRUE);
         NULL != IterLeaseAgent && !Found;
         IterLeaseAgent = RtlEnumerateGenericTable(&DriverContext->LeaseAgentContextHashTable, FALSE))
    {
        LeaseAgentContext = *((PLEASE_AGENT_CONTEXT*)IterLeaseAgent);
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

    CompleteIoRequest(Request, Status, BytesTransferred);
}

VOID
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

    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());
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
    for (IterLeaseAgent = RtlEnumerateGenericTable(
        &DriverContext->LeaseAgentContextHashTable,
        TRUE);
    NULL != IterLeaseAgent && !FoundLeasingApplication;
    IterLeaseAgent = RtlEnumerateGenericTable(
        &DriverContext->LeaseAgentContextHashTable,
        FALSE)) 
    {
        //
        // Retrieve lease agent context.
        //
        LeaseAgentContext = *((PLEASE_AGENT_CONTEXT*)IterLeaseAgent);

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
        for (IterLeasingApplication = RtlEnumerateGenericTable(
            &LeaseAgentContext->LeasingApplicationContextHashTable,
            TRUE);
        NULL != IterLeasingApplication && !FoundLeasingApplication;
        IterLeasingApplication = RtlEnumerateGenericTable(
            &LeaseAgentContext->LeasingApplicationContextHashTable,
            FALSE)) 
        {
         
            //
            // Retrieve leasing application context.
            //
            LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);

            if (LeasingApplicationContext->Instance == GetRemoteLeaseExpirationTimeInputBuffer->LeasingApplicationHandle)
            {
                //
                // Found the leasing application.
                //
                FoundLeasingApplication = TRUE;

                for (IterRemoteLeaseAgent = RtlEnumerateGenericTable(
                    &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
                    TRUE);
                NULL != IterRemoteLeaseAgent;
                IterRemoteLeaseAgent = RtlEnumerateGenericTable(
                    &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
                    FALSE)) 
                {
                    //
                    // Retrieve remote lease agent.
                    //
                    RemoteLeaseAgentContextIter = *((PREMOTE_LEASE_AGENT_CONTEXT*)IterRemoteLeaseAgent);

                    //
                    // Find the lease identifier within this remote lease agent.
                    //
                    for (IterRemoteLeaseIdentifier = RtlEnumerateGenericTable(
                        &RemoteLeaseAgentContextIter->SubjectHashTable,
                        TRUE);
                    NULL != IterRemoteLeaseIdentifier;
                    IterRemoteLeaseIdentifier = RtlEnumerateGenericTable(
                        &RemoteLeaseAgentContextIter->SubjectHashTable,
                        FALSE))
                    {
                        LeaseRelationshipIdentifier = *((PLEASE_RELATIONSHIP_IDENTIFIER*)IterRemoteLeaseIdentifier);
                        GetRemoteLeaseExpirationTimeInputBuffer->RemoteLeasingApplicationIdentifier[MAX_PATH] = L'\0';

                        if (!IsRemoteLeaseAgentInPing(RemoteLeaseAgentContextIter) &&
                            0 == wcscmp(LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
                            GetRemoteLeaseExpirationTimeInputBuffer->RemoteLeasingApplicationIdentifier))
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

    CompleteIoRequest(
        Request,
        Status,
        BytesTransferred
        );
}

VOID
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
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());
    BOOLEAN ClearAll = FALSE;
    BOOLEAN ClearAllProcess = FALSE;
    BOOLEAN Deleted = FALSE;
    //
    // Check request output size.
    //
    if (0 != OutputBufferLength)
    {
        EventWriteInvalidReq5(NULL);
        Status = STATUS_INVALID_PARAMETER;

        goto End;
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
            goto End;
        }

        LAssert(NULL != InputBuffer);
        LAssert(Length == InputBufferLength);
    }
    else
    {
        EventWriteInvalidReq1(NULL);
        Status = STATUS_INVALID_PARAMETER;

        goto End;
    }

    //
    // Cast input buffer to its type.
    //
    LeaseAgentInputBuffer = (PTRANSPORT_BEHAVIOR_BUFFER)InputBuffer;

    if (L'\0' == LeaseAgentInputBuffer->Alias[0])
    {
        EventWriteRemoveLeaseBehavior(NULL, L"(process)");
        ClearAllProcess = TRUE;
    }
    else if (L'*' == LeaseAgentInputBuffer->Alias[0])
    {
        EventWriteRemoveLeaseBehavior(NULL, L"*");
        ClearAll = TRUE;
    }

    PEPROCESS CurrentProcess = PsGetCurrentProcess();
    KIRQL OldIrql;
    KeAcquireSpinLock(&DriverContext->TransportBehaviorTableAccessSpinLock, &OldIrql);

    IterTransportBlockingBehavior = RtlEnumerateGenericTable(
        &DriverContext->TransportBehaviorTable,
        TRUE
        );

    while (NULL != IterTransportBlockingBehavior) {

        descriptor = *((PTRANSPORT_BLOCKING_TEST_DESCRIPTOR*)IterTransportBlockingBehavior);

        if (ClearAll ||
            (ClearAllProcess && CurrentProcess == descriptor->Process) ||
            (0 == wcscmp(LeaseAgentInputBuffer->Alias, descriptor->Alias)))
        {
            EventWriteRemoveLeaseBehavior(NULL, descriptor->Alias);

            Deleted = RtlDeleteElementGenericTable(
                &DriverContext->TransportBehaviorTable,
                &descriptor
                );

            LAssert(Deleted);
        }

        IterTransportBlockingBehavior = RtlEnumerateGenericTable(
            &DriverContext->TransportBehaviorTable,
            FALSE
            );
    }

    KeReleaseSpinLock(&DriverContext->TransportBehaviorTableAccessSpinLock, OldIrql);

End:
    //
    // Complete request.
    //
    CompleteIoRequest(Request, Status, 0);
}

VOID
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
    BOOLEAN Inserted;

    PTRANSPORT_BEHAVIOR_BUFFER LeaseAgentInputBuffer = NULL;

    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());

    //
    // Check request output size.
    //
    if (0 != OutputBufferLength)
    {
        EventWriteInvalidReq5(NULL);
        Status = STATUS_INVALID_PARAMETER;

        goto End;
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
            goto End;
        }

        LAssert(NULL != InputBuffer);
        LAssert(Length == InputBufferLength);
    }
    else
    {
        EventWriteInvalidReq1(NULL);
        Status = STATUS_INVALID_PARAMETER;

        goto End;
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
        LeaseAgentInputBuffer->Alias,
        (UINT64)PsGetCurrentProcessId());

    PTRANSPORT_BLOCKING_TEST_DESCRIPTOR desc = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(TRANSPORT_BLOCKING_TEST_DESCRIPTOR),
        LEASE_MESSAGE_TAG
        );

    LAssert(NULL != desc);
    
    desc->FromAny = LeaseAgentInputBuffer->FromAny;
    desc->ToAny = LeaseAgentInputBuffer->ToAny;
    desc->LocalSocketAddress = LeaseAgentInputBuffer->LocalSocketAddress;
    desc->RemoteSocketAddress = LeaseAgentInputBuffer->RemoteSocketAddress;
    desc->BlockingType = LeaseAgentInputBuffer->BlockingType;
    
    Status = RtlStringCbCopyW(
        (NTSTRSAFE_PWSTR)&desc->Alias,
        TEST_TRANSPORT_ALIAS_LENGTH,
        (NTSTRSAFE_PWSTR)&LeaseAgentInputBuffer->Alias
        );
    desc->Process = PsGetCurrentProcess();
    LAssert(STATUS_SUCCESS == Status);

  
    desc->Instance = GetInstance();

    KIRQL OldIrql;
    KeAcquireSpinLock(&DriverContext->TransportBehaviorTableAccessSpinLock, &OldIrql);
    RtlInsertElementGenericTable(
        &DriverContext->TransportBehaviorTable,
        &desc,
        sizeof(TRANSPORT_BLOCKING_TEST_DESCRIPTOR),
        &Inserted
        );

    KeReleaseSpinLock(&DriverContext->TransportBehaviorTableAccessSpinLock, OldIrql);
    LAssert(Inserted);

End:
    //
    // Complete request.
    //
    CompleteIoRequest(Request, Status, 0);
}


UpdateIoctlRequestTime(ULONG IoControlCode)
{
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());

    ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    GetCurrentTime(&DriverContext->LastHeartbeatUpdateTime);

    DriverContext->LastHeartbeatErrorCode = IoControlCode;

    ExReleaseFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);
}

VOID
CreateLeasingApplication(
    __in WDFREQUEST Request,
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
    PVOID IterLeaseAgent = NULL;
    ULONG BytesTransferred = 0;
    size_t Length = 0;
    BOOLEAN Inserted = FALSE;
    BOOLEAN Deleted = FALSE;
    KIRQL OldIrql;

    PLEASE_AGENT_CONTEXT LeaseAgentContext = NULL;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    PLEASING_APPLICATION_CONTEXT* LeasingApplicationContextExistent = NULL;
    PCREATE_LEASING_APPLICATION_INPUT_BUFFER_DEVICE_IOCTL CreateLeasingApplicationInputBuffer = NULL;
    PLEASING_APPLICATION_BUFFER_DEVICE_IOCTL LeasingApplicationOutputBuffer = NULL;
    PEPROCESS UserModeCallingProcessHandle = NULL;

    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());

    //
    // Retrieve the calling process handle.
    //
    UserModeCallingProcessHandle = PsGetCurrentProcess();
    LAssert(NULL != UserModeCallingProcessHandle);

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

    ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    //
    // Look up the lease agent in the hash table.
    //
    for (IterLeaseAgent = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable, 
            TRUE);
         NULL != IterLeaseAgent;
         IterLeaseAgent = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable, 
            FALSE)) {

        //
        // Retrieve lease agent context.
        //
        LeaseAgentContext = *((PLEASE_AGENT_CONTEXT*)IterLeaseAgent);

        KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

        //
        // Check the lease agent handle against the lease agent address.
        //
        if (LeaseAgentContext == CreateLeasingApplicationInputBuffer->LeaseAgentHandle) {

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

            //
            // Attempt to add the leasing application.
            //
            LeasingApplicationContextExistent = RtlInsertElementGenericTable(
                &LeaseAgentContext->LeasingApplicationContextHashTable,
                &LeasingApplicationContext,
                sizeof(PLEASING_APPLICATION_CONTEXT),
                &Inserted
                );
            
            //
            // Check insert result.
            //
            if (NULL == LeasingApplicationContextExistent) {
            
                LAssert(!Inserted);
            
                 EventWriteLeaseAppOnLeaseAgent1(
                    NULL,
                    LeasingApplicationContext->LeasingApplicationIdentifier,
                    TransportIdentifier(LeaseAgentContext->Transport),
                    LeaseAgentContext->Instance.QuadPart,
                    NULL
                    ); 
                //
                // Deallocate leasing application.
                //
                LeasingApplicationDestructor(LeasingApplicationContext);

                //
                // Hash table memory allocation failed.
                //
                Status = STATUS_INSUFFICIENT_RESOURCES;
                
            } else if (*LeasingApplicationContextExistent != LeasingApplicationContext) {

            
                LAssert(!Inserted);
            
                 EventWriteLeaseAppOnLeaseAgent2(
                    NULL,
                    (*LeasingApplicationContextExistent)->LeasingApplicationIdentifier,
                    TransportIdentifier(LeaseAgentContext->Transport),
                    LeaseAgentContext->Instance.QuadPart,
                    NULL
                    ); 

                //
                // Deallocate leasing application.
                //
                LeasingApplicationDestructor(LeasingApplicationContext);
                
                //
                // The leasing application already exists.
                //
                Status = STATUS_OBJECTID_EXISTS;
                
            } else {

                LAssert(Inserted);

                //
                // Pre-allocate all leasing application event callback.
                //
                if (EnqueueLeasingApplicationAllLeaseEvents(LeasingApplicationContext))
                {
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
                    // Populate the output buffer with new leasing application handle.
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
                else
                {
                    Deleted = RtlDeleteElementGenericTable(
                        &LeaseAgentContext->LeasingApplicationContextHashTable,
                        &LeasingApplicationContext
                        );
                    LAssert(Deleted);

                    //
                    // Deallocate leasing application.
                    //
                    LeasingApplicationDestructor(LeasingApplicationContext);
                    
                    //
                    // Pre-allocation events failed.
                    //
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                }
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
    if (NULL == IterLeaseAgent) {

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

    //
    // Complete the request.
    //
    CompleteIoRequest(
        Request,
        Status,
        BytesTransferred
        );
}

VOID
RegisterLeasingApplication(
    __in WDFREQUEST Request,
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
    PVOID IterLeasingApplication = NULL;
    size_t Length = 0;
    BOOLEAN Found = FALSE;
    BOOLEAN Delayed = FALSE;
    KIRQL OldIrql;
    
    PLEASE_AGENT_CONTEXT LeaseAgentContext = NULL;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER RegisterLeasingApplicationInputBuffer = NULL;

    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());

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
    for (IterLeaseAgent = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable, 
            TRUE);
         NULL != IterLeaseAgent && !Delayed;
         IterLeaseAgent = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable, 
            FALSE)) {

        //
        // Retrieve lease agent context.
        //
        LeaseAgentContext = *((PLEASE_AGENT_CONTEXT*)IterLeaseAgent);

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
            for (IterLeasingApplication = RtlEnumerateGenericTable(
                    &LeaseAgentContext->LeasingApplicationContextHashTable, 
                    TRUE);
                 NULL != IterLeasingApplication && !Found;
                 IterLeasingApplication = RtlEnumerateGenericTable(
                    &LeaseAgentContext->LeasingApplicationContextHashTable, 
                    FALSE)) {
            
                //
                // Retrieve leasing application context.
                //
                LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);

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
                        // Acquire the extra reference for the request. The driver owns its lifetime now.
                        //
                        WdfObjectReference(Request);
                        
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
    if (!Delayed) {

        //
        // Complete the request.
        //
        CompleteIoRequest(Request, STATUS_INVALID_PARAMETER, 0);
    }
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
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = NULL;
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
        TRUE == LeaseAgentContext->IsInDelayLeaseFailureLeaseAgentCloseTimer ||
        TRUE == LeasingApplicationContext->IsBeingUnregistered)
    {
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
    for (IterRemoteLeaseAgent = RtlEnumerateGenericTable(&LeaseAgentContext->RemoteLeaseAgentContextHashTable, TRUE);
        NULL != IterRemoteLeaseAgent;
        IterRemoteLeaseAgent = RtlEnumerateGenericTable(&LeaseAgentContext->RemoteLeaseAgentContextHashTable, FALSE))
    {
        //
        // Retrieve remote lease agent context.
        //
        RemoteLeaseAgentContext = *((PREMOTE_LEASE_AGENT_CONTEXT*)IterRemoteLeaseAgent);

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
        EventWriteGetExpireTimeGlobalLeaseExpireTimeLess(
            NULL,
            LeasingApplicationContext->LeasingApplicationIdentifier,
            Min.QuadPart,
            RequestTTL,
            LeasingApplicationContext->GlobalLeaseExpireTime.QuadPart,
            Now.QuadPart
            );

        Min = LeasingApplicationContext->GlobalLeaseExpireTime;
    }

    if (MAXLONGLONG == Min.QuadPart) 
    {
        //
        // This lease agent only has monitor relationships.
        //
        *TimeToLive = MAXLONG;

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
    PVOID IterLeasingApplication = NULL;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    BOOLEAN Compare = FALSE;

    LONG LeasingApplicationTTL;
    
    //
    // Check to see if this lease agent contains the leasing application.
    //
    for (IterLeasingApplication = RtlEnumerateGenericTable(
            &LeaseAgentContext->LeasingApplicationContextHashTable, 
            TRUE);
         NULL != IterLeasingApplication;
         IterLeasingApplication = RtlEnumerateGenericTable(
            &LeaseAgentContext->LeasingApplicationContextHashTable, 
            FALSE)) {
    
        //
        // Retrieve leasing application context.
        //
        LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);

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
                        0, // requestTTL
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

VOID
UnregisterLeasingApplication(
    __in WDFREQUEST Request,
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
    PVOID IterLeaseAgent = NULL;
    size_t Length = 0;
    BOOLEAN Found = FALSE;
    KIRQL OldIrql;

    PLEASE_AGENT_CONTEXT LeaseAgentContext = NULL;
    PUNREGISTER_LEASING_APPLICATION_INPUT_BUFFER_DEVICE_IOCTL LeasingApplicationInputBuffer = NULL;

    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());

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
    for (IterLeaseAgent = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable, 
            TRUE);
         NULL != IterLeaseAgent;
         IterLeaseAgent = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable, 
            FALSE)) {
    
        //
        // Retrieve lease agent context.
        //
        LeaseAgentContext = *((PLEASE_AGENT_CONTEXT*)IterLeaseAgent);

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
    CompleteIoRequest(Request, Status, 0);
}

VOID
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

    RemoveAllLeasingApplication(TRUE);

    //
    // Complete the request.
    //
    CompleteIoRequest(Request, Status, 0);
}

VOID
EstablishLeaseRelationship(
    __in WDFREQUEST Request,
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
    PVOID IterLeasingApplication = NULL;
    PVOID IterRemoteLeaseAgent = NULL;
    ULONG BytesTransferred = 0;
    size_t Length = 0;
    BOOLEAN InsertedRemoteLeaseAgent = FALSE;
    BOOLEAN InsertedLeaseRelationship = FALSE;
    BOOLEAN Deleted = FALSE;
    BOOLEAN Found = FALSE;
    NTSTATUS ConnectionStatus1 = STATUS_SUCCESS;
    KIRQL OldIrql;
    
    PLEASE_AGENT_CONTEXT LeaseAgentContext = NULL;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContextIter = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContextActive = NULL;

    PREMOTE_LEASE_AGENT_CONTEXT* RemoteLeaseAgentContextExistent = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER* LeaseRelationshipIdentifierExistent = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifierPending = NULL;

    PLEASE_RELATIONSHIP_IDENTIFIER* LeaseRelationshipIdentifierInTable = NULL;

    PESTABLISH_LEASE_INPUT_BUFFER_DEVICE_IOCTL EstablishLeaseInputBuffer = NULL;
    PESTABLISH_LEASE_OUTPUT_BUFFER_DEVICE_IOCTL EstablishLeaseOutputBuffer = NULL;

    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());

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
    for (IterLeaseAgent = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable, 
            TRUE);
         NULL != IterLeaseAgent && !Found;
         IterLeaseAgent = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable, 
            FALSE)) {

        //
        // Retrieve lease agent context.
        //
        LeaseAgentContext = *((PLEASE_AGENT_CONTEXT*)IterLeaseAgent);

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
        for (IterLeasingApplication = RtlEnumerateGenericTable(
                &LeaseAgentContext->LeasingApplicationContextHashTable, 
                TRUE);
             NULL != IterLeasingApplication && !Found;
             IterLeasingApplication = RtlEnumerateGenericTable(
                &LeaseAgentContext->LeasingApplicationContextHashTable, 
                FALSE)) {

            //
            // Retrieve leasing application context.
            //
            LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);

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

                //
                // Check to see if this lease agent contains already the remote lease agent.
                // If a remote lease agent of the same remote address exists and is still  
                // functional, then reuse it.
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
                    RemoteLeaseAgentContextIter = *((PREMOTE_LEASE_AGENT_CONTEXT*)IterRemoteLeaseAgent);

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
                if (NULL == IterRemoteLeaseAgent) {

                    //
                    // Check connection status for the newly created remote lease agent.
                    // If the conneciton failed, there is nothing to add.
                    //
                    if (NT_SUCCESS(ConnectionStatus1)) {
                        
                        //
                        // The remote lease agent was not found. Insert the new one in the hash table.
                        //
                        RemoteLeaseAgentContextExistent = RtlInsertElementGenericTable(
                            &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
                            &RemoteLeaseAgentContext,
                            sizeof(PREMOTE_LEASE_AGENT_CONTEXT),
                            &InsertedRemoteLeaseAgent
                            );

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
                            LAssert(*RemoteLeaseAgentContextExistent == RemoteLeaseAgentContext);

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
                                                   &RemoteLeaseAgentContext->PartnerTarget);

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
                                RemoteLeaseAgentContextActive->IsActive = FALSE;
                                SetRemoteLeaseAgentFailed(RemoteLeaseAgentContextActive);
                            }
                        }
                        
                    } else {
                    
                        EventWriteRemoteLeaseAgentConnectionFail(
                            NULL,
                            RemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                            RemoteLeaseAgentContext->Instance.QuadPart,
                            ConnectionStatus1
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
                        Status = ConnectionStatus1;
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

                if (NULL != RtlLookupElementGenericTable(
                        &RemoteLeaseAgentContext->SubjectFailedPendingHashTable, 
                        &LeaseRelationshipIdentifier
                        ) ||
                    NULL != RtlLookupElementGenericTable(
                        &RemoteLeaseAgentContext->SubjectTerminatePendingHashTable, 
                        &LeaseRelationshipIdentifier
                        )
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

                LeaseRelationshipIdentifierInTable = RtlLookupElementGenericTable(
                    &RemoteLeaseAgentContext->SubjectHashTable,
                    &LeaseRelationshipIdentifier);

                if (NULL != LeaseRelationshipIdentifierInTable)
                {
                    //
                    // Populate the output buffer with existing lease.
                    //
                    EstablishLeaseOutputBuffer->LeaseHandle = *LeaseRelationshipIdentifierInTable;

                    // If the lease relationship identifier is already in the subject pending list,
                    // Previous lease is still being established, return existing lease handle in SubjectList
                    // OnLeaseEstablish callback will be called when the RESPONSE is received.

                    LeaseRelationshipIdentifierInTable = RtlLookupElementGenericTable(
                        &RemoteLeaseAgentContext->SubjectEstablishPendingHashTable, 
                        &LeaseRelationshipIdentifier);

                    if (NULL != LeaseRelationshipIdentifierInTable)
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
                LeaseRelationshipIdentifierExistent = RtlInsertElementGenericTable(
                    &RemoteLeaseAgentContext->SubjectHashTable,
                    &LeaseRelationshipIdentifier,
                    sizeof(PLEASE_RELATIONSHIP_IDENTIFIER),
                    &InsertedLeaseRelationship
                    );
                
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
                    LeaseRelationshipIdentifierPending = AddToLeaseRelationshipIdentifierList(
                        LeasingApplicationContext,
                        LeaseRelationshipIdentifier->RemoteLeasingApplicationIdentifier,
                        &RemoteLeaseAgentContext->SubjectEstablishPendingHashTable
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
                        Deleted = RtlDeleteElementGenericTable(
                            &RemoteLeaseAgentContext->SubjectHashTable,
                            &LeaseRelationshipIdentifier
                            );
                        LAssert(Deleted);
                
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
                        Deleted = RtlDeleteElementGenericTable(
                            &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
                            &RemoteLeaseAgentContext
                            );
                        LAssert(Deleted);
                        
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
                    EstablishLease(RemoteLeaseAgentContext);
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
    
    CompleteIoRequest(
        Request,
        Status,
        BytesTransferred
        );
}

VOID
TerminateLeaseRelationship(
    __in WDFREQUEST Request,
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
    PVOID IterLeaseAgent = NULL;
    PVOID IterLeasingApplication = NULL;
    PVOID IterRemoteLeaseAgent = NULL;
    PVOID IterRemoteLeaseIdentifier = NULL;
    size_t Length = 0;
    BOOLEAN FoundLeaseRelationshipIdentifier = FALSE;
    KIRQL OldIrql;
    
    PLEASE_AGENT_CONTEXT LeaseAgentContext = NULL;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = NULL;
    PLEASE_RELATIONSHIP_IDENTIFIER LeaseRelationshipIdentifier = NULL;

    PTERMINATE_LEASE_BUFFER_DEVICE_IOCTL LeaseInputBuffer = NULL;
    
    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());

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

        EventWriteLeaseDriverTextTraceInfo(NULL, L"Lease handle is null");
    
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
    for (IterLeaseAgent = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable, 
            TRUE);
         NULL != IterLeaseAgent && !FoundLeaseRelationshipIdentifier;
         IterLeaseAgent = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable, 
            FALSE)) {
    
        //
        // Retrieve lease agent context.
        //
        LeaseAgentContext = *((PLEASE_AGENT_CONTEXT*)IterLeaseAgent);

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
        for (IterLeasingApplication = RtlEnumerateGenericTable(
            &LeaseAgentContext->LeasingApplicationContextHashTable,
            TRUE);
            NULL != IterLeasingApplication && !FoundLeaseRelationshipIdentifier;
            IterLeasingApplication = RtlEnumerateGenericTable(
                &LeaseAgentContext->LeasingApplicationContextHashTable,
                FALSE))
        {
            LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);

            if (LeasingApplicationContext->Instance == LeaseInputBuffer->LeasingApplicationHandle)
            {
                //
                // Find the remote lease agent that will initiate the lease termination.
                //
                for (IterRemoteLeaseAgent = RtlEnumerateGenericTable(
                    &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
                    TRUE);
                    NULL != IterRemoteLeaseAgent && !FoundLeaseRelationshipIdentifier;
                    IterRemoteLeaseAgent = RtlEnumerateGenericTable(
                        &LeaseAgentContext->RemoteLeaseAgentContextHashTable,
                        FALSE)) {

                    //
                    // Retrieve remote lease agent context.
                    //
                    RemoteLeaseAgentContext = *((PREMOTE_LEASE_AGENT_CONTEXT*)IterRemoteLeaseAgent);

                    //
                    // Find the lease identifier within this remote lease agent.
                    //
                    for (IterRemoteLeaseIdentifier = RtlEnumerateGenericTable(
                        &RemoteLeaseAgentContext->SubjectHashTable,
                        TRUE);
                        NULL != IterRemoteLeaseIdentifier && !FoundLeaseRelationshipIdentifier;
                        IterRemoteLeaseIdentifier = RtlEnumerateGenericTable(
                            &RemoteLeaseAgentContext->SubjectHashTable,
                            FALSE)) {

                        //
                        // Retrieve the lease identifier.
                        //
                        LeaseRelationshipIdentifier = *((PLEASE_RELATIONSHIP_IDENTIFIER*)IterRemoteLeaseIdentifier);

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

                                EventWriteLeaseDriverTextTraceInfo(NULL, L"Remote lease agent failed or suspended");

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
    CompleteIoRequest(Request, Status, 0);
}

VOID
GetLeasingApplicationExpirationTime(
    __in WDFREQUEST Request,
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
    PVOID IterLeasingApplication = NULL;
    ULONG BytesTransferred = 0;
    size_t Length = 0;
    LARGE_INTEGER Now;
    BOOLEAN Found = FALSE;
    KIRQL OldIrql;

    PLEASE_AGENT_CONTEXT LeaseAgentContext = NULL;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    PGET_LEASING_APPLICATION_EXPIRATION_INPUT_BUFFER GetLeasingApplicationExpirationTimeInputBuffer = NULL;
    PGET_LEASING_APPLICATION_EXPIRATION_OUTPUT_BUFFER GetLeasingApplicationExpirationTimeOutputBuffer = NULL;

    LONG InputRequestTTL = 0;

    //
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());

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
    for (IterLeaseAgent = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable, 
            TRUE);
         NULL != IterLeaseAgent && !Found;
         IterLeaseAgent = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable, 
            FALSE)) {

        //
        // Retrieve lease agent context.
        //
        LeaseAgentContext = *((PLEASE_AGENT_CONTEXT*)IterLeaseAgent);

        KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

        //
        // Check to see if this lease agent contains the leasing application.
        //
        for (IterLeasingApplication = RtlEnumerateGenericTable(
                &LeaseAgentContext->LeasingApplicationContextHashTable, 
                TRUE);
             NULL != IterLeasingApplication && !Found;
             IterLeasingApplication = RtlEnumerateGenericTable(
                &LeaseAgentContext->LeasingApplicationContextHashTable, 
                FALSE)) {
        
            //
            // Retrieve leasing application context.
            //
            LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);

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
    CompleteIoRequest(
        Request,
        Status,
        BytesTransferred
        );
}

VOID
SetGlobalLeaseExpirationTime(
    __in WDFREQUEST Request,
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

    PLEASE_AGENT_CONTEXT LeaseAgentContext = NULL;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    PGLOBAL_LEASE_EXPIRATION_INPUT_BUFFER SetLeaseExpirationTimeInputBuffer = NULL;

    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());

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
    for (IterLeaseAgent = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable, 
            TRUE);
         NULL != IterLeaseAgent && !Found;
         IterLeaseAgent = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable, 
            FALSE)) {

        //
        // Retrieve lease agent context.
        //
        LeaseAgentContext = *((PLEASE_AGENT_CONTEXT*)IterLeaseAgent);

        KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

        //
        // Check to see if this lease agent contains the leasing application.
        //
        for (IterLeasingApplication = RtlEnumerateGenericTable(
                &LeaseAgentContext->LeasingApplicationContextHashTable, 
                TRUE);
             NULL != IterLeasingApplication && !Found;
             IterLeasingApplication = RtlEnumerateGenericTable(
                &LeaseAgentContext->LeasingApplicationContextHashTable, 
                FALSE)) {
        
            //
            // Retrieve leasing application context.
            //
            LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);
        
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
    CompleteIoRequest(Request, Status, 0);
}

VOID
ProcessArbitrationResult(
    __in WDFREQUEST Request,
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

    PLEASE_AGENT_CONTEXT LeaseAgentContext = NULL;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContext = NULL;
    PREMOTE_LEASE_AGENT_CONTEXT RemoteLeaseAgentContextIter = NULL;
    PARBITRATION_RESULT_INPUT_BUFFER ArbitrationResultInputBuffer = NULL;

    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());

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
    for (IterLeaseAgent = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable, 
            TRUE);
         NULL != IterLeaseAgent && !Found;
         IterLeaseAgent = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable, 
            FALSE)) {
        //
        // Retrieve lease agent context.
        //
        LeaseAgentContext = *((PLEASE_AGENT_CONTEXT*)IterLeaseAgent);

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
        for (IterLeasingApplication = RtlEnumerateGenericTable(
                &LeaseAgentContext->LeasingApplicationContextHashTable, 
                TRUE);
             NULL != IterLeasingApplication && !Found;
             IterLeasingApplication = RtlEnumerateGenericTable(
                &LeaseAgentContext->LeasingApplicationContextHashTable, 
                FALSE)) {
        
            //
            // Retrieve leasing application context.
            //
            LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);
        
            if (LeasingApplicationContext->Instance == ArbitrationResultInputBuffer->LeasingApplicationHandle) {

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
            RemoteLeaseAgentContextIter = *((PREMOTE_LEASE_AGENT_CONTEXT*)IterRemoteLeaseAgent);
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
    CompleteIoRequest(Request, Status, 0);
}

VOID
ProcessRemoteCertVerifyResult(
    __in WDFREQUEST Request,
    __in size_t OutputBufferLength,
    __in size_t InputBufferLength
    )

/*++

Routine Description:

    Processes an remote certification verify result for a given call back operation.

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
    KIRQL OldIrql;

    PLEASE_AGENT_CONTEXT LeaseAgentContext = NULL;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    PREMOTE_CERT_VERIFY_CALLBACK_OPERATION_INPUT_BUFFER RemoteCertVerifyResultInputBuffer = NULL;
    BOOLEAN Deleted = FALSE;
    BOOLEAN Inserted = FALSE;

    PLEASE_REMOTE_CERTIFICATE CertLookup = NULL;
    PLEASE_REMOTE_CERTIFICATE CertTableEntry = NULL;

    PLEASE_REMOTE_CERTIFICATE* IterCertLookupExistent = NULL;
    PLEASE_REMOTE_CERTIFICATE* IterCertInsertExistent = NULL;

    BOOLEAN CertInTrustedList = FALSE;

    //
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());
    PVOID IterLeaseAgent = NULL;
    PVOID IterLeasingApplication = NULL;
    BOOLEAN Found = FALSE;

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
    if (sizeof(REMOTE_CERT_VERIFY_CALLBACK_OPERATION_INPUT_BUFFER) == InputBufferLength) {
        
        Status = WdfRequestRetrieveInputBuffer(
            Request, 
            sizeof(REMOTE_CERT_VERIFY_CALLBACK_OPERATION_INPUT_BUFFER),
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
    RemoteCertVerifyResultInputBuffer = (PREMOTE_CERT_VERIFY_CALLBACK_OPERATION_INPUT_BUFFER)InputBuffer;

    //
    // Check input buffer - lease agent handle.
    //
    if (NULL == RemoteCertVerifyResultInputBuffer->LeasingApplicationHandle) {

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
    // Check input buffer.
    //
    if (NULL == RemoteCertVerifyResultInputBuffer->CertVerifyOperation) {

        EventWriteInvalidReq1(
            NULL
            );
        //
        // This is an invalid request.
        //
        Status = STATUS_INVALID_PARAMETER;

        goto End;
    }

    CertLookup = LeaseRemoteCertificateConstructor(0, NULL, RemoteCertVerifyResultInputBuffer->CertVerifyOperation, LeasingApplicationContext);

    ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    //
    // Look up the lease agent in the hash table.
    //
    for (IterLeaseAgent = RtlEnumerateGenericTable(&DriverContext->LeaseAgentContextHashTable, TRUE);
         NULL != IterLeaseAgent && !Found;
         IterLeaseAgent = RtlEnumerateGenericTable(&DriverContext->LeaseAgentContextHashTable, FALSE))
    {
        //
        // Retrieve lease agent context.
        //
        LeaseAgentContext = *((PLEASE_AGENT_CONTEXT*)IterLeaseAgent);

        KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

        //
        // Check to see if this lease agent contains the leasing application.
        //
        for (IterLeasingApplication = RtlEnumerateGenericTable(&LeaseAgentContext->LeasingApplicationContextHashTable, TRUE);
             NULL != IterLeasingApplication && !Found;
             IterLeasingApplication = RtlEnumerateGenericTable(&LeaseAgentContext->LeasingApplicationContextHashTable, FALSE))
        {
            //
            // Retrieve leasing application context.
            //
            LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);

            if (LeasingApplicationContext->Instance == RemoteCertVerifyResultInputBuffer->LeasingApplicationHandle &&
                FALSE == LeasingApplicationContext->IsBeingUnregistered)
            {
                Found = TRUE;
                break;
            }
        }

        if (Found)
        {
            //
            // Remove the certificate entry from the pending list.
            //
            IterCertLookupExistent = RtlLookupElementGenericTable(
                &LeaseAgentContext->CertificatePendingList,
                &CertLookup);

            //
            // The certificate context may not be added to the pending list successfully
            // Because it is added after raise the cert verify request to the user mode
            //
            if (NULL == IterCertLookupExistent)
            {
                EventWriteFailedVerifyOperationTableLookup(
                    NULL,
                    (LONGLONG)RemoteCertVerifyResultInputBuffer->CertVerifyOperation,
                    RemoteCertVerifyResultInputBuffer->VerifyResult
                    );

                Status = STATUS_INSUFFICIENT_RESOURCES;

                KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);

                break;
            }

            //
            // Complete the verify callback
            //
            TransportProcessRemoteCertVerifyResult(
                RemoteCertVerifyResultInputBuffer->CertVerifyOperation,
                RemoteCertVerifyResultInputBuffer->VerifyResult);

            CertTableEntry = *((PLEASE_REMOTE_CERTIFICATE*)IterCertLookupExistent);

            Deleted = RtlDeleteElementGenericTable(
                &LeaseAgentContext->CertificatePendingList,
                &CertTableEntry
                );
            LAssert(Deleted);

            //
            // For a positive verify, add the cert context to the trusted list in lease agent
            //
            if (NT_SUCCESS(RemoteCertVerifyResultInputBuffer->VerifyResult))
            {
                IterCertInsertExistent = RtlInsertElementGenericTable(
                    &LeaseAgentContext->TrustedCertificateCache,
                    &CertTableEntry,
                    sizeof(PLEASE_REMOTE_CERTIFICATE),
                    &Inserted
                    );

                //
                // Check insert result.
                //
                if (NULL == IterCertInsertExistent)
                {
                    LAssert(!Inserted);
                    EventWriteFailedTrustedCertListInsertion(
                        NULL,
                        (LONGLONG)RemoteCertVerifyResultInputBuffer->CertVerifyOperation,
                        RemoteCertVerifyResultInputBuffer->VerifyResult
                        );

                    Status = STATUS_INSUFFICIENT_RESOURCES;

                }
                else if (*IterCertInsertExistent != CertTableEntry)
                {
                    LAssert(!Inserted);
                    EventWriteCertExistInTrustedList(
                        NULL,
                        (LONGLONG)RemoteCertVerifyResultInputBuffer->CertVerifyOperation,
                        RemoteCertVerifyResultInputBuffer->VerifyResult
                        );

                    CertInTrustedList = TRUE;

                    //
                    // Remote cert is already in the trusted list
                    // The IO should be completed with success, but the cert need to be deallocated
                    //
                    Status = STATUS_SUCCESS;

                }
                else
                {
                    //
                    // A new cert context entry was inserted to the trusted list.
                    //
                    LAssert(Inserted);
                    EventWriteInsertCertInTrustedList(
                        NULL,
                        (LONGLONG)RemoteCertVerifyResultInputBuffer->CertVerifyOperation,
                        RemoteCertVerifyResultInputBuffer->VerifyResult
                        );

                    //
                    // Done.
                    //
                    Status = STATUS_SUCCESS;
                }
            }

            if (!NT_SUCCESS(RemoteCertVerifyResultInputBuffer->VerifyResult)
                || !NT_SUCCESS(Status)
                || TRUE == CertInTrustedList)
            {
                //
                // Destroy the cert that is deleted from pending list, but not added to the trusted list.
                //
                LeaseRemoteCertificateDestructor(CertTableEntry);
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

    if (NULL != CertLookup)
    {
        LeaseRemoteCertificateDestructor(CertLookup);
    }

    if (!NT_SUCCESS(Status))
    {
        EventWriteFailedRemoteCertVerifyResult(
            NULL,
            Status,
            (LONGLONG)RemoteCertVerifyResultInputBuffer->CertVerifyOperation,
            RemoteCertVerifyResultInputBuffer->VerifyResult);

    }
    //
    // Complete request.
    //
    CompleteIoRequest(Request, Status, 0);
}

VOID
UpdateCertificate(
    __in WDFREQUEST Request,
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
    BOOLEAN LeaseAgentCertUpdate = FALSE;

    PLEASE_AGENT_CONTEXT LeaseAgentContext = NULL;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = NULL;
    PUPDATE_CERTFICATE_INPUT_BUFFER UpdateCertInputBuffer = NULL;

    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());

    //
    // Retrieve the input buffer from the request.
    //
    if (sizeof(UPDATE_CERTFICATE_INPUT_BUFFER) == InputBufferLength) {
        
        Status = WdfRequestRetrieveInputBuffer(
            Request, 
            sizeof(UPDATE_CERTFICATE_INPUT_BUFFER),
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
    UpdateCertInputBuffer = (PUPDATE_CERTFICATE_INPUT_BUFFER)InputBuffer;

    //
    // Check input buffer - lease agent handle.
    //
    if (NULL == UpdateCertInputBuffer->LeasingApplicationHandle)
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
    for (IterLeaseAgent = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable, 
            TRUE);
         NULL != IterLeaseAgent && !Found;
         IterLeaseAgent = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable, 
            FALSE))
     {
        //
        // Retrieve lease agent context.
        //
        LeaseAgentContext = *((PLEASE_AGENT_CONTEXT*)IterLeaseAgent);

        KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

        //
        // Check to see if this lease agent contains the leasing application.
        //
        for (IterLeasingApplication = RtlEnumerateGenericTable(
                &LeaseAgentContext->LeasingApplicationContextHashTable, 
                TRUE);
             NULL != IterLeasingApplication && !Found;
             IterLeasingApplication = RtlEnumerateGenericTable(
                &LeaseAgentContext->LeasingApplicationContextHashTable, 
                FALSE))
         {
            //
            // Retrieve leasing application context.
            //
            LeasingApplicationContext = *((PLEASING_APPLICATION_CONTEXT*)IterLeasingApplication);
        
            if (LeasingApplicationContext->Instance == UpdateCertInputBuffer->LeasingApplicationHandle &&
                PsGetCurrentProcess() == LeasingApplicationContext->ProcessHandle)
            {
                // Leasing application found.
                Found = TRUE;
                break;
            }
        }

        if (Found)
        {

            if (IsSecurityProviderSsl(LeaseAgentContext->SecurityType) && memcmp(UpdateCertInputBuffer->CertHash, LeaseAgentContext->CertHash, SHA1_LENGTH) != 0)
            {
                //First update the cert hash content in lease agent
                UpdateCertInputBuffer->CertHashStoreName[SCH_CRED_MAX_STORE_NAME_SIZE - 1] = L'\0';
                LeaseAgentCertUpdate = UpdateLeaseAgentCertHash(
                    LeaseAgentContext, 
                    UpdateCertInputBuffer->CertHash, 
                    UpdateCertInputBuffer->CertHashStoreName, 
                    &certStoreNameLength);

            }
            else
            {
                // 
                // the cert hash may have already been updated by some other leasing application
                // a match is found, goto end, nothing futher need to done
                // 
                 EventWriteLeaseSecuritySettingMatch(
                    NULL,
                    LeasingApplicationContext->LeasingApplicationIdentifier,
                    TransportIdentifier(LeaseAgentContext->Transport),
                    LeaseAgentContext->Instance.QuadPart
                    );
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
    else
    {

        //
        // Set the new credential
        //
        if (TRUE == LeaseAgentCertUpdate)
        {
            Status = TransportSetCredentials(
                LeaseAgentContext->CertHash,
                LeaseAgentContext->CertHashStoreName,
                certStoreNameLength, 
                LeaseAgentContext->Transport);
                    
            if (!NT_SUCCESS(Status))
            {
                EventWriteTransportSetCredentialsFailed(
                    NULL,
                    LeasingApplicationContext->LeasingApplicationIdentifier,
                    TransportIdentifier(LeaseAgentContext->Transport),
                    LeaseAgentContext->Instance.QuadPart
                    );
            }
            else
            {
                EventWriteTransportSetCredentialsSucceeded(
                    NULL,
                    LeasingApplicationContext->LeasingApplicationIdentifier,
                    TransportIdentifier(LeaseAgentContext->Transport),
                    LeaseAgentContext->Instance.QuadPart
                    );
            }
        }
        else
        {
            EventWriteUpdateCertificateFailed(
                NULL,
                LeasingApplicationContext->LeasingApplicationIdentifier,
                TransportIdentifier(LeaseAgentContext->Transport),
                LeaseAgentContext->Instance.QuadPart
                );
        }
    }

End:

    //
    // Complete the request.
    //
    CompleteIoRequest(Request, Status, 0);
}

void RemoveAllLeasingApplication(BOOLEAN IsProcessAssert)
{
    PVOID IterLeaseAgent = NULL;
    KIRQL OldIrql;

    PLEASE_AGENT_CONTEXT LeaseAgentContext = NULL;
    PEPROCESS UserModeCallingProcessHandle = NULL;
    HANDLE UserModeCallingProcessId = NULL;

    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());

    //
    // Retrieve the calling process handle.
    //
    UserModeCallingProcessHandle = PsGetCurrentProcess();
    UserModeCallingProcessId = PsGetCurrentProcessId();
    LAssert(NULL != UserModeCallingProcessHandle);

    EventWriteProcessCleanup(
        NULL,
        (UINT64)UserModeCallingProcessId);

    ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    //
    // Look up the lease agent in the hash table that contains 
    // this leasing application.
    //
    for (IterLeaseAgent = RtlEnumerateGenericTable(
        &DriverContext->LeaseAgentContextHashTable,
        TRUE);
        NULL != IterLeaseAgent;
        IterLeaseAgent = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable,
            FALSE)) {

        //
        // Retrieve lease agent context.
        //
        LeaseAgentContext = *((PLEASE_AGENT_CONTEXT*)IterLeaseAgent);

        KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

        UpdateAssertLeasingApplication(
            LeaseAgentContext,
            UserModeCallingProcessHandle,
            IsProcessAssert);

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
LeaseLayerEvtCleanup(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    Called when a user mode process that has a handle opened to this driver
    is terminated (normally or abnormally). We use this routine to identify
    the leasing applications that might have died without unregistering.
    Then we unregister all those applications.
    
Parameters Description:

    DeviceObject - DeviceObject represents the instance that is associated with the
        incoming Irp parameter.

    Irp - represents the cleanup operation associated with DeviceObject.

Return Value Description:

    STATUS_SUCCESS.

--*/

{
    UNREFERENCED_PARAMETER(DeviceObject);

    EventWriteProcessExit(NULL);

    RemoveAllLeasingApplication(FALSE);

    //
    // Complete the request.
    //
    IoSetCancelRoutine(Irp, NULL);
    IoCompleteRequest(Irp, 0);

    return STATUS_SUCCESS;
}

VOID
LeaseLayerEvtIoDeviceControl (
    __in WDFQUEUE Queue,
    __in WDFREQUEST Request,
    __in size_t OutputBufferLength,
    __in size_t InputBufferLength,
    __in ULONG IoControlCode
    )
    
/*++

Routine Description:

    Performs device I/O to the Lease Layer device. 

Arguments:

    Queue - Handle to the framework queue object that is associated with the
        I/O request.
        
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
    UNREFERENCED_PARAMETER(Queue);
    
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

        CreateLeaseAgent(
            Request,
            OutputBufferLength,
            InputBufferLength
            );
        
        break;

    //
    // Close an existent lease agent.
    //
    case IOCTL_LEASING_CLOSE_LEASE_AGENT:

        CloseLeaseAgent(
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

        CloseLeaseAgent(
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

        CreateLeasingApplication(
            Request,
            OutputBufferLength,
            InputBufferLength
            );

        break;

    //
    // Register leasing application.
    //
    case IOCTL_REGISTER_LEASING_APPLICATION:

        RegisterLeasingApplication(
            Request,
            OutputBufferLength,
            InputBufferLength
            );
        
        break;

    //
    // Unregister leasing application.
    //
    case IOCTL_UNREGISTER_LEASING_APPLICATION:
        
        UnregisterLeasingApplication(
            Request,
            OutputBufferLength,
            InputBufferLength
            );
        
        break;

    //
    // Establish a lease.
    //
    case IOCTL_ESTABLISH_LEASE:

        EstablishLeaseRelationship(
            Request,
            OutputBufferLength,
            InputBufferLength
            );
        
        break;

    //
    // Terminate a lease.
    //
    case IOCTL_TERMINATE_LEASE:

        TerminateLeaseRelationship(
            Request,
            OutputBufferLength,
            InputBufferLength
            );
        
        break;

    //
    // Return application expiration time.
    //
    case IOCTL_GET_LEASING_APPLICATION_EXPIRATION_TIME:

        GetLeasingApplicationExpirationTime(
            Request,
            OutputBufferLength,
            InputBufferLength
            );

        break;

    //
    // Set global lease expiration time.
    //

    case IOCTL_SET_GLOBAL_LEASE_EXPIRATION_TIME:

        SetGlobalLeaseExpirationTime(
            Request,
            OutputBufferLength,
            InputBufferLength
            );

        break;

    //
    // Process arbitration result.
    //
    case IOCTL_ARBITRATION_RESULT:

        ProcessArbitrationResult(
            Request,
            OutputBufferLength,
            InputBufferLength
            );

        break;

    //
    // Process remote cert verify result.
    //
    case IOCTL_REMOTE_CERT_VERIFY_RESULT:

        ProcessRemoteCertVerifyResult(
            Request,
            OutputBufferLength,
            InputBufferLength
            );

        break;


    //
    // Update the certificate.
    //
    case IOCTL_UPDATE_CERTIFICATE:

        UpdateCertificate(
            Request,
            OutputBufferLength,
            InputBufferLength
            );

        break;

    //
    // Block a connection by dropping all messages.
    //
    case IOCTL_BLOCK_LEASE_CONNECTION:

        BlockLeaseConnection(
            Request,
            OutputBufferLength,
            InputBufferLength
            );

        break;

    // Set security descriptor in the listener.
    case IOCTL_SET_SECURITY_DESCRIPTOR:

        SetListenerSecurityDescriptor(
            Request,
            OutputBufferLength,
            InputBufferLength
            );

        break;

    // Update lease duration from config update
    case IOCTL_UPDATE_LEASE_DURATION:

        UpdateConfigLeaseDuration(
            Request,
            OutputBufferLength,
            InputBufferLength
            );

        break;

    case IOCTL_QUERY_LEASE_DURATION:

        QueryLeaseDuration(
            Request,
            OutputBufferLength,
            InputBufferLength
            );

        break;


    case IOCTL_GET_REMOTE_LEASE_EXPIRATION_TIME:

        GetRemoteLeaseExpirationTime(
            Request,
            OutputBufferLength,
            InputBufferLength
            );

        break;

    case IOCTL_ADD_TRANSPORT_BEHAVIOR:

        addLeaseBehavior(
         Request,
            OutputBufferLength,
            InputBufferLength
            );
        break;


    case IOCTL_HEARTBEAT:

        Heartbeat(
            Request,
            OutputBufferLength,
            InputBufferLength
            );

        break;

    case IOCTL_CLEAR_TRANSPORT_BEHAVIOR:

        removeLeaseBehavior(
            Request,
            OutputBufferLength,
            InputBufferLength
            );

        break;

    case IOCTL_CRASH_LEASING_APPLICATION:

        CrashLeasingApplication(
            Request,
            OutputBufferLength,
            InputBufferLength
            );

        break;

    case IOCTL_UPDATE_LEASE_GLOBAL_CONFIG:

        UpdateLeaseGlobalConfig(
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
        CompleteIoRequest(Request, STATUS_INVALID_DEVICE_REQUEST, 0);

        break;
    }

    EventWriteApiEnd(
        NULL,
        GetLeaseIoCode(IoControlCode)
    );
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

    case IOCTL_HEARTBEAT:
        IoCodeString = L"HEARTBEAT";
        break;

    case IOCTL_CLEAR_TRANSPORT_BEHAVIOR:
        IoCodeString = L"CLEAR_TRANSPORT_BEHAVIOR";
        break;

    case IOCTL_CRASH_LEASING_APPLICATION:
        IoCodeString = L"CRASH_LEASING_APPLICATION";
        break;

    case IOCTL_UPDATE_LEASE_GLOBAL_CONFIG:
        IoCodeString = L"UPDATE_LEASE_GLOBAL_CONFIG";
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

VOID CheckHeartbeatUnresponsiveTime(PDRIVER_CONTEXT DriverContext, PREMOTE_LEASE_AGENT_CONTEXT ActiveRemoteLeaseAgentContext, LARGE_INTEGER Now)
{
    LONG UpdateTimePassedMilliseconds = 0;

    if (NULL == ActiveRemoteLeaseAgentContext)
    {
        // Reset the last Update time for heartbeat to 0 if there is no active remote lease agent.
        DriverContext->LastHeartbeatUpdateTime.QuadPart = 0;
    }
    else
    {
        // Check if ioctl update time is up to date if there is at least one heartbeat
        if ((DriverContext->UnresponsiveDuration > 0) &&
            (DriverContext->LastHeartbeatUpdateTime.QuadPart != 0) &&
            (IS_LARGE_INTEGER_GREATER_THAN_LARGE_INTEGER(Now, DriverContext->LastHeartbeatUpdateTime)))
        {
            //
            // Compute time passed.
            //
            UpdateTimePassedMilliseconds = (LONG)((Now.QuadPart - DriverContext->LastHeartbeatUpdateTime.QuadPart) / MILLISECOND_TO_NANOSECOND_FACTOR);

            EventWriteUnresponsiveDurationCheck(
                NULL,
                ActiveRemoteLeaseAgentContext->RemoteLeaseAgentIdentifier,
                ActiveRemoteLeaseAgentContext->Instance.QuadPart,
                UpdateTimePassedMilliseconds,
                DriverContext->LastHeartbeatErrorCode,
                DriverContext->UnresponsiveDuration);

            if (UpdateTimePassedMilliseconds > DriverContext->UnresponsiveDuration)
            {
                // There is no ioctl update for at least UnresponsiveDuration
                EventWriteLeaseDriverTextTraceError(
                    NULL,
                    L"There is no heartbeat update for ",
                    UpdateTimePassedMilliseconds);

                // If there is no update, then either it is timeout or we have some error code.
                // winerror ERROR_SUCCESS
                if (0 == DriverContext->LastHeartbeatErrorCode)
                {
                    // winerror WAIT_TIMEOUT
                    DriverContext->LastHeartbeatErrorCode = 258L;
                }

                KeBugCheckEx(
                    LEASLAYR_BUGCHECK_CODE,
                    DriverContext->LastHeartbeatErrorCode,
                    (ULONG_PTR)__FILE__,
                    (ULONG_PTR)__LINE__,
                    0);
            }
        }
    }
}

VOID
DoMaintenance(
    __in PDRIVER_CONTEXT DriverContext
    )
{
    PVOID IterLeaseAgent = NULL;
    PLEASE_AGENT_CONTEXT LeaseAgentContext = NULL;
    BOOLEAN Deleted = FALSE;
    BOOLEAN RestartLeaseAgent = TRUE;
    KIRQL OldIrql;
    PREMOTE_LEASE_AGENT_CONTEXT ActiveRemoteLeaseAgentContext = NULL;

    LARGE_INTEGER Now;
    GetCurrentTime(&Now);

    EventWritePerformMaintenance(NULL);

    //
    // Run cleanup of lease agents and remote lease agents.
    //
    ExAcquireFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);

    //
    // Look up the failed lease agents and remote lease agents.
    //
    IterLeaseAgent = RtlEnumerateGenericTable(
        &DriverContext->LeaseAgentContextHashTable,
        TRUE
        );

    while (NULL != IterLeaseAgent)
    {
        //
        // Retrieve lease agent context.
        //
        LeaseAgentContext = *((PLEASE_AGENT_CONTEXT*)IterLeaseAgent);

        KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

        //
        // Deallocate unregistered leasing application from this lease agent, if any.
        //
        ClearLeasingApplicationList(&LeaseAgentContext->LeasingApplicationContextUnregisterList);
        //
        // Check if any leasing application not exited in time
        //
        CheckHangLeasingApplication(&LeaseAgentContext->LeasingApplicationContextUnregisterList);
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
        if (!IsLeaseAgentFailed(LeaseAgentContext))
        {
            if (CanLeaseAgentBeFailed(LeaseAgentContext))
            {
                if (0 == LeaseAgentContext->TimeToBeFailed.QuadPart)
                {
                    LeaseAgentContext->TimeToBeFailed.QuadPart = Now.QuadPart + (LONGLONG) (MAINTENANCE_INTERVAL - 1) * 1000 * MILLISECOND_TO_NANOSECOND_FACTOR;
                }
                else if (LeaseAgentContext->TimeToBeFailed.QuadPart < Now.QuadPart)
                {
                    SetLeaseAgentFailed(LeaseAgentContext, FALSE);
                }
            }
            else
            {
                if (0 != LeaseAgentContext->TimeToBeFailed.QuadPart)
                {
                    // reset fail time in case previous maintenance already change it
                    LeaseAgentContext->TimeToBeFailed.QuadPart = 0;
                }

                if (NULL == ActiveRemoteLeaseAgentContext)
                {
                    ActiveRemoteLeaseAgentContext = GetActiveRemoteLeaseAgent(LeaseAgentContext);
                }
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

            EventWriteLeaseAgentCleanup(
                NULL,
                TransportIdentifier(LeaseAgentContext->Transport),
                LeaseAgentContext->Instance.QuadPart
                );

            //
            // Remote remote lease agent from the lease agent hash table.
            //
            Deleted = RtlDeleteElementGenericTable(
                &DriverContext->LeaseAgentContextHashTable,
                &LeaseAgentContext
                );
            LAssert(Deleted);
        }    

        KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);

        if (RestartLeaseAgent) {

            //
            // Deallocate lease agent.
            //
            LeaseAgentDestructor(LeaseAgentContext);
        }

        //
        // Move on to the next element.
        //
        IterLeaseAgent = RtlEnumerateGenericTable(
            &DriverContext->LeaseAgentContextHashTable,
            RestartLeaseAgent
            );
    }

    CheckHeartbeatUnresponsiveTime(DriverContext, ActiveRemoteLeaseAgentContext, Now);

    ExReleaseFastMutex(&DriverContext->LeaseAgentContextHashTableAccessMutex);
}

VOID
MaintenanceWorkerThread(
    __in PVOID Context
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
    PWORKER_THREAD_CONTEXT WorkerThreadContext = (PWORKER_THREAD_CONTEXT)Context;

    // 
    // Retrieve driver context area.
    //
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());

    //Timeout.QuadPart = -MAINTENANCE_INTERVAL * 1000 * MILLISECOND_TO_NANOSECOND_FACTOR;

    EventWriteWorkerThread1(
        NULL
        ); 

    while (!StopThread) {

        //
        // This thread will wake up every 15s to perform cleanup.
        //
        Timeout.QuadPart = -DriverContext->MaintenanceInterval * MILLISECOND_TO_NANOSECOND_FACTOR;

        //
        // Wait for the shutdown event to be signaled.
        //
        Status = KeWaitForSingleObject(
            &WorkerThreadContext->Shutdown,
            Executive,
            KernelMode,
            FALSE,
            &Timeout
            );

        if (STATUS_SUCCESS == Status) {

            EventWriteWorkerThread2(
                NULL
                );

            //
            // The driver is unloading.
            //
            StopThread = TRUE;
            
        } else {
            DoMaintenance(DriverContext);
        }

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
    PDRIVER_CONTEXT DriverContext = LeaseLayerDriverGetContext(WdfGetDriver());
    KIRQL OldIrql;
    LARGE_INTEGER instance;

    KeQuerySystemTime(&instance);

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
