// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace HttpClient;

void ClientRequest::SendRequestAsyncOperation::OnStart(AsyncOperationSPtr const &thisSPtr)
{
    auto operation = clientRequest_->BeginSendRequestHeaders(
        memRef_._Param,
        [this](AsyncOperationSPtr const &operation)
        {
            this->OnSendHeadersComplete(operation, false);
        },
        thisSPtr);

    OnSendHeadersComplete(operation, true);
}

void ClientRequest::SendRequestAsyncOperation::OnSendHeadersComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = clientRequest_->EndSendRequestHeaders(operation, &winHttpError_);
    if (!error.IsSuccess())
    {
        TryComplete(operation->Parent, error);
        return;
    }

    bool noMoreData = true;

    auto sendBodyAsyncOperation = clientRequest_->BeginSendRequestChunk(
        memRef_,
        noMoreData,
        false,
        [this](AsyncOperationSPtr const &operation)
        {
            this->OnSendBodyComplete(operation, false);
        },
        operation->Parent);

    OnSendBodyComplete(sendBodyAsyncOperation, true);
}

void ClientRequest::SendRequestAsyncOperation::OnSendBodyComplete(
    AsyncOperationSPtr const &operation,
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    auto error = clientRequest_->EndSendRequestChunk(operation, &winHttpError_);

    TryComplete(operation->Parent, error);
}

ErrorCode ClientRequest::SendRequestAsyncOperation::End(AsyncOperationSPtr const &asyncOperation, ULONG *winHttpError)
{
    auto thisPtr = AsyncOperation::End<SendRequestAsyncOperation>(asyncOperation);

    if (!thisPtr->Error.IsSuccess())
    {
        *winHttpError = thisPtr->winHttpError_;
    }

    return thisPtr->Error;
}
