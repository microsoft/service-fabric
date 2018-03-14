// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService::ResolveServiceAsyncOperation : public EntreeService::ServiceResolutionAsyncOperationBase
    {
    public:
        ResolveServiceAsyncOperation(
            __in GatewayProperties & properties,
            Transport::MessageUPtr && receivedMessage,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

    protected:

        void OnRetry(Common::AsyncOperationSPtr const & thisSPtr);

        // No-op: GetServiceDescriptionAsyncOperation is used to fetch PSDs
        //
        void OnStoreCommunicationFinished(Common::AsyncOperationSPtr const &, Transport::MessageUPtr &&) override { }

        Transport::MessageUPtr CreateMessageForStoreService() override { return Transport::MessageUPtr(); }

    private:
        void GetPsd(Common::AsyncOperationSPtr const &);
        void OnGetPsdComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);
    };
}
