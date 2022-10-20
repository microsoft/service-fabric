// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpServer
{
    class HttpServerImpl::OpenKtlAsyncServiceOperation
        : public HttpServerImpl::AsyncKtlServiceOperationBase
    {
        DENY_COPY(OpenKtlAsyncServiceOperation)

    public:
        OpenKtlAsyncServiceOperation(
            std::wstring listenUri,
            uint activeListeners,
            Transport::SecurityProvider::Enum credentialType,
            KHttpServer::RequestHandler &reqHandler,
            KHttpServer::RequestHeaderHandler &reqHeaderHandler,
            KHttpServer::SPtr &service,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent)
            : AsyncKtlServiceOperationBase(nullptr, callback, parent)
            , listenUri_(listenUri)
            , activeListeners_(activeListeners)
            , reqHandler_(reqHandler)
            , reqHeaderHandler_(reqHeaderHandler)
            , service_(service)
            , credentialType_(credentialType)
        {
        }

        static Common::ErrorCode End(__in Common::AsyncOperationSPtr const & asyncOperation);

        NTSTATUS StartKtlServiceOperation(
            __in KAsyncContextBase* const ParentAsync,
            __in KAsyncServiceBase::OpenCompletionCallback callbackPtr);

    private:
        std::wstring listenUri_;
        uint activeListeners_;
        KHttpServer::RequestHandler &reqHandler_;
        KHttpServer::RequestHeaderHandler &reqHeaderHandler_;
        KHttpServer::SPtr &service_;
        Transport::SecurityProvider::Enum credentialType_;
    };
}
