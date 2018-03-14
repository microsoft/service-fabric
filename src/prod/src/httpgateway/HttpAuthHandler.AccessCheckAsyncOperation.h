// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class HttpAuthHandler::AccessCheckAsyncOperation : public Common::TimedAsyncOperation
    {
    public:
        AccessCheckAsyncOperation(
            HttpAuthHandler& owner,
            HttpServer::IRequestMessageContext & request,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent)
            : owner_(owner)
            , request_(request)
            , TimedAsyncOperation(timeout, callback, parent)
        {
        }

        static Common::ErrorCode End(
            Common::AsyncOperationSPtr const& operation,
            __out USHORT & httpStatusOnError,
            __out std::wstring & headerName,
            __out std::wstring & headerValue,
            __out Transport::RoleMask::Enum &roleOnSuccess);

        HttpServer::IRequestMessageContext & request_;
        USHORT httpStatusOnError_;
        std::wstring authHeaderValue_;
        std::wstring authHeaderName_;

    protected:
        void OnStart(Common::AsyncOperationSPtr const& thisSPtr);

    private:
        HttpAuthHandler &owner_;
    };
}
