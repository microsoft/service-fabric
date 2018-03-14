// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class HttpGatewayImpl::CloseAsyncOperation
        : public Common::AsyncOperation
    {
        
        DENY_COPY(CloseAsyncOperation)

    public:

        CloseAsyncOperation(
            HttpGatewayImpl &owner,
            Common::TimeSpan const &timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent)
            : Common::AsyncOperation(callback, parent)
            , owner_(owner)
            , timeoutHelper_(timeout)
        {
        }

        static Common::ErrorCode End(Common::AsyncOperationSPtr const& operation);

    protected:
        
        void OnStart(__in Common::AsyncOperationSPtr const& thisSPtr);

    private:
        void OnCloseComplete(Common::AsyncOperationSPtr const& thisSPtr, bool);

        Common::TimeoutHelper timeoutHelper_;
        HttpGatewayImpl & owner_;
    };
}
