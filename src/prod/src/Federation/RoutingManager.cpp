// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Federation;
using namespace Transport;
using namespace Common;
using namespace std;

StringLiteral const TraceState("State");
StringLiteral const TraceForward("Forward");
StringLiteral const TraceReject("Reject");
StringLiteral const TraceProcess("Process");
StringLiteral const TraceHold("Hold");
StringLiteral const TraceFault("Fault");
StringLiteral const TraceIgnore("Ignore");
StringLiteral const TraceExpire("Expire");

RoutingManager::RoutingManager(__in SiteNode & siteNode)
    : siteNode_(siteNode)
{
}

RoutingManager::~RoutingManager()
{
}

RoutingHeader RoutingManager::GetRoutingHeader(
    __in Message & message,
    NodeInstance const & target,
    wstring const & toRing,
    TimeSpan expiration,
    TimeSpan retryTimeout,
    bool useExactRouting,
    bool expectsReply) const
{
    // Use the same MessageId for the MessageIdHeader and the RoutingHeader so the RelatesTo can reference the same callback
    if (message.MessageId.IsEmpty())
    {
        message.Headers.Add(MessageIdHeader());
    }

    return RoutingHeader(this->siteNode_.Instance, siteNode_.RingName, target, toRing, message.MessageId, expiration, retryTimeout, useExactRouting, expectsReply);
}

AsyncOperationSPtr RoutingManager::InternalBeginRoute(
    MessageUPtr && message,
    bool isRequest,
    NodeId nodeId,
    uint64 instance,
    wstring const & toRing,
    bool useExactRouting,
    TimeSpan retryTimeout,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    ASSERT_IFNOT(message, "Cannot route empty message");
    ASSERT_IFNOT(timeout >= Common::TimeSpan::Zero,
        "Invalid BeginRoute timeout: {0}", timeout);
    ASSERT_IFNOT(retryTimeout >= Common::TimeSpan::Zero,
        "Invalid BeginRoute retryTimeout: {0}", retryTimeout);

    auto header = this->GetRoutingHeader(*message, NodeInstance(nodeId, instance), toRing, timeout, retryTimeout, useExactRouting, isRequest);

    WriteNoise(
        TraceState,
        "{0} is starting to route message {1}, request = {2}",
        this->siteNode_.Id,
        message->MessageId,
        isRequest);

    // TODO: remove retry timeout since we can get it from the header
    auto localSiteNodeSPtr = this->siteNode_.GetSiteNodeSPtr();
    return AsyncOperation::CreateAndStart<RouteAsyncOperation>(
        localSiteNodeSPtr,
        std::move(message),
        nodeId,
        (message->Idempotent ? retryTimeout : TimeSpan::MaxValue),
        header,
        timeout,
        callback,
        parent);
}

AsyncOperationSPtr RoutingManager::BeginRoute(
    MessageUPtr && message,
    NodeId nodeId,
    uint64 instance,
    wstring const & toRing,
    bool useExactRouting,
    TimeSpan retryTimeout,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return this->InternalBeginRoute(std::move(message), false, nodeId, instance, toRing, useExactRouting, retryTimeout, timeout, callback, parent);
}

ErrorCode RoutingManager::EndRoute(AsyncOperationSPtr const & operation)
{
    MessageUPtr reply; // reply is ignored

    auto error = RouteAsyncOperation::End(operation, reply);

    WriteNoise(
        TraceState,
        "{0} has ended routing a message",
        this->siteNode_.Id);

    return error;
}

shared_ptr<AsyncOperation> RoutingManager::BeginRouteRequest(
    MessageUPtr && request,
    NodeId nodeId,
    uint64 instance,
    wstring const & toRing,
    bool useExactRouting,
    TimeSpan retryTimeout,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return this->InternalBeginRoute(std::move(request), true, nodeId, instance, toRing, useExactRouting, retryTimeout, timeout, callback, parent);
}

ErrorCode RoutingManager::EndRouteRequest(AsyncOperationSPtr const & operation, __out MessageUPtr & reply)
{
    auto error = RouteAsyncOperation::End(operation, reply);

    WriteNoise(
        TraceState,
        "{0} has ended routing a request",
        this->siteNode_.Id);

    return error;
}

void RoutingManager::SendReply(
    Transport::MessageUPtr && reply,
    PartnerNodeSPtr const & from, 
    Transport::MessageId const & relatesToId,
    RequestReceiverContextUPtr const & requestContext,
    bool sendAckToPreviousHop,
    bool isIdempotent)
{
    this->siteNode_.pointToPointManager_->PToPSend(std::move(reply), from, true, PToPActor::Direct);

    if (sendAckToPreviousHop)
    {
        requestContext->Reply(FederationMessage::GetRoutingAck().CreateMessage());
    }
    else
    {
        requestContext->From->OnSend(false);
    }

    if (isIdempotent)
    {
        bool removed = TryRemoveMessageIdFromProcessingSet(relatesToId);
        ASSERT_IFNOT(removed, "Message {0} should have been in the processing set", relatesToId);
    }
}

void RoutingManager::DispatchRoutedOneWayAndRequestMessages(
    __inout MessageUPtr & message,
    PartnerNodeSPtr const & from,
    RoutingHeader const & routingHeader,
    __inout RequestReceiverContextUPtr && requestReceiverContext,
    bool sendRoutingAck)
{
    bool removed = message->Headers.TryRemoveHeader<RoutingHeader>();
    ASSERT_IFNOT(removed, "routing header not present");

    // An application may expect the same messageid it gave to a routed message on the other side, this is also useful for testing
    message->Headers.TryRemoveHeader<MessageIdHeader>();
    message->Headers.Add(MessageIdHeader(routingHeader.MessageId));

    SiteNodeSPtr localSiteNodeSPtr = this->siteNode_.GetSiteNodeSPtr();
    bool isIdempotent = message->Idempotent;
    MessageId messageId = routingHeader.MessageId;

    // Check if the routed message expects a reply or not, this is the application reply, not the routing ack
    if (routingHeader.ExpectsReply)
    {
        RequestReceiverContextUPtr context = make_unique<RoutedRequestReceiverContext>(localSiteNodeSPtr, from, routingHeader.From, messageId, move(requestReceiverContext), sendRoutingAck, isIdempotent);

        // TODO: Remove this exception for broadcast during M1 refactoring
        BroadcastHeader broadcastHeader;
        if (message->Headers.TryReadFirst(broadcastHeader))
        {
            this->siteNode_.broadcastManagerUPtr_->OnRoutedBroadcastMessage(message, move(broadcastHeader), context);
        }
        else
        {
            MulticastHeader multicastHeader;
            if (message->Headers.TryReadFirst(multicastHeader))
            {
                this->siteNode_.multicastManagerUPtr_->OnRoutedMulticastMessage(message, move(multicastHeader), routingHeader, context);
            }
            else
            {
                this->siteNode_.pointToPointManager_->ActorDispatchRequest(message, context);
            }
        }
    }
    else
    {
        OneWayReceiverContextUPtr context = make_unique<RoutedOneWayReceiverContext>(localSiteNodeSPtr, from, routingHeader.From, messageId, move(requestReceiverContext), sendRoutingAck, isIdempotent);
        this->siteNode_.pointToPointManager_->ActorDispatchOneWay(message, context);
    }

    WriteNoise(
        TraceState,
        "Message {0} returned from being dispatched",
        messageId);
}

void RoutingManager::PreProcessRoutedMessages(__in Message & message, __inout RequestReceiverContextUPtr & requestReceiverContext)
{
    ASSERT_IF(message.FaultErrorCodeValue != ErrorCodeValue::Success, "Should be success");

    PToPHeader header;
    ASSERT_IFNOT(message.Headers.TryReadFirst(header), "Should have PToP headers since we came from P2P already");

    RoutingHeader routingHeader;
    if (!message.Headers.TryReadFirst(routingHeader))
    {
        WriteInfo(
            TraceReject,
            "dropping message: did not have a RoutingHeader: {0}({1}), status = {2}",
            message.MessageId, message.RetryCount, message.Status);
        auto error = ErrorCode(ErrorCodeValue::InvalidState);
        requestReceiverContext->InternalReject(error);
        return;
    }

    if (!this->siteNode_.IsRouting)
    {
        ASSERT_IF(
            header.To.InstanceId >= this->siteNode_.Instance.InstanceId && !this->siteNode_.IsShutdown,
            "Routed message {0} targeted to {1} sent to {2} phase {3}",
            message.MessageId, header.To, this->siteNode_.Instance, this->siteNode_.Phase);

        WriteInfo(TraceReject, "dropping message: reject {0}({1}) as node is in phase {2}", message.MessageId, message.RetryCount, this->siteNode_.Phase);
        auto error = ErrorCode(ErrorCodeValue::NodeIsNotRoutingFault);
        requestReceiverContext->InternalReject(error);
        return;
    }

    if (message.Idempotent)
    {
        if (!this->TryAddMessageIdToProcessingSet(routingHeader.MessageId))
        {
            WriteNoise(TraceIgnore, "Routed message is already being processed: RoutingId = {0}, MessageId = {1}({2})",
                routingHeader.MessageId,
                message.MessageId,
                message.RetryCount);

            return;
        }
    }

    auto messageId = message.MessageId;
    this->siteNode_.pointToPointManager_->RemovePToPHeaders(message);
    message.Headers.TryRemoveHeader<MessageIdHeader>();

    auto context = unique_ptr<RoutingContext>(new RoutingContext(message.Clone(), move(requestReceiverContext), routingHeader, header.From, messageId));

    this->ProcessRoutedMessages(std::move(context));
}

void RoutingManager::ProcessRoutedMessages(RoutingContextUPtr && context)
{
    auto messageId = context->messageId_;
    RoutingHeader const & routingHeader = context->routingHeader_;
    auto & requestReceiverContext = context->requestReceiverContext_;
    auto & request = context->message_;
    LONG retryCount = request->RetryCount;

    TimeSpan remainingTime = context->timeoutHelper_.GetRemainingTime();

    WriteNoise(
        TraceProcess,
        "Processing a message: {0}({1}), Routing MessageId = {2}, at node {3} destined for {4} with expiration of {5}",
        messageId,
        retryCount,
        routingHeader.MessageId,
        this->siteNode_.Instance,
        routingHeader.To,
        remainingTime);

    if (remainingTime <= TimeSpan::FromMilliseconds(100))
    {
        WriteInfo(TraceExpire, "Routed message has expired: Routing MessageId = {0}, MessageId = {1}({2})",
            context->routingHeader_.MessageId,
            messageId,
            retryCount);

        if (request->Idempotent)
        {
            // Not sure if it is in the list since this timeout could be on the first try or a retry
            TryRemoveMessageIdFromProcessingSet(routingHeader.MessageId);
        }

        return;
    }

    bool thisOwnsRoutingToken;
    PartnerNodeSPtr closestTarget;

    if (context->hopFrom_ == siteNode_.Instance && routingHeader.FromRing == siteNode_.RingName)
    {
        // We will only send a routed message to the local node if it is final destination.
        thisOwnsRoutingToken = true;
    }
    else if (remainingTime <= FederationConfig::GetConfig().MessageTimeout && context->message_->Action == FederationMessage::GetUpdateRequest().Action)
    {
        // UpdateRequest delivery can relax destination.
        thisOwnsRoutingToken = true;
    }
    else if (!this->TryGetRoutingHop(context, routingHeader.ToRing, closestTarget, thisOwnsRoutingToken))
    {
        WriteInfo(
            TraceHold,
            "Holding message {0}({1}) with routing id {2} at node {3} that was sent to node {4}",
            messageId,
            retryCount,
            routingHeader.MessageId,
            this->siteNode_.Instance,
            routingHeader.To);
        return;
    }

    // Do not send an ack back for the first hop since the final destination application will handle that
    bool sendRoutingAck = !context->isFirstHop_;

    // For now, process if we own the routing token even if we aren't the closest
    if (thisOwnsRoutingToken)
    {
        // This is the destination dispatch
        if (routingHeader.UseExactRouting && !this->siteNode_.Instance.Match(routingHeader.To))
        {
            // closest node, owning token, but not exact NodeInstance, send a fault
            WriteInfo(
                TraceReject,
                "Message {0}({1}) with routing id {2} is using exact routing but the closest ({3}) node does not match 'to' node {4}",
                messageId,
                retryCount,
                routingHeader.MessageId,
                this->siteNode_.Instance,
                routingHeader.To);

            if (request->Idempotent)
            {
                ASSERT_IFNOT(this->TryRemoveMessageIdFromProcessingSet(routingHeader.MessageId), "Message should have been in the processing set");
            }

            auto error = ErrorCode(ErrorCodeValue::RoutingNodeDoesNotMatchFault);
            requestReceiverContext->InternalReject(error);
            return;
        }

        auto from = this->siteNode_.Table.Get(routingHeader.From, routingHeader.FromRing);

        if (!from)
        {
            WriteInfo(
                TraceFault,
                "Message {0}({1}) with routing id {2} at node {3} reached its destination, but {4} is not in our routing table to reply to",
                messageId,
                retryCount,
                routingHeader.MessageId,
                this->siteNode_.Instance,
                routingHeader.From);

            if (request->Idempotent)
            {
                ASSERT_IFNOT(this->TryRemoveMessageIdFromProcessingSet(routingHeader.MessageId), "Message should have been in the processing set");
            }

            // TODO: reply with a suitable fault message
            auto error = ErrorCode(ErrorCodeValue::InvalidState);
            requestReceiverContext->InternalReject(error);
            return;
        }

        WriteNoise(
                TraceProcess,
                "Message {0}({1}) with routing id {2} sent to {3} has reached its destination at {4}",
                messageId,
                retryCount,
                routingHeader.MessageId,
                routingHeader.To,
                this->siteNode_.Instance);

        this->DispatchRoutedOneWayAndRequestMessages(request, from, routingHeader, move(requestReceiverContext), sendRoutingAck);
    }
    else
    {
        ASSERT_IF(this->siteNode_.Instance.Match(closestTarget->Instance), "can't forward to self");

        // TODO: Checks for bad retry timeouts such as MaxValue
        TimeSpan timeout = remainingTime;

        if (request->Idempotent && routingHeader.RetryTimeout < remainingTime)
        {
            timeout = routingHeader.RetryTimeout;

            // TODO: should trace and drop, but catch these errors and throw for now
            ASSERT_IF(timeout == TimeSpan::Zero, "this shouldn't happen");
        }

        auto clonedMessage = request->Clone();

        // In case compaction has placed any PToP headers into our orginal
        this->siteNode_.pointToPointManager_->RemovePToPHeaders(*clonedMessage);
        clonedMessage->Headers.TryRemoveHeader<MessageIdHeader>();

        // and a new id for this message and update the context
        clonedMessage->Headers.Add(MessageIdHeader());
        context->messageId_ = clonedMessage->MessageId;

        WriteInfo(
            TraceForward,
            "Routing message {0}({1}) with routing id {2} from {3} to {4} with timeout of {5} via {6}: {7}",
            context->messageId_,
            retryCount,
            routingHeader.MessageId,
            this->siteNode_.Instance,
            routingHeader.To,
            timeout,
            closestTarget->Instance,
            *request);

        // Use P2P so we don't add RoutingHeader again (it should still be there from clone)
        MoveUPtr<RoutingContext> mover(std::move(context));
        this->siteNode_.pointToPointManager_->BeginPToPSendRequest(
            std::move(clonedMessage),
            closestTarget,
            false,
            PToPActor::Routing,
            timeout,
            [this, mover, sendRoutingAck, closestTarget](AsyncOperationSPtr contextSPtr) mutable -> void
            {
                this->OnRouteReply(mover.TakeUPtr(), sendRoutingAck, closestTarget, contextSPtr);
            },
            AsyncOperationSPtr());
    }
}

void RoutingManager::OnRouteReply(RoutingContextUPtr && context, bool sendRoutingAck, PartnerNodeSPtr const & hopTo, AsyncOperationSPtr const & contextSPtr)
{
    // NOTE: context has requestReceiverContext which has the callback to sitenode which is keeping it alive so we can pass this pointer

    MessageUPtr reply;
    ErrorCode error = this->siteNode_.pointToPointManager_->EndPToPSendRequest(contextSPtr, reply);

    if(!this->siteNode_.IsShutdown)
    {
        if (error.IsSuccess())
        {
            WriteNoise(
                TraceProcess,
                "Hop completed for message {0}({1}) from {2} to {3}. RoutingId = {4}, Reason = {5}, SendAck = {6}",
                context->messageId_,
                context->message_->RetryCount,
                this->siteNode_.Instance,
                hopTo,
                context->routingHeader_.MessageId,
                error.ReadValue(),
                sendRoutingAck);

            if (sendRoutingAck)
            {
                context->requestReceiverContext_->Reply(FederationMessage::GetRoutingAck().CreateMessage());
            }
            else
            {
                context->requestReceiverContext_->From->OnSend(false);
            }

            if (context->message_->Idempotent)
            {
                ASSERT_IFNOT(this->TryRemoveMessageIdFromProcessingSet(context->routingHeader_.MessageId), "Message should have been in the processing set");
            }
        }
        else
        {
            WriteInfo(
                TraceFault,
                "Hop failed for message {0}({1}) from {2} to {3}. RoutingId = {4}, Reason = {5}",
                context->messageId_,
                context->message_->RetryCount,
                this->siteNode_.Instance,
                hopTo,
                context->routingHeader_.MessageId,
                error.ReadValue());

            if (IsRetryable(error, context->message_->Idempotent))
            {
                // Update the cached message with the new routingheader
                context->UpdateRoutingHeader();

                this->ProcessRoutedMessages(std::move(context));
            }
            else
            {
                if (context->message_->Idempotent)
                {
                    this->TryRemoveMessageIdFromProcessingSet(context->routingHeader_.MessageId);
                }
                WriteInfo(
                    TraceReject,
                    "dropping message: {0}({1}), status = {2}, non-retryable error: {3}",
                    context->message_->MessageId, context->message_->RetryCount, context->message_->Status, error);
                context->requestReceiverContext_->InternalReject(error);
            }
        }
    }
    else
    {
        WriteNoise(
            TraceIgnore,
            "Hop reply result ignored for message {0}({1}) from {2} to {3} because node is shutdown. RoutingId = {4}, Reason = {5}",
            context->messageId_,
            context->message_->RetryCount,
            this->siteNode_.Instance,
            hopTo,
            context->routingHeader_.MessageId,
            error.ReadValue());
    }
}

// Return true if we can continue to process this message or false if the current node must wait for events that changes the token or neighborhood
// If returning false the context will moved to a list else it will remain
bool RoutingManager::TryGetRoutingHop(__in RoutingContextUPtr & context, wstring const & toRing, __out PartnerNodeSPtr & closestNode, __out bool & thisOwnsRoutingToken)
{
    // We need the lock here in case we decide to hold the message, but the events come in before adding to the list.  Taking the lock now will prevent the
    // processing from happening until this has completed.

    if(this->siteNode_.IsShutdown)
    {
        WriteWarning(TraceIgnore, "Routing dropping message {0} because node is now shutdown", context->routingHeader_.MessageId);

        return false;
    }

    AcquireExclusiveLock lock(this->routedMessageHoldingListLock_);

    closestNode = this->siteNode_.Table.GetRoutingHop(context->routingHeader_.To.Id, toRing, false, thisOwnsRoutingToken);

    if(!closestNode)
    {
        //The node is not opened so we can delay delivery until node is open
        this->routedMessageHoldingList_.push_back(std::move(context));

        return false;
    }

    bool thisIsClosestNodeId = closestNode->Instance.Match(siteNode_.Instance) && (closestNode->RingName == siteNode_.RingName);
    if (thisIsClosestNodeId && !thisOwnsRoutingToken)
    {
        if (context->message_->Action != FederationMessage::GetUpdateRequest().Action)
        {
            this->routedMessageHoldingList_.push_back(std::move(context));
            return false;
        }

        thisOwnsRoutingToken = true;
    }

    return true;
}

void RoutingManager::ClearRoutedMessageHoldingList()
{
    AcquireExclusiveLock lock(this->routedMessageHoldingListLock_);

    for (auto & context : this->routedMessageHoldingList_)
    {
        WriteInfo(TraceHold, "Routing dropping message {0}({1}) from holding table", context->messageId_, context->message_->RetryCount);

        // TODO: Reject each request with a cancel error code
        // TODO: The id may still be in the processing set, we could remove it, but SiteNode should be shutting down anyway...
    }

    this->routedMessageHoldingList_.clear();
}

bool RoutingManager::TryAddMessageIdToProcessingSet(MessageId const & id)
{
    AcquireExclusiveLock lock(this->routedMessageProcessingSetLock_);

    auto findIter = this->routedMessageProcessingSet_.find(id);

    if (findIter != this->routedMessageProcessingSet_.end())
    {
        return false;
    }

    this->routedMessageProcessingSet_.insert(id);

    return true;
}

bool RoutingManager::TryRemoveMessageIdFromProcessingSet(MessageId const & id)
{
    AcquireExclusiveLock lock(this->routedMessageProcessingSetLock_);

    auto findIter = this->routedMessageProcessingSet_.find(id);

    if (findIter == this->routedMessageProcessingSet_.end())
    {
        return false;
    }

    this->routedMessageProcessingSet_.erase(findIter);

    return true;
}

void RoutingManager::ProcessRoutedMessageHoldingList()
{
    // Token change must have happened, process all messages
    list<RoutingContextUPtr> listToProcess;

    {
        AcquireExclusiveLock lock(this->routedMessageHoldingListLock_);

        WriteInfo(
            TraceHold,
            "Processing holding list at {0} with size of {1}",
            this->siteNode_.Instance,
            this->routedMessageHoldingList_.size());

        // This code will add any item that was removed to the other list to be processed
        auto removeIter = remove_if(
            this->routedMessageHoldingList_.begin(),
            this->routedMessageHoldingList_.end(),
            [&listToProcess, this](RoutingContextUPtr & context) -> bool
        {
            bool result = false;
            bool thisIsClosestNodeId = false;
            bool thisOwnsRoutingToken = false;

            RoutingHeader routingHeader = context->routingHeader_;

            // TODO: this is common in ProcessRoutedMessages, consider refactoring
            
            PartnerNodeSPtr closestTarget = this->siteNode_.Table.GetRoutingHop(routingHeader.To.Id, routingHeader.ToRing, false, thisOwnsRoutingToken);
            if (!closestTarget)
            {
                // If a null target was returned we can't do anything
                WriteInfo(
                    TraceHold,
                    "Inspecting held message at node {0} sent from {1} to {2} with id of {3}. No ClosestTarget",
                    this->siteNode_.Instance,
                    routingHeader.From,
                    routingHeader.To,
                    routingHeader.MessageId);
            }
            else
            {
                thisIsClosestNodeId = closestTarget->Instance.Match(this->siteNode_.Instance);

                // Route or process if we own the token or if we dont own the token and we are not the closest anymore
                if (thisOwnsRoutingToken || (!thisIsClosestNodeId && !thisOwnsRoutingToken))
                {
                    listToProcess.push_back(std::move(context));
                    result = true;
                }

                WriteInfo(
                    TraceHold,
                    "Inspecting held message at node {0} sent from {1} to {2} with id of {3}. ClosestTarget = {4}, thisIsClosestNodeId = {5}, thisOwnsRoutingToken = {6}, result = {7}",
                    this->siteNode_.Instance,
                    routingHeader.From,
                    routingHeader.To,
                    routingHeader.MessageId,
                    closestTarget->Instance,
                    thisIsClosestNodeId,
                    thisOwnsRoutingToken,
                    result);
            }

            return result;
        });

        this->routedMessageHoldingList_.erase(removeIter, this->routedMessageHoldingList_.end());
    }

    WriteInfo(
        TraceHold,
        "Processing holding list at {0} found {1} items to dispatch",
        this->siteNode_.Instance,
        listToProcess.size());

    for (auto iter = listToProcess.begin(); iter != listToProcess.end(); ++iter)
    {
        auto & context = *iter;

        // Update the expiration
        context->UpdateRoutingHeader();

        WriteInfo(
            TraceHold,
            "Processing {0}({1}), Routing MessageId = {2}, from holding table",
            context->messageId_,
            context->message_->RetryCount,
            context->routingHeader_.MessageId);

        this->ProcessRoutedMessages(std::move(context));
    }
}

void RoutingManager::Stop()
{
    this->ClearRoutedMessageHoldingList();
}

void RoutingManager::RoutingContext::UpdateRoutingHeader()
{
    // Update the cached message with the new routingheader
    this->routingHeader_.Expiration = this->timeoutHelper_.GetRemainingTime();
    if (this->routingHeader_.Expiration > TimeSpan::Zero)
    {
        bool removed = this->message_->Headers.TryRemoveHeader<RoutingHeader>();
        ASSERT_IFNOT(removed, "Header must be there");
        this->message_->Headers.Add(this->routingHeader_);
    }
}

bool RoutingManager::IsRetryable(ErrorCode error, bool isIdempotent)
{
    if (error.IsError(ErrorCodeValue::Timeout))
    {
        return isIdempotent;
    }

    return error.IsError(ErrorCodeValue::NodeIsNotRoutingFault) || error.IsError(ErrorCodeValue::P2PNodeDoesNotMatchFault);
}
