

/*++

    (c) 2011 by Microsoft Corp. All Rights Reserved.

    ksocket.cpp

    Description:
      Kernel Tempate Library (KTL): KSocket

      User & kernel mode TCP

    History:
      raymcc          2-May-2011         Initial draft


--*/

#include <ktl.h>
#include <ktrace.h>

#if KTL_USER_MODE
 #include <ws2tcpip.h>
 #include <ws2ipdef.h>
 #include <iphlpapi.h>
#else
 #include <netioapi.h>
#endif

    // CONFLICT (raymcc)
    // Remove this once things work
#define TEMP_DEFAULT_PORT 5005



#define _printf(...)   KDbgPrintf(__VA_ARGS__)

// Performance counters (non-interlocked)
//
ULONGLONG __KSocket_BytesWritten = 0;
ULONGLONG __KSocket_BytesRead = 0;
ULONGLONG __KSocket_Writes = 0;
ULONGLONG __KSocket_Reads = 0;

LONG      __KSocket_SocketObjects = 0;
LONG      __KSocket_SocketCount = 0;
LONG      __KSocket_AddressCount = 0;
LONG      __KSocket_AddressObjects = 0;
LONG      __KSocket_WriteFailures = 0;
LONG      __KSocket_ReadFailures = 0;
LONG      __KSocket_ConnectFailures = 0;
LONG      __KSocket_PendingOperations = 0;

LONG      __KSocket_PendingWrites = 0;
LONG      __KSocket_PendingReads = 0;

LONG      __KSocket_Shutdown = 0;
LONG      __KSocket_ShutdownPhase = 0;
LONG      __KSocket_Accepts = 0;
LONG      __KSocket_Connects = 0;
LONG      __KSocket_Closes = 0;
LONG      __KSocket_PendingAccepts = 0;
LONG      __KSocket_AcceptFailures = 0;

LONG      __KSocket_OpNumber = 0;
LONG      __KSocket_Version  = 111;
LONG      __KSocket_LastWriteError = 0;

NTSTATUS
KSocket_EnumLocalAddresses(
    __in KAllocator* Alloc,
    __inout KArray<SOCKADDR_STORAGE>& Addresses
    );

///////////////////////////////////////////////////////////////////////////////


#if KTL_USER_MODE


//
//  MakeSocket
//
//  Wrapper for socket() to track socket counts.
//
static SOCKET WSAAPI
MakeSocket(
    __in  int af,
    __in  int type,
    __in  int protocol
   )
{

    SOCKET s = socket(af, type, protocol);
    if (s != INVALID_SOCKET)
    {
        InterlockedIncrement(&__KSocket_SocketCount);
    }
    _printf(">> Created socket %llx", s);

    return s;
}

//
//  DeleteSocket
//
//  Wrapper for closesocket() to track socket counts
//
static int
DeleteSocket(
    __in SOCKET& s
    )
{
    int res = WSAENOTSOCK;
    if (s != INVALID_SOCKET)
    {
       _printf(">> Closed socket %llx", s);
        res = closesocket(s);
        if (res == 0)
        {
            InterlockedDecrement(&__KSocket_SocketCount);
            InterlockedIncrement(&__KSocket_Closes);
            s = INVALID_SOCKET;
        }
    }
    else
    {
        _printf(">> Attempted to close an invalid socket\n");
    }
    return res;
}



//
//  KUmNetAddress::KUmNetAddress
//
KUmNetAddress::KUmNetAddress()
    : _StringAddress(GetThisAllocator())
{
    _Port = 0;
    _UserData = nullptr;
    RtlZeroMemory(&_SocketAddress, sizeof(SOCKADDR_STORAGE));
    InterlockedIncrement(&__KSocket_AddressObjects);
}

KUmNetAddress::KUmNetAddress(SOCKADDR_STORAGE const* socketAddress)
    : _StringAddress(GetThisAllocator())
{
    KMemCpySafe(&_SocketAddress, sizeof(_SocketAddress), socketAddress, sizeof(_SocketAddress));

    if (_SocketAddress.ss_family == AF_INET)
    {
        _Port = RtlUshortByteSwap(PSOCKADDR_IN(&_SocketAddress)->sin_port);
    }
    else
    {
        KFatal(_SocketAddress.ss_family == AF_INET6);
        _Port = RtlUshortByteSwap(PSOCKADDR_IN6(&_SocketAddress)->sin6_port);
    }

    // todo, leikong, set _StringAddress

    _UserData = nullptr;
    InterlockedIncrement(&__KSocket_AddressObjects);
}

//
//  KUmNetAddress::~KUmNetAddress
//
KUmNetAddress::~KUmNetAddress()
{
    InterlockedDecrement(&__KSocket_AddressObjects);
}

#pragma warning(push)

//
// Disable prefast warning 24007:
// This function or type supports IPv4 only.
#pragma warning(disable : 24007)

//
// Disable prefast warning 24002:
// This type is IPv4 specific.
#pragma warning(disable : 24002)

NTSTATUS KUmNetAddress::ToStringW(__out_ecount(len) PWSTR buf, __inout ULONG & len) const
{
    if (AF_INET == _SocketAddress.ss_family)
    {
        PSOCKADDR_IN sockAddrIn = (PSOCKADDR_IN)(&_SocketAddress);
        return RtlIpv4AddressToStringExW(
            &sockAddrIn->sin_addr,
            sockAddrIn->sin_port,
            buf,
            &len);
    }

    if (AF_INET6 == _SocketAddress.ss_family)
    {
        PSOCKADDR_IN6 sockAddrIn6 = (PSOCKADDR_IN6)(&_SocketAddress);
        return RtlIpv6AddressToStringExW(
            &sockAddrIn6->sin6_addr,
            sockAddrIn6->sin6_scope_id,
            sockAddrIn6->sin6_port,
            buf,
            &len);
    }

    return STATUS_INVALID_PARAMETER;
}

NTSTATUS KUmNetAddress::ToStringA(__out_ecount(len) PSTR buf, __inout ULONG & len) const
{
    if (AF_INET == _SocketAddress.ss_family)
    {
        PSOCKADDR_IN sockAddrIn = (PSOCKADDR_IN)(&_SocketAddress);
        return RtlIpv4AddressToStringExA(
            &sockAddrIn->sin_addr,
            sockAddrIn->sin_port,
            buf,
            &len);
    }

    if (AF_INET6 == _SocketAddress.ss_family)
    {
        PSOCKADDR_IN6 sockAddrIn6 = (PSOCKADDR_IN6)(&_SocketAddress);
        return RtlIpv6AddressToStringExA(
            &sockAddrIn6->sin6_addr,
            sockAddrIn6->sin6_scope_id,
            sockAddrIn6->sin6_port,
            buf,
            &len);
    }

    return STATUS_INVALID_PARAMETER;
}

#pragma warning(pop)

///////////////////////////////////////////////////////////////////////////////
//
//  KUmBindAddressOp

//
//
//
KUmBindAddressOp::KUmBindAddressOp(__in KAllocator& Alloc)
    :   _StringAddress(Alloc)
{
    _Allocator = &Alloc;
    _Port = 0;
    _UserData = nullptr;
    _addressFamily = AF_UNSPEC;
    InterlockedIncrement(&__KSocket_PendingOperations);
}

//
//
//
KUmBindAddressOp::~KUmBindAddressOp()
{
    InterlockedDecrement(&__KSocket_PendingOperations);
}

_Use_decl_annotations_
NTSTATUS KUmBindAddressOp::StartBind(
    PWSTR  StringAddress,
    ADDRESS_FAMILY addressFamily,
    USHORT Port,
    PCWSTR sspiTarget,
    KAsyncContextBase::CompletionCallback Callback,
    KAsyncContextBase* const ParentContext)
{
    if (__KSocket_Shutdown)
    {
        return STATUS_SHUTDOWN_IN_PROGRESS;
    }

    _addressFamily = addressFamily;
    _Port = Port;
    _StringAddress = StringAddress;

    if (!NT_SUCCESS(_StringAddress.Status()))
    {
        // failure in KWString operator=
        return _StringAddress.Status();
    }

    // Attempt to allocate a new address object.
    _umAddress = _new (KTL_TAG_NET, *_Allocator) KUmNetAddress();
    if (!_umAddress)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(_umAddress->Status()))
    {
        return _umAddress->Status();
    }

    if (sspiTarget != nullptr)
    {
        _address = _new (KTL_TAG_NET, *_Allocator) KSspiAddress(
            up_cast<INetAddress, KUmNetAddress>(_umAddress),
            sspiTarget);

        if (!_address)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        _address = up_cast<INetAddress, KUmNetAddress>(_umAddress);
    }

    Start(ParentContext, Callback);
    return STATUS_SUCCESS;
}

NTSTATUS KUmBindAddressOp::StartBind(
    __in PWSTR  StringAddress,
    __in ADDRESS_FAMILY addressFamily,
    __in USHORT Port,
    __in_opt    KAsyncContextBase::CompletionCallback Callback,
    __in_opt    KAsyncContextBase* const ParentContext)
{
    return StartBind(
        StringAddress,
        addressFamily,
        Port,
        nullptr,
        Callback,
        ParentContext);
}

//
//  KUmBindAddressOp::StartBind
//
NTSTATUS
KUmBindAddressOp::StartBind(
    __in PWSTR  StringAddress,
    __in USHORT Port,
    __in_opt    KAsyncContextBase::CompletionCallback Callback,
    __in_opt    KAsyncContextBase* const ParentContext
    )
{
    return StartBind(
        StringAddress,
        AF_INET,
        Port,
        nullptr,
        Callback,
        ParentContext);
}

//
//  KUmBindAddressOp::OnStart
//
VOID
KUmBindAddressOp::OnStart()
{
    ADDRINFOW hints;
    PADDRINFOW presult = NULL;

    RtlZeroMemory(&hints, sizeof(hints));

    hints.ai_family = _addressFamily;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int error = GetAddrInfoW(_StringAddress, NULL, &hints, &presult);

    if (error != 0 || presult == nullptr)
    {
        FreeAddrInfoW(presult);
        _umAddress.Reset();
        _address.Reset();
        // TODO error -> ntstatus
        Complete(STATUS_INVALID_ADDRESS);
        return;
    }

    if (presult->ai_family == AF_INET)
    {
        *PSOCKADDR_IN(&_umAddress->_SocketAddress) = *PSOCKADDR_IN(presult->ai_addr);
        PSOCKADDR_IN(&_umAddress->_SocketAddress)->sin_port = htons(_Port);
        }
        else
        {
        *PSOCKADDR_IN6(&_umAddress->_SocketAddress) = *PSOCKADDR_IN6(presult->ai_addr);
        PSOCKADDR_IN6(&_umAddress->_SocketAddress)->sin6_port = htons(_Port);
    }

    _umAddress->_StringAddress = _StringAddress;

    FreeAddrInfoW(presult);

    Complete(STATUS_SUCCESS);
}

///////////////////////////////////////////////////////////////////////////////

//
//  KUmSocketWriteOp::KUmSocketWriteOp
//
//  Constructor
//
//  This is private/friend-only: called by KUmSocket
//
KUmSocketWriteOp::KUmSocketWriteOp(
    __in KUmSocket::SPtr Socket,
    __in KAllocator& Alloc
    ) : _Buffers(Alloc)
{
    _Socket = Socket;
    _Allocator = &Alloc;

    // Zero the other fields

    _BytesToTransfer = 0;
    _BytesTransferred = 0;

    RtlZeroMemory(&_Olap, sizeof(KTL_SOCKET_OVERLAPPED));

    _Olap._KtlObject = this;
    _Olap._KtlObjectType = KTL_SOCKET_OVERLAPPED::eTypeWrite;

    InterlockedIncrement(&__KSocket_PendingOperations);
}


//  KUmSocketWriteOp::KUmSocketWriteOp
//
//  Destructor
//
KUmSocketWriteOp::~KUmSocketWriteOp()
{
    InterlockedDecrement(&__KSocket_PendingOperations);
}


//
//  KUmSocketWriteOp::StartWrite
//
NTSTATUS
KUmSocketWriteOp::StartWrite(
    __in       ULONG BufferCount,
    __in       KMemRef* Buffers,
    __in       ULONG Priority,
    __in_opt   KAsyncContextBase::CompletionCallback Callback,
    __in_opt   KAsyncContextBase* const ParentContext
    )
{
    if (__KSocket_Shutdown)
    {
        return STATUS_SHUTDOWN_IN_PROGRESS;
    }
    // Param validation
    //
    if (BufferCount == 0)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (Buffers == nullptr)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    _Priority = Priority;

    // Total up the bytes/buffers and copy the info to internal WSABUF structs.
    //
    NTSTATUS status = _Buffers.Reserve(BufferCount);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    BOOLEAN setCountSucceeded = _Buffers.SetCount(BufferCount);
    KAssert(setCountSucceeded);
    #if !DBG
    UNREFERENCED_PARAMETER(setCountSucceeded);
    #endif

    for (ULONG i = 0; i < BufferCount; i++)
    {
        _Buffers[i].len = Buffers[i]._Param;
        _Buffers[i].buf = (CHAR*) Buffers[i]._Address;
        _BytesToTransfer += Buffers[i]._Param;
    }

    // Schedule it for starting.
    //
    Start(ParentContext, Callback);
    return STATUS_PENDING;
}

//
// KUmSocketWriteOp::OnStart
//
VOID
KUmSocketWriteOp::OnStart()
{
    _Socket->AcquireApiLock();

    // todo, enable "TCP_NODELAY" when _Priority is ePriorityHigh, as is done in KKmSocketWriteOp.
    // WSASend does not support per send flag like WSK_FLAG_NODELAY in WskSend, so we probably should
    // consider using setsocketopt(TCP_NODELAY), set it when _Priority is ePriorityHigh and the 
    // option hasn't been set on the socket.

    AcquireActivities(1);
    int Result = WSASend(
        _Socket->GetSocketHandle(),   // The socket
        _Buffers.Data(),              // WSABUF array
        _Buffers.Count(),             // Size of above array
        &_BytesTransferred,     // Bytes sent; not used for IOCP
        0,                      // Flags
        &_Olap,                 // OVERLAPPED
        NULL                    // Completion routine; not used for IOCP
        );

    _Socket->ReleaseApiLock();

    if ((Result == 0) || (WSAGetLastError() == WSA_IO_PENDING))
    {
        // Rely on the IOCP for completion
        return;
    }

    Complete(Result);
    ReleaseActivities(1);
}


//
// KUmSocketWriteOp::GetSocket
//
KSharedPtr<ISocket>
KUmSocketWriteOp::GetSocket()
{
    return up_cast<ISocket, KUmSocket>(_Socket);
}


//
//  KUmSocketWriteOp::IOCP_Completion
//
VOID
KUmSocketWriteOp::IOCP_Completion(
    __in ULONG Status,
    __in ULONG BytesTransferred
    )
{
    KFinally([&] () { ReleaseActivities(1); });

    //KDbgPrintf("socket %llx: write completed: status=%x, written=%d", _Socket->GetSocketId(), Status, BytesTransferred);

    _BytesTransferred = BytesTransferred;

    __KSocket_BytesWritten += _BytesTransferred;  // This is merely statistical and is not interlocked

    if (_BytesTransferred == 0)
    {
        // TBD
        InterlockedIncrement(&__KSocket_WriteFailures);
    }
    else
    {
        __KSocket_Writes++;
    }

    Complete(Status);
}




///////////////////////////////////////////////////////////////////////////////


//
//  KUmSocketReadOp::KUmSocketReadOp
//
//  Constructor
//
//  This is private/friend-only: called by KUmSocket
//
KUmSocketReadOp::KUmSocketReadOp(
    __in KUmSocket::SPtr Socket,
    __in KAllocator& Alloc
    ) : _Buffers(Alloc)
{
    _Socket = Socket;
    _Allocator = &Alloc;

    // Zero the other fields

    _BytesAvailable = 0;
    _BytesTransferred = 0;

    RtlZeroMemory(&_Olap, sizeof(KTL_SOCKET_OVERLAPPED));

    _Olap._KtlObject = this;
    _Olap._KtlObjectType = KTL_SOCKET_OVERLAPPED::eTypeRead;

    InterlockedIncrement(&__KSocket_PendingOperations);
}


//
//  KUmSocketReadOp::~KUmSocketReadOp
//
KUmSocketReadOp::~KUmSocketReadOp()
{
    InterlockedDecrement(&__KSocket_PendingOperations);
}


//
//  KUmSocketReadOp::StartWrite
//
NTSTATUS
KUmSocketReadOp::StartRead(
    __in       ULONG BufferCount,
    __in       KMemRef* Buffers,
    __in       ULONG Flags,
    __in_opt   KAsyncContextBase::CompletionCallback Callback,
    __in_opt   KAsyncContextBase* const ParentContext
    )
{
    if (__KSocket_Shutdown)
    {
        return STATUS_SHUTDOWN_IN_PROGRESS;
    }

    // Param validation
    //
    if (BufferCount == 0)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (Buffers == nullptr)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    // Total up the bytes/buffers and copy the info to internal WSABUF structs.
    //
    NTSTATUS status = _Buffers.Reserve(BufferCount);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    BOOLEAN setCountSucceeded = _Buffers.SetCount(BufferCount);
    KAssert(setCountSucceeded);
#if !DBG
    UNREFERENCED_PARAMETER(setCountSucceeded);
#endif

    for (ULONG i = 0; i < BufferCount; i++)
    {
        _Buffers[i].len = Buffers[i]._Size;
        _Buffers[i].buf = (CHAR*) Buffers[i]._Address;
        _BytesAvailable += Buffers[i]._Size;
    }

    _flags = Flags;

    // Schedule it for starting.
    //
    Start(ParentContext, Callback);
    return STATUS_PENDING;
}

//
//  KUmSocketReadOp::GetSocket
//
KSharedPtr<ISocket>
KUmSocketReadOp::GetSocket()
{
    return up_cast<ISocket, KUmSocket>(_Socket);
}

//
//  KUmSocketReadOp::OnStart
//
VOID
KUmSocketReadOp::OnStart()
{
    SOCKET  Sock = _Socket->GetSocketHandle();
    _dwFlagsTemp = (_flags & ISocketReadOp::ReadAll) ? MSG_WAITALL : 0;

    _Socket->AcquireApiLock();

    AcquireActivities(1);
    int Result = WSARecv(
        Sock,                   // The socket
        _Buffers.Data(),        // WSABUF array
        _Buffers.Count(),       // Size of above array
        &_BytesTransferred,     // Bytes received
        &_dwFlagsTemp,          // Flags
        &_Olap,                 // OVERLAPPED
        NULL                    // Completion routine; not used for IOCP
        );

    _Socket->ReleaseApiLock();

    if ((Result == 0) || (WSAGetLastError() == WSA_IO_PENDING))
    {
        return;
    }

    Complete(Result);
    ReleaseActivities(1);
}


//
//  KUmSocketReadOp::IOCP_Completion
//
//  Called by _Raw_IOCP_Completion when the socket op completes.
//
VOID
KUmSocketReadOp::IOCP_Completion(
    __in ULONG Status,
    __in ULONG BytesTransferred
    )
{
    KFinally([&]() { ReleaseActivities(1); });

    //KDbgPrintf("socket %llx: read completed: status=%x, read=%d", _Socket->GetSocketId(), Status, BytesTransferred);

    _BytesTransferred = BytesTransferred;
    __KSocket_BytesRead += _BytesTransferred; // Not interlocked; statistical info only

    if (_BytesTransferred == 0)
    {
        InterlockedIncrement(&__KSocket_ReadFailures);
    }
    else
    {
        __KSocket_Reads++;
    }

    Complete(Status);
}



///////////////////////////////////////////////////////////////////////////////


ULONG KUmSocketAcceptOp::_gs_AcceptCount = 0;

void KUmSocketAcceptOp::SetSocketAddresses(ISocket & socket)
{
    LPFN_GETACCEPTEXSOCKADDRS pfnGetAcceptExSockaddrs = nullptr;
    GUID GuidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
    DWORD dwBytes;

    WSAIoctl(_ListenSocket,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &GuidGetAcceptExSockaddrs,
        sizeof(GuidGetAcceptExSockaddrs),
        &pfnGetAcceptExSockaddrs,
        sizeof(pfnGetAcceptExSockaddrs),
        &dwBytes,
        NULL,
        NULL
        );

    SOCKET_ADDRESS local;
    SOCKET_ADDRESS remote;
    pfnGetAcceptExSockaddrs(
        _acceptContext,
        0, // outBufLen - ((sizeof(sockaddr_in) + 16) * 2),
        sizeof(SOCKADDR_STORAGE) + 16,
        sizeof(SOCKADDR_STORAGE) + 16,
        &local.lpSockaddr,
        &local.iSockaddrLength,
        &remote.lpSockaddr,
        &remote.iSockaddrLength);

    KFatal(local.lpSockaddr);
    KFatal(remote.lpSockaddr);

    INetAddress::SPtr localAddress = _new (KTL_TAG_NET, GetThisAllocator()) KUmNetAddress(
        reinterpret_cast<SOCKADDR_STORAGE const*>(local.lpSockaddr));
    if (!localAddress)
    {
        Complete(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    KUmSocket& umSocket = static_cast<KUmSocket&>(socket);
    umSocket.SetLocalAddr(Ktl::Move(localAddress));

    INetAddress::SPtr remoteAddress = _new (KTL_TAG_NET, GetThisAllocator()) KUmNetAddress(
        reinterpret_cast<SOCKADDR_STORAGE const*>(remote.lpSockaddr));
    if (!remoteAddress)
    {
        Complete(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    umSocket.SetRemoteAddr(Ktl::Move(remoteAddress));
}

//
//  KUmSocketAcceptOp::OnStart
//
//
VOID
KUmSocketAcceptOp::OnStart()
{
    LPFN_ACCEPTEX pfnAcceptEx = NULL;
    GUID GuidAcceptEx = WSAID_ACCEPTEX;
    DWORD dwBytes;
    NTSTATUS Result;

    // AcceptEx is an 'extension' that has to be loaded via
    // a pointer-to-function
    //
    WSAIoctl(_ListenSocket,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &GuidAcceptEx,
        sizeof(GuidAcceptEx),
        &pfnAcceptEx,
        sizeof(pfnAcceptEx),
        &dwBytes,
        NULL,
        NULL
        );

    // Create an accepting socket
    //
    _NewSocket = MakeSocket(_AddressFamily, SOCK_STREAM, IPPROTO_TCP);
    if (_NewSocket == INVALID_SOCKET)
    {
        Complete(STATUS_INTERNAL_ERROR);
        return;
    }

    // We now associate an IO completion port with this socket for
    // subsequent reads/writes.
    //
    Result = _Allocator->GetKtlSystem().DefaultThreadPool().RegisterIoCompletionCallback(
        HANDLE(_NewSocket),
        KUmTcpTransport::_Raw_IOCP_Completion,
        nullptr,
        &_IoRegistrationContext
        );

    if (!NT_SUCCESS(Result))
    {
        Complete(STATUS_INTERNAL_ERROR);
        return;
    }

    // Empty our overlapped structure and accept connections.
    //
    AcquireActivities(1);
    BOOL bRes = pfnAcceptEx(
        _ListenSocket,
        _NewSocket,
        _acceptContext,
        0,                         // outBufLen - ((sizeof(sockaddr_in) + 16) * 2),
        sizeof(SOCKADDR_STORAGE) + 16,
        sizeof(SOCKADDR_STORAGE) + 16,
        &dwBytes,
        &_Olap
        );

    if (bRes)
    {
        return;
    }

    int LastError = WSAGetLastError();
    if (LastError == WSA_IO_PENDING)
    {
        return;
    }

    Complete((LastError == WSAECONNRESET) ? STATUS_CANCELLED : STATUS_INTERNAL_ERROR);
    ReleaseActivities(1);
}

//
//  KUmSocketAcceptOp::IOCP_Completion
//
//  Called by _Raw_IOCP_Completion when the accept occurs.
//
VOID
KUmSocketAcceptOp::IOCP_Completion(
    __in ULONG OpStatus
    )
{
    KFinally([&] () { ReleaseActivities(1); });

    if (OpStatus != NO_ERROR)
    {
        NTSTATUS status = STATUS_UNEXPECTED_NETWORK_ERROR;
        if (OpStatus == ERROR_OPERATION_ABORTED)
        {
            status = STATUS_REQUEST_ABORTED;  // Translate to NTSTATUS
        }

        Complete(status);
        return;
    }

    InterlockedIncrement(&_gs_AcceptCount);

    // Create the ISocket via KUmSocket
    //
    _Socket = _new(KTL_TAG_NET, *_Allocator) KUmSocket(_NewSocket, *_Allocator, _IoRegistrationContext);
    InterlockedIncrement(&__KSocket_Accepts);

    if (!_Socket)
    {
        Complete(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    SetSocketAddresses(*_Socket);

    _NewSocket = INVALID_SOCKET;
    _IoRegistrationContext = nullptr;

    Complete(OpStatus);
}

//
//  KUmSocketAcceptOp::KUmSocketAcceptOp
//
KUmSocketAcceptOp::KUmSocketAcceptOp(
    __in SOCKET Listener,
    __in ADDRESS_FAMILY AddressFamily,
    __in KAllocator& Alloc
    )
{
    _Allocator = &Alloc;
    _AddressFamily = AddressFamily;
    _ListenSocket = Listener;
    _NewSocket = INVALID_SOCKET;
    RtlZeroMemory(&_Olap, sizeof(KTL_SOCKET_OVERLAPPED));
    _Olap._KtlObject = this;
    _Olap._KtlObjectType = KTL_SOCKET_OVERLAPPED::eTypeAccept;
    _IoRegistrationContext = nullptr;
    InterlockedIncrement(&__KSocket_PendingOperations);
}

VOID KUmSocketAcceptOp::OnReuse()
{
    _Socket = nullptr;
    if (_NewSocket != INVALID_SOCKET)
    {
        DeleteSocket(_NewSocket);
        _NewSocket = INVALID_SOCKET;
    }

    RtlZeroMemory(_acceptContext, sizeof(_acceptContext));
}

//
// KUmSocketAcceptOp Destructor
//
KUmSocketAcceptOp::~KUmSocketAcceptOp()
{
    // These can occur if the allocation of the new KUmSocket failed.
    //
    if (_IoRegistrationContext)
    {
        _Allocator->GetKtlSystem().DefaultThreadPool().UnregisterIoCompletionCallback(_IoRegistrationContext);
    }

    if (_NewSocket != INVALID_SOCKET)
    {
        DeleteSocket(_NewSocket);
    }

    InterlockedDecrement(&__KSocket_PendingOperations);
}


//
//  KUmSocketListenOp::OnStart
//
//
VOID
KUmSocketListenOp::OnStart()
{
    Complete(STATUS_SUCCESS);
}


//
//  KUmSocketListenOp::KUmSocketListenOp
//
KUmSocketListenOp::KUmSocketListenOp(
    __in SOCKET Listener,
    __in KAllocator& Alloc
    )
{
    UNREFERENCED_PARAMETER(Listener);

    _Allocator = &Alloc;
    InterlockedIncrement(&__KSocket_PendingOperations);
}


//
// KUmSocketListenOp Destructor
//
KUmSocketListenOp::~KUmSocketListenOp()
{
    InterlockedDecrement(&__KSocket_PendingOperations);
}

///////////////////////////////////////////////////////////////////////////////
//
//  KUmSocket
//

//
// KUmSocket::KUmSocket
//
KUmSocket::KUmSocket(
    __in SOCKET Socket,
    __in KAllocator& Alloc,
    __in PVOID IoRegCtx
    )
{
    _Allocator = &Alloc;
    _UserData = nullptr;
    _Socket = Socket;
    _IoRegistrationContext = IoRegCtx;
    InterlockedIncrement(&__KSocket_SocketObjects);

    SetConstructorStatus(STATUS_SUCCESS);
}

//
// KUmSocket::Close
//
VOID
KUmSocket::Close()
{
    _ApiLock.Acquire();
    DeleteSocket(_Socket);
    _Socket = INVALID_SOCKET;
    _ApiLock.Release();
}


//
// KUmSocket Destructor
//
KUmSocket::~KUmSocket()
{
    _Allocator->GetKtlSystem().DefaultThreadPool().UnregisterIoCompletionCallback(_IoRegistrationContext);
    Close();
    InterlockedDecrement(&__KSocket_SocketObjects);
}

//
//  KUmSocket::CreateReadOp
//
NTSTATUS
KUmSocket::CreateReadOp(
    __out ISocketReadOp::SPtr& NewOp
    )
{
    KUmSocketReadOp* NewRead = _new (KTL_TAG_NET, *_Allocator) KUmSocketReadOp(this, *_Allocator);
    if (!NewRead)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    NewOp = NewRead;
    return STATUS_SUCCESS;
}

//
//  KUmSocket::CreateWriteOp
//
NTSTATUS
KUmSocket::CreateWriteOp(
    __out ISocketWriteOp::SPtr& NewOp
    )
{
    KUmSocketWriteOp* NewWrite = _new(KTL_TAG_NET, *_Allocator) KUmSocketWriteOp(this, *_Allocator);
    if (!NewWrite)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    NewOp = NewWrite;
    return STATUS_SUCCESS;
}

void KUmSocket::SetLocalAddr(INetAddress::SPtr && localAddress)
{
    KFatal(!_localAddress);
    _localAddress = Ktl::Move(localAddress);

    char addressString[128];
    ULONG len = sizeof(addressString);
    if (_localAddress->ToStringA(addressString, len) == STATUS_SUCCESS)
    {
        KDbgPrintf("socket %llx: local address: %s", GetSocketId(), addressString);
    }
}

void KUmSocket::SetRemoteAddr(INetAddress::SPtr && remoteAddress)
{
    KFatal(!_remoteAddress);
    _remoteAddress = Ktl::Move(remoteAddress);

    char addressString[128];
    ULONG len = sizeof(addressString);
    if (_remoteAddress->ToStringA(addressString, len) == STATUS_SUCCESS)
    {
        KDbgPrintf("socket %llx: remote address: %s", GetSocketId(), addressString);
    }
}

//
// KUmSocket::GetLocalAddress
//
NTSTATUS
KUmSocket::GetLocalAddress(
    __out INetAddress::SPtr& Addr
    )
{
    Addr = _localAddress;
    return Addr ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

//
//  KUmSocket::GetRemoteAddress
//
NTSTATUS
KUmSocket::GetRemoteAddress(
        __out INetAddress::SPtr& Addr
    )
{
    Addr = _remoteAddress;
    return Addr ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}


///////////////////////////////////////////////////////////////////////////////

//
// KUmSocketConnectOp::KUmSocketConnectOp
//

KUmSocketConnectOp::KUmSocketConnectOp(
    __in KAllocator& Alloc
    )
{
    _Allocator = &Alloc;
    RtlZeroMemory(&_Olap, sizeof(KTL_SOCKET_OVERLAPPED));
    _Olap._KtlObjectType = KTL_SOCKET_OVERLAPPED::eTypeConnect;
    _Olap._KtlObject = this;
    _IoRegistrationContext = nullptr;
    _SocketHandle = INVALID_SOCKET;
    SetConstructorStatus(STATUS_PENDING);
    InterlockedIncrement(&__KSocket_PendingOperations);
}

//
//  KUmSocketConnectOp::~KUmSocketConnectOp
//
KUmSocketConnectOp::~KUmSocketConnectOp()
{
    // These may occur if the KUmSocket failed to allocate
    // in the IOCP_Completion
    //
    if (_IoRegistrationContext)
    {
        _Allocator->GetKtlSystem().DefaultThreadPool().UnregisterIoCompletionCallback(_IoRegistrationContext);
    }
    if (_SocketHandle != INVALID_SOCKET)
    {
        DeleteSocket(_SocketHandle);
        _SocketHandle = INVALID_SOCKET;
    }
    InterlockedDecrement(&__KSocket_PendingOperations);
}


//
//  KUmSocketConnectOp::StartConnect
//
NTSTATUS
KUmSocketConnectOp::StartConnect(
    __in      INetAddress::SPtr& ConnectFrom,
    __in      INetAddress::SPtr& ConnectTo,
    __in_opt  KAsyncContextBase::CompletionCallback Callback,
    __in_opt  KAsyncContextBase* const ParentContext
    )
{
    if (__KSocket_Shutdown)
    {
        return STATUS_SHUTDOWN_IN_PROGRESS;
    }

    _From = ConnectFrom;
    _To = ConnectTo;
    Start(ParentContext, Callback);
    return STATUS_PENDING;
}

//
//  KUmSocketConnectOp::IOCP_Completion
//
VOID
KUmSocketConnectOp::IOCP_Completion(
    __in ULONG OpStatus
    )
{
    KFinally([&] () { ReleaseActivities(1); });

    if (OpStatus != NO_ERROR)
    {
        InterlockedIncrement(&__KSocket_ConnectFailures);
        Complete(STATUS_UNEXPECTED_NETWORK_ERROR);
        return;
    }

    // Create the ISocket via KUmSocket and transfer the info to it.
    //
    _Socket = _new (KTL_TAG_NET, *_Allocator) KUmSocket(_SocketHandle, *_Allocator, _IoRegistrationContext);
    InterlockedIncrement(&__KSocket_Connects);
    if (!_Socket)
    {
        Complete(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    KUmSocket& umSocket = static_cast<KUmSocket&>(*_Socket);
    umSocket.SetRemoteAddr(Ktl::Move(_To));

    SOCKADDR_STORAGE localSocketAddress = {};
    int localAddressSize = sizeof(localSocketAddress);
    if (getsockname(
        _SocketHandle,
        reinterpret_cast<sockaddr*>(&localSocketAddress),
        &localAddressSize) != 0)
    {
        OpStatus = ::GetLastError();
        Complete(OpStatus);
        return;
    }

    INetAddress::SPtr localAddress = _new (KTL_TAG_NET, GetThisAllocator()) KUmNetAddress(&localSocketAddress);
    if (!localAddress)
    {
        Complete(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    umSocket.SetLocalAddr(Ktl::Move(localAddress));

    _IoRegistrationContext = nullptr;
    _SocketHandle = INVALID_SOCKET;

    Complete(OpStatus);
}

//
//  KUmSocketConnectOp::OnStart
//
VOID
KUmSocketConnectOp::OnStart()
{
    LPFN_CONNECTEX pfnConnectEx = NULL;
    GUID GuidConnectEx = WSAID_CONNECTEX;
    DWORD dwBytes;

    // Create the socket
    //
    KUmNetAddress::SPtr To = down_cast<KUmNetAddress, INetAddress>(_To);
    _SocketHandle = MakeSocket(To->Get()->sa_family, SOCK_STREAM, IPPROTO_TCP);
    if (_SocketHandle == INVALID_SOCKET)
    {
        Complete(STATUS_INTERNAL_ERROR);
        return;
    }

    SOCKADDR_STORAGE zeroAddress;
    PSOCKADDR pAddress;
    int size;

    if (_From == nullptr)
    {
        RtlZeroMemory(&zeroAddress, sizeof(zeroAddress));
        zeroAddress.ss_family = To->Get()->sa_family;
        pAddress = PSOCKADDR(&zeroAddress);
        size = sizeof(zeroAddress);
    }
    else
    {
        KUmNetAddress::SPtr From = down_cast<KUmNetAddress, INetAddress>(_From);
        pAddress = From->Get();
        size = (int)From->Size();
    }

    if ( bind( _SocketHandle, pAddress, size )  == SOCKET_ERROR )
    {
       DeleteSocket(_SocketHandle);
       _SocketHandle = INVALID_SOCKET;
       Complete(STATUS_UNSUCCESSFUL);
       return;
    }

    // Now associate it with the IOCP
    //
    NTSTATUS Result = _Allocator->GetKtlSystem().DefaultThreadPool().RegisterIoCompletionCallback(
        HANDLE(_SocketHandle),
        KUmTcpTransport::_Raw_IOCP_Completion,
        nullptr,
        &_IoRegistrationContext
        );

    if (!NT_SUCCESS(Result))
    {
        Complete(STATUS_INTERNAL_ERROR);
        return;
    }

    // Find the ConnectEx pointer
    //
    int iRes = WSAIoctl(_SocketHandle,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &GuidConnectEx,
        sizeof(GuidConnectEx),
        &pfnConnectEx,
        sizeof(pfnConnectEx),
        &dwBytes,
        NULL,
        NULL
        );

    if (iRes != 0)
    {
        Complete(STATUS_INTERNAL_ERROR);
        return;
    }


    // ConnectEx
    //
    DWORD dwSent = 0;
    AcquireActivities(1);
    BOOL Res = pfnConnectEx(
        _SocketHandle,      // Bound socket
        To->Get(),          // Address
        (int)To->Size(),    // Address length
        NULL,               // Initial sent data; not used
        0,                  // Send buffer length
        &dwSent,            // Result of initially sent data; not used in this case,
        &_Olap              // Overlapped IO
        );

    if (Res || (WSAGetLastError() == ERROR_IO_PENDING))
    {
        return;
    }

    Complete(STATUS_INTERNAL_ERROR);
    ReleaseActivities(1);
}




///////////////////////////////////////////////////////////////////////////////

//
// KUmSocketListener::KUmSocketListener
//
KUmSocketListener::KUmSocketListener(
    __in KAllocator& Alloc
    )
{
    _Allocator = &Alloc;
    _ListenSocket = INVALID_SOCKET;
    _IoRegistrationContext = nullptr;
    _AcceptTerminated = FALSE;
}

//
// KUmSocketListener::~KUmSocketListener
//
KUmSocketListener::~KUmSocketListener()
{
    Shutdown();
    if (_IoRegistrationContext)
    {
        _Allocator->GetKtlSystem().DefaultThreadPool().UnregisterIoCompletionCallback(_IoRegistrationContext);
    }
}

//
//  KUmSocketListener::Shutdown
//

VOID
KUmSocketListener::Shutdown()
{
    if (_ListenSocket != INVALID_SOCKET)
    {
        DeleteSocket(_ListenSocket);
        _ListenSocket = INVALID_SOCKET;
    }
}


//
//  KUmSocketListener::Initialize
//
NTSTATUS
KUmSocketListener::Initialize(
    __in INetAddress::SPtr& Address
    )
{
    NTSTATUS Result;

    KSharedPtr<KUmNetAddress> NetAddr = down_cast<KUmNetAddress, INetAddress>(Address);

    _AddressFamily = NetAddr->Get()->sa_family;

    // Create the main listening socket
    //
    _ListenSocket = MakeSocket( _AddressFamily, SOCK_STREAM, IPPROTO_TCP );
    if (_ListenSocket == INVALID_SOCKET)
    {
        return STATUS_UNSUCCESSFUL;
    }

    // Associate the listen socket with an IOCP
    //
    // This callback will be used to handle async 'Accept' operations
    //
    Result = _Allocator->GetKtlSystem().DefaultThreadPool().RegisterIoCompletionCallback(
        HANDLE(_ListenSocket),
        KUmTcpTransport::_Raw_IOCP_Completion,
        nullptr,
        &_IoRegistrationContext
        );

    if (!NT_SUCCESS(Result))
    {
        return STATUS_INTERNAL_ERROR;
    }

    if ( bind( _ListenSocket, NetAddr->Get(), (int)NetAddr->Size())  == SOCKET_ERROR )
    {
        DeleteSocket(_ListenSocket);
        _ListenSocket = INVALID_SOCKET;
        return STATUS_UNSUCCESSFUL;
    }

    // Start listening on the listening socket
    //
    if (listen(_ListenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

//
//  KUmSocketListener::CreateAcceptOp
//
NTSTATUS
KUmSocketListener::CreateAcceptOp(
    __out ISocketAcceptOp::SPtr& Accept
    )
{
    KUmSocketAcceptOp* NewAccept = _new (KTL_TAG_NET, *_Allocator) KUmSocketAcceptOp(_ListenSocket, _AddressFamily, *_Allocator);
    if (!NewAccept)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    Accept = NewAccept;
    return STATUS_SUCCESS;
}

//
//  KUmSocketListener::CreateListenOp
//
NTSTATUS
KUmSocketListener::CreateListenOp(
    __out ISocketListenOp::SPtr& Listen
    )
{
    KUmSocketListenOp* NewListen = _new (KTL_TAG_NET, *_Allocator) KUmSocketListenOp(_ListenSocket, *_Allocator);
    if (!NewListen)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    Listen = NewListen;
    return STATUS_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////

//
//  KUmTcpTransport::KUmTcpTransport
//
KUmTcpTransport::KUmTcpTransport(
    __in KAllocator& Alloc
    ) :
    _SocketTable(Alloc)
{
    _Allocator = &Alloc;
    RtlZeroMemory(&_WsaData, sizeof(WSADATA));
    _Shutdown = FALSE;
}

//
// KUmTcpTransport::~KUmTcpTransport
//
KUmTcpTransport::~KUmTcpTransport()
{
   // Shutdown();
}




//
//  KUmTcpTransport::_Raw_IOCP_Completion
//
//  IOCP callback for all types of operations: Accept, Read, Write.
//  Branching to the correct operation occurs from within here.
//
VOID
KUmTcpTransport::_Raw_IOCP_Completion(
    __in_opt VOID* Context,
    __inout OVERLAPPED* Overlapped,
    __in ULONG Status,
    __in ULONG BytesTransferred
    )
{
    UNREFERENCED_PARAMETER(Context);

    // The Overlapped object is actually the derivative KTL_SOCKET_OVERLAPPED.
    // So, we cast to that and jump back into a member function for this instance.
    //
    KTL_SOCKET_OVERLAPPED* Ovlap = (KTL_SOCKET_OVERLAPPED *) Overlapped;

    if (Ovlap->_KtlObjectType == KTL_SOCKET_OVERLAPPED::eTypeAccept)
    {
        KUmSocketAcceptOp* Accept = (KUmSocketAcceptOp *) Ovlap->_KtlObject;
        Accept->IOCP_Completion(Status);
    }
    else if (Ovlap->_KtlObjectType == KTL_SOCKET_OVERLAPPED::eTypeRead)
    {
        KUmSocketReadOp* Read = (KUmSocketReadOp *) Ovlap->_KtlObject;
        Read->IOCP_Completion(Status, BytesTransferred);
    }
    else if (Ovlap->_KtlObjectType == KTL_SOCKET_OVERLAPPED::eTypeWrite)
    {
        KUmSocketWriteOp* Write = (KUmSocketWriteOp *) Ovlap->_KtlObject;
        Write->IOCP_Completion(Status, BytesTransferred);
    }
    else if (Ovlap->_KtlObjectType == KTL_SOCKET_OVERLAPPED::eTypeConnect)
    {
        KUmSocketConnectOp* Connect = (KUmSocketConnectOp *) Ovlap->_KtlObject;
        Connect->IOCP_Completion(Status);
    }
}

//
//  KUmTcpTransport::Initialize
//
NTSTATUS
KUmTcpTransport::Initialize(
    __in  KAllocator& Alloc,
    __out ITransport::SPtr& Transport
    )
{
    KUmTcpTransport* Trans = _new (KTL_TAG_NET, Alloc) KUmTcpTransport(Alloc);
    if (!Trans)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    int iResult = WSAStartup( MAKEWORD(2,2), &Trans->_WsaData );
    if (iResult)
    {
        _delete(Trans);
        return STATUS_UNSUCCESSFUL;
    }
    Transport = Trans;
    return STATUS_SUCCESS;
}

//
//  KUmTcpTransport::CreateBindAddressOp
//
NTSTATUS
KUmTcpTransport::CreateBindAddressOp(
    __out IBindAddressOp::SPtr& Bind
    )
{
    KUmBindAddressOp* NewOp = _new(KTL_TAG_NET, *_Allocator) KUmBindAddressOp(*_Allocator);
    if (!NewOp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    Bind = NewOp;
    return STATUS_SUCCESS;
}

//
//  KUmTcpTransport::CreateListener
//
NTSTATUS
KUmTcpTransport::CreateListener(
    __in  INetAddress::SPtr& Address,
    __out ISocketListener::SPtr& Listener
    )
{
    KUmSocketListener::SPtr NewListener = _new (KTL_TAG_NET, *_Allocator) KUmSocketListener(*_Allocator);
    if (!NewListener)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    NTSTATUS Result = NewListener->Initialize(Address);
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    Listener = up_cast<ISocketListener, KUmSocketListener>(NewListener);
    _Listener = Listener;
    return STATUS_SUCCESS;
}

//
//  KUmTcpTransport::CreateConnectOp
//
NTSTATUS
KUmTcpTransport::CreateConnectOp(
    __out ISocketConnectOp::SPtr& Connect
    )
{
    KUmSocketConnectOp* NewOp = _new(KTL_TAG_NET, *_Allocator) KUmSocketConnectOp(*_Allocator);
    if (!NewOp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    Connect = NewOp;
    return STATUS_SUCCESS;
}

//
// KumTcpTransport::SpawnDefaultClientAddress
//
NTSTATUS
KUmTcpTransport::SpawnDefaultClientAddress(
    __out INetAddress::SPtr& Address
    )
{
    Address.Reset();

    KArray<SOCKADDR_STORAGE> LocalAddresses(GetThisAllocator());

    NTSTATUS Result = KSocket_EnumLocalAddresses(_Allocator, LocalAddresses);
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    KUmNetAddress::SPtr NewAddress = _new (KTL_TAG_NET, *_Allocator) KUmNetAddress;
    if (!NewAddress)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KMemCpySafe(&NewAddress->_SocketAddress, sizeof(NewAddress->_SocketAddress), &LocalAddresses[0], sizeof(SOCKADDR_STORAGE));
    NewAddress->_Port = 0;

    if (NewAddress->_SocketAddress.ss_family == AF_INET)
    {
        PSOCKADDR_IN(&NewAddress->_SocketAddress)->sin_port = htons(NewAddress->_Port);
    }
    else
    {
        PSOCKADDR_IN6(&NewAddress->_SocketAddress)->sin6_port = htons(NewAddress->_Port);
    }


    Address = down_cast<INetAddress, KUmNetAddress>(NewAddress);

    return STATUS_SUCCESS;
}

//
//  KUmTcpTransport::SpawnDefaultListenerAddresses
//
NTSTATUS
KUmTcpTransport::SpawnDefaultListenerAddresses(
    __inout KArray<INetAddress::SPtr>& AddressList
    )
{
    KArray<SOCKADDR_STORAGE> LocalAddresses(GetThisAllocator());

    NTSTATUS Result = KSocket_EnumLocalAddresses(_Allocator, LocalAddresses);
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    for (ULONG i = 0; i < LocalAddresses.Count(); i++)
    {
        KUmNetAddress::SPtr NewAddress = _new (KTL_TAG_NET, *_Allocator) KUmNetAddress;
        if (!NewAddress)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        KMemCpySafe(&NewAddress->_SocketAddress, sizeof(NewAddress->_SocketAddress), &LocalAddresses[i], sizeof(SOCKADDR_STORAGE));
        NewAddress->_Port = TEMP_DEFAULT_PORT;

        if (NewAddress->_SocketAddress.ss_family == AF_INET)
        {
            PSOCKADDR_IN(&NewAddress->_SocketAddress)->sin_port = htons(NewAddress->_Port);
        }
        else
        {
            PSOCKADDR_IN6(&NewAddress->_SocketAddress)->sin6_port = htons(NewAddress->_Port);
        }

        AddressList.Append(down_cast<INetAddress, KUmNetAddress>(NewAddress));
    }

    return STATUS_SUCCESS;
}

NTSTATUS KUmTcpTransport::CreateNetAddress(SOCKADDR_STORAGE const * address, __out INetAddress::SPtr & netAddress)
{
    netAddress = _new (KTL_TAG_NET, *_Allocator) KUmNetAddress(address);
    if (netAddress)
    {
        return STATUS_SUCCESS;
    }

    return STATUS_INSUFFICIENT_RESOURCES;
}

//
//  KUmTcpTransport::Shutdown
//
VOID
KUmTcpTransport::Shutdown()
{
    _Shutdown = TRUE;

    while (__KSocket_PendingOperations ||
           __KSocket_SocketCount    ||
           __KSocket_AddressCount   ||
           __KSocket_AddressObjects ||
           __KSocket_SocketObjects
           )
    {
        KNt::Sleep(50);
    }

    // Brutally close everything
    WSACleanup();

    //_Listener = 0;
}




#else

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//      KERNEL MODE AREA     //////////////////////////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



//
//  KKmSocketWriteOp::KKmSocketWriteOp
//
KKmSocketWriteOp::KKmSocketWriteOp(
    __in KSharedPtr<KKmSocket> Socket,
    __in KAllocator& Alloc
    )
{
    _Allocator = &Alloc;
    _Socket = Socket;
    InterlockedIncrement(&__KSocket_PendingOperations);

    _DataBuffer.Offset = 0;
    _DataBuffer.Length = 0;
    _DataBuffer.Mdl = nullptr;

    _OpNumber = InterlockedIncrement(&__KSocket_OpNumber);
}


KKmSocketWriteOp::~KKmSocketWriteOp()
{
    InterlockedDecrement(&__KSocket_PendingOperations);
    Cleanup();
}

#if KTL_IO_TIMESTAMP

KIoTimestamps const * KKmSocketWriteOp::IoTimestamps()
{
    return &_timestamps;
}

#endif

//
//  KKmSocketWriteOp::SendComplete
//
_Function_class_(IO_COMPLETION_ROUTINE)
_IRQL_requires_same_
NTSTATUS
KKmSocketWriteOp::_WriteComplete(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);

    KKmSocketWriteOp* Op = (KKmSocketWriteOp*)Context;
    NTSTATUS status = Op->WriteComplete(Irp);
    Op->ReleaseActivities(1);
    return status;
}

//
//  KKmSocketWriteOp::WriteComplete
//
NTSTATUS
KKmSocketWriteOp::WriteComplete(
    PIRP Irp
    )
{
#if KTL_IO_TIMESTAMP
    _timestamps.RecordIoComplete();
#endif

    //KDbgPrintf(
    //    "socket %llx: write completed: status=%x, written=%d",
    //    _Socket->GetSocketId(), Irp->IoStatus.Status, (ULONG)(Irp->IoStatus.Information));

    InterlockedDecrement(&__KSocket_PendingWrites);

    if (Irp->IoStatus.Status == STATUS_SUCCESS)
    {
        // _printf("WskSend() succeeded op=%d\n", _OpNumber);
        _BytesTransferred = (ULONG)(Irp->IoStatus.Information);
        __KSocket_BytesWritten += _BytesTransferred;  // Statistcal; not interlocked by design
        __KSocket_Writes++;

        Complete(STATUS_SUCCESS);
    }

    // Error
    else
    {
        // _printf("!! ERROR----------> WskSend() failure of %u bytes, op=%d\n", _BytesToTransfer, _OpNumber);
        _BytesTransferred = 0;
        InterlockedIncrement(&__KSocket_WriteFailures);
        Complete(Irp->IoStatus.Status);
        __KSocket_LastWriteError = Irp->IoStatus.Status;
    }

    IoFreeIrp(Irp);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

//
// KKmSocketWriteOp::OnStart
//
VOID
KKmSocketWriteOp::OnStart()
{
    if (!_Socket->Ok())
    {
        Complete(STATUS_FILE_CLOSED);
        return;
    }

    // Allocate an IRP
    //
    PIRP Irp = IoAllocateIrp(1, FALSE);
    if (!Irp)
    {
        Complete(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    // Set the completion routine for the IRP
    //
    IoSetCompletionRoutine(
        Irp,
        KKmSocketWriteOp::_WriteComplete,
        this,    // Context
        TRUE,
        TRUE,
        TRUE
        );

    if (!_Socket->Lock())
    {
        IoFreeIrp(Irp);
        Complete(STATUS_FILE_CLOSED);
        return;
    }

    // Get pointer to the provider dispatch structure
    //
    PWSK_PROVIDER_CONNECTION_DISPATCH Dispatch =
        (PWSK_PROVIDER_CONNECTION_DISPATCH)(_Socket->Get()->Dispatch);

    // _printf("WskSend() of %u bytes (op=%d)\n", _BytesToTransfer, _OpNumber);

    InterlockedIncrement(&__KSocket_PendingWrites);
    AcquireActivities(1);

    ULONG sendFlag = (_Priority == ISocketWriteOp::ePriorityHigh) ? WSK_FLAG_NODELAY : 0;

#if KTL_IO_TIMESTAMP
    _timestamps.RecordIoStart();
#endif

    Dispatch->WskSend(
      _Socket->Get(),
      &_DataBuffer,
      sendFlag,
      Irp
      );

    _Socket->Unlock();
}

//
//  KKmSocketWriteOp::StartWrite
//
NTSTATUS
KKmSocketWriteOp::StartWrite(
    __in       ULONG BufferCount,
    __in       KMemRef* Buffers,
    __in       ULONG Priority,
    __in_opt   KAsyncContextBase::CompletionCallback Callback,
    __in_opt   KAsyncContextBase* const ParentContext
    )
{
    if (__KSocket_Shutdown)
    {
        return STATUS_SHUTDOWN_IN_PROGRESS;
    }

    _Priority = Priority;
    _DataBuffer.Offset = 0;

    PMDL Prev = nullptr;
    ULONG Total = 0;
    for (ULONG i = 0; i < BufferCount; i++)
    {
        KMemRef* Current = &Buffers[i];

        PMDL NewMdl = IoAllocateMdl(
            Current->_Address,
            Current->_Param,
            FALSE,
            FALSE,
            NULL
            );

        if (NewMdl == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        MmBuildMdlForNonPagedPool(NewMdl);
        if (i == 0)
        {
            _DataBuffer.Mdl = NewMdl;
            Prev = NewMdl;
        }
        else
        {
            Prev->Next = NewMdl;
            Prev = NewMdl;
        }

        Total += Current->_Param;
    }

    _DataBuffer.Length = Total;
    _BytesToTransfer = Total;

    Start(ParentContext, Callback);
    return STATUS_SUCCESS;
}

//
//  KKmSocketWriteOp::OnReuse
//
VOID
KKmSocketWriteOp::OnReuse()
{
    Cleanup();
}

//
//  KKmSocketWriteOp::Cleanup
//
VOID
KKmSocketWriteOp::Cleanup()
{
#if KTL_IO_TIMESTAMP
    _timestamps.Reset();
#endif

    PMDL Current, Next;
    Current = _DataBuffer.Mdl;
    for (; Current != nullptr; Current = Next)
    {
        Next = Current->Next;
        IoFreeMdl(Current);
    }

    _DataBuffer.Mdl = nullptr;
}

////////////////////////////////////////////////////////////////////////////

//
//  KKmSocketReadOp::KKmSocketReadOp
//
KKmSocketReadOp::KKmSocketReadOp(
    __in KSharedPtr<KKmSocket> Socket,
    __in KAllocator& Alloc
    )
{
    _Allocator = &Alloc;
    _Socket = Socket;
    InterlockedIncrement(&__KSocket_PendingOperations);

    _DataBuffer.Offset = 0;
    _DataBuffer.Length = 0;
    _DataBuffer.Mdl = nullptr;

    _OpNumber = InterlockedIncrement(&__KSocket_OpNumber);
    _BytesTransferred = 0;
    _BytesToTransfer = 0;
}

KKmSocketReadOp::~KKmSocketReadOp()
{
    InterlockedDecrement(&__KSocket_PendingOperations);
    Cleanup();
}

#if KTL_IO_TIMESTAMP

KIoTimestamps const * KKmSocketReadOp::IoTimestamps()
{
    return &_timestamps;
}

#endif

//
//  KKmSocketReadOp::ReadComplete
//
//  IRQL == DISPATCH-LEVEL
//
_Function_class_(IO_COMPLETION_ROUTINE)
_IRQL_requires_same_
NTSTATUS
KKmSocketReadOp::_ReadComplete(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);

    KKmSocketReadOp* Op = (KKmSocketReadOp*)Context;
    NTSTATUS status = Op->ReadComplete(Irp);
    Op->ReleaseActivities(1);
    return status;
}

//
//  KKmSocketReadOp::ReadComplete
//
//  IRQL == DISPATCH-LEVEL
//
NTSTATUS
KKmSocketReadOp::ReadComplete(
    PIRP Irp
    )
{
#if KTL_IO_TIMESTAMP
    _timestamps.RecordIoComplete();
#endif

    //KDbgPrintf(
    //    "socket %llx: read completed: status=%x, read=%d",
    //    _Socket->GetSocketId(), Irp->IoStatus.Status, (ULONG)(Irp->IoStatus.Information));

    InterlockedDecrement(&__KSocket_PendingReads);
    if (Irp->IoStatus.Status == STATUS_SUCCESS)
    {
        // _printf("WskReceive succeeded, op=%d\n", _OpNumber);
        _BytesTransferred = (ULONG)(Irp->IoStatus.Information);
        __KSocket_BytesRead += _BytesTransferred; // Statistical; no need for interlock
        __KSocket_Reads++;

        Complete(STATUS_SUCCESS);
    }

    // Error
    else
    {
        // _printf("WskReceive error.  Bytes=%d, Status = 0x%X Op=%d\n", _BytesToTransfer, Irp->IoStatus.Status, _OpNumber);
        _BytesTransferred = 0;
        InterlockedIncrement(&__KSocket_ReadFailures);
        Complete(Irp->IoStatus.Status);
    }

    IoFreeIrp(Irp);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

//
// KKmSocketReadOp::OnStart
//
VOID
KKmSocketReadOp::OnStart()
{
    if (!_Socket->Ok())
    {
        Complete(STATUS_FILE_CLOSED);
        return;
    }

    // Allocate an IRP
    //
    PIRP Irp = IoAllocateIrp(1, FALSE);
    if (!Irp)
    {
        Complete(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    // Set the completion routine for the IRP
    //
    IoSetCompletionRoutine(
        Irp,
        KKmSocketReadOp::_ReadComplete,
        this,    // Context
        TRUE,
        TRUE,
        TRUE
        );

    if (!_Socket->Lock())
    {
        Complete(STATUS_FILE_CLOSED);
        IoFreeIrp(Irp);
        return;
    }

    // Get pointer to the provider dispatch structure
    //
    PWSK_PROVIDER_CONNECTION_DISPATCH Dispatch =
        (PWSK_PROVIDER_CONNECTION_DISPATCH)(_Socket->Get()->Dispatch);

    ULONG flags = (_flags & ISocketReadOp::ReadAll) ? WSK_FLAG_WAITALL : 0;

    InterlockedIncrement(&__KSocket_PendingReads);
    AcquireActivities(1);

#if KTL_IO_TIMESTAMP
    _timestamps.RecordIoStart();
#endif

    Dispatch->WskReceive(
      _Socket->Get(),
      &_DataBuffer,
      flags,
      Irp
      );

    _Socket->Unlock();
}

//
//  KKmSocketReadOp::StartRead
//
NTSTATUS
KKmSocketReadOp::StartRead(
    __in       ULONG BufferCount,
    __in       KMemRef* Buffers,
    __in       ULONG Flags,
    __in_opt   KAsyncContextBase::CompletionCallback Callback,
    __in_opt   KAsyncContextBase* const ParentContext
    )
{
    if (__KSocket_Shutdown)
    {
        return STATUS_SHUTDOWN_IN_PROGRESS;
    }

    _DataBuffer.Offset = 0;

    PMDL Prev = nullptr;
    ULONG Total = 0;
    for (ULONG i = 0; i < BufferCount; i++)
    {
        KMemRef* Current = &Buffers[i];

        PMDL NewMdl = IoAllocateMdl(
            Current->_Address,
            Current->_Size,
            FALSE,
            FALSE,
            NULL
            );

        if (NewMdl == nullptr)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        MmBuildMdlForNonPagedPool(NewMdl);

        if (i == 0)
        {
            _DataBuffer.Mdl = NewMdl;
            Prev = NewMdl;
        }
        else
        {
            Prev->Next = NewMdl;
            Prev = NewMdl;
        }

        Total += Current->_Size;
    }

    _DataBuffer.Length = Total;
    _BytesToTransfer = Total;
    _flags = Flags;

    Start(ParentContext, Callback);
    return STATUS_SUCCESS;
}

//
//  KKmSocketReadOp::OnReuse
//
VOID
KKmSocketReadOp::OnReuse()
{
    Cleanup();
}

//
// KKmSocketReadOp::Cleanup
//
VOID
KKmSocketReadOp::Cleanup()
{
#if KTL_IO_TIMESTAMP
    _timestamps.Reset();
#endif

    PMDL Current, Next;
    Current = _DataBuffer.Mdl;
    for (; Current != nullptr; Current = Next)
    {
        Next = Current->Next;
        IoFreeMdl(Current);
    }

    _DataBuffer.Mdl = NULL;
}



///////////////////////////////////////////////////////////////////////////////
//

//
//  KKmSocketBindOp::StartBind
//
NTSTATUS
KKmSocketBindOp::StartBind(
   __in KKmSocket::SPtr Socket,
   __in KKmNetAddress::SPtr LocalAddress,
   __in_opt  KAsyncContextBase::CompletionCallback Callback,
   __in_opt  KAsyncContextBase* const ParentContext
   )
{
    if (__KSocket_Shutdown)
    {
        return STATUS_SHUTDOWN_IN_PROGRESS;
    }

    _Socket = Socket;
    _Address = LocalAddress;
    Start(ParentContext, Callback);
    return STATUS_SUCCESS;
}

NTSTATUS
KKmSocketBindOp::Create(
    __in  KAllocator& Alloc,
    __out KKmSocketBindOp::SPtr& BindOp
    )
{
    KKmSocketBindOp* NewBind = _new (KTL_TAG_NET, Alloc) KKmSocketBindOp;
    if (!NewBind)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    BindOp = NewBind;
    return STATUS_SUCCESS;
}

//
//  KKmSocketBindOp::OnStart
//
VOID
KKmSocketBindOp::OnStart()
{
    // Allocate an IRP
    //
    PIRP Irp =
    IoAllocateIrp(
      1,
      FALSE
      );

    // Check result
    //
    if (!Irp)
    {
        // Return error
        Complete(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    // Set the completion routine for the IRP
    IoSetCompletionRoutine(
        Irp,
        KKmSocketBindOp::_BindComplete,
        this,
        TRUE,
        TRUE,
        TRUE
        );

    if (!_Socket->Lock())
    {
        Complete(STATUS_FILE_CLOSED);
        IoFreeIrp(Irp);
        return;
    }

    // Get pointer to the socket's provider dispatch structure
    //
    PWSK_PROVIDER_LISTEN_DISPATCH Dispatch =
         (PWSK_PROVIDER_LISTEN_DISPATCH)(_Socket->Get()->Dispatch);

    PSOCKADDR Addr;
    SOCKADDR_STORAGE storage;

    if (_Address == nullptr)
    {
        RtlZeroMemory(&storage, sizeof(storage));
        storage.ss_family = _Socket->_AddressFamily;
        Addr = PSOCKADDR(&storage);
    }
    else
    {
        Addr = _Address->Get();
    }

    PWSK_SOCKET Socket = _Socket->Get();

    // Initiate the bind operation on the socket
    //
    AcquireActivities(1);
    Dispatch->WskBind(
        Socket,
        Addr,
        0,  // No flags
        Irp
        );

    _Socket->Unlock();
}

//
//  KKmSocketBindOp::_BindComplete
//
_Function_class_(IO_COMPLETION_ROUTINE)
_IRQL_requires_same_
NTSTATUS
KKmSocketBindOp::_BindComplete(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);

    KKmSocketBindOp* This = (KKmSocketBindOp*) Context;
    NTSTATUS status = This->BindComplete(Irp);
    This->ReleaseActivities(1);
    return status;
}

//
//  KKmSocketBindOp::BindComplete
//
NTSTATUS
KKmSocketBindOp::BindComplete(
        PIRP Irp
        )
{
    NTSTATUS Res = Irp->IoStatus.Status;

    if (NT_SUCCESS(Res))
    {
        Complete(Res);
    }

    // Error status
    //
    else
    {
        Complete(Res);
    }

    // Free the IRP
    IoFreeIrp(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}



///////////////////////////////////////////////////////////////////////////////




///////////////////////////////////////////////////////////////////////////////
//
//


//
// KKmTcpTransport::KKmTcpTransport
//
KKmTcpTransport::KKmTcpTransport(
    __in KAllocator& Alloc
    )
{
    _Allocator = &Alloc;

    _RegistrationOk = FALSE;
    _ProviderNpiOk = FALSE;

    RtlZeroMemory(&_WskRegistration, sizeof(WSK_REGISTRATION));
    RtlZeroMemory(&_WskClientDispatch, sizeof(WSK_CLIENT_DISPATCH));
    RtlZeroMemory(&_WskClientNpi, sizeof(WSK_CLIENT_NPI));
    RtlZeroMemory(&_WskProviderNpi, sizeof(WSK_PROVIDER_NPI));
}

//
//  KKmTcpTransport::Initialize
//
NTSTATUS
KKmTcpTransport::Initialize(
    __in  KAllocator& Allocator,
    __out ITransport::SPtr& Transport
    )
{
    KKmTcpTransport* NewInstance = _new (KTL_TAG_NET, Allocator) KKmTcpTransport(Allocator);
    if (!NewInstance)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS Res = NewInstance->_Initialize();
    if (!NT_SUCCESS(Res))
    {
        _delete(NewInstance);
        return Res;
    }

    Transport = NewInstance;
    return STATUS_SUCCESS;
}




//
//  KKmTcpTransport::_Initialize
//
NTSTATUS
KKmTcpTransport::_Initialize()
{
    NTSTATUS Result;

    _WskClientDispatch.Version = MAKE_WSK_VERSION(1, 1);
    _WskClientDispatch.Reserved = 0;
    _WskClientDispatch.WskClientEvent = NULL;

    _WskClientNpi.ClientContext = NULL;
    _WskClientNpi.Dispatch = &_WskClientDispatch;

    Result = WskRegister(&_WskClientNpi, &_WskRegistration);

    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    _RegistrationOk = TRUE;

    Result = WskCaptureProviderNPI(
        &_WskRegistration,
        WSK_NO_WAIT,
        &_WskProviderNpi
        );


    if (Result == STATUS_NOINTERFACE)
    {
        //_printf("Unable to get the right version\n");
    }

    if (Result == STATUS_DEVICE_NOT_READY)
    {
       // _printf("Not ready\n");
    }

    if (NT_SUCCESS(Result))
    {
        _ProviderNpiOk = TRUE;
    }

    return Result;
}

//
//  KKmTcpTransport::Cleanup
//
VOID
KKmTcpTransport::Cleanup()
{
    while (__KSocket_PendingOperations ||
           __KSocket_SocketCount    ||
           __KSocket_AddressCount   ||
           __KSocket_AddressObjects ||
           __KSocket_SocketObjects
           )
    {
        KNt::Sleep(50);
    }

    if (_RegistrationOk)
    {
        WskReleaseProviderNPI(&_WskRegistration);
    }

    if (_ProviderNpiOk)
    {
        WskDeregister(&_WskRegistration);
    }
}

//
//  KKmTcpTransport::Shutdown
//
VOID
KKmTcpTransport::Shutdown()
{
    Cleanup();
}

///////////////////////////////////////////////////////////////////////////////
//

//
// KKmTcpTransport::CreateConnectOp
//
NTSTATUS
KKmTcpTransport::CreateConnectOp(
    __out ISocketConnectOp::SPtr& Connect
    )
{
    NTSTATUS Result;
    KKmSocketConnectOp::SPtr NewConnect;
    Result = KKmSocketConnectOp::Create(*_Allocator, &_WskProviderNpi, NewConnect);

    if (!NT_SUCCESS(Result))
    {
        return Result;
    }
    Connect = up_cast<ISocketConnectOp, KKmSocketConnectOp>(NewConnect);
    return Result;
}


NTSTATUS
KKmTcpTransport::CreateBindAddressOp(
    __out IBindAddressOp::SPtr& Bind
    )
{
    KKmBindAddressOp* NewBind = _new (KTL_TAG_NET, *_Allocator) KKmBindAddressOp(*_Allocator, &_WskProviderNpi);
    if (!NewBind)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    Bind = NewBind;
    return STATUS_SUCCESS;
}


//
//  KKmTcpTransport::CreateListener
//
NTSTATUS
KKmTcpTransport::CreateListener(
    __in  INetAddress::SPtr& Address,
    __out ISocketListener::SPtr& Listener
    )
{
    KKmSocketListener* Listen = _new (KTL_TAG_NET, *_Allocator) KKmSocketListener(*_Allocator, Address, &_WskProviderNpi);
    if (!Listen)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    Listener = Listen;
    return STATUS_SUCCESS;
}


//
// KKmTcpTransport::SpawnDefaultClientAddress
//
NTSTATUS
KKmTcpTransport::SpawnDefaultClientAddress(
    __out INetAddress::SPtr& Address
    )
{
    Address.Reset();
    KKmNetAddress::SPtr NewAddress = _new (KTL_TAG_NET, *_Allocator) KKmNetAddress(L"..localmachine", 0, &_WskProviderNpi, *_Allocator);

    if (!NewAddress)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    if (!NT_SUCCESS(NewAddress->Status()))
    {
        return NewAddress->Status();
    }
    SOCKADDR_STORAGE Addr;
    RtlZeroMemory(&Addr, sizeof(Addr));
    Addr.ss_family = AF_INET;
    NewAddress->SetDirect(PSOCKADDR(&Addr));
    INetAddress::SPtr Tmp = down_cast<INetAddress, KKmNetAddress>(NewAddress);
    Address = Tmp;

    return STATUS_SUCCESS;
}

//
//  KKmTcpTransport::SpawnDefaultListenerAddress
//

NTSTATUS
KKmTcpTransport::SpawnDefaultListenerAddresses(
    __inout KArray<INetAddress::SPtr>& AddressList
    )
{
    // CONFLICT (raymcc)
    //
    KKmNetAddress::SPtr NewAddress = _new (KTL_TAG_NET, *_Allocator) KKmNetAddress(L"..localmachine", TEMP_DEFAULT_PORT, &_WskProviderNpi, *_Allocator);
    if (!NewAddress)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    if (!NT_SUCCESS(NewAddress->Status()))
    {
        return NewAddress->Status();
    }
    SOCKADDR_STORAGE Addr;
    RtlZeroMemory(&Addr, sizeof(Addr));
    Addr.ss_family = AF_INET;
    NewAddress->SetDirect(PSOCKADDR(&Addr));
    AddressList.Append(down_cast<INetAddress, KKmNetAddress>(NewAddress));

    return STATUS_SUCCESS;
}

NTSTATUS KKmTcpTransport::CreateNetAddress(SOCKADDR_STORAGE const * address, __out INetAddress::SPtr & netAddress)
{
    netAddress = _new (KTL_TAG_NET, *_Allocator) KKmNetAddress(address, &_WskProviderNpi, *_Allocator);
    if (netAddress)
    {
        return STATUS_SUCCESS;
    }

    return STATUS_INSUFFICIENT_RESOURCES;
}


///////////////////////////////////////////////////////////////////////////////
//
// KKmBindAddressOp


//
//  KKmBindAddressOp::KKmBindAddressOp
//
KKmBindAddressOp::KKmBindAddressOp(
    __in KAllocator& Alloc,
    __in PWSK_PROVIDER_NPI WskProviderNpi
    )
    :   _StringAddress(Alloc)
{
    _Port = 0;
    _WskProviderNpi = WskProviderNpi;
    _AddressInfo = nullptr;
    _Allocator = &Alloc;
    _UserData = nullptr;
    _addressFamily = AF_UNSPEC;
    InterlockedIncrement(&__KSocket_PendingOperations);
}


//
//  KKmBindAddressOp::~KKmBindAddressOp
//
KKmBindAddressOp::~KKmBindAddressOp()
{
    if (_AddressInfo)
    {
        _WskProviderNpi->Dispatch->WskFreeAddressInfo(
           _WskProviderNpi->Client,
           _AddressInfo
           );
    }

    InterlockedDecrement(&__KSocket_PendingOperations);
}

_Use_decl_annotations_
NTSTATUS KKmBindAddressOp::StartBind(
    PWSTR  StringAddress,
    ADDRESS_FAMILY addressFamily,
    USHORT Port,
    PCWSTR sspiTarget,
    KAsyncContextBase::CompletionCallback Callback,
    KAsyncContextBase* const ParentContext)
{
    if (__KSocket_Shutdown)
    {
        return STATUS_SHUTDOWN_IN_PROGRESS;
    }

    _addressFamily = addressFamily;
    _Port = Port;
    _StringAddress = StringAddress;

    if (!NT_SUCCESS(_StringAddress.Status()))
    {
        // failure in KWString operator=
        return _StringAddress.Status();
    }

    _kmAddress = _new (KTL_TAG_NET, *_Allocator) KKmNetAddress(StringAddress, USHORT(Port), _WskProviderNpi, *_Allocator);
    if (!_kmAddress)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(_kmAddress->Status()))
    {
        return _kmAddress->Status();
    }

    if (sspiTarget != nullptr)
    {
        _address = _new (KTL_TAG_NET, *_Allocator) KSspiAddress(
            up_cast<INetAddress, KKmNetAddress>(_kmAddress),
            sspiTarget);

        if (!_address)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        _address = up_cast<INetAddress, KKmNetAddress>(_kmAddress);
    }

    Start(ParentContext, Callback);
    return STATUS_SUCCESS;
}

NTSTATUS
KKmBindAddressOp::StartBind(
    __in PWSTR  StringAddress,
    __in ADDRESS_FAMILY addressFamily,
    __in USHORT Port,
    __in_opt    KAsyncContextBase::CompletionCallback Callback,
    __in_opt    KAsyncContextBase* const ParentContext)
{
    return StartBind(
        StringAddress,
        addressFamily,
        Port,
        nullptr,
        Callback,
        ParentContext);
}

//
//  KKmBindAddressOp::StartMakeAddress
//
NTSTATUS
KKmBindAddressOp::StartBind(
    __in PWSTR  StringAddress,
    __in USHORT Port,
    __in_opt    KAsyncContextBase::CompletionCallback Callback,
    __in_opt    KAsyncContextBase* const ParentContext
    )
{
    return StartBind(
        StringAddress,
        AF_INET,
        Port,
        nullptr,
        Callback,
        ParentContext);
}

//
//  KKmBindAddressOp::OnStart
//
VOID
KKmBindAddressOp::OnStart()
{
    // Allocate an IRP
    //
    PIRP Irp =
    IoAllocateIrp(
      1,
      FALSE
      );

    // Check result
    //
    if (!Irp)
    {
        // Return error
        Complete(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    // Set the completion routine for the IRP
    IoSetCompletionRoutine(
        Irp,
        KKmBindAddressOp::_AddrComplete,
        this,
        TRUE,
        TRUE,
        TRUE
        );

    ADDRINFOEXW Hints;
    RtlZeroMemory(&Hints, sizeof(ADDRINFOEXW));
    Hints.ai_family = _addressFamily;
    Hints.ai_socktype = SOCK_STREAM;
    Hints.ai_protocol = IPPROTO_TCP;

    AcquireActivities(1);
    _WskProviderNpi->Dispatch->WskGetAddressInfo(
        _WskProviderNpi->Client,
        PUNICODE_STRING(_StringAddress),
        nullptr,                                // No service
        NS_ALL,                                 // namespace
        nullptr,                                // No specific provider
        &Hints,                                 // Hints
        &_AddressInfo,
        NULL,                                   // Process (none)
        NULL,                                   // Thread (none)
        Irp
        );
}


//
//  KKmBindAddressOp::_AddrComplete
//
_Function_class_(IO_COMPLETION_ROUTINE)
_IRQL_requires_same_
NTSTATUS
KKmBindAddressOp::_AddrComplete(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);

    KKmBindAddressOp* This = (KKmBindAddressOp *) Context;
    NTSTATUS status = This->AddrComplete(Irp);
    This->ReleaseActivities(1);
    return status;
}


//
//  KKmBindAddressOp::AddrComplete
//
NTSTATUS
KKmBindAddressOp::AddrComplete(
    PIRP Irp
    )
{
    if (Irp->IoStatus.Status == STATUS_SUCCESS)
    {
        if (_AddressInfo == nullptr || _AddressInfo->ai_addr == nullptr)
        {
            // TODO success but null address
        Complete(STATUS_UNSUCCESSFUL);
        }
        else
        {
            // Transfer ownership to the KKmNetAddress
            //
            _kmAddress->Set(_AddressInfo);
            _AddressInfo = nullptr;
            InterlockedIncrement(&__KSocket_AddressCount);
            Complete(STATUS_SUCCESS);
        }
    }

    // Error status
    //
    else
    {
        Complete(Irp->IoStatus.Status);
    }

    // Free the IRP
    IoFreeIrp(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


///////////////////////////////////////////////////////////////////////////////
//
//  KKmSocketCreateOp


//
//  KKmSocketCreateOp::Create
//
NTSTATUS
KKmSocketCreateOp::Create(
    __in  KAllocator& Alloc,
    __in  PWSK_PROVIDER_NPI Prov,
    __out KKmSocketCreateOp::SPtr& New
    )
{
    KKmSocketCreateOp* NewPtr = _new (KTL_TAG_NET, Alloc) KKmSocketCreateOp(Alloc, Prov);
    if (!NewPtr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    New = NewPtr;
    return STATUS_SUCCESS;
}

//
//  KKmSocketCreateOp::KKmSocketCreateOp
//
KKmSocketCreateOp::KKmSocketCreateOp(
    __in KAllocator& Alloc,
    __in PWSK_PROVIDER_NPI ProviderNpi
    )
{
    _SocketType = 0;
    _WskProviderNpi = ProviderNpi;
    _Allocator = &Alloc;
    InterlockedIncrement(&__KSocket_PendingOperations);
}

//
//  KKmSocketCreateOp::~KKmSocketCreateOp
//
KKmSocketCreateOp::~KKmSocketCreateOp()
{
    InterlockedDecrement(&__KSocket_PendingOperations);
}

//
//  KKmSocketCreateOp::StartCreate
//
NTSTATUS
KKmSocketCreateOp::StartCreate(
    __in      ADDRESS_FAMILY AddressFamily,
    __in      ULONG SocketType,
    __in_opt  KAsyncContextBase::CompletionCallback Callback,
    __in_opt  KAsyncContextBase* const ParentContext
    )
{
    if (__KSocket_Shutdown)
    {
        return STATUS_SHUTDOWN_IN_PROGRESS;
    }

    _AddressFamily = AddressFamily;
    _SocketType = SocketType;
    KKmSocket* New = _new (KTL_TAG_NET, *_Allocator) KKmSocket(*_Allocator, AddressFamily);
    if (!New)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _Socket = New;
    Start(ParentContext, Callback);
    return STATUS_PENDING;
}


//
//  KKmSocketCreateOp::OnStart
//
VOID
KKmSocketCreateOp::OnStart()
{
   // Allocate the closing IRP for the parent socket
    NTSTATUS Status = _Socket->AllocateClosingIrp();
    if (!NT_SUCCESS(Status))
    {
        Complete(Status);
        return;
    }

   // If we get here, we now have a ready-to-run IRP for closing the socket

    PIRP Irp = IoAllocateIrp(1, FALSE);
    if (!Irp)
    {
      // Return error
      //
      Complete(STATUS_INSUFFICIENT_RESOURCES);
      return;
    }

    // Set the completion routine for the IRP
    //
    IoSetCompletionRoutine(
      Irp,
      KKmSocketCreateOp::_CreateComplete,
      this,
      TRUE,
      TRUE,
      TRUE
      );

    // Initiate the creation of the socket
    //
    AcquireActivities(1);

    //
    // Disable prefast warning 6014
    // IRP will be freed in _CreateComplete
    #pragma warning(suppress : 6014)
    _WskProviderNpi->Dispatch->WskSocket(
          _WskProviderNpi->Client,
          _AddressFamily,
          SOCK_STREAM,
          IPPROTO_TCP,
          _SocketType,
          NULL,
          NULL,
          NULL,
          NULL,
          NULL,
          Irp
          );
}


//
//  KKmSocketCreateOp::_CreateComplete
//
_Function_class_(IO_COMPLETION_ROUTINE)
_IRQL_requires_same_
NTSTATUS
KKmSocketCreateOp::_CreateComplete(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);

    KKmSocketCreateOp* This = (KKmSocketCreateOp *) Context;
    NTSTATUS status = This->CreateComplete(Irp);
    This->ReleaseActivities(1);
    return status;
}

//
//  KKmSocketCreateOp::CreateComplete
//
NTSTATUS
KKmSocketCreateOp::CreateComplete(
    PIRP Irp
    )
{
    if (Irp->IoStatus.Status == STATUS_SUCCESS)
    {
        PWSK_SOCKET SocketPtr = (PWSK_SOCKET) Irp->IoStatus.Information;
        ULONG SockType = _SocketType == WSK_FLAG_LISTEN_SOCKET? KKmSocket::eTypeListener : KKmSocket::eTypeOutbound;
        _Socket->Set(SocketPtr, SockType);
        InterlockedIncrement(&__KSocket_SocketCount);
        Complete(STATUS_SUCCESS);
    }

    // Error status
    //
    else
    {
        Complete(Irp->IoStatus.Status);
    }

    // Free the IRP
    IoFreeIrp(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


///////////////////////////////////////////////////////////////////////////////
//
//  KKmSocket
//

NTSTATUS
KKmSocket::CreateReadOp(
    __out ISocketReadOp::SPtr& NewOp
    )
{
    if (!Ok())
    {
        return STATUS_FILE_CLOSED;
    }

    KKmSocketReadOp* NewRead = _new(KTL_TAG_NET, *_Allocator) KKmSocketReadOp(this, *_Allocator);
    if (!NewRead)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    NewOp = NewRead;
    return STATUS_SUCCESS;
}

NTSTATUS
KKmSocket::CreateWriteOp(
    __out ISocketWriteOp::SPtr& NewOp
    )
{
    if (!Ok())
    {
        return STATUS_FILE_CLOSED;
    }

    KKmSocketWriteOp* NewWrite = _new(KTL_TAG_NET, *_Allocator) KKmSocketWriteOp(this, *_Allocator);
    if (!NewWrite)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    NewOp = NewWrite;
    return STATUS_SUCCESS;
}


//
//  KKmSocket::KKmSocket
//
KKmSocket::KKmSocket(
    __in KAllocator& Alloc,
    __in ADDRESS_FAMILY AddressFamily
    )
{
    _AddressFamily = AddressFamily;
    _CloseRequest = FALSE;
    _ApiUseCount = 0;
    _WskSocketPtr = nullptr;
    _UserData = nullptr;
    _Allocator = &Alloc;
    _Type = eTypeNull;
    _pClosingIrp = NULL;
    InterlockedIncrement(&__KSocket_SocketObjects);
}

//
//  KKmSocket::KKmSocket
//
KKmSocket::KKmSocket(
    __in KAllocator& Alloc
    )
{
    _AddressFamily = AF_UNSPEC;
    _CloseRequest = FALSE;
    _ApiUseCount = 0;
    _WskSocketPtr = nullptr;
    _UserData = nullptr;
    _Allocator = &Alloc;
    _Type = eTypeNull;
    _pClosingIrp = NULL;
    InterlockedIncrement(&__KSocket_SocketObjects);
}

//
//
//
KKmSocket::~KKmSocket()
{
    // Note the intentional absence of the
    // corresponding decrement on the socket count.
    // Because of the async closure of sockets,
    // the decrement is in the completion routine.
    InterlockedDecrement(&__KSocket_SocketObjects);
    Close();

    if (_pClosingIrp)
    {
        IoFreeIrp(_pClosingIrp);
    }

}


void KKmSocket::SetLocalAddr(INetAddress::SPtr && localAddress)
{
    KFatal(!_LocalAddr);
    _LocalAddr = Ktl::Move(localAddress);

    char addressString[128];
    ULONG len = sizeof(addressString);
    if (_LocalAddr->ToStringA(addressString, len) == STATUS_SUCCESS)
    {
        KDbgPrintf("socket %llx: local address: %s", GetSocketId(), addressString);
    }
}

void KKmSocket::SetRemoteAddr(INetAddress::SPtr && remoteAddress)
{
    KFatal(!_RemoteAddr);
    _RemoteAddr = Ktl::Move(remoteAddress);

    char addressString[128];
    ULONG len = sizeof(addressString);
    if (_RemoteAddr->ToStringA(addressString, len) == STATUS_SUCCESS)
    {
        KDbgPrintf("socket %llx: remote address: %s", GetSocketId(), addressString);
    }
}

NTSTATUS
KKmSocket::GetLocalAddress(
    __out INetAddress::SPtr& Addr
    )
{
    Addr = _LocalAddr;
    return Addr ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

NTSTATUS
KKmSocket::GetRemoteAddress(
    __out INetAddress::SPtr& Addr
    )
{
    Addr = _RemoteAddr;
    return Addr ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

//
// KKmSocket::Lock
//
BOOLEAN
KKmSocket::Lock()
{
    BOOLEAN RetVal = TRUE;
    _SpinLock.Acquire();
    if (_CloseRequest)
    {
        RetVal = FALSE;
    }
    else
    {
        _ApiUseCount++;
    }
    _SpinLock.Release();
    return RetVal;
}

//
// KKmSocket::Unlock
//
VOID
KKmSocket::Unlock()
{
    BOOLEAN ExecClose = FALSE;
    _SpinLock.Acquire();
    _ApiUseCount--;
    if (_ApiUseCount == 0 && _CloseRequest)
    {
        ExecClose = TRUE;
    }
    _SpinLock.Release();
    if (ExecClose)
    {
        ExecuteClose();
    }
}

//
// KKmSocket::Close
//
VOID
KKmSocket::Close()
{
    BOOLEAN ExecClose = FALSE;
    _SpinLock.Acquire();
    _CloseRequest = TRUE;
    if (_ApiUseCount == 0)
    {
        ExecClose = TRUE;
    }
    _SpinLock.Release();
    if (ExecClose)
    {
        ExecuteClose();
    }
}


VOID
KKmSocket::ExecuteClose()
{
    if (_WskSocketPtr == 0)
    {
        // This special case may occur via a destructor where
        // nobody closed the socket before self-destruct

        return;
    }

    // Get pointer to the socket's provider dispatch structure
    //
    PWSK_PROVIDER_BASIC_DISPATCH Dispatch =
         (PWSK_PROVIDER_BASIC_DISPATCH)(_WskSocketPtr->Dispatch);


    // Set the completion routine for the IRP
    IoSetCompletionRoutine(
        _pClosingIrp,
        KKmSocket::_CloseComplete,
        nullptr,
        TRUE,
        TRUE,
        TRUE
        );

    // No need to do AddRef(), because close completion callback only needs to access static data
    Dispatch->WskCloseSocket(
        _WskSocketPtr,
        _pClosingIrp
        );

    _WskSocketPtr = nullptr;
    _pClosingIrp = nullptr;
}

NTSTATUS
KKmSocket::AllocateClosingIrp()
{
    KAssert(_pClosingIrp == NULL);

    _pClosingIrp = IoAllocateIrp(1, FALSE);
    if (!_pClosingIrp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    return STATUS_SUCCESS;
}

//
//  KKmSocketClose::_CloseComplete
//
//  Note this is static so that it will work correctly
//  when the instance is 'gone'.
//
_Function_class_(IO_COMPLETION_ROUTINE)
_IRQL_requires_same_
NTSTATUS
KKmSocket::_CloseComplete(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Context);

    if (Irp->IoStatus.Status == STATUS_SUCCESS)
    {
        InterlockedDecrement(&__KSocket_SocketCount);
        InterlockedIncrement(&__KSocket_Closes);
    }
    else
    {
        // TBD: Log error
    }

    IoFreeIrp(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}





///////////////////////////////////////////////////////////////////////////////
//
// KKmSocketConnectOp
//


KKmSocketConnectOp::KKmSocketConnectOp(
    __in KAllocator& Alloc,
    __in PWSK_PROVIDER_NPI ProvPtr
    )
{
    _Allocator = &Alloc;
    _WskProviderNpiPtr = ProvPtr;
    RtlZeroMemory(&_localSocketAddress, sizeof(_localSocketAddress));
    InterlockedIncrement(&__KSocket_PendingOperations);
    _OpNumber = InterlockedIncrement(&__KSocket_OpNumber);
}


KKmSocketConnectOp::~KKmSocketConnectOp()
{
    Cleanup();
    InterlockedDecrement(&__KSocket_PendingOperations);

}

//
//  KKmSocketConnectOp::Cleanup
//
VOID
KKmSocketConnectOp::Cleanup()
{
    // TBD
}


//
// KKmSocketConnectOp::Create
//
NTSTATUS
KKmSocketConnectOp::Create(
    __in  KAllocator& Alloc,
    __in  PWSK_PROVIDER_NPI Prov,
    __out KKmSocketConnectOp::SPtr& NewInstance
    )
{
    KKmSocketConnectOp* Tmp = _new (KTL_TAG_NET, Alloc) KKmSocketConnectOp(Alloc, Prov);
    if (!Tmp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    NewInstance = Tmp;
    return STATUS_SUCCESS;
}

//
//  KKmSocketConnectOp::StartConnect
//
//
NTSTATUS
KKmSocketConnectOp::StartConnect(
    __in      INetAddress::SPtr& ConnectFrom,
    __in      INetAddress::SPtr& ConnectTo,
    __in_opt  KAsyncContextBase::CompletionCallback Callback,
    __in_opt  KAsyncContextBase* const ParentContext
    )
{
    if (__KSocket_Shutdown)
    {
        return STATUS_SHUTDOWN_IN_PROGRESS;
    }

    _From = ConnectFrom;
    _To = ConnectTo;

    Start(ParentContext, Callback);
    return STATUS_SUCCESS;
}

//
//  KKmSocketConnectOp::OnStart
//
//  This begins a series of parent-child operations.  First we actually have to create
//  the socket using a KKmSocketCreateOp.  When that is done, we have to bind it,
//  which is done in the completion callback for the KKmSocketCreateOp.
//
VOID
KKmSocketConnectOp::OnStart()
{
    KKmSocketCreateOp::SPtr SocketCreatePtr;
    NTSTATUS Result = KKmSocketCreateOp::Create(*_Allocator, _WskProviderNpiPtr, SocketCreatePtr);

    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        return;
    }

    KAsyncContextBase::CompletionCallback CBack(this, &KKmSocketConnectOp::SocketCreateResult);

    KKmNetAddress::SPtr To = down_cast<KKmNetAddress, INetAddress>(_To);

    Result = SocketCreatePtr->StartCreate(To->Get()->sa_family, WSK_FLAG_CONNECTION_SOCKET, CBack, this);
    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        return;
    }
}



//
//  KKmSocketConnectOp::SocketCreateResult
//
VOID
KKmSocketConnectOp::SocketCreateResult(
    __in KAsyncContextBase* const Parent,
    __in KAsyncContextBase& Op
    )
{
    UNREFERENCED_PARAMETER(Parent);

    // If error, complete this op and we're done.
    KKmSocketCreateOp& CreateReq = (KKmSocketCreateOp&) Op;
    NTSTATUS Result = CreateReq.Status();

    if (!NT_SUCCESS(Result))
    {
        _printf("ERROR --------> KKmSocketConnectOp failure (op=%d) status = 0x%x\n", _OpNumber, Result);
        Complete(Result);
        return;
    }

    // If here, we have a socket.
    //
    _Socket = CreateReq.Get();

    // Next, we have to bind the socket.
    //
    KKmSocketBindOp::SPtr BindOp;

    Result = KKmSocketBindOp::Create(*_Allocator, BindOp);
    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        return;
    }

    KAsyncContextBase::CompletionCallback CBack(this, &KKmSocketConnectOp::SocketBindResult);

    Result = BindOp->StartBind(_Socket, down_cast<KKmNetAddress, INetAddress>(_From), CBack, this);
    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        return;
    }
}


//
//  KKmSocketConnectOp::SocketBindResult
//
VOID
KKmSocketConnectOp::SocketBindResult(
    __in KAsyncContextBase* const Parent,
    __in KAsyncContextBase& Op
    )
{
    UNREFERENCED_PARAMETER(Parent);

    KKmSocketBindOp& BindRequest = (KKmSocketBindOp&) Op;
    NTSTATUS Result = BindRequest.Status();
    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        return;
    }

    ContinueConnect();
}



//
//  KKmSocketConnectOp::_ConnectComplete
//
_Function_class_(IO_COMPLETION_ROUTINE)
_IRQL_requires_same_
NTSTATUS
KKmSocketConnectOp::_ConnectComplete(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);

    KKmSocketConnectOp* Op = (KKmSocketConnectOp*)Context;
    NTSTATUS status = Op->ConnectComplete(Irp);
    Op->ReleaseActivities(1);
    return status;
}


//
// KKmSocketConnectOp::OnStart
//
VOID
KKmSocketConnectOp::ContinueConnect()
{
    // Allocate an IRP
    //
    PIRP Irp = IoAllocateIrp(1, FALSE);

    if (!Irp)
    {
        Complete(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    if (!_Socket->Lock())
    {
        IoFreeIrp(Irp);
        Complete(STATUS_FILE_CLOSED);
        return;
    }

    // Get pointer to the provider dispatch structure
    //
    PWSK_PROVIDER_CONNECTION_DISPATCH Dispatch =
        (PWSK_PROVIDER_CONNECTION_DISPATCH)(_Socket->Get()->Dispatch);

    // Set the completion routine for the IRP
    //
    IoSetCompletionRoutine(
        Irp,
        KKmSocketConnectOp::_ConnectComplete,
        this,    // Context
        TRUE,
        TRUE,
        TRUE
        );

    // Initiate the send operation on the socket
    //
    PWSK_SOCKET PSock = _Socket->Get();

    KKmNetAddress::SPtr NetAddr =  down_cast<KKmNetAddress, INetAddress>(_To);
    PSOCKADDR SockAddr = NetAddr->Get();

    AcquireActivities(1);
    Dispatch->WskConnect(
        PSock,
        SockAddr,
        0,
        Irp
        );

    _Socket->Unlock();
}

//
//  KKmSocketConnectOp::ConnectComplete
//
NTSTATUS
KKmSocketConnectOp::ConnectComplete(
    PIRP Irp
    )
{
    NTSTATUS Res =  Irp->IoStatus.Status;

    if (NT_SUCCESS(Res))
    {
        InterlockedIncrement(&__KSocket_Connects);

        _Socket->SetRemoteAddr(Ktl::Move(_To));
        StartGetLocalAddress(Irp);
    }
    else
    {
        InterlockedIncrement(&__KSocket_ConnectFailures);
        Complete(Res);
        IoFreeIrp(Irp);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

void KKmSocketConnectOp::StartGetLocalAddress(PIRP irp)
{
    IoReuseIrp(irp, STATUS_UNSUCCESSFUL);

    if (!_Socket->Lock())
    {
        IoFreeIrp(irp);
        Complete(STATUS_FILE_CLOSED);
        return;
    }

    // Get pointer to the provider dispatch structure
    //
    PWSK_PROVIDER_CONNECTION_DISPATCH Dispatch =
        (PWSK_PROVIDER_CONNECTION_DISPATCH)(_Socket->Get()->Dispatch);

    // Set the completion routine for the IRP
    //
    IoSetCompletionRoutine(
        irp,
        KKmSocketConnectOp::_GetLocalAddressComplete,
        this,    // Context
        TRUE,
        TRUE,
        TRUE
        );

    // Initiate the send operation on the socket
    //
    PWSK_SOCKET PSock = _Socket->Get();

    AcquireActivities(1);
    Dispatch->WskGetLocalAddress(
        PSock,
        reinterpret_cast<PSOCKADDR>(&_localSocketAddress),
        irp);

    _Socket->Unlock();
}

_Function_class_(IO_COMPLETION_ROUTINE)
_IRQL_requires_same_
NTSTATUS KKmSocketConnectOp::_GetLocalAddressComplete(
    PDEVICE_OBJECT deviceObject,
    PIRP irp,
    PVOID context)
{
    UNREFERENCED_PARAMETER(deviceObject);

    KKmSocketConnectOp* thisPtr = (KKmSocketConnectOp*)context;
    NTSTATUS status = thisPtr->GetLocalAddressComplete(irp);
    thisPtr->ReleaseActivities(1);
    return status;
}

NTSTATUS KKmSocketConnectOp::GetLocalAddressComplete(PIRP irp)
{
   NTSTATUS status = irp->IoStatus.Status;

   if (NT_SUCCESS(status))
   {
       INetAddress::SPtr localAddress = _new (KTL_TAG_NET, GetThisAllocator()) KKmNetAddress(
           &_localSocketAddress,
           _WskProviderNpiPtr,
           GetThisAllocator());

       if (localAddress)
       {
           _Socket->SetLocalAddr(Ktl::Move(localAddress));
           Complete(STATUS_SUCCESS);
       }
       else
       {
           Complete(STATUS_INSUFFICIENT_RESOURCES);
       }
   }
   else
   {
       Complete(status);
   }

   IoFreeIrp(irp);
   return STATUS_MORE_PROCESSING_REQUIRED;
}

///////////////////////////////////////////////////////////////////////////////
//
//


KKmSocketListener::KKmSocketListener(
    __in KAllocator& Alloc,
    __in INetAddress::SPtr& Address,
    __in PWSK_PROVIDER_NPI WskProviderNpi
    )
{
    _Allocator = &Alloc;
    _Address = Address;
    _WskProviderNpi = WskProviderNpi;
    _ListenerSocket = nullptr;
    _UserData = nullptr;
    _AcceptTerminated = FALSE;
}



KKmSocketListener::~KKmSocketListener()
{
    // Tbd, if necessary
}

VOID
KKmSocketListener::Shutdown()
{
    if (_ListenerSocket)
    {
        _ListenerSocket->Close();
    }
}


NTSTATUS
KKmSocketListener::CreateAcceptOp(
    __out ISocketAcceptOp::SPtr& AcceptOp
    )
{
    KKmSocketAcceptOp* Accept = _new (KTL_TAG_NET, *_Allocator) KKmSocketAcceptOp(*_Allocator, _Address, KKmSocketListener::SPtr(this), _WskProviderNpi);
    if (!Accept)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    AcceptOp = Accept;
    return STATUS_SUCCESS;
}

NTSTATUS
KKmSocketListener::CreateListenOp(
    __out ISocketListenOp::SPtr& ListenOp
    )
{
    KKmSocketListenOp* Listen = _new (KTL_TAG_NET, *_Allocator) KKmSocketListenOp(*_Allocator, _Address, KKmSocketListener::SPtr(this), _WskProviderNpi);
    if (!Listen)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    ListenOp = Listen;
    return STATUS_SUCCESS;
}




///////////////////////////////////////////////////////////////////////////////
//

//
//  KKmNetAddress::KKmNetAddress
//
KKmNetAddress::KKmNetAddress(
    __in PWSTR StringAddr,
    __in USHORT Port,
    __in PWSK_PROVIDER_NPI Provider,
    __in KAllocator& Alloc
    )
    : _StringForm(Alloc)
{
    _Port = Port;
    _StringForm = StringAddr;
    _AddressInfo = nullptr;
    _UserData = nullptr;
    _WskProviderNpi = Provider;
    InterlockedIncrement(&__KSocket_AddressObjects);
    RtlZeroMemory(&_RawAddress, sizeof(SOCKADDR_STORAGE));
    _Allocator = &Alloc;
    _FreeAddressOp = _new (KTL_TAG_NET, *_Allocator) KKmFreeAddressOp;
    if (_FreeAddressOp == nullptr)
    {
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
    }
}

KKmNetAddress::KKmNetAddress(
    SOCKADDR_STORAGE const* rawAddress,
    __in PWSK_PROVIDER_NPI Provider,
    __in KAllocator & Alloc)
    : _StringForm(Alloc)
{
    KMemCpySafe(&_RawAddress, sizeof(_RawAddress), rawAddress, sizeof(_RawAddress));

    if (_RawAddress.ss_family == AF_INET)
    {
        _Port = RtlUshortByteSwap(PSOCKADDR_IN(&_RawAddress)->sin_port);
    }
    else
    {
        KFatal(_RawAddress.ss_family == AF_INET6);
        _Port = RtlUshortByteSwap(PSOCKADDR_IN6(&_RawAddress)->sin6_port);
    }

    // todo, leikong, set _StringForm
    _AddressInfo = nullptr;
    _FreeAddressOp = nullptr;

    _WskProviderNpi = Provider;
    _UserData = nullptr;
    _Allocator = &Alloc;

    InterlockedIncrement(&__KSocket_AddressObjects);
}

//
// KKmNetAddress::~KKmNetAddress
//
KKmNetAddress::~KKmNetAddress()
{
    Cleanup();
    InterlockedDecrement(&__KSocket_AddressObjects);
}

//
// KKmNetAddress::Cleanup
//
VOID
KKmNetAddress::Cleanup()
{
    if (_AddressInfo && _FreeAddressOp)
    {
         // This must be an async OP, because this is called from destructor, IRQL can be
         // higher than passive, WskFreeAddressInfo can only be called at passive level.
        _FreeAddressOp->StartFree(_WskProviderNpi, _AddressInfo);
    }
}

void KKmNetAddress::Set(PADDRINFOEXW Inf)
{
    _AddressInfo = Inf;

    if (Inf->ai_addr->sa_family == AF_INET)
    {
#pragma warning(suppress : 24002) // Dealing with IPv4 specifically
        PSOCKADDR_IN Tmp = (PSOCKADDR_IN) Inf->ai_addr;
        Tmp->sin_port = RtlUshortByteSwap(_Port);

        *PSOCKADDR_IN(&_RawAddress) = *Tmp;
        PSOCKADDR_IN(&_RawAddress)->sin_port = Tmp->sin_port;
    }
    else
    {
        PSOCKADDR_IN6 Tmp = (PSOCKADDR_IN6) Inf->ai_addr;
        Tmp->sin6_port = RtlUshortByteSwap(_Port);

        *PSOCKADDR_IN6(&_RawAddress) = *Tmp;
        PSOCKADDR_IN6(&_RawAddress)->sin6_port = Tmp->sin6_port;
    }
}

void KKmNetAddress::SetDirect(PSOCKADDR Src)
{
    if (Src->sa_family == AF_INET)
    {
#pragma warning(suppress : 24002) // Dealing with IPv4 specifically
        *PSOCKADDR_IN(&_RawAddress) = *PSOCKADDR_IN(Src);
        PSOCKADDR_IN(&_RawAddress)->sin_port = RtlUshortByteSwap(_Port);
    }
    else
    {
        *PSOCKADDR_IN6(&_RawAddress) = *PSOCKADDR_IN6(Src);
        PSOCKADDR_IN6(&_RawAddress)->sin6_port = RtlUshortByteSwap(_Port);
    }
}

VOID
KKmFreeAddressOp::OnStart()
    {
        _WskProviderNpi->Dispatch->WskFreeAddressInfo(
            _WskProviderNpi->Client,
            _AddressInfo
            );

    InterlockedDecrement(&__KSocket_AddressCount);
    Complete(STATUS_SUCCESS);
}

#pragma warning(push)

//
// Disable prefast warning 24007:
// This function or type supports IPv4 only.
#pragma warning(disable : 24007)

//
// Disable prefast warning 24002:
// This type is IPv4 specific.
#pragma warning(disable : 24002)

NTSTATUS KKmNetAddress::ToStringW(__out_ecount(len) PWSTR buf, __inout ULONG & len) const
{
    if (AF_INET == _RawAddress.ss_family)
    {
        PSOCKADDR_IN sockAddrIn = (PSOCKADDR_IN)(&_RawAddress);
        return RtlIpv4AddressToStringExW(
            &sockAddrIn->sin_addr,
            sockAddrIn->sin_port,
            buf,
            &len);
    }

    if (AF_INET6 == _RawAddress.ss_family)
    {
        PSOCKADDR_IN6 sockAddrIn6 = (PSOCKADDR_IN6)(&_RawAddress);
        return RtlIpv6AddressToStringExW(
            &sockAddrIn6->sin6_addr,
            sockAddrIn6->sin6_scope_id,
            sockAddrIn6->sin6_port,
            buf,
            &len);
    }

    return STATUS_INVALID_PARAMETER;
}

NTSTATUS KKmNetAddress::ToStringA(__out_ecount(len) PSTR buf, __inout ULONG & len) const
{
    if (AF_INET == _RawAddress.ss_family)
    {
        PSOCKADDR_IN sockAddrIn = (PSOCKADDR_IN)(&_RawAddress);
        return RtlIpv4AddressToStringExA(
            &sockAddrIn->sin_addr,
            sockAddrIn->sin_port,
            buf,
            &len);
    }

    if (AF_INET6 == _RawAddress.ss_family)
    {
        PSOCKADDR_IN6 sockAddrIn6 = (PSOCKADDR_IN6)(&_RawAddress);
        return RtlIpv6AddressToStringExA(
            &sockAddrIn6->sin6_addr,
            sockAddrIn6->sin6_scope_id,
            sockAddrIn6->sin6_port,
            buf,
            &len);
    }

    return STATUS_INVALID_PARAMETER;
}

#pragma warning(pop)

///////////////////////////////////////////////////////////////////////////////
//
//  KKmSocketListenOp
//
//
//  In kernel mode, the accept op is an encapsulation of a number of operations,
//  similar to KKmSocketConnectOp operation.
//
//  The distinct steps are:
//  (1) KKmSocketCreateOp (make the socket)
//  (2) KKmSocketBindOp   (bind it to the local address)
//  (3) KKmSocketListen   (post an accept for incoming connections)
//

KKmSocketListenOp::KKmSocketListenOp(
    __in KAllocator& Alloc,
    __in INetAddress::SPtr& Address,
    __in KKmSocketListener::SPtr Listener,
    __in PWSK_PROVIDER_NPI WskProviderNpi
    )
{
    _Allocator = &Alloc;
    _Address = Address;
    _WskProviderNpi = WskProviderNpi;
    _Listener = Listener;
    InterlockedIncrement(&__KSocket_PendingOperations);
    _AcceptorSocket = nullptr;
    _UserData = nullptr;

    _OpNumber = InterlockedIncrement(&__KSocket_OpNumber);
}


KKmSocketListenOp::~KKmSocketListenOp()
{
    InterlockedDecrement(&__KSocket_PendingOperations);
}


//
// KKmSocketListenOp::StartListen
//
NTSTATUS
KKmSocketListenOp::StartListen(
    __in_opt  KAsyncContextBase::CompletionCallback Callback,
    __in_opt  KAsyncContextBase* const ParentContext
    )
{
    if (__KSocket_Shutdown)
    {
        return STATUS_SHUTDOWN_IN_PROGRESS;
    }

    Start(ParentContext, Callback);
    return STATUS_PENDING;
}

//
//  KKmSocketListenOp::OnStart
//
VOID
KKmSocketListenOp::OnStart()
{
    if (_Listener->_ListenerSocket)
    {
        Complete(STATUS_SUCCESS);
        return;
    }

    // If here, we might have to create the main listener. Go find out.
    //
    KAsyncContextBase::LockAcquiredCallback CBack(this, &KKmSocketListenOp::OnCreateSocketLock);
    AcquireLock(_Listener->_CreateSocketLock, CBack);
}

VOID
KKmSocketListenOp::OnCreateSocketLock(
    __in BOOLEAN IsAcquired,
    __in KAsyncLock & LockAttempted)
{
    UNREFERENCED_PARAMETER(LockAttempted);

    if (!IsAcquired)
    {
        Complete(STATUS_UNSUCCESSFUL);
        return;
    }

    if (_Listener->_ListenerSocket)
    {
        _AcceptorSocket = _Listener->_ListenerSocket;
        ReleaseLock(_Listener->_CreateSocketLock);
        Complete(STATUS_SUCCESS);
        return;
    }

    KKmSocketCreateOp::SPtr SocketCreatePtr;
    NTSTATUS Result = KKmSocketCreateOp::Create(*_Allocator, _WskProviderNpi, SocketCreatePtr);
    if (!NT_SUCCESS(Result))
    {
        ReleaseLock(_Listener->_CreateSocketLock);
        Complete(Result);
        return;
    }

    KAsyncContextBase::CompletionCallback CBack(this, &KKmSocketListenOp::SocketCreateResult);
    KKmNetAddress::SPtr Address = down_cast<KKmNetAddress, INetAddress>(_Address);
    Result = SocketCreatePtr->StartCreate(Address->Get()->sa_family, WSK_FLAG_LISTEN_SOCKET, CBack, this);
    if (!NT_SUCCESS(Result))
    {
        ReleaseLock(_Listener->_CreateSocketLock);
        Complete(Result);
        return;
    }
}

//
// KKmSocketListenOp::SocketCreateResult
//
VOID
KKmSocketListenOp::SocketCreateResult(
    __in KAsyncContextBase* const Parent,
    __in KAsyncContextBase& Op
    )
{
    UNREFERENCED_PARAMETER(Parent);

    // If error, complete this op and we're done.
    //
    KKmSocketCreateOp& CreateReq = (KKmSocketCreateOp&) Op;
    NTSTATUS Result = CreateReq.Status();

    if (!NT_SUCCESS(Result))
    {
        ReleaseLock(_Listener->_CreateSocketLock);
        Complete(Result);
        return;
    }

    // If here, we have an acceptor socket. Hurrah!
    //
    _AcceptorSocket = CreateReq.Get();
    _Listener->_ListenerSocket = _AcceptorSocket;

    // Next, we have to bind the socket.
    //
    KKmSocketBindOp::SPtr BindOp;

    Result = KKmSocketBindOp::Create(*_Allocator, BindOp);
    if (!NT_SUCCESS(Result))
    {
        ReleaseLock(_Listener->_CreateSocketLock);
        Complete(Result);
        return;
    }

    KAsyncContextBase::CompletionCallback CBack(this, &KKmSocketListenOp::SocketBindResult);

    Result = BindOp->StartBind(_AcceptorSocket, down_cast<KKmNetAddress, INetAddress>(_Address), CBack, this);
    if (!NT_SUCCESS(Result))
    {
        ReleaseLock(_Listener->_CreateSocketLock);
        Complete(Result);
        return;
    }
}

//
//  KKmSocketListenOp::SocketBindResult
//
VOID
KKmSocketListenOp::SocketBindResult(
    __in KAsyncContextBase* const Parent,
    __in KAsyncContextBase& Op
    )
{
    UNREFERENCED_PARAMETER(Parent);

    KKmSocketBindOp& BindRequest = (KKmSocketBindOp&) Op;
    NTSTATUS Result = BindRequest.Status();

    ReleaseLock(_Listener->_CreateSocketLock);

    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        return;
    }

    Complete(STATUS_SUCCESS);
}

//
//  KKmSocketListenOp::Cleanup
//
VOID
KKmSocketListenOp::Cleanup()
{
}



///////////////////////////////////////////////////////////////////////////////
//
//  KKmSocketAcceptOp
//
//
//  In kernel mode, the listen op is an encapsulation of a number of operations.
//  Accept listens on demand if necessary and then posts an accept.
//

KKmSocketAcceptOp::KKmSocketAcceptOp(
    __in KAllocator& Alloc,
    __in INetAddress::SPtr& Address,
    __in KKmSocketListener::SPtr Listener,
    __in PWSK_PROVIDER_NPI WskProviderNpi
    )
{
    _Allocator = &Alloc;
    _Address = Address; // todo, leikong, do we really need _Address at all?
    RtlZeroMemory(&_localSocketAddress, sizeof(_localSocketAddress));
    RtlZeroMemory(&_remoteSocketAddress, sizeof(_remoteSocketAddress));
    _WskProviderNpi = WskProviderNpi;
    _Listener = Listener;
    InterlockedIncrement(&__KSocket_PendingOperations);
    _AcceptorSocket = nullptr;
    _ConnectedSocket = nullptr;
    _UserData = nullptr;

    _OpNumber = InterlockedIncrement(&__KSocket_OpNumber);
}


KKmSocketAcceptOp::~KKmSocketAcceptOp()
{
    InterlockedDecrement(&__KSocket_PendingOperations);
}



//
//  KKmSocketAcceptOp::AcceptComplete
//
_Function_class_(IO_COMPLETION_ROUTINE)
_IRQL_requires_same_
NTSTATUS
KKmSocketAcceptOp::_AcceptComplete(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);

    KKmSocketAcceptOp* Op = (KKmSocketAcceptOp*)Context;
    NTSTATUS status = Op->AcceptComplete(Irp);
    Op->ReleaseActivities(1);
    return status;
}

//
//  KKmSocketAcceptOp::AcceptComplete
//
NTSTATUS
KKmSocketAcceptOp::AcceptComplete(
    PIRP Irp
    )
{
   InterlockedDecrement(&__KSocket_PendingAccepts);

   if (Irp->IoStatus.Status == STATUS_SUCCESS)
   {
        // _printf("KKmSocketAcceptOp::AcceptComplete Succeeded (Op=%d)\n", _OpNumber);

       PWSK_SOCKET Tmp = (PWSK_SOCKET)(Irp->IoStatus.Information);
       InterlockedIncrement(&__KSocket_SocketCount);
       InterlockedIncrement(&__KSocket_Accepts);
       _ConnectedSocket->Set(Tmp, KKmSocket::eTypeInbound);

       INetAddress::SPtr localAddress = _new (KTL_TAG_NET, GetThisAllocator()) KKmNetAddress(
           &_localSocketAddress,
           _WskProviderNpi,
           GetThisAllocator());

       INetAddress::SPtr remoteAddress = _new (KTL_TAG_NET, GetThisAllocator()) KKmNetAddress(
           &_remoteSocketAddress,
           _WskProviderNpi,
           GetThisAllocator());

       if (localAddress && remoteAddress)
       {
           _ConnectedSocket->SetLocalAddr(Ktl::Move(localAddress));
           _ConnectedSocket->SetRemoteAddr(Ktl::Move(remoteAddress));
           Complete(STATUS_SUCCESS);
       }
       else
       {
           Complete(STATUS_INSUFFICIENT_RESOURCES);
       }
   }

   // Error
   else
   {
       // _printf("!! ERROR----------> KKmSocketAcceptOp::AcceptComplete Failed: Status code =0x%x op=%d\n", Irp->IoStatus.Status, _OpNumber);
       Complete(Irp->IoStatus.Status);
       InterlockedIncrement(&__KSocket_AcceptFailures);
   }

   IoFreeIrp(Irp);
   return STATUS_MORE_PROCESSING_REQUIRED;
}

//
// KKmSocketAcceptOp::ContinueAccept
//
VOID
KKmSocketAcceptOp::ContinueAccept()
{
    // Allocate an IRP
    //
    PIRP Irp = IoAllocateIrp(1, FALSE);

    if (!Irp)
    {
        Complete(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    // Set the completion routine for the IRP
    //
    IoSetCompletionRoutine(
        Irp,
        KKmSocketAcceptOp::_AcceptComplete,
        this,    // Context
        TRUE,
        TRUE,
        TRUE
        );

    if (!_AcceptorSocket->Lock())
    {
        IoFreeIrp(Irp);
        Complete(STATUS_FILE_CLOSED);
        return;
    }

    // Get pointer to the provider dispatch structure
    //
    PWSK_PROVIDER_LISTEN_DISPATCH Dispatch =
        (PWSK_PROVIDER_LISTEN_DISPATCH)(_AcceptorSocket->Get()->Dispatch);


    InterlockedIncrement(&__KSocket_PendingAccepts);

    // _printf("KKmSocketAcceptOp:: WskAccept() call op=%d\n", _OpNumber);

    // Initiate the accept operation on the socket
    //
    AcquireActivities(1);
    Dispatch->WskAccept(
      _AcceptorSocket->Get(),
      0,
      this,
      NULL,     // May re-enable this later
      reinterpret_cast<PSOCKADDR>(&_localSocketAddress),
      reinterpret_cast<PSOCKADDR>(&_remoteSocketAddress),
      Irp
      );

    _AcceptorSocket->Unlock();
}

//
// KKmSocketAcceptOp::StartAccept
//
NTSTATUS
KKmSocketAcceptOp::StartAccept(
    __in_opt  KAsyncContextBase::CompletionCallback Callback,
    __in_opt  KAsyncContextBase* const ParentContext
    )
{
    if (__KSocket_Shutdown)
    {
        return STATUS_SHUTDOWN_IN_PROGRESS;
    }

    Start(ParentContext, Callback);
    return STATUS_PENDING;
}

//
//  KKmSocketAcceptOp::OnStart
//
VOID
KKmSocketAcceptOp::OnStart()
{
    // Create a place to hold the final connected socket
    //
    KKmSocket* Tmp = _new(KTL_TAG_NET, *_Allocator) KKmSocket(*_Allocator);
    if (!Tmp)
    {
        Complete(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    _ConnectedSocket = Tmp;

   // Allocate the closing IRP for the parent socket

    NTSTATUS Status = _ConnectedSocket->AllocateClosingIrp();
    if (!NT_SUCCESS(Status))
    {
        Complete(Status);
        return;
    }

   // If we get here, we now have a ready-to-run IRP for closing the socket

    if (_Listener->_ListenerSocket)
    {
        _AcceptorSocket = _Listener->_ListenerSocket;
        ContinueAccept();
        return;
    }

    ISocketListenOp::SPtr listenOp;
    NTSTATUS status = _Listener->CreateListenOp(listenOp);

    if (NT_SUCCESS(status))
    {
        KAsyncContextBase::CompletionCallback CBack(this, &KKmSocketAcceptOp::SocketListenResult);
        listenOp->StartListen(CBack, this);
    }
    else
    {
        Complete(status);
    }
}

//
// KKmSocketAcceptOp::OnListenComplete
//
VOID
KKmSocketAcceptOp::SocketListenResult(
    __in KAsyncContextBase* const Parent,
    __in KAsyncContextBase& Op
    )
{
    UNREFERENCED_PARAMETER(Parent);

    NTSTATUS status = Op.Status();

    if (NT_SUCCESS(status))
    {
        _AcceptorSocket = _Listener->_ListenerSocket;
        ContinueAccept();
    }
    else
    {
        Complete(status);
    }
}



//
//  KKmSocketAcceptOp::Cleanup
//
VOID
KKmSocketAcceptOp::Cleanup()
{
}

#endif


//
//  KSocket_EnumLocalAddresses
//
//  Helper to enumerate available IP addresses at startup.
//
NTSTATUS
KSocket_EnumLocalAddresses(
    __in KAllocator* Alloc,
    __inout KArray<SOCKADDR_STORAGE>& Addresses
    )
{
    ULONG i;
    NTSTATUS Result = 0;
    PMIB_UNICASTIPADDRESS_TABLE pipTable = NULL;

    UNREFERENCED_PARAMETER(Alloc);

    Result = GetUnicastIpAddressTable(AF_INET, &pipTable);

    if (!NT_SUCCESS(Result))
    {
        return STATUS_INTERNAL_ERROR;
    }

    KFinally([&](){ FreeMibTable(pipTable); });

    _printf("**** ADDRESS ENUM:  Table Size = %u\n", pipTable->NumEntries);

    // Allocate InfoRow from the heap so that it will not overflow the stack
    KUniquePtr<MIB_IF_ROW2> InfoRow = _new(KTL_TAG_NET, *Alloc) MIB_IF_ROW2;
    if (!InfoRow)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for (i = 0; i < pipTable->NumEntries; i++)
    {
        RtlZeroMemory((PMIB_IF_ROW2)InfoRow, sizeof(*InfoRow));
        InfoRow->InterfaceIndex = pipTable->Table[i].InterfaceIndex;
     
        Result = GetIfEntry2((PMIB_IF_ROW2)InfoRow);
        if (!NT_SUCCESS(Result))
        {
            return STATUS_INTERNAL_ERROR;
        }

        //
        // Skip loopback adapters
        //
        if (InfoRow->Type != IF_TYPE_ETHERNET_CSMACD && (InfoRow->Type != IF_TYPE_IEEE80211))
        {
            continue;
        }

        //
        // Eliminate network adapters not in UP state.
        // These adapters may have stale IP addresses from their last up.
        // Binding to them will fail.
        //
        if (InfoRow->OperStatus != IfOperStatusUp)
        {
            continue;
        }

        if (pipTable->Table[i].Address.si_family != AF_INET)
        {
            continue; // Filter out IPv6 for now
        }

        // Filter out various types of virtualized IP addresses.
        // This is really a hack until we find out which field really contains
        // the bits we need.
        //
//        if (memcmp((void*) L"Local Area Connection", (void *) InfoRow->Alias, 20) != 0 &&
//            wcscmp(L"Wireless Network Connection", InfoRow->Alias) != 0)
//        {
//            continue;
//        }

        // Store the address.
        //
        SOCKADDR_STORAGE storage;
        *(PSOCKADDR_IN)&storage = *(PSOCKADDR_IN)&pipTable->Table[i].Address.Ipv4;

        // TODO copying to a storage first involves a double copy
        // TODO what's the right way to IPv6 enable this
        Result = Addresses.Append(storage);
        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        // _printf("**** ADDRESS ENUM: Accepted address %u.%u.%u.%u\n", UCHAR(tmp->sa_data[2]), UCHAR(tmp->sa_data[3]), UCHAR(tmp->sa_data[4]), UCHAR(tmp->sa_data[5]));
    }

    _printf("**** ADDRESS ENUM: Returning a total of %u addresses\n", Addresses.Count());

    return STATUS_SUCCESS;
}

#if KTL_IO_TIMESTAMP

KIoTimestamps::KIoTimestamps()
{
    Reset();
}

ULONGLONG KIoTimestamps::IoStartTime() const
{
    return _ioStartTime;
}

ULONGLONG KIoTimestamps::IoCompleteTime() const
{
    return _ioCompleteTime;
}

void KIoTimestamps::Reset()
{
    _ioStartTime = 0;
    _ioCompleteTime = 0;
}

_Use_decl_annotations_
void KIoTimestamps::RecordCurrentTime(ULONGLONG & timestamp)
{
    timestamp = KNt::GetTickCount64();
}

void KIoTimestamps::RecordIoStart()
{
    RecordCurrentTime(_ioStartTime);
}

void KIoTimestamps::RecordIoComplete()
{
    RecordCurrentTime(_ioCompleteTime);
}

#endif
