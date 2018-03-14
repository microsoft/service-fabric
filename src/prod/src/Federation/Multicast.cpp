// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace std;
using namespace Transport;

#define MULTICAST_ERROR_MESSAGE_PROPERTY "MulticastError"
#define MULTICAST_DESTINATION_MESSAGE_PROPERTY "MulticastDestination"

MulticastReplyContext::MulticastReplyContext(MessageUPtr && message, vector<NodeInstance> && destinations, SiteNode & siteNode)
    : message_(move(message)),
    destinations_(destinations),
    siteNode_(siteNode),
    replies_(ReaderQueue<Message>::Create())
{
}

IMultipleReplyContextSPtr MulticastReplyContext::CreateAndStart(MessageUPtr && message, vector<NodeInstance> && destinations, SiteNode & siteNode)
{
    MulticastReplyContext* multicastReplyContext = new MulticastReplyContext(move(message), move(destinations), siteNode);
    IMultipleReplyContextSPtr result((IMultipleReplyContext*) multicastReplyContext);
    multicastReplyContext->Start();
    return result;
}

MulticastReplyContext::~MulticastReplyContext()
{
    for (auto requestIter = this->requests_.begin(); requestIter != this->requests_.end(); ++ requestIter)
    {
        (*requestIter)->Cancel();
    }
}

void MulticastReplyContext::Start()
{
    for (auto iter = destinations_.cbegin(); iter != destinations_.cend(); ++ iter)
    {
        NodeInstance const & destination = *iter;
        WriteNoise("Start", siteNode_.TraceId, "Routing request to {0}", destination);
        auto request = siteNode_.BeginRouteRequest(
            message_->Clone(),
            destination.Id,
            destination.InstanceId,
            true,
            TimeSpan::MaxValue,
            [this, destination] (AsyncOperationSPtr const & operation) { OnReply(operation, destination); },
            siteNode_.CreateAsyncOperationRoot());
        this->requests_.push_back(move(request));
    }
}

void MulticastReplyContext::Close()
{
    this->replies_->Close();
}

void MulticastReplyContext::OnReply(AsyncOperationSPtr const & operation, NodeInstance const & requestDestination)
{
    MessageUPtr reply;
    ErrorCode result = siteNode_.EndRouteRequest(operation, reply);
    WriteNoise("OnReply", siteNode_.TraceId, "EndRouteRequest() returns {0} for {1}", result, requestDestination);

    if (!result.IsSuccess())
    {
        reply = make_unique<Message>();
        reply->AddProperty(result.ReadValue(), MULTICAST_ERROR_MESSAGE_PROPERTY);
    }

    reply->AddProperty(requestDestination, MULTICAST_DESTINATION_MESSAGE_PROPERTY);
    this->replies_->EnqueueAndDispatch(move(reply));
}

AsyncOperationSPtr MulticastReplyContext::BeginReceive(TimeSpan timeout, AsyncCallback const & callback, AsyncOperationSPtr const & parent)
{
    return this->replies_->BeginDequeue(timeout, callback, parent);
}

ErrorCode MulticastReplyContext::EndReceive(AsyncOperationSPtr const & operation, __out MessageUPtr & reply, __out NodeInstance & requestDestination)
{
    auto result = this->replies_->EndDequeue(operation, reply);
    if (!result.IsSuccess())
    {
        return result;
    }

    NodeInstance* requestDestinationPtr = reply->TryGetProperty<NodeInstance>(MULTICAST_DESTINATION_MESSAGE_PROPERTY);
    ASSERT_IF(requestDestinationPtr == nullptr, "requestDestination message property missing");
    requestDestination = *requestDestinationPtr;

    ErrorCodeValue::Enum * routeRequestResult;
    routeRequestResult = reply->TryGetProperty<ErrorCodeValue::Enum>(MULTICAST_ERROR_MESSAGE_PROPERTY);
    if (routeRequestResult)
    {
        result = *routeRequestResult;
        return result;
    }

    return result;
}
