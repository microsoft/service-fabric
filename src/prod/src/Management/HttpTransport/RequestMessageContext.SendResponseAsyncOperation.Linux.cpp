// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "cpprest/json.h"
#include "cpprest/http_listener.h"
#include "cpprest/uri.h"
#include "cpprest/asyncrt_utils.h"

using namespace std;
using namespace Common;
using namespace HttpServer;
using namespace HttpCommon;

StringLiteral const TraceType("HttpRequestSendResponse");

RequestMessageContext::SendResponseAsyncOperation::SendResponseAsyncOperation(
    Common::ErrorCode operationStatus,
    Common::ByteBufferUPtr bodyUPtr,
    RequestMessageContext & messageContext,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
    : Common::AsyncOperation(callback, parent)
    , messageContext_(messageContext)
{
    if (bodyUPtr)
    {
        messageContext_.responseUPtr_->set_body(move(*bodyUPtr));
    }

    status_code httpStatus;
    wstring httpStatusLine;
    HttpCommon::HttpUtil::ErrorCodeToHttpStatus(operationStatus.ReadValue(), httpStatus, httpStatusLine);

    string reasonPhrase;
    StringUtility::Utf16ToUtf8(httpStatusLine, reasonPhrase);
    messageContext_.responseUPtr_->set_status_code(httpStatus);
    messageContext_.responseUPtr_->set_reason_phrase(reasonPhrase);
}

RequestMessageContext::SendResponseAsyncOperation::SendResponseAsyncOperation(
    USHORT statusCode,
    std::wstring const& statusDescription,
    Common::ByteBufferUPtr bodyUPtr,
    RequestMessageContext & messageContext,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
    : Common::AsyncOperation(callback, parent)
    , messageContext_(messageContext)
{
    if (bodyUPtr)
    {
        messageContext_.responseUPtr_->set_body(move(*bodyUPtr));
    }

    string reasonPhrase;
    StringUtility::Utf16ToUtf8(statusDescription, reasonPhrase);
    messageContext_.responseUPtr_->set_status_code(statusCode);
    messageContext_.responseUPtr_->set_reason_phrase(reasonPhrase);
}

void RequestMessageContext::SendResponseAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{
    messageContext_.requestUPtr_->reply(*messageContext_.responseUPtr_).then([this, thisSPtr](pplx::task<void> t)
    {
        try
        {
            t.get();
        }
        catch (...)
        {
            auto eptr = std::current_exception();
            RequestMessageContext::HandleException(eptr, messageContext_.GetClientRequestId());
            TryComplete(thisSPtr, ErrorCodeValue::OperationFailed);
            return;
        }

        TryComplete(thisSPtr, ErrorCodeValue::Success);
        return;
    });
}

void RequestMessageContext::SendResponseAsyncOperation::OnCancel()
{
    // TODO: Add cancellation support
    WriteInfo(TraceType, "Cancellation requested for http_request. ClientRequestId: {0}", messageContext_.GetClientRequestId());
}

ErrorCode RequestMessageContext::SendResponseAsyncOperation::End(__in Common::AsyncOperationSPtr const& thisSPtr)
{
    auto operation = AsyncOperation::End<SendResponseAsyncOperation>(thisSPtr);
    return operation->Error;
}
