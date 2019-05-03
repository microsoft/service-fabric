// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace HttpGateway;


void RequestMessageContext::SendResponseAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{
    KHttpUtils::HttpResponseCode httpStatus;
    string httpStatusLine;

    if (statusCode_ == 0)
    {
        messageContext_.ErrorCodeToHttpStatus(operationStatus_, httpStatus, httpStatusLine);
    }
    else
    {
        httpStatus = (KHttpUtils::HttpResponseCode) statusCode_;
        StringUtility::UnicodeToAnsi(statusDescription_, httpStatusLine);
    }

    if (bodyUPtr_)
    {
        KMemRef memRef(static_cast<ULONG>(bodyUPtr_->size()), bodyUPtr_->data(), static_cast<ULONG>(bodyUPtr_->size()));
        NTSTATUS status = messageContext_.kHttpRequest_->SetResponseContent(memRef);
        if (!NT_SUCCESS(status))
        {
            TryComplete(thisSPtr, ErrorCode::FromNtStatus(status));
            return;
        }
    }

    //
    // KHttpServerRequest interface hides the actual async when sending response. The callback we register is not the
    // actual async completion callback, but we are safe to release our self reference in the callback.
    //
    thisSPtr_ = shared_from_this();

    messageContext_.kHttpRequest_->SendResponse(
        httpStatus,
        const_cast<char*>(httpStatusLine.c_str()),
        KHttpServerRequest::ResponseCompletion(this, &RequestMessageContext::SendResponseAsyncOperation::OnResponseCompletion));
}

void RequestMessageContext::SendResponseAsyncOperation::OnCancel()
{
    messageContext_.kHttpRequest_->Cancel();
}

void RequestMessageContext::SendResponseAsyncOperation::OnResponseCompletion(
    __in KHttpServerRequest::SPtr request,
    __in NTSTATUS status)
{
    TryComplete(shared_from_this(), ErrorCode::FromNtStatus(status));
    thisSPtr_ = nullptr;
}

ErrorCode RequestMessageContext::SendResponseAsyncOperation::End(__in Common::AsyncOperationSPtr const & asyncOperation)
{
    auto operation = AsyncOperation::End<SendResponseAsyncOperation>(asyncOperation);

    return operation->Error;
}
