// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class HttpGatewayImpl::AccessCheckAsyncOperation : public Common::AsyncOperation
    {
    public:
        AccessCheckAsyncOperation(
            HttpGatewayImpl& owner,
            HttpServer::IRequestMessageContext & request,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent)
            : AsyncOperation(callback, parent)
            , owner_(owner)
            , request_(request)
            , timeoutHelper_(timeout)
            , handlerItr_(owner.httpAuthHandlers_.cbegin())
            , authOperationError_(Common::ErrorCodeValue::NotFound)
            , role_(Transport::RoleMask::Enum::None)
        {
        }

        static Common::ErrorCode End(
            Common::AsyncOperationSPtr const& operation,
            __out USHORT & httpStatusOnError,
            __out std::wstring & headerName,
            __out std::wstring & headerValue,
            __out Transport::RoleMask::Enum &role);

    protected:
        void OnStart(Common::AsyncOperationSPtr const& thisSPtr);

    private:
        void StartOrContinueCheckAccess(Common::AsyncOperationSPtr const& operation);
        void OnCheckAccessComplete(Common::AsyncOperationSPtr const& operation, bool);

        HttpGatewayImpl &owner_;
        HttpServer::IRequestMessageContext & request_;
        Common::TimeoutHelper timeoutHelper_;
        USHORT httpStatusOnError_;
        std::wstring authHeaderValue_;
        std::wstring authHeaderName_;
        std::vector<HttpAuthHandlerSPtr>::const_iterator handlerItr_;
        Common::ErrorCode authOperationError_;
        Transport::RoleMask::Enum role_;
    };
}
