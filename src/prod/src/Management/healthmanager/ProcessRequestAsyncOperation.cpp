// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace Transport;
using namespace Store;
using namespace std;

using namespace Management::HealthManager;

StringLiteral const TraceComponent("ProcessRequestAO");

ProcessRequestAsyncOperation::ProcessRequestAsyncOperation(
     __in EntityCacheManager & entityManager,
    Store::PartitionedReplicaId const & partitionedReplicaId,
    Common::ActivityId const & activityId,
    Transport::MessageUPtr && request,
    Federation::RequestReceiverContextUPtr && requestContext,
    Common::TimeSpan const timeout, 
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
    : TimedAsyncOperation(timeout, callback, parent)
    , ReplicaActivityTraceComponent(partitionedReplicaId, activityId)
    , entityManager_(entityManager)
    , passedThroughError_(ErrorCodeValue::Success)
    , request_(move(request))
    , requestContext_(move(requestContext))
{
}

ProcessRequestAsyncOperation::ProcessRequestAsyncOperation(
    __in EntityCacheManager & entityManager,
    Store::PartitionedReplicaId const & partitionedReplicaId,
    Common::ActivityId const & activityId,
    Common::ErrorCode && passedThroughError,
    Transport::MessageUPtr && request,
    Federation::RequestReceiverContextUPtr && requestContext,
    Common::TimeSpan const timeout, 
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
    : TimedAsyncOperation(timeout, callback, parent)
    , ReplicaActivityTraceComponent(partitionedReplicaId, activityId)
    , entityManager_(entityManager)
    , passedThroughError_(move(passedThroughError))
    , request_(move(request))
    , requestContext_(move(requestContext))
{
}

ProcessRequestAsyncOperation::~ProcessRequestAsyncOperation()
{
}

void ProcessRequestAsyncOperation::SetReply(Transport::MessageUPtr && reply)
{
    reply_ = std::move(reply);
}

void ProcessRequestAsyncOperation::SetReplyAndComplete(
    Common::AsyncOperationSPtr const & thisSPtr,
    Transport::MessageUPtr && reply,
    Common::ErrorCode && error)
{
    if (this->TryStartComplete())
    {
        reply_ = std::move(reply);
        this->FinishComplete(thisSPtr, move(error));
    }
}

void ProcessRequestAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    if (!passedThroughError_.IsSuccess())
    {
        this->TryComplete(thisSPtr, move(passedThroughError_));
        return;
    }

    // NOTE: In HandleRequest, derived classes should call TimedAsyncOperation::OnStart
    // when initial validation is done, before any processing is started.
    this->HandleRequest(thisSPtr);
}

void ProcessRequestAsyncOperation::HandleRequest(AsyncOperationSPtr const &)
{
    Assert::CodingError("{0}: ProcessRequestAsyncOperation::HandleRequest should not be called for base class. Instead OnStart should have completed with passThroughError", this->TraceId);
}

ErrorCode ProcessRequestAsyncOperation::End(
    AsyncOperationSPtr const & asyncOperation,
    __out MessageUPtr & reply,
    __out RequestReceiverContextUPtr & requestContext)
{
    auto casted = AsyncOperation::End<ProcessRequestAsyncOperation>(asyncOperation);
    auto error = casted->Error;

    if (error.IsSuccess())
    {
        if (casted->reply_)
        {
            reply = move(casted->reply_);
        }
        else
        {
            TRACE_LEVEL_AND_TESTASSERT(casted->WriteWarning, TraceComponent, "{0}: Reply not set on Success", casted->TraceId);
            reply = HealthManagerMessage::GetClientOperationSuccess(casted->ActivityId);
        }
    }
    else if (error.IsError(ErrorCodeValue::Timeout))
    {
        reply = move(casted->reply_);
        if (reply)
        {
            error = ErrorCode(ErrorCodeValue::Success);
        }
    }
    else
    {
        reply = nullptr;
    }

    // request context is used to reply with errors to client, so always return it
    requestContext = move(casted->requestContext_);

    return error;
}
