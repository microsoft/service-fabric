// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

// This header file contains all shared, unique, weak pointer types well-known in transport
// as well as forward declarations of corresponding types

namespace Transport
{
    class Message;
    typedef std::unique_ptr<Message> MessageUPtr;

    class IDatagramTransport;
    typedef std::shared_ptr<IDatagramTransport> IDatagramTransportSPtr;

    class TcpDatagramTransport;
    typedef std::shared_ptr<TcpDatagramTransport> TcpDatagramTransportSPtr;
    typedef std::weak_ptr<TcpDatagramTransport> TcpDatagramTransportWPtr;

    class TcpSendTarget;
    typedef std::shared_ptr<TcpSendTarget> TcpSendTargetSPtr;
    typedef std::weak_ptr<TcpSendTarget> TcpSendTargetWPtr;

    class TcpConnection;
    typedef std::shared_ptr<TcpConnection> TcpConnectionSPtr;
    typedef std::weak_ptr<TcpConnection> TcpConnectionWPtr;

    class SecurityCredentials;
    typedef std::shared_ptr<SecurityCredentials> SecurityCredentialsSPtr;

    struct IConnection;
    typedef std::shared_ptr<IConnection> IConnectionSPtr;
    typedef std::weak_ptr<IConnection> IConnectionWPtr;
    class TcpConnection;
    typedef std::shared_ptr<TcpConnection> TcpConnectionSPtr;

    class RequestAsyncOperation;
    typedef std::shared_ptr<RequestAsyncOperation> RequestAsyncOperationSPtr;

    struct NamedAddress;

    class SecuritySettings;
    class SecurityContext;
    typedef std::shared_ptr<SecurityContext> SecurityContextSPtr;
    class SecurityContextSsl;
    class SecurityContextWin;

    class TransportSecurity;
    typedef std::shared_ptr<TransportSecurity> TransportSecuritySPtr;

    class PerfCounters;
    typedef std::shared_ptr<PerfCounters> PerfCountersSPtr;

    class Throttle;
    class AcceptThrottle;
    class IBufferFactory;

#ifdef PLATFORM_UNIX
    
typedef struct _SecBuffer {
	unsigned long cbBuffer;		// Size of the buffer, in bytes
	unsigned long BufferType;	// Type of the buffer (below)
	void * pvBuffer;		// Pointer to the buffer
} SecBuffer, * PSecBuffer;

typedef struct _SecBufferDesc {
	unsigned long ulVersion;
	unsigned long cBuffers;
	PSecBuffer pBuffers;
} SecBufferDesc, *PSecBufferDesc;

#endif
}
