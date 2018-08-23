/*++

    (c) 2011 by Microsoft Corp. All Rights Reserved.

    ksocket.h

    Description:
      Kernel Tempate Library (KTL): KSocket

      Defines generic duplex socket abstraction for use with TCP and UDP.

    History:
      raymcc          2-May-2011         Initial draft

--*/

#pragma once



// Perf counters & trackers
//
extern ULONGLONG __KSocket_BytesWritten;
extern ULONGLONG __KSocket_BytesRead;
extern ULONGLONG __KSocket_Writes;
extern ULONGLONG __KSocket_Reads;
extern LONG      __KSocket_SocketObjects;
extern LONG      __KSocket_SocketCount;
extern LONG      __KSocket_AddressCount;
extern LONG      __KSocket_AddressObjects;
extern LONG      __KSocket_WriteFailures;
extern LONG      __KSocket_ReadFailures;
extern LONG      __KSocket_ConnectFailures;
extern LONG      __KSocket_PendingOperations;
extern LONG      __KSocket_PendingReads;
extern LONG      __KSocket_PendingWrites;
extern LONG      __KSocket_PendingAccepts;
extern LONG      __KSocket_Shutdown;
extern LONG      __KSocket_ShutdownPhase;
extern LONG      __KSocket_Accepts;
extern LONG      __KSocket_AcceptFailures;
extern LONG      __KSocket_Connects;
extern LONG      __KSocket_Closes;
extern LONG      __KSocket_OpNumber;

extern LONG      __KSocket_Version;
extern LONG      __KSocket_LastWriteError;

typedef enum { eSocketEventCreate, eSocketEventDestruct, eSocketEventClose } KSocketEventType;
typedef KDelegate<VOID(ISocket::SPtr, KSocketEventType)> SocketEvent;



// Non-class helper
//
NTSTATUS
KSocket_EnumLocalAddresses(
    __in KAllocator* Alloc,
    __inout KArray<SOCKADDR_STORAGE>& Addresses
    );

///////////////////////////////////////////////////////////////////////////////
//
//  User-mode implementation (Winsock)
//
//

#if KTL_USER_MODE



struct KTL_SOCKET_OVERLAPPED : OVERLAPPED
{
    enum { eNull = 0, eTypeAccept = 1, eTypeConnect = 2, eTypeRead = 3, eTypeWrite = 4 };

    ULONG _KtlObjectType;
    PVOID _KtlObject;
    PVOID _UserData;

    KTL_SOCKET_OVERLAPPED()
    {
        _KtlObjectType = eNull;
        _KtlObject = nullptr;
        _UserData = nullptr;
    }
};

class KUmNetAddress : public INetAddress
{
    K_FORCE_SHARED(KUmNetAddress);
    friend class KUmBindAddressOp;
    friend class KUmSocketAcceptOp;
    friend class KUmSocketConnectOp;
    friend class KUmTcpTransport;

public:
    typedef KSharedPtr<KUmNetAddress> SPtr;

    USHORT
    GetPort()
    {
        return _Port;
    }

    ULONG GetFormat()
    {
        return _SocketAddress.ss_family == AF_INET ? eWinsock_IPv4 : eWinsock_IPv6;
    }

    PSOCKADDR
    Get()
    {
        return PSOCKADDR(&_SocketAddress);
    }

    size_t
    Size() override
    {
        return sizeof(SOCKADDR_STORAGE);
    }

    VOID
    SetUserData(
        __in PVOID Ptr
    )
    {
        _UserData = Ptr;
    }

    PVOID
    GetUserData()
    {
        return _UserData;
    }

    virtual NTSTATUS ToStringW(__out_ecount(len) PWSTR buf, __inout ULONG & len) const;

    virtual NTSTATUS ToStringA(__out_ecount(len) PSTR buf, __inout ULONG & len) const;

private:
    KUmNetAddress(SOCKADDR_STORAGE const* rawAddress);

    USHORT              _Port;
    KWString            _StringAddress;
    SOCKADDR_STORAGE    _SocketAddress;
    PVOID               _UserData;
};


//
//  KUmBindAddressOp
//
//  Used to create INetAddress objects from a port+string-based address (host name or IP).
//
class KUmBindAddressOp : public IBindAddressOp
{
    friend class KUmTcpTransport;
    K_FORCE_SHARED(KUmBindAddressOp);

public:
    typedef KSharedPtr<KUmBindAddressOp> SPtr;

    virtual NTSTATUS
    StartBind(
        __in PWSTR  StringAddress,
        __in USHORT Port,
        __in_opt    KAsyncContextBase::CompletionCallback Callback = 0,
        __in_opt    KAsyncContextBase* const ParentContext = nullptr
        );

    virtual NTSTATUS
    StartBind(
        __in PWSTR  StringAddress,
        __in ADDRESS_FAMILY addressFamily,
        __in USHORT Port,
        __in_opt    KAsyncContextBase::CompletionCallback Callback = 0,
        __in_opt    KAsyncContextBase* const ParentContext = nullptr
        );

    virtual NTSTATUS
    StartBind(
        _In_ PWSTR  StringAddress,
        ADDRESS_FAMILY addressFamily,
        USHORT Port,
        PCWSTR sspiTarget,
        _In_opt_ KAsyncContextBase::CompletionCallback Callback = 0,
        _In_opt_ KAsyncContextBase* const ParentContext = nullptr);

    VOID
    OnStart();

    PVOID
    GetUserData()
    {
        return _UserData;
    }

    VOID
    SetUserData(
        PVOID Ptr
        )
    {
        _UserData = Ptr;
    }

    INetAddress::SPtr const & GetAddress() { return _address; }

private:

    KUmBindAddressOp(
        __in KAllocator& Alloc
        );

private:
    KAllocator*         _Allocator;
    KUmNetAddress::SPtr _umAddress;
    INetAddress::SPtr _address;
    KWString            _StringAddress;
    ADDRESS_FAMILY _addressFamily;
    USHORT              _Port;
    LPVOID              _UserData;
};


class KUmSocket;


class KUmSocketWriteOp : public ISocketWriteOp
{
    friend class KUmSocket;
    friend class KUmTcpTransport;

public:
    typedef KSharedPtr<KUmSocketWriteOp> SPtr;

    // StartWrite
    //
    // Begins an asynchronous write to the underlying stream socket.
    //
    // Parameters:
    //      BufferCount         The number of buffers to be transmitted
    //      Buffers             A pointer to an array of KMemRef structs.
    //                          The _Param field of each struct must be set
    //                          to the number of bytes to be transmitted for that block.
    //                          This is because the blocks are likely to be allocated in page-sized chunks
    //                          and reused for various sends/receives, and the _Size field would be needed to reflect
    //                          the allocated size.
    //      Priority            The priority of the outgoing transmission.  Writes with a lower priority are
    //                          queued behind writes of higher priority.
    //      Callback            The async completion callback.
    //      ParentContext       The parent context, if any
    //
    //  Return value:
    //      STATUS_PENDING                  If successful.
    //      STATUS_INSUFFICIENT_RESOURCES   If insufficient memory to send the buffer.
    //      STATUS_INTERNAL_ERROR
    //
    virtual NTSTATUS
    StartWrite(
        __in       ULONG BufferCount,
        __in       KMemRef* Buffers,
        __in       ULONG Priority,
        __in_opt   KAsyncContextBase::CompletionCallback Callback = 0,
        __in_opt   KAsyncContextBase* const ParentContext = nullptr
        );


    VOID
    OnStart();

    ULONG
    BytesTransferred()
    {
        return _BytesTransferred;
    }


    ULONG
    BytesToTransfer()
    {
        return _BytesToTransfer;
    }

    KSharedPtr<ISocket>
    GetSocket();

    VOID
    SetUserData(
        __in PVOID Ptr
    )
    {
        _Olap._UserData = Ptr;
    }

    PVOID
    GetUserData()
    {
        return _Olap._UserData;
    }

private:
    // Private functions

    KUmSocketWriteOp(
        __in KSharedPtr<KUmSocket> Socket,
        __in KAllocator& Alloc
        );

   ~KUmSocketWriteOp();

    VOID
    IOCP_Completion(
        __in ULONG Status,
        __in ULONG BytesTransferred
        );



private:
    // Data members

    KSharedPtr<KUmSocket>   _Socket;
    KAllocator             *_Allocator;
    ULONG                   _Priority;
    KArray<WSABUF>          _Buffers;
    ULONG                   _BytesToTransfer;
    ULONG                   _BytesTransferred;
    KSpinLock               _SocketLock;
    KTL_SOCKET_OVERLAPPED   _Olap;
};



// KUmSocketReadOp
//
// Implements a read operation from a stream-oriented socket.
//
class KUmSocketReadOp : public ISocketReadOp
{
    friend class KUmSocket;
    friend class KUmTcpTransport;

public:
    typedef KSharedPtr<KUmSocketReadOp> SPtr;

    // StartRead
    //
    // Begins an asynchronous write to the underlying socket.
    //
    // Parameters:
    //      BufferCount         The number of buffers available to receive input
    //      Buffers             A pointer to an array of KMemRef structs.
    //                          The _Size field of each struct must be set
    //                          to the number of bytes available in that block. _Param is ignored on input.
    //      Callback            The async completion callback.
    //      ParentContext       The parent context, if any
    //
    //  Return value:
    //      STATUS_PENDING                  If successful.
    //      STATUS_INSUFFICIENT_RESOURCES   If insufficient memory to send the buffer.
    //      STATUS_INTERNAL_ERROR
    //
    virtual NTSTATUS
    StartRead(
        __in       ULONG BufferCount,
        __in       KMemRef* Buffers,
        __in       ULONG Flags,
        __in_opt   KAsyncContextBase::CompletionCallback Callback = 0,
        __in_opt   KAsyncContextBase* const ParentContext = nullptr
        );

    VOID
    OnStart();

    ULONG
    BytesTransferred()
    {
        return _BytesTransferred;
    }

    ULONG
    BytesToTransfer()
    {
        return _BytesToTransfer;
    }

    KSharedPtr<ISocket>
    GetSocket();

    VOID
    SetUserData(
        __in PVOID Ptr
    )
    {
        _Olap._UserData = Ptr;
    }

    PVOID
    GetUserData()
    {
        return _Olap._UserData;
    }

private:
    // Private functions

    KUmSocketReadOp(
        __in KSharedPtr<KUmSocket> Socket,
        __in KAllocator& Alloc
        );

   ~KUmSocketReadOp();

    VOID
    IOCP_Completion(
        __in ULONG Status,
        __in ULONG BytesTransferred
        );



private:
    // Data members

    KSharedPtr<KUmSocket>   _Socket;
    KAllocator             *_Allocator;
    KArray<WSABUF>          _Buffers;
    ULONG                   _BytesAvailable;
    ULONG                   _BytesTransferred;
    ULONG                   _BytesToTransfer;
    KTL_SOCKET_OVERLAPPED   _Olap;
    ULONG                   _flags;
	DWORD					_dwFlagsTemp;
};

//
//  KUmSocketAcceptOp
//
class KUmSocketAcceptOp : public ISocketAcceptOp
{
    friend class KUmSocketListener;

public:
    typedef KSharedPtr<KUmSocketAcceptOp> SPtr;

    virtual NTSTATUS
    StartAccept(
        __in_opt  KAsyncContextBase::CompletionCallback Callback = 0,
        __in_opt  KAsyncContextBase* const ParentContext = nullptr
        )
        {
            Start(ParentContext, Callback);
            return STATUS_PENDING;
        }


    virtual VOID
    OnStart();

    VOID
    SetUserData(
        __in PVOID Ptr
    )
    {
        _Olap._UserData = Ptr;
    }

    PVOID
    GetUserData()
    {
        return _Olap._UserData;
    }

    virtual KSharedPtr<ISocket>
    GetSocket()
    {
        return _Socket;
    }

    VOID
    IOCP_Completion(
        __in ULONG Status
        );

    VOID OnReuse();

private:
    KUmSocketAcceptOp(
        __in SOCKET Listener,
        __in ADDRESS_FAMILY AddressFamily,
        __in KAllocator& Alloc
        );

   ~KUmSocketAcceptOp();

   void SetSocketAddresses(ISocket & socket);

private:
    static ULONG _gs_AcceptCount;

    ISocket::SPtr _Socket;
    ADDRESS_FAMILY _AddressFamily;
    SOCKET        _NewSocket;
    SOCKET        _ListenSocket;
    UCHAR _acceptContext[(sizeof(SOCKADDR_STORAGE) + 16)*2];
    KTL_SOCKET_OVERLAPPED   _Olap;
    KAllocator*   _Allocator;
    PVOID         _IoRegistrationContext;
};


//
//  KUmSocketListenOp
//
class KUmSocketListenOp : public ISocketListenOp
{
    friend class KUmSocketListener;

public:
    typedef KSharedPtr<KUmSocketListenOp> SPtr;

    virtual NTSTATUS
    StartListen(
        __in_opt  KAsyncContextBase::CompletionCallback Callback = 0,
        __in_opt  KAsyncContextBase* const ParentContext = nullptr
        )
        {
            Start(ParentContext, Callback);
            return STATUS_PENDING;
        }


    virtual VOID
    OnStart();

    VOID
    SetUserData(
        __in PVOID Ptr
    )
    {
        _UserData = Ptr;
    }

    PVOID
    GetUserData()
    {
        return _UserData;
    }

    VOID
    OnReuse()
    {
    }

private:
    KUmSocketListenOp(
        __in SOCKET Listener,
        __in KAllocator& Alloc
        );

   ~KUmSocketListenOp();

private:
    KAllocator*   _Allocator;
    PVOID         _UserData;
};


//
//  KUmSocket
//
class KUmSocket : public ISocket
{
    friend class KUmSocketAcceptOp;
    friend class KUmSocketConnectOp;

public:
    typedef KSharedPtr<KUmSocket> SPtr;

    virtual VOID
    Close();

    virtual NTSTATUS
    CreateReadOp(
        __out ISocketReadOp::SPtr& NewOp
        );

    virtual NTSTATUS
    CreateWriteOp(
        __out ISocketWriteOp::SPtr& NewOp
        );

    virtual ULONG
    GetMaxMessageSize()
    {
        return 0x4000;
    }

    virtual NTSTATUS
    GetLocalAddress(
        __out INetAddress::SPtr& Addr
        );

    virtual NTSTATUS
    GetRemoteAddress(
        __out INetAddress::SPtr& Addr
        );

    virtual ULONG
    PendingOps()
    {
        return 0;
    }

    VOID
    SetUserData(
        __in PVOID Ptr
    )
    {
        _UserData = Ptr;
    }

    PVOID
    GetUserData()
    {
        return _UserData;
    }


    BOOLEAN
    Ok()
    {
        return _Socket != INVALID_SOCKET? TRUE : FALSE;
    }


    PVOID
    GetSocketId()
    {
        return PVOID(_Socket);
    }

#if KTL_USER_MODE
    _Requires_lock_not_held_(this->_ApiLock._Lock) _Acquires_lock_(this->_ApiLock._Lock)
#else
    _Requires_lock_not_held_(this->_ApiLock._SpinLock) _Acquires_lock_(this->_ApiLock._SpinLock) \
    __drv_maxIRQL(DISPATCH_LEVEL) __drv_raisesIRQL(DISPATCH_LEVEL) _At_(this->_ApiLock._OldIrql, __drv_savesIRQL)
#endif
    BOOLEAN
    Lock()
    {
        _ApiLock.Acquire();
        return TRUE;
    }

#if KTL_USER_MODE
    _Requires_lock_held_(this->_ApiLock._Lock) _Releases_lock_(this->_ApiLock._Lock)
#else
    _Requires_lock_held_(this->_ApiLock._SpinLock) _Releases_lock_(this->_ApiLock._SpinLock) \
    __drv_requiresIRQL(DISPATCH_LEVEL) _At_(this->_ApiLock._OldIrql, __drv_restoresIRQL)
#endif
    VOID
    Unlock()
    {
        _ApiLock.Release();
    }

public:
    // User-mode helpers
    //
    SOCKET
    GetSocketHandle()
    {
        return _Socket;
    }

#if KTL_USER_MODE
    _Requires_lock_not_held_(this->_ApiLock._Lock) _Acquires_lock_(this->_ApiLock._Lock)
#else
    _Requires_lock_not_held_(this->_ApiLock._SpinLock) _Acquires_lock_(this->_ApiLock._SpinLock) \
    __drv_maxIRQL(DISPATCH_LEVEL) __drv_raisesIRQL(DISPATCH_LEVEL) _At_(this->_ApiLock._OldIrql, __drv_savesIRQL)
#endif
    VOID
    AcquireApiLock()
    {
        _ApiLock.Acquire();
    }

#if KTL_USER_MODE
    _Requires_lock_held_(this->_ApiLock._Lock) _Releases_lock_(this->_ApiLock._Lock)
#else
    _Requires_lock_held_(this->_ApiLock._SpinLock) _Releases_lock_(this->_ApiLock._SpinLock) \
    __drv_requiresIRQL(DISPATCH_LEVEL) _At_(this->_ApiLock._OldIrql, __drv_restoresIRQL)
#endif
    VOID
    ReleaseApiLock()
    {
        _ApiLock.Release();
    }

private:
    KUmSocket(
        __in SOCKET Socket,
        __in KAllocator& Alloc,
        __in PVOID IoRegCtx
        );

   ~KUmSocket();

    void SetLocalAddr(INetAddress::SPtr && localAddress);
    void SetRemoteAddr(INetAddress::SPtr && remoteAddress);

    KAllocator* _Allocator;
    SOCKET      _Socket;
    PVOID       _IoRegistrationContext;
    PVOID       _UserData;
    KSpinLock   _ApiLock;
    INetAddress::SPtr _localAddress;
    INetAddress::SPtr _remoteAddress;
};


///////////////////////////////////////////////////////////////////////////////
//

class KUmSocketListener : public ISocketListener
{
    friend class KUmTcpTransport;

public:
    typedef KSharedPtr<KUmSocketListener> SPtr;

    virtual NTSTATUS
    CreateAcceptOp(
        __out ISocketAcceptOp::SPtr& Accept
        );

    virtual NTSTATUS
    CreateListenOp(
        __out ISocketListenOp::SPtr& Listen
        );

    virtual VOID
    Shutdown();

    PVOID
    GetUserData()
    {
        return _UserData;
    }

    VOID
    SetUserData(
        __in PVOID UserData
        )
    {
        _UserData = UserData;
    }

    BOOLEAN
    AcceptTerminated()
    {
        return _AcceptTerminated;
    }

    VOID
    SetAcceptTerminated()
    {
        _AcceptTerminated = TRUE;
    }

private:
    KUmSocketListener(
        __in KAllocator& Alloc
        );

   ~KUmSocketListener();

    NTSTATUS
    Initialize(
        __in INetAddress::SPtr& Address
        );

    SOCKET      _ListenSocket;
    KAllocator* _Allocator;
    PVOID       _IoRegistrationContext;
    PVOID       _UserData;
    BOOLEAN     _AcceptTerminated;
    ADDRESS_FAMILY _AddressFamily;
};

///////////////////////////////////////////////////////////////////////////////
//
//
class KUmSocketConnectOp : public ISocketConnectOp
{
    friend class KUmTcpTransport;

public:
    typedef KSharedPtr<KUmSocketConnectOp> SPtr;

    virtual NTSTATUS
    StartConnect(
        __in      INetAddress::SPtr& ConnectFrom,
        __in      INetAddress::SPtr& ConnectTo,
        __in_opt  KAsyncContextBase::CompletionCallback Callback = 0,
        __in_opt  KAsyncContextBase* const ParentContext = nullptr
        );

    virtual VOID
    OnStart();

    virtual ISocket::SPtr
    GetSocket()
    {
        return _Socket;
    }

    VOID
    SetUserData(
        __in PVOID Ptr
    )
    {
        _Olap._UserData = Ptr;
    }

    PVOID
    GetUserData()
    {
        return _Olap._UserData;
    }

    VOID
    IOCP_Completion(
        __in ULONG Status
        );

private:
    KUmSocketConnectOp(
        __in KAllocator& Alloc
        );

   ~KUmSocketConnectOp();

    KAllocator*             _Allocator;
    INetAddress::SPtr       _From;
    INetAddress::SPtr       _To;
    ISocket::SPtr           _Socket;
    SOCKET                  _SocketHandle;
    PVOID                   _IoRegistrationContext;
    KTL_SOCKET_OVERLAPPED   _Olap;
};

///////////////////////////////////////////////////////////////////////////////
//

class KUmTcpTransport : public ITransport
{
    friend class KUmSocketListener;
    friend class KUmSocketAcceptOp;
    friend class KUmSocketConnectOp;

public:
    typedef KSharedPtr<KUmTcpTransport> SPtr;

    static NTSTATUS
    Initialize(
        __in  KAllocator& Allocator,
        __out ITransport::SPtr& Transport
        );

    virtual NTSTATUS
    CreateConnectOp(
        __out ISocketConnectOp::SPtr& Connect
        );

    virtual NTSTATUS
    CreateListener(
        __in  INetAddress::SPtr& Address,
        __out ISocketListener::SPtr& Listener
        );

    virtual NTSTATUS
    CreateBindAddressOp(
        __out IBindAddressOp::SPtr& Bind
        );

    virtual NTSTATUS CreateNetAddress(
        SOCKADDR_STORAGE const * address,
        __out INetAddress::SPtr & netAddress);

    virtual VOID
    Shutdown();

    NTSTATUS
    SpawnDefaultClientAddress(
        __out INetAddress::SPtr& Address
        );

    NTSTATUS
    SpawnDefaultListenerAddresses(
        __inout KArray<INetAddress::SPtr>& Addresses
        );

    static VOID
    _Raw_IOCP_Completion(
        __in_opt VOID* Context,
        __inout OVERLAPPED* Overlapped,
        __in ULONG StatusCode,
        __in ULONG Transferred
        );

    KUmTcpTransport(
        __in KAllocator& Alloc
        );

   ~KUmTcpTransport();

    NTSTATUS
    Initialize();

private:
    WSADATA     _WsaData;
    KAllocator* _Allocator;

    ISocketListener::SPtr   _Listener;
    KSpinLock               _SocketTableLock;
    KArray<KUmSocket::SPtr> _SocketTable;
    BOOLEAN                 _Shutdown;

};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//      KERNEL MODE AREA     //////////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#else

class KKmSocket;
class KKmFreeAddressOp;

//
//  KKmNetAddress
//
//  The INetAddress abstraction for kernel mode TCP/IP addresses.
//
//  These are created by using the KKmMakeNetAddress class.
//
class KKmNetAddress : public INetAddress
{
    friend class KKmBindAddressOp;
    friend class KKmSocketAcceptOp;
    friend class KKmSocketConnectOp;
    friend class KKmTcpTransport;

public:
    typedef KSharedPtr<KKmNetAddress> SPtr;

    USHORT
    GetPort()
    {
        return _Port;
    }

    ULONG GetFormat()
    {
        return (Get()->sa_family == AF_INET) ? eWinsock_IPv4 : eWinsock_IPv6;
    }

    size_t
    Size() override
    {
        return sizeof(SOCKADDR_STORAGE);
    }

    // todo, need to merge this with ToStringW() and reconsider whether _StringForm can be non-numerical address.
    PWSTR
    GetStringForm()
    {
        return PWSTR(_StringForm);
    }

    PSOCKADDR
    Get()
    {
        if (_AddressInfo)
        {
            return _AddressInfo->ai_addr;
        }
        else
        {
            return PSOCKADDR(&_RawAddress);
        }
    }

    PVOID
    GetUserData()
    {
        return _UserData;
    }

    VOID
    SetUserData(
        __in PVOID UserData
        )
    {
        _UserData = UserData;
    }

    virtual NTSTATUS ToStringW(__out_ecount(len) PWSTR buf, __inout ULONG & len) const;

    virtual NTSTATUS ToStringA(__out_ecount(len) PSTR buf, __inout ULONG & len) const;

private:
    KKmNetAddress(
        __in PWSTR StringAddr,
        __in USHORT Port,
        __in PWSK_PROVIDER_NPI Provider,
        __in KAllocator& Alloc
        );

    KKmNetAddress(
        SOCKADDR_STORAGE const* rawAddress,
        __in PWSK_PROVIDER_NPI Provider,
        __in KAllocator & Alloc
        );

   ~KKmNetAddress();

    void Set(PADDRINFOEXW In);
    void SetDirect(PSOCKADDR Src);

    VOID
    Cleanup();

private:
    SOCKADDR_STORAGE  _RawAddress;
    USHORT            _Port;
    PWSK_PROVIDER_NPI _WskProviderNpi;
    PADDRINFOEXW      _AddressInfo;
    KWString          _StringForm;
    PVOID             _UserData;
    KAllocator*       _Allocator;
    KSharedPtr<KKmFreeAddressOp> _FreeAddressOp;

};

//
//  class KKmFreeAddressOp
//
class KKmFreeAddressOp : public KAsyncContextBase
{
public:
    typedef KSharedPtr<KKmFreeAddressOp> SPtr;

    NTSTATUS
    StartFree(
        __in PWSK_PROVIDER_NPI ProvNpi,
        __in PADDRINFOEXW      AddrInf,
        __in_opt    KAsyncContextBase::CompletionCallback Callback = 0,
        __in_opt    KAsyncContextBase* const ParentContext = nullptr
        )
    {
        _WskProviderNpi = ProvNpi;
        _AddressInfo = AddrInf;
        Start(ParentContext, Callback);
        return STATUS_PENDING;
    }

    VOID
    OnStart();

    KKmFreeAddressOp()
    {
        _WskProviderNpi = nullptr;
        _AddressInfo = nullptr;
    }

private:
    PWSK_PROVIDER_NPI _WskProviderNpi;
    PADDRINFOEXW     _AddressInfo;
};

//
//  KKmBindAddressOp
//
//  Used to create INetAddress objects from a port+string-based address (host name or IP).
//
class KKmBindAddressOp : public IBindAddressOp
{
    friend class KKmTcpTransport;
    K_FORCE_SHARED(KKmBindAddressOp);

public:
    typedef KSharedPtr<KKmBindAddressOp> SPtr;

    virtual NTSTATUS
    StartBind(
        __in PWSTR  StringAddress,
        __in USHORT Port,
        __in_opt    KAsyncContextBase::CompletionCallback Callback = 0,
        __in_opt    KAsyncContextBase* const ParentContext = nullptr
        );

    virtual NTSTATUS
    StartBind(
        __in PWSTR  StringAddress,
        __in ADDRESS_FAMILY addressFamily,
        __in USHORT Port,
        __in_opt    KAsyncContextBase::CompletionCallback Callback = 0,
        __in_opt    KAsyncContextBase* const ParentContext = nullptr
        );

    virtual NTSTATUS
    StartBind(
        _In_ PWSTR  StringAddress,
        ADDRESS_FAMILY addressFamily,
        USHORT Port,
        PCWSTR sspiTarget,
        _In_opt_ KAsyncContextBase::CompletionCallback Callback = 0,
        _In_opt_ KAsyncContextBase* const ParentContext = nullptr);

    VOID
    OnStart();

    PVOID
    GetUserData()
    {
        return _UserData;
    }

    VOID
    SetUserData(
        PVOID Ptr
        )
    {
        _UserData = Ptr;
    }

    INetAddress::SPtr const & GetAddress() { return _address; }

private:

    KKmBindAddressOp(
        __in KAllocator& Alloc,
        __in PWSK_PROVIDER_NPI WskProviderNpi
        );

    _Function_class_(IO_COMPLETION_ROUTINE)
    _IRQL_requires_same_
    static NTSTATUS
    _AddrComplete(
        PDEVICE_OBJECT DeviceObject,
        PIRP Irp,
        PVOID Context
        );

    NTSTATUS
    AddrComplete(
        PIRP Irp
        );

private:
    KAllocator*         _Allocator;
    KKmNetAddress::SPtr _kmAddress;
    INetAddress::SPtr _address;
    KWString            _StringAddress;
    ADDRESS_FAMILY _addressFamily;
    USHORT              _Port;
    PWSK_PROVIDER_NPI   _WskProviderNpi;
    PADDRINFOEXW        _AddressInfo;
    PVOID               _UserData;
};


class KKmSocket;

class KKmSocketWriteOp : public ISocketWriteOp
{
    friend class KKmSocket;
    friend class KKmTcpTransport;

public:
    typedef KSharedPtr<KKmSocketWriteOp> SPtr;

    // StartWrite
    //
    // Begins an asynchronous write to the underlying stream socket.
    //
    // Parameters:
    //      BufferCount         The number of buffers to be transmitted
    //      Buffers             A pointer to an array of KMemRef structs.
    //                          The _Param field of each struct must be set
    //                          to the number of bytes to be transmitted for that block.
    //                          This is because the blocks are likely to be allocated in page-sized chunks
    //                          and reused for various sends/receives, and the _Size field would be needed to reflect
    //                          the allocated size.
    //      Priority            The priority of the outgoing transmission.  Writes with a lower priority are
    //                          queued behind writes of higher priority.
    //      Callback            The async completion callback.
    //      ParentContext       The parent context, if any
    //
    //  Return value:
    //      STATUS_PENDING                  If successful.
    //      STATUS_INSUFFICIENT_RESOURCES   If insufficient memory to send the buffer.
    //      STATUS_INTERNAL_ERROR
    //
    virtual NTSTATUS
    StartWrite(
        __in       ULONG BufferCount,
        __in       KMemRef* Buffers,
        __in       ULONG Priority,
        __in_opt   KAsyncContextBase::CompletionCallback Callback = 0,
        __in_opt   KAsyncContextBase* const ParentContext = nullptr
        );


    VOID
    OnStart();

    ULONG
    BytesTransferred()
    {
        return _BytesTransferred;
    }

    ULONG
    BytesToTransfer()
    {
        return _BytesToTransfer;
    }

    KSharedPtr<ISocket>
    GetSocket()
    {
        return up_cast<ISocket, KKmSocket>(_Socket);
    }

    VOID
    SetUserData(
        __in PVOID Ptr
    )
    {
        _UserData = Ptr;
    }

    PVOID
    GetUserData()
    {
        return _UserData;
    }

#if KTL_IO_TIMESTAMP
    KIoTimestamps const * IoTimestamps() override;
#endif

protected:
    VOID OnReuse();

private:
    _Function_class_(IO_COMPLETION_ROUTINE)
    _IRQL_requires_same_
    static NTSTATUS
    _WriteComplete(
        PDEVICE_OBJECT DeviceObject,
        PIRP Irp,
        PVOID Context
        );

    NTSTATUS
    WriteComplete(
        PIRP Irp
        );

    KKmSocketWriteOp(
        __in KSharedPtr<KKmSocket> Socket,
        __in KAllocator& Alloc
        );

   ~KKmSocketWriteOp();

    VOID
    Cleanup();

private:
    // Data members

    KAllocator             *_Allocator;
    ULONG                   _Priority;
    KSharedPtr<KKmSocket>   _Socket;
    WSK_BUF                 _DataBuffer;
    ULONG                   _BytesToTransfer;
    ULONG                   _BytesTransferred;
    PVOID                   _UserData;
    LONG                    _OpNumber;

#if KTL_IO_TIMESTAMP
    KIoTimestamps _timestamps;
#endif
};



// KKmSocketReadOp
//
// Implements a read operation from a stream-oriented socket.
//
class KKmSocketReadOp : public ISocketReadOp
{
    friend class KKmSocket;
    friend class KUmTcpTransport;

public:
    typedef KSharedPtr<KKmSocketReadOp> SPtr;

    // StartRead
    //
    // Begins an asynchronous write to the underlying socket.
    //
    // Parameters:
    //      BufferCount         The number of buffers available to receive input
    //      Buffers             A pointer to an array of KMemRef structs.
    //                          The _Size field of each struct must be set
    //                          to the number of bytes available in that block. _Param is ignored on input.
    //      Callback            The async completion callback.
    //      ParentContext       The parent context, if any
    //
    //  Return value:
    //      STATUS_PENDING                  If successful.
    //      STATUS_INSUFFICIENT_RESOURCES   If insufficient memory to send the buffer.
    //      STATUS_INTERNAL_ERROR
    //
    virtual NTSTATUS
    StartRead(
        __in       ULONG BufferCount,
        __in       KMemRef* Buffers,
        __in       ULONG Flags,
        __in_opt   KAsyncContextBase::CompletionCallback Callback = 0,
        __in_opt   KAsyncContextBase* const ParentContext = nullptr
        );

    VOID
    OnStart();

    ULONG
    BytesTransferred()
    {
        return _BytesTransferred;
    }

    ULONG
    BytesToTransfer()
    {
        return _BytesToTransfer;
    }

    KSharedPtr<ISocket>
    GetSocket()
    {
        return up_cast<ISocket, KKmSocket>(_Socket);
    }

    VOID
    SetUserData(
        __in PVOID Ptr
    )
    {
        _UserData = Ptr;
    }

    PVOID
    GetUserData()
    {
        return _UserData;
    }

#if KTL_IO_TIMESTAMP
    KIoTimestamps const * IoTimestamps() override;
#endif

protected:
    VOID OnReuse();

private:
    // Private functions

    KKmSocketReadOp(
        __in KSharedPtr<KKmSocket> Socket,
        __in KAllocator& Alloc
        );

   ~KKmSocketReadOp();

    _Function_class_(IO_COMPLETION_ROUTINE)
    _IRQL_requires_same_
    static NTSTATUS
    _ReadComplete(
        PDEVICE_OBJECT DeviceObject,
        PIRP Irp,
        PVOID Context
        );

    NTSTATUS
    ReadComplete(
        PIRP Irp
        );

    VOID
    Cleanup();

private:
    // Data members

    KSharedPtr<KKmSocket>   _Socket;
    KAllocator             *_Allocator;
    ULONG                   _BytesTransferred;
    ULONG                   _BytesToTransfer;
    WSK_BUF                 _DataBuffer;
    PVOID                   _UserData;
    LONG                    _OpNumber;
    ULONG                   _flags;

#if KTL_IO_TIMESTAMP
    KIoTimestamps _timestamps;
#endif
};

//
//  KKmSocketAcceptOp
//
class KKmSocketAcceptOp : public ISocketAcceptOp
{
    friend class KKmSocketListener;

public:
    typedef KSharedPtr<KKmSocketAcceptOp> SPtr;

    virtual NTSTATUS
    StartAccept(
        __in_opt  KAsyncContextBase::CompletionCallback Callback = 0,
        __in_opt  KAsyncContextBase* const ParentContext = nullptr
        );

    virtual VOID
    OnStart();


    VOID
    SetUserData(
        __in PVOID Ptr
    )
    {
        _UserData = Ptr;
    }

    PVOID
    GetUserData()
    {
        return _UserData;
    }

    virtual KSharedPtr<ISocket>
    GetSocket()
    {
        return up_cast<ISocket, KKmSocket>(_ConnectedSocket);
    }



private:
    KKmSocketAcceptOp(
        __in KAllocator& Alloc,
        __in INetAddress::SPtr& Address,
        __in KSharedPtr<KKmSocketListener> Listener,
        __in PWSK_PROVIDER_NPI WskProviderNpi
        );

   ~KKmSocketAcceptOp();

    VOID
    ContinueAccept();

    VOID
    SocketListenResult(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& Op
        );

    _Function_class_(IO_COMPLETION_ROUTINE)
    _IRQL_requires_same_
    static NTSTATUS
    _AcceptComplete(
        PDEVICE_OBJECT DeviceObject,
        PIRP Irp,
        PVOID Context
        );

    NTSTATUS
    AcceptComplete(
        PIRP Irp
        );

    VOID
    Cleanup();

private:
    KSharedPtr<KKmSocket>           _AcceptorSocket;
    KSharedPtr<KKmSocket>           _ConnectedSocket;
    KSharedPtr<KKmSocketListener>   _Listener;

    PWSK_PROVIDER_NPI     _WskProviderNpi;
    INetAddress::SPtr     _Address;
    SOCKADDR_STORAGE _localSocketAddress;
    SOCKADDR_STORAGE _remoteSocketAddress;
    KAllocator*           _Allocator;
    PVOID                 _UserData;
    LONG                  _OpNumber;
};

//
//  KKmSocketListenOp
//
class KKmSocketListenOp : public ISocketListenOp
{
    friend class KKmSocketListener;

public:
    typedef KSharedPtr<KKmSocketListenOp> SPtr;

    virtual NTSTATUS
    StartListen(
        __in_opt  KAsyncContextBase::CompletionCallback Callback = 0,
        __in_opt  KAsyncContextBase* const ParentContext = nullptr
        );

    virtual VOID
    OnStart();


    VOID
    SetUserData(
        __in PVOID Ptr
    )
    {
        _UserData = Ptr;
    }

    PVOID
    GetUserData()
    {
        return _UserData;
    }

private:
    KKmSocketListenOp(
        __in KAllocator& Alloc,
        __in INetAddress::SPtr& Address,
        __in KSharedPtr<KKmSocketListener> Listener,
        __in PWSK_PROVIDER_NPI WskProviderNpi
        );

   ~KKmSocketListenOp();

    VOID
    SocketCreateResult(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& Op
        );

    VOID
    SocketBindResult(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& Op
        );

    VOID
    OnCreateSocketLock(
        __in BOOLEAN IsAcquired,
        __in KAsyncLock & LockAttempted);

    VOID
    Cleanup();

private:
    KSharedPtr<KKmSocket>           _AcceptorSocket;
    KSharedPtr<KKmSocketListener>   _Listener;

    PWSK_PROVIDER_NPI     _WskProviderNpi;
    INetAddress::SPtr     _Address;
    KAllocator*           _Allocator;
    PVOID                 _UserData;
    LONG                  _OpNumber;
};


//////////////////////////////////////////////////////////////////////////////
//
//  KKmSocket
//

class KKmSocket : public ISocket
{
    friend class KKmSocketAcceptOp;
    friend class KKmSocketBindOp;
    friend class KKmSocketConnectOp;
    friend class KKmSocketCreateOp;

public:
    typedef KSharedPtr<KKmSocket> SPtr;
    enum { eTypeNull = 0, eTypeListener = 1, eTypeInbound = 2, eTypeOutbound = 3 };

    virtual VOID
    Close();

    virtual NTSTATUS
    CreateReadOp(
        __out ISocketReadOp::SPtr& NewOp
        );

    virtual NTSTATUS
    CreateWriteOp(
        __out ISocketWriteOp::SPtr& NewOp
        );

    virtual ULONG
    GetMaxMessageSize()
    {
        return 0x4000;
    }


    virtual NTSTATUS
    GetLocalAddress(
        __out INetAddress::SPtr& Addr
        );

    virtual NTSTATUS
    GetRemoteAddress(
        __out INetAddress::SPtr& Addr
        );

    virtual ULONG
    PendingOps()
    {
        return 0;
    }

    VOID
    SetUserData(
        __in PVOID Ptr
    )
    {
        _UserData = Ptr;
    }

    PVOID
    GetUserData()
    {
        return _UserData;
    }


    PWSK_SOCKET
    Get()
    {
        return _WskSocketPtr;
    }

    BOOLEAN
    Ok()
    {
        if (_CloseRequest || _WskSocketPtr == nullptr)
        {
            return FALSE;
        }
        return TRUE;
    }

    PVOID
    GetSocketId()
    {
        return _WskSocketPtr;
    }

    BOOLEAN
    Lock();

    VOID
    Unlock();

    VOID
    ExecuteClose();

private:
    VOID
    Set(
        __in PWSK_SOCKET Socket,
        __in ULONG Type
        )
    {
        _WskSocketPtr = Socket;
        _Type = Type;
    }

    KKmSocket(
        __in KAllocator& Alloc
        );

    KKmSocket(
        __in KAllocator& Alloc,
        __in ADDRESS_FAMILY AddressFamily
        );


   ~KKmSocket();

    _Function_class_(IO_COMPLETION_ROUTINE)
    _IRQL_requires_same_
    static NTSTATUS
    _CloseComplete(
        PDEVICE_OBJECT DeviceObject,
        PIRP Irp,
        PVOID Context
        );

    void SetLocalAddr(INetAddress::SPtr && localAddress);
    void SetRemoteAddr(INetAddress::SPtr && remoteAddress);

    NTSTATUS
    AllocateClosingIrp();

private:
    KSpinLock   _SpinLock;
    BOOLEAN     _CloseRequest;
    LONG        _ApiUseCount;

    ULONG       _Type;
    KAllocator* _Allocator;
    PVOID       _UserData;
    PWSK_SOCKET _WskSocketPtr;
    ADDRESS_FAMILY _AddressFamily;
    INetAddress::SPtr _LocalAddr;
    INetAddress::SPtr _RemoteAddr;
    PIRP _pClosingIrp;
};

///////////////////////////////////////////////////////////////////////////////
//



///////////////////////////////////////////////////////////////////////////////
//
// Binds the KKmSocket to a local address so that it can connect or receive
// connections.
//
class KKmSocketBindOp : public KAsyncContextBase
{
    friend class KKmTcpTransport;

public:
    typedef KSharedPtr<KKmSocketBindOp> SPtr;

    static NTSTATUS
    Create(
        __in  KAllocator& Alloc,
        __out KKmSocketBindOp::SPtr& BindOp
        );

    NTSTATUS
    StartBind(
        __in KKmSocket::SPtr Socket,
        __in KKmNetAddress::SPtr LocalAddress,
        __in_opt  KAsyncContextBase::CompletionCallback Callback = 0,
        __in_opt  KAsyncContextBase* const ParentContext = nullptr
        );

    virtual VOID
    OnStart();

private:
    _Function_class_(IO_COMPLETION_ROUTINE)
    _IRQL_requires_same_
    static NTSTATUS
    _BindComplete(
        PDEVICE_OBJECT DeviceObject,
        PIRP Irp,
        PVOID Context
        );

    NTSTATUS
    BindComplete(
        PIRP Irp
        );

    KKmSocket::SPtr _Socket;
    KKmNetAddress::SPtr _Address;
};

///////////////////////////////////////////////////////////////////////////////
//
// Creates a socket
//
class KKmSocketCreateOp : public KAsyncContextBase
{
    K_FORCE_SHARED(KKmSocketCreateOp);

public:
    typedef KSharedPtr<KKmSocketCreateOp> SPtr;

    static NTSTATUS
    Create(
        __in  KAllocator& Alloc,
        __in  PWSK_PROVIDER_NPI Prov,
        __out KKmSocketCreateOp::SPtr& NewPtr
        );

    //
    //  StartCreate
    //
    //  Starts creation of a socket.
    //
    //  Parameters:
    //      AddressFamily AF_INET or AF_INET6
    //      SocketType  WSK_FLAG_LISTEN_SOCKET or WSK_FLAG_CONNECTION_SOCKET
    //
    NTSTATUS
    StartCreate(
        __in      ADDRESS_FAMILY AddressFamily,
        __in      ULONG SocketType,
        __in_opt  KAsyncContextBase::CompletionCallback Callback,
        __in_opt  KAsyncContextBase* const ParentContext
        );

    virtual VOID
    OnStart();

    KKmSocket::SPtr Get()
    {
        return _Socket;
    }

private:

    KKmSocketCreateOp(
        __in KAllocator& Alloc,
        __in PWSK_PROVIDER_NPI ProviderNpi
        );

    _Function_class_(IO_COMPLETION_ROUTINE)
    _IRQL_requires_same_
    static NTSTATUS
    _CreateComplete(
        PDEVICE_OBJECT DeviceObject,
        PIRP Irp,
        PVOID Context
        );

    NTSTATUS
    CreateComplete(
        PIRP Irp
        );

    KKmSocket::SPtr _Socket;
    PWSK_PROVIDER_NPI _WskProviderNpi;
    ADDRESS_FAMILY _AddressFamily;
    ULONG _SocketType;
    KAllocator* _Allocator;
};





///////////////////////////////////////////////////////////////////////////////
//

class KKmSocketListener : public ISocketListener
{
    friend class KKmTcpTransport;
    friend class KKmSocketAcceptOp;
    friend class KKmSocketListenOp;

public:
    typedef KSharedPtr<KKmSocketListener> SPtr;

    virtual NTSTATUS
    CreateAcceptOp(
        __out ISocketAcceptOp::SPtr& Accept
        );

    virtual NTSTATUS
    CreateListenOp(
        __out ISocketListenOp::SPtr& Listen
        );

    virtual VOID
    Shutdown();

    PVOID
    GetUserData()
    {
        return _UserData;
    }

    VOID
    SetUserData(
        __in PVOID UserData
        )
    {
        _UserData = UserData;
    }

    BOOLEAN
    AcceptTerminated()
    {
        return _AcceptTerminated;
    }

    VOID
    SetAcceptTerminated()
    {
        _AcceptTerminated = TRUE;
    }

private:
    KKmSocketListener(
        __in KAllocator& Alloc,
        __in INetAddress::SPtr& Address,
        __in PWSK_PROVIDER_NPI WskProviderNpi
        );

   ~KKmSocketListener();

    KAllocator*        _Allocator;
    INetAddress::SPtr  _Address;
    PVOID              _UserData;
    KKmSocket::SPtr    _ListenerSocket;
    KAsyncLock         _CreateSocketLock;
    PWSK_PROVIDER_NPI  _WskProviderNpi;
    BOOLEAN            _AcceptTerminated;
};

///////////////////////////////////////////////////////////////////////////////
//
//
class KKmSocketConnectOp : public ISocketConnectOp
{
    friend class KUmTcpTransport;

public:
    typedef KSharedPtr<KKmSocketConnectOp> SPtr;

    static NTSTATUS
    Create(
        __in  KAllocator& Alloc,
        __in  PWSK_PROVIDER_NPI Prov,
        __out KKmSocketConnectOp::SPtr& NewInstance
        );


    virtual NTSTATUS
    StartConnect(
        __in      INetAddress::SPtr& ConnectFrom,
        __in      INetAddress::SPtr& ConnectTo,
        __in_opt  KAsyncContextBase::CompletionCallback Callback = 0,
        __in_opt  KAsyncContextBase* const ParentContext = nullptr
        );

    virtual VOID
    OnStart();

    virtual ISocket::SPtr
    GetSocket()
    {
        return up_cast<ISocket, KKmSocket>(_Socket);
    }

    VOID
    SetUserData(
        __in PVOID Ptr
    )
    {
        _UserData = Ptr;
    }

    PVOID
    GetUserData()
    {
        return _UserData;
    }

private:
    KKmSocketConnectOp(
        __in KAllocator& Alloc,
        __in PWSK_PROVIDER_NPI ProvPtr
        );


   ~KKmSocketConnectOp();

    VOID
    ContinueConnect();

    VOID
    SocketCreateResult(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& Op
        );

    VOID
    SocketBindResult(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& Op
        );

    VOID
    Cleanup();

    _Function_class_(IO_COMPLETION_ROUTINE)
    _IRQL_requires_same_
    static NTSTATUS
    _ConnectComplete(
        PDEVICE_OBJECT DeviceObject,
        PIRP Irp,
        PVOID Context
        );

    NTSTATUS
    ConnectComplete(
        PIRP Irp
        );

    void StartGetLocalAddress(PIRP irp);

    _Function_class_(IO_COMPLETION_ROUTINE)
    _IRQL_requires_same_
    static NTSTATUS _GetLocalAddressComplete(
        PDEVICE_OBJECT deviceObject,
        PIRP irp,
        PVOID context);

    NTSTATUS GetLocalAddressComplete(PIRP irp);

private:
    KAllocator*             _Allocator;
    PWSK_PROVIDER_NPI       _WskProviderNpiPtr;
    INetAddress::SPtr       _From;
    INetAddress::SPtr       _To;
    SOCKADDR_STORAGE _localSocketAddress;
    KKmSocket::SPtr         _Socket;
    PVOID                   _UserData;
    LONG                    _OpNumber;
};




///////////////////////////////////////////////////////////////////////////////
//

class KKmTcpTransport : public ITransport
{
    friend class KKmSocketListener;
    friend class KKmSocketAcceptOp;
    friend class KKmSocketConnectOp;
public:
    typedef KSharedPtr<KKmTcpTransport> SPtr;

    static NTSTATUS
    Initialize(
        __in KAllocator& Allocator,
        __out ITransport::SPtr& Transport
        );

    virtual NTSTATUS
    CreateConnectOp(
        __out ISocketConnectOp::SPtr& Connect
        );

    virtual NTSTATUS
    CreateListener(
        __in  INetAddress::SPtr& Address,
        __out ISocketListener::SPtr& Listener
        );

    virtual NTSTATUS
    CreateBindAddressOp(
        __out IBindAddressOp::SPtr& Bind
        );

    virtual NTSTATUS CreateNetAddress(
        SOCKADDR_STORAGE const * address,
        __out INetAddress::SPtr & netAddress);

    virtual VOID
    Shutdown();


    PWSK_PROVIDER_NPI
    GetProviderNpi()
    {
        return &_WskProviderNpi;
    }

    NTSTATUS
    SpawnDefaultClientAddress(
        __out INetAddress::SPtr& Address
        );

    NTSTATUS
    SpawnDefaultListenerAddresses(
        __inout KArray<INetAddress::SPtr>& AddressList
        );

// CONFLICT: (raymcc)
    KKmTcpTransport(
        __in KAllocator& Alloc
        );

   ~KKmTcpTransport()
    {
       //Cleanup();
    }

    NTSTATUS
    _Initialize();

    VOID
    Cleanup();


private:
    BOOLEAN                 _ShutdownFlag;

    KAllocator*             _Allocator;
    WSK_REGISTRATION        _WskRegistration;
    WSK_CLIENT_NPI          _WskClientNpi;
    WSK_CLIENT_DISPATCH     _WskClientDispatch;
    WSK_PROVIDER_NPI        _WskProviderNpi;
    BOOLEAN                 _RegistrationOk;
    BOOLEAN                 _ProviderNpiOk;
};




#endif
