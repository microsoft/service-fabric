// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;
using namespace std;

static const StringLiteral TraceType("ListenSocket");

IListenSocket::IListenSocket(
    Endpoint const & endpoint,
    AcceptCompleteCallback && acceptCompleteCallback,
    wstring const & traceId)
    : listenEndpoint_(endpoint)
    , acceptCompleteCallback_(move(acceptCompleteCallback))
    , traceId_(traceId)
    , suspended_(false)
{
}

Endpoint const & IListenSocket::ListenEndpoint() const
{
    return listenEndpoint_;
}

void IListenSocket::SuspendAccept()
{
    suspended_ = true;
}

bool IListenSocket::AcceptSuspended() const
{
    return suspended_;
}

Socket & IListenSocket::AcceptedSocket()
{
    return acceptedSocket_;
}

Common::Endpoint const& IListenSocket::AcceptedRemoteEndpoint() const
{
    return acceptedRemoteEndpoint_;
}

ErrorCode IListenSocket::Bind()
{
    auto bindEndpoint = listenEndpoint_;

    if (TransportConfig::GetConfig().AlwaysListenOnAnyAddress && !listenEndpoint_.IsLoopback())
    {
        if (listenEndpoint_.IsIPv4())
        {
            bindEndpoint = Common::IPv4AnyAddress;
            bindEndpoint.Port = listenEndpoint_.Port;
            WriteInfo(
                    TraceType, traceId_,
                    "will listen on 'any' address {0}",
                    bindEndpoint.ToString());
        }
        else
        {
            bindEndpoint = Common::IPv6AnyAddress;
            bindEndpoint.Port = listenEndpoint_.Port;
            WriteInfo(
                    TraceType, traceId_,
                    "will listen on 'any' address {0}",
                    bindEndpoint.ToString());
        }
    }

    return listenSocket_.Bind(bindEndpoint);
}

ErrorCode IListenSocket::AdjustSockName()
{
    //
    // Our default configuration is to listen on IpAddrAny for non loopback addresses. The listenEndpoint_
    // member still points to the ip that is given. So for dynamic listen ports, we should only overwrite
    // the port in the listenEndpoint_ member.
    //
    Endpoint currentListenEndpoint;
    auto error = Endpoint::GetSockName(listenSocket_, currentListenEndpoint);
    if (error.IsSuccess())
    {
        listenEndpoint_.Port = currentListenEndpoint.Port;
    }

    return error;
}
