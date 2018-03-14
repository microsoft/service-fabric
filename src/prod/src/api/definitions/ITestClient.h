// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{

    class ITestClient
    {
    public:
        virtual ~ITestClient() {};

        virtual Common::ErrorCode GetNthNamingPartitionId(
            ULONG n,
            __inout Reliability::ConsistencyUnitId &partitionId) = 0;

        virtual Common::AsyncOperationSPtr BeginResolvePartition(
            Reliability::ConsistencyUnitId const& partitionId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndResolvePartition(
            Common::AsyncOperationSPtr const & operation,
            __inout IResolvedServicePartitionResultPtr &result) = 0;

        virtual Common::ErrorCode EndResolvePartition(
            Common::AsyncOperationSPtr const & operation,
            __inout Naming::ResolvedServicePartitionSPtr &result) = 0;

        virtual Common::AsyncOperationSPtr BeginResolveNameOwner(
            Common::NamingUri const& name,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndResolveNameOwner(
            Common::AsyncOperationSPtr const & operation,
            __inout IResolvedServicePartitionResultPtr &result) = 0;

        virtual Common::ErrorCode NodeIdFromNameOwnerAddress(
            std::wstring const& address,
            Federation::NodeId & federationNodeId) = 0;

        virtual Common::ErrorCode NodeIdFromFMAddress(
            std::wstring const& address,
            Federation::NodeId & federationNodeId) = 0;

        virtual Common::ErrorCode GetCurrentGatewayAddress(
            __inout std::wstring &result) = 0;
    };
}
