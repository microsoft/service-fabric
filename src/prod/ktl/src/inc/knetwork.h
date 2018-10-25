    /*++

    (c) 2011 by Microsoft Corp. All Rights Reserved.

    knetwork.h

    Description:
      Kernel Tempate Library (KTL): Networking & Transport Support

      Defines classes for networking and datagram transmission

    History:
      raymcc          28-Feb-2011         Initial draft
      raymcc          02-Apr-2011         Updates for in-memory transport
      raymcc          26-May-2011         Incorporate TCP network
      raymcc          29-Jun-2012         Updated to use KNetworkEndpoint

--*/

#pragma once

typedef enum { KtlNetPriorityHigh = 1, KtlNetPriorityNormal = 2, KtlNetPriorityLow = 3 } KtlNetPriority;

class KNetwork;
class KNetBinding;
class KNetServiceLayer;
class KNetAcquireSocketOp;

extern LONG __KNetwork_PendingOperations;
extern LONG __KNetwork_DroppedInboundMessages;
extern LONG __KNetwork_DroppedOutboundMessages;
extern LONG __KNetwork_PendingAccepts;


///////////////////////////////////////////////////////////////////////////////
//
//  KNetUriInfo
//

class KNetUriInfo : public KObject<KNetUriInfo>, public KShared<KNetUriInfo> {

    K_FORCE_SHARED(KNetUriInfo);

public:

    enum { eNull = 0, eServerName = 1, eVNet = 2, eLocalMachine = 3 }  eFlags;

    static
    NTSTATUS
    Create(
        __out KNetUriInfo::SPtr& NetUriInfo,
        __in KAllocator& Allocator,
        __in ULONG AllocationTag = KTL_TAG_NET
        );

    NTSTATUS Parse(
        __in KWString& RawUri
        );

    KRvdTcpNetworkEndpoint::SPtr _NetEp;
    KWString _FullUri;
    KWString _Authority;
    KWString _RelUri;
    KGuid    _GUID;
    USHORT   _Port;
    ULONG    _Flags;
};

///////////////////////////////////////////////////////////////////////////////
//
// _KNetworkHeader
//
//
#define KNet_Message_Signature              0xAAAA000000000000
#define KNet_ContentType_SystemMesasge      0x0000000000000001
#define KNet_ContentType_KtlSerialMessage   0x0000000000000002

struct KNetworkHeader {

    KNetworkHeader()
    {
        Zero();
    }

    VOID Zero()
    {
        _Control = KNet_Message_Signature;
        _ActivityId = 0;

        _DestUriLength = 0;
        _SrcUriLength = 0;
        _MetaLength = 0;
        _DataLength = 0;
        _DeferredLength = 0;
    }

    ULONGLONG _Control;
    ULONGLONG _ActivityId;

    ULONG     _MetaLength;
    ULONG     _DataLength;
    ULONG     _DeferredLength;

    USHORT    _DestUriLength;
    USHORT    _SrcUriLength;

};

///////////////////////////////////////////////////////////////////////////////
//
//
// KInDatagram
//
// Represents an inbound datagram received from the network.
//
class KInDatagram : public KObject<KInDatagram>, public KShared<KInDatagram> {

    K_FORCE_SHARED(KInDatagram);

    KInDatagram(
        __in VOID* Src,
        __in KActivityId ActId

        )
    {
        _UntypedObject = Src;
        _ActId = ActId;
    }

public:

    // GetActivityId
    //
    // Returns the ActivityId sent with this datagram.
    //
    KActivityId
    GetActivityId()
    {
        return _ActId;
    }

    // AcquireObject
    //
    // Used to retrieve the raw pointer to the deserialized object.  The caller
    // becomes the owner.  The allocate used in in the K_CONSTRUCT implementation
    // should be used for deallocation.
    //
    // Parameters:
    //      Object      Receives the raw pointer to the object.
    //
    // Return values:
    //      STATUS_SUCCESS
    //      STATUS_INSUFFICIENT_RESOURCES
    //      STATUS_INTERNAL_ERROR           (Deserialization failed)
    //
    template <class T>
    NTSTATUS
    AcquireObject(
        __out T*& Object
        )
    {
        if (_UntypedObject)
        {
            Object = (T*) _UntypedObject;
        }
        return Status();
    }

    // AcquireObject
    //
    // An overload for objects which are KShared-compatible.
    //
    // Parameters:
    //      Ptr         Receives the shared pointer to the newly deserialized object.
    //
    // Return values:
    //      STATUS_SUCCESS
    //      STATUS_INSUFFICIENT_RESOURCES
    //      STATUS_INTERNAL_ERROR           (Deserialization failed)
    //
    template <class T>
    NTSTATUS
    AcquireObject(
        __out KSharedPtr<T>& Ptr
        )
    {
        Ptr = (T*) _UntypedObject;
        return Status();
    }

private:

    friend class KDatagramReadOp;

    VOID* _UntypedObject;
    KActivityId _ActId;
    KNetworkHeader  _NetHeader;

};

inline KInDatagram::~KInDatagram() {};


///////////////////////////////////////////////////////////////////////////////
//
// KDatagramWriteOp
//
// Represents a write of a single datagram.  These objects cannot be constructed directly
// but are obtained from the KNetBinding which represents the remote computer.
//
class KDatagramWriteOp : public KAsyncContextBase {

    K_FORCE_SHARED(KDatagramWriteOp);

public:

    //
    // StartWrite
    //
    // Begins an asynchronous write to the remote endpoint associated this object
    // when it was constructed via KNetBinding::CreateDatagramWriteOp.
    //
    // Parameters:
    //      Object          The object to be transmitted. This object must have a K_SERIALIZE
    //                      implementation.
    //      Flags           Reserved; must be zero
    //      Callback        Async status callbak. The caller must ensure that <Object> is alive
    //                      and readable until the Callback is invoked.
    //      ParentContext   Optional pointer to parent context.
    //      Priority        One of the KtlNetPriority enum values.
    //      ActId           ActivityId
    //      ConnectTimeout  Specifies the amount of time the write should wait for a
    //                      connection to complete in milliseconds.
    //
    //
    // Return values:
    //   STATUS_PENDING                  Is the non-error case.
    //   STATUS_INSUFFICIENT_RESOURCES   Not enough memory to complete the oepration.
    //
    template <class T>
    NTSTATUS
    StartWrite(
        __in       T& Object,
        __in       ULONG Flags,
        __in_opt   KAsyncContextBase::CompletionCallback Callback,
        __in_opt   KAsyncContextBase* const ParentContext = nullptr,
        __in       KtlNetPriority Priority = KtlNetPriorityNormal,
        __in       KActivityId  ActId = 0,
        __in       ULONG ConnectTimeout = 100
        )
    {
        if (__KSocket_Shutdown)
        {
            return STATUS_SHUTDOWN_IN_PROGRESS;
        }

        KAssert( Callback || _UserMessage);

        _Serializer = _new (KTL_TAG_NET, GetThisAllocator()) KSerializer;
        if (!_Serializer)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        NTSTATUS Result = _Serializer->Serialize(Object);
        if (!NT_SUCCESS(Result))
        {
            return Result;
        }
        _Priority = Priority;
        _ActId = ActId;
        _Flags = Flags;
        _ConnectTimeout = ConnectTimeout;
        Start(ParentContext, Callback);
        return STATUS_PENDING;
    }

    template <class T>
    NTSTATUS
    StartWrite(
        __in       KSharedPtr<T> ObjectSPtr,
        __in       ULONG Flags,
        __in_opt   KAsyncContextBase::CompletionCallback Callback,
        __in_opt   KAsyncContextBase* const ParentContext = nullptr,
        __in       KtlNetPriority Priority = KtlNetPriorityNormal,
        __in       KActivityId  ActId = 0,
        __in       ULONG ConnectTimeout = 100
        )
    {
        if (!ObjectSPtr)
        {
            return STATUS_INVALID_PARAMETER_1;
        }

        //
        // Keep a ref count on the object to keep it alive while this async is active.
        //

        T* rawPtr = ObjectSPtr.Detach();
        _UserMessage.Attach(rawPtr);

        return StartWrite<T>(
            *rawPtr,
            Flags,
            Callback,
            ParentContext,
            Priority,
            ActId,
            ConnectTimeout
            );
    }

private:

    VOID
    OnStart();

    VOID
    NetworkSend();

    VOID
    SerializeToSocket();

    VOID
    NetWriteResult(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& Op
        );

    VOID SocketAcquiredSignal(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& Op
        );

    VOID SocketAcquiredTimeout(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& Op
        );

    friend class KNetBinding;

    ISocket::SPtr                         _WriteSocket;
    KSerializer::SPtr                     _Serializer;
    KSharedPtr<KNetBinding>               _Binding;
    KSharedPtr<KTimer>                    _Timer;
    KSharedPtr<KAsyncEvent::WaitContext>  _WaitContext;
    KNetworkHeader                        _NetHeader;
    ULONG                                 _ConnectTimeout;
    ULONG                                 _BytesToTransfer;
    KtlNetPriority                        _Priority;
    KActivityId                           _ActId;
    ULONG                                 _Flags;
    KSharedBase::SPtr                     _UserMessage;

};


///////////////////////////////////////////////////////////////////////////////
//
//
// KNetBinding
//
// Represents a logical duplex binding to a specific client endpoint.
//
//
//
class KNetBinding : public KObject<KNetBinding>, public KShared<KNetBinding> {

    K_FORCE_SHARED(KNetBinding);

public:

    // CreateDatagramWriteOp
    //
    // Creates a new instance of KDatagramWriteOp which is bound to the remote
    // endpoint associated with this KNetBinding instance.
    //
    // Parameters:
    //      NewObject       Receives the newly allocated write op.
    //      AllocationTag   Tag to use for allocation.
    //      Allocator       Allocator to use for creating the object.
    //
    // Return value:
    //      STATUS_SUCCESS
    //      STATUS_INSUFFICIENT_RESOURCES
    //
    NTSTATUS
    CreateDatagramWriteOp(
        __out KDatagramWriteOp::SPtr& NewObject,
        __in  ULONG AllocationTag = KTL_TAG_NET
        );


    // GetLocalNetworkEp
    //
    // Returns the KNetworkEndpoint of the local side of the network binding
    //
    KNetworkEndpoint::SPtr
    GetLocalNetworkEp()
    {
        return _LocalNetEp;
    }


    // GetRemoteNetworkEp
    //
    // Returns the KNetworkEndpoint of the remote side of the network binding
    //
    KNetworkEndpoint::SPtr
    GetRemoteNetworkEp()
    {
        return _RemoteNetEp;
    }


    //  GetRemoteEp
    //
    //  Returns the GUID of the remote endpoint associated with this binding.
    //
    //  Parameters:
    //          Ep      The remote GUID.
    //
    VOID
    GetRemoteEp(
        __out GUID& Ep
        )
    {
        Ep = _RemoteEp;
    }

    //  GetRemoteEpUri
    //
    //  Returns the URI of the remote endpoint.
    //
    //  Parameters:
    //      Uri         Receives the URI string.
    //
    VOID
    GetRemoteEpUri(
        __out KWString& Uri
        )
    {
        Uri = _RemoteUri;
    }

    //  GetLocalEpUri
    //
    //  Returns the Uri of the local endpoint associated with this binding.
    //
    //  Parameters:
    //      Uri     Receives the URI string of the local endpoint.
    //
    VOID
    GetLocalEpUri(
        __out KWString& Uri
        )
    {
        Uri = _LocalUri;
    }

    //  GetLocalEp
    //
    //  Returns the GUID associated with the local endpoint of this binding.
    //
    //  Return values:
    //      Ep      Receives the GUID associated with the local endpoint.
    //
    VOID
    GetLocalEp(
        __out GUID& Ep
        )
    {
        Ep = _LocalEp;
    }

    BOOLEAN
    IsEqual(
        __in SPtr& Comparand
        )
    {
        if (  _LocalEp  == Comparand->_LocalEp &&
              _RemoteEp == Comparand->_RemoteEp
            )
        {
            return TRUE;
        }
        return FALSE;
    }

    // Network Destination
    //
    // Returns TRUE if the remote ep is reachable only via
    // the network.
    //
    BOOLEAN
    NetworkDestination()
    {
        return _NetworkDestination;
    }

    VOID
    SetReadSocket(
        __in ISocket::SPtr& Sock
        )
    {
        _ReadSocket = Sock;
        _NetworkDestination = TRUE;
    }

    VOID
    SetWriteSocket(
        __in ISocket::SPtr& Sock
        )
    {
        K_LOCK_BLOCK(_Lock)
        {
            _WriteSocket = Sock;
            _NetworkDestination = TRUE;
        }
    }

    ISocket::SPtr& GetReadSocket()
    {
        return _ReadSocket;
    }

    ISocket::SPtr GetWriteSocket()
    {
        K_LOCK_BLOCK(_Lock)
        {
            if (_WriteSocket && _WriteSocket->Ok())
            {
                return _WriteSocket;
            }
            else
            {
                _WriteSocket = nullptr;
            }

        }
        return nullptr;
    }

    KSharedPtr<KNetwork>
    GetNetwork()
    {
        return _Network;
    }

    PWSTR
    GetLocalUriAddress()
    {
        return PWSTR(_LocalUri);
    }

    ULONG
    GetLocalUriLength()
    {
        return _LocalUri.Length();
    }

    PWSTR
    GetRemoteUriAddress()
    {
        return PWSTR(_RemoteUri);
    }

    ULONG
    GetRemoteUriLength()
    {
        return _RemoteUri.Length();
    }

    NTSTATUS
    AcquireSocket(
        __out KSharedPtr<KAsyncEvent::WaitContext>&  WaitContext,
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase::CompletionCallback Callback
        );

    VOID
    SocketAcquiredSignal(
         __in KAsyncContextBase* const Parent,
         __in KAsyncContextBase& Op
         );


private:

    friend class KNetwork;

    // Legacy
    KGuid         _LocalEp;
    KGuid         _RemoteEp;
    KWString      _LocalUri;
    KWString      _RemoteUri;

    // Next-gen
    KNetworkEndpoint::SPtr _LocalNetEp;
    KNetworkEndpoint::SPtr _RemoteNetEp;
    KNetUriInfo::SPtr      _DestInf;

    KSpinLock     _Lock;

    BOOLEAN           _NetworkDestination;
    BOOLEAN           _SocketAcquireActive;
    KAsyncEvent       _SocketAcquired;
    ISocket::SPtr     _ReadSocket;
    ISocket::SPtr     _WriteSocket;

    KSharedPtr<KNetwork>  _Network;

};

///////////////////////////////////////////////////////////////////////////////
//
//  VNetwork
//
//  The temporary 'in memory transport'
//
//  This represents a virtual network for the in-memory case.  Each
//  KNetwork object represents a physical node and are all recorded
//  in this object.  There is exactly one instance of this for all
//  runtime cases.
//
//  All KNetwork instances are recorded within the VNetwork
//  This object creates the necessary background thread to support
//  simulated delivery via in-memory transfer.
//

class VNetwork : public KObject<VNetwork>, public KShared<VNetwork> {

    K_FORCE_SHARED(VNetwork);

public:

    // This message is used to simulate a datagram in transit
    // on the network.
    //
    class VNetMessage : public KShared<VNetMessage>, public KObject<VNetMessage> {

        K_FORCE_SHARED(VNetMessage);

    public:

        KGuid           _Id;
        KActivityId     _ActId;
        KWString        _DestinationUri;
        KWString        _OriginatingUri;
        KtlNetPriority  _Priority;
        _KTransferAgent _Xfer;

    private:

        friend class KDatagramWriteOp;

    };

    // AcquireVNetPtr
    //
    // Returns a pointer to the sole instance of VNetwork.
    // If a non-null is returned, it is guaranteed to be
    // available until ReleaseVNetPtr() is called.
    //
    static VNetwork*
    Acquire();

    static VOID
    Release();

    // Called by KtlSystem to allocate the single VNetwork instance.
    //
    static NTSTATUS
    Startup(
        __in KAllocator& Alloc
        );

    // Add
    //
    // Called by KNetwork instances as they are created.  VNetwork
    // needs to be aware of all KNetwork instances for the purposes
    // of routing the message.
    //
    NTSTATUS
    Add(
        __in KSharedPtr<KNetwork> Net
        );

    // Remove
    //
    // Called when a KNetwork instance shuts down.  This removes
    // the KNetwork from the VNetwork, so that no more traffic
    // will occur to it.
    //
    VOID
    Remove(
        __in KSharedPtr<KNetwork> Net
        );

    // Post
    //
    // This simulates delivering a datagram to a network, similar to a TCP UDP message.
    // The in-memory implementation assumes that the destination endpoint is present
    // somewhere in one of the KNetwork instances, or an error is returned.
    //
    NTSTATUS
    Post(
        __in VNetMessage::SPtr& Msg
        );


    // EnableFaultInjection
    //
    // Enables network fault injection.
    //
    VOID
    EnableFaultInjection();

    // DisableFaultInjection
    //
    // Disables network fault injection.
    //
    VOID
    DisableFaultInjection();

    // GetFaultInjector
    //
    // If fault injection is enabled, this will return a shared
    // pointer to the injector interface
    //
    NTSTATUS
    GetFaultInjector(
        __out KNetFaultInjector::SPtr& Injector
        );

    // Shutdown
    //
    // Shuts down VNetwork and all KNetwork instances.
    //
    static VOID
    Shutdown();

private:

    friend class KNetwork;

    static const LONG VNET_SHUTDOWN_FLAG = 0x80000000;

    NTSTATUS
    StartThreads();

    static ULONG
    _UmThreadEntry(PVOID _This);

    static VOID
    _KmThreadEntry(PVOID _This);

    void ThreadProc();

    void Dispatch(
        __in VNetMessage::SPtr Msg
        );

    void
    ShutdownSequence();

    // Fault injection functions

    VOID
    Redelivery(__in PVOID);       // Callback for redelivery after delay

    VOID
    DelayMessage(
        __in VNetMessage::SPtr& Msg,
        __in ULONG Delay
        );

    HANDLE                        _ThreadHandle;

    KQueue<VNetMessage::SPtr>     _Queue;
    KSpinLock                     _QueueLock;
    KSpinLock                     _TableLock;
    KAutoResetEvent               _NewMsgEvent;
    KAutoResetEvent               _ShutdownEvent;
    KArray<KSharedPtr<KNetwork>>  _Networks;

    BOOLEAN                       _Shutdown;
    static VNetwork*              _g_pVNetPtr;
    static LONG                   _g_VNetPtrUseCount;

    // Fault injection parts

    BOOLEAN                       _InjectorEnabled;
    KNetFaultInjector::SPtr       _Injector;
    _ObjectDelayEngine*           _DelayEngine;

};


///////////////////////////////////////////////////////////////////////////////
//
//
// KDatagramReadOp
//
//
class KDatagramReadOp : public KAsyncContextBase {

    K_FORCE_SHARED(KDatagramReadOp);

public:

    //
    //  This version for use by in-memory transport VNetwork
    //
    NTSTATUS
    StartRead(
        __in VNetwork::VNetMessage::SPtr Msg,
        __in KSharedPtr<KNetwork>& NetPtr,
        __in_opt   KAsyncContextBase::CompletionCallback Callback,
        __in_opt   KAsyncContextBase* const ParentContext = nullptr
        )
    {
        _ReceivingNetwork = NetPtr;
        _Msg = Msg;
        _InMemoryMode = TRUE;
        Start(ParentContext, Callback);
        return STATUS_PENDING;
    }

    //
    //  This version for use by KNetServiceLayer ('real' network)
    //
    NTSTATUS
    StartRead(
        __in KNetworkHeader& Header,
        __in KNetBinding::SPtr& Binding,
        __in_opt   KAsyncContextBase::CompletionCallback Callback,
        __in_opt   KAsyncContextBase* const ParentContext = nullptr
        )
    {
        if (__KSocket_Shutdown)
        {
            return STATUS_SHUTDOWN_IN_PROGRESS;
        }

        _Binding = Binding;
        _NetHeader = Header;
        _ReceivingNetwork = _Binding->GetNetwork();
        _InMemoryMode = FALSE;
        Start(ParentContext, Callback);
        return STATUS_PENDING;
    }

private:

    VOID
    OnStart();

    VOID
    Phase1ReadComplete(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& Op
        );

    VOID
    Phase2ReadComplete(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& Op
        );

    VOID
    OnMemoryTransportStart();

    VOID
    OnNetworkTransportStart();

    VOID
    FinalizeAndDeliver();

    friend class KNetwork;

    KMemRef                      _MetaMemRef;
    KMemRef                      _DataMemRef;
    KNetworkHeader               _NetHeader;
    KNetBinding::SPtr            _Binding;
    BOOLEAN                      _InMemoryMode;
    PVOID                        _TheObject;
    ULONG                        _BytesToTransfer;
    KSharedPtr<KNetwork>         _ReceivingNetwork;
    VNetwork::VNetMessage::SPtr  _Msg;
    KDeserializer::SPtr          _Deserializer;
    PVOID                        _GenReceiverInf;  // For forward decl

};

///////////////////////////////////////////////////////////////////////////////
//
//
// KNetwork
//
// Represents a manager for general 'network' operations.  This is the
// root object for creating bindings with remote clients.
//
// Note that in peer-to-peer scenarios, the remote client might establish
// the binding to the current computer first.   In that case, GetBinding
// will return the binding that already exists.
//
class KNetwork : public KAsyncContextBase {

    K_FORCE_SHARED(KNetwork);

public:

    // Create
    //
    // Creates a new network object
    //
    // Parameters:
    //      Flags               Reserved.
    //      Alloc               Allocator to use and pass down the chain of objects
    //      Network             Receives the new network object.
    //
    // Return values:
    //
    //      STATUS_INSUFFICIENT_RESOURCES
    //      STATUS_INVALID_PARAMETER_1 through STATUS_INVALID_PARAMETER_2
    //      STATUS_UNEXPECTED_NETWORK_ERROR on network failure.
    //
    static NTSTATUS
    Create(
        __in  ULONG Flags,
        __in  KAllocator& Alloc,
        __out KNetwork::SPtr& Network,
        __in_opt KAsyncContextBase* const ParentAsyncContext = nullptr,
        __in_opt CompletionCallback CallbackPtr = nullptr
        );

    // ReceiverType (deprecated)
    //
    // This delegate models the callback type for incoming messages.
    // User must must implement methods of this signature to receive
    // incoming messages or replies to messages which are sent.
    //
    // Parameters:
    //      To            The local endpoint to which the message was being sent.
    //      From          The remote endpoint which was the originator of the message.
    //      Flags         Reserved
    //      ContextValue  The value supplied when registering this endpoint via the
    //                    RegisterReceiver() call.
    //      Binding       A binding that can be used to reply to the message.
    //      Message       The incoming datagram message.
    //
    typedef KDelegate<
        VOID(
            __in const GUID& To,
            __in const GUID& From,
            __in const ULONG Flags,
            __in const PVOID ContextValue,
            __in KNetBinding::SPtr& Binding,
            __in KInDatagram::SPtr& Message
            )>
            ReceiverType;


    // KNetReceiverType (use this instead of ReceiverType)
    //
    // This delegate models the callback type for incoming messages.
    // User must implement methods of this signature to receive
    // incoming messages or replies to messages which are sent.
    //
    // Parameters:
    //      Binding       A binding that can be used to reply to the message.
    //      Message       The incoming datagram message.
    //
    typedef KDelegate<
        VOID(
            __in KNetBinding::SPtr& Binding,
            __in KInDatagram::SPtr& Message
            )>
            KNetReceiverType;


    //  AcceptorType
    //
    //  Defines a callback type for a filter function for accepting
    //  or rejecting incoming connections.  The return value is important
    //  because it tells the system whether to accept or refuse the connection.
    //
    //  Providing an acceptor is completely optional.
    //
    //  Parameters:
    //      From        The originating network address who is attempting the connection.
    //      Flags       Provider-specific, usually zero.
    //      Meta        A metadata package that is transport specific, which may contain
    //                  credentials, context information or other data about the entity
    //                  attempting the connection.
    //
    typedef KDelegate<
        NTSTATUS(
            __in const GUID& From,
            __in ULONG Flags,
            __in KDatagramMetadata& Meta
            )>
            AcceptorType;

    //  RegisterAcceptor
    //
    //  Registers a filter function for new connections.  This allows
    //  the caller to inspect incoming connections and to accept or
    //  refuse them.  This call is optional and if not made, the
    //  underlying transport will perform whatever security it supports
    //  on all incoming connections.
    //
    //  If this is used, it should be registered prior to publishing endpoints.
    //
    NTSTATUS
    RegisterAcceptor(
        __in ULONG Flags,
        __in AcceptorType AcceptorCallback
        );

    // RegisterReceiver
    //
    // Registers a datagram receiver KDelegate.
    //
    // Parameters:
    //   LocalEp        The local ep for which the receiver is being registered.
    //
    //   MessageId      The message type GUID for which this receiver is being
    //                  registered.  If NULL, this receiver will be used for all
    //                  message types sent to the endpoint specified in LocalEp.
    //
    //   KConstruct     A reference to the K_CONSTRUCT implementation for the class
    //                  implied by the GUID 'MessageType'.   Use the K_CONSTRUCT_REF
    //                  helper macro with the Class name.
    //
    //   Flags          Reserved
    //
    //   UserContext    An arbitrary pointer value that will be pass through to the
    //                  receiver when a message arrives and is dispatched for
    //                  deserialization.
    //
    //   Receiver       The callback which will receive the deserialized objects.
    //
    // Return values:
    //   STATUS_SUCCESS
    //   STATUS_INVALID_PARAMETER_1 through STATUS_INVALID_PARAMETER_6
    //   STATUS_INSUFFICIENT_RESOURCES
    //   STATUS_DUPLICATE_NAME         if a re-registration of the same endpoint is attempted.
    //   STATUS_DUPLICATE_OBJECTID     if the message type already has a registration for this endpoint.
    //
    template <class T>
    NTSTATUS
    RegisterReceiver(
        __in const GUID& LocalEp,
        __in const GUID& MessageId,
        __in ULONG Flags,
        __in PVOID UserContext,
        __in ReceiverType Receiver
        )
    {
        // The template allows us to capture the pointers to the K_CONSTRUCT and K_DESERIALIZE implementations.
        // We are essentially using the class T as a parameter.
        //
        return RegisterTypedReceiver(
            PfnKConstruct((NTSTATUS (*)(KInChannel& In, T*& _This))  __KConstruct),      // Capture pointer to KConstruct and erase its type
            PfnKDeserialize(((KInChannel& (*)(KInChannel& In, T& _This))  operator >>)), // Capture pointer to operator >> and erase its type
            LocalEp,
            MessageId,
            Flags,
            UserContext,
            Receiver
            );
    }


    // RegisterReceiver (overload)
    //
    // Registers a receiver and returns a new-style KNetworkEndpoint.
    //
    // Parameters:
    //      RelativeOrFullUri       If a relative URI, the full URI will be composed based on the current KNetwork root URI.
    //                              If a full URI, it must be a prefix match for the current KNetwork root URI.
    //      MessageId               A message type ID to be associated with this URI.
    //      UserContext             Arbitrary user data
    //      Receiver                The message receiver callback.
    //      NewEp                   The resulting KNetworkEndpoint for this receiver.
    //
    template <class T>
    NTSTATUS
    RegisterReceiver(
        __in  KUriView&   RelativeOrFullUri,
        __in  const GUID& MessageId,
        __in  PVOID UserContext,
        __in  KNetReceiverType Receiver,
        __out KNetworkEndpoint::SPtr& NewEp
        )
     {
        return RegisterTypedReceiver(
            RelativeOrFullUri,
            PfnKConstruct((NTSTATUS (*)(KInChannel& In, T*& _This))  __KConstruct),      // Capture pointer to KConstruct and erase its type
            PfnKDeserialize(((KInChannel& (*)(KInChannel& In, T& _This))  operator >>)), // Capture pointer to operator >> and erase its type
            MessageId,
            UserContext,
            Receiver,
            NewEp
            );
     }


    // UnregisterReceiver
    //
    // Unregisters a previously registered receiver.
    //
    // Calls in progress will complete, but after this call returns no further messages
    // will be dispatched to the receivers that were unregistered.
    //
    // Parameters:
    //    LocalEp           The endpoint for which a receiver will be unregistered.
    //    MessageId         The message-specific receiver to deactivate. If NULL
    //                      then all receivers for the specified endpoint are deactivated.
    //
    //    Flags             Reserved
    // Return value:
    //    STATUS_SUCESS
    //    STATUS_INVALID_ADDRESS  The LocalEp is not valid or registered as an endpoint.
    //    STATUS_NOT_FOUND        The specfic message type was not found as a registered receiver type.
    //
    NTSTATUS
    UnregisterReceiver(
        __in const GUID& LocalEp,
        __in const GUID* MessageId,
        __in ULONG Flags
        );


    // UnregisterReceiver (overload)
    //
    // Unregisters a receiver previously registered with the new-style RegisterReceiver which returns a
    // KNetworkEndpoint.
    //
    NTSTATUS
    UnregisterReceiver(
        __in const GUID& MessageId,
        __in KNetworkEndpoint::SPtr Ep
        );

    // GetBinding
    //
    // Creates or retrieves a logical binding/connection to a remote endpoint.  Internally, this may
    // take the form of one or more physical connections using one or more protocols.
    //
    // This API will expand in the future to contain credentials, specify options
    // on protocols and the number of connections, etc.
    //
    // Parameters:
    //      RemoteEp            The remote endpoint to which a binding/connection is needed.
    //
    //      OriginatingLocalEp  The local endpoint which should be used as the 'originator' of
    //                          datagrams sent using this binding.   Since the user can publish
    //                          more than one endpoint, the receiver of the datagrams needs to know
    //                          which endpoint sent it in order to reply properly.
    //
    //                          If the underlying GUID is a null guid, then the 'default' endpoint
    //                          will be used.
    //
    //      Flags       Reserved
    //
    //      Binding     Receives the network binding for the remote connection. This binding
    //                  is typically not fully connected on return, but will resolve and
    //                  complete asynchronously.  To test the connction status or ping
    //                  the remote endpoint, see the methods on KNetBinding.
    //                  Letting the Binding SPtr go out of scope will close and deallocate
    //                  any connections associated with it.
    //
    //  Return value:
    //      STATUS_SUCCESS
    //      STATUS_INVALID_PARAMETER_1 through STATUS_INVALID_PARAMETER_4
    //      STATUS_INSUFFICIENT_RESOURCES
    //      STATUS_INTERNAL_ERROR
    //      STATUS_INVALID_ADDRESS
    //
    NTSTATUS
    GetBinding(
        __in  GUID& RemoteEp,
        __in  GUID& OriginatingLocalEp,
        __in  ULONG Flags,
        __out KNetBinding::SPtr& Binding
        );

    // GetBinding (overload)
    //
    // This overload retrieves a binding based on KNetworkEndpoints.
    //
    // Parameters:
    //      LocalSenderEp       This endpoint object must have been obtained via
    //                          the new overload of RegisterReceiver.
    //
    //      RemoteReceiverEp    This endpoint is not validated and does not have
    //                          to be registered ahead of time.  It is presumed
    //                          to be correct and is indirectlyl checked when the
    //                          caller tries to send a message to it later.
    //
    //      Binding             Receives the binding.
    //
    //
    NTSTATUS
    GetBinding(
        __in  KNetworkEndpoint::SPtr& LocalSenderEp,
        __in  KNetworkEndpoint::SPtr& RemoteReceiverEp,
        __out KNetBinding::SPtr& Binding
        );

    // MakeLocalEndpoint
    //
    // Creates a KNetworkEndpoint by using the already-existing authority
    // associated with the current KNetwork instance and appending the RelativeUri to it.
    //
    // This can be used to create a KNetworkEndpoint for use in Bindings
    // that have no corresponding receiver.
    //
    NTSTATUS
    MakeLocalEndpoint(
        __in      KUriView& RelativeUri,
        __out     KNetworkEndpoint::SPtr& FullEp
        );

    // RegisterEp
    //
    // Registers an endpoint URI under a caller-provided GUID, which will become the
    // handle for that endpoint subsequent API calls.
    //
    // Parameters:
    //    Uri           The string-based URI that is being registered.
    //    EpType        A bitmask describing the endpoint.
    //
    //                  If eLocalEp, you are registering the endpoint for inbound
    //                  traffic as a destination for other remote clients.
    //
    //                  If eRemoteEp, you are registering this purely as a handle
    //                  for outbound traffic only.
    //
    //                  You may use a bitmask of eLocalEp|eRemoteEp if you plan
    //                  to send messages to yourself.
    //
    //    GUID          The GUID the caller wishes to associated with the URI.
    //                  Note that this is an in-parameter.
    //
    // Return values:
    //
    //      STATUS_OBJECT_NAME_EXISTS if the root URI already exists on the network.
    //      STATUS_OBJECTID_EXISTS    if the GUID is already associated with some other Uri.
    //
    //      STATUS_INSUFFICIENT_RESOURCES
    //      STATUS_INVALID_PARAMETER_1 through STATUS_INVALID_PARAMETER_3
    //      STATUS_UNEXPECTED_NETWORK_ERROR on network failure.
    //      STATUS_SUCCESS
    //
    enum { eLocalEp = 0x1, eRemoteEp = 0x2 };

    NTSTATUS
    RegisterEp(
        __in  KWString& Uri,
        __in  ULONG EpType,
        __in  GUID& Endpoint
        );

    // GetEp
    //
    // Translates a Uri to its endpoint URI.
    //
    // Parameters:
    //
    //
    NTSTATUS
    GetEp(
        __in   KWString& Uri,
        __out  ULONG& EpType,
        __out  GUID& Endpoint
        );

    // GetEp
    //
    // Translates the GUID Ep to its equivalent string URI
    //
    NTSTATUS
    GetEp(
        __in  GUID& Endpoint,
        __out ULONG& EpType,
        __out KWString& Uri
        );

    // GetFaultInjector
    //
    // Returns a shared pointer to the fault injector.
    //
    static NTSTATUS
    GetFaultInjector(
        __out KNetFaultInjector::SPtr& Injector
        );

    static VOID
    EnableFaultInjection();

    static VOID
    DisableFaultInjection();

    // Shutdown
    //
    // This should be called when the KNetwork instance is no longer needed.
    // Letting the KNetwork object go out of scope without calling this
    // may cause a garbage-collection effect and slow down performance of
    // the system.
    //
    // TBD: This can be removed once we have async improvements
    //      We should just be able to let the object go out of scope
    //      to shut it down
    //
    VOID
    Shutdown();

private:  // Functions

    class EpEntry : public KShared<EpEntry>, KObject<EpEntry> {

        K_FORCE_SHARED(EpEntry);

    public:

        KGuid             _Ep;
        ULONG             _Flags;
//        KWString          _Uri;
        KNetUriInfo::SPtr _UriInfo;

    private:

        friend class KNetwork;

    };


    // ReceiverInfo
    //
    // These are the objects used in the _ReceiverTable.  The newer style
    // of this uses a KNetworkEndpoint, and the old style uses a GUID.
    // In cases where KNetworkEndpoint is used, the GUID is autopopulated.
    //
    //
    class ReceiverInfo : public KShared<ReceiverInfo>, KObject<ReceiverInfo> {

        K_FORCE_SHARED(ReceiverInfo);

        ReceiverInfo(__in KNetwork::SPtr Network)
        {
            _KConstruct = nullptr;
            _KDeserialize = nullptr;
            _Flags = 0;
            _UserContext = nullptr;
            _Network = Ktl::Move(Network);

            //
            // The ReceiverInfo structure is referenced while any callouts are
            // active. By holding an Activies reference to the KNetwork object while
            // ReceiverInfo is alive the KNetwork is prevented from completing.
            //
            _Network->AcquireActivities(1);
        }

    public:

        friend KNetwork;

        KNetworkEndpoint::SPtr _LocalNetEp;
        KGuid                  _LocalEp;
        KGuid                  _MessageId;
        PfnKConstruct          _KConstruct;
        PfnKDeserialize        _KDeserialize;
        ULONG                  _Flags;
        PVOID                  _UserContext;
        ReceiverType           _OldReceiver;
        KNetReceiverType       _KNetReceiver;
        KNetwork::SPtr         _Network;

    };

    NTSTATUS
    CreateDatagramReadOp(
        __out KDatagramReadOp::SPtr& NewReadOp
        )
    {
        NewReadOp = _new (KTL_TAG_NET, GetThisAllocator()) KDatagramReadOp;
        if (!NewReadOp)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        return STATUS_SUCCESS;
    }

    EpEntry::SPtr
    LookupByUri(
        __in KWString& Uri,
        __in BOOLEAN AcquireLock
        );

    EpEntry::SPtr
    LookupByGUID(
        __in GUID& Guid,
        __in BOOLEAN AcquireLock
        );

    NTSTATUS
    RegisterTypedReceiver(
        __in PfnKConstruct KConstruct,
        __in PfnKDeserialize KDeserializer,
        __in const GUID& LocalEp,
        __in const GUID& MessageId,
        __in ULONG Flags,
        __in PVOID UserContext,
        __in ReceiverType Receiver
        );

    NTSTATUS
    RegisterTypedReceiver(
        __in KUriView& LocalRelativeUri,
        __in PfnKConstruct KConstruct,
        __in PfnKDeserialize KDeserializer,
        __in const GUID& MessageId,
        __in PVOID UserContext,
        __in KNetReceiverType Receiver,
        __out KNetworkEndpoint::SPtr& Ep
        );

    NTSTATUS
    MakeAndRegisterLocalEndpoint(
        __in  KUriView& PathOnlyUri,
        __out KNetworkEndpoint::SPtr& FullEp,
        __out GUID& NewGuid
        );

    NTSTATUS
    FindReceiverInfo(
        __in const GUID& MessageId,
        __out ReceiverInfo::SPtr& ReceiverInf
        );

    VOID
    OnStart();

    friend class VNetwork;
    friend class KDatagramReadOp;
    friend class KDatagramWriteOp;
    friend class KNetBinding;
    friend class KNetHeaderReadOp;

    KArray<EpEntry::SPtr>         _EpTable;
    KArray<ReceiverInfo::SPtr>    _ReceiverTable;
    KSpinLock                     _TableLock;
    KSharedPtr<KNetServiceLayer>  _SvcLayer;
};

///////////////////////////////////////////////////////////////////////////////
//

class KNetHostFile : public KObject<KNetHostFile> {

public:

    KNetHostFile(
        __in KAllocator& Alloc
        ) :
        _Output(Alloc)
    {
        _Allocator = &Alloc;
    }

    NTSTATUS
    Parse(
        __in  KWString& PathToFile,
        __out ULONG& ErrorLine
        );

    KArray<KNetUriInfo::SPtr> _Output;

private:

    BOOLEAN
    IsHexDigit(WCHAR c);

    BOOLEAN
    IsDigit(WCHAR c);

    BOOLEAN
    IsWhitespace(WCHAR c);

    NTSTATUS
    StripComment(
        _Outref_result_buffer_(Count) WCHAR*& Current,
        _Inout_ ULONG& Count
        );

    NTSTATUS
    TryGuid(
        _Outref_result_buffer_(Count) WCHAR*& Current,
        _Inout_ ULONG& Count,
        __out   KGuid& Guid
        );

    NTSTATUS
    TryPort(
        _Outref_result_buffer_(Count) WCHAR*& Current,
        _Inout_ ULONG& Count,
        __out   USHORT& Port
        );

    NTSTATUS
    TryUri(
        _Outref_result_buffer_(Count) WCHAR*& Current,
        _Inout_ ULONG& Count,
        __in    USHORT DefaultPort,
        __out   KWString& FullUri,
        __out   KWString& Server,
        __out   KWString& RelativeUri,
        __out   USHORT&   Port,
        __out   ULONG&    Flags
        );

    NTSTATUS
    ParseLine(
        __in  KWString& Line,
        __out KGuid&    Guid,
        __out USHORT&   Port,
        __out KWString& FullUri,
        __out KWString& ServerStr,
        __out KWString& RelativeUri,
        __out ULONG&    Flags
        );

    KAllocator* _Allocator;

};

///////////////////////////////////////////////////////////////////////////////
//
// KNetServiceLayerStartup
//

class KNetServiceLayerStartup : public KAsyncContextBase {

    K_FORCE_SHARED(KNetServiceLayerStartup);

public:

    NTSTATUS
    StartNetwork(
        __in       KNetworkEndpoint::SPtr RootEndpoint,
        __in_opt   KAsyncContextBase::CompletionCallback Callback,
        __in_opt   KAsyncContextBase* const ParentContext = nullptr
        );

    VOID
    ZeroCounters();

private:

    VOID
    OnStart();

    VOID
    Completed(
        __in NTSTATUS Status,
        __in PVOID Ctx
        );

    friend class KNetServiceLayer;

    KNetServiceLayer* _SvcLayer;

};


///////////////////////////////////////////////////////////////////////////////
//
// KNetServiceLayer
//
// This is the actual network-aware dispatching layer for any plug-in transport.
//

class KNetServiceLayer : public KObject<KNetServiceLayer>, public KShared<KNetServiceLayer> {

    K_FORCE_SHARED(KNetServiceLayer);

    KNetServiceLayer(
        __in KRvdTcpNetworkEndpoint::SPtr RootEp
        );

public:

    typedef KDelegate<VOID(NTSTATUS Result, PVOID Ctx)> StatusCallback;
    typedef KDelegate<NTSTATUS(ULONG MessageId, KWString& Result)> SysMessageHandler;

    static
    NTSTATUS
    CreateStartup(
        __in  KAllocator& Allocator,
        __out KNetServiceLayerStartup::SPtr& Op
        )
    {
        KNetServiceLayerStartup* Tmp = _new(KTL_TAG_NET, Allocator) KNetServiceLayerStartup;
        if (!Tmp)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Op = Tmp;
        return STATUS_SUCCESS;
    }

    VOID
    NextSequentialGuid(
        __out GUID& NewGuid
        );

    static
    KNetServiceLayer::SPtr
    Get()
    {
        if (_gSvcLayer)
        {
            return *_gSvcLayer;
        }
        else
        {
            return KNetServiceLayer::SPtr();
        }
    }

    static VOID
    Shutdown();

    VOID
    _Shutdown();

    NTSTATUS
    PublishEp(
        __in KNetUriInfo::SPtr NewUriInf,
        __in GUID&             EpGuid,
        __in KNetwork::SPtr    OwningNetwork
        );

    NTSTATUS
    UnpublishEp(
        __in KWString& RelUri
        );

    NTSTATUS
    RevokeEp(
        __in KWString* &Uri
        );

    NTSTATUS
    CreateAcquireSocketOp(
        __out KSharedPtr<KNetAcquireSocketOp>& NewOp
        );

    NTSTATUS
    LoadExternalAddressFile();

    NTSTATUS
    RegisterSystemMessageHandler(
        __in ULONG Id,
        __in SysMessageHandler CBack
        );

    NTSTATUS
    AddAddressEntry(
        __in KNetUriInfo::SPtr& Inf
        );

    VOID
    Startup(
        __in StatusCallback StartupCallback,
        __in PVOID Ctx
        );

    NTSTATUS
    UriToGuid(
        __in KWString& Uri,
        __out GUID& Guid
        );

    NTSTATUS
    GuidToUri(
        __in GUID& Guid,
        __out KWString& Uri,
        __out USHORT& RemotePort
        );

    VOID
    DumpAddressTableToDebugger();

    VOID
    DumpEpTableToDebugger();

    KRvdTcpNetworkEndpoint::SPtr
    GetRootEp()
    {
        return _RootEp;
    }


private:

    NTSTATUS
    BuildLookupTables(
        __in KArray<KNetUriInfo::SPtr>& Src
        );

    VOID
    OnAcceptComplete(
        KAsyncContextBase* const Parent,
        KAsyncContextBase& Op
        );

    VOID
    PrepostRead(
        __in ISocket::SPtr& Socket
        );

    VOID
    OnPrepostReadComplete(
        KAsyncContextBase* const Parent,
        KAsyncContextBase& Op
        );

    NTSTATUS
    PostAccept();

    VOID
    RemoveSocketFromInboundTable(
        __in ISocket::SPtr& Socket
        );

    NTSTATUS
    AddSocketToInboundTable(
        __in ISocket::SPtr& Socket
        );

    class PublishedEpEntry : public KShared<PublishedEpEntry>, public KObject<PublishedEpEntry> {

        K_FORCE_SHARED(PublishedEpEntry);

    public:

        KNetUriInfo::SPtr   _UriInfo;
        KNetwork::SPtr      _OwningNetwork;

    private:

        friend class KNetServiceLayer;

    };

    class SocketEntry : public KShared<SocketEntry>, public KObject<SocketEntry> {

        K_FORCE_SHARED(SocketEntry);

    public:

        KSharedAsyncLock::SPtr _Lock;
        KWString               _Server;
        USHORT                 _Port;
        ULONG                  _State;
        BOOLEAN                _Ready;
        BOOLEAN                _Inbound;
        BOOLEAN                _Outbound;
        INetAddress::SPtr      _Address;
        ISocket::SPtr          _Socket;

    private:

        friend class KNetAcquireSocketOp;

    };

    class EpEntry : public KShared<EpEntry>, public KObject<EpEntry> {

        K_FORCE_SHARED(EpEntry);

    public:

        KNetUriInfo::SPtr   _UriInfo;

    private:

        friend class KNetServiceLayer;

    };

    friend class KNetHeaderReadOp;
    friend class KNetAcquireSocketOp;
    friend class KNetServiceLayerStartup;

    static KNetServiceLayer::SPtr* _gSvcLayer;

    KHashTable<KWString, PublishedEpEntry::SPtr>   _PublishedEpTable;
    KHashTable<KWString, EpEntry::SPtr>            _AddressTableByFullUri;
    KHashTable<GUID,     EpEntry::SPtr>            _AddressTableByGuid;
    KHashTable<KWString, SocketEntry::SPtr>        _SocketTable;
    KArray<ISocket::SPtr>                          _InboundSocketTable;

    ITransport::SPtr  _Transport;

    KArray<INetAddress::SPtr> _LocalServerAddresses;
    INetAddress::SPtr         _LocalClientAddress;

    KSpinLock               _TableLock;

    KRvdTcpNetworkEndpoint::SPtr    _RootEp;
    ISocketListener::SPtr           _Listener;
    BOOLEAN                         _ShutdownFlag;
    StatusCallback                  _StartupCallback;
    PVOID                           _StartupCtx;

};


///////////////////////////////////////////////////////////////////////////////
//
//   KNetAcquireSocketOp
//
class KNetAcquireSocketOp : public KAsyncContextBase {

    K_FORCE_SHARED(KNetAcquireSocketOp);

    KNetAcquireSocketOp(
        __in KNetServiceLayer* SvcLayer
        )
    {
        _SvcLayer = SvcLayer;
        _TargetPort  = 0;
    }

public:

    NTSTATUS
    StartAcquire(
        __in KNetUriInfo::SPtr Dest,
        __in USHORT TargetPort,
        __in_opt   KAsyncContextBase::CompletionCallback Callback,
        __in_opt   KAsyncContextBase* const ParentContext = nullptr
        );

    ISocket::SPtr
    GetSocket()
    {
        return _Socket;
    }

private:

    VOID
    LockCallback(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& Op
        );

    VOID
    OnAddressComplete(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& Op
        );

    VOID
    OnSocketComplete(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& Op
        );

    VOID
    OnStart();

    friend class KNetServiceLayer;

    KSharedPtr<KNetServiceLayer>        _SvcLayer;
    KNetUriInfo::SPtr                   _Dest;
    KSharedAsyncLock::Handle::SPtr      _LockHandle;
    KNetServiceLayer::SocketEntry::SPtr _SockEnt;
    ISocket::SPtr                       _Socket;
    LONGLONG                            _StartTime;
    USHORT                              _TargetPort;

};

//////////////////////////////////////////////////////////////////////////////////////
//

//
//  KNetHeaderReadOp
//
//  Used for incoming messages.  Reads the header and URI values and then
//  dispatches to the owning KNetwork instance.
//
//
class KNetHeaderReadOp : public KAsyncContextBase {

    K_FORCE_SHARED(KNetHeaderReadOp);

public:

    static NTSTATUS
    Create(
        __in  KAllocator& Allocator,
        __out KNetHeaderReadOp::SPtr& NewOp
        );

    NTSTATUS
    StartRead(
        __in ISocket::SPtr& Socket,
        __in KNetServiceLayer::SPtr SvcLayer,
        __in_opt   KAsyncContextBase::CompletionCallback Callback,
        __in_opt   KAsyncContextBase* const ParentContext = nullptr
        );

    ISocket::SPtr
    GetSocket()
    {
        return _Socket;
    }

private:

    NTSTATUS
    Initialize(
        );

    VOID
    OnStart();

    VOID
    Phase1ReadComplete(
        KAsyncContextBase* const Parent,
        KAsyncContextBase& Op
        );

    VOID
    Phase2ReadComplete(
        KAsyncContextBase* const Parent,
        KAsyncContextBase& Op
        );

    VOID
    StripMessage();

    VOID
    StripReadComplete(
        KAsyncContextBase* const Parent,
        KAsyncContextBase& Op
        );

    VOID
    DatagramReadComplete(
        KAsyncContextBase* const Parent,
        KAsyncContextBase& Op
        );

    VOID
    SystemMessageWriteComplete(
        KAsyncContextBase* const Parent,
        KAsyncContextBase& Op
        );

    VOID
    HandleSystemMessage();

    friend class KNetServiceLayer;

    KNetServiceLayer::SPtr _SvcLayer;
    ISocket::SPtr          _Socket;
    KNetworkHeader         _Header;
    KActivityId            _ActId;

    PUCHAR         _UriBuffer;
    UNICODE_STRING _SystemMessage;

    KNetUriInfo::SPtr       _DestUriInf;
    KNetUriInfo::SPtr       _SrcUriInf;

    ULONG                   _StripSizeRequired;
    KBuffer::SPtr           _StripBuffer;
};
