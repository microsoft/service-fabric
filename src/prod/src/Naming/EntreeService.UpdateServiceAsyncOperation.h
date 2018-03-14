// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService::UpdateServiceAsyncOperation : public EntreeService::NamingRequestAsyncOperationBase
    {
    public:
        UpdateServiceAsyncOperation(
            __in GatewayProperties & properties,
            Transport::MessageUPtr && receivedMessage,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

    protected:
        void OnStartRequest(Common::AsyncOperationSPtr const & thisSPtr) override;

        void OnRetry(Common::AsyncOperationSPtr const & thisSPtr);

    private:
        void OnStoreCommunicationFinished(Common::AsyncOperationSPtr const & thisSPtr, Transport::MessageUPtr && reply);

        void OnRequestCompleted(
            Common::AsyncOperationSPtr const & asyncOperation,
            bool expectedCompletedSynchronously);

        void DoUpdate(Common::AsyncOperationSPtr const & asyncOperation);

        Transport::MessageUPtr CreateMessageForStoreService();

        UpdateServiceRequestBody requestBody_;
        bool systemServiceUpdate_;
    };
}

