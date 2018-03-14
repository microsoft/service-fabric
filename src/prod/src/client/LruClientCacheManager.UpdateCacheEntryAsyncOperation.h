// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    // Used by notification to update the cache. If the PSD
    // is missing or needs refreshing, it will be fetched from Naming
    // first.
    //
    class LruClientCacheManager::UpdateCacheEntryAsyncOperation 
        : public CacheAsyncOperationBase
    {
        DENY_COPY(UpdateCacheEntryAsyncOperation)

    public:
        UpdateCacheEntryAsyncOperation(
            __in LruClientCacheManager &,
            Common::NamingUri const &,
            Naming::ResolvedServicePartitionSPtr const &,
            Transport::FabricActivityHeader const &,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        static Common::ErrorCode End(
            Common::AsyncOperationSPtr const &);

        void OnStart(Common::AsyncOperationSPtr const &);

    protected:

        virtual void OnProcessCachedPsd(
            LruClientCacheEntrySPtr const &,
            Common::AsyncOperationSPtr const &);

        virtual void OnProcessRefreshedPsd(
            LruClientCacheEntrySPtr const &,
            Common::AsyncOperationSPtr const &);

    private:

        void ProcessRsp(
            LruClientCacheEntrySPtr const &,
            Common::AsyncOperationSPtr const &);

        Naming::ResolvedServicePartitionSPtr rsp_;
    };
}
