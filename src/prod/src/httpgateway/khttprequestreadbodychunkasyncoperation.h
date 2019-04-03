// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace HttpGateway
{
    ///This class reads HTTP request body in chunks according to the range passed by KMemRef object
    class KHttpRequestReadBodyChunkAsyncOperation
        : public Common::KtlProxyAsyncOperation
    {
        DENY_COPY(KHttpRequestReadBodyChunkAsyncOperation);

    public:

        KHttpRequestReadBodyChunkAsyncOperation(
            KHttpServerRequest::AsyncResponseDataContext::SPtr &asyncContext,
            KHttpServerRequest::SPtr const &request,
            KMemRef &memRef,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);

        static Common::ErrorCode End(
            __in Common::AsyncOperationSPtr const & asyncOperation,
            __in KMemRef &memRef);

        NTSTATUS StartKtlAsyncOperation(
            __in KAsyncContextBase * const parentAsyncContext,
            __in KAsyncContextBase::CompletionCallback callback);

    private:

        KMemRef memRef_;
        KHttpServerRequest::AsyncResponseDataContext::SPtr &asyncContext_;
        KHttpServerRequest::SPtr const &request_;
    };
}
