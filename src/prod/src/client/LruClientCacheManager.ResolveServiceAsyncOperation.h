// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class LruClientCacheManager::ResolveServiceAsyncOperation 
        : public CacheAsyncOperationBase
    {
        DENY_COPY(ResolveServiceAsyncOperation)

    public:
        ResolveServiceAsyncOperation(
            __in LruClientCacheManager &,
            Common::NamingUri const &,
            Naming::ServiceResolutionRequestData const &,
            Naming::ResolvedServicePartitionMetadataSPtr const & previousResult,
            Transport::FabricActivityHeader &&,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        static Common::ErrorCode End(
            Common::AsyncOperationSPtr const &,
            __out Naming::ResolvedServicePartitionSPtr &,
            __out Common::ActivityId &);

        virtual void OnStart(Common::AsyncOperationSPtr const &) override;
        
    protected:

        __declspec(property(get=get_RequestData)) Naming::ServiceResolutionRequestData & RequestData;
        inline Naming::ServiceResolutionRequestData & get_RequestData() { return request_; }

        void InitializeRequestVersion();

        Naming::ResolvedServicePartitionSPtr && TakeRspResult();

        void SendRequest(
            LruClientCacheEntrySPtr const &,
            Client::ClientServerRequestMessageUPtr &&,
            Common::AsyncOperationSPtr const &);

        virtual void ProcessReply(
            __in Common::ErrorCode &,
            Client::ClientServerReplyMessageUPtr &&,
            LruClientCacheEntrySPtr const &,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode ProcessRspReply(
            Naming::ResolvedServicePartition &&,
            LruClientCacheEntrySPtr const &,
            Common::AsyncOperationSPtr const &);

        void OnRspErrorAndComplete(
            LruClientCacheEntrySPtr const &,
            Common::ErrorCode const &,
            Common::AsyncOperationSPtr const &);

        void OnRspError(
            LruClientCacheEntrySPtr const &,
            Common::ErrorCode const &,
            Common::AsyncOperationSPtr const &);

    protected: // CacheAsyncOperationBase

        void OnProcessCachedPsd(
            LruClientCacheEntrySPtr const &,
            Common::AsyncOperationSPtr const &) override;

        void OnProcessRefreshedPsd(
            LruClientCacheEntrySPtr const &,
            Common::AsyncOperationSPtr const &) override;

    private:

        void GetRsp(
            LruClientCacheEntrySPtr const &,
            Common::AsyncOperationSPtr const &);

        void OnGetCachedRspComplete(
            LruClientCacheEntrySPtr const &,
            Common::AsyncOperationSPtr const &,
            bool expectedCompletedSynchronously);

        void OnRspCacheMiss(
            LruClientCacheEntrySPtr const &,
            Common::AsyncOperationSPtr const &);

        void OnRequestComplete(
            LruClientCacheEntrySPtr const &,
            Common::AsyncOperationSPtr const &,
            bool expectedCompletedSynchronously);

        void ProcessCachedRsp(
            LruClientCacheEntrySPtr const &,
            Naming::ResolvedServicePartitionSPtr const &,
            Common::AsyncOperationSPtr const &);

        void InvalidateCachedRsp(
            LruClientCacheEntrySPtr const &,
            Naming::ResolvedServicePartitionSPtr const &,
            Common::AsyncOperationSPtr const &);

        void OnInvalidateCachedRspComplete(
            LruClientCacheEntrySPtr const &,
            Common::AsyncOperationSPtr const &,
            bool expectedCompletedSynchronously);
    
        void OnSuccess(
            Naming::ResolvedServicePartitionSPtr const &,
            Common::AsyncOperationSPtr const &);

        Naming::ServiceResolutionRequestData request_;
        Naming::ResolvedServicePartitionMetadataSPtr previousRspMetadata_;
        Naming::ResolvedServicePartitionSPtr currentRsp_;
    };
}
