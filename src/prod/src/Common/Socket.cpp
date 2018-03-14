// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using std::swap;

#ifdef PLATFORM_UNIX

int WSAAPI closesocket(
    _In_ SOCKET s
    )
{
    close(s);
    return 0;
}

#endif

static INIT_ONCE initSocketOnce_ = INIT_ONCE_STATIC_INIT;            
static bool wsockStarted = Sockets::Startup(); 

bool Sockets::IsAFSupported (AddressFamily::Enum af)
{
    switch (af)
    {
    case AddressFamily::InterNetwork:
        return IsIPv4Supported();

    case AddressFamily::InterNetworkV6:
        return IsIPv6Supported();

    default:
        coding_error("AddressFamily Must be InterNetwork or InterNetworkV6.");
    }
}

//
// This method is from the IPv6 wiki at
// http://wiki/default.aspx/Microsoft.Projects.Tnlpdev/Ipv6Faq.html
//
bool Sockets::IsIPv4Supported()
{
    SOCKET sock = socket( AF_INET, SOCK_DGRAM, 0 );
    bool supported = (sock != INVALID_SOCKET) || (::WSAGetLastError() != WSAEAFNOSUPPORT);
    ::closesocket( sock );

    return supported;
}

bool Sockets::IsIPv6Supported()
{
    SOCKET sock = socket( AF_INET6, SOCK_DGRAM, 0 );
    bool supported = (sock != INVALID_SOCKET) || (::WSAGetLastError() != WSAEAFNOSUPPORT);
    ::closesocket( sock );

    return supported;
}

Socket::Socket(SOCKET handle) 
    : handle_(CHK_SOCKET_HANDLE( handle )) 
{
#ifndef PLATFORM_UNIX
    InitExtensionFunctions();
#endif
}

Socket::Socket(Socket && r)
    : handle_(r.handle_)
#ifndef PLATFORM_UNIX
    , AcceptEx_(r.AcceptEx_)
    , GetAcceptExSockaddrs_(r.GetAcceptExSockaddrs_)
    , ConnectEx_(r.ConnectEx_)
    , DisconnectEx_(r.DisconnectEx_)
#endif
{
    r.handle_ = INVALID_SOCKET;
}

void Socket::swap(Socket && r)
{
    handle_ = r.handle_;
    r.handle_ = INVALID_SOCKET;
#ifndef PLATFORM_UNIX
    AcceptEx_ = r.AcceptEx_;
    GetAcceptExSockaddrs_ = r.GetAcceptExSockaddrs_;
    ConnectEx_ = r.ConnectEx_;
    DisconnectEx_ = r.DisconnectEx_;
#endif
}

void Socket::swap(Socket & r)
{
    std::swap(handle_, r.handle_);
#ifndef PLATFORM_UNIX
    std::swap(AcceptEx_, r.AcceptEx_);
    std::swap(GetAcceptExSockaddrs_, r.GetAcceptExSockaddrs_);
    std::swap(ConnectEx_, r.ConnectEx_);
    std::swap(DisconnectEx_, r.DisconnectEx_);
#endif
}

Socket::~Socket()
{
    Close(SocketShutdown::None);
}

bool Socket::closed() const
{
    return INVALID_SOCKET == handle_;
}

SOCKET Socket::GetHandle() const
{
    return handle_;
}

SOCKET Socket::DetachHandle()
{ 
    SOCKET detachedHandle = handle_; 
    handle_ = INVALID_SOCKET; 
    return detachedHandle;
}

Endpoint Socket::GetLocalEP() const
{
    return Endpoint(*this);
}

void Socket::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("<socket handle='{0:x}'/>", handle_);
}

ErrorCode Socket::Shutdown(SocketShutdown::Enum how)
{
    if (handle_ == INVALID_SOCKET) 
        return ErrorCode::Success();

    if (how != SocketShutdown::None)
    {
        if (::shutdown(handle_, how) != 0)
        {
            return ErrorCode::FromWin32Error(::WSAGetLastError());
        }
    }

    return ErrorCode::Success();
}

ErrorCode Socket::Close(SocketShutdown::Enum how)
{
    if (handle_ == INVALID_SOCKET) 
        return ErrorCode::Success();

    if (how != SocketShutdown::None)
    {
        if (::shutdown(handle_, how) != 0)
        {
            return ErrorCode::FromWin32Error(::WSAGetLastError());
        }
    }

    if (::closesocket(handle_) != 0)
    {
        return ErrorCode::FromWin32Error(::WSAGetLastError());
    }

    handle_ = INVALID_SOCKET;
    return ErrorCode::Success();
}

ErrorCode Socket::Bind(const Endpoint& localEP)
{
    if (::bind(handle_, localEP, localEP.AddressLength) != 0)
    {
        return ErrorCode::FromWin32Error(::WSAGetLastError());
    }

    return ErrorCode::Success();
}

ErrorCode Socket::Listen(int backlog)
{
    if (::listen(handle_, backlog) != 0)
    {
        return ErrorCode::FromWin32Error(::WSAGetLastError());
    }

    return ErrorCode::Success();
}

ErrorCode Socket::GetSocketOption(int level, int name, int& value)
{
    socklen_t optlen = sizeof(value);
    if (getsockopt(handle_, level, name, (char*)(&value), &optlen) != 0)
    {
        return ErrorCode::FromWin32Error(::WSAGetLastError());
    }

    return ErrorCode::Success();
}

ErrorCode Socket::SetSocketOption(int level, int name, const char * optVal, int optLen)
{
    if (::setsockopt(handle_, level, name, optVal, optLen) != 0)
    {
        return ErrorCode::FromWin32Error(::WSAGetLastError());
    }

    return ErrorCode::Success();
}

ErrorCode Socket::SetSocketOption(int level, int name, int value)
{
    return SetSocketOption(level, name, reinterpret_cast<const char*>(&value), sizeof(value));
}

#ifdef PLATFORM_UNIX

bool Sockets::Startup()
{
    return true;
}

ErrorCode Socket::Open(
    SocketType::Enum const & socketType,
    AddressFamily::Enum const & addressFamily)
{
    int type    = (SocketType::Tcp == socketType) ? SOCK_STREAM : SOCK_DGRAM;
    int proto    = (SocketType::Tcp == socketType) ? IPPROTO_TCP : IPPROTO_UDP;

    handle_ = socket(static_cast<int>(addressFamily), type | SOCK_CLOEXEC, proto);
    if (handle_ == -1)
    {
        return ErrorCode::FromErrno();
    }

    return ErrorCode::Success();
}

ErrorCode Socket::SetNonBlocking()
{
    auto flags = fcntl(handle_, F_GETFL, NULL);
    if (flags < 0)
    {
        return ErrorCode::FromErrno();
    }

    if (fcntl(handle_, F_SETFL, flags|O_NONBLOCK) < 0)
    {
        return ErrorCode::FromErrno();
    }

    return ErrorCode();
}

ErrorCode Socket::EnableFastLoopback()
{
    return ErrorCodeValue::NotImplemented;
}

#else

ErrorCode Socket::Open(
    SocketType::Enum const & socketType,
    AddressFamily::Enum const & addressFamily)
{
    int type    = (SocketType::Tcp == socketType) ? SOCK_STREAM : SOCK_DGRAM;
    int proto    = (SocketType::Tcp == socketType) ? IPPROTO_TCP : IPPROTO_UDP;

    handle_ = ::WSASocket( static_cast<int>( addressFamily ),
        type,
        proto,
        nullptr,0,
        WSA_FLAG_OVERLAPPED);

    if (handle_ == INVALID_SOCKET)
    {
        return ErrorCode::FromWin32Error(::WSAGetLastError());
    }

    InitExtensionFunctions();

    return ErrorCode::Success();
}

ErrorCode Socket::AsyncReceive(BufferSequence_t const & b, __in LPWSAOVERLAPPED lpOverlapped)
{
    DWORD flags = 0;
    DWORD bytesTransferred = 0;

    int result = WSARecv(handle_, b.GetWSABUFs(), b.GetBufCount(),
        &bytesTransferred,
        &flags,
        lpOverlapped,
        nullptr); // completion callback

    if (result)
        return ErrorCode::FromWin32Error(::WSAGetLastError());

    return ErrorCode::Success();
}

ErrorCode Socket::AsyncSend(ConstBufferSequence_t const & b, __in LPWSAOVERLAPPED lpOverlapped)
{
    DWORD bytesTransferred = 0;

    int result = WSASend(handle_, b.GetWSABUFs(), b.GetBufCount(),
        &bytesTransferred,
        0, // flags
        lpOverlapped,
        nullptr); // completion callback

    if (result)
        return ErrorCode::FromWin32Error(::WSAGetLastError());

    return ErrorCode::Success();
}

static void InitExtensionFunction(SOCKET socket, PVOID lpfn, GUID const & guid)
{
    DWORD bytes;
    ASSERT_IF(
        WSAIoctl(
            socket, 
            SIO_GET_EXTENSION_FUNCTION_POINTER,
            const_cast<GUID*>(&guid), sizeof(guid),
            lpfn, sizeof(lpfn), &bytes, NULL, NULL) != 0,
        "Failed to get extension function pointer");
}

static GUID WSAGUID_ACCEPTEX = WSAID_ACCEPTEX;
static GUID WSAGUID_GETACCEPTEXSOCKADDRS = WSAID_GETACCEPTEXSOCKADDRS;
static GUID WSAGUID_CONNECTEX = WSAID_CONNECTEX;
static GUID WSAGUID_DISCONNECTEX = WSAID_DISCONNECTEX;

void Socket::InitExtensionFunctions()
{
    InitExtensionFunction(handle_, &AcceptEx_, WSAGUID_ACCEPTEX);
    InitExtensionFunction(handle_, &GetAcceptExSockaddrs_, WSAGUID_GETACCEPTEXSOCKADDRS);
    InitExtensionFunction(handle_, &ConnectEx_, WSAGUID_CONNECTEX);
    InitExtensionFunction(handle_, &DisconnectEx_, WSAGUID_DISCONNECTEX);
}

ErrorCode Socket::AcceptEx(
    Socket const & acceptSocket,
    PVOID lpOutputBuffer,
    DWORD dwReceiveDataLength,
    __in   LPDWORD lpdwBytesReceived,
    __in   LPOVERLAPPED lpOverlapped
    )
{
    ASSERT_IF(dwReceiveDataLength < MIN_ACCEPTEX_BUFFER_SIZE, "see AcceptEx in msdn");
    ASSERT_IF(AcceptEx_ == nullptr, "AcceptEx extension is not available");

    if (AcceptEx_(handle_, acceptSocket.GetHandle(),
        lpOutputBuffer, dwReceiveDataLength - MIN_ACCEPTEX_BUFFER_SIZE,
        ADDRESS_LENGTH, ADDRESS_LENGTH,
        lpdwBytesReceived,
        lpOverlapped) )
    {
        return ErrorCode::Success();
    }

    return ErrorCode::FromWin32Error(::WSAGetLastError());
}

void Socket::GetAcceptExSockaddrs(
    PVOID lpOutputBuffer,
    DWORD dwReceiveDataLength,
    SOCKET_ADDRESS & local,
    SOCKET_ADDRESS & remote)
{
    ASSERT_IF( GetAcceptExSockaddrs_ == nullptr , "GetAcceptExSockaddrs not availalbe");

    GetAcceptExSockaddrs_(
        lpOutputBuffer,
        dwReceiveDataLength - MIN_ACCEPTEX_BUFFER_SIZE,
        ADDRESS_LENGTH,
        ADDRESS_LENGTH,
        &local.lpSockaddr,
        &local.iSockaddrLength,
        &remote.lpSockaddr,
        &remote.iSockaddrLength);
}

ErrorCode Socket::ConnectEx(
    Endpoint remoteEP,
    __in_opt  PVOID lpSendBuffer,
    __in      DWORD dwSendDataLength,
    __out     LPDWORD lpdwBytesSent,
    __in      LPOVERLAPPED lpOverlapped)
{
    ASSERT_IF(ConnectEx_ == nullptr , "ConnextEx is not available");

    if(ConnectEx_(
        handle_,
        remoteEP.get_AsSockAddr(),
        remoteEP.get_AddressLength(),
        lpSendBuffer,
        dwSendDataLength,
        lpdwBytesSent,
        lpOverlapped))
    {
        return ErrorCode();
    }

    return ErrorCode::FromWin32Error(::WSAGetLastError());
}

ErrorCode Socket::DisconnectEx(__in LPOVERLAPPED lpOverlapped)
{
    if (this->DisconnectEx_(handle_, lpOverlapped, 0, 0))
    {
        return ErrorCode::Success();
    }

    return ErrorCode::FromWin32Error(::WSAGetLastError());
}

ErrorCode Socket::EnableFastLoopback()
{
    BOOL enable = TRUE;
    DWORD bytesReturned = 0;

    if (WSAIoctl(
        handle_,
        SIO_LOOPBACK_FAST_PATH,
        &enable,
        sizeof(enable),
        nullptr,
        0,
        &bytesReturned,
        0,
        0) == 0)
    {
        return ErrorCodeValue::Success;
    }

    return ErrorCode::FromWin32Error(::WSAGetLastError());
}

static ::WSADATA wsaData = {};

static BOOL SocketsStartupImpl(
    PINIT_ONCE InitOnce,
    PVOID Parameter, 
    PVOID *lpContext)
{
    UNREFERENCED_PARAMETER(InitOnce);
    UNREFERENCED_PARAMETER(Parameter);
    UNREFERENCED_PARAMETER(lpContext);
    int status = WSAStartup( 0x0202, &wsaData );
    ASSERT_IFNOT(status == 0, "WSAStartup failed: {0}", ::WSAGetLastError());

    return (status==0)?TRUE:FALSE;
}

bool Sockets::Startup()
{
    PVOID context = NULL;
    return ::InitOnceExecuteOnce(
        &initSocketOnce_,
        SocketsStartupImpl,
        NULL,
        &context) != 0 ;        
}

#endif
