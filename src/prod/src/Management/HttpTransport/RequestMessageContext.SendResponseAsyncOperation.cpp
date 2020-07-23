// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace HttpServer;
using namespace HttpCommon;

void RequestMessageContext::SendResponseAsyncOperation::OnStart(AsyncOperationSPtr const& thisSPtr)
{
    USHORT httpStatus;
    wstring httpStatusLine;

    if (statusCode_ == 0)
    {
        HttpUtil::ErrorCodeToHttpStatus(operationStatus_, httpStatus, httpStatusLine);
    }
    else
    {
        httpStatus = statusCode_;
        httpStatusLine = statusDescription_;
    }

    if (memRef_._Size > 0)
    {
        wstring temp;
        StringWriter writer(temp);
        writer.Write("{0}", memRef_._Size);

        //
        // Set the content length header since we are not doing chunked transfers.
        //
        messageContext_.SetResponseHeader(*HttpConstants::ContentLengthHeader, temp);
    }

    auto asyncOperation = messageContext_.BeginSendResponseHeaders(
        httpStatus,
        httpStatusLine,
        (memRef_._Size > 0) ? true : false,
        [this](AsyncOperationSPtr const &operation)
        {
            this->OnSendResponseHeadersComplete(operation, false);
        },
        thisSPtr);

    OnSendResponseHeadersComplete(asyncOperation, true);
}

void RequestMessageContext::SendResponseAsyncOperation::OnSendResponseHeadersComplete(
    AsyncOperationSPtr const &operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = messageContext_.EndSendResponseHeaders(operation);
    if (!error.IsSuccess())
    {
        TryComplete(operation->Parent, error);
        return;
    }

    if (!(memRef_._Size > 0))
    {
        TryComplete(operation->Parent, ErrorCodeValue::Success);
        return;
    }

    auto sendBodyOperation = messageContext_.BeginSendResponseChunk(
        memRef_,
        true,
        false,
        [this](AsyncOperationSPtr const &operation)
        {
            this->OnSendBodyComplete(operation, false);
        },
        operation->Parent);

    OnSendBodyComplete(sendBodyOperation, true);
}

void RequestMessageContext::SendResponseAsyncOperation::OnSendBodyComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = messageContext_.EndSendResponseChunk(operation);
    TryComplete(operation->Parent, error);
}

void RequestMessageContext::SendResponseAsyncOperation::OnCancel()
{
    messageContext_.kHttpRequest_->Cancel();
}

ErrorCode RequestMessageContext::SendResponseAsyncOperation::End(__in Common::AsyncOperationSPtr const & asyncOperation)
{
    auto operation = AsyncOperation::End<SendResponseAsyncOperation>(asyncOperation);

    return operation->Error;
}
