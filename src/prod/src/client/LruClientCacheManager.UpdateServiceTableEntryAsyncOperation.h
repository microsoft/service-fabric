// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    // Used by notification (v3.0) to update the cache. If the PSD
    // is missing or needs refreshing, it will be fetched from Naming
    // first.
    //
    class LruClientCacheManager::UpdateServiceTableEntryAsyncOperation 
        : public CacheAsyncOperationBase
    {
        DENY_COPY(UpdateServiceTableEntryAsyncOperation)

    public:
        UpdateServiceTableEntryAsyncOperation(
            __in LruClientCacheManager &,
            Common::NamingUri const &,
            Reliability::ServiceTableEntrySPtr const &,
            Reliability::GenerationNumber const &,
            Transport::FabricActivityHeader const &,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        static Common::ErrorCode End(
            Common::AsyncOperationSPtr const &,
            __out ServiceNotificationResultSPtr &);

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

        bool TryCreateRsp(
            LruClientCacheEntrySPtr const &);

        Reliability::ServiceTableEntrySPtr ste_;
        Reliability::GenerationNumber generation_;
        Naming::ResolvedServicePartitionSPtr rsp_;
    };
}

