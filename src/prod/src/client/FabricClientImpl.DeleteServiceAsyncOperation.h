// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class FabricClientImpl::DeleteServiceAsyncOperation : public FabricClientImpl::ClientAsyncOperationBase
    {
    public:
        DeleteServiceAsyncOperation(
            __in FabricClientImpl &client,
            ServiceModel::DeleteServiceDescription const & description,
            __in Common::TimeSpan timeout,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent,
            __in_opt Common::ErrorCode && passThroughError = Common::ErrorCode::Success());

        static Common::ErrorCode End(
            Common::AsyncOperationSPtr const & asyncOperation);

    protected:
        void OnStartOperation(Common::AsyncOperationSPtr const & thisSPtr);

    private:
        void ForwardToClusterManager(
            Common::AsyncOperationSPtr const & thisSPtr,
            Common::NamingUri const& appName);

        void OnForwardToClusterManagerComplete(
            Common::AsyncOperationSPtr const & operation,
            bool expectedCompletedSynchronously);

        void ForwardToNaming(
            Common::AsyncOperationSPtr const & thisSPtr);

        void OnForwardToNamingComplete(
            Common::AsyncOperationSPtr const & operation,
            bool expectedCompletedSynchronously);

        void ForwardToFailoverManager(
            Common::AsyncOperationSPtr const & thisSPtr);

        void OnForwardToFailoverManagerComplete(
            Common::AsyncOperationSPtr const &operation,
            bool expectedCompletedSynchronously);

        void OnGetServiceDescriptionComplete(
            Common::AsyncOperationSPtr const & operation,
            bool expectedCompletedSynchronously);

        Transport::FabricActivityHeader activityHeader_;
        ServiceModel::DeleteServiceDescription description_;
    };
}
