// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService::PropertyBatchAsyncOperation : public EntreeService::NamingRequestAsyncOperationBase
    {
    public:
        PropertyBatchAsyncOperation(
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

        Transport::MessageUPtr CreateMessageForStoreService();
        
        NamePropertyOperationBatch batchRequest_;
        NamePropertyOperationBatchResult batchResult_;
    };
}
