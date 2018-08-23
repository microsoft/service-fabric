// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class LruClientCacheManager::GetPsdAsyncOperation : public CacheAsyncOperationBase
    {
        DENY_COPY(GetPsdAsyncOperation)

    public:
        GetPsdAsyncOperation(
            __in LruClientCacheManager &,
            Common::NamingUri const &,
            Transport::FabricActivityHeader const &,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        static Common::ErrorCode End(
            Common::AsyncOperationSPtr const &,
            __out Naming::PartitionedServiceDescriptor &);

        virtual void OnStart(Common::AsyncOperationSPtr const &) override;
        
    protected: // CacheAsyncOperationBase

        void OnProcessCachedPsd(
            LruClientCacheEntrySPtr const &,
            Common::AsyncOperationSPtr const &) override;

        void OnProcessRefreshedPsd(
            LruClientCacheEntrySPtr const &,
            Common::AsyncOperationSPtr const &) override;

        Naming::PartitionedServiceDescriptor psd_;
    };
}
