/*++

    (c) 2011 by Microsoft Corp. All Rights Reserved.

    ITransport.h

    Description:
      Kernel Tempate Library (KTL): ITransport.h

      Defines abstract socket-like interfaces for network transport providers.
      Specific implementations, such as for TCP, should derive
      concrete classes from each interface.

    History:
      raymcc          2-May-2011         Initial draft

--*/

#pragma once

class ISocket;

class INetAddress : public KObject<INetAddress>, public KShared<INetAddress>
{
public:
    typedef KSharedPtr<INetAddress> SPtr;
    enum { eNone = 0, eWinsock_IPv4 = 1, eWSK_IPv4, eWinsock_IPv6, eWSK_IPv6 };

    virtual PSOCKADDR Get() = 0;

    virtual ULONG
    GetFormat() = 0;

    virtual PVOID
    GetUserData() = 0;

    virtual VOID
    SetUserData(
        __in PVOID Data
        ) = 0;

    virtual size_t
    Size() = 0;

    virtual NTSTATUS ToStringW(__out_ecount(len) PWSTR buf, __inout ULONG & len) const = 0;

    virtual NTSTATUS ToStringA(__out_ecount(len) PSTR buf, __inout ULONG & len) const = 0;

    virtual ~INetAddress(){}
};

#if KTL_IO_TIMESTAMP

class KIoTimestamps
{
public:
    KIoTimestamps();
    void Reset();

    ULONGLONG IoStartTime() const;
    void RecordIoStart();

    ULONGLONG IoCompleteTime() const;
    void RecordIoComplete();

    static void RecordCurrentTime(_Out_ ULONGLONG & timestamp);

private:
    ULONGLONG _ioStartTime;
    ULONGLONG _ioCompleteTime;
};

class KSspiTimestamps
{
public:
    KSspiTimestamps();

    void Reset();
    
    // encryption here means encryption or decryption
    ULONGLONG EncryptStartTime() const;
    void RecordEncryptStart();

    ULONGLONG EncryptCompleteTime() const;
    void RecordEncryptComplete();

    ULONGLONG NegotiateStartTime() const;
    void RecordNegotiateStart();

    ULONGLONG NegotiateCompleteTime() const;
    void RecordNegotiateComplete();

private:
    // Only the first send leads to negotiate operations, _negotiateStartTime and _negotiateCompleteTime are set to zero after the first send.
    ULONGLONG _negotiateStartTime;
    ULONGLONG _negotiateCompleteTime;

    ULONGLONG _encryptStartTime;
    ULONGLONG _encryptCompleteTime;
};

#endif

//
//  ISocketWriteOp
//
//  Models an individual async write to an abstract socket.
//
class ISocketWriteOp : public KAsyncContextBase
{
public:
    typedef KSharedPtr<ISocketWriteOp> SPtr;
    virtual ~ISocketWriteOp(){}

    enum { ePriorityNormal = 2, ePriorityHigh = 1, ePriorityLow = 3 };

    virtual NTSTATUS
    StartWrite(
        __in       ULONG BufferCount,
        __in       KMemRef* Buffers,
        __in       ULONG Priority,
        __in_opt   KAsyncContextBase::CompletionCallback Callback = 0,
        __in_opt   KAsyncContextBase* const ParentContext = nullptr
        ) = 0;


    virtual VOID
    OnStart() = 0;

    virtual KSharedPtr<ISocket>
    GetSocket() = 0;

    virtual ULONG
    BytesToTransfer() = 0;

    virtual ULONG
    BytesTransferred() = 0;

    virtual VOID
    SetUserData(
        __in PVOID Ptr
    ) = 0;

    virtual PVOID
    GetUserData() = 0;

#if KTL_IO_TIMESTAMP
    virtual KIoTimestamps const * IoTimestamps() { return nullptr; }
    virtual KSspiTimestamps const * SspiTimestamps() { return nullptr; }
#endif
};


// ISocketReadOp
//
// Models an individual async read from an abstract socket.
//
class ISocketReadOp : public KAsyncContextBase
{
public:
    typedef KSharedPtr<ISocketReadOp> SPtr;
    enum { ReadAny = 0x0, ReadAll = 0x1 };

    virtual ~ISocketReadOp(){}

    virtual NTSTATUS
    StartRead(
        __in       ULONG BufferCount,
        __in       KMemRef* Buffers,
        __in       ULONG Flags,
        __in_opt   KAsyncContextBase::CompletionCallback Callback,
        __in_opt   KAsyncContextBase* const ParentContext = nullptr
        ) = 0;

    virtual VOID
    OnStart() = 0;

    virtual KSharedPtr<ISocket>
    GetSocket() = 0;

    virtual ULONG
    BytesToTransfer() = 0;

    virtual ULONG
    BytesTransferred() = 0;

    virtual VOID
    SetUserData(
        __in PVOID Ptr
    ) = 0;

    virtual PVOID
    GetUserData() = 0;

#if KTL_IO_TIMESTAMP
    virtual KIoTimestamps const * IoTimestamps() { return nullptr; }
    virtual KSspiTimestamps const * SspiTimestamps() { return nullptr; }
#endif
};




//  ISocket
//
//  Represents a connection or binding between a client & server.
//
class ISocket : public KObject<ISocket>, public KShared<ISocket>
{
public:
    typedef KSharedPtr<ISocket> SPtr;

    virtual ~ISocket(){}

    virtual VOID
    Close() = 0;

    virtual NTSTATUS
    CreateReadOp(
        __out ISocketReadOp::SPtr& NewOp
        ) = 0;

    virtual NTSTATUS
    CreateWriteOp(
        __out ISocketWriteOp::SPtr& NewOp
        ) = 0;

    virtual ULONG
    GetMaxMessageSize() = 0;

    virtual NTSTATUS
    GetLocalAddress(
        __out INetAddress::SPtr& Addr
        ) = 0;

    virtual NTSTATUS
    GetRemoteAddress(
        __out INetAddress::SPtr& Addr
        ) = 0;

    virtual ULONG
    PendingOps() = 0;

    virtual VOID
    SetUserData(
        __in PVOID Ptr
    ) = 0;

    virtual PVOID
    GetUserData() = 0;

    virtual BOOLEAN
    Ok() = 0;

    virtual PVOID
    GetSocketId() = 0;

    virtual BOOLEAN
    Lock() = 0;

    virtual VOID
    Unlock() = 0;
};

//
//  ISocketConnectOp
//
//
class ISocketConnectOp : public KAsyncContextBase
{
public:
    typedef KSharedPtr<ISocketConnectOp> SPtr;

    virtual ~ISocketConnectOp(){}

    virtual NTSTATUS
    StartConnect(
        __in      INetAddress::SPtr& ConnectFrom,
        __in      INetAddress::SPtr& ConnectTo,
        __in_opt  KAsyncContextBase::CompletionCallback Callback = 0,
        __in_opt  KAsyncContextBase* const ParentContext = nullptr
        ) = 0;

    virtual VOID
    OnStart() = 0;

    virtual KSharedPtr<ISocket>
    GetSocket() = 0;

    virtual VOID
    SetUserData(
        __in PVOID Ptr
    ) = 0;

    virtual PVOID
    GetUserData() = 0;
};

//
//  ISocketAcceptOp
//
//  For receiving incoming connections in a 'server' role.  The server
//  code should keep several of these pre-posted in order to receive
//  incoming connections.
//
class ISocketAcceptOp : public KAsyncContextBase
{
public:
    typedef KSharedPtr<ISocketAcceptOp> SPtr;
    virtual ~ISocketAcceptOp() {}

    virtual NTSTATUS
    StartAccept(
        __in_opt  KAsyncContextBase::CompletionCallback Callback = 0,
        __in_opt  KAsyncContextBase* const ParentContext = nullptr
        ) = 0;

    virtual VOID
    OnStart() = 0;

    virtual KSharedPtr<ISocket>
    GetSocket() = 0;

    virtual VOID
    SetUserData(
        __in PVOID Ptr
    ) = 0;

    virtual PVOID
    GetUserData() = 0;
};

//
//  ISocketListenOp
//
//  For receiving incoming connections in a 'server' role.  The server
//  code should keep several of these pre-posted in order to receive
//  incoming connections.
//
class ISocketListenOp : public KAsyncContextBase
{
public:
    typedef KSharedPtr<ISocketListenOp> SPtr;
    virtual ~ISocketListenOp() {}

    virtual NTSTATUS
    StartListen(
        __in_opt  KAsyncContextBase::CompletionCallback Callback = 0,
        __in_opt  KAsyncContextBase* const ParentContext = nullptr
        ) = 0;

    virtual VOID
    OnStart() = 0;

    virtual VOID
    SetUserData(
        __in PVOID Ptr
    ) = 0;

    virtual PVOID
    GetUserData() = 0;
};


//  ISocketListener
//
//  Represents an abstract listener for new incoming connections.  The listener
//  remains active until it is Shutdown(), after which it will no longer accept
//  new incoming connections.  After that, previously existing connections will have
//  to be shut down individually.
//
class ISocketListener: public KObject<ISocketListener>, public KShared<ISocketListener>
{
public:
    typedef KSharedPtr<ISocketListener> SPtr;
    virtual ~ISocketListener(){}

    virtual NTSTATUS
    CreateAcceptOp(
        __out ISocketAcceptOp::SPtr& Accept
        ) = 0;

    virtual NTSTATUS
    CreateListenOp(
        __out ISocketListenOp::SPtr& Accept
        ) = 0;

    virtual VOID
    SetUserData(
        __in PVOID Ptr
    ) = 0;

    virtual PVOID
    GetUserData() = 0;

    virtual VOID
    Shutdown() = 0;

    virtual VOID
    SetAcceptTerminated() = 0;

    virtual BOOLEAN
    AcceptTerminated() = 0;
};

// IBindAddressOp
//
// Used to bind/create an address object for use in socket connctions & accepts.
//
class IBindAddressOp : public KAsyncContextBase
{
public:
    typedef KSharedPtr<IBindAddressOp> SPtr;
    virtual ~IBindAddressOp(){ }

    virtual NTSTATUS
    StartBind(
        __in PWSTR  StringAddress,
        __in USHORT Port,
        __in_opt    KAsyncContextBase::CompletionCallback Callback = 0,
        __in_opt    KAsyncContextBase* const ParentContext = nullptr
        ) = 0;

    virtual NTSTATUS
    StartBind(
        __in PWSTR  StringAddress,
        __in ADDRESS_FAMILY addressFamily,
        __in USHORT Port,
        __in_opt    KAsyncContextBase::CompletionCallback Callback = 0,
        __in_opt    KAsyncContextBase* const ParentContext = nullptr
        ) = 0;

    virtual NTSTATUS
    StartBind(
        _In_ PWSTR  StringAddress,
        ADDRESS_FAMILY addressFamily,
        USHORT Port,
        PCWSTR sspiTarget,
        _In_opt_ KAsyncContextBase::CompletionCallback Callback = 0,
        _In_opt_ KAsyncContextBase* const ParentContext = nullptr
        ) = 0;

    virtual VOID
    SetUserData(
        __in PVOID Ptr
    ) = 0;

    virtual PVOID
    GetUserData() = 0;

    virtual INetAddress::SPtr const & GetAddress() = 0;
};

//
//  ITransport
//
//  Abstraction for a general-purpose transport (TCP, UDP, RDMA, etc.)
//
class ITransport : public KObject<ITransport>, public KShared<ITransport>
{
public:
    typedef KSharedPtr<ITransport> SPtr;

    virtual ~ITransport(){}

    virtual NTSTATUS CreateNetAddress(
        SOCKADDR_STORAGE const * address,
        __out INetAddress::SPtr & netAddress) = 0;

    virtual NTSTATUS
    CreateBindAddressOp(
        __out IBindAddressOp::SPtr& Bind
        ) = 0;

    virtual NTSTATUS
    CreateConnectOp(
        __out ISocketConnectOp::SPtr& Connect
        ) = 0;

    virtual NTSTATUS
    CreateListener(
        __in  INetAddress::SPtr& Address,
        __out ISocketListener::SPtr& Listener
        ) = 0;

    virtual NTSTATUS
    SpawnDefaultClientAddress(
        __out INetAddress::SPtr& Address
        ) = 0;

    virtual NTSTATUS
    SpawnDefaultListenerAddresses(
        __inout KArray<INetAddress::SPtr>& Addresses
        ) = 0;

    virtual VOID
    Shutdown() = 0;
};



