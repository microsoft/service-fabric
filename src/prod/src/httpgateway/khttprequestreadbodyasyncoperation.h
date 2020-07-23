// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    class KHttpRequestReadBodyAsyncOperation
        : public Common::KtlProxyAsyncOperation
    {
        DENY_COPY(KHttpRequestReadBodyAsyncOperation);

    public:

        KHttpRequestReadBodyAsyncOperation(
            KHttpServerRequest::AsyncResponseDataContext::SPtr &asyncContext,
            KHttpServerRequest::SPtr const &request,
            ULONG size,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);

        static Common::ErrorCode End(
            __in Common::AsyncOperationSPtr const & asyncOperation,
            __in KBuffer::SPtr &buffer);

        NTSTATUS StartKtlAsyncOperation(
            __in KAsyncContextBase * const parentAsyncContext,
            __in KAsyncContextBase::CompletionCallback callback);

        KBuffer::SPtr body_;
        ULONG size_;

    private:

        KHttpServerRequest::AsyncResponseDataContext::SPtr &asyncContext_;
        KHttpServerRequest::SPtr const &request_;
    };
}
