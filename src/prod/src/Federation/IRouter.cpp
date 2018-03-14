// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Federation
{
    using namespace std;
    using namespace Common;

    AsyncOperationSPtr IRouter::BeginRoute(
        Transport::MessageUPtr && message,
        NodeId nodeId,
        uint64 instance,
        bool useExactRouting,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return OnBeginRoute(move(message), nodeId, instance, L"", useExactRouting, FederationConfig::GetConfig().RoutingRetryTimeout, timeout, callback, parent);
    }

    AsyncOperationSPtr IRouter::BeginRoute(
        Transport::MessageUPtr && message,
        NodeId nodeId,
        uint64 instance,
        bool useExactRouting,
        TimeSpan retryTimeout,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return OnBeginRoute(move(message), nodeId, instance, L"", useExactRouting, retryTimeout, timeout, callback, parent);
    }

    AsyncOperationSPtr IRouter::BeginRoute(
        Transport::MessageUPtr && message,
        NodeId nodeId,
        uint64 instance,
        std::wstring const & toRing,
        bool useExactRouting,
        TimeSpan retryTimeout,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return OnBeginRoute(move(message), nodeId, instance, toRing, useExactRouting, retryTimeout, timeout, callback, parent);
    }

    AsyncOperationSPtr IRouter::BeginRouteRequest(
        Transport::MessageUPtr && request,
        NodeId nodeId,
        uint64 instance,
        bool useExactRouting,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return OnBeginRouteRequest(move(request), nodeId, instance, L"", useExactRouting, FederationConfig::GetConfig().RoutingRetryTimeout, timeout, callback, parent);
    }

    AsyncOperationSPtr IRouter::BeginRouteRequest(
        Transport::MessageUPtr && request,
        NodeId nodeId,
        uint64 instance,
        bool useExactRouting,
        TimeSpan retryTimeout,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return OnBeginRouteRequest(move(request), nodeId, instance, L"", useExactRouting, retryTimeout, timeout, callback, parent);
    }
    AsyncOperationSPtr IRouter::BeginRouteRequest(
        Transport::MessageUPtr && request,
        NodeId nodeId,
        uint64 instance,
        std::wstring const & toRing,
        bool useExactRouting,
        TimeSpan retryTimeout,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return OnBeginRouteRequest(move(request), nodeId, instance, toRing, useExactRouting, retryTimeout, timeout, callback, parent);
    }

}
