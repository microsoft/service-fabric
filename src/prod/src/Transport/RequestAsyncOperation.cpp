// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;
using namespace std;

RequestAsyncOperation::RequestAsyncOperation(
    RequestTable & owner,
    Transport::MessageId const & messageId,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : TimedAsyncOperation(timeout, callback, parent),
    owner_(owner),
    messageId_(messageId),
    reply_()
{
}

void RequestAsyncOperation::SetReply(MessageUPtr && reply)
{
    reply_ = move(reply);
}

Common::ErrorCode RequestAsyncOperation::End(AsyncOperationSPtr const & operationSPtr, MessageUPtr & reply)
{
    auto thisPtr = AsyncOperation::End<RequestAsyncOperation>(operationSPtr);

    reply = std::move(thisPtr->reply_);

    return thisPtr->Error;
}

void RequestAsyncOperation::OnTimeout(AsyncOperationSPtr const & operationSPtr)
{
    RequestAsyncOperationSPtr operation;
    this->owner_.TryRemoveEntry(this->MessageId, operation);

    TimedAsyncOperation::OnTimeout(operationSPtr);
}

void RequestAsyncOperation::OnCancel()
{
    RequestAsyncOperationSPtr operation;
    this->owner_.TryRemoveEntry(this->MessageId, operation);
}
