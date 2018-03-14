// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService::ProcessServiceNotificationRequestAsyncOperation : public EntreeService::RequestAsyncOperationBase
    {
    public:
        ProcessServiceNotificationRequestAsyncOperation(
            __in GatewayProperties & properties,
            Transport::MessageUPtr && receivedMessage,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

    protected:
        void OnStartRequest(Common::AsyncOperationSPtr const & thisSPtr) override;

    private:
        void StartRequest(Common::AsyncOperationSPtr const & thisSPtr);
    };
}

