// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService::GetServiceDescriptionAsyncOperation : public EntreeService::NamingRequestAsyncOperationBase
    {
    public:
        GetServiceDescriptionAsyncOperation(
            __in GatewayProperties & properties,
            Transport::MessageUPtr && receivedMessage,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode End(
            Common::AsyncOperationSPtr const & asyncOperation, 
            __out PartitionedServiceDescriptorSPtr & psd);

        static Common::ErrorCode End(
            Common::AsyncOperationSPtr const & asyncOperation, 
            __out PartitionedServiceDescriptorSPtr & psd,
            __out __int64 & storeVersion);

        static Common::AsyncOperationSPtr BeginGetCached(
            __in GatewayProperties & properties,
            Common::NamingUri const & name,
            Transport::FabricActivityHeader const & activityHeader,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode EndGetCached(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out PartitionedServiceDescriptorSPtr & psd);

        static Common::ErrorCode EndGetCached(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out PartitionedServiceDescriptorSPtr & psd,
            __out __int64 & storeVersion);

    protected:

        virtual Transport::MessageUPtr CreateMessageForStoreService() override;
        virtual void OnRetry(Common::AsyncOperationSPtr const & thisSPtr) override;
        virtual void OnStartRequest(Common::AsyncOperationSPtr const & thisSPtr) override;
        virtual void OnCompleted() override;

    private:

        GetServiceDescriptionAsyncOperation(
            __in GatewayProperties & properties,
            Common::NamingUri const & name,
            Transport::FabricActivityHeader const & activityHeader,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

        GetServiceDescriptionAsyncOperation(
            __in GatewayProperties & properties,
            Common::NamingUri const & name,
            Transport::FabricActivityHeader const & activityHeader,
            bool expectsReplyMessage,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

        void StartGetServiceDescription(
            Common::AsyncOperationSPtr const & thisSPtr);

        void RefreshPsdCache(
            Common::AsyncOperationSPtr const & thisSPtr);
        void OnTryRefreshPsdComplete(
            Common::AsyncOperationSPtr const & operation,
            bool expectedCompletedSynchronously);

        void GetPsdFromCache(
            Common::AsyncOperationSPtr const & thisSPtr);
        void OnTryGetPsdComplete(
            Common::AsyncOperationSPtr const & operation,
            bool expectedCompletedSynchronously);

        void ProcessPsdCacheResult(
            Common::AsyncOperationSPtr const & thisSPtr,
            Common::ErrorCode const &,
            bool isFirstWaiter,
            GatewayPsdCacheEntrySPtr const &);

        void FetchPsdFromFM(
            Common::AsyncOperationSPtr const & thisSPtr);
        void OnRequestToFMComplete(
            Common::AsyncOperationSPtr const & operation,
            bool expectedCompletedSynchronously);

        void FetchPsdFromNaming(
            Common::AsyncOperationSPtr const & thisSPtr);
        void OnStoreCommunicationFinished(
            Common::AsyncOperationSPtr const & thisSPtr, 
            Transport::MessageUPtr && reply);

        void OnResolveNamingServiceRetryableError(
            Common::AsyncOperationSPtr const & thisSPtr,
            Common::ErrorCode const & error) override;
        void OnResolveNamingServiceNonRetryableError(
            Common::AsyncOperationSPtr const & thisSPtr,
            Common::ErrorCode const & error) override;
        void OnRouteToNodeFailedRetryableError(
            Common::AsyncOperationSPtr const & thisSPtr,
            Transport::MessageUPtr & reply,
            Common::ErrorCode const & error) override;
        void OnRouteToNodeFailedNonRetryableError(
            Common::AsyncOperationSPtr const & thisSPtr,
            Common::ErrorCode const & error) override;

        void SetReplyAndCompleteWithCurrentPsd(
            Common::AsyncOperationSPtr const & thisSPtr);

        void TryCompleteOnUserServiceNotFound(
            Common::AsyncOperationSPtr const & thisSPtr); 

        void TryCompleteAndFailCacheWaiters(
            Common::AsyncOperationSPtr const & thisSPtr, 
            Common::ErrorCode const & error);

        void TryCompleteAndCancelCacheWaiters(
            Common::AsyncOperationSPtr const & thisSPtr,
            Common::ErrorCode const & error);

        std::wstring cachedName_;
        bool isSystemService_;
        PartitionedServiceDescriptorSPtr psd_;
        __int64 storeVersion_;
        bool refreshCache_;
        bool expectsReplyMessage_;
    };
}
