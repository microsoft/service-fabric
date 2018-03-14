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

StringLiteral const TraceComponent("ProcessReport");

GetSequenceStreamProgressAsyncOperation::GetSequenceStreamProgressAsyncOperation(
     __in EntityCacheManager & entityManager,
    Store::PartitionedReplicaId const & partitionedReplicaId,
    Common::ActivityId const & activityId,
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
{
}

GetSequenceStreamProgressAsyncOperation::~GetSequenceStreamProgressAsyncOperation()
{
}

void GetSequenceStreamProgressAsyncOperation::HandleRequest(Common::AsyncOperationSPtr const & thisSPtr)
{
    if (this->Request.Action != HealthManagerTcpMessage::GetSequenceStreamProgressAction)
    {
        TRACE_LEVEL_AND_TESTASSERT(
            this->WriteInfo, 
            TraceComponent, 
            "{0}: GetSequenceStreamProgressAsyncOperation received action {1}, not supported",
            this->TraceId,
            this->Request.Action);

        TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::InvalidMessage));
        return;
    }

    GetSequenceStreamProgressMessageBody messageBody;
    if (!this->Request.GetBody(messageBody))
    {
        TRACE_LEVEL_AND_TESTASSERT(this->WriteInfo, TraceComponent, "{0}: Error getting GetSequenceStreamProgressMessageBody", this->TraceId);

        TryComplete(thisSPtr, ErrorCode(ErrorCodeValue::InvalidMessage));
        return;
    }

    sequenceStreamId_ = messageBody.MoveSequenceStreamId();
    instance_ = messageBody.Instance;

    TimedAsyncOperation::OnStart(thisSPtr);
    this->GetProgress(thisSPtr, false);
}

void GetSequenceStreamProgressAsyncOperation::GetProgress(AsyncOperationSPtr const & thisSPtr, bool isRetry)
{
    // Pass the request to entity manager and collect the result
    // to put into the reply 
    // If the cache encounters an error processing result,
    // return an error in the reply    
    SequenceStreamResult sequenceStreamResult(sequenceStreamId_.Kind, sequenceStreamId_.SourceId);
    auto error = this->EntityManager.GetSequenceStreamProgress(sequenceStreamId_, instance_, isRetry, /*out*/sequenceStreamResult);
    if (error.IsError(ErrorCodeValue::UpdatePending))
    {
        ASSERT_IF(isRetry,
            "GetSequenceStreamProgress returned UpdatePending on retry for {0} {1} instance {2}",
            sequenceStreamId_.Kind,
            sequenceStreamId_.SourceId,
            instance_);

        SequenceStreamInformation sequenceInformation(
            sequenceStreamId_.Kind,
            sequenceStreamId_.SourceId,
            instance_);

        this->EntityManager.AddSequenceStreamContext(
            SequenceStreamRequestContext(
                this->ReplicaActivityId,
                thisSPtr,
                *this,
                move(sequenceInformation)));
    }
    else
    {
        if (error.IsSuccess())
        {
            this->SetReplyAndComplete(
                thisSPtr,
                HealthManagerMessage::GetClientOperationSuccess(
                    GetSequenceStreamProgressReplyMessageBody(
                        move(sequenceStreamResult)),
                    this->ActivityId),
                move(error));
        }
        else
        {
            this->TryComplete(thisSPtr, move(error));
        }
    }
}

void GetSequenceStreamProgressAsyncOperation::OnSequenceStreamProcessed(
    AsyncOperationSPtr const & thisSPtr,
    SequenceStreamRequestContext const & context,
    ErrorCode const & error)
{
    this->EntityManager.OnContextCompleted(context);

    if (error.IsSuccess())
    {
        this->GetProgress(thisSPtr, true);
    }
    else
    {
        this->TryComplete(thisSPtr, error);
    }
}
