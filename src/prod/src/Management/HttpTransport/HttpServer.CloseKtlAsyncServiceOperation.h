// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpServer
{
    class HttpServerImpl::CloseKtlAsyncServiceOperation
        : public HttpServerImpl::AsyncKtlServiceOperationBase
    {
        DENY_COPY(CloseKtlAsyncServiceOperation)

    public:
        CloseKtlAsyncServiceOperation(
            KHttpServer::SPtr &service,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent)
            : AsyncKtlServiceOperationBase(nullptr, callback, parent)
            , service_(service)
        {
        }

        static Common::ErrorCode End(__in Common::AsyncOperationSPtr const & asyncOperation);

        NTSTATUS StartKtlServiceOperation(
            __in KAsyncContextBase* const ParentAsync,
            __in KAsyncServiceBase::OpenCompletionCallback callbackPtr);

    private:
        KHttpServer::SPtr &service_;
    };
}
