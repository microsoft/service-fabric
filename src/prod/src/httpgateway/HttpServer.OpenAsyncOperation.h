// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class HttpGatewayImpl::OpenAsyncOperation
        : public Common::AsyncOperation
    {
        DENY_COPY(OpenAsyncOperation)

    public:
        OpenAsyncOperation(
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
        
        void OnStart(Common::AsyncOperationSPtr const& thisSPtr);

    private:
        void OnOpenComplete(Common::AsyncOperationSPtr const& thisSPtr, bool);

#if !defined(PLATFORM_UNIX)
        void OnServiceResolverOpenComplete(Common::AsyncOperationSPtr const& thisSPtr, bool);
#endif

        HttpGatewayImpl & owner_;
        Common::TimeoutHelper timeoutHelper_;
    };
}
