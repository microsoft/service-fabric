// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace ServiceModel;
using namespace ClientServerTransport;

using namespace Management::ResourceManager;

StringLiteral const TraceComponent("ReplicaActivityAsyncOperation");

template<typename TReceiverContext>
ReplicaActivityAsyncOperation<TReceiverContext>::ReplicaActivityAsyncOperation(
    MessageUPtr requestMsg,
    TReceiverContextUPtr receiverContext,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
    : TimedAsyncOperation(timeout, callback, parent)
    , activityId_(FabricActivityHeader::FromMessage(*requestMsg).ActivityId)
    , requestMsg_(move(requestMsg))
    , receiverContext_(move(receiverContext))
    , replyMsg_(nullptr)
{
}

template<typename TReceiverContext>
ErrorCode ReplicaActivityAsyncOperation<TReceiverContext>::End(
    AsyncOperationSPtr const & asyncOperation,
    __out MessageUPtr & replyMsg,
    __out TReceiverContextUPtr & receiverContext)
{
    auto casted = AsyncOperation::End<ReplicaActivityAsyncOperation<TReceiverContext>>(asyncOperation);

    if (casted->replyMsg_)
    {
        replyMsg = move(casted->replyMsg_);
    }

    receiverContext = move(casted->receiverContext_);

    return casted->Error;
}

template<typename TReceiverContext>
void ReplicaActivityAsyncOperation<TReceiverContext>::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    TimedAsyncOperation::OnStart(thisSPtr);
    this->Execute(thisSPtr);
}

template<typename TReceiverContext>
void ReplicaActivityAsyncOperation<TReceiverContext>::OnTimeout(AsyncOperationSPtr const & thisSPtr)
{
    Trace.WriteWarning(
        TraceComponent,
        "{0}: Timeout.",
        this->ActivityId);

    TimedAsyncOperation::OnTimeout(thisSPtr);
}

template<typename TReceiverContext>
void ReplicaActivityAsyncOperation<TReceiverContext>::SetReply(unique_ptr<ClientServerMessageBody> body)
{
    if (body)
    {
        ResourceManagerMessage rmMsg(this->RequestMsg.Action, this->RequestMsg.Actor, move(body), this->ActivityId);
        this->replyMsg_ = move(rmMsg.GetTcpMessage());
    }
    else
    {
        ResourceManagerMessage rmMsg(this->RequestMsg.Action, this->RequestMsg.Actor, this->ActivityId);
        this->replyMsg_ = move(rmMsg.GetTcpMessage());
    }
}

template<typename TReceiverContext>
void ReplicaActivityAsyncOperation<TReceiverContext>::SetReply(MessageUPtr && replyMsg)
{
    this->replyMsg_ = move(replyMsg);
}

template<typename TReceiverContext>
void ReplicaActivityAsyncOperation<TReceiverContext>::Complete(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error)
{
    if (!this->replyMsg_)
    {
        this->SetReply(static_cast<unique_ptr<ClientServerMessageBody>>(nullptr));
    }

    this->TryComplete(thisSPtr, error);
}

template class Management::ResourceManager::ReplicaActivityAsyncOperation<Transport::IpcReceiverContext>;
template class Management::ResourceManager::ReplicaActivityAsyncOperation<Federation::RequestReceiverContext>;
