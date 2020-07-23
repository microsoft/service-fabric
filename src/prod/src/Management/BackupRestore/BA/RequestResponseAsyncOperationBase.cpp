// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

using namespace Common;
using namespace Transport;
using namespace Management;
using namespace BackupRestoreAgentComponent;

StringLiteral const TraceComponent("RequestResponseAsyncOperationBase");

RequestResponseAsyncOperationBase::RequestResponseAsyncOperationBase(
    MessageUPtr && message,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : TimedAsyncOperation(timeout, callback, parent)
    , receivedMessage_(std::move(message))
{
    activityId_ = FabricActivityHeader::FromMessage(*receivedMessage_).ActivityId;
}

RequestResponseAsyncOperationBase::~RequestResponseAsyncOperationBase()
{
}

void RequestResponseAsyncOperationBase::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    TimedAsyncOperation::OnStart(thisSPtr);
}

void RequestResponseAsyncOperationBase::OnCompleted()
{
    TimedAsyncOperation::OnCompleted();
}

void RequestResponseAsyncOperationBase::SetReplyAndComplete(
    AsyncOperationSPtr const & thisSPtr,
    Transport::MessageUPtr && reply,
    Common::ErrorCode const & error)
{
    if (this->TryStartComplete())
    {
        reply_ = move(reply);
        this->FinishComplete(thisSPtr, error);
    }
}

ErrorCode RequestResponseAsyncOperationBase::End(
    Common::AsyncOperationSPtr const & asyncOperation,
    __out Transport::MessageUPtr & reply)
{
    auto casted = AsyncOperation::End<RequestResponseAsyncOperationBase>(asyncOperation);
    auto error = casted->Error;

    if (error.IsSuccess())
    {
        reply = std::move(casted->reply_);
    }

    return error;
}
