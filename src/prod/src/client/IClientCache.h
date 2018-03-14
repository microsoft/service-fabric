// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    // Provides shared access to the PSD and RSP cache by notification
    // and other client operations like GetServiceDescription, 
    // Create/DeleteService, etc.
    //
    class IClientCache
    {
    public:
        virtual ~IClientCache() { }

        virtual void EnsureCacheCallback(
            Common::NamingUri const &, 
            LruClientCacheCallback::RspUpdateCallback const &) = 0;

        virtual void ReleaseCacheCallback(
            Common::NamingUri const &) = 0;

        virtual bool TryUpdateOrGetPsdCacheEntry(
            Common::NamingUri const &,
            Naming::PartitionedServiceDescriptor const &,
            Common::ActivityId const &,
            __out LruClientCacheEntrySPtr &) = 0;

        virtual Common::AsyncOperationSPtr BeginUpdateCacheEntries(
            std::multimap<Common::NamingUri, Naming::ResolvedServicePartitionSPtr> &&,
            Common::ActivityId const &,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndUpdateCacheEntries(
            Common::AsyncOperationSPtr const &,
            __out std::vector<Naming::ResolvedServicePartitionSPtr> &) = 0;

        virtual bool TryGetEntry(
            Common::NamingUri const &,
            Naming::PartitionKey const &,
            __out Naming::ResolvedServicePartitionSPtr &) = 0;

        virtual void GetAllEntriesForName(
            Common::NamingUri const &, 
            __out std::vector<Naming::ResolvedServicePartitionSPtr> &) = 0;

        virtual void ClearCacheEntriesWithName(
            Common::NamingUri const &,
            Common::ActivityId const &) = 0;

        virtual void UpdateCacheOnError(
            Common::NamingUri const &,
            Naming::PartitionKey const &,
            Common::ActivityId const &,
            Common::ErrorCode const) = 0;
    };
}
