// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
int64 LeaseInstanceId = Common::Random().Next();

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
    auto LeaseEvent = new LEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER();
    
    if (NULL == LeaseEvent) {
        
        EventWriteAllocateFail11(
            NULL
            );
        goto Error;
    }

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


//KDEFERRED_ROUTINE UnregisterExpireTimer;

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
    LARGE_INTEGER NewAppInstance; 
    auto LeasingApplicationContext = new LEASING_APPLICATION_CONTEXT();
    if (NULL == LeasingApplicationContext) {
        
        EventWriteAllocateFail12(
            NULL
            );
        goto Error;
    }

    //RtlZeroMemory(LeasingApplicationContext, sizeof(LEASING_APPLICATION_CONTEXT));

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

    LeasingApplicationContext->LeasingApplicationIdentifier = (LPWSTR)ExAllocatePoolWithTag(
        NonPagedPool,
        AppIdSize,
        LEASE_APPLICATION_ID_TAG
        );
    NewAppInstance = GetInstance();
    LARGE_INTEGER_ADD_INTEGER(NewAppInstance, LeaseInstanceId);
    LeasingApplicationContext->Instance = (HANDLE)NewAppInstance.QuadPart;
    
    if (NULL == LeasingApplicationContext->LeasingApplicationIdentifier) {

        EventWriteAllocateFail13(
            NULL
            );
        goto Error;
    }

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
    //TODO shuxu
//    RtlInitializeGenericTable(
//            &LeasingApplicationContext->LeasingApplicationEventQueue,
//            (PRTL_GENERIC_COMPARE_ROUTINE)GenericTableCompareLeaseEvents,
//            (PRTL_GENERIC_ALLOCATE_ROUTINE)GenericTableAllocate,
//            (PRTL_GENERIC_FREE_ROUTINE)GenericTableDeallocate,
//            NULL
//            );

    //
    // Initialize Unregister timer.
    //
    CreateTimer(LeasingApplicationContext->UnregisterTimer, UnregisterExpireTimer, LeasingApplicationContext);
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
    auto LeaseRemoteCert = new LEASE_REMOTE_CERTIFICATE();
    if (NULL == LeaseRemoteCert)
    {
        EventWriteAllocateFailLeaseRemoteCert( NULL );
        goto Error;
    }

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
    auto ListenEndPoint = new TRANSPORT_LISTEN_ENDPOINT();
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
    auto MsgExt = new LEASE_MESSAGE_EXT();
    if (NULL == MsgExt)
    {
        EventWriteLeaseDriverTextTraceError(NULL, L"Message extension allocation failed", 0);
        return NULL;
    }

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
    //
    // Iterate over the remaining events and deallocate them.
    //
    for(PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER EventBuffer: LeasingApplicationContext->LeasingApplicationEventQueue){
        // Delete the lease event buffer.
        //
        LeaseEventDestructor(EventBuffer);
    }

    LeasingApplicationContext->LeasingApplicationEventQueue.clear();
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
    if( NULL == LeasingApplicationContext) return;
    
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

    // ClearLeasingApplicationTTLFileMap(LeasingApplicationContext->Instance);

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
    PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER EventBufferExistent = NULL;
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
        auto existingIter =
                std::find(LeasingApplicationContext->LeasingApplicationEventQueue.begin(),
                          LeasingApplicationContext->LeasingApplicationEventQueue.end(),
                          EventBuffer
                );
        if(LeasingApplicationContext->LeasingApplicationEventQueue.end() == existingIter){
            LeasingApplicationContext->LeasingApplicationEventQueue.push_back(EventBuffer);
            Inserted = TRUE;
            EventBufferExistent = EventBuffer;
        }
        else {
            Inserted = FALSE;
            EventBufferExistent = *existingIter;
        }

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
            LAssert(EventBufferExistent == EventBuffer);
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
    bool foundEvent = false;
    for(PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER LeaseEventBufferItem: LeasingApplicationContext->LeasingApplicationEventQueue){
        //
        // Retrieve lease event buffer.
        //
        LeaseEventBuffer = LeaseEventBufferItem;
        if (LEASING_APPLICATION_NONE != LeaseEventBuffer->EventType) {
            //
            // Found first used buffer.
            //
            foundEvent = true;
            break;
        }
    }

    if (foundEvent) {

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
        auto removeIter = std::find(LeasingApplicationContext->LeasingApplicationEventQueue.begin(),
        LeasingApplicationContext->LeasingApplicationEventQueue.end(),
        LeaseEventBuffer);
        if(removeIter != LeasingApplicationContext->LeasingApplicationEventQueue.end()){
            LeasingApplicationContext->LeasingApplicationEventQueue.erase(removeIter);
        }
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
    BOOLEAN Deleted = FALSE;

    //
    // Retrieve the first unused buffer.
    //
    for(PLEASE_LAYER_EVENT_INPUT_OUTPUT_BUFFER LeaseEventBufferItem: LeasingApplicationContext->LeasingApplicationEventQueue){

        if (LEASING_APPLICATION_NONE == LeaseEventBufferItem->EventType)
        {
            LeaseEventBuffer = LeaseEventBufferItem;
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
        auto removeIter = std::find(LeasingApplicationContext->LeasingApplicationEventQueue.begin(),
                                    LeasingApplicationContext->LeasingApplicationEventQueue.end(),
                                    LeaseEventBuffer);
        if(removeIter != LeasingApplicationContext->LeasingApplicationEventQueue.end()){
            LeasingApplicationContext->LeasingApplicationEventQueue.erase(removeIter);
        }
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
    ReqContext = &LeasingApplicationContext->DelayedRequest->requestContext;
         
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
        //jc
        //LINUXTODO, consider facking an InternalRegisterLeasingApplication here
        //instead of relying on the same DelayedRequest for all callbacks
if (LeasingApplicationChangeRefCount(&LeasingApplicationContext->Callbacks, LeasingApplicationHandle, TRUE)) {

        auto context = LeasingApplicationContext->Callbacks.CallbackContext;

        if (LEASING_APPLICATION_EXPIRED == EventType){
            auto callback = LeasingApplicationContext->Callbacks.LeasingApplicationExpired;
            Threadpool::Post([callback, context,LeasingApplicationContext,LeasingApplicationHandle] { callback(context);  auto Decremented = LeasingApplicationChangeRefCount( &LeasingApplicationContext->Callbacks,  LeasingApplicationHandle, FALSE);
ASSERT_IF(FALSE == Decremented, "Failed to decrement ref count");});
        }
        else if(LEASING_APPLICATION_LEASE_ESTABLISHED == EventType){
            auto callback = LeasingApplicationContext->Callbacks.LeasingApplicationLeaseEstablished;
            wstring remoteLeasingApplicationIdentifier(RemoteLeasingApplicationIdentifier); //LINUXTODO, avoid copying this everytime?
            Threadpool::Post([callback, context, LeaseHandle, remoteLeasingApplicationIdentifier,LeasingApplicationContext,LeasingApplicationHandle]
            {
                callback(LeaseHandle, remoteLeasingApplicationIdentifier.c_str(), context);
                auto Decremented = LeasingApplicationChangeRefCount( &LeasingApplicationContext->Callbacks,  LeasingApplicationHandle, FALSE);
                ASSERT_IF(FALSE == Decremented, "Failed to decrement ref count");
            });
        }
        else if(LEASING_APPLICATION_ARBITRATE == EventType){
            auto callback = LeasingApplicationContext->Callbacks.LeasingApplicationArbitrate;
            auto remoteSocketAddress = *RemoteSocketAddress;
            Threadpool::Post([callback, LeasingApplicationHandle, LeaseAgentInstance, LocalTTL, remoteSocketAddress, RemoteLeaseAgentInstance, RemoteTTL, remoteVersion, monitorLeaseInstance, subjectLeaseInstance, remoteArbitrationDurationUpperBound, RemoteLeasingApplicationIdentifier, context, LeasingApplicationContext] () mutable
            {
                callback(LeasingApplicationHandle, LeaseAgentInstance, LocalTTL, &remoteSocketAddress, RemoteLeaseAgentInstance, RemoteTTL, remoteVersion, monitorLeaseInstance, subjectLeaseInstance, remoteArbitrationDurationUpperBound, RemoteLeasingApplicationIdentifier, context);
                auto Decremented = LeasingApplicationChangeRefCount( &LeasingApplicationContext->Callbacks,  LeasingApplicationHandle, FALSE);
                ASSERT_IF(FALSE == Decremented, "Failed to decrement ref count");
            });
        }
        else if(REMOTE_LEASING_APPLICATION_EXPIRED == EventType){
            auto callback = LeasingApplicationContext->Callbacks.RemoteLeasingApplicationExpired;
            wstring remoteLeasingApplicationIdentifier(RemoteLeasingApplicationIdentifier); //LINUXTODO, avoid copying this everytime?
            Threadpool::Post([callback, context, remoteLeasingApplicationIdentifier,LeasingApplicationContext,LeasingApplicationHandle] { callback(remoteLeasingApplicationIdentifier.c_str(), context);
            auto Decremented = LeasingApplicationChangeRefCount( &LeasingApplicationContext->Callbacks,  LeasingApplicationHandle, FALSE);
            ASSERT_IF(FALSE == Decremented, "Failed to decrement ref count");});
        }
        else if(LEASING_APPLICATION_NONE == EventType){
            auto Decremented = LeasingApplicationChangeRefCount( &LeasingApplicationContext->Callbacks,  LeasingApplicationHandle, FALSE);
            ASSERT_IF(FALSE == Decremented, "Failed to decrement ref count");
        }
        else
        {
            Assert::CodingError("unexpected event type: {0}", (int)EventType);
        }


//TODO shuxu delete callback if needed, see windows code
}

    }
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
            LeasingApplicationContext->Instance,
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
        // A pre-allocated event in the queue is taken by PopulateEventBuffer
        // Another event need to be allocated in the queue
        // In case of failure, we will have less blank events in the queue
        // Which will be handled inside of Dequeue_Leasing_Application_LeaseEvent(FALSE)
        //
        EnqueueLeasingApplicationLeaseEvent(LeasingApplicationContext);
    }
}

