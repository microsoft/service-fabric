// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService::CreateSystemServiceAsyncOperation : public EntreeService::RequestAsyncOperationBase
    {
    public:
        CreateSystemServiceAsyncOperation(
            __in GatewayProperties & properties,
            Transport::MessageUPtr && request,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

    protected:

        void OnRetry(Common::AsyncOperationSPtr const & thisSPtr);
        void OnStartRequest(Common::AsyncOperationSPtr const & thisSPtr);

    private:

        Common::ErrorCode InitializeServiceDescription();
        void CreateSystemService(Common::AsyncOperationSPtr const &);
        void OnCreateServiceCompleted(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        Reliability::ServiceDescription svcDescription_;
        Reliability::ConsistencyUnitDescription cuid_;
    };
}
