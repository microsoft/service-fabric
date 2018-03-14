// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService::PrefixResolveServiceAsyncOperation : public EntreeService::ServiceResolutionAsyncOperationBase
    {
    public:
        PrefixResolveServiceAsyncOperation(
            __in GatewayProperties & properties,
            Transport::MessageUPtr && receivedMessage,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

    protected:

        void OnTimeout(Common::AsyncOperationSPtr const & thisSPtr) override;

        void OnRetry(Common::AsyncOperationSPtr const & thisSPtr) override;

        void GetPsd(Common::AsyncOperationSPtr const &) override;

        void FetchPsdFromNaming(Common::AsyncOperationSPtr const &);

        void OnStoreCommunicationFinished(
            Common::AsyncOperationSPtr const & thisSPtr, 
            Transport::MessageUPtr && reply) override;

        Transport::MessageUPtr CreateMessageForStoreService() override;

    protected:

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

    private:

        void OnGetPsdComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void TryCompleteAndFailCacheWaiters(
            Common::AsyncOperationSPtr const &, 
            Common::ErrorCode const &);

        void TryCompleteAndCancelCacheWaiters(
            Common::AsyncOperationSPtr const & thisSPtr,
            Common::ErrorCode const & error);
    };
}

