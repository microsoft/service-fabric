// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    using namespace Naming;

    class FabricClientImpl::GetServiceDescriptionAsyncOperation : public FabricClientImpl::ClientAsyncOperationBase
    {
    public:
        GetServiceDescriptionAsyncOperation(
            __in FabricClientImpl &client,
            Common::NamingUri const &associatedName,
            bool fetchCached,
            __in Transport::FabricActivityHeader && activityId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent,
            __in_opt Common::ErrorCode && passThroughError = Common::ErrorCode::Success());

        static Common::ErrorCode EndGetServiceGroupDescription(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out Naming::ServiceGroupDescriptor &result);

        static Common::ErrorCode EndGetServiceDescription(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out Naming::PartitionedServiceDescriptor &result);

    protected:
        void OnStartOperation(Common::AsyncOperationSPtr const & thisSPtr);

    private:

        void FetchFromCache(Common::AsyncOperationSPtr const & thisSPtr);
        void OnFetchFromCacheComplete(Common::AsyncOperationSPtr const& thisSPtr, bool expectedCompletedSynchronously);

        void FetchFromNamingService(Common::AsyncOperationSPtr const & thisSPtr);
        void OnFetchFromNamingServiceComplete(Common::AsyncOperationSPtr const& thisSPtr, bool expectedCompletedSynchronously);

        Common::NamingUri associatedName_;
        bool fetchCached_;
        Naming::PartitionedServiceDescriptor description_;
    };
}
