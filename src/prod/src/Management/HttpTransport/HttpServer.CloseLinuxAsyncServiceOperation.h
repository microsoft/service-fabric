// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpServer
{
    class HttpServerImpl::CloseLinuxAsyncServiceOperation
        : public HttpServerImpl::LinuxAsyncServiceBaseOperation
    {
        DENY_COPY(CloseLinuxAsyncServiceOperation)

    public:
        CloseLinuxAsyncServiceOperation(
            LHttpServer::SPtr &server,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent)
            : LinuxAsyncServiceBaseOperation(callback, parent)
            , server_(server)
        {
        }

        static Common::ErrorCode End(__in Common::AsyncOperationSPtr const & asyncOperation);

        bool StartLinuxServiceOperation();

    private:
        LHttpServer::SPtr &server_;
    };
}
