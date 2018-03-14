// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class LruClientCacheManager::PrefixResolveServiceAsyncOperation 
        : public ResolveServiceAsyncOperation
    {
        DENY_COPY(PrefixResolveServiceAsyncOperation)

    public:
        PrefixResolveServiceAsyncOperation(
            __in LruClientCacheManager &,
            Common::NamingUri const &,
            Naming::ServiceResolutionRequestData const &,
            Naming::ResolvedServicePartitionMetadataSPtr const & previousResult,
            bool bypassCache,
            Transport::FabricActivityHeader &&,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        static Common::ErrorCode End(
            Common::AsyncOperationSPtr const &,
            __out Naming::ResolvedServicePartitionSPtr &,
            __out Common::ActivityId &);

        void OnStart(Common::AsyncOperationSPtr const &) override;

    protected:

        void GetPrefixCachedPsd(Common::AsyncOperationSPtr const &);

        Common::ErrorCode EndGetCachedPsd(
            Common::AsyncOperationSPtr const &,
            __out bool & isFirstWaiter,
            __out LruClientCacheEntrySPtr &) override;

        // The workflow on PSD cache miss is different from exact resolution,
        // but the workflow on PSD cache hit is the same since prefix resolution
        // only affects the target service being resolved.
        //
        void OnPsdCacheMiss(Common::AsyncOperationSPtr const &) override;

        void ProcessReply(
            __in Common::ErrorCode &,
            Client::ClientServerReplyMessageUPtr &&,
            LruClientCacheEntrySPtr const &,
            Common::AsyncOperationSPtr const &) override;

        // There are different waiter lists for the prefix and inner cache
        //
        void InvalidateCachedPsd(
            LruClientCacheEntrySPtr const &,
            Common::AsyncOperationSPtr const &) override;

    private:

        void OnError(
            LruClientCacheEntrySPtr const & cacheEntry,
            Common::ErrorCode const &,
            Common::AsyncOperationSPtr const &);

        LruClientCacheManager::LruPrefixCache & prefixCache_;
        bool bypassCache_;
    };
}
