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
using namespace ClientServerTransport;
using namespace Management::HealthManager;

StringLiteral const TraceComponent("ProcessQuery");

ProcessQueryRequestAsyncOperation::ProcessQueryRequestAsyncOperation(
     __in EntityCacheManager & entityManager,
    Store::PartitionedReplicaId const & partitionedReplicaId,
    Common::ActivityId const & activityId,
    __in Query::QueryMessageHandlerUPtr & queryMessageHandler,
    Transport::MessageUPtr && request,
    Federation::RequestReceiverContextUPtr && requestContext,
    Common::TimeSpan const timeout, 
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & parent)
    : ProcessRequestAsyncOperation(
        entityManager, 
        partitionedReplicaId, 
        activityId, 
        move(request), 
        move(requestContext), 
        timeout, 
        callback, 
        parent)
    , queryMessageHandler_(queryMessageHandler)
{
}

ProcessQueryRequestAsyncOperation::~ProcessQueryRequestAsyncOperation()
{
}

void ProcessQueryRequestAsyncOperation::HandleRequest(Common::AsyncOperationSPtr const & thisSPtr)
{
    if (this->Request.Action != QueryTcpMessage::QueryAction)   
    {
        TRACE_LEVEL_AND_TESTASSERT(
            this->WriteInfo,
            TraceComponent,
            "{0}: ProcessQueryRequestAsyncOperation received action {1}, not supported",
            this->TraceId,
            this->Request.Action);

        TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::InvalidMessage));
        return;
    }

    TimedAsyncOperation::OnStart(thisSPtr);

    auto inner = queryMessageHandler_->BeginProcessQueryMessage(
        this->Request, 
        this->RemainingTime, 
        [this](AsyncOperationSPtr const & operation) { this->OnProcessQueryMessageComplete(operation, false /*expectedCompletedSynchronously*/); },
        thisSPtr);
    this->OnProcessQueryMessageComplete(inner, true /*expectedCompletedSynchronously*/); 
}

void ProcessQueryRequestAsyncOperation::OnProcessQueryMessageComplete(
    AsyncOperationSPtr const & operation, 
    bool expectedCompletedSynchronously)
{
    if (operation->CompletedSynchronously != expectedCompletedSynchronously)
    {
        return;
    }

    AsyncOperationSPtr const & thisSPtr = operation->Parent;

    MessageUPtr reply;
    ErrorCode error = queryMessageHandler_->EndProcessQueryMessage(operation, reply);
    
    if (error.IsSuccess())
    {
        if (!reply)
        {
            TRACE_LEVEL_AND_TESTASSERT(this->WriteInfo, TraceComponent, "{0}: Reply not set on Success", this->TraceId);
        }
        else if (reply->Action.empty())
        {
            TRACE_LEVEL_AND_TESTASSERT(
                this->WriteInfo, TraceComponent,
                "{0}: OnProcessQueryMessageComplete: Reply action is empty for {1}",
                this->TraceId,
                reply->MessageId);
        }

        this->SetReplyAndComplete(thisSPtr, move(reply), move(error));
        return;
    }
   
    this->TryComplete(thisSPtr, move(error));
}
