// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace SystemServices
{
    class SystemServiceResolver;
    typedef std::shared_ptr<SystemServiceResolver> SystemServiceResolverSPtr;

    class SystemServiceResolver
    {
    public:
        static SystemServiceResolverSPtr Create(
            __in Query::QueryGateway & query,
            __in Reliability::ServiceResolver & resolver,
            Common::ComponentRoot const &);

        Common::AsyncOperationSPtr BeginResolve(
            std::wstring const & serviceName,
            Common::ActivityId const &,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginResolve(
            Reliability::ConsistencyUnitId const &,
            Common::ActivityId const &,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndResolve(
            Common::AsyncOperationSPtr const &, 
            __out SystemServiceLocation & primaryLocation,
            __out std::vector<std::wstring> & secondaryLocations);

        Common::ErrorCode EndResolve(
            Common::AsyncOperationSPtr const &,
            __out std::vector<Reliability::ServiceTableEntry> & serviceEntries,
            __out Naming::PartitionInfo & partitionInfo,
            __out Reliability::GenerationNumber & generation);

        void MarkLocationStale(std::wstring const & serviceName);

        void MarkLocationStale(Reliability::ConsistencyUnitId const &);

    private:
        SystemServiceResolver(
            __in Query::QueryGateway & query,
            __in Reliability::ServiceResolver & resolver,
            Common::ComponentRoot const &);

        class Impl;
        std::unique_ptr<SystemServiceResolver::Impl> implUPtr_;
    };
}
