// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "LeasLayr.h"
#include "LeaseApp.tmh"

PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER
LeaseEventConstructor(
    VOID
    )
    
/*++

Routine Description:

    Constructs a new lease event.

Arguments:

    n/a
    
Return Value:

    NULL if the new lease event cannot be allocated and initilized.

--*/

{
    PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER LeaseEvent = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(LEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER),
        LEASE_EVENT_TAG
        );
    
    if (NULL == LeaseEvent) {
        
        EventWriteAllocateFail24(
            NULL
            );
        goto Error;
    }

    RtlZeroMemory(LeaseEvent, sizeof(LEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER));

    GetCurrentTime(&LeaseEvent->insertionTime);

    return LeaseEvent;

Error:

    return NULL;
}

VOID
LeaseEventDestructor(
    PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER LeaseEvent
    )
    
/*++

Routine Description:

    Deallocates an existent lease event.

Arguments:

    LeaseEvent - lease event to be deallocated.
    
Return Value:

    n/a.

--*/

{
    ExFreePool(LeaseEvent);
}



KDEFERRED_ROUTINE UnregisterExpireTimer;

VOID
UnregisterExpireTimer(
    __in PKDPC Dpc,
    __in_opt PVOID DeferredContext,
    __in_opt PVOID SystemArgument1,
    __in_opt PVOID SystemArgument2
)

/*++

Routine Description:

    Leasing application unregister timer.

Parameters Description:

    Dpc - unregister timer Dpc routine.

    DeferredContext - Leasing application context.

    SystemArgument1 - n/a

    SystemArgument2 - n/a

Return Value:

    n/a

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    KIRQL OldIrql;

    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = (PLEASING_APPLICATION_CONTEXT)DeferredContext;
    PLEASE_AGENT_CONTEXT LeaseAgentContext;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    if (NULL == LeasingApplicationContext)
    {
        return;
    }

    EventWriteUnregisterTimerDPC(
        NULL,
        LeasingApplicationContext->LeasingApplicationIdentifier,
        Status
        );

    LeaseAgentContext = LeasingApplicationContext->LeaseAgentContext;
    LAssert(LeaseAgentContext);

    KeAcquireSpinLock(&LeaseAgentContext->Lock, &OldIrql);

    //
    // Skip over already failed lease agents.
    //
    if (!IsLeaseAgentFailed(LeaseAgentContext))
    {
        //
        // Take this application out of this lease agent.
        //
        UnregisterApplication(
            LeaseAgentContext,
            LeasingApplicationContext,
            TRUE
            );
    }

    MoveToUnregisterList(
        LeasingApplicationContext
        );

    ClearCertificateList(&LeaseAgentContext->CertificatePendingList, TRUE, LeasingApplicationContext);

    KeReleaseSpinLock(&LeaseAgentContext->Lock, OldIrql);
}

PLEASING_APPLICATION_CONTEXT
LeasingApplicationConstructor(
    __in LPCWSTR LeasingApplicationIdentifier,
    __in_opt PEPROCESS ProcessHandle
    )
    
/*++

Routine Description:

    Constructs a new leasing application.

Arguments:

    LeasingApplicationIdentifier - the leasing application identifier for the new 
        aplication.

    ProcessHandle - the process handle requesting the application to be created.

Return Value:

    NULL if the new leasing application context cannot be allocated and initilized.

--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    size_t WcharLength = 0;
    size_t AppIdLength = 0;
    size_t AppIdSize = 0;
    PLEASING_APPLICATION_CONTEXT LeasingApplicationContext = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(LEASING_APPLICATION_CONTEXT),
        LEASE_APPLICATION_CTXT_TAG
        );
    
    if (NULL == LeasingApplicationContext) {
        
        EventWriteAllocateFail4(
            NULL
            );
        goto Error;
    }

    RtlZeroMemory(LeasingApplicationContext, sizeof(LEASING_APPLICATION_CONTEXT));

    //
    // Populate leasing application context.
    //
    Status = RtlStringCchLengthW(
        LeasingApplicationIdentifier,
        MAX_PATH + 1,
        &WcharLength
        );

    if (!NT_SUCCESS(Status))
    {
        EventWriteLeaseDriverTextTraceError(NULL, L"Str length check failed with ", Status);
        goto Error;
    }

    Status = RtlULongLongAdd(
        WcharLength,
        1,
        &AppIdLength);

    if (!NT_SUCCESS(Status))
    {
        EventWriteLeaseDriverTextTraceError(
            NULL,
            L"AppIdSize RtlULongLongAdd failed in LeasingApplicationConstructor",
            Status);

        goto Error;
    }

    Status = RtlULongLongMult(
        AppIdLength,
        sizeof(WCHAR),
        &AppIdSize);

    if (!NT_SUCCESS(Status))
    {
        EventWriteLeaseDriverTextTraceError(
            NULL,
            L"AppIdSize RtlULongLongMult failed in LeasingApplicationConstructor",
            Status);

        goto Error;
    }

    LeasingApplicationContext->LeasingApplicationIdentifier = ExAllocatePoolWithTag(
        NonPagedPool,
        AppIdSize,
        LEASE_APPLICATION_ID_TAG
        );

    LeasingApplicationContext->Instance = (HANDLE)GetInstance().QuadPart;

    if (NULL == LeasingApplicationContext->LeasingApplicationIdentifier) {

        EventWriteAllocateFail5(
            NULL
            );
        goto Error;
    }

    RtlZeroMemory(
        LeasingApplicationContext->LeasingApplicationIdentifier, 
        (WcharLength + 1) * sizeof(WCHAR)
        );

    Status = RtlStringCchCopyW(
        LeasingApplicationContext->LeasingApplicationIdentifier,
        WcharLength + 1,
        LeasingApplicationIdentifier
        );

    if (!NT_SUCCESS(Status))
    {
        EventWriteLeaseDriverTextTraceError(NULL, L"Str copy failed with ", Status);
        goto Error;
    }

    //
    // Used during serialization.
    //
    LeasingApplicationContext->LeasingApplicationIdentifierByteCount = 
        (ULONG)(WcharLength + 1) * sizeof(WCHAR);

    //
    // Set process handle.
    //
    LeasingApplicationContext->ProcessHandle = ProcessHandle;

    LeasingApplicationContext->GlobalLeaseExpireTime.QuadPart = MAXLONGLONG;

    LeasingApplicationContext->LastGrantExpireTime.QuadPart = MAXLONGLONG;

    //
    // Initialize hash tables.
    //
    RtlInitializeGenericTable(
            &LeasingApplicationContext->LeasingApplicationEventQueue,
            (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareLeaseEvents,
            (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
            (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
            NULL
            );

    //
    // Initialize Unregister timer.
    //
    KeInitializeTimer(&LeasingApplicationContext->UnregisterTimer);
    //
    // Initialize the Unregister timer deferred procedure call.
    //
    KeInitializeDpc(
        &LeasingApplicationContext->UnregisterTimerDpc,
        UnregisterExpireTimer,
        LeasingApplicationContext
        );

    LeasingApplicationContext->IsBeingUnregistered = FALSE;

    return LeasingApplicationContext;

Error:
    
    //
    // Perform cleanup if any.
    //
    if (NULL != LeasingApplicationContext)
    {
        if (NULL != LeasingApplicationContext->LeasingApplicationIdentifier)
        {
            ExFreePool(LeasingApplicationContext->LeasingApplicationIdentifier);
        }
        ExFreePool(LeasingApplicationContext);
    }
    
    return NULL;
}

PLEASE_REMOTE_CERTIFICATE 
LeaseRemoteCertificateConstructor(
    __in ULONG cbCert,
    __in PBYTE pbCert,
    __in PVOID verifyOp,
    __in PLEASING_APPLICATION_CONTEXT leasingAppHandle
    )
    
/*++

Routine Description:

    Construct an lease remote certificate struct.

Arguments:

Return Value:

    n/a.

--*/

{
    PLEASE_REMOTE_CERTIFICATE LeaseRemoteCert = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(LEASE_REMOTE_CERTIFICATE),
        LEASELAYER_REMOTE_CERT_CONTEXT
        );
    
    if (NULL == LeaseRemoteCert)
    {
        EventWriteAllocateFailLeaseRemoteCert( NULL );
        goto Error;
    }

    RtlZeroMemory(LeaseRemoteCert, sizeof(LEASE_REMOTE_CERTIFICATE));

    LeaseRemoteCert->cbCertificate = cbCert;
    LeaseRemoteCert->pbCertificate = pbCert;
    LeaseRemoteCert->verifyOperation = verifyOp;
    LeaseRemoteCert->certLeasingAppHandle = leasingAppHandle;

    return LeaseRemoteCert;

Error:
    //
    // Perform cleanup if any.
    //
    if (NULL != LeaseRemoteCert)
    {
        ExFreePool(LeaseRemoteCert);
    }
    
    return NULL;
}

PTRANSPORT_LISTEN_ENDPOINT 
LeaseListenEndpointConstructor( )
/*++
Routine Description:
    Allocate memory for a listen endpoint struct.
--*/
{
    PTRANSPORT_LISTEN_ENDPOINT ListenEndPoint = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(TRANSPORT_LISTEN_ENDPOINT),
        LEASE_LISTEN_ENDPOINT
        );

    if (NULL == ListenEndPoint)
    {
        EventWriteLeaseDriverTextTraceError(NULL, L"Listen endpoint allocation failed", 0);
        return NULL;
    }

    RtlZeroMemory(ListenEndPoint, sizeof(TRANSPORT_LISTEN_ENDPOINT));

    return ListenEndPoint;
}

VOID 
LeaseListenEndpointDestructor( __in PTRANSPORT_LISTEN_ENDPOINT ListenEndPoint )
/*++
Routine Description:
    Deallocate a listen endpoint struct.
--*/
{
    if (NULL != ListenEndPoint)
    {
        ExFreePool(ListenEndPoint);
    }
}

PLEASE_MESSAGE_EXT
MessageExtensionConstructor( )
/*++
    Allocate memory for a message extension.
--*/
{
    PLEASE_MESSAGE_EXT MsgExt = ExAllocatePoolWithTag(
        NonPagedPool,
        sizeof(LEASE_MESSAGE_EXT),
        LEASE_MESSAGE_TAG
        );

    if (NULL == MsgExt)
    {
        EventWriteLeaseDriverTextTraceError(NULL, L"Message extension allocation failed", 0);
        return NULL;
    }

    RtlZeroMemory(MsgExt, sizeof(LEASE_MESSAGE_EXT));

    return MsgExt;
}

VOID 
MessageExtensionDestructor( __in PLEASE_MESSAGE_EXT MsgExt )
/*++
    Deallocate a message extension.
--*/
{
    if (NULL != MsgExt)
    {
        ExFreePool(MsgExt);
    }
}

VOID
FreeLeasingApplicationEventQueue(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    )

/*++

Routine Description:

    Destructs all remaining lease event buffers for a leasing application.

Arguments:

    LeasingApplicationContext - the leasing application identifier to be cleaned up.

Return Value:

    n/a.

--*/

{
    PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER EventBuffer = NULL; 
    PVOID IterEventBuffer = NULL;
    BOOLEAN Deleted = FALSE;

    //
    // Iterate over the remaining events and deallocate them.
    //
    IterEventBuffer = RtlEnumerateGenericTable(
        &LeasingApplicationContext->LeasingApplicationEventQueue,
        TRUE
        );
        
    while (NULL != IterEventBuffer) {

        //
        // Retrieve lease event buffer.
        //
        EventBuffer = *((PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER*)IterEventBuffer);

        //
        // Remove the lease event buffer from the hash table.
        //
        Deleted = RtlDeleteElementGenericTable(
            &LeasingApplicationContext->LeasingApplicationEventQueue,
            &EventBuffer
            );
        LAssert(Deleted);
        
        //
        // Delete the lease event buffer.
        //
        LeaseEventDestructor(EventBuffer);

        //
        // Move on to the next element.
        //
        IterEventBuffer = RtlEnumerateGenericTable(
            &LeasingApplicationContext->LeasingApplicationEventQueue,
            TRUE
            );
    }
}

VOID
ClearCertificateList(
    __in PRTL_GENERIC_TABLE HashTable,
    __in BOOLEAN CompleteVerify,
    __in PLEASING_APPLICATION_CONTEXT unregisterAppHandle
    )

/*++

Routine Description:

    Empties a hash table containing certificates.

Arguments:

    HashTable - list with certs to clear out.

    CompleteVerify - if the list is pending list, we need to complete the verify callback

Return Value:

    n/a
    
--*/

{
    PVOID Iter = NULL;
    PLEASE_REMOTE_CERTIFICATE LeaseCertContext = NULL;
    BOOLEAN Deleted = FALSE;

    //
    // Iterate over the leasing applications and deallocate them.
    //
    Iter = RtlEnumerateGenericTable(HashTable, TRUE);
        
    while (NULL != Iter) {
             
        //
        // Retrieve lease relationship identifier.
        //
        LeaseCertContext = *((PLEASE_REMOTE_CERTIFICATE*)Iter);

        //
        // Remove list entry.
        //

        if (unregisterAppHandle == NULL || unregisterAppHandle == LeaseCertContext->certLeasingAppHandle)
        {
            Deleted = RtlDeleteElementGenericTable(
                HashTable,
                &LeaseCertContext
                );
            LAssert(Deleted);

            if (TRUE == CompleteVerify)
            {
                TransportProcessRemoteCertVerifyResult(LeaseCertContext->verifyOperation, STATUS_UNSUCCESSFUL);
            }

            //
            // Deallocate leasing application identifier.
            //
            LeaseRemoteCertificateDestructor(LeaseCertContext);
        }

        //
        // Move on the next element.
        //
        Iter = RtlEnumerateGenericTable(HashTable, TRUE);
    }
}


VOID 
LeasingApplicationDestructor(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    )
    
/*++

Routine Description:

    Deallocates an existent leasing application.

Arguments:

    LeasingApplicationContext - the leasing application identifier to be freed.

Return Value:

    n/a.

--*/

{
    if (NULL != LeasingApplicationContext->DelayedRequest) {

        //
        // Need to complete the delayed request.
        //
        DelayedRequestCompletion(
            LeasingApplicationContext,
            LEASING_APPLICATION_NONE,
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
    }

    //
    // Free lease event buffer lists.
    //
    FreeLeasingApplicationEventQueue(LeasingApplicationContext);

    DequeueUnregisterTimer(LeasingApplicationContext);
    //
    // Perform cleanup.
    //
    ExFreePool(LeasingApplicationContext->LeasingApplicationIdentifier);
    ExFreePool(LeasingApplicationContext);
}

VOID 
LeaseRemoteCertificateDestructor(
    __in PLEASE_REMOTE_CERTIFICATE remoteCert
    )
    
/*++

Routine Description:

    Deallocates an lease remote certificate struct.

Arguments:

Return Value:

    n/a.

--*/

{
    if (NULL != remoteCert)
    {
        if (NULL != remoteCert->pbCertificate)
        {
            ExFreePool(remoteCert->pbCertificate);
        }

        ExFreePool(remoteCert);
    }
}

BOOLEAN
EnqueueLeasingApplicationLeaseEvent(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    )

/*++

Routine Description:

    Allocates the next lease event buffer for this leasing application.

Arguments:

    LeasingApplicationContext - the leasing application to contain the event buffer.

Return Value:

    FALSE if no buffer can be allocated.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER EventBuffer = NULL;
    PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER* EventBufferExistent = NULL; 
    BOOLEAN Inserted = FALSE;

    //
    // Create event in the leasing application.
    //
    EventBuffer = LeaseEventConstructor();
    
    if (NULL == EventBuffer) {
        
        //
        // Could not allocate event.
        //
        Status = STATUS_INSUFFICIENT_RESOURCES;
        
    } else {
    
        //
        // Insert lease event into the leasing application hash table.
        //
        EventBufferExistent = RtlInsertElementGenericTable(
            &LeasingApplicationContext->LeasingApplicationEventQueue,
            &EventBuffer,
            sizeof(PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER),
            &Inserted
            );
    
        //
        // Check insert result.
        //
        if (NULL == EventBufferExistent) {
        
            LAssert(!Inserted);

            //
            // Deallocate buffer.
            //
            LeaseEventDestructor(EventBuffer);
            
            //
            // Hash table memory allocation failed.
            //
            Status = STATUS_INSUFFICIENT_RESOURCES;
            
        } else {
    
            //
            // New lease event was added.
            //
            LAssert(Inserted);
            LAssert(*EventBufferExistent == EventBuffer);
        }
    }

    return NT_SUCCESS(Status);
}

const LPWSTR GetEventType(
    __in LEASE_LAYER_EVENT_TYPE Eventype
    )
    
/*++

Routine Description:

    Returns readable event type.

Parameters Description:

    EventType - event type for pretty printing.

Return Value:

    Stringyfied event type.

--*/

{
    LPWSTR EventTypeString = L"";

    switch (Eventype) {

    case LEASING_APPLICATION_NONE:
        EventTypeString = L"NONE";
        break;
    case LEASING_APPLICATION_EXPIRED:
        EventTypeString = L"EXPIRED";
        break;
    case LEASING_APPLICATION_LEASE_ESTABLISHED:
        EventTypeString = L"LEASE ESTABLISHED";
        break;
    case LEASING_APPLICATION_ARBITRATE:
        EventTypeString = L"ARBITRATE";
        break;
    case REMOTE_LEASING_APPLICATION_EXPIRED:
        EventTypeString = L"REMOTE EXPIRED";
        break;
    case REMOTE_CERTIFICATE_VERIFY:
        EventTypeString = L"REMOTE CERTIFICATE VERIFY";
        break;
    }

    return EventTypeString;
}

VOID 
TryDequeueLeasingApplicationLeaseEvent(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    )

/*++

Routine Description:

    Checks if any new lease event has happened in the meantime.

Arguments:

    LeasingApplicationContext - the leasing application containing the event buffers.

Return Value:

    n/a.

--*/

{
    PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER LeaseEventBuffer = NULL;
    PVOID IterLeaseEventBuffer = NULL;
    BOOLEAN Deleted = FALSE;
    BOOLEAN HighPriority = FALSE;

    //
    // This routine is called only when we know we have a delayed request available.
    //
    LAssert(NULL != LeasingApplicationContext->DelayedRequest);

    //
    // Retrieve the first unused buffer.
    //
    for (IterLeaseEventBuffer = RtlEnumerateGenericTable(&LeasingApplicationContext->LeasingApplicationEventQueue, TRUE);
         NULL != IterLeaseEventBuffer;
         IterLeaseEventBuffer = RtlEnumerateGenericTable(&LeasingApplicationContext->LeasingApplicationEventQueue, FALSE)) {

        //
        // Retrieve lease event buffer.
        //
        LeaseEventBuffer = *((PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER*)IterLeaseEventBuffer);

        if (LEASING_APPLICATION_NONE != LeaseEventBuffer->EventType) {

            //
            // Found first used buffer.
            //
            break;
        }
    }

    if (NULL != IterLeaseEventBuffer) {

        EventWriteRaisingQueuingEventLeaseAppRemote3(
            NULL,
            GetEventType(LeaseEventBuffer->EventType),
            LeasingApplicationContext->LeasingApplicationIdentifier,
            LeaseEventBuffer->RemoteLeasingApplicationIdentifier
            );

        //
        // Completion requires high priority if the lease is expired or needs to arbitrate.
        //
        HighPriority = (
            LEASING_APPLICATION_EXPIRED == LeaseEventBuffer->EventType || 
            LEASING_APPLICATION_ARBITRATE == LeaseEventBuffer->EventType ||
            REMOTE_CERTIFICATE_VERIFY == LeaseEventBuffer->EventType
            );

        //
        // Remove the lease event buffer from the hash table.
        //
        Deleted = RtlDeleteElementGenericTable(
            &LeasingApplicationContext->LeasingApplicationEventQueue,
            &LeaseEventBuffer
            );
        LAssert(Deleted);

        //
        // Complete request.
        //
        DelayedRequestCompletion(
            LeasingApplicationContext,
            LeaseEventBuffer->EventType,
            LeaseEventBuffer->LeaseHandle,
            LeasingApplicationContext->Instance,
            LeaseEventBuffer->RemoteLeasingApplicationIdentifier,
            LeaseEventBuffer->RemoteTTL,
            LeaseEventBuffer->LocalTTL,
            &LeaseEventBuffer->RemoteSocketAddress,
            LeaseEventBuffer->LeaseAgentInstance,
            LeaseEventBuffer->RemoteLeaseAgentInstance,
            STATUS_SUCCESS,
            HighPriority,
            LeaseEventBuffer->cbCertificate,
            LeaseEventBuffer->pbCertificate,
            LeaseEventBuffer->remoteCertVerifyCompleteOp,
            LeaseEventBuffer->HealthReportReportCode,
            LeaseEventBuffer->HealthReportDynamicProperty,
            LeaseEventBuffer->HealthReportExtraDescription,
            LeaseEventBuffer->remoteVersion,
            LeaseEventBuffer->monitorLeaseInstance,
            LeaseEventBuffer->subjectLeaseInstance,
            LeaseEventBuffer->remoteArbitrationDurationUpperBound
            );
                
        //
        // Deallocate the lease event buffer.
        //
        LeaseEventDestructor(LeaseEventBuffer);
    }
}

PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER
DequeueLeasingApplicationLeaseEvent(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in LEASE_LAYER_EVENT_TYPE EventType,
    __in BOOLEAN Deallocate
    )

/*++

Routine Description:

    Retrieves the next unused lease event buffer for this leasing application.

Arguments:

    LeasingApplicationContext - the leasing application containing the event buffers.

    EventType - event category.

    Deallocate - specifies that the lease event buffer be removed.

Return Value:

    NULL if no buffer can be found.

--*/

{
    PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER LeaseEventBuffer = NULL;
    PVOID IterLeaseEventBuffer = NULL;
    BOOLEAN Deleted = FALSE;

    //
    // Retrieve the first unused buffer.
    //
    for (IterLeaseEventBuffer = RtlEnumerateGenericTable(&LeasingApplicationContext->LeasingApplicationEventQueue, TRUE);
         NULL != IterLeaseEventBuffer;
         IterLeaseEventBuffer = RtlEnumerateGenericTable(&LeasingApplicationContext->LeasingApplicationEventQueue, FALSE))
    {
        LeaseEventBuffer = *((PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER*)IterLeaseEventBuffer);

        if (LEASING_APPLICATION_NONE == LeaseEventBuffer->EventType)
        {
            //
            // Found first unused buffer.
            //
            break;
        }
    }

    //
    // Deallocate TRUE: means the event is removed from the event queue
    // Deallocate FALSE: means we need to enqueue a real event callback, 
    // which will take a pre-allocated blank event space, so a new blank event need to be inserted
    //
    if (Deallocate)
    {
         if( NULL == LeaseEventBuffer )
         {
             return NULL;
         }
        //
        // Remove the lease event buffer from the hash table.
        //
        Deleted = RtlDeleteElementGenericTable(
            &LeasingApplicationContext->LeasingApplicationEventQueue,
            &LeaseEventBuffer
            );
        LAssert(Deleted);

        //
        // Deallocate the lease event buffer.
        //
        LeaseEventDestructor(LeaseEventBuffer);

        //
        // Reset return value.
        //
        LeaseEventBuffer = NULL;

    }
    else
    {
        ASSERT(LEASING_APPLICATION_NONE != EventType);

        if( NULL == LeaseEventBuffer )
        {
            LeaseEventBuffer = LeaseEventConstructor();

            if (NULL == LeaseEventBuffer )
            {
                return NULL;
            }
        }
        //
        // Mark the lease event buffer as used.
        //
        LeaseEventBuffer->EventType = EventType;
    }

    return LeaseEventBuffer;
}

BOOLEAN
EnqueueLeasingApplicationAllLeaseEvents(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext
    )
{
    ULONG index = 0;
    BOOLEAN enqueued = FALSE;

    for (;index < EVENT_COUNT; index++)
    {
        enqueued = EnqueueLeasingApplicationLeaseEvent(LeasingApplicationContext);
        if (!enqueued)
        {
            FreeLeasingApplicationEventQueue(LeasingApplicationContext);
            break;
        }
    }

    return enqueued;
}

VOID
PopulateEventBuffer(
    __in PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER EventBuffer,
    __in LEASE_LAYER_EVENT_TYPE EventType,
    __in HANDLE LeaseHandle,
    __in HANDLE LeasingApplicationHandle,
    __in LPCWSTR RemoteLeasingApplicationIdentifier,
    __in LONG RemoteTTL,
    __in LONG LocalTTL,
    __in PTRANSPORT_LISTEN_ENDPOINT RemoteSocketAddress,
    __in LONGLONG LeaseAgentInstance,
    __in LONGLONG RemoteLeaseAgentInstance,
    __in LONG cbCertificate,
    __in_bcount(cbCertificate) PBYTE pbCertificate,
    __in PVOID remoteCertVerifyCompleteOp,
    __in LONG HealthReportReportCode,
    __in LPCWSTR HealthReportDynamicProperty,
    __in LPCWSTR HealthReportExtraDescription,
    __in USHORT remoteVersion,
    __in LONGLONG monitorLeaseInstance,
    __in LONGLONG subjectLeaseInstance,
    __in LONG remoteArbitrationDurationUpperBound
    )

/*++

Routine Description:

    Populates the event notification buffer for a leasing application.

Arguments:

    EventType - event category.

    LeasenHandle - lease handle.

    LeasingApplicationHandle - leasing application handle.
    
    RemoteLeasingApplicationIdentifier - remote leasing application.
    
    TimeToLive - remaining time to live.

    RemoteSocketAddress - remote socket address to be used for arbitration.

    LeaseAgentInstance - lease agent instance.
    
    RemoteLeaseAgentInstance - remote lease agent instance.

Return Value:

    Nothing.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(remoteArbitrationDurationUpperBound);
    //
    // Write the output buffer.
    //
    EventBuffer->EventType = EventType;
    EventBuffer->LeaseHandle = LeaseHandle;
    EventBuffer->LeasingApplicationHandle = LeasingApplicationHandle;
    EventBuffer->RemoteTTL = RemoteTTL;
    EventBuffer->LocalTTL = LocalTTL;
    EventBuffer->LeaseAgentInstance = LeaseAgentInstance;
    EventBuffer->RemoteLeaseAgentInstance = RemoteLeaseAgentInstance;

    EventBuffer->cbCertificate = cbCertificate;
    EventBuffer->remoteCertVerifyCompleteOp = remoteCertVerifyCompleteOp;
    EventBuffer->remoteVersion = remoteVersion;
    EventBuffer->remoteArbitrationDurationUpperBound = remoteArbitrationDurationUpperBound;
    EventBuffer->monitorLeaseInstance = monitorLeaseInstance;
    EventBuffer->subjectLeaseInstance = subjectLeaseInstance;

    EventBuffer->HealthReportReportCode = HealthReportReportCode;

    if (NULL != pbCertificate)
    {
        if (memcpy_s(
            &EventBuffer->pbCertificate,
            sizeof(EventBuffer->pbCertificate),
            pbCertificate,
            sizeof(EventBuffer->pbCertificate)
            ))
        {
            Status = STATUS_DATA_ERROR;
            EventWriteLeaseDriverTextTraceError(NULL, L"Mem copy failed", Status);
        }
    }

    if (NULL != RemoteLeasingApplicationIdentifier)
    {
        Status = RtlStringCchCopyW(
            EventBuffer->RemoteLeasingApplicationIdentifier,
            MAX_PATH + 1,
            RemoteLeasingApplicationIdentifier
            );

        if (!NT_SUCCESS(Status))
        {
            EventWriteLeaseDriverTextTraceError(NULL, L"Str copy failed with ", Status);
        }
    }

    if (NULL != RemoteSocketAddress)
    {
        if (memcpy_s(
            &EventBuffer->RemoteSocketAddress,
            sizeof(TRANSPORT_LISTEN_ENDPOINT),
            RemoteSocketAddress,
            sizeof(TRANSPORT_LISTEN_ENDPOINT)
            ))
        {
            Status = STATUS_DATA_ERROR;
            EventWriteLeaseDriverTextTraceError(NULL, L"Mem copy failed", Status);
        }
    }

    if (NULL != HealthReportDynamicProperty)
    {
        Status = RtlStringCchCopyW(
            EventBuffer->HealthReportDynamicProperty,
            MAX_PATH + 1,
            HealthReportDynamicProperty
        );

        if (!NT_SUCCESS(Status))
        {
            EventWriteLeaseDriverTextTraceError(NULL, L"Str copy failed with ", Status);
        }
    }

    if (NULL != HealthReportExtraDescription)
    {
        Status = RtlStringCchCopyW(
            EventBuffer->HealthReportExtraDescription,
            MAX_PATH + 1,
            HealthReportExtraDescription
        );

        if (!NT_SUCCESS(Status))
        {
            EventWriteLeaseDriverTextTraceError(NULL, L"Str copy failed with ", Status);
        }
    }
}

VOID
DelayedRequestCompletion(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in LEASE_LAYER_EVENT_TYPE EventType,
    __in_opt HANDLE LeaseHandle,
    __in HANDLE LeasingApplicationHandle,
    __in_opt LPCWSTR RemoteLeasingApplicationIdentifier,
    __in LONG RemoteTTL,
    __in LONG LocalTTL,
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
    )

/*++

Routine Description:

    Completes a delayed I/O.

Arguments:

    LeasingApplicationContext - the leasing application firing the event.

    EventType - event category.

    LeaseHandle - lease handle.

    LeasingApplicationHandle - leasing application handle.
    
    RemoteLeasingApplicationIdentifier - remote leasing application.
    
    TimeToLive - remaining time to live.

    RemoteSocketAddress - remote socket address to be used for arbitration.

    LeaseAgentInstance - lease agent instance.
    
    RemoteLeaseAgentInstance - remote lease agent instance.

    CompletionStatus - status used for completion.

    IsHighPriority - specifies if the completion should be done with high priority.

Return Value:

    n/a

--*/

{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PREQUEST_CONTEXT ReqContext = NULL;
    PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER RegisterLeasingApplicationOutputBuffer = NULL;
    BOOLEAN CompleteRequest = FALSE;
    KIRQL OldIrql;
    size_t Length = 0;
        
    //
    // Get the request's context area.
    //
    LAssert(NULL != LeasingApplicationContext->DelayedRequest);
    ReqContext = LeaseLayerRequestGetContext(LeasingApplicationContext->DelayedRequest);
         
    //
    // Acquire request spinlock.
    //
    KeAcquireSpinLock(&ReqContext->Lock, &OldIrql);

    if (!ReqContext->IsCanceled) {

        //
        // Unmark this request as cancelable.
        //
        Status = WdfRequestUnmarkCancelable(LeasingApplicationContext->DelayedRequest);
        
        if (STATUS_CANCELLED != Status) {

            LAssert(STATUS_SUCCESS == Status);
            //
            // Mark the fact that the request is to be completed.
            //
            ReqContext->IsCompleted = TRUE;

            //
            // This request will be completed shortly.
            //
            CompleteRequest = TRUE;
        }
    }
    
    //
    // Release request spinlock.
    //
    KeReleaseSpinLock(&ReqContext->Lock, OldIrql);


    //
    // Complete the request if possible.
    //
    if (CompleteRequest) {
        
        //
        // Write the request's output buffer.
        //
        Status = WdfRequestRetrieveOutputBuffer(
            LeasingApplicationContext->DelayedRequest, 
            sizeof(LEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER),
            &RegisterLeasingApplicationOutputBuffer,
            &Length
            );
        LAssert(STATUS_SUCCESS == Status); // this was checked before
        
        //
        // Write the output buffer.
        //
        PopulateEventBuffer(
            RegisterLeasingApplicationOutputBuffer,
            EventType,
            LeaseHandle,
            LeasingApplicationHandle,
            RemoteLeasingApplicationIdentifier,
            RemoteTTL,
            LocalTTL,
            RemoteSocketAddress,
            LeaseAgentInstance,
            RemoteLeaseAgentInstance,
            cbCertificate,
            pbCertificate,
            remoteCertVerifyCompleteOp,
            HealthReportReportCode,
            HealthReportDynamicProperty,
            HealthReportExtraDescription,
            remoteVersion,
            monitorLeaseInstance,
            subjectLeaseInstance,
            remoteArbitrationDurationUpperBound
            );
        
        //
        // Set the number of bytes transfered.
        //
        WdfRequestSetInformation(
            LeasingApplicationContext->DelayedRequest, 
            sizeof(LEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER)
            );
        //
        // Complete request.
        //
        if (IsHighPriority) {
    
            WdfRequestCompleteWithPriorityBoost(
                LeasingApplicationContext->DelayedRequest, 
                CompletionStatus, 
                IO_NO_INCREMENT
                );
    
        } else {
            
            WdfRequestComplete(
                LeasingApplicationContext->DelayedRequest, 
                CompletionStatus
                );
        }
    }
    
    //
    // Remove the extra reference.
    //
    WdfObjectDereference(LeasingApplicationContext->DelayedRequest);
    
    //
    // Reset the delayed request.
    //
    LeasingApplicationContext->DelayedRequest = NULL;
}

VOID
TryDelayedRequestCompletion(
    __in PLEASING_APPLICATION_CONTEXT LeasingApplicationContext,
    __in LEASE_LAYER_EVENT_TYPE EventType,
    __in_opt HANDLE LeaseHandle,
    __in HANDLE LeasingApplicationHandle,
    __in_opt LPCWSTR RemoteLeasingApplicationIdentifier,
    __in LONG RemoteTTL,
    __in LONG LocalTTL,
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
    )

/*++

Routine Description:

    Attempts to complete a delayed I/O or buffers the event.

Arguments:

    LeasingApplicationContext - the leasing application firing the event.

    EventType - event category.

    LeaseHandle - lease handle.

    LeasingApplicationHandle - leasing application handle.
    
    RemoteLeasingApplicationIdentifier - remote leasing application.
    
    TimeToLive - remaining time to live.

    RemoteSocketAddress - remote asocket address used for arbitration.

    LeaseAgentInstance - lease agent instance.
    
    RemoteLeaseAgentInstance - remote lease agent instance.
    
    CompletionStatus - status used for completion.

    IsHighPriority - specifies if the completion should be done with high priority.

Return Value:

    Nothing.

--*/

{
    PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER LeaseEventBuffer = NULL;

    if (NULL == RemoteLeasingApplicationIdentifier) {

        if (NULL != LeasingApplicationContext->DelayedRequest)
        {
            EventWriteRaisingQueuingEventLeaseApp1(
                NULL,
                GetEventType(EventType),
                LeasingApplicationContext->LeasingApplicationIdentifier,
                NULL
                );
        }
        else
        {
            EventWriteRaisingQueuingEventLeaseApp2(
                NULL,
                GetEventType(EventType),
                LeasingApplicationContext->LeasingApplicationIdentifier,
                NULL
                );
        }

    } else {

        if (NULL != LeasingApplicationContext->DelayedRequest)
        {
            EventWriteRaisingQueuingEventLeaseAppRemote1(
            NULL,
            GetEventType(EventType),
            LeasingApplicationContext->LeasingApplicationIdentifier,
            RemoteLeasingApplicationIdentifier
            ); 
        }
        else
        {
            EventWriteRaisingQueuingEventLeaseAppRemote2(
            NULL,
            GetEventType(EventType),
            LeasingApplicationContext->LeasingApplicationIdentifier,
            RemoteLeasingApplicationIdentifier
            ); 
        }
    }
    
    if (NULL != LeasingApplicationContext->DelayedRequest)
    {
        //
        // Delayed request is available
        // Raise the event immediately by complete request.
        //
        DelayedRequestCompletion(
            LeasingApplicationContext,
            EventType,
            LeaseHandle,
            LeasingApplicationHandle,
            RemoteLeasingApplicationIdentifier,
            RemoteTTL,
            LocalTTL,
            RemoteSocketAddress,
            LeaseAgentInstance,
            RemoteLeaseAgentInstance,
            CompletionStatus,
            IsHighPriority,
            cbCertificate,
            pbCertificate,
            remoteCertVerifyCompleteOp,
            HealthReportReportCode,
            HealthReportDynamicProperty,
            HealthReportExtraDescription,
            remoteVersion,
            monitorLeaseInstance,
            subjectLeaseInstance,
            remoteArbitrationDurationUpperBound
            );
        
    }
    else
    {
        //
        // Write on the pre-allocated lease event buffer.
        //
        LeaseEventBuffer = DequeueLeasingApplicationLeaseEvent(
            LeasingApplicationContext, 
            EventType, 
            FALSE
            );

        if (NULL == LeaseEventBuffer)
        {
            EventWriteRaisingQueuingEventLeaseAppFailed(
                NULL,
                GetEventType(EventType),
                LeasingApplicationContext->LeasingApplicationIdentifier
                );

            return;
        }

        //
        // Write the output buffer.
        //
        PopulateEventBuffer(
            LeaseEventBuffer,
            EventType,
            LeaseHandle,
            LeasingApplicationHandle,
            RemoteLeasingApplicationIdentifier,
            RemoteTTL,
            LocalTTL,
            RemoteSocketAddress,
            LeaseAgentInstance,
            RemoteLeaseAgentInstance,
            cbCertificate,
            pbCertificate,
            remoteCertVerifyCompleteOp,
            HealthReportReportCode,
            HealthReportDynamicProperty,
            HealthReportExtraDescription,
            remoteVersion,
            monitorLeaseInstance,
            subjectLeaseInstance,
            remoteArbitrationDurationUpperBound
            );

        //
        // A pre-allocated event in the queue is taken by PopulateEventBuffer
        // Another event need to be allocated in the queue
        // In case of failure, we will have less blank events in the queue
        // Which will be handled inside of Dequeue_Leasing_Application_LeaseEvent(FALSE)
        //
        EnqueueLeasingApplicationLeaseEvent(LeasingApplicationContext);
    }
}
