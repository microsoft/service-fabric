// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class Sockets
    {
    public:
        static bool Startup();
        static bool IsAFSupported (AddressFamily::Enum);
        static bool IsIPv6Supported ();
        static bool IsIPv4Supported ();
    };

    class Socket
    {
        DENY_COPY(Socket)

    public:
        enum { ADDRESS_LENGTH = sizeof(SOCKADDR_STORAGE) + 16 };
        enum { MIN_ACCEPTEX_BUFFER_SIZE = 2 * ADDRESS_LENGTH };

        Socket() : handle_(INVALID_SOCKET) {}
        explicit Socket(SOCKET handle) ;
        Socket ( Socket && r );
        ~Socket() ;

        ErrorCode Open(
            SocketType::Enum const & socketType,
            AddressFamily::Enum const & addressFamily = AddressFamily::InterNetwork);

        ErrorCode Shutdown(SocketShutdown::Enum how);
        ErrorCode Close(SocketShutdown::Enum how);
        bool closed() const;

        ErrorCode Bind(const Endpoint& localEP);
        ErrorCode Listen(int backlog = SOMAXCONN);

        Endpoint GetLocalEP() const;

        ErrorCode GetSocketOption(int level, int name, int& value);
        ErrorCode SetSocketOption(int level, int name, int value);
        ErrorCode SetSocketOption(int level, int name, const char * optVal, int optLen);

        ErrorCode SetNonBlocking();
        ErrorCode EnableFastLoopback();

        void swap(Socket && r);
        void swap(Socket & r);

        SOCKET GetHandle() const;
        SOCKET DetachHandle();

        void WriteTo(TextWriter & w, FormatOptions const &) const;

    private:
        SOCKET handle_;

#ifndef PLATFORM_UNIX
    public:
        ErrorCode AcceptEx(
            Socket const & acceptSocket,
            PVOID lpOutputBuffer,
            DWORD dwReceiveDataLength,
            __in   LPDWORD lpdwBytesReceived,
            __in   LPOVERLAPPED lpOverlapped
            );

        void GetAcceptExSockaddrs(
            PVOID lpOutputBuffer,
            DWORD dwReceiveDataLength,
            SOCKET_ADDRESS & local,
            SOCKET_ADDRESS & remote);

        ErrorCode ConnectEx(
            Endpoint remoteEP,
            __in_opt  PVOID lpSendBuffer,
            __in      DWORD dwSendDataLength,
            __out     LPDWORD lpdwBytesSent,
            __in      LPOVERLAPPED lpOverlapped);

        ErrorCode DisconnectEx(__in LPOVERLAPPED lpOverlapped);
        ErrorCode AsyncReceive(BufferSequence_t const & b, __in LPWSAOVERLAPPED lpOverlapped);
        ErrorCode AsyncSend(ConstBufferSequence_t const & b, __in LPWSAOVERLAPPED lpOverlapped);

        void InitExtensionFunctions();

    private:
        LPFN_ACCEPTEX AcceptEx_;
        LPFN_GETACCEPTEXSOCKADDRS GetAcceptExSockaddrs_;
        LPFN_CONNECTEX ConnectEx_;
        LPFN_DISCONNECTEX DisconnectEx_;
#endif
    }; //Socket

    inline void swap(Socket & a, Socket & b) { a.swap(b); }
};
