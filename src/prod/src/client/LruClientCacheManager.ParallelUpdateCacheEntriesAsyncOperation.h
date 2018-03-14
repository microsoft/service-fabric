// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    // Used by notification to update the client cache. A single notification
    // update may contain multiple RSPs, possibly for the same service.
    // If the PSD cache entry is missing for any given update, then it will
    // be fetched from Naming.
    //
    class LruClientCacheManager::ParallelUpdateCacheEntriesAsyncOperation 
        : public CacheAsyncOperationBase
    {
        DENY_COPY(ParallelUpdateCacheEntriesAsyncOperation)

    public:
        ParallelUpdateCacheEntriesAsyncOperation(
            __in LruClientCacheManager &,
            std::multimap<Common::NamingUri, Naming::ResolvedServicePartitionSPtr> &&,
            Common::ActivityId const &,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        static Common::ErrorCode End(
            Common::AsyncOperationSPtr const &,
            __out std::vector<Naming::ResolvedServicePartitionSPtr> &);

        void OnStart(Common::AsyncOperationSPtr const &);

    protected:

        virtual void OnProcessCachedPsd(
            LruClientCacheEntrySPtr const &,
            Common::AsyncOperationSPtr const &)
        {
            // Intentional no-op
        }

        virtual void OnProcessRefreshedPsd(
            LruClientCacheEntrySPtr const &,
            Common::AsyncOperationSPtr const &)
        {
            // Intentional no-op
        }

    private:

        void UpdateCacheEntries(Common::AsyncOperationSPtr const &);

        void OnUpdateCacheEntryComplete(
            Common::NamingUri const &,
            Naming::ResolvedServicePartitionSPtr const &,
            Common::AsyncOperationSPtr const &,
            bool expectedCompletedSynchronously);

        std::multimap<Common::NamingUri, Naming::ResolvedServicePartitionSPtr> rsps_;
        Common::atomic_long pendingCount_;
        std::vector<Naming::ResolvedServicePartitionSPtr> successRsps_;
        Common::ExclusiveLock successRspsLock_;
    };
}
