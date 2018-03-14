// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;
using namespace std;

static const StringLiteral TraceType("ListenSocket");

ListenSocket::ListenSocket(
    Endpoint const & endpoint,
    AcceptCompleteCallback && acceptCompleteCallback,
    wstring const & traceId)
    : IListenSocket(endpoint, move(acceptCompleteCallback), traceId)
    , eventLoop_(IDatagramTransport::GetDefaultTransportEventLoopPool()->Assign())
    , fdContext_(nullptr)
{
}

void ListenSocket::AcceptCallback(int sd, uint events)
{
    Invariant(sd == listenSocket_.GetHandle());

    if (events & EPOLLERR)
    {
        WriteError(TraceType, traceId_, "events = {0:x}, EPOLLERR set", events);

        int sockError;
        socklen_t sockErrorSize = sizeof(sockError);
        if (getsockopt(listenSocket_.GetHandle(), SOL_SOCKET, SO_ERROR, &sockError, &sockErrorSize) < 0)
        {
            auto error = ErrorCode::FromErrno();
            WriteError(
                TraceType, traceId_,
                "getsockopt({0:x}) failed: {1}",
                listenSocket_.GetHandle(), 
                error);

            acceptCompleteCallback_(*this, error);
            return;
        }

        acceptCompleteCallback_(
            *this,
            sockError? ErrorCode::FromErrno(sockError) : ErrorCodeValue::OperationFailed);
        return;
    }

    socklen_t addrLen = sizeof(sockaddr_in6);
    auto acceptedSd = accept(
        listenSocket_.GetHandle(),
        acceptedRemoteEndpoint_.as_sockaddr(),
        &addrLen);

    if (acceptedSd < 0)
    {
        auto error = ErrorCode::FromErrno();
        WriteError(TraceType, traceId_, "accept failed: {0}", error);
        acceptCompleteCallback_(*this, error);
        return;
    }

    acceptedSocket_.swap(Socket(acceptedSd));
    auto error = acceptedSocket_.SetNonBlocking();
    if (!error.IsSuccess())
    {
         WriteError(
            TraceType, traceId_,
            "failed to make socket {0:x} non-blocking with error: {1}",
            acceptedSocket_.GetHandle(),
            error);
    }

    acceptCompleteCallback_(*this, error);
}

ErrorCode ListenSocket::OnOpen()
{
    if (TransportConfig::GetConfig().TestOnlyValidateIPv6Usage && !listenEndpoint_.IsIPv6())
    {   
        // fail open if this test-only setting config is enabled and the endpoint is not IPv6
        return ErrorCodeValue::InvalidAddress;
    }

    ErrorCode error = listenSocket_.Open(SocketType::Tcp, listenEndpoint_.AddressFamily);
    if (!error.IsSuccess())
    {
        return error;
    }

    error = listenSocket_.SetSocketOption(SOL_SOCKET, SO_REUSEADDR, 1);
    if (!error.IsSuccess())
    {
         WriteError(
            TraceType, traceId_,
            "failed to enable SO_REUSEADDR: {0}", 
            error);
    }
 
    error = Bind();
    if (!error.IsSuccess())
    {
        if (error.IsErrno(EADDRINUSE))
        {
            WriteWarning(
                    TraceType, traceId_,
                    "failed to bind on {0}, already in use by {1}",
                    listenEndpoint_.ToString(),
                    TestPortHelper::GetTcpPortOwnerInfo(listenEndpoint_.Port));

            error = ErrorCodeValue::AddressAlreadyInUse;
            return error;
        }

        WriteWarning(
            TraceType, traceId_,
            "failed to bind on {0} with error: {1}",
            listenEndpoint_.ToString(),
            error);

        return error;
    }

    if (listenEndpoint_.Port == 0)
    {
        error = AdjustSockName();
        if (!error.IsSuccess())
        {
            WriteError(
                TraceType, traceId_,
                "failed to get local name of listening socket: {0}",
                error);

            return error;
        }
    }

    auto listenBacklog = (TransportConfig::GetConfig().TcpListenBacklog > 0) ? TransportConfig::GetConfig().TcpListenBacklog : SOMAXCONN;
    error = listenSocket_.Listen(listenBacklog);
    WriteTrace(
        error.IsSuccess() ? LogLevel::Info : LogLevel::Error,
        TraceType, traceId_,
        "starts listening on {0} with backlog = 0x{1:x} : {2}",
        listenEndpoint_.ToString(),
        listenBacklog,
        error);

    if (!error.IsSuccess())
    {
        WriteError(TraceType, traceId_, "failed to listen: {0}", error);
        return error;
    }

    error = listenSocket_.SetNonBlocking();
    if (!error.IsSuccess())
    {
         WriteError(
            TraceType, traceId_,
            "failed to make listen socket nonblocking: {0}", 
            error);

         return error; 
    }

    fdContext_ = eventLoop_.RegisterFd(
        listenSocket_.GetHandle(),
        EPOLLIN,
        true,
        [this] (int sd, uint events) { AcceptCallback(sd, events); });

    return error;
}

ErrorCode ListenSocket::OnClose()
{
    if (fdContext_)
    {
        eventLoop_.UnregisterFd(fdContext_, true); 
    }

    return listenSocket_.Close(SocketShutdown::None);
}

void ListenSocket::OnAbort()
{
    OnClose();
}

ErrorCode ListenSocket::SubmitAccept()
{
    if (suspended_)
    {
        suspended_ = false;
        trace.AcceptResumed(traceId_, listenEndpoint_.ToString());
    }

    eventLoop_.Activate(fdContext_); 
    return ErrorCode();
}
