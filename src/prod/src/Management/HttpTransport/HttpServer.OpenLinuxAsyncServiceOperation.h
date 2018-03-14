// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpServer
{
    class HttpServerImpl::OpenLinuxAsyncServiceOperation
        : public HttpServerImpl::LinuxAsyncServiceBaseOperation
    {
        DENY_COPY(OpenLinuxAsyncServiceOperation)

    public:
        OpenLinuxAsyncServiceOperation(
            std::wstring listenUri,
            Transport::SecurityProvider::Enum credentialType,
            LHttpServer::RequestHandler &reqHandler,
            LHttpServer::SPtr &server,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent)
            : LinuxAsyncServiceBaseOperation(callback, parent)
            , listenUri_(listenUri)
            , reqHandler_(reqHandler)
            , server_(server)
            , credentialType_(credentialType)
        {
        }

        static Common::ErrorCode End(__in Common::AsyncOperationSPtr const & asyncOperation);

        bool StartLinuxServiceOperation();

    private:
        std::wstring listenUri_;
        LHttpServer::RequestHandler reqHandler_;
        LHttpServer::SPtr &server_;
        Transport::SecurityProvider::Enum credentialType_;
    };
}
