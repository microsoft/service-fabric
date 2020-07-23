// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpApplicationGateway
{
    class IServiceResolver
    {
    public:
        virtual ~IServiceResolver() {};

        virtual Common::AsyncOperationSPtr BeginOpen(
            Common::TimeSpan &timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        virtual Common::ErrorCode EndOpen(Common::AsyncOperationSPtr const &operation) = 0;

        virtual Common::AsyncOperationSPtr BeginPrefixResolveServicePartition(
            ParsedGatewayUriSPtr const &parsedUri,
            Api::IResolvedServicePartitionResultPtr &prevResult,
            std::wstring const &traceId,
            KAllocator &,
            Common::TimeSpan &timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;
        
        virtual Common::ErrorCode EndResolveServicePartition(
            Common::AsyncOperationSPtr const &operation,
            Api::IResolvedServicePartitionResultPtr &prevResult) = 0;

        virtual Common::ErrorCode GetReplicaEndpoint(
            Api::IResolvedServicePartitionResultPtr const &rspResult,
            TargetReplicaSelector::Enum targetReplicaSelector,
            __out std::wstring &) = 0;
        
        virtual std::wstring GetServiceName(Api::IResolvedServicePartitionResultPtr const &rspResult) = 0;
    };
}
