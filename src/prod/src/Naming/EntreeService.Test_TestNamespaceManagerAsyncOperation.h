// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService::Test_TestNamespaceManagerAsyncOperation : public EntreeService::RequestAsyncOperationBase
    {
    public:
        Test_TestNamespaceManagerAsyncOperation(
            __in GatewayProperties &,
            Transport::MessageUPtr &&,
            Common::TimeSpan timeout,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

    protected:

        bool AccessCheck();

        void OnStartRequest(Common::AsyncOperationSPtr const & thisSPtr);

    private:

        void OnResolveCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

        void SendRequest(
            Common::AsyncOperationSPtr const & thisSPtr, 
            SystemServices::SystemServiceLocation const & primaryLocation, 
            Transport::ISendTarget::SPtr const & primaryTarget);

        void OnRequestCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);
    };
}
