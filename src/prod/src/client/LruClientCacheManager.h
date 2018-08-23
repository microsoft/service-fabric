// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class LruClientCacheManager 
        : public IClientCache
        , public INotificationClientCache
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Client>
    {
        DENY_COPY(LruClientCacheManager)

    public:
        typedef Common::LruCache<Common::NamingUri, LruClientCacheEntry> LruCache;
        typedef Common::LruPrefixCache<Common::NamingUri, LruClientCacheEntry> LruPrefixCache;
        typedef std::unordered_map<
            Common::NamingUri, 
            LruClientCacheCallbackSPtr,
            Common::NamingUri::Hasher,
            Common::NamingUri::Hasher> LruCallbacks;

    public:
        LruClientCacheManager(
            __in FabricClientImpl &,
            __in FabricClientInternalSettings &);

        ~LruClientCacheManager();

        Common::AsyncOperationSPtr BeginGetPsd(
            Common::NamingUri const &,
            Transport::FabricActivityHeader const &,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndGetPsd(
            Common::AsyncOperationSPtr const &, 
            __out Naming::PartitionedServiceDescriptor &);
        
        Common::AsyncOperationSPtr BeginResolveService(
            Common::NamingUri const &,
            Naming::ServiceResolutionRequestData const &, 
            Naming::ResolvedServicePartitionMetadataSPtr const & previousResult,
            Transport::FabricActivityHeader &&,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndResolveService(
            Common::AsyncOperationSPtr const &, 
            __out Naming::ResolvedServicePartitionSPtr &,
            __out Common::ActivityId &);

        Common::AsyncOperationSPtr BeginPrefixResolveService(
            Common::NamingUri const &,
            Naming::ServiceResolutionRequestData const &, 
            Naming::ResolvedServicePartitionMetadataSPtr const & previousResult,
            bool bypassCache,
            Transport::FabricActivityHeader &&,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndPrefixResolveService(
            Common::AsyncOperationSPtr const &, 
            __out Naming::ResolvedServicePartitionSPtr &,
            __out Common::ActivityId &);

        LruClientCacheCallbackSPtr GetCacheCallback(Common::NamingUri const &);

        void ClearCacheCallbacks();

        static bool ErrorIndicatesInvalidService(Common::ErrorCode const &);

        static bool ErrorIndicatesInvalidPartition(Common::ErrorCode const &);

    public:

        //
        // IClientCache: Used primarily by service notifications to interact with cache
        //

        virtual void EnsureCacheCallback(
            Common::NamingUri const &, 
            LruClientCacheCallback::RspUpdateCallback const &);

        virtual void ReleaseCacheCallback(
            Common::NamingUri const &);

        virtual bool TryUpdateOrGetPsdCacheEntry(
            Common::NamingUri const &,
            Naming::PartitionedServiceDescriptor const &,
            Common::ActivityId const &,
            __out LruClientCacheEntrySPtr &);

        virtual Common::AsyncOperationSPtr BeginUpdateCacheEntries(
            std::multimap<Common::NamingUri, Naming::ResolvedServicePartitionSPtr> &&,
            Common::ActivityId const &,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        virtual Common::ErrorCode EndUpdateCacheEntries(
            Common::AsyncOperationSPtr const &,
            __out std::vector<Naming::ResolvedServicePartitionSPtr> &);

        virtual bool TryGetEntry(
            Common::NamingUri const &,
            Naming::PartitionKey const &,
            __out Naming::ResolvedServicePartitionSPtr &);

        virtual void GetAllEntriesForName(
            Common::NamingUri const &, 
            __out std::vector<Naming::ResolvedServicePartitionSPtr> &);

        virtual void ClearCacheEntriesWithName(
            Common::NamingUri const &,
            Common::ActivityId const &);

        virtual void UpdateCacheOnError(
            Common::NamingUri const &,
            Naming::PartitionKey const &,
            Common::ActivityId const &,
            Common::ErrorCode const);

    public:

        //
        // INotificationClientCache: Used by v3.0 notifications to interact with cache
        //

        virtual Common::AsyncOperationSPtr BeginUpdateCacheEntries(
            __in Naming::ServiceNotification &,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        virtual Common::ErrorCode EndUpdateCacheEntries(
            Common::AsyncOperationSPtr const &,
            __out std::vector<ServiceNotificationResultSPtr> &);

    private:
        class CacheAsyncOperationBase;
        class GetPsdAsyncOperation;
        class ResolveServiceAsyncOperation;
        class UpdateCacheEntryAsyncOperation;
        class ParallelUpdateCacheEntriesAsyncOperation;
        class UpdateServiceTableEntryAsyncOperation;
        class ProcessServiceNotificationAsyncOperation;
        class PrefixResolveServiceAsyncOperation;

        FabricClientImpl & client_;

        LruClientCacheManager::LruCache cache_;
        LruClientCacheManager::LruPrefixCache prefixCache_;

        // Callbacks must be maintained separate from cache entries in order to 
        // preserve registered callbacks across cache entry updates.
        //
        LruClientCacheManager::LruCallbacks callbacks_;
        Common::RwLock callbacksLock_;
    };

    typedef std::unique_ptr<LruClientCacheManager> LruClientCacheManagerUPtr;
}
