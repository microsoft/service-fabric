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
{
}

ErrorCode ListenSocket::SubmitAccept()
{
    auto error = acceptedSocket_.Open(SocketType::Tcp, listenEndpoint_.AddressFamily);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType, traceId_,
            "{0}: failed to open socket to accept: {1}",
            listenEndpoint_.ToString(), error);
        Complete(error, 0);
        return error;
    }

    io_.StartIo();

    DWORD bytesTransferred = 0;
    error = listenSocket_.AcceptEx(
        acceptedSocket_,
        connectData_,
        sizeof(connectData_),
        &bytesTransferred,
        OverlappedPtr());

    if (error.IsSuccess() || error.IsWin32Error(WSA_IO_PENDING))
    {
        if (suspended_)
        {
            suspended_ = false;
            trace.AcceptResumed(traceId_, listenEndpoint_.ToString());
        }

        return ErrorCode();
    }

    io_.CancelIo();

    Complete(error, 0);
    return error;
}

void ListenSocket::OnComplete(PTP_CALLBACK_INSTANCE, ErrorCode const & result, ULONG_PTR)
{
    auto error = result;
    if (error.IsSuccess())
    {
        SOCKET rawSocketHandle = acceptedSocket_.GetHandle();
        error = acceptedSocket_.SetSocketOption(SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (const char*)&rawSocketHandle, sizeof(rawSocketHandle));
        if (!error.IsSuccess())
        {
            WriteError(
                TraceType, traceId_,
                "{0}: failed to set SO_UPDATE_ACCEPT_CONTEXT on accepted socket: {1}, shutdown(SD_SEND) will fail",
                listenEndpoint_.ToString(), error);
        }

        SOCKET_ADDRESS localAddress;
        SOCKET_ADDRESS remoteAddress;

        listenSocket_.GetAcceptExSockaddrs(connectData_, sizeof(connectData_), localAddress, remoteAddress);

        // Need to work around an ISA firewall client bug, where remote address retrieval
        // may fail silently with ISA firewall client installed on the machine
        if (!remoteAddress.lpSockaddr || (remoteAddress.iSockaddrLength <= 0))
        {
            WriteError(
                TraceType, traceId_,
                "{0}: accept completed, but failed to retrieve remote address",
                listenEndpoint_.ToString());

            error = ErrorCodeValue::OperationFailed;
        }

        acceptedRemoteEndpoint_ = Endpoint(remoteAddress);
    }

    if (!error.IsSuccess())
    {
        acceptedSocket_.Close(SocketShutdown::None);
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

    ErrorCode errorCode = listenSocket_.Open(SocketType::Tcp, listenEndpoint_.AddressFamily);
    if (!errorCode.IsSuccess())
    {
        return errorCode;
    }

    errorCode = Bind();
    if (!errorCode.IsSuccess())
    {
        wstring ownerInfo;
        bool ownerInfoRetrieved =
            listenEndpoint_.IsIPv4() ?
            TestPortHelper::GetTcpPortOwnerInfo(listenEndpoint_.Port, ownerInfo).IsSuccess() :
            TestPortHelper::GetTcp6PortOwnerInfo(listenEndpoint_.Port, ownerInfo).IsSuccess();

        if (ownerInfoRetrieved)
        {
            WriteWarning(
                TraceType, traceId_,
                "failed to bind on {0}, error = {1}, port {2} already held by {3}",
                listenEndpoint_.ToString(),
                errorCode,
                listenEndpoint_.Port,
                ownerInfo);
        }
        else
        {
            WriteWarning(
                TraceType, traceId_,
                "failed to bind on {0} with error: {1}, port owner info not found",
                listenEndpoint_.ToString(),
                errorCode);
        }

        if (errorCode.IsWin32Error(WSAEADDRINUSE) || errorCode.IsWin32Error(WSAEACCES))
        {
            // port conflict
            errorCode = ErrorCodeValue::AddressAlreadyInUse;
        }

        return errorCode;
    }

    if (listenEndpoint_.Port == 0)
    {
        errorCode = AdjustSockName();
        if (!errorCode.IsSuccess())
        {
            WriteError(
                TraceType, traceId_,
                "failed to get local name of listening socket: {0}",
                errorCode);

            return errorCode;
        }
    }

    auto listenBacklog = (TransportConfig::GetConfig().TcpListenBacklog > 0) ? TransportConfig::GetConfig().TcpListenBacklog : SOMAXCONN;
    errorCode = listenSocket_.Listen(listenBacklog);
    WriteTrace(
        errorCode.IsSuccess() ? LogLevel::Info : LogLevel::Error,
        TraceType, traceId_,
        "starts listening on {0} with backlog = 0x{1:x} : {2}",
        listenEndpoint_.ToString(),
        listenBacklog,
        errorCode);

    if (!errorCode.IsSuccess())
    {
        WriteError(TraceType, traceId_, "failed to listen: {0}", errorCode);
        return errorCode;
    }

    errorCode = io_.Open((HANDLE)listenSocket_.GetHandle());
    if (!errorCode.IsSuccess())
    {
        WriteError(TraceType, traceId_, "failed to create threadpool IO: {0}", errorCode);
        return errorCode;
    }

    TcpTransportUtility::EnableTcpFastLoopbackIfNeeded(listenSocket_, traceId_);

    return ErrorCodeValue::Success;
}

ErrorCode ListenSocket::OnClose()
{
    io_.Close();
    listenSocket_.Close(SocketShutdown::None);
    return ErrorCodeValue::Success;
}

void ListenSocket::OnAbort()
{
    OnClose();
}
