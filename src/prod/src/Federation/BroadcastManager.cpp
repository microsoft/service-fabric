// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Federation;
using namespace Transport;
using namespace Common;
using namespace std;

const int BroadcastStepCountMax = 10; // If the step count has ever gone this far, rebroadcast the message to the whole ring

StringLiteral const BroadcastTimerTag("Broadcast");

StringLiteral const TraceStart("Start");
StringLiteral const TraceRoute("Route");
StringLiteral const TraceRange("Range");
StringLiteral const TraceReceive("Receive");
StringLiteral const TraceDrop("Drop");
StringLiteral const TraceFault("Fault");
StringLiteral const TraceCancel("Cancel");

class BroadcastManager::ReliableOneWayBroadcastOperation : public AsyncOperation
{
public:
    ReliableOneWayBroadcastOperation(AsyncCallback const & callback, AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent)
    {
    }

    void OnStart(AsyncOperationSPtr const &)
    {
    }
};

BroadcastManager::BroadcastManager(SiteNode & siteNode)
    :   siteNode_(siteNode),
        broadcastMessagesAlreadySeen_(FederationConfig::GetConfig().BroadcastContextKeepDuration),
        reliableBroadcastContexts_(FederationConfig::GetConfig().BroadcastContextKeepDuration),
        closed_(false)
{
    SiteNodeSPtr siteNodeSPtr = siteNode.GetSiteNodeSPtr();
    timer_ = Timer::Create(
        BroadcastTimerTag,
        [this, siteNodeSPtr] (TimerSPtr const &)
        {
            this->OnTimer();
        },
        true);

    TimeSpan interval = FederationConfig::GetConfig().BroadcastContextKeepDuration;
    timer_->Change(interval, interval);
}

BroadcastManager::~BroadcastManager()
{
}

void BroadcastManager::OnTimer()
{
    AcquireWriteLock grab(lock_);
    this->broadcastMessagesAlreadySeen_.RemoveExpiredEntries();
    this->reliableBroadcastContexts_.RemoveExpiredEntries();
}

BroadcastHeader BroadcastManager::AddBroadcastHeaders(__in Message & message, bool expectsReply, bool expectsAck)
{
    MessageId id = message.MessageId;
    if (id.IsEmpty())
    {
        id = MessageId();
    }
    else
    {
        message.Headers.TryRemoveHeader<MessageIdHeader>();
    }

    message.Idempotent = true;

    auto header = BroadcastHeader(this->siteNode_.Instance, id, expectsReply, expectsAck, siteNode_.RingName);

    message.Headers.Add(header);

    return header;
}

void BroadcastManager::OnBroadcastCompleted(MessageId const & broadcastId)
{
    this->requestTable_.Remove(broadcastId);
}

void BroadcastManager::Broadcast(MessageUPtr && message)
{
    auto header = this->AddBroadcastHeaders(*message, false, false);
    this->InternalBroadcast(std::move(message), header);
}

AsyncOperationSPtr BroadcastManager::BeginBroadcast(MessageUPtr && message, bool toAllRings, AsyncCallback const & callback, AsyncOperationSPtr const & parent)
{
    auto header = this->AddBroadcastHeaders(*message, false, true);
    WriteInfo(
        TraceStart,
        "Broadcast started for {0}",
        header);

    AsyncOperationSPtr operation = AsyncOperation::CreateAndStart<ReliableOneWayBroadcastOperation>(callback, parent);
    if (!BroadcastWithAck(message, toAllRings, NodeIdRange::Full, header, nullptr, operation))
    {
        operation->TryComplete(operation, ErrorCodeValue::ObjectClosed);
    }

    return operation;
}

ErrorCode BroadcastManager::EndBroadcast(AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<ReliableOneWayBroadcastOperation>(operation)->Error;
}

IMultipleReplyContextSPtr BroadcastManager::BroadcastRequest(MessageUPtr && message, TimeSpan retryInterval)
{
    auto header = this->AddBroadcastHeaders(*message, true, false);

    // It is the responsibility of the person making the broadcast to keep SiteNode alive for this context
    BroadcastReplyContextSPtr replyContext = make_shared<BroadcastReplyContext>(
        *this,
        message->Clone(),
        header.BroadcastId,
        retryInterval);

    auto error = this->requestTable_.TryAdd(header.BroadcastId, replyContext);
    if (error.IsSuccess())
    {
        this->InternalBroadcast(std::move(message), header);

        // Start the timer to do retries
        replyContext->Start();

        return replyContext;
    }
    else
    {
        if (error.IsError(ErrorCodeValue::ObjectClosed))
        {
            WriteInfo(TraceStart, "dropping message {0}, couldn't add context: {1}", header.BroadcastId, error);
        }
        else
        {
            WriteError(TraceStart, "dropping message {0}, couldn't add context: {1}", header.BroadcastId, error);
            Assert::TestAssert("broadcast message dropping message {0} err {1}", header.BroadcastId, error);
        }

        return IMultipleReplyContextSPtr();
    }
}

void BroadcastManager::InternalBroadcast(MessageUPtr && message, BroadcastHeader const & header)
{
    WriteInfo(
        TraceStart,
        "Broadcast started for {0}",
        header);

    this->BroadcastMessageToRange(message, NodeIdRange::Full, header);
    auto from = dynamic_pointer_cast<PartnerNode>(this->siteNode_.shared_from_this());
    this->BroadcastToSuccessorAndPredecessor(message, from, 0, header);
    this->DispatchBroadcastMessage(message, header);
}

void BroadcastManager::ProcessBroadcastMessage(__in MessageUPtr & message, PartnerNodeSPtr const & hopFrom)
{
    BroadcastRelatesToHeader relatesToHeader;
    if (message->Headers.TryReadFirst(relatesToHeader))
    {
        // Reply message, dispatch it
        this->DispatchReplyMessage(*message, relatesToHeader.MessageId, hopFrom->Token.Range);
        return;
    }
    
    ASSERT_IFNOT(message->Idempotent, "Message must be idempotent to be a broadcast message)");
    
    BroadcastHeader broadcastHeader;
    if (!message->Headers.TryReadFirst(broadcastHeader))
    {
        Assert::CodingError("No BroadcastHeader at node: {0} for message {1}({2})",
            this->siteNode_.Instance,
            message->MessageId,
            message->RetryCount);
    }

    {
        AcquireWriteLock grab(lock_);
        if (!this->broadcastMessagesAlreadySeen_.TryAdd(broadcastHeader.BroadcastId))
        {
            // Already seen, drop it
            WriteNoise(
                TraceDrop,
                "Broadcast dropping message {0}, already seen at node {1} with BroadcastId {2}",
                message->MessageId,
                this->siteNode_.Instance,
                broadcastHeader.BroadcastId);
            return;
        }
    }

    this->ForwardAndDispatch(message, hopFrom, broadcastHeader, nullptr);
}

bool BroadcastManager::DispatchBroadcastMessage(
    MessageUPtr & message,
    BroadcastHeader const & broadcastHeader,
    RequestReceiverContextUPtr && routedRequestContext)
{
    // An application may expect the same messageid it gave to a broadcast message on the other side, this is also useful for testing
    ASSERT_IF(!message->MessageId.IsEmpty(), "MessageId not removed for {0}", *message);
    message->Headers.Add(MessageIdHeader(broadcastHeader.BroadcastId));

    PartnerNodeSPtr from = this->siteNode_.Table.Get(broadcastHeader.From, broadcastHeader.FromRing);
    if (!from)
    {
        WriteWarning(
            TraceFault,
            "Could not get the nodeId, {0}, from the routing table at node {1} for message broadcast id {2}",
            broadcastHeader.From,
            this->siteNode_.Instance,
            broadcastHeader.BroadcastId);
        return false;
    }
    
    if (broadcastHeader.ExpectsReply)
    {
        ASSERT_IFNOT(message->RelatesTo.IsEmpty(), "not a PToP request");

        RequestReceiverContextUPtr context = make_unique<BroadcastRequestReceiverContext>(this->siteNode_.GetSiteNodeSPtr(), move(routedRequestContext), from, broadcastHeader.From, broadcastHeader.BroadcastId);
        this->siteNode_.GetPointToPointManager().ActorDispatchRequest(message, context);
    }
    else
    {
        OneWayReceiverContextUPtr context;
        if (broadcastHeader.ExpectsAck)
        {
            context = make_unique<BroadcastAckReceiverContext>(*this, broadcastHeader.BroadcastId, from, broadcastHeader.From);
        }
        else
        {
            context = make_unique<OneWayReceiverContext>(from, broadcastHeader.From, broadcastHeader.BroadcastId);
        }

        this->siteNode_.GetPointToPointManager().ActorDispatchOneWay(message, context);
    }

    return true;
}

void BroadcastManager::ForwardAndDispatch(
    MessageUPtr & message, 
    PartnerNodeSPtr const & hopFrom,
    BroadcastHeader const & header,
    RequestReceiverContextUPtr && requestContext)
{
    // Clean message of PToP and message id headers so we can forward it safely
    message->Headers.TryRemoveHeader<MessageIdHeader>();
    this->siteNode_.GetPointToPointManager().RemovePToPHeaders(*message);

    BroadcastRangeHeader rangeHeader;
    BroadcastStepHeader stepHeader;
    if (message->Headers.TryGetAndRemoveHeader(rangeHeader))
    {
        if (header.ExpectsAck)
        {
            this->BroadcastWithAck(message, false, rangeHeader.Range, header, move(requestContext), nullptr);
            return;
        }
        else
        {
            this->BroadcastMessageToRange(message, rangeHeader.Range, header);
            this->BroadcastToSuccessorAndPredecessor(message, hopFrom, 0, header);
        }
    }
    else
    {
        ASSERT_IF(header.ExpectsAck, "Range header not found for broadcast with header {0}", header);

        if (message->Headers.TryGetAndRemoveHeader(stepHeader))
        {
            if (stepHeader.Count >= BroadcastStepCountMax)
            {
                this->BroadcastMessageToRange(message, NodeIdRange::Full, header);
                this->BroadcastToSuccessorAndPredecessor(message, hopFrom, 0, header);
            }
            else
            {
                this->BroadcastToSuccessorAndPredecessor(message, hopFrom, stepHeader.Count + 1, header);
            }
        }
        else
        {
            WriteError(
                TraceFault,
                "No range or step header at node: {0} for broadcast id {1}",
                this->siteNode_.Instance,
                header.BroadcastId);

            return;
        }
    }

    this->DispatchBroadcastMessage(message, header, move(requestContext));
}

void BroadcastManager::BroadcastToSuccessorAndPredecessor(MessageUPtr & message, PartnerNodeSPtr const & hopFrom, int broadcastStepCount, BroadcastHeader const & header)
{
    ASSERT_IFNOT(hopFrom, "message must have come from somewhere");

    // TODO: Send successor and predecessor nodes seperately for now to simplify the code, use the same header collection later
    // TODO: optimize by not sending if they are already in the exponential list
    auto successor = this->siteNode_.Table.GetSuccessor();
    auto predecessor = this->siteNode_.Table.GetPredecessor();

    auto stepHeader = BroadcastStepHeader(broadcastStepCount);

    if (successor && 
        !hopFrom->Instance.Match(successor->Instance) &&
        !this->siteNode_.Instance.Match(successor->Instance))
    {
        BroadcastEventSource::Events->ForwardToSuccessor(
            message->Action,
            header.BroadcastId,
            successor->Instance,
            this->siteNode_.Instance);

        auto clone = message->Clone();
        clone->Headers.Add(stepHeader);
        this->siteNode_.GetPointToPointManager().PToPSend(std::move(clone), successor, true, PToPActor::Broadcast);
    }

    if (predecessor &&
        predecessor->Id != successor->Id &&
        !hopFrom->Instance.Match(predecessor->Instance) &&
        !this->siteNode_.Instance.Match(predecessor->Instance))
    {
        BroadcastEventSource::Events->ForwardToPredecessor(
            message->Action,
            header.BroadcastId,
            predecessor->Instance,
            this->siteNode_.Instance);

        auto clone = message->Clone();
        clone->Headers.Add(stepHeader);
        this->siteNode_.GetPointToPointManager().PToPSend(std::move(clone), predecessor, true, PToPActor::Broadcast);
    }
}

void BroadcastManager::BroadcastMessageToRange(
    MessageUPtr & message,
    NodeIdRange const & range, 
    BroadcastHeader const & header)
{
    ASSERT_IF(range == NodeIdRange::Empty, "range shouldn't be empty");

    WriteInfo(
        TraceRange,
        "Broadcasting message {0} header {1} to range: {2} on node {3} hood range {4}",
        message->Action,
        header,
        range,
        this->siteNode_.Id,
        this->siteNode_.Table.GetHoodRange());

    vector<PartnerNodeSPtr> targets;
    vector<NodeIdRange> subRanges;

    this->siteNode_.Table.PartitionRanges(range, targets, subRanges, true);

    if (targets.size() == 0)
    {
        WriteInfo(
            TraceRange,
            "there are no nodes to broadcast message broadcast id {0} from node {1}",
            header.BroadcastId,
            this->siteNode_.Instance);

        return;
    }

    MessageHeadersCollection headersCollection(targets.size());
    for (size_t i = 0; i < targets.size(); i++)
    {
        headersCollection[i].Add(BroadcastRangeHeader(subRanges[i]));
        WriteInfo(
            TraceRange,
            "Broadcast {0} range {1} assigned to {2}",
            header.BroadcastId,
            subRanges[i],
            targets[i]->Id);
    }

    this->siteNode_.GetPointToPointManager().PToPMulticastSend(message->Clone(), targets, std::move(headersCollection), PToPActor::Broadcast);
}

bool BroadcastManager::BroadcastWithAck(
    MessageUPtr & message,
    bool toAllRings,
    NodeIdRange const & range, 
    BroadcastHeader const & header,
    RequestReceiverContextUPtr && requestContext,
    AsyncOperationSPtr const & operation)
{
    ASSERT_IF(range == NodeIdRange::Empty, "range shouldn't be empty");

    WriteInfo(
        TraceRange,
        "Broadcasting message {0} header {1} to range: {2} on node {3}",
        message->Action,
        header,
        range,
        this->siteNode_.Id);

    MessageId broadcastId = header.BroadcastId;

    vector<PartnerNodeSPtr> targets;
    vector<NodeIdRange> subRanges;
    vector<wstring> externalRings;

    NodeIdRange localRange = this->siteNode_.Table.PartitionRanges(range, targets, subRanges, false);
    if (toAllRings)
    {
        this->siteNode_.Table.GetExternalRings(externalRings);
    }

    bool dispatch = false;
    bool completeRequest = false;
    {
        AcquireWriteLock grab(lock_);
        if (closed_)
        {
            return false;
        }

        auto it = reliableBroadcastContexts_.find(broadcastId);
        if (it == reliableBroadcastContexts_.end())
        {
            vector<NodeIdRange> pending = subRanges;
            pending.push_back(localRange);
            
            bool added = this->reliableBroadcastContexts_.TryAdd(
                header.BroadcastId,
                BroadcastForwardContext(siteNode_.Id, broadcastId, range, move(pending), operation, move(requestContext), externalRings));
            ASSERT_IF(!added, "Add multicast context failed");

            dispatch = true;
        }
        else
        {
            subRanges.clear();
            targets.clear();
            completeRequest = !it->second.AddRange(range, localRange, subRanges, requestContext);
        }
    }

    for (size_t i = 0; i < subRanges.size(); i++)
    {
        NodeIdRange subRange = subRanges[i];
        MessageUPtr forwardMessage = message->Clone();
        forwardMessage->Headers.Add(BroadcastRangeHeader(subRange));
        NodeId targetId;

        if (targets.size() > 0)
        {
            targetId = targets[i]->Id;

            WriteInfo(
                TraceRange,
                "Broadcast {0} range {1} assigned to {2} on node {3}",
                broadcastId, subRange, targetId, siteNode_.Id);
        }
        else
        {
            targetId = subRange.Begin.GetSuccMidPoint(subRange.End);

            WriteInfo(
                TraceRange,
                "Broadcast {0} range {1} midpoint {2} on node {3}",
                broadcastId, subRange, targetId, siteNode_.Id);
        }

        this->siteNode_.BeginRouteRequest(
            std::move(forwardMessage), 
            targetId,
            0, 
            false,
            TimeSpan::MaxValue,
            [this, broadcastId, subRange](AsyncOperationSPtr const & operation)
            {
                MessageUPtr reply;
                ErrorCode error = this->siteNode_.EndRouteRequest(operation, reply);
                this->ProcessBroadcastAck(broadcastId, L"", subRange, error);
            },
            AsyncOperationSPtr());
    }
    
    for (size_t i = 0; i < externalRings.size(); i++)
    {
        wstring ringName = externalRings[i];
        MessageUPtr forwardMessage = message->Clone();
        forwardMessage->Headers.Add(BroadcastRangeHeader(NodeIdRange::Full));

        this->siteNode_.BeginRouteRequest(
            std::move(forwardMessage), 
            siteNode_.Id,
            0,
            ringName,
            false,
            FederationConfig::GetConfig().RoutingRetryTimeout,
            TimeSpan::MaxValue,
            [this, broadcastId, ringName](AsyncOperationSPtr const & operation)
            {
                MessageUPtr reply;
                ErrorCode error = this->siteNode_.EndRouteRequest(operation, reply);
                this->ProcessBroadcastAck(broadcastId, ringName, NodeIdRange::Full, error);
            },
            AsyncOperationSPtr());
    }
    if (completeRequest)
    {
        requestContext->Reply(FederationMessage::GetBroadcastAck().CreateMessage());
    }

    if (dispatch)
    {
        DispatchBroadcastMessage(message, header);
    }

    return true;
}

void BroadcastManager::DispatchReplyMessage(__in Message & reply, MessageId const & broadcastId, NodeIdRange const & range)
{
    BroadcastReplyContextSPtr context;
    if (this->requestTable_.TryGet(broadcastId, context))
    {
        WriteNoise(
            TraceReceive,
            "Got a Broadcast reply at node: {0} for message {1}({2}) with RelatesTo of {3}",
            this->siteNode_.Instance,
            reply.MessageId,
            reply.RetryCount,
            broadcastId);

        context->OnReply(reply.Clone(), range);
    }
    else
    {
        WriteNoise(
            TraceDrop,
            "Broadcast dropping message: reply at node {0} for message {1}({2}) with RelatesTo of {3} because context has been cancelled or completed",
            this->siteNode_.Instance,
            reply.MessageId,
            reply.RetryCount,
            broadcastId);
    }
}

void BroadcastManager::OnPToPOneWayMessage(__in MessageUPtr & message, OneWayReceiverContextUPtr & oneWayReceiverContext)
{
    WriteNoise(
        TraceReceive,
        "Received a Broadcast message at node: {0} for message {1}({2}), hopped from {3}, FaultValue = {4}",
        this->siteNode_.Instance,
        message->MessageId,
        message->RetryCount,
        oneWayReceiverContext->From,
        message->FaultErrorCodeValue);

    Common::ErrorCode completionCode(message->FaultErrorCodeValue);

    if (completionCode.IsSuccess())
    {
        this->ProcessBroadcastMessage(message, oneWayReceiverContext->From);
    }
    else
    {
        WriteInfo(TraceReceive, "Broadcast ignored message at node: {0} for message {1}({2}), hopped from {3}, FaultValue = {4}",
            this->siteNode_.Instance,
            message->MessageId,
            message->RetryCount,
            oneWayReceiverContext->From,
            message->FaultErrorCodeValue);
    }

    oneWayReceiverContext->Accept();
}

void BroadcastManager::OnPToPRequestMessage(__in MessageUPtr &, RequestReceiverContextUPtr &)
{
    Assert::CodingError("We shouldn't be here");
}

void BroadcastManager::OnRoutedBroadcastMessage(__in MessageUPtr & message, BroadcastHeader && header, RequestReceiverContextUPtr & requestReceiverContext)
{
    auto from = requestReceiverContext->From;

    if (!header.ExpectsAck)
    {
        AcquireWriteLock grab(lock_);
        this->broadcastMessagesAlreadySeen_.TryAdd(header.BroadcastId);
    }

    message->Headers.TryRemoveHeader<ExpectsReplyHeader>();

    WriteNoise(
        TraceReceive,
        "Received a routed Broadcast message at node: {0} for message {1}({2}), hopped from {3}",
        this->siteNode_.Instance,
        header.BroadcastId,
        message->RetryCount,
        from);

    this->ForwardAndDispatch(message, requestReceiverContext->From, header, move(requestReceiverContext));
}

void BroadcastManager::RouteBroadcastMessageToHole(MessageUPtr && message, MessageId const & broadcastId, NodeIdRange nodeIdRange, TimeSpan timeout)
{
    ASSERT_IF(nodeIdRange.IsEmpty, "can't target an empty range");

    message->Headers.Add(BroadcastRangeHeader(nodeIdRange));

    // If range is full, we can route to any NodeId
    auto midpointDistance = (nodeIdRange.End.IdValue - nodeIdRange.Begin.IdValue) >> 1; // get midpoint
    auto midpoint = NodeId(nodeIdRange.Begin.IdValue + midpointDistance);

    WriteInfo(
        TraceRoute,
        "Broadcast routing message with BroadcastId {0} from node {1} to midpoint {2} of range {3}",
        this->siteNode_.Instance,
        broadcastId,
        midpoint,
        nodeIdRange);

    ASSERT_IFNOT(message->Idempotent, "Message must be idempotent to be a broadcast message)");

    this->siteNode_.BeginRouteRequest(std::move(message), midpoint, 0, false, timeout,
        [this, broadcastId, midpoint](AsyncOperationSPtr const & operation) -> void
        {
            MessageUPtr reply;
            auto errorCode = this->siteNode_.EndRouteRequest(operation, reply);

            if (errorCode.IsSuccess())
            {
                ASSERT_IFNOT(reply, "reply was empty in broadcast route callback");

                BroadcastRangeHeader rangeHeader;
                if (reply->Headers.TryGetAndRemoveHeader(rangeHeader))
                {
                    this->DispatchReplyMessage(*reply, broadcastId, rangeHeader.Range);
                }
                else
                {
                    WriteError(
                        TraceRoute,
                        "Broadcast route of message with BroadcastId {0} failed from node {1} to midpoint {2} with no BroadcastRangeHeader.  Reply MessageId = {3}",
                        broadcastId,
                        this->siteNode_.Instance,
                        midpoint,
                        reply->MessageId);

                    Assert::CodingError("No broadcast header on reply {0}", reply->MessageId);
                }
            }
            else
            {
                WriteInfo(
                    TraceRoute,
                    "Broadcast route of message with BroadcastId {0} failed from node {1} to midpoint {2} with error {3}",
                    broadcastId,
                    this->siteNode_.Instance,
                    midpoint,
                    errorCode.ReadValue());
            }
        },
        AsyncOperationSPtr());
}

void BroadcastManager::ProcessBroadcastAck(MessageId const & broadcastId, wstring const & ringName, NodeIdRange range, ErrorCode error)
{
    AcquireWriteLock grab(lock_);
    auto it = reliableBroadcastContexts_.find(broadcastId);
    if (it == reliableBroadcastContexts_.end())
    {
        WriteWarning(TraceDrop,
            "Ignore ACK for {0} range {1} on {2} as context can not be found",
            broadcastId, range, this->siteNode_.Id);
        return;
    }

    if (error.IsSuccess())
    {
        if (ringName.empty())
        {
            it->second.CompleteRange(range);
            WriteInfo(TraceRange, "Context {0} completed sub range {1}",
                it->second, range);
        }
        else
        {
            it->second.CompleteRing(ringName);
            WriteInfo(TraceRange, "Context {0} completed external ring {1}",
                it->second, ringName);
        }
    }
    else
    {
        if (ringName.empty())
        {
            WriteWarning(TraceRange, "Context {0} failed for sub range {1} with error {2}",
                it->second, range, error);
        }
        else
        {
            WriteWarning(TraceRange, "Context {0} failed for external ring {1} with error {2}",
                it->second, ringName, error);
        }

        it->second.SetError(error);
        reliableBroadcastContexts_.erase(it);
    }
}

void BroadcastManager::ProcessBroadcastLocalAck(MessageId const & broadcastId, ErrorCode error)
{
    AcquireWriteLock grab(lock_);

    auto it = reliableBroadcastContexts_.find(broadcastId);

    ASSERT_IF(it == reliableBroadcastContexts_.end(),
        "context not found for {0} on {1}",
        broadcastId, siteNode_.Id);

    if (error.IsSuccess())
    {
        it->second.CompleteLocalRange();

        WriteInfo(TraceRange, "Context {0} processed local ACK", it->second);
    }
    else
    {
        WriteWarning(TraceRange, "Context {0} failed for local ACK with {1}", it->second, error);

        it->second.SetError(error);
        reliableBroadcastContexts_.erase(it);
    }
}

void BroadcastManager::Stop()
{
    auto itemsToCancel = this->requestTable_.Close();

    WriteInfo(
        TraceCancel,
        "Cancel all pending requests at {0}",
        this->siteNode_.Instance);

    // Cancel all outstanding requests to cleanup timers and remove callbacks that have captured shared_ptr's
    for (auto pair : itemsToCancel)
    {
        auto broadcastReplyContext = static_cast<BroadcastReplyContext*>(pair.second.get());
        broadcastReplyContext->Cancel();
    }

    {
        AcquireWriteLock grab(lock_);
        for (auto it = reliableBroadcastContexts_.begin(); it != reliableBroadcastContexts_.end(); ++it)
        {
            it->second.SetError(ErrorCodeValue::OperationCanceled);
        }

        reliableBroadcastContexts_.clear();

        closed_ = true;
    }

    timer_->Cancel();
}
