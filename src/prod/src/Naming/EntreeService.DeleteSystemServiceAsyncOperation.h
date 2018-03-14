// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService::DeleteSystemServiceAsyncOperation : public EntreeService::RequestAsyncOperationBase
    {
    public:
        DeleteSystemServiceAsyncOperation(
            __in GatewayProperties & properties,
            Transport::MessageUPtr && request,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

    protected:

        void OnRetry(Common::AsyncOperationSPtr const & thisSPtr);
        void OnStartRequest(Common::AsyncOperationSPtr const & thisSPtr);
        void OnCompleted();

    private:

        Common::ErrorCode InitializeServiceName();
        void DeleteSystemService(Common::AsyncOperationSPtr const &);
        void OnDeleteServiceCompleted(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        std::wstring svcName_;
    };
}
