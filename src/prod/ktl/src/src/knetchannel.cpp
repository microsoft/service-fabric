/*++

Copyright (c) 2011  Microsoft Corporation

Module Name:

    knetchannel.cpp

Abstract:

    This file implements the KNetChannel abstractions. KNetChannel supports client-server communication pattern similar to async RPC.

Author:

    Peng Li (pengli)    24-Oct-2011

Environment:

    Kernel Mode or User Mode

Notes:

Revision History:

--*/

#include <ktl.h>
#include <ktrace.h>

#ifndef GetTraceTime
#define GetTraceTime()  KNt::GetPerformanceTime()
#endif

//
// KNetChannelBaseMessage methods.
//

const KTagType KNetChannelBaseMessage::_ThisTypeTag = "NetChMsg";

// {A47FDA02-C7C1-4311-A479-3C503740A068}
const GUID KNetChannelBaseMessage::MessageTypeId = 
    { 0xa47fda02, 0xc7c1, 0x4311, { 0xa4, 0x79, 0x3c, 0x50, 0x37, 0x40, 0xa0, 0x68 } };

KNetChannelBaseMessage::KNetChannelBaseMessage()
{
    SetTypeTag(KNetChannelBaseMessage::_ThisTypeTag);

    _ClientRequestId = 0;
    _RetryCount = 0;
    _Flags = 0;

    _Message = nullptr;
    
    _KSerializer = nullptr;
}

KNetChannelBaseMessage::~KNetChannelBaseMessage()
{
    Zero();
}    

VOID
KNetChannelBaseMessage::Zero()
{
    _ClientRequestId = 0;
    _RetryCount = 0;
    _Flags = 0;

    _RequestMessageTypeId.SetToNull();
    _ReplyMessageTypeId.SetToNull();   
    
    _KSerializer = nullptr;        

    _Message = nullptr;                
}   

KOutChannel& operator<<(KOutChannel& Out, KNetChannelBaseMessage& This)    
{
    K_EMIT_MESSAGE_GUID(KNetChannelBaseMessage::MessageTypeId);

    Out << This._ClientRequestId;
    Out << This._RetryCount;
    Out << This._Flags;
    Out << This._RequestMessageTypeId;
    Out << This._ReplyMessageTypeId;

    //
    // Invoke the captured serialization function pointer to serialize the user payload.
    //

    This._KSerializer(Out, *((PUCHAR)This._Message.RawPtr()));

    return Out;
}

KInChannel& operator>>(KInChannel& In, KNetChannelBaseMessage& This)
{
    K_VERIFY_MESSAGE_GUID(KNetChannelBaseMessage::MessageTypeId);    

    In >> This._ClientRequestId;
    In >> This._RetryCount;
    In >> This._Flags;
    In >> This._RequestMessageTypeId;
    In >> This._ReplyMessageTypeId;

    KGuid messageTypeId = (This._Flags & KNetChannelBaseMessage::FlagRequest)? This._RequestMessageTypeId : This._ReplyMessageTypeId;

    KMessageTypeInfoTable* typeTable = (KMessageTypeInfoTable*)In.UserContext;
    KMessageTypeInfo::SPtr typeInfo;

    NTSTATUS status = typeTable->Get(messageTypeId, typeInfo);
    if (NT_SUCCESS(status))
    {
        KAssert(typeInfo);
        
        //
        // Set the UserContext on the In channel to what the user registered.
        //
        
        In.UserContext = typeInfo->_UserContext;

        //
        // Invoke the captured kconstruct function pointer to create the user payload.
        //

        PVOID userPayload = nullptr;        
        status = typeInfo->_KConstruct(In, userPayload); 
        if (NT_SUCCESS(status))
        {
            //
            // Use _Message SPTR to keep the constructed user payload message alive.  
            // Do this before constructing and deserializing This._Message.
            // This will release This._Message in all error code paths.            
            //
            
            KAssert(userPayload);
            This._Message = typeInfo->_KCastToSharedBase(userPayload);

            //
            // Invoke the captured deserialization function pointer to deserialize the user payload.
            //

            typeInfo->_KDeserializer(In, userPayload);
        }
        else
        {
            In.SetStatus(status);
        }

        //
        // Restore the UserContext on the In channel.
        //        

        In.UserContext = typeTable;
    }
    else
    {
        In.SetStatus(status);
    }

    return In;
}

NTSTATUS __KConstruct(KInChannel& In, KNetChannelBaseMessage*& _This)
{
    _This = _new(KTL_TAG_NET_CHANNEL, In.Allocator()) KNetChannelBaseMessage();
    NTSTATUS status = (_This == nullptr) ? STATUS_INSUFFICIENT_RESOURCES : _This->Status();
    return status;    
}

//
// KNetChannelNtStatusMessage methods.
//

// {F603FF0E-33EB-4DD2-873A-B702BD14243F}
const GUID KNetChannelNtStatusMessage::MessageTypeId = 
    { 0xf603ff0e, 0x33eb, 0x4dd2, { 0x87, 0x3a, 0xb7, 0x2, 0xbd, 0x14, 0x24, 0x3f } };

K_SERIALIZE_MESSAGE(KNetChannelNtStatusMessage, KNetChannelNtStatusMessage::MessageTypeId)
{
    K_EMIT_MESSAGE_GUID(KNetChannelNtStatusMessage::MessageTypeId);

    Out << This._StatusCode;

    return Out;
}

K_DESERIALIZE_MESSAGE(KNetChannelNtStatusMessage, KNetChannelNtStatusMessage::MessageTypeId)
{
    K_VERIFY_MESSAGE_GUID(KNetChannelNtStatusMessage::MessageTypeId);
    
    In >> This._StatusCode;

    return In;
}    

NTSTATUS __KConstruct(KInChannel& In, KNetChannelNtStatusMessage*& _This)
{
    _This = _new(KTL_TAG_NET_CHANNEL, In.Allocator()) KNetChannelNtStatusMessage();
    NTSTATUS status = (_This == nullptr) ? STATUS_INSUFFICIENT_RESOURCES : _This->Status();
    return status;    
}

//
// KNetChannelRequestInstance methods.
//

const ULONG KNetChannelRequestInstance::_QueueLinkageOffset =
    FIELD_OFFSET(KNetChannelRequestInstance, _QueueLinkage);

KNetChannelRequestInstance::KNetChannelRequestInstance(
    __in KNetChannelServer& ServerObject
    ) : _ServerObject(&ServerObject)
{
}

KNetChannelRequestInstance::~KNetChannelRequestInstance() 
{
}

NTSTATUS
KNetChannelRequestInstance::StartRequest(
    __in KNetBinding::SPtr NetBinding,
    __in KInDatagram::SPtr InDatagram,            
    __in KNetChannelBaseMessage::SPtr RequestMessage,
    __in KDelegate<VOID(KNetChannelRequestInstance::SPtr)> ReceiverCallback,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync
    )

/*++

Routine Description:

    This method starts this request instance. The request instance is started before being given to the user callback.

Arguments:

    NetBinding - Supplies the network binding for the inbound request message.

    InDatagram - Supplies the network datagram for the inbound request message.

    RequestMessage - Supplie the knetchannel inbound message.

    ReceiverCallback - Supplies the user-registered callback delegate.

    Completion - Supplies the async completion callback.

    ParentAsync - Supplies the parent async for this KNetChannelRequestInstance.

Return Value:

    NTSTATUS

--*/

{
    _NetBinding = Ktl::Move(NetBinding);
    KAssert(_NetBinding);

    _InDatagram = Ktl::Move(InDatagram);
    KAssert(_InDatagram);    

    _RequestMessage = Ktl::Move(RequestMessage);
    KAssert(_RequestMessage);

    _ReceiverCallback = ReceiverCallback;
    
    _CompleteRequestCalled = 0;    

    //
    // Allocate the reply write op here so that OnCancel() can count on the existence of this async.
    //
    
    NTSTATUS status = _NetBinding->CreateDatagramWriteOp(_ReplyWriteOp, KTL_TAG_NET_CHANNEL);
    if (!NT_SUCCESS(status))
    {
        KTraceOutOfMemory(GetActivityId(), status, nullptr, 0, 0);        
        return status;
    }

    //
    // Allocate the reply message.
    //

    _ReplyMessage = _new(KTL_TAG_NET_CHANNEL, GetThisAllocator()) KNetChannelBaseMessage();
    if (!_ReplyMessage)
    {
        KTraceOutOfMemory(GetActivityId(), status, nullptr, 0, 0);  
        return status;
    }    
    
    Start(ParentAsync, Completion);
    return STATUS_PENDING;
}

VOID 
KNetChannelRequestInstance::OnStart()
{
    //
    // Invoke the user-registered callback.
    //

    _ReceiverCallback(this);    
}

VOID 
KNetChannelRequestInstance::OnCancel()
{
    KAssert(_ReplyWriteOp);
    _ReplyWriteOp->Cancel();
    
    Complete(STATUS_CANCELLED);
}

VOID
KNetChannelRequestInstance::CompleteRequest(
    __in NTSTATUS CompletionStatus,
    __in ULONG Flags,
    __in KtlNetPriority Priority
    )

/*++

Routine Description:

    This method is a special version of CompletionRequest<ReplyType>() where the server only returns a status code.

Arguments:

    CompletionStatus - Supplies the status code of the server reply.

    Flags - Supplies the flag used for sending back the reply.

    Priority - Supplies the network priority for sending back the reply.

Return Value:

    None

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    
    KNetChannelNtStatusMessage::SPtr reply = _new(KTL_TAG_NET_CHANNEL, GetThisAllocator()) 
        KNetChannelNtStatusMessage(CompletionStatus);
    if (!reply)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(GetActivityId(), status, nullptr, 0, 0);
        Complete(status);
        
        return;        
    }

    //
    // Invoke the templatized method with the right type.
    //
    
    CompleteRequest<KNetChannelNtStatusMessage>(reply, Flags, Priority);
}

VOID
KNetChannelRequestInstance::FailRequest(
    __in NTSTATUS CompletionStatus,
    __in ULONG Flags,
    __in KtlNetPriority Priority
    )

/*++

Routine Description:

    This method is a special version of CompletionRequest<ReplyType>() where the server side operation failed 
    and did not produce a valid reply. This method bypasses the type-check for the reply type and directly 
    sends an error message back.

Arguments:

    CompletionStatus - Supplies the status code of the server side operation.

    Flags - Supplies the flag used for sending back the reply.

    Priority - Supplies the network priority for sending back the reply.

Return Value:

    None

--*/

{
    //
    // CompletionStatus must be an error code.
    //
    KFatal(!NT_SUCCESS(CompletionStatus));

    _ServerObject->SendSystemStatusMessage(
        _NetBinding, 
        GetActivityId(), 
        _RequestMessage, 
        CompletionStatus, 
        Flags, 
        Priority
        );
}

VOID
KNetChannelRequestInstance::CompletionCallback(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingSubOp
    )

/*++

Routine Description:

    This method is the callback for completing this request instance async. 

Arguments:

    Parent - Supplies the parent async

    CompletingSubOp - Supplies the async op that is completing.

Return Value:

    None

--*/

{
    UNREFERENCED_PARAMETER(Parent);    
    UNREFERENCED_PARAMETER(CompletingSubOp);        
    
    //
    // Remove this request from the server queue.
    // This request has completed but its memory is still alive.
    //

    _ServerObject->RemoveRequest(*this);
}

VOID
KNetChannelRequestInstance::SendReplyCallback(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingSubOp
    )

/*++

Routine Description:

    This method is the callback for KDatagramWriteOp::StartWrite() to send back the reply message.

Arguments:

    Parent - Supplies the parent async

    CompletingSubOp - Supplies the async op that is completing.

Return Value:

    None

--*/

{
    UNREFERENCED_PARAMETER(Parent);    
    
    KAssert(this->IsInApartment());
    
    NTSTATUS status = CompletingSubOp.Status();
    CompletingSubOp.Reuse();
    
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &CompletingSubOp, 0, 0);
    }

    _ReplyMessage = nullptr;
        
    Complete(status);
    return;
}        

//
// KNetChannelServer methods.
//

KNetChannelServer::KNetChannelServer(
    __in ULONG AllocationTag,
    __in_opt ConfigParameters* Parameters
    ) :
    _AllocationTag(AllocationTag),
    _ReceiverTable(GetThisAllocator()),
    _PendingRequests(KNetChannelRequestInstance::_QueueLinkageOffset),
    _LocalUri(GetThisAllocator())
{
    NTSTATUS status = STATUS_SUCCESS;
    
    if (Parameters)
    {
        _ConfigParameters = *Parameters;
    }

    //
    // Create a network object.
    //

    KAsyncContextBase::CompletionCallback networkShutdownCallback(this, &KNetChannelServer::NetworkShutdownCallback);
    status = KNetwork::Create(0, GetThisAllocator(), _Network, nullptr, networkShutdownCallback);
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);                
        SetConstructorStatus(status);        

        return;
    }    

    status = _ReceiverTable.Initialize(_ConfigParameters.ReceiverTableSize, ReceiverKey::ComputeHash);
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);                
        SetConstructorStatus(status);

        return;
    }

    status = KMessageTypeInfoTable::Create(AllocationTag, GetThisAllocator(), _TypeInfoTable);
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);                
        SetConstructorStatus(status);

        return;
    }    
}

KNetChannelServer::~KNetChannelServer()
{
}

NTSTATUS
KNetChannelServer::Create(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __out KNetChannelServer::SPtr& Result,        
    __in_opt ConfigParameters* Parameters
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    Result = nullptr;
    
    KNetChannelServer::SPtr server = _new(AllocationTag, Allocator) KNetChannelServer(AllocationTag, Parameters);
    if (!server)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = server->Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    Result = Ktl::Move(server);
    return STATUS_SUCCESS;    
}

NTSTATUS
KNetChannelServer::Activate(
    __in KWString& LocalUri,
    __in KGuid const& LocalEp,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride
    )

/*++

Routine Description:

    This method activates the KNetChannelServer object.

Arguments:

    LocalUri - Supplies the local URI this server listens on.

    LocalEp - Supplies the network endpoint of this server.

    Completion - Supplies the async completion callback. It gets called when this server object is deactivated.

    GlobalContextOverride - Supplies an optional global context.

Return Value:

    None

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    
    status = _Network->RegisterEp(
        LocalUri,
        KNetwork::eLocalEp | KNetwork::eRemoteEp,
        (GUID&)LocalEp
        );
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        return status;
    }

    _LocalEp = LocalEp;
    _LocalUri = LocalUri;

    status = _LocalUri.Status();
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        return status;
    }

    //
    // Register the generic receiver callback to receive KNetChannelBaseMessage messages.
    //

    KNetwork::ReceiverType receiverCallback;
    receiverCallback.Bind(this, &KNetChannelServer::RequestMessageHandler);
    
    status = _Network->RegisterReceiver<KNetChannelBaseMessage>(
        _LocalEp,                                   // LocalEp
        KNetChannelBaseMessage::MessageTypeId,      // MessageId
        0,                                          // Flags
        _TypeInfoTable.RawPtr(),                    // UserContext
        receiverCallback                            // Receiver
        );
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        return status;
    }    
    
    Start(ParentAsync, Completion, GlobalContextOverride);
    return STATUS_PENDING;
}

VOID 
KNetChannelServer::Deactivate()
{
    //
    // Shutd down the network.
    //
    
    if (_Network)
    {
        _Network->Shutdown();
        _Network = nullptr;

        //
        // The deactivation will continue in NetworkShutdownCallback().
        //
        return;
    }

    Complete(STATUS_SUCCESS);    
}

VOID
KNetChannelServer::NetworkShutdownCallback(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingSubOp
    )

/*++

Routine Description:

    This method is the callback for shutting down the network. 

Arguments:

    Parent - Supplies the parent async

    CompletingSubOp - Supplies the async op that is completing.

Return Value:

    None

--*/

{
    UNREFERENCED_PARAMETER(Parent);    
    UNREFERENCED_PARAMETER(CompletingSubOp);        
    
    //
    // At this point, all network receive callbacks should have stopped.
    // Therefore, no more requests will enter the request queue.
    //
    // Cancel all pending requests.    
    //

    KNodeList<KNetChannelRequestInstance> localQueue(KNetChannelRequestInstance::_QueueLinkageOffset);
    LONG count = 0;

    K_LOCK_BLOCK(_RequestTableLock)
    {
        count = (LONG)_PendingRequests.Count();
        KAssert(count >= 0);
        localQueue.AppendTail(_PendingRequests);
    }

    KAssert(localQueue.Count() == (ULONG)count);

    KNetChannelRequestInstance* request = nullptr;
    while ((request = localQueue.RemoveHead()) != nullptr)
    {
        request->Cancel();
    }    

    //
    // Complete this async. All request instances are children of this server async.
    // Therefore, this server async object will not complete until all request instance asyncs have completed.
    //

    Complete(STATUS_SUCCESS);
}

VOID 
KNetChannelServer::OnStart()
{
    //
    // Nothing
    //
}

VOID
KNetChannelServer::RequestMessageHandler(
    __in const GUID& To,
    __in const GUID& From,
    __in const ULONG Flags,
    __in const PVOID ContextValue,
    __in KNetBinding::SPtr& Binding,
    __in KInDatagram::SPtr& Message
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(To);    
    UNREFERENCED_PARAMETER(From);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(ContextValue);    

    KNetChannelBaseMessage::SPtr requestMessage;

    status = Message->AcquireObject(requestMessage);
    if (!NT_SUCCESS(status))
    {
        //
        // This message cannot be acquired. Ignore it.
        //

        KTraceFailedAsyncRequest(status, this, 0, 0);

        if (requestMessage && status == STATUS_NOT_FOUND)
        {
            //
            // We could not construct the message because the type information
            // for the inbound user payload message is not registered.
            // Immediately send a system message back to the client.
            // This will fail the pending client request right away instead of letting it retry and timeout.
            //

            SendSystemStatusMessage(Ktl::Move(Binding), Message->GetActivityId(), Ktl::Move(requestMessage), RPC_NT_ENTRY_NOT_FOUND);
        }
        
        return;
    }

    KFatal(requestMessage->_Message.RawPtr());

    //
    // Look up the user-registered callback information.
    //

    ReceiverKey key(requestMessage->_RequestMessageTypeId, requestMessage->_ReplyMessageTypeId);
    KNetChannelReceiverType receiverCallback;
    
    K_LOCK_BLOCK(_ReceiverTableLock)
    {
        status = _ReceiverTable.Get(key, receiverCallback);
        
    }   // Release _ReceiverTableLock

    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    
        //
        // We could not find the appropriate receiver for this inbound message.
        // Immediately send a system message back to the client.
        // This will fail the pending client request right away instead of letting it retry and timeout.        
        //
    
        SendSystemStatusMessage(Ktl::Move(Binding), Message->GetActivityId(), Ktl::Move(requestMessage), RPC_NT_ENTRY_NOT_FOUND);        
        return;
    }                    

    //
    // Create a request instance from the received message.
    //

    KNetChannelRequestInstance::SPtr requestInstance = _new(_AllocationTag, GetThisAllocator()) KNetChannelRequestInstance(*this);
    if (!requestInstance)
    {
        KTraceOutOfMemory(GetActivityId(), STATUS_INSUFFICIENT_RESOURCES, this, sizeof(KNetChannelRequestInstance), 0);
        return;        
    }

    //
    // We complete this server async after shuting down the network. 
    // Since this network receive callback is still running, this server async must be in the operating state.
    //

    KAssert(this->Status() == STATUS_PENDING);

    KAsyncContextBase::CompletionCallback callback(requestInstance.RawPtr(), &KNetChannelRequestInstance::CompletionCallback);
    status = requestInstance->StartRequest(
        Ktl::Move(Binding),
        Ktl::Move(Message),
        Ktl::Move(requestMessage),        
        receiverCallback,
        callback,       // Callback
        this            // ParentAsync
        );
    
    if (K_ASYNC_SUCCESS(status))
    {
        K_LOCK_BLOCK(_RequestTableLock)
        {
            _PendingRequests.AppendTail(requestInstance.RawPtr());
        }        
    }
}

VOID
KNetChannelServer::SendSystemStatusMessage(
    __in KNetBinding::SPtr Binding,
    __in KActivityId ActId,
    __in KNetChannelBaseMessage::SPtr RequestMessage,
    __in NTSTATUS SystemStatus,
    __in ULONG Flags,
    __in KtlNetPriority Priority    
    )

/*++

Routine Description:

    This method sends a KNetChannelNtStatusMessage to the client with the speficied status code.

Arguments:

    Binding - Supplies the network binding from which the client request message is received.

    ActId - Supplies the activity id of the client request.

    RequestMessage - Supplies the inbound client request message.

    SystemStatus - Supplies the status code.

    Flags - Supplies the flag for network write op.

    Priority - Supplies the network send priority.

Return Value:

    None

--*/

{
    KDatagramWriteOp::SPtr writeOp;

    NTSTATUS status = Binding->CreateDatagramWriteOp(
        writeOp, 
        KTL_TAG_NET_CHANNEL
        );
    if (!NT_SUCCESS(status))
    {
        KTraceOutOfMemory(GetActivityId(), status, nullptr, 0, 0);        
        return;
    }

    KNetChannelNtStatusMessage::SPtr statusMessage = _new(KTL_TAG_NET_CHANNEL, GetThisAllocator()) 
        KNetChannelNtStatusMessage(SystemStatus);
    if (!statusMessage)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(GetActivityId(), status, nullptr, 0, 0);
        
        return;        
    }

    KNetChannelBaseMessage::SPtr message = _new(KTL_TAG_NET_CHANNEL, GetThisAllocator()) KNetChannelBaseMessage();
    if (!message)
    {
        KTraceOutOfMemory(GetActivityId(), status, nullptr, 0, 0);        
        return;
    }

    //
    // Construct the system message.
    // We override _ReplyMessageTypeId field so that the message can be correctly deserialized.
    //

    message->_Flags = KNetChannelBaseMessage::FlagSystemMessage;
    message->_ClientRequestId = RequestMessage->_ClientRequestId;

    message->_RequestMessageTypeId = RequestMessage->_RequestMessageTypeId;
    message->_ReplyMessageTypeId = KNetChannelNtStatusMessage::MessageTypeId;   

    //
    // Erase its type but capture the serialization function pointer.
    //
    message->SetMessageType<KNetChannelNtStatusMessage>();
    message->_Message = statusMessage.RawPtr();

    //
    // Send the message.
    //
    
    status = writeOp->StartWrite(
        Ktl::Move(message),
        Flags,              // Flags
        nullptr,            // Callback
        this,               // ParentAsync
        Priority,
        ActId
        );            
    if (!K_ASYNC_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);            
        return;                
    }
}

BOOLEAN
KNetChannelServer::RemoveRequest(
    __in KNetChannelRequestInstance& Request
    )

/*++

Routine Description:

    This method removes the given request instance from the queue. 

Arguments:

    Request - Supplies the request instance object.

Return Value:

    TRUE - The request was in the queue.

    FALSE - The request was not in the queue.

--*/

{
    KNetChannelRequestInstance* requestInstance = nullptr;
    K_LOCK_BLOCK(_RequestTableLock)
    {
        requestInstance = _PendingRequests.Remove(&Request);
    }

    return requestInstance ? TRUE : FALSE;
}

NTSTATUS
KNetChannelServer::AddKnownClient(
    __in KGuid const& ClientId,
    __in KWString& ClientUri
    )

/*++

Routine Description:

    This routine adds network endpoint information of client.

Arguments:

    ClientId - Supplies the client endpoint.

    ClientUri - Supplies the URI.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS status = STATUS_SUCCESS;

    status = _Network->RegisterEp(
        ClientUri,
        KNetwork::eRemoteEp,
        (GUID&)ClientId
        );
    return status;
}

//
// The base RequestContext class.
//

KNetChannelClient::RequestContext::RequestContext()
{
}

KNetChannelClient::RequestContext::~RequestContext()
{
}

//
// RequestContextInternal methods.
//
  
const ULONG KNetChannelClient::RequestContextInternal::_QueueLinkageOffset =
    FIELD_OFFSET(KNetChannelClient::RequestContextInternal, _QueueLinkage);

const KTagType KNetChannelClient::RequestContextInternal::_ThisTypeTag = "NetChReq";

KNetChannelClient::RequestContextInternal::RequestContextInternal()
{
    SetTypeTag(_ThisTypeTag);
    Zero();
}

KNetChannelClient::RequestContextInternal::RequestContextInternal(
    __in KNetChannelClient& ClientObject
    ) : _Client(&ClientObject)
{
    SetTypeTag(_ThisTypeTag);
    Zero();

    NTSTATUS status = STATUS_SUCCESS;
    
    KNetBinding::SPtr netBinding = _Client->GetNetBinding();
    if (!netBinding)
    {
        status = STATUS_INVALID_DEVICE_STATE;
        SetConstructorStatus(status);
        return;        
    }

    status = netBinding->CreateDatagramWriteOp(
        _RequestWriteOp, 
        KTL_TAG_NET_CHANNEL
        );
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    status = _Client->_RetryTimerRoot->AllocateTimerInstance(_RetryTimer);
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }    
}

KNetChannelClient::RequestContextInternal::~RequestContextInternal()
{
    KAssert(!_ActivityAcquired);
}

VOID
KNetChannelClient::RequestContextInternal::Zero()
{
    _ClientRequestId = 0;
    _ActivityId = 0;
    
    _RequestTimeoutIn100ns = 0;

    if (_RequestWriteOp)
    {
        _RequestWriteOp->Reuse();
    }

    if (_RetryTimer)
    {
        _RetryTimer->Reuse();
    }

    if (_RequestMessage)
    {
        _RequestMessage->Zero();
    }

    _ReplyMessage = nullptr;
    
    _IsRequestCompleting = FALSE;

    _ActivityAcquired = FALSE;
}

VOID 
KNetChannelClient::RequestContextInternal::CompleteRequest(
    __in NTSTATUS CompletionStatus
    )

/*++

Routine Description:

    This method completes this client request context.     

    An active request is in the _PendingRequests queue. Whichever codepath that takes the request out of the queue
    owns the request and should continue the completion.    

Arguments:

    CompletionStatus - Supplies the completion status.

Return Value:

    None

--*/

{        
    K_LOCK_BLOCK(_CompletionLock)
    {
        //
        // Mark this request as completing. Any async context that uses this
        // request as the parent is expected to acquire this lock, check this flag,
        // and if this flag is FALSE, start the async context.
        //
        // This protocol ensures that any child async is started while this request
        // context is in the operating state.
        //

        _IsRequestCompleting = TRUE;

    }   // Release _CompletionLock

    //
    // If the retry timer is started before the lock block, we can cancel it here.
    // If we try to start the retry timer after the lock block, it will see _IsRequestCompleting is set to TRUE.
    // Therefore, we will not start the timer.
    //
    
    if (_RetryTimer)
    {
        _RetryTimer->StopTimer();
    }

    if (_RequestWriteOp)
    {
        _RequestWriteOp->Cancel();
    }

    if (_ActivityAcquired)
    {
        _Client->ReleaseActivities();
        _ActivityAcquired = FALSE;
    }
    
    Complete(CompletionStatus);
}

VOID
KNetChannelClient::RequestContextInternal::OnStart()
{
    _StartTime = GetTraceTime();    
    SendRequestMessage();
}

VOID
KNetChannelClient::RequestContextInternal::OnReuse()
{
    Zero();
}

VOID
KNetChannelClient::RequestContextInternal::OnCancel()
{
    BOOLEAN wasInQueue = FALSE;

    //
    // An active request is in the _PendingRequests queue. Whichever codepath that takes the request out of the queue
    // owns the request and should continue the completion.
    //    
    
    K_LOCK_BLOCK(_Client->_RequestTableLock)
    {
        if (_Client->_PendingRequests.Remove(this))
        {
            wasInQueue = TRUE;
        }
        
    }   // Release _RequestTableLock

    if (wasInQueue)
    {
        CompleteRequest(STATUS_CANCELLED);
        return;
    }
}

VOID
KNetChannelClient::RequestContextInternal::SendRequestMessage()

/*++

Routine Description:

    This method sends a request message to the server.

Arguments:

    None

Return Value:

    None

--*/

{    
    K_LOCK_BLOCK(_CompletionLock)
    {
        if (_IsRequestCompleting)
        {
            //
            // This request is being completed. Do not send messages.
            //

            return;
        }

        _RequestMessage->_RetryCount++;

        KAsyncContextBase::CompletionCallback callback(this, &KNetChannelClient::RequestContextInternal::SendRequestCallback);
        NTSTATUS status = _RequestWriteOp->StartWrite(
            *(_RequestMessage),
            0,                  // Flags
            callback,           // Callback
            this,               // ParentAsync
            _SendNetPriority,
            _ActivityId
            );
        
        if (!K_ASYNC_SUCCESS(status))
        {
            //
            // If the send failed synchronously, start the retry timer immediately.
            //

            KTraceFailedAsyncRequest(status, this, 0, 0);

            KAsyncContextBase::CompletionCallback callback1;
            callback1.Bind(this, &KNetChannelClient::RequestContextInternal::RetryTimerCallback);

            _RetryTimer->StartTimer(
                this,       // ParentAsync                        
                callback1
                );
        }    
    }   // Release _CompletionLock
}

VOID
KNetChannelClient::RequestContextInternal::SendRequestCallback(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingSubOp
    )

/*++

Routine Description:

    This method is the callback for sending a request message.

Arguments:

    Parent - Supplies the parent async

    CompletingSubOp - Supplies the async op that is completing.

Return Value:

    None

--*/

{
    UNREFERENCED_PARAMETER(Parent);    
    
    KAssert(this->IsInApartment());
    KAssert(&CompletingSubOp == static_cast<KAsyncContextBase*>(_RequestWriteOp.RawPtr()));    
    
    NTSTATUS status = CompletingSubOp.Status();
    CompletingSubOp.Reuse();

    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, &CompletingSubOp, 0, 0);
    }    

    //
    // Start a retry timer.
    //

    K_LOCK_BLOCK(_CompletionLock)
    {
        if (_IsRequestCompleting)
        {
            //
            // This request is being completed. Do not start the retry timer.
            //

            return;
        }

        KAsyncContextBase::CompletionCallback callback;
        callback.Bind(this, &KNetChannelClient::RequestContextInternal::RetryTimerCallback);

        _RetryTimer->StartTimer(
            this,       // ParentAsync                        
            callback
            );
        
    }   // Release _CompletionLock    
}

VOID
KNetChannelClient::RequestContextInternal::RetryTimerCallback(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingSubOp
    )

/*++

Routine Description:

    This method is the callback for _RetryTimer.

Arguments:

    Parent - Supplies the parent async

    CompletingSubOp - Supplies the async op that is completing.

Return Value:

    None

--*/

{
    UNREFERENCED_PARAMETER(Parent);    
    
    KAssert(this->IsInApartment());
    KAssert(&CompletingSubOp == static_cast<KAsyncContextBase*>(_RetryTimer.RawPtr()));        

    NTSTATUS status = CompletingSubOp.Status();
    CompletingSubOp.Reuse();

    if (!NT_SUCCESS(status))
    {
        //
        // This timer has been cancelled, either because this request is
        // cancelled or the request has successfully completed.
        // Do not start another timer.
        //

        KAssert(status == STATUS_CANCELLED);
        return;
    }

    //
    // Check if the request should be timed out.
    //

    LONGLONG delay = GetTraceTime() - _StartTime;
    if (delay >= _RequestTimeoutIn100ns)
    {
        KTraceFailedAsyncRequest(
            STATUS_IO_TIMEOUT,
            this,
            (ULONGLONG)delay,
            (ULONGLONG)_RequestTimeoutIn100ns
            );

        BOOLEAN wasInQueue = FALSE;
        
        //
        // An active request is in the _PendingRequests queue. Whichever codepath that takes the request out of the queue
        // owns the request and should continue the completion.
        //    
        
        K_LOCK_BLOCK(_Client->_RequestTableLock)
        {
            if (_Client->_PendingRequests.Remove(this))
            {
                wasInQueue = TRUE;
            }
            
        }   // Release _RequestTableLock
        
        if (wasInQueue)
        {
            CompleteRequest(STATUS_IO_TIMEOUT);            
        }    
        
        return;
    }

    //
    // Try to send another request message.
    //

    SendRequestMessage();
}

VOID 
KNetChannelClient::RequestContextInternal::ReplyMessageHandler(
    __in KNetChannelBaseMessage::SPtr ReplyMessage
    )

/*++

Routine Description:

    This method gets called when a matching reply message is received on the client.

Arguments:

    ReplyMessage - Supplies the reply message.

Return Value:

    None

--*/

{
    KAssert(ReplyMessage->_ClientRequestId == _RequestMessage->_ClientRequestId);

    if (ReplyMessage->_Flags & KNetChannelBaseMessage::FlagSystemMessage)
    {
        //
        // This is a reply generated by the server on behalf of the user code.
        //
        
        if (ReplyMessage->_ReplyMessageTypeId == KNetChannelNtStatusMessage::MessageTypeId)
        {
            KNetChannelNtStatusMessage::SPtr statusMessage = static_cast<KNetChannelNtStatusMessage*>(ReplyMessage->_Message.RawPtr());
            ReplyMessage->_Message = nullptr;
            ReplyMessage->_ReplyMessageTypeId = _RequestMessage->_ReplyMessageTypeId;
                
            CompleteRequest(statusMessage->_StatusCode);            
            return;
        }
        else
        {
            //
            // Unknown system message type.
            //
            
            KAssert(FALSE);
            return;
        }
    }    
    
    _ReplyMessage = Ktl::Move(ReplyMessage);
    CompleteRequest(STATUS_SUCCESS);
}

//
// KNetChannelClient methods.
//

KNetChannelClient::KNetChannelClient(
    __in ULONG AllocationTag,
    __in_opt ConfigParameters* Parameters
    ) :
    _AllocationTag(AllocationTag),
    _PendingRequests(RequestContextInternal::_QueueLinkageOffset),
    _LocalUri(GetThisAllocator()),
    _RemoteUri(GetThisAllocator())
{
    NTSTATUS status = STATUS_SUCCESS;
    
    if (Parameters)
    {
        _ConfigParameters = *Parameters;
    }

    status = KMessageTypeInfoTable::Create(AllocationTag, GetThisAllocator(), _TypeInfoTable);
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    //
    // Register for the internal type KNetChannelNtStatusMessage.
    //
    
    status = RegisterServerReplyType<KNetChannelNtStatusMessage>(&GetThisAllocator());
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }    

    //
    // Create a root object for request retry timers.
    //

    KAdaptiveTimerRoot::Parameters timerParameters;
    timerParameters.InitializeParameters();    

    LPCWSTR newParams[2] = {0};

    WCHAR valueString[33] = {0};    // Max 32-bit integer

    // Max timeout
    newParams[0] = L"MaximumTimeout";
    
    swprintf_s(&valueString[0], ARRAYSIZE(valueString), L"%d", _ConfigParameters.MaximumTimeout);    
    newParams[1] = &valueString[0];
    
    timerParameters.ModifyParameters(ARRAYSIZE(newParams), newParams);

    // Min timeout
    newParams[0] = L"MinimumTimeout";
    
    swprintf_s(&valueString[0], ARRAYSIZE(valueString), L"%d", _ConfigParameters.MinimumTimeout);    
    newParams[1] = &valueString[0];
    
    timerParameters.ModifyParameters(ARRAYSIZE(newParams), newParams);
    
    status = KAdaptiveTimerRoot::Create(
        _RetryTimerRoot,
        timerParameters,
        GetThisAllocator(),
        KTL_TAG_TIMER
        );
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    _LastTimeStamp = KNt::GetSystemTime();
    _IsActive = TRUE;
}

KNetChannelClient::~KNetChannelClient()
{
}

NTSTATUS
KNetChannelClient::Create(
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __out KNetChannelClient::SPtr& Result,        
    __in_opt ConfigParameters* Parameters
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    Result = nullptr;
    
    KNetChannelClient::SPtr client = _new(AllocationTag, Allocator) KNetChannelClient(AllocationTag, Parameters);
    if (!client)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = client->Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    Result = Ktl::Move(client);
    return STATUS_SUCCESS;    
}

NTSTATUS
KNetChannelClient::Activate(
    __in KWString& ClientUri,
    __in KGuid const& ClientNetworkEndpoint,
    __in KWString& ServerUri,
    __in KGuid const& ServerNetworkEndpoint,
    __in KAsyncContextBase::CompletionCallback Completion,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride
    )

/*++

Routine Description:

    This method activates the KNetChannelClient object.

Arguments:

    ClientUri - Supplies the local URI this client listens on.

    ClientNetworkEndpoint - Supplies the network endpoint of this client.

    ServerUri - Supplies the server URI.

    ServerNetworkEndpoint - Supplies the network endpoint of the server.    

    Completion - Supplies the async completion callback. It gets called when this server object is deactivated.

    GlobalContextOverride - Supplies an optional global context.

Return Value:

    None

--*/

{
    NTSTATUS status = STATUS_SUCCESS;
    
    //
    // Create a network object and register endpoint of this node.
    //

    KAsyncContextBase::CompletionCallback networkShutdownCallback(this, &KNetChannelClient::NetworkShutdownCallback);
    status = KNetwork::Create(0, GetThisAllocator(), _Network, nullptr, networkShutdownCallback);
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        return status;
    }

    _LocalEp = ClientNetworkEndpoint;
    _LocalUri = ClientUri;

    status = _LocalUri.Status();
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        return status;
    }    

    status = _Network->RegisterEp(
        _LocalUri,
        KNetwork::eLocalEp | KNetwork::eRemoteEp,
        (GUID&)_LocalEp
        );
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        return status;
    }

    _RemoteEp = ServerNetworkEndpoint;
    _RemoteUri = ServerUri;

    status = _RemoteUri.Status();
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        return status;
    }    

    status = _Network->RegisterEp(
        _RemoteUri,
        KNetwork::eRemoteEp,
        (GUID&)_RemoteEp
        );
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        return status;
    }    

    //
    // Register the generic receiver callback to receive KNetChannelBaseMessage messages.
    //

    KNetwork::ReceiverType receiverCallback;
    receiverCallback.Bind(this, &KNetChannelClient::ReplyMessageHandler);
    
    status = _Network->RegisterReceiver<KNetChannelBaseMessage>(
        _LocalEp,                                   // LocalEp
        KNetChannelBaseMessage::MessageTypeId,      // MessageId
        0,                                          // Flags
        _TypeInfoTable.RawPtr(),                    // UserContext
        receiverCallback                            // Receiver
        );
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        return status;
    }

    //
    // Create a network binding to the server.
    //

    status = _Network->GetBinding(
        _RemoteEp,              // RemoteEp
        (GUID&)_LocalEp,        // OriginatingLocalEp
        0,                      // Flags
        _NetBinding
        );
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        return status;
    }        

    Start(ParentAsync, Completion, GlobalContextOverride);
    return STATUS_PENDING;
}

VOID 
KNetChannelClient::Deactivate()
{
    K_LOCK_BLOCK(_RequestTableLock)
    {
        //
        // Shut off new requests.
        //
        
        _IsActive = FALSE;
        
    }   // Release _RequestTableLock    
    
    if (_Network)
    {
        _Network->Shutdown();
        _Network = nullptr;

        //
        // The deactivation will continue in NetworkShutdownCallback().
        //
        return;
    }

    Complete(STATUS_SUCCESS);    
}

VOID
KNetChannelClient::NetworkShutdownCallback(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingSubOp
    )
{
    UNREFERENCED_PARAMETER(Parent);    
    UNREFERENCED_PARAMETER(CompletingSubOp);        
    
    //
    // At this point, all network receive callbacks should have stopped.
    // Therefore, no more replies will arrive. 
    //
    // Cancel all pending requests.    
    //

    K_LOCK_BLOCK(_RequestTableLock)
    {
        RequestContextInternal* request = nullptr;        
        for (request = _PendingRequests.PeekHead(); request; request = _PendingRequests.Successor(request))
        {
            request->Cancel();
        }
    }

    Complete(STATUS_SUCCESS);
}

VOID 
KNetChannelClient::OnStart()
{
    //
    // Nothing
    //
}

//
// This method allows this client to 'connect' a server at a different URI. 
// This is useful when the server instance may have restarted at a different location.
//

NTSTATUS 
KNetChannelClient::SetServerAddress(
    __in KWString& ServerUri,
    __in KGuid const& ServerNetworkEndpoint
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    KNetBinding::SPtr netBinding;

    status = _Network->RegisterEp(
        ServerUri,
        KNetwork::eRemoteEp,
        (GUID&)ServerNetworkEndpoint
        );
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        return status;
    }        

    //
    // Create a network binding to the server at the new URI.
    //
    
    status = _Network->GetBinding(
        (GUID&)ServerNetworkEndpoint,   // RemoteEp
        (GUID&)_LocalEp,                // OriginatingLocalEp
        0,                              // Flags
        netBinding
        );
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        return status;
    }            

    K_LOCK_BLOCK(_ConfigLock)
    {
        _RemoteUri = ServerUri;
        _RemoteEp = ServerNetworkEndpoint;

        _NetBinding = Ktl::Move(netBinding);
    }

    return STATUS_SUCCESS;
}

KNetBinding::SPtr
KNetChannelClient::GetNetBinding()
{
    KNetBinding::SPtr netBinding;
    
    K_LOCK_BLOCK(_ConfigLock)
    {
        netBinding = _NetBinding;
    }

    return netBinding;    
}

NTSTATUS
KNetChannelClient::AllocateRequestContext(
    __out RequestContext::SPtr& Context,
    __in ULONG AllocationTag
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    
    Context = nullptr;
    
    RequestContext::SPtr context = _new(AllocationTag, GetThisAllocator()) RequestContextInternal(*this);
    if (!context)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    Context = Ktl::Move(context);
    return STATUS_SUCCESS;
}

LONGLONG 
KNetChannelClient::CreateClientRequestId()

/*++

Routine Description:

    This method generates a new client request id. Client time ids
    monotonically increase within the same client boot sessions.
    They are generally close to the wallclock on the client.

Arguments:

    None

Return Value:

    The new client request id.

--*/

{
    LONGLONG oldValue;
    LONGLONG newValue;

    for (;;)
    {
        oldValue = *((LONGLONG volatile *)(&_LastTimeStamp));
        newValue = KNt::GetSystemTime();

        if (newValue <= oldValue)
        {
            newValue = oldValue + 1;
        }

        if (InterlockedCompareExchange64((LONGLONG volatile *)&_LastTimeStamp,
                newValue, oldValue) == oldValue)
        {
            return newValue;
        }
    }    
}

KActivityId 
KNetChannelClient::ComputeActivityId(
    __in LONGLONG ClientRequestId
    )

/*++

Routine Description:

    This method computes an activity id based on a client request id. 

Arguments:

    ClientRequestId - Supplies the client request id.

Return Value:

    The computed activity id.

--*/

{
    ULONGLONG activityId = 0;

    activityId = KChecksum::Crc64(&_LocalEp, sizeof(_LocalEp), activityId);
    activityId = KChecksum::Crc64(&_RemoteEp, sizeof(_RemoteEp), activityId);
    activityId = KChecksum::Crc64(&ClientRequestId, sizeof(ClientRequestId), activityId);    
    
    return activityId;
}

VOID
KNetChannelClient::ReplyMessageHandler(
    __in const GUID& To,
    __in const GUID& From,
    __in const ULONG Flags,
    __in const PVOID ContextValue,
    __in KNetBinding::SPtr& Binding,
    __in KInDatagram::SPtr& Message
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(To);    
    UNREFERENCED_PARAMETER(From);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(ContextValue);    
    UNREFERENCED_PARAMETER(Binding);        

    KNetChannelBaseMessage::SPtr replyMessage;

    status = Message->AcquireObject(replyMessage);
    if (!NT_SUCCESS(status))
    {
        //
        // This message cannot be acquired. Ignore it.
        //
        
        KTraceFailedAsyncRequest(status, this, 0, 0);        
        return;
    }

    //
    // Try to find the matching request.
    // If there is no matching request, the request may have been cancelled or this is a duplicate reply.
    //

    KNetChannelClient::RequestContextInternal::SPtr requestContext;

    K_LOCK_BLOCK(_RequestTableLock)
    {
        KNetChannelClient::RequestContextInternal* rawPtr = _PendingRequests.PeekHead();
        for (; rawPtr; rawPtr = _PendingRequests.Successor(rawPtr))
        {
            if (rawPtr->_ClientRequestId == replyMessage->_ClientRequestId)
            {
                requestContext = _PendingRequests.Remove(rawPtr);
                break;
            }
        }
        
    }   // Release _RequestTableLock

    if (requestContext)
    {
        KAssert(requestContext->_ActivityId == Message->GetActivityId());        
        requestContext->ReplyMessageHandler(Ktl::Move(replyMessage));
    }
}    
