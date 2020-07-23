// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Store;
using namespace ServiceModel;

using namespace Management::ResourceManager;
using namespace Management::CentralSecretService;

ClientRequestAsyncOperation::ClientRequestAsyncOperation(
    Management::CentralSecretService::SecretManager & secretManager,
    IpcResourceManagerService & resourceManager,
    MessageUPtr requestMsg,
    IpcReceiverContextUPtr receiverContext,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : TimedAsyncOperation(timeout, callback, parent)
    , ReplicaActivityTraceComponent<TraceTaskCodes::CentralSecretService>(
        secretManager.PartitionedReplicaId,
        FabricActivityHeader::FromMessage(*requestMsg).ActivityId)
    , secretManager_(secretManager)
    , resourceManager_(resourceManager)
    , requestMsgUPtr_(move(requestMsg))
    , receiverContextUPtr_(move(receiverContext))
    , replyMsgUPtr_(nullptr)
{
}

ErrorCode ClientRequestAsyncOperation::End(
    AsyncOperationSPtr const & asyncOperation,
    __out MessageUPtr & replyMsgUPtr,
    __out IpcReceiverContextUPtr & receiverContextUPtr)
{
    auto clientRequestOperationSPtr = AsyncOperation::End<ClientRequestAsyncOperation>(asyncOperation);

    replyMsgUPtr = move(clientRequestOperationSPtr->replyMsgUPtr_);
    receiverContextUPtr = move(clientRequestOperationSPtr->receiverContextUPtr_);

    return clientRequestOperationSPtr->Error;
}

void ClientRequestAsyncOperation::OnTimeout(AsyncOperationSPtr const & operationSPtr)
{
    WriteWarning(
        this->TraceComponent,
        "{0}: {1} -> Timeout.",
        this->TraceId,
        this->TraceComponent);

    TimedAsyncOperation::OnTimeout(operationSPtr);
}

void ClientRequestAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    WriteInfo(
        this->TraceComponent,
        "{0}: {1} -> Begins.",
        this->TraceId,
        this->TraceComponent);

    TimedAsyncOperation::OnStart(thisSPtr);
    this->Execute(thisSPtr);
}

void ClientRequestAsyncOperation::OnCompleted()
{
    TimedAsyncOperation::OnCompleted();

    if (this->Error.IsSuccess())
    {
        WriteInfo(
            this->TraceComponent,
            "{0}: {1} -> Ended.",
            this->TraceId,
            this->TraceComponent);
    }
    else
    {
        WriteError(
            this->TraceComponent,
            "{0}: {1} -> Ended. Error: {2}",
            this->TraceId,
            this->TraceComponent,
            this->Error);
    }
}

void ClientRequestAsyncOperation::OnCancel()
{
    TimedAsyncOperation::OnCancel();

    WriteInfo(
        this->TraceComponent,
        "{0}: {1} -> Cancelled.",
        this->TraceId,
        this->TraceComponent);
}

void ClientRequestAsyncOperation::Complete(
    AsyncOperationSPtr const & thisSPtr,
    ErrorCode const & errorCode)
{
    if (!this->TryComplete(thisSPtr, errorCode))
    {
        Assert::CodingError("Failed to complete {0}.", this->TraceComponent);
    }
}

void ClientRequestAsyncOperation::SetReply(unique_ptr<ClientServerMessageBody> body)
{
    MessageUPtr msgUPtr = body ? make_unique<Message>(body->GetTcpMessageBody()) : make_unique<Message>();

    msgUPtr->Headers.Add(Transport::ActionHeader(this->RequestMsg.Action));
    msgUPtr->Headers.Add(Transport::ActorHeader(this->RequestMsg.Actor));
    msgUPtr->Headers.Add(Transport::FabricActivityHeader(this->ActivityId));

    this->replyMsgUPtr_ = move(msgUPtr);
}

void ClientRequestAsyncOperation::SetReply(MessageUPtr && replyMsg)
{
    this->replyMsgUPtr_ = move(replyMsg);
}