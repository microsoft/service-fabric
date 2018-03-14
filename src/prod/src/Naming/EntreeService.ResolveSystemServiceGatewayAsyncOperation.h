// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService::ResolveSystemServiceGatewayAsyncOperation : public EntreeService::RequestAsyncOperationBase
    {
    public:
        ResolveSystemServiceGatewayAsyncOperation(
            __in GatewayProperties &,
            Transport::MessageUPtr &&,
            Common::TimeSpan,
            Common::AsyncCallback const &, 
            Common::AsyncOperationSPtr const &);

    protected:
        void OnStartRequest(Common::AsyncOperationSPtr const &) override;
        void OnRetry(Common::AsyncOperationSPtr const &) override;

    private:

        void Resolve(Common::AsyncOperationSPtr const &);
        void OnResolveCompleted(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        std::wstring targetServiceName_;
    };
}
