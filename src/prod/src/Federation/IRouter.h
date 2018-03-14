// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    struct IRouter
    {
        Common::AsyncOperationSPtr BeginRoute(
            Transport::MessageUPtr && message,
            NodeId nodeId,
            uint64 instance,
            bool useExactRouting,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::AsyncOperationSPtr BeginRoute(
            Transport::MessageUPtr && message,
            NodeId nodeId,
            uint64 instance,
            bool useExactRouting,
            Common::TimeSpan retryTimeout,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::AsyncOperationSPtr BeginRoute(
            Transport::MessageUPtr && message,
            NodeId nodeId,
            uint64 instance,
            std::wstring const & toRing,
            bool useExactRouting,
            Common::TimeSpan retryTimeout,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        virtual Common::ErrorCode EndRoute(Common::AsyncOperationSPtr const & operation) = 0;

        Common::AsyncOperationSPtr BeginRouteRequest(
            Transport::MessageUPtr && request,
            NodeId nodeId,
            uint64 instance,
            bool useExactRouting,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::AsyncOperationSPtr BeginRouteRequest(
            Transport::MessageUPtr && request,
            NodeId nodeId,
            uint64 instance,
            bool useExactRouting,
            Common::TimeSpan retryTimeout,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::AsyncOperationSPtr BeginRouteRequest(
            Transport::MessageUPtr && request,
            NodeId nodeId,
            uint64 instance,
            std::wstring const & toRing,
            bool useExactRouting,
            Common::TimeSpan retryTimeout,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        virtual Common::ErrorCode EndRouteRequest(Common::AsyncOperationSPtr const & operation, Transport::MessageUPtr & reply) const = 0;

    protected:
        virtual Common::AsyncOperationSPtr OnBeginRoute(
            Transport::MessageUPtr && message,
            NodeId nodeId,
            uint64 instance,
            std::wstring const & toRing,
            bool useExactRouting,
            Common::TimeSpan retryTimeout,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::AsyncOperationSPtr OnBeginRouteRequest(
            Transport::MessageUPtr && request,
            NodeId nodeId,
            uint64 instance,
            std::wstring const & toRing,
            bool useExactRouting,
            Common::TimeSpan retryTimeout,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;
    };
}
