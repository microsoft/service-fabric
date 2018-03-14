// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Federation;
using namespace Transport;
using namespace Common;
using namespace std;

StringLiteral const MulticastTimerTag("Multicast");

StringLiteral const TraceStart("Start");
StringLiteral const TraceForward("Forward");
StringLiteral const TraceDrop("Drop");
StringLiteral const TraceFault("Fault");

MulticastManager::MulticastManager(SiteNode & siteNode)
    :   siteNode_(siteNode),
        multicastContexts_(FederationConfig::GetConfig().BroadcastContextKeepDuration),
        closed_(false)
{
    SiteNodeSPtr siteNodeSPtr = siteNode.GetSiteNodeSPtr();
    timer_ = Timer::Create(
        MulticastTimerTag,
        [this, siteNodeSPtr] (TimerSPtr const &)
        {
            this->OnTimer();
        },
        true);

    TimeSpan interval = FederationConfig::GetConfig().BroadcastContextKeepDuration;
    timer_->Change(interval, interval);
}

MulticastManager::~MulticastManager()
{
}

void MulticastManager::OnTimer()
{
    AcquireWriteLock grab(lock_);
    this->multicastContexts_.RemoveExpiredEntries();
}

MulticastHeader MulticastManager::AddMulticastHeaders(Message & message) const
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

    auto header = MulticastHeader(this->siteNode_.Instance, id);

    message.Headers.Add(header);

    return header;
}

AsyncOperationSPtr MulticastManager::BeginMulticast(
    MessageUPtr && message,
    vector<NodeInstance> const & targets,
    TimeSpan retryTimeout,
    TimeSpan timeout,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
{
    bool includeLocal = false;
    vector<NodeInstance> nodes;

    for (auto it = targets.begin(); it != targets.end(); ++it)
    {
        if (*it == siteNode_.Instance)
        {
            includeLocal = true;
        }
        else
        {
            nodes.push_back(*it);
        }
    }

    sort(nodes.begin(), nodes.end());

    auto header = this->AddMulticastHeaders(*message);
    WriteInfo(
        TraceStart,
        "Multicast started for {0} on {1}",
        header, siteNode_.Id);

    MulticastOperationSPtr operation = make_shared<MulticastOperation>(callback, parent);
    operation->Start(operation);
    if (!MulticastWithAck(message, header, move(nodes), includeLocal, retryTimeout, timeout, nullptr, operation))
    {
        operation->TryComplete(operation, ErrorCodeValue::ObjectClosed);
    }

    return operation;
}

ErrorCode MulticastManager::EndMulticast(
    AsyncOperationSPtr const & operation, 
    __out vector<NodeInstance> & failedTargets,
    __out std::vector<NodeInstance> & pendingTargets)
{
    auto multicastOperation = AsyncOperation::End<MulticastOperation>(operation);
    multicastOperation->MoveResults(failedTargets, pendingTargets);

    return multicastOperation->Error;
}

vector<MulticastForwardContext::SubTree> MulticastManager::BuildSubtrees(vector<NodeInstance> const & targets)
{
    vector<MulticastForwardContext::SubTree> result;

    FederationConfig const & config = FederationConfig::GetConfig();
    int total = static_cast<int>(targets.size());
    if (total == 0)
    {
        return result;
    }

    int subTreeCount = min(config.BroadcastPropagationFactor, total);
    int nodePerTree = total / subTreeCount;
    int biggerTreeCount = total - nodePerTree * subTreeCount;

    for (int i = 0, j = 0; i < subTreeCount; i++)
    {
        int size = nodePerTree;
        if (biggerTreeCount-- > 0)
        {
            size++;
        }

        int rootIndex = j + (size - 1) / 2;

        MulticastForwardContext::SubTree subTree;
        subTree.Targets.reserve(size);
        while (size--)
        {
            if (j == rootIndex)
            {
                subTree.Root = targets[j];
            }
            else
            {
                subTree.Targets.push_back(targets[j]);
            }
            j++;
        }

        result.push_back(move(subTree));
    }

    return result;
}

void MulticastManager::ForwardMessage(
    MessageUPtr && message,
    MessageId const & multicastId,
    vector<MulticastForwardContext::SubTree> const & subTrees,
    TimeSpan retryTimeout,
    TimeSpan timeout)
{
    // Every hop gets less time
    if (timeout > retryTimeout)
    {
        timeout = timeout - retryTimeout;
    }

    for (size_t i = 0; i < subTrees.size(); i++)
    {
        NodeInstance target = subTrees[i].Root;
        MessageUPtr forwardMessage = (i == subTrees.size() - 1 ? move(message) : message->Clone());
        forwardMessage->Headers.Add(MulticastTargetsHeader(subTrees[i].Targets));

        StopwatchTime expireTime = Stopwatch::Now() + timeout;
        this->siteNode_.BeginRouteRequest(
            std::move(forwardMessage), 
            target.Id,
            target.InstanceId, 
            true,
            retryTimeout,
            timeout,
            [this, multicastId, target, retryTimeout, expireTime](AsyncOperationSPtr const & operation)
            {
                MulticastAckBody body;
                MessageUPtr reply;
                ErrorCode error = this->siteNode_.EndRouteRequest(operation, reply);
                if (error.IsSuccess())
                {
                    if (!reply->GetBody(body))
                    {
                        return;
                    }
                }

                vector<NodeInstance> failed;
                vector<NodeInstance> unknown;
                body.MoveResults(failed, unknown);

                this->ProcessMulticastAck(multicastId, target, move(failed), move(unknown), error, retryTimeout, expireTime - Stopwatch::Now());
            },
            AsyncOperationSPtr());
    }
}

bool MulticastManager::MulticastWithAck(
    MessageUPtr & message,
    MulticastHeader const & header,
    vector<NodeInstance> && targets,
    bool includeLocal,
    TimeSpan retryTimeout,
    TimeSpan timeout,
    RequestReceiverContextUPtr && requestContext,
    MulticastOperationSPtr const & operation)
{
    MessageId multicastId = header.MulticastId;

    vector<MulticastForwardContext::SubTree> subTrees;
    vector<NodeInstance> downTargets = siteNode_.getRoutingTable().RemoveDownNodes(targets);

    bool dispatch = includeLocal;
    MessageUPtr reply;
    {
        AcquireWriteLock grab(lock_);
        if (closed_)
        {
            return false;
        }

        auto it = multicastContexts_.find(multicastId);
        if (it == multicastContexts_.end())
        {
            subTrees = BuildSubtrees(targets);
            vector<MulticastForwardContext::SubTree> pending(subTrees);
            if (includeLocal)
            {
                MulticastForwardContext::SubTree localTree;
                localTree.Root = siteNode_.Instance;
                pending.push_back(move(localTree));
            }

            MessageUPtr clone = (subTrees.size() > 0 ? message->Clone() : nullptr);
            bool added = multicastContexts_.TryAdd(
                multicastId,
                MulticastForwardContext(siteNode_.Id, move(clone), multicastId, move(pending), operation, move(requestContext)));
            ASSERT_IF(!added, "Add multicast context failed");

            it = multicastContexts_.find(multicastId);
        }
        else
        {
            dispatch = false;

            if (it->second.RemoveDuplicateTargets(targets))
            {
                reply = it->second.CreateAckMessage();
            }
            else
            {
                subTrees = BuildSubtrees(targets);
                it->second.AddSubTrees(message, subTrees, move(requestContext));
            }
        }

        for (NodeInstance const & downTarget : downTargets)
        {
            it->second.AddFailedTarget(downTarget);
        }

        WriteInfo(
            TraceForward,
            "Forwarding multicast message {0} context: {1}, timeout: {2}",
            message->Action,
            it->second,
            timeout);
    }

    if (subTrees.size() > 0)
    {
        ForwardMessage(message->Clone(), multicastId, subTrees, retryTimeout, timeout);
    }

    if (reply)
    {
        requestContext->Reply(move(reply));
    }

    if (dispatch)
    {
        DispatchMulticastMessage(message, header);
    }

    return true;
}

void MulticastManager::ProcessMulticastAck(
    MessageId const & multicastId,
    NodeInstance const & target,
    vector<NodeInstance> && failedTargets,
    vector<NodeInstance> && unknownTargets,
    ErrorCode error,
    TimeSpan retryTimeout,
    TimeSpan timeout)
{
    RoutingTable & table = siteNode_.getRoutingTable();
    for (NodeInstance const & failedTarget : failedTargets)
    {
        table.SetShutdown(failedTarget, siteNode_.RingName);
    }

    MessageUPtr message;
    vector<MulticastForwardContext::SubTree> subTrees;
    {
        AcquireWriteLock grab(lock_);
        auto it = multicastContexts_.find(multicastId);
        if (it == multicastContexts_.end())
        {
            WriteWarning(TraceDrop,
                "Ignore ACK for {0} from {1} on {2} as context can not be found",
                multicastId, target, this->siteNode_.Id);
            return;
        }

        for (NodeInstance const & failedTarget : failedTargets)
        {
            it->second.AddFailedTarget(failedTarget);
        }

        for (NodeInstance const & unknownTarget : unknownTargets)
        {
            it->second.AddUnknownTarget(unknownTarget);
        }

        if (error.IsSuccess())
        {
            it->second.Complete(target);

            WriteInfo(TraceForward, "Context {0} got ACK from {1}",
                it->second, target);
        }
        else if (error.IsError(ErrorCodeValue::RoutingNodeDoesNotMatchFault))
        {
            WriteInfo(TraceForward, "Context {0} target {1} failed",
                it->second, target);

            if (timeout > TimeSpan::Zero)
            {
                message = it->second.FailTarget(target, subTrees);
                error = ErrorCode::Success();
            }
            else
            {
                error = ErrorCodeValue::Timeout;
            }
        }
        
        if (!error.IsSuccess())
        {
            WriteWarning(TraceForward, "Context {0} failed for {1} with error {2}",
                it->second, target, error);

            it->second.SetError(error);
            multicastContexts_.erase(it);
        }
    }

    if (message)
    {
        ForwardMessage(move(message), multicastId, subTrees, retryTimeout, timeout);
    }
}

void MulticastManager::ProcessMulticastLocalAck(MessageId const & contextId, ErrorCode error)
{
    ProcessMulticastAck(contextId, siteNode_.Instance, vector<NodeInstance>(), vector<NodeInstance>(), error,TimeSpan::Zero, TimeSpan::Zero);
}

void MulticastManager::OnRoutedMulticastMessage(
    MessageUPtr & message,
    MulticastHeader && header,
    RoutingHeader const & routingHeader,
    RequestReceiverContextUPtr & requestReceiverContext)
{
    auto from = requestReceiverContext->From;

    WriteNoise(
        "Receive",
        "Received multicast message at node: {0} for message {1}({2}), hopped from {3}",
        this->siteNode_.Instance,
        header.MulticastId,
        message->RetryCount,
        from);

    message->Headers.TryRemoveHeader<MessageIdHeader>();
    this->siteNode_.GetPointToPointManager().RemovePToPHeaders(*message);

    vector<NodeInstance> targets;
    MulticastTargetsHeader targetsHeader;
    if (message->Headers.TryGetAndRemoveHeader(targetsHeader))
    {
        targets = targetsHeader.MoveTargets();
    }

    MulticastWithAck(message, header, move(targets), true, routingHeader.RetryTimeout, routingHeader.Expiration, move(requestReceiverContext), nullptr);
}

bool MulticastManager::DispatchMulticastMessage(
    MessageUPtr & message,
    MulticastHeader const & header)
{
    ASSERT_IF(!message->MessageId.IsEmpty(), "MessageId not removed for {0}", *message);
    message->Headers.Add(MessageIdHeader(header.MulticastId));

    PartnerNodeSPtr from = this->siteNode_.Table.Get(header.From);
    if (!from)
    {
        WriteWarning(
            TraceFault,
            "Could not get the nodeId, {0}, from the routing table at node {1} for message multicast id {2}",
            header.From,
            this->siteNode_.Instance,
            header.MulticastId);
        return false;
    }
    
    OneWayReceiverContextUPtr context = make_unique<MulticastAckReceiverContext>(*this, header.MulticastId, from, header.From);

    this->siteNode_.GetPointToPointManager().ActorDispatchOneWay(message, context);

    return true;
}

void MulticastManager::Stop()
{
    AcquireWriteLock grab(lock_);

    for (auto it = multicastContexts_.begin(); it != multicastContexts_.end(); ++it)
    {
        it->second.SetError(ErrorCodeValue::OperationCanceled);
    }
    multicastContexts_.clear();

    timer_->Cancel();

    closed_ = true;
}
