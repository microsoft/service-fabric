/*++

Copyright (c) 2011  Microsoft Corporation

Module Name:

    knetchannel.h

Abstract:

    This file defines abstractions to support client-server communication pattern similar to async RPC.

    On the server side, KNetChannelServer listens on a user-specified network endpoint. The server application
    registers callbacks for request message types it intends to handle. Once a request message
    with the speficied type arrives, the server application's callback is invoked with a KNetChannelRequestInstance object.

    A KNetChannelRequestInstance object is modeled after an IRP. It contains the received message and
    all the context information about the client. The server application can process the request packet in the background.
    When it is ready to send a reply back to the client, the server application should call
    KNetChannelRequestInstance::CompleteRequest() once and only once with a reply message.

    On the client side, each KNetChannelClient object is connected to a single KNetChannelServer on the network.
    Many clients can connect to the same KNetChannelServer. The client application can send requests to the server and get replies.
    A client request is a typed object that is agreed between the client and the server. For each client request type,
    the reply type is also agreed between the client and the server. Conceptually, if the server were an RPC server,
    each <RequestType, ReplyType> pair defines an entry point on the server. Both request and reply messages are strongly typed.
    They are identified by user-supplied, unique GUIDs.

    All request and reply message types are assumed to derive from KShared<>. Each message type must have a static public
    GUID type field called 'MessageTypeId' to identify itself. It must also be serializable. The user is responsible
    for implementing all the serialization and deserialization functions for their request and reply message types.

    KNetChannel does not guarantee reliable message delieveries. Client requests may be delivered
    zero or more times (due to retries), and may be delievered out of order. KNetChannelClient uses adaptive
    retries to maximize the chance of delivering messages to the server. If after the user-specified timeout period has passed,
    KNetChannelClient still does not get a reply, it will fail the I/O with STATUS_IO_TIMEOUT. However, in many cases,
    a client request is delivered to the server exactly once and gets back exactly one reply.

    If a client sends a request for a <RequestType, ReplyType> entry that is not registered on the server, the client I/O
    fails with RPC_NT_ENTRY_NOT_FOUND.

Author:

    Peng Li (pengli)    24-Oct-2011

Environment:

    Kernel Mode or User Mode

Notes:

Revision History:

--*/

#pragma once

//
// Typeless base message for both request and reply messages.
// It contains the common header for typed messages.
// This message is reserved for KNetChannel internal use.
// It is the common carrier for user-spefied, typed messages.
//

class KNetChannelBaseMessage :
    public KObject<KNetChannelBaseMessage>,
    public KShared<KNetChannelBaseMessage>
{
    K_SERIALIZABLE(KNetChannelBaseMessage);

public:

    typedef KSharedPtr<KNetChannelBaseMessage> SPtr;

    KNetChannelBaseMessage();
    ~KNetChannelBaseMessage();

    VOID Zero();

    template <class T>
    VOID
    SetMessageType()
    {
        //
        // Capture the serialization function pointer to type T.
        //
        _KSerializer = K_SERIALIZE_REF(T);
    }

    //
    // Unique ID to identify the client request.
    //
    LONGLONG _ClientRequestId;

    //
    // Retry count of this request message, as determined by the client.
    //
    ULONG _RetryCount;

    //
    // _Flags is bitwise OR of the following constants:
    //

    static const ULONG FlagRequest = 0x1;            // This message is a client request
    static const ULONG FlagSystemMessage = 0x2;      // This message is a system message

    ULONG _Flags;

    //
    // Useful for type-safety check.
    //
    KGuid _RequestMessageTypeId;
    KGuid _ReplyMessageTypeId;

    //
    // Typeless user payload message.
    // Its type has been stripped off but KNetChannel knows it is derived from KSharedBase.
    // The message is kept alive by the ref count of this KSharedBase::SPtr.
    //
    KSharedBase::SPtr _Message;

    //
    // Type-erased serialization function pointer to the user payload message _Message.
    // This only applies to an outbound message.
    // Saving this function pointer allows us to re-serialize and re-send the same request message.
    //
    PfnKSerialize _KSerializer;

    static const KTagType _ThisTypeTag;
    static const GUID MessageTypeId;
};

//
// A message type that only contains an NTSTATUS code.
// This is commonly used as the reply message from a server.
// It is carried inside a KNetChannelBaseMessage on wire.
//

class KNetChannelNtStatusMessage :
    public KObject<KNetChannelNtStatusMessage>,
    public KShared<KNetChannelNtStatusMessage>
{
    K_SERIALIZABLE(KNetChannelNtStatusMessage);

public:

    typedef KSharedPtr<KNetChannelNtStatusMessage> SPtr;

    KNetChannelNtStatusMessage()
    {
        _StatusCode = STATUS_SUCCESS;
    }

    KNetChannelNtStatusMessage(
        __in NTSTATUS StatusCode
        )
    {
        _StatusCode = StatusCode;
    }

    ~KNetChannelNtStatusMessage()
    {
    }

    NTSTATUS _StatusCode;
    static const GUID MessageTypeId;
};

//
// This is the receiver callback type on the server side.
//

class KNetChannelRequestInstance;

typedef KDelegate<
    VOID(
        __in KSharedPtr<KNetChannelRequestInstance> NewRequest
        )>
        KNetChannelReceiverType;

//
// This class represents a new request received from a client.
// The server-side user code should take this request, process it, then call CompleteRequest() at some point.
// Once CompleteRequest() is called, the user should not touch this request again.
//

class KNetChannelRequestInstance : public KAsyncContextBase
{
    K_FORCE_SHARED(KNetChannelRequestInstance);

public:

    KNetBinding::SPtr GetNetworkBinding()
    {
        return _NetBinding;
    }

    //
    // BUGBUG
    // This method overrides KAsyncContextBase::GetActivityId().
    // If KNetChannelRequestInstance is casted to a KAsyncContextBase,
    // someone then calls GetActivityId() on the casted pointer, it will return 0.
    //

    KActivityId GetActivityId()
    {
        return _InDatagram->GetActivityId();
    }

    LONGLONG
    GetClientRequestId()
    {
        return _RequestMessage->_ClientRequestId;
    }

    ULONG
    GetClientRequestRetryCount()
    {
        return _RequestMessage->_RetryCount;
    }

    template <class RequestType>
    NTSTATUS
    AcquireRequest(
        __out KSharedPtr<RequestType>& Ptr
        );

    template <class ReplyType>
    VOID
    CompleteRequest(
        __in KSharedPtr<ReplyType> Reply,
        __in ULONG Flags = 0,
        __in KtlNetPriority Priority = KtlNetPriorityNormal
        );

    VOID
    CompleteRequest(
        __in NTSTATUS CompletionStatus,
        __in ULONG Flags = 0,
        __in KtlNetPriority Priority = KtlNetPriorityNormal
        );

    VOID
    FailRequest(
        __in NTSTATUS CompletionStatus,
        __in ULONG Flags = 0,
        __in KtlNetPriority Priority = KtlNetPriorityNormal
        );

    static const ULONG _QueueLinkageOffset;

private:

    friend class KNodeList<KNetChannelRequestInstance>;
    friend class KNetChannelServer;

    //
    // Make Complete() private so that it can only be called in this class.
    // The external user should call CompleteRequest() instead.
    //
    using KAsyncContextBase::Complete;

    FAILABLE
    KNetChannelRequestInstance(
        __in KNetChannelServer& ServerObject
        );

    NTSTATUS
    StartRequest(
        __in KNetBinding::SPtr NetBinding,
        __in KInDatagram::SPtr InDatagram,
        __in KNetChannelBaseMessage::SPtr RequestMessage,
        __in KNetChannelReceiverType ReceiverCallback,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr
        );

    VOID OnStart();
    VOID OnCancel();

    VOID
    SendReplyCallback(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingSubOp
        );

    VOID
    CompletionCallback(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingSubOp
        );

    //
    // Linkage to put this request in a KNodeList<> queue.
    //
    KListEntry _QueueLinkage;

    KNetBinding::SPtr _NetBinding;
    KInDatagram::SPtr _InDatagram;

    KNetChannelBaseMessage::SPtr _RequestMessage;

    KDatagramWriteOp::SPtr _ReplyWriteOp;
    KNetChannelBaseMessage::SPtr _ReplyMessage;

    volatile LONG _CompleteRequestCalled;

    //
    // A pointer to the server object that creates this request instance.
    //
    KSharedPtr<KNetChannelServer> _ServerObject;

    //
    // The user registered callback.
    //
    KNetChannelReceiverType _ReceiverCallback;
};

//
// Templatized KNetChannelRequestInstance methods.
//

template <class RequestType>
NTSTATUS
KNetChannelRequestInstance::AcquireRequest(
    __out KSharedPtr<RequestType>& Ptr
    )

/*++

Routine Description:

    This method extracts the user request message out of this request instance.
    The user is expected to call this method once in their receiver callback.

Arguments:

    Ptr - Returns an SPTR to the extracted user request message.

Return Value:

    NTSTATUS

--*/

{
    Ptr = nullptr;

    // Type-safety check
    KFatal(RequestType::MessageTypeId == _RequestMessage->_RequestMessageTypeId);

    RequestType* rawPtr = static_cast<RequestType*>(_RequestMessage->_Message.RawPtr());
    Ptr = rawPtr;

    return STATUS_SUCCESS;
}

template <class ReplyType>
VOID
KNetChannelRequestInstance::CompleteRequest(
    __in KSharedPtr<ReplyType> Reply,
    __in ULONG Flags,
    __in KtlNetPriority Priority
    )

/*++

Routine Description:

    This method completes this request instance with a reply message.
    The user is expected to call this method once as the final step in processing the request.

Arguments:

    Reply - Supplies the reply message.

    Flags - Supplies the flag used for sending back the reply.

    Priority - Supplies the network priority for sending back the reply.

Return Value:

    None

--*/

{
    NTSTATUS status = STATUS_SUCCESS;

    //
    // The user should only call CompleteRequest() once.
    //

    LONG r = InterlockedCompareExchange(&_CompleteRequestCalled, 1, 0);
    KAssert(!r);
    if (r)
    {
        return;
    }

    // Type-safety check
    KFatal(ReplyType::MessageTypeId == _RequestMessage->_ReplyMessageTypeId);

    KAssert(_ReplyMessage);

    _ReplyMessage->_Flags = 0;
    _ReplyMessage->_ClientRequestId = _RequestMessage->_ClientRequestId;
    _ReplyMessage->_RetryCount = _RequestMessage->_RetryCount;

    _ReplyMessage->_RequestMessageTypeId = _RequestMessage->_RequestMessageTypeId;
    _ReplyMessage->_ReplyMessageTypeId = ReplyType::MessageTypeId;

    //
    // Erase its type but capture the serialization function pointer.
    //
    _ReplyMessage->SetMessageType<ReplyType>();
    _ReplyMessage->_Message = Reply.RawPtr();

    BOOLEAN activityAcquired = TryAcquireActivities();
    if (!activityAcquired)
    {
        return;
    }

    KAssert(_ReplyWriteOp);

    KAsyncContextBase::CompletionCallback callback(this, &KNetChannelRequestInstance::SendReplyCallback);
    status = _ReplyWriteOp->StartWrite(
        *(_ReplyMessage),
        Flags,              // Flags
        callback,           // Callback
        this,               // ParentAsync
        Priority,
        GetActivityId()
        );

    ReleaseActivities();

    if (!K_ASYNC_SUCCESS(status))
    {
        Complete(status);
        return;
    }
}

//
// The server object.
//

class KNetChannelServer : public KAsyncContextBase
{
    K_FORCE_SHARED(KNetChannelServer);

public:

    struct ConfigParameters
    {
        ULONG ReceiverTableSize;

        ConfigParameters()
        {
            //
            // Initializes all parameters to their default values.
            //

            ReceiverTableSize = 17;
        }
    };

    static
    NTSTATUS
    Create(
        __in KAllocator& Allocator,
        __in ULONG AllocationTag,
        __out KNetChannelServer::SPtr& Result,
        __in_opt ConfigParameters* Parameters = nullptr
        );

    NTSTATUS
    Activate(
        __in KWString& LocalUri,
        __in KGuid const& LocalEp,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        );

    VOID Deactivate();

    template <class RequestType, class ReplyType>
    NTSTATUS
    RegisterReceiver(
        __in KNetChannelReceiverType Receiver,
        __in_opt PVOID UserContext
        );

    template <class RequestType>
    NTSTATUS
    RegisterReceiverWithReturnStatusType(
        __in KNetChannelReceiverType Receiver,
        __in_opt PVOID UserContext
        );

    NTSTATUS
    AddKnownClient(
        __in KGuid const& ClientId,
        __in KWString& ClientUri
        );

private:

    friend class KNetChannelRequestInstance;

    FAILABLE
    KNetChannelServer(
        __in ULONG AllocationTag,
        __in_opt ConfigParameters* Parameters
        );

    VOID OnStart();

    // Make Cancel() private so that it is not callable from outside.
    using KAsyncContextBase::Cancel;

    // Disable Reuse().
    VOID Reuse();

    VOID
    NetworkShutdownCallback(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingSubOp
        );

    VOID
    RequestMessageHandler(
        __in const GUID& To,
        __in const GUID& From,
        __in const ULONG Flags,
        __in const PVOID ContextValue,
        __in KNetBinding::SPtr& Binding,
        __in KInDatagram::SPtr& Message
        );

    VOID
    SendSystemStatusMessage(
        __in KNetBinding::SPtr Binding,
        __in KActivityId ActId,
        __in KNetChannelBaseMessage::SPtr RequestMessage,
        __in NTSTATUS SystemStatus,
        __in ULONG Flags = 0,
        __in KtlNetPriority Priority = KtlNetPriorityNormal
        );

    BOOLEAN
    RemoveRequest(
        __in KNetChannelRequestInstance& Request
        );

    //
    // The config parameters for this server.
    //
    ConfigParameters _ConfigParameters;

    //
    // The network object representing this node.
    //
    KNetwork::SPtr _Network;

    //
    // URI of this server.
    //
    KWString _LocalUri;

    //
    // Network endpoint of this server.
    //
    KGuid _LocalEp;

    //
    // The default memory allocation tag.
    //
    const ULONG _AllocationTag;

    //
    // A hash table of user-registered callbacks.
    //

    struct ReceiverKey
    {
        KGuid _RequestMessageTypeId;
        KGuid _ReplyMessageTypeId;

        ReceiverKey()
        {
        }

        ReceiverKey(
            __in GUID& RequestTypeId,
            __in GUID& ReplyTypeId
            )
        {
            _RequestMessageTypeId = RequestTypeId;
            _ReplyMessageTypeId = ReplyTypeId;
        }

        BOOLEAN operator==(const ReceiverKey& Right) const
        {
            return ((_RequestMessageTypeId == Right._RequestMessageTypeId) && (_ReplyMessageTypeId == Right._ReplyMessageTypeId));
        }

        static ULONG
        ComputeHash(
            __in const ReceiverKey& Key
            )
        {
            ULONG hash = 0;

            hash = KChecksum::Crc32(&Key._RequestMessageTypeId, sizeof(Key._RequestMessageTypeId), hash);
            hash = KChecksum::Crc32(&Key._ReplyMessageTypeId, sizeof(Key._ReplyMessageTypeId), hash);

            return hash;
        }
    };

    KHashTable<ReceiverKey, KNetChannelReceiverType> _ReceiverTable;
    KSpinLock _ReceiverTableLock;

    //
    // _TypeInfoTable contains all deserialization functions for user payload messages.
    //
    KMessageTypeInfoTable::SPtr _TypeInfoTable;

    //
    // A list of pending client requests, protected by _RequestTableLock.
    //
    KNodeList<KNetChannelRequestInstance> _PendingRequests;
    KSpinLock _RequestTableLock;
};

template <class RequestType, class ReplyType>
NTSTATUS
KNetChannelServer::RegisterReceiver(
    __in KNetChannelReceiverType Receiver,
    __in_opt PVOID UserContext
    )

/*++

Routine Description:

    This method registers an entry of the server interface to handle client requests.
    An entry of the server interface is defined by a pair <RequestType, ReplyType>.

Arguments:

    Receiver - Supplies the callback delegate to be invoked for this type of <RequestType, ReplyType> requests.

    UserContext - Supplies an opaque context. This opaque context is attached to the In channel
        when constructing and deserializing the inbound client request messages.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS status = STATUS_SUCCESS;

    //
    // Record the receiver information in the table.
    // The receive callback to KNetwork is always KNetChannelServer::RequestMessageHandler().
    // But KNetChannelServer::RequestMessageHandler() recovers the user-registered callbacks by a table lookup.
    //

    ReceiverKey key((GUID&)RequestType::MessageTypeId, (GUID&)ReplyType::MessageTypeId);

    K_LOCK_BLOCK(_ReceiverTableLock)
    {
        status = _ReceiverTable.Put(key, Receiver);
    }

    if (!NT_SUCCESS(status))
    {
        return status;
    }

    //
    // Record the type information for deserializing inbound messages.
    //

    status = _TypeInfoTable->PutSharedType<RequestType>((GUID&)RequestType::MessageTypeId, UserContext);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return STATUS_SUCCESS;
}

template <class RequestType>
NTSTATUS
KNetChannelServer::RegisterReceiverWithReturnStatusType(
    __in KNetChannelReceiverType Receiver,
    __in_opt PVOID UserContext
    )

/*++

Routine Description:

    This is a special version of RegisterReceiver<> where the server reply type is KNetChannelNtStatusMessage.

Arguments:

    Receiver - Supplies the callback delegate to be invoked for this type of <RequestType, ReplyType> requests.

    UserContext - Supplies an opaque context. This opaque context is attached to the In channel
        when constructing and deserializing the inbound client request messages.

Return Value:

    NTSTATUS

--*/

{
    return RegisterReceiver<RequestType, KNetChannelNtStatusMessage>(Receiver, UserContext);
}

//
// The client object.
//

class KNetChannelClient : public KAsyncContextBase
{
    K_FORCE_SHARED(KNetChannelClient);

public:

    struct ConfigParameters
    {
        ConfigParameters()
        {
            //
            // Initialize default values
            //

            MaximumTimeout = 10 * 1000;     // 10 seconds
            MinimumTimeout = 20;            // 20 milliseconds
        }

        ULONG MaximumTimeout;   // Max retry interval in milliseconds
        ULONG MinimumTimeout;   // Min retry interval in milliseconds
    };

    static
    NTSTATUS
    Create(
        __in KAllocator& Allocator,
        __in ULONG AllocationTag,
        __out KNetChannelClient::SPtr& Result,
        __in_opt ConfigParameters* Parameters = nullptr
        );

    NTSTATUS
    Activate(
        __in KWString& ClientUri,
        __in KGuid const& ClientNetworkEndpoint,
        __in KWString& ServerUri,
        __in KGuid const& ServerNetworkEndpoint,
        __in KAsyncContextBase::CompletionCallback Completion,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        );

    NTSTATUS
    SetServerAddress(
        __in KWString& ServerUri,
        __in KGuid const& ServerNetworkEndpoint
        );

    VOID Deactivate();

    template <class T>
    NTSTATUS
    RegisterServerReplyType(
        __in_opt PVOID UserContext
        );

    //
    // This is the async context for a client to request a service from the server.
    // After the request is completed, the client can extract the reply message from this context.
    // This context also contains an SPTR to the user request message.
    //

    class RequestContext : public KAsyncContextBase
    {
        K_FORCE_SHARED_WITH_INHERITANCE(RequestContext);

    public:

        KActivityId GetActivityId()
        {
            return _ActivityId;
        }

        LONGLONG
        GetClientRequestId()
        {
            return _ClientRequestId;
        }

        ULONG
        GetServerReplyRetryCount()
        {
            return (_ReplyMessage == nullptr ) ? 0 : _ReplyMessage->_RetryCount;
        }

        template <class RequestType>
        NTSTATUS
        AcquireRequest(
            __out KSharedPtr<RequestType>& Ptr
            )
        {
            Ptr = nullptr;

            if (_RequestMessage)
            {
                // Type-safety check
                KFatal(RequestType::MessageTypeId == _RequestMessage->_RequestMessageTypeId);

                RequestType* rawPtr = static_cast<ReplyType*>(_RequestMessage->_Message.RawPtr());
                Ptr = rawPtr;

                return STATUS_SUCCESS;
            }
            else
            {
                return STATUS_NOT_FOUND;
            }
        }

        template <class ReplyType>
        NTSTATUS
        AcquireReply(
            __out KSharedPtr<ReplyType>& Ptr
            )
        {
            Ptr = nullptr;

            if (_ReplyMessage)
            {
                // Type-safety check
                KFatal(ReplyType::MessageTypeId == _ReplyMessage->_ReplyMessageTypeId);

                ReplyType* rawPtr = static_cast<ReplyType*>(_ReplyMessage->_Message.RawPtr());
                Ptr = rawPtr;

                return STATUS_SUCCESS;
            }
            else
            {
                return STATUS_NOT_FOUND;
            }
        }

        NTSTATUS
        AcquireReply(
            __out NTSTATUS& CompletionStatus
            )
        {
            if (_ReplyMessage)
            {
                // Type-safety check
                KFatal(KNetChannelNtStatusMessage::MessageTypeId == _ReplyMessage->_ReplyMessageTypeId);

                KNetChannelNtStatusMessage* rawPtr = static_cast<KNetChannelNtStatusMessage*>(_ReplyMessage->_Message.RawPtr());
                CompletionStatus = rawPtr->_StatusCode;

                return STATUS_SUCCESS;
            }
            else
            {
                return STATUS_NOT_FOUND;
            }
        }

    protected:

        //
        // This is the unique ID of this request.
        //
        LONGLONG _ClientRequestId;

        KActivityId _ActivityId;

        KNetChannelBaseMessage::SPtr _RequestMessage;
        KNetChannelBaseMessage::SPtr _ReplyMessage;
    };

    NTSTATUS
    AllocateRequestContext(
        __out RequestContext::SPtr& Context,
        __in ULONG AllocationTag = KTL_TAG_NET_CHANNEL
        );

    template <class RequestType, class ReplyType>
    NTSTATUS
    CallServer(
        __in KSharedPtr<RequestType> Request,
        __in_opt KAsyncContextBase::CompletionCallback Callback,
        __in_opt KAsyncContextBase* const ParentContext = nullptr,
        __in_opt RequestContext* ChildAsync = nullptr,
        __in KActivityId ActId = 0,
        __in ULONG RequestTimeoutInMs = 5000,
        __in KtlNetPriority Priority = KtlNetPriorityNormal
        );

    template <class RequestType>
    NTSTATUS
    CallServerWithReturnStatus(
        __in KSharedPtr<RequestType> Request,
        __in_opt KAsyncContextBase::CompletionCallback Callback,
        __in_opt KAsyncContextBase* const ParentContext = nullptr,
        __in_opt RequestContext* ChildAsync = nullptr,
        __in KActivityId ActId = 0,
        __in ULONG RequestTimeoutInMs = 5000,
        __in KtlNetPriority Priority = KtlNetPriorityNormal
        );

private:

    //
    // Forward declarations.
    //
    class RequestContextInternal;

    FAILABLE
    KNetChannelClient(
        __in ULONG AllocationTag,
        __in_opt ConfigParameters* Parameters
        );

    VOID OnStart();

    // Make Cancel() private so that it is not callable from outside.
    using KAsyncContextBase::Cancel;

    // Disable Reuse().
    VOID Reuse();

    KNetBinding::SPtr GetNetBinding();

    VOID
    NetworkShutdownCallback(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingSubOp
        );

    VOID
    ReplyMessageHandler(
        __in const GUID& To,
        __in const GUID& From,
        __in const ULONG Flags,
        __in const PVOID ContextValue,
        __in KNetBinding::SPtr& Binding,
        __in KInDatagram::SPtr& Message
        );

    LONGLONG CreateClientRequestId();

    KActivityId
    ComputeActivityId(
        __in LONGLONG ClientRequestId
        );

    //
    // Expose this method as private so that RequestContextInternal can call it.
    //
    using KAsyncContextBase::ReleaseActivities;

    //
    // Indicates if Desctivate() has been called on this object.
    //
    BOOLEAN _IsActive;

    //
    // A list of pending client requests, protected by _RequestTableLock.
    //
    KNodeList<RequestContextInternal> _PendingRequests;
    KSpinLock _RequestTableLock;

    //
    // The last used client timestamp. It monotonically increases for each new request.
    //
    LONGLONG _LastTimeStamp;

    //
    // The root object for retrying client requests.
    //
    KAdaptiveTimerRoot::SPtr _RetryTimerRoot;

    //
    // The config parameters for this server.
    //
    ConfigParameters _ConfigParameters;

    //
    // The network object representing this node.
    //
    KNetwork::SPtr _Network;

    //
    // The network binding between this client and the server.
    //
    KNetBinding::SPtr _NetBinding;

    //
    // URI of this client.
    //
    KWString _LocalUri;

    //
    // Network endpoint of this client.
    //
    KGuid _LocalEp;

    //
    // URI of the server.
    //
    KWString _RemoteUri;

    //
    // Network endpoint of the server.
    //
    KGuid _RemoteEp;

    //
    // This lock protectes _NetBinding, _RemoteEp, and _RemoteUri.
    //
    KSpinLock _ConfigLock;

    //
    // The default memory allocation tag.
    //
    const ULONG _AllocationTag;

    //
    // _TypeInfoTable contains all deserialization functions for user payload messages.
    //
    KMessageTypeInfoTable::SPtr _TypeInfoTable;
};

//
// This class contains functionalities common to all types of request-reply pairs:
//  (1) Request-reply matching
//  (2) Adaptive retries
//

class KNetChannelClient::RequestContextInternal : public RequestContext
{
    K_FORCE_SHARED(RequestContextInternal);

public:

    RequestContextInternal(
        __in KNetChannelClient& ClientObject
        );

    VOID Zero();

    VOID
    CompleteRequest(
        __in NTSTATUS CompletionStatus
        );

    VOID
    ReplyMessageHandler(
        __in KNetChannelBaseMessage::SPtr ReplyMessage
        );

    template <class RequestType, class ReplyType>
    NTSTATUS
    StartRequest(
        __in KSharedPtr<RequestType> Request,
        __in_opt KAsyncContextBase::CompletionCallback Callback,
        __in_opt KAsyncContextBase* const ParentContext,
        __in KActivityId ActId,
        __in ULONG RequestTimeoutInMs,
        __in KtlNetPriority Priority
        );

    static const ULONG _QueueLinkageOffset;
    static const KTagType _ThisTypeTag;

private:

    friend class KNodeList<RequestContextInternal>;
    friend class KNetChannelClient;

    VOID OnStart();
    VOID OnReuse();
    VOID OnCancel();

    VOID SendRequestMessage();

    VOID
    SendRequestCallback(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingSubOp
        );

    VOID
    RetryTimerCallback(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingSubOp
        );

    //
    // Linkage to put this request in a KNodeList<> queue.
    //
    KListEntry _QueueLinkage;

    //
    // The net channel client object. It is an SPTR so the client object guarantees to be alive.
    //
    KNetChannelClient::SPtr _Client;

    //
    // The write op to send the request to the server.
    //
    KDatagramWriteOp::SPtr _RequestWriteOp;

    //
    // The priority to send the request message.
    //
    KtlNetPriority _SendNetPriority;

    //
    // The retry timer for the request message.
    //
    KAdaptiveTimerRoot::TimerInstance::SPtr _RetryTimer;

    //
    // Request timeout value in 100 ns.
    //
    LONGLONG _RequestTimeoutIn100ns;

    //
    // Time at various points of this request's lifetime.
    //
    LONGLONG _StartTime;

    //
    // Boolean flag to indicates if this request is completing.
    //
    BOOLEAN _IsRequestCompleting;

    //
    // This lock protects _IsRequestCompleting
    //
    KSpinLock _CompletionLock;

    //
    // Indicates if this async holds an activity on the client object.
    //
    BOOLEAN _ActivityAcquired;
};

template <class RequestType, class ReplyType>
NTSTATUS
KNetChannelClient::RequestContextInternal::StartRequest(
    __in KSharedPtr<RequestType> Request,
    __in_opt KAsyncContextBase::CompletionCallback Callback,
    __in_opt KAsyncContextBase* const ParentContext,
    __in KActivityId ActId,
    __in ULONG RequestTimeoutInMs,
    __in KtlNetPriority Priority
    )

/*++

Routine Description:

    This method starts a client request context.

Arguments:

    Request - Supplies the user payload request message.

    Callback - Supplies the completion callback.

    Parent - Supplies the parent async.

    ActId - Supplies the activity id for this request. If ActId is 0, KNetChannelClient generates an unique activity id for tracing purposes.

    RequestTimeoutInMs - Supplies the timeout value in milliseconds. If this request cannot be completed with RequestTimeoutInMs,
        it will fail with STATUS_IO_TIMEOUT.

    Priority - Supplies the network priority to send the request.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS status = STATUS_SUCCESS;

    _RequestMessage = _new(KTL_TAG_NET_CHANNEL, GetThisAllocator()) KNetChannelBaseMessage();
    if (!_RequestMessage)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        return status;
    }

    _RequestMessage->_Flags = KNetChannelBaseMessage::FlagRequest;
    _RequestMessage->_ClientRequestId = _Client->CreateClientRequestId();
    _ClientRequestId = _RequestMessage->_ClientRequestId;

    _RequestMessage->_RequestMessageTypeId = RequestType::MessageTypeId;
    _RequestMessage->_ReplyMessageTypeId = ReplyType::MessageTypeId;

    //
    // Erase its type but capture the serialization function pointer.
    //
    _RequestMessage->SetMessageType<RequestType>();
    _RequestMessage->_Message = Request.RawPtr();

    _ActivityId = (ActId == 0) ? _Client->ComputeActivityId(_RequestMessage->_ClientRequestId) : ActId;
    _SendNetPriority = Priority;

    _RequestTimeoutIn100ns = RequestTimeoutInMs * 10000i64;

    Start(ParentContext, Callback);
    return STATUS_PENDING;
}

template <class T>
NTSTATUS
KNetChannelClient::RegisterServerReplyType(
    __in_opt PVOID UserContext
    )

/*++

Routine Description:

    This method registers a type of server reply message.

Arguments:

    UserContext - Supplies an opaque context. This opaque context is attached to the In channel
        when constructing and deserializing the inbound server reply messages.

Return Value:

    NTSTATUS

--*/

{
    //
    // Record the type information for deserializing incoming messages.
    //

    NTSTATUS status = _TypeInfoTable->PutSharedType<T>((GUID&)T::MessageTypeId, UserContext);
    return status;
}

template <class RequestType, class ReplyType>
NTSTATUS
KNetChannelClient::CallServer(
    __in KSharedPtr<RequestType> Request,
    __in_opt KAsyncContextBase::CompletionCallback Callback,
    __in_opt KAsyncContextBase* const ParentContext,
    __in_opt RequestContext* ChildAsync,
    __in KActivityId ActId,
    __in ULONG RequestTimeoutInMs,
    __in KtlNetPriority Priority
    )

/*++

Routine Description:

    This method starts a client request. When the request async completes, the client can extract the server reply message from the cnotext.

Arguments:

    Request - Supplies the user payload request message.

    Callback - Supplies the completion callback.

    Parent - Supplies the parent async.

    ChildAsync - Supplies the optional async context.

    ActId - Supplies the activity id for this request. If ActId is 0, KNetChannelClient generates an unique activity id for tracing purposes.

    RequestTimeoutInMs - Supplies the timeout value in milliseconds. If this request cannot be completed within RequestTimeoutInMs,
        it will fail with STATUS_IO_TIMEOUT.

    Priority - Supplies the network priority to send the request.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS status = STATUS_SUCCESS;

    RequestContext::SPtr childAsync = ChildAsync;
    if (!childAsync)
    {
        status = AllocateRequestContext(childAsync);
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }
    else
    {
        KFatal(childAsync->GetTypeTag() == RequestContextInternal::_ThisTypeTag);
    }

    RequestContextInternal* async = static_cast<RequestContextInternal*>(childAsync.RawPtr());

    async->_ActivityAcquired = TryAcquireActivities();
    if (!async->_ActivityAcquired)
    {
        return STATUS_FILE_CLOSED;
    }

    status = async->StartRequest<RequestType, ReplyType>(Request, Callback, ParentContext, ActId, RequestTimeoutInMs, Priority);
    if (K_ASYNC_SUCCESS(status))
    {
        K_LOCK_BLOCK(_RequestTableLock)
        {
            _PendingRequests.AppendTail(async);
            if (!_IsActive)
            {
                //
                // The _IsActive flag is stricter than TryAcquireActivities().
                // It gets set at the beginning of KNetChannelClient::Deactivate().
                // Once this flag is set, the client object starts running down the network I/Os
                // and will fail all pending I/Os after that.
                //

                async->Cancel();
            }

        }   // Release _RequestTableLock
    }

    return STATUS_PENDING;
}

template <class RequestType>
NTSTATUS
KNetChannelClient::CallServerWithReturnStatus(
    __in KSharedPtr<RequestType> Request,
    __in_opt KAsyncContextBase::CompletionCallback Callback,
    __in_opt KAsyncContextBase* const ParentContext,
    __in_opt RequestContext* ChildAsync,
    __in KActivityId ActId,
    __in ULONG RequestTimeoutInMs,
    __in KtlNetPriority Priority
    )

/*++

Routine Description:

    This method is a special version of the CallServer<> method where the server reply only contains an NTSTATUS code.

Arguments:

    Request - Supplies the user payload request message.

    Callback - Supplies the completion callback.

    Parent - Supplies the parent async.

    ChildAsync - Supplies the optional async context.

    ActId - Supplies the activity id for this request. If ActId is 0, KNetChannelClient generates an unique activity id for tracing purposes.

    RequestTimeoutInMs - Supplies the timeout value in milliseconds. If this request cannot be completed within RequestTimeoutInMs,
        it will fail with STATUS_IO_TIMEOUT.

    Priority - Supplies the network priority to send the request.

Return Value:

    NTSTATUS

--*/

{
    return CallServer<RequestType, KNetChannelNtStatusMessage>(
        Request,
        Callback,
        ParentContext,
        ChildAsync,
        ActId,
        RequestTimeoutInMs,
        Priority
        );
}

