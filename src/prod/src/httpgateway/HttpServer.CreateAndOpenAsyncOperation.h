// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class HttpGatewayImpl::CreateAndOpenAsyncOperation
        : public Common::AsyncOperation
    {
        DENY_COPY(CreateAndOpenAsyncOperation)

    public:

        CreateAndOpenAsyncOperation(
            Common::FabricNodeConfigSPtr && config,
            Common::AsyncCallback const & callback)
            : AsyncOperation(callback, Common::AsyncOperationSPtr())
            , config_(std::move(config))
        {
        }

        static Common::ErrorCode End(
            __in Common::AsyncOperationSPtr const& operation,
            __out HttpGatewayImplSPtr & server);

    protected:

        void OnStart(__in Common::AsyncOperationSPtr const & thisSPtr);

    private:
        
        Common::FabricNodeConfigSPtr config_;
        HttpGatewayImplSPtr server_;
    };
}
