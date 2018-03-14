// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService::ForwardToFileStoreServiceAsyncOperation : public EntreeService::RequestAsyncOperationBase
    {
    public:
        ForwardToFileStoreServiceAsyncOperation(
            __in GatewayProperties & properties,
            Transport::MessageUPtr && receivedMessage,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

    protected:
        void OnRetry(Common::AsyncOperationSPtr const & thisSPtr);
        void OnStartRequest(Common::AsyncOperationSPtr const & thisSPtr) override;

    private:
        void StartResolve(Common::AsyncOperationSPtr const & thisSPtr);
        void OnResolveRequestComplete(
            Common::AsyncOperationSPtr const & asyncOperation,
            bool expectedCompletedSynchronously);

        bool IsRetryable(Common::ErrorCode const &);

        Common::NamingUri serviceName_;
        std::wstring partitionId_;
        Transport::FabricActivityHeader activityHeader_;
    };
}
