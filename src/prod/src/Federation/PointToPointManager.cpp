// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "PointToPointEventSource.h"

using namespace Federation;
using namespace Transport;
using namespace Common;
using namespace std;

PointToPointEventSource Events;

StringLiteral const TraceDrop("Drop");
StringLiteral const TraceStringMessage("Message");
StringLiteral const TraceResend("Resend");
StringLiteral const TraceFault("Fault");
StringLiteral const TraceRegister("Register");

class P2PRequestAsyncOperation : public RequestAsyncOperation
{
public:
    P2PRequestAsyncOperation(
        NodeInstance const & target,
        wstring const & ringName,
        RequestTable & owner,
        Transport::MessageId const & messageId,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        :   RequestAsyncOperation(owner, messageId, timeout, callback, parent),
            Target(target),
            RingName(ringName)
    {
    }

    NodeInstance Target;
    wstring RingName;
};

PointToPointManager::PointToPointManager(__in SiteNode & siteNode)
    : siteNode_(siteNode)
    , applicationActorMapClosed_(false)
    , loopbackDispatcher_(wformatString("{0}_loopback", siteNode.Id), siteNode, false /* forceEnqueue */, FederationConfig::GetConfig().LoopbackJobQueueThreadMax)
{
    loopbackDispatcher_.SetExtraTracing(FederationConfig::GetConfig().TraceLoopbackJobQueue);

    this->pointToPointActorMap_[PToPActor::Direct] = MessageHandlerPair(
        [&](MessageUPtr & message, OneWayReceiverContextUPtr & oneWayReceiverContext){ this->ActorDispatchOneWay(message, oneWayReceiverContext); },
        [&](MessageUPtr & message, RequestReceiverContextUPtr & requestReceiverContext){ this->ActorDispatchRequest(message, requestReceiverContext); },
        true);

    // TODO: Make Federation use the PToP actor map
    //this->pointToPointActorMap_[PToPActor::Federation] = 
    //    [&](OneWayReceiverContext & oneWayReceiverContext){ FederationOneWayMessageHandler(oneWayReceiverContext); },
    //    [&](RequestReceiverContext & requestReceiverContext){ FederationRequestMessageHandler(requestReceiverContext); });

    this->pointToPointActorMap_[PToPActor::Routing] = MessageHandlerPair(
        [&](MessageUPtr &, OneWayReceiverContextUPtr &){ Assert::CodingError("Routed message dispatched to a oneway handler"); },
        [&](MessageUPtr & message, RequestReceiverContextUPtr & requestReceiverContext){ this->siteNode_.routingManager_->PreProcessRoutedMessages(*message, requestReceiverContext); },
        true);

    this->pointToPointActorMap_[PToPActor::Broadcast] = MessageHandlerPair(
        [&](MessageUPtr & message, OneWayReceiverContextUPtr & oneWayReceiverContext)
        {
            this->siteNode_.broadcastManagerUPtr_->OnPToPOneWayMessage(message, oneWayReceiverContext);
        },
        [&](MessageUPtr & message, RequestReceiverContextUPtr & requestReceiverContext)
        {
            this->siteNode_.broadcastManagerUPtr_->OnPToPRequestMessage(message, requestReceiverContext);
        },
        true);
}

PointToPointManager::~PointToPointManager()
{
}

void PointToPointManager::Stop()
{
    {
        Common::AcquireWriteLock lock(this->applicationActorMapEntryLock_);
        this->applicationActorMap_.clear();
        this->applicationActorMapClosed_ = true;
    }

    this->requestTable_.Close();
    this->loopbackDispatcher_.Close();
}

void PointToPointManager::AddPToPHeaders(__inout Message & message, PartnerNodeSPtr const & to, bool exactInstance, PToPActor::Enum actor) const
{
    bool isToRemoteRing = (to->RingName != siteNode_.RingName);
    if (to->Id != siteNode_.Id || isToRemoteRing)
    {
        this->siteNode_.Table.AddNeighborHeaders(message, isToRemoteRing);
    }

    message.Headers.Add(PToPHeader(this->siteNode_.Instance, to->Instance, actor, siteNode_.RingName, to->RingName, exactInstance));
}

void PointToPointManager::RemovePToPHeaders(__inout Message & message)
{
    message.Headers.TryRemoveHeader<PToPHeader>();

    if (message.ExpectsReply)
    {
        message.Headers.TryRemoveHeader<ExpectsReplyHeader>();
    }
}

bool PointToPointManager::ProcessPToPHeaders(
    __in Message & message,
    __out PToPHeader & header,
    __out PartnerNodeSPtr & source)
{
    if (!message.Headers.TryReadFirst(header))
    {
        return false;
    }

    if (FederationMessage::IsFederationMessage(message))
    {
        bool instanceMatched = (siteNode_.IsRingNameMatched(header.ToRing) && (header.To == this->siteNode_.Instance));
        source = siteNode_.Table.ProcessNeighborHeaders(message, header.From, header.FromRing, instanceMatched);
    }
    else
    {
        source = siteNode_.Table.ProcessNodeHeaders(message, header.From, header.FromRing);
    }

    if (!source)
    {
        return false;
    }

    if (source->Instance == header.From)
    {
        source->OnReceive(message.ExpectsReply);
    }

    source->UpdateLastAccess(Stopwatch::Now());

    return true;
}

bool PointToPointManager::GetFromInstance(__inout Transport::Message & message, NodeInstance & fromInstance)
{
    PToPHeader header;
    if (!message.Headers.TryReadFirst(header))
    {
        return false;
    }

    fromInstance = header.From;

    return true;
}

void PointToPointManager::CancelPToPRequest(Transport::MessageId const & messageId)
{
    RequestAsyncOperationSPtr operation;
    if (this->requestTable_.TryRemoveEntry(messageId, operation))
    {
        operation->Cancel();
    }
}

// Will send a request message again without updating the context table.  RetryHeader will be added.
void PointToPointManager::RetryPToPSendRequest(MessageUPtr && request, PartnerNodeSPtr const & to, bool exactInstance, PToPActor::Enum actor, LONG retryNumber)
{
    // TODO: we could check to make sure the request is still in the context table before sending
    // TODO: all routing headers should already be added but we could do checks
    WriteInfo(TraceResend, request->Action,
        "{0} resending request: {1}, retry count: {2}",
        this->siteNode_.Id, request->TraceId(), retryNumber);

    request->Headers.Add(ExpectsReplyHeader(true));
    request->Headers.Add(RetryHeader(retryNumber));

    this->PToPSend(std::move(request), to, exactInstance, actor);
}

bool AllowedWithoutLease(wstring const & action)
{
    return (action == FederationMessage::GetArbitrateRequest().Action ||
            action == FederationMessage::GetArbitrateReply().Action ||
            action == FederationMessage::GetExtendedArbitrateRequest().Action ||
            action == FederationMessage::GetDelayedArbitrateReply().Action ||
            action == FederationMessage::GetDepart().Action);
}

void PointToPointManager::PToPSend(MessageUPtr && message, PartnerNodeSPtr const & to, bool exactInstance, PToPActor::Enum actor) const
{
    if (!AllowedWithoutLease(message->Action) && this->siteNode_.IsLeaseExpired())
    {
        WriteWarning(TraceDrop, message->Action,
            "{0} dropping outoing message {1} with id {2} as lease is expired",
            this->siteNode_.Id, message->Action, message->TraceId());
        return;
    }

    if (message->MessageId.IsEmpty())
    {
        message->Headers.Add(MessageIdHeader());
    }

    this->AddPToPHeaders(*message, to, exactInstance, actor);

    // PartnerNodes in the Booting phase were built by an IVoteProxy
    // and not retrieved from the routing table
    ISendTarget::SPtr const & targetSPtr = to->Target;
    if (targetSPtr)
    {
        Events.Send(
            message->Action,
            this->siteNode_.IdString,
            to->Instance,
            message->TraceId(),
            message->IsReply(),
            message->RetryCount);

        if (to->Instance == siteNode_.Instance && to->RingName == siteNode_.RingName)
        {
            TimeSpan delay;            
            if (!UnreliableTransportConfig::GetConfig().IsDisabled())
            {
                delay = UnreliableTransportConfig::GetConfig().GetDelay(siteNode_.IdString, siteNode_.IdString, *message);
            }
            else
            {
                delay = TimeSpan::Zero;
            }

            if (delay == TimeSpan::MaxValue)
            {
                WriteInfo(TraceDrop, message->Action,
                    "{0} unreliable channel dropping message {1} to {2} id {3}({4})",
                    this->siteNode_.Id, message->Action, to->Instance, message->TraceId(), message->RetryCount);
            }
            else if (delay == TimeSpan::Zero)
            {
                JobItem ji(move(message));
                loopbackDispatcher_.Enqueue(move(ji));
            }
            else
            {
                auto root = siteNode_.GetSiteNodeSPtr();
                MoveUPtr<Message> mover(std::move(message));
                Threadpool::Post(
                    [root, mover] () mutable 
                    {
                        MessageUPtr message = mover.TakeUPtr();
                        root->GetPointToPointManager().ProcessIncomingTransportMessages(message, nullptr); },
                    delay);
            }

            return;
        }

        if(this->siteNode_.Channel)
        {
            to->OnSend(message->ExpectsReply);
            this->siteNode_.Channel->SendOneWay(targetSPtr, std::move(message));
        }
        else
        {
            WriteWarning(TraceDrop, message->Action,
                "{0} dropping message {1} to {2} id {3}({4}) as channel does not exist",
                this->siteNode_.Id, message->Action, to->Instance, message->TraceId(), message->RetryCount);
        }
    }
    else
    {
        WriteWarning(TraceDrop, message->Action,
            "{0} P2P dropping message to {1} as it is already down",
            this->siteNode_.Id, to->Instance);
    }
}

// MessageId added by SiteNode
void PointToPointManager::PToPMulticastSend(
    MessageUPtr && message,
    vector<PartnerNodeSPtr> const & targets,
    MessageHeadersCollection && headersCollection,
    PToPActor::Enum actor)
{
    this->siteNode_.Table.AddNeighborHeaders(*message, false);

    ASSERT_IFNOT(message->MessageId.IsEmpty(), "mutlticast must add the id");

    for (size_t i = 0; i < targets.size(); ++i)
    {
        headersCollection[i].Add(PToPHeader(this->siteNode_.Instance, targets[i]->Instance, actor, siteNode_.RingName, targets[i]->RingName, true));
        headersCollection[i].Add(MessageIdHeader());
    }

    vector<NamedAddress> addresses(targets.size());
    transform(targets.begin(), targets.end(), addresses.begin(),
        [](PartnerNodeSPtr const & partner) -> NamedAddress
    {
        return NamedAddress(partner->Address, partner->Id.ToString());
    }
    );

    size_t failedResolves = 0;
    auto resolvedTargets = this->siteNode_.multicastChannelUPtr_->Resolve(addresses.begin(), addresses.end(), failedResolves);

    if (failedResolves != 0)
    {
        // Failing to resolve a target means the node/transport is shutting down and we don't need to send
        WriteInfo(
            TraceFault,
            "{0} Broadcast failed to resolve {1} addresses out of {2}",
            this->siteNode_.Id, failedResolves, addresses.size());
    }
    else
    {
        this->siteNode_.multicastChannelUPtr_->Send(resolvedTargets, std::move(message), std::move(headersCollection));
    }
}

shared_ptr<AsyncOperation> PointToPointManager::BeginPToPSendRequest(
    MessageUPtr && request,
    PartnerNodeSPtr const & to,
    bool exactInstance,
    PToPActor::Enum actor,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent) const
{
    ASSERT_IF(request == nullptr, "Request can't be null");

    MessageId messageId = request->MessageId;

    ExpectsReplyHeader header;
    if (!request->Headers.TryReadFirst(header))
    {
        request->Headers.Add(ExpectsReplyHeader(true));
    }
    else
    {
        ASSERT_IFNOT(request->ExpectsReply, "Message was marked to not receive a reply but send as a request");
    }

    if (request->MessageId.IsEmpty())
    {
        request->Headers.Add(MessageIdHeader());
    }

    SiteNodeSPtr localSiteNodeSPtr = const_pointer_cast<SiteNode>(this->siteNode_.GetSiteNodeSPtr());

    shared_ptr<RequestAsyncOperation> requestAsyncOperation(new P2PRequestAsyncOperation(
            to->Instance,
            to->RingName,
            this->requestTable_,
            request->MessageId,
            timeout,
            callback,
            parent));
    requestAsyncOperation->Start(requestAsyncOperation);

    auto error = this->requestTable_.TryInsertEntry(request->MessageId, requestAsyncOperation);
    if (error.IsSuccess())
    {
        if (requestAsyncOperation->IsCompleted)
        {
            // If there was a race and we added to the table after OnComplete ran,
            // we must remove it again since it will not itself.
            this->requestTable_.TryRemoveEntry(request->MessageId, requestAsyncOperation);
        }
        else
        {
            this->PToPSend(std::move(request), to, exactInstance, actor);
        }
    }
    else
    {
        requestAsyncOperation->TryComplete(requestAsyncOperation, error);
    }

    return requestAsyncOperation;
}

ErrorCode PointToPointManager::EndPToPSendRequest(shared_ptr<AsyncOperation> const & operation, __out MessageUPtr & message) const
{
    ErrorCode error = RequestAsyncOperation::End(operation, message);
    if (error.IsError(ErrorCodeValue::P2PNodeDoesNotMatchFault))
    {
        P2PRequestAsyncOperation* requestOperation = dynamic_cast<P2PRequestAsyncOperation*>(operation.get());
        WriteInfo(
            "Fault",
            "{0} processed P2PNodeDoesNotMatchFault for {1}",
            siteNode_.Id,
            requestOperation->Target);

        siteNode_.Table.SetShutdown(requestOperation->Target, requestOperation->RingName);
    }

    return error;
}

void PointToPointManager::ProcessIncomingTransportMessages(__in MessageUPtr & message, ISendTarget::SPtr const & replyTargetSPtr)
{
    UNREFERENCED_PARAMETER(replyTargetSPtr);

    if (!message->IsValid)
    {
        WriteWarning(TraceDrop, message->Action,
            "{0} dropping message {1} with id {2} as status is invalid: {3}",
            this->siteNode_.IdString, message->Action, message->TraceId(), message->Status);
        return;
    }

    if (!AllowedWithoutLease(message->Action) && this->siteNode_.IsLeaseExpired())
    {
        WriteWarning(TraceDrop, message->Action,
            "{0} dropping message {1} with id {2} as lease is expired",
            this->siteNode_.IdString, message->Action, message->TraceId());
        return;
    }

    WriteNoise(
        TraceStringMessage,
        message->Action,
        "{0} received message {1}",
        this->siteNode_.IdString, 
        message);

    PToPHeader header;
    PartnerNodeSPtr from;

    if (!ProcessPToPHeaders(*message, header, from))
    {
        WriteWarning(TraceDrop, message->Action,
            "{0} received message without 'from' or 'to' header or message corrupted.  Dropping message {1}",
            this->siteNode_.IdString, message->TraceId());
        return;
    }

    if (!message->IsValid)
    {
        WriteWarning(TraceDrop, message->Action,
            "{0} dropping message {1} with id {2} as status is invalid: {3}",
            this->siteNode_.IdString, message->Action, message->TraceId(), message->Status);
        return;
    }

    NodeInstance to = header.To;

    ASSERT_IF(!from, "from must be valid at this point");

    Events.Receive(
        message->Action,
        this->siteNode_.IdString,
        header.From,
        message->TraceId(),
        message->IsReply(),
        message->RetryCount);

    // Instance is not important for voter protocol messages
    if ((!message->Idempotent && header.ExactInstance && !this->siteNode_.Instance.Match(to)) ||
        (this->siteNode_.Id != to.Id) ||
        !siteNode_.IsRingNameMatched(header.ToRing))
    {
        if (message->ExpectsReply)
        {
            NodeDoesNotMatchFaultBody body(to, header.ToRing);
            // TODO: refactor to SendFederationFaultMethod

            auto faultMessage = FederationMessage::GetNodeDoesNotMatchFault().CreateMessage(body);
            faultMessage->Headers.Add(RelatesToHeader(message->MessageId));
            faultMessage->Headers.Add(FaultHeader(ErrorCodeValue::P2PNodeDoesNotMatchFault));

            this->PToPSend(std::move(faultMessage), from, true, PToPActor::Direct);
        }
        else if (message->Actor == FederationMessage::Actor)
        {
            NodeDoesNotMatchFaultBody body(to, header.ToRing);
            auto faultMessage = FederationMessage::GetNodeDoesNotMatchFault().CreateMessage(body);

            faultMessage->Headers.Add(FaultHeader(ErrorCodeValue::P2PNodeDoesNotMatchFault));
            this->PToPSend(std::move(faultMessage), from, true, PToPActor::Direct);
        }
        else if (!message->RelatesTo.IsEmpty())
        {
            // reply, drop it
            WriteInfo(TraceDrop, message->Action,
                "{0} dropping reply message {1} targeted to {2}",
                this->siteNode_.IdString, message->TraceId(), to);
        }
        else
        {
            WriteInfo(TraceDrop, message->Action,
                "{0} dropping message {1} targeted to {2}",
                this->siteNode_.IdString, message->TraceId(), to);
        }
    }
    else if (!message->RelatesTo.IsEmpty())
    {
        if (!this->requestTable_.OnReplyMessage(*message))
        {
            WriteInfo(TraceDrop, message->Action,
                "{0} dropping reply message {1} as context not found", 
                this->siteNode_.IdString, message->TraceId());
        }
    }
    else
    {
        // This is a regular oneway or request message
        this->DispatchOneWayAndRequestMessages(message, from, header.From, header.Actor);
    }
}

void PointToPointManager::DispatchOneWayAndRequestMessages(__in MessageUPtr & message, PartnerNodeSPtr const & from, NodeInstance const & fromInstance, PToPActor::Enum actor)
{
    // Find the handler pair for the p2p actor
    if (actor < 0 || PToPActor::UpperBound <= actor)
    {
        WriteWarning(TraceDrop, message->Action,
            "{0} incoming message actor is not valid: {1} - {2}",
            this->siteNode_.Id, actor, message->TraceId());
        return;
    }

    MessageHandlerPair handlerPair = this->pointToPointActorMap_[actor];

    // Check if the incoming message expects a reply or not
    if (message->ExpectsReply)
    {
        auto context = make_unique<RequestReceiverContext>(this->siteNode_.GetSiteNodeSPtr(), from, fromInstance, message->MessageId);
        if (!handlerPair.HandleRequestMessage(message, context))
        {
            WriteInfo(TraceDrop, message->Action,
                "{0} incoming message does not have a P2P request handler: {1}",
                this->siteNode_.Id, message->TraceId());
        }
    }
    else
    {
        auto context = make_unique<OneWayReceiverContext>(from, fromInstance, message->MessageId);
        if (!handlerPair.HandleOneWayMessage(message, context))
        {
            WriteInfo(TraceDrop, message->Action,
                "{0} incoming message does not have a P2P one way handler: {1}",
                this->siteNode_.Id, message->TraceId());
        }
    }
}

void PointToPointManager::ActorDispatchOneWay(__inout MessageUPtr & message, __inout OneWayReceiverContextUPtr & receiverContext)
{
    MessageHandlerPair handlerPair;

    bool actorIsRegistered = false;
    bool messageWasDropped = true;
    bool foundHandler = false;

    {
        Common::AcquireReadLock lock(this->applicationActorMapEntryLock_);

        auto findIter = this->applicationActorMap_.find(message->Actor);
        actorIsRegistered = (findIter != this->applicationActorMap_.end());

        if (actorIsRegistered)
        {
            for (auto & item : findIter->second.handlers_)
            {
                if (item.first == nullptr || item.first->Match(*message))
                {
                    // If the filter was null, it is a match all
                    foundHandler = true;
                    handlerPair = item.second;

                    break;
                }
            }
        }
    }

    if (foundHandler)
    {
        MessageUPtr clone = message->Clone();
        messageWasDropped = !handlerPair.HandleOneWayMessage(clone, receiverContext);
    }

    if (!actorIsRegistered || messageWasDropped)
    {
        WriteInfo(
            TraceDrop,
            message->Action,
            "{0} incoming message does not have a one way handler: {1}",
            this->siteNode_.Id,
            message->Actor);

        receiverContext->Reject(ErrorCode(ErrorCodeValue::MessageHandlerDoesNotExistFault));
    }
}

void PointToPointManager::ActorDispatchRequest(__inout MessageUPtr & message, __inout RequestReceiverContextUPtr & requestReceiverContext)
{
    MessageHandlerPair handlerPair;

    bool actorIsRegistered = false;
    bool messageWasDropped = true;
    bool foundHandler = false;

    {
        Common::AcquireReadLock lock(this->applicationActorMapEntryLock_);

        auto findIter = this->applicationActorMap_.find(message->Actor);
        actorIsRegistered = (findIter != this->applicationActorMap_.end());

        if (actorIsRegistered)
        {
            for (auto & item : findIter->second.handlers_)
            {
                if (item.first == nullptr || item.first->Match(*message))
                {
                    // If the filter was null, it is a match all
                    foundHandler = true;
                    handlerPair = item.second;

                    break;
                }
            }
        }
    }

    if (foundHandler)
    {
        MessageUPtr clone = message->Clone();
        messageWasDropped = !handlerPair.HandleRequestMessage(clone, requestReceiverContext);
    }

    if (!actorIsRegistered || messageWasDropped)
    {
        WriteInfo(
            TraceDrop,
            message->Action,
            "{0} incoming message does not have a request handler: {1}",
            this->siteNode_.Id,
            message->Actor);

        requestReceiverContext->InternalReject(ErrorCode(ErrorCodeValue::MessageHandlerDoesNotExistFault));
    }
}

void PointToPointManager::RegisterMessageHandler(
    Transport::Actor::Enum actor,
    OneWayMessageHandler const & oneWayMessageHandler,
    RequestMessageHandler const & requestMessageHandler,
    bool dispatchOnTransportThread,
    IMessageFilterSPtr && filter)
{
    MessageHandlerPair handlerPair(oneWayMessageHandler, requestMessageHandler, dispatchOnTransportThread);

    bool addingMultipleActorHandlers = false;

    {
        Common::AcquireWriteLock lock(this->applicationActorMapEntryLock_);
        if (this->applicationActorMapClosed_)
        {
            return;
        }

        auto findIter = this->applicationActorMap_.find(actor);

        if (findIter == this->applicationActorMap_.end())
        {
            // This will be the first entry
            ActorMapEntry entry;
            entry.handlers_.push_back(std::make_pair(filter, handlerPair));
            this->applicationActorMap_.insert(make_pair(actor, entry));
        }
        else
        {
            // One entry exists already, so we can add to it
            findIter->second.handlers_.push_back(std::make_pair(filter, handlerPair));
            addingMultipleActorHandlers = true;
        }
    }

    if (addingMultipleActorHandlers)
    {
        WriteInfo(
            TraceRegister,
            "Registering multiple handlers for {1} (2)",
            actor,
            static_cast<void*>(filter.get()));
    }
}

bool PointToPointManager::UnRegisterMessageHandler(Transport::Actor::Enum actor, IMessageFilterSPtr const & filter)
{
    Common::AcquireWriteLock lock(this->applicationActorMapEntryLock_);

    bool removedHandler = false;

    if (this->applicationActorMapClosed_)
    {
        return removedHandler;
    }

    auto findIter = this->applicationActorMap_.find(actor);

    if (findIter != this->applicationActorMap_.end())
    {
        for (auto iter = findIter->second.handlers_.begin(); iter != findIter->second.handlers_.end(); ++iter)
        {
            // iter is a pair of a filter and handler
            if (iter->first == filter)
            {
                findIter->second.handlers_.erase(iter);
                removedHandler = true;

                // the iterator is no longer stable after the deletion
                break;
            }
        }

        if (findIter->second.handlers_.empty())
        {
            // If the handlers list is now 0, remove the whole entry
            this->applicationActorMap_.erase(findIter);
        }
    }

    return removedHandler;
}

PointToPointManager::JobItem::JobItem()
{
}

PointToPointManager::JobItem::JobItem(MessageUPtr && message)
: message_(move(message))
{
}

PointToPointManager::JobItem::JobItem(JobItem && other)
: message_(move(other.message_))
{
}

PointToPointManager::JobItem& PointToPointManager::JobItem::operator=(JobItem && other)
{
    if (this != &other)
    {
        message_ = move(other.message_);
    }

    return *this;
}

bool PointToPointManager::JobItem::ProcessJob(SiteNode & siteNode)
{
    ASSERT_IF(message_ == nullptr, "Message cannot be null");
    siteNode.GetPointToPointManager().ProcessIncomingTransportMessages(message_, nullptr);
    return true;
}
