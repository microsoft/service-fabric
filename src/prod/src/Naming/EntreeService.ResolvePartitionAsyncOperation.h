// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService::ResolvePartitionAsyncOperation : public EntreeService::AdminRequestAsyncOperationBase
    {
    public:
        ResolvePartitionAsyncOperation(
            __in GatewayProperties & properties,
            Transport::MessageUPtr && receivedMessage,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

    protected:
        void OnRetry(Common::AsyncOperationSPtr const & thisSPtr);
        void OnStartRequest(Common::AsyncOperationSPtr const & thisSPtr) override;

    private:
        void StartResolve(Common::AsyncOperationSPtr const & thisSPtr);
        void OnRequestComplete(
            Common::AsyncOperationSPtr const & asyncOperation,
            bool expectedCompletedSynchronously);
        void OnFMResolved(
            Common::AsyncOperationSPtr const & asyncOperation,
            bool expectedCompletedSynchronously);
        void CompleteOrRetry(Common::AsyncOperationSPtr const & thisSPtr, Common::ErrorCode const &);
    };
}
