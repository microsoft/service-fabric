// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService::AdminRequestAsyncOperationBase : public EntreeService::RequestAsyncOperationBase
    {
    public:
        AdminRequestAsyncOperationBase(
            __in GatewayProperties & properties,
            Transport::MessageUPtr && receivedMessage,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

    protected:
        void OnRetry(Common::AsyncOperationSPtr const & thisSPtr);
    };
}
