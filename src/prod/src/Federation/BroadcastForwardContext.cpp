// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Federation;

StringLiteral TraceComplete("Complete");

BroadcastForwardContext::ForwardRequest::ForwardRequest(
    NodeIdRange const & requestRange,
    vector<NodeIdRange> && subRanges,
    vector<wstring> const & externalRings,
    AsyncOperationSPtr const & operation, 
    RequestReceiverContextUPtr && requestContext)
    :   requestRange_(requestRange),
        pending_(move(subRanges)),
        pendingRings_(externalRings),
        operation_(operation),
        requestContext_(move(requestContext))
{
}

BroadcastForwardContext::ForwardRequest::ForwardRequest(BroadcastForwardContext::ForwardRequest && other)
    :   requestRange_(other.requestRange_),
        pending_(move(other.pending_)),
        pendingRings_(move(other.pendingRings_)),
        operation_(move(other.operation_)),
        requestContext_(move(other.requestContext_))
{
}

BroadcastForwardContext::ForwardRequest & BroadcastForwardContext::ForwardRequest::operator =(BroadcastForwardContext::ForwardRequest && other)
{
    if (this != & other)
    {
        requestRange_ = other.requestRange_;
        pending_ = move(other.pending_);
        pendingRings_ = move(other.pendingRings_);
        operation_ = other.operation_;
        requestContext_= move(other.requestContext_);
    }

    return *this;
}

bool BroadcastForwardContext::ForwardRequest::CompleteIfNeeded(Common::ErrorCode error)
{
    if (error.IsSuccess() && (pending_.size() > 0 || pendingRings_.size() > 0))
    {
        return false;
    }

    if (operation_)
    {
        AsyncOperationSPtr operation = move(operation_);
        Threadpool::Post([operation, error] { operation->TryComplete(operation, error); });
    }

    if (requestContext_)
    {
        if (error.IsSuccess())
        {
            MoveUPtr<RequestReceiverContext> mover(move(requestContext_));
            Threadpool::Post([mover] () mutable { mover.TakeUPtr()->Reply(FederationMessage::GetBroadcastAck().CreateMessage()); });
        }
        else
        {
            requestContext_->Ignore();
        }
    }

    return true;
}

void BroadcastForwardContext::ForwardRequest::CompleteRange(NodeIdRange const & range)
{
    auto it = find(pending_.begin(), pending_.end(), range);
    if (it != pending_.end())
    {
        pending_.erase(it);
    }
}

void BroadcastForwardContext::ForwardRequest::CompleteLocalRange(NodeId nodeId, NodeIdRange & localRange)
{
    for (auto it = pending_.begin(); it != pending_.end(); ++it)
    {
        if (it->Contains(nodeId))
        {
            localRange = NodeIdRange::Merge(localRange, *it);
            pending_.erase(it);
            return;
        }
    }
}

void BroadcastForwardContext::ForwardRequest::CompleteRing(wstring const & ringName)
{
    for (auto it = pendingRings_.begin(); it != pendingRings_.end(); ++it)
    {
        if (*it == ringName)
        {
            pendingRings_.erase(it);
            return;
        }
    }
}

void BroadcastForwardContext::ForwardRequest::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write("Request {0} pending {1}", requestRange_, pending_);
    if (pendingRings_.size() > 0)
    {
        w.Write(pendingRings_);
    }
}

BroadcastForwardContext::BroadcastForwardContext()
{
}

BroadcastForwardContext::BroadcastForwardContext(
    NodeId nodeId,
    Transport::MessageId const & broadcastId, 
    NodeIdRange const & requestRange,
    std::vector<NodeIdRange> && subRanges,
    AsyncOperationSPtr const & operation,
    RequestReceiverContextUPtr && requestContext,
    vector<wstring> const & externalRings)
    :   nodeId_(nodeId),
        broadcastId_(broadcastId),
        error_(ErrorCode::Success())
{
    requests_.push_back(ForwardRequest(requestRange, move(subRanges), externalRings, operation, move(requestContext)));
}

BroadcastForwardContext::BroadcastForwardContext(BroadcastForwardContext && other)
    :   nodeId_(other.nodeId_),
        broadcastId_(other.broadcastId_),
        error_(other.error_),
        acked_(move(other.acked_)),
        requests_(move(other.requests_)),
        ackedRings_(move(other.ackedRings_))
{
}

void BroadcastForwardContext::CompletIfNeeded()
{
    for (auto it = requests_.begin(); it != requests_.end();)
    {
        if (it->CompleteIfNeeded(error_))
        {
            BroadcastManager::WriteInfo(TraceComplete, "Broadcast {0} range {1} completed on node {2}", 
                broadcastId_, it->requestRange_, nodeId_);

            it = requests_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void BroadcastForwardContext::SetError(ErrorCode error)
{
    error_ = error;
    CompletIfNeeded();
}

void BroadcastForwardContext::CompleteRange(NodeIdRange const & range)
{
    for (auto it = requests_.begin(); it != requests_.end(); ++it)
    {
        it->CompleteRange(range);
    }

    acked_.push_back(range);

    CompletIfNeeded();
}

void BroadcastForwardContext::CompleteLocalRange()
{
    NodeIdRange localRange = NodeIdRange::Empty;
    for (auto it = requests_.begin(); it != requests_.end(); ++it)
    {
        it->CompleteLocalRange(nodeId_, localRange);
    }

    ASSERT_IF(localRange.IsEmpty, "local range not found: {0}", *this);

    acked_.push_back(localRange);

    CompletIfNeeded();
}

void BroadcastForwardContext::CompleteRing(wstring const & ringName)
{
    for (auto it = requests_.begin(); it != requests_.end(); ++it)
    {
        it->CompleteRing(ringName);
    }

    ackedRings_.push_back(ringName);

    CompletIfNeeded();
}

bool BroadcastForwardContext::AddRange(
    NodeIdRange const & range, 
    NodeIdRange const & neighborRange, 
    vector<NodeIdRange> & result,
    RequestReceiverContextUPtr & requestContext)
{
    bool localAcked = false;
    for (auto it = acked_.begin(); it != acked_.end() && !localAcked; ++it)
    {
        if (it->Contains(nodeId_))
        {
            localAcked = true;
            *it = NodeIdRange::Merge(*it, neighborRange);
        }
    }

    vector<NodeIdRange> excludeRanges = acked_;
    excludeRanges.push_back(neighborRange);

    range.Subtract(excludeRanges, result);

    vector<NodeIdRange> pending = result;
    if (!localAcked)
    {
        pending.push_back(neighborRange);
    }

    if (pending.size() == 0)
    {
        return false;
    }

    requests_.push_back(ForwardRequest(range, move(pending), vector<wstring>(), nullptr, move(requestContext)));

    return true;
}

void BroadcastForwardContext::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write("{0} on node {1} Acked: {2}, error: {3}, requests: {4}",
        broadcastId_, nodeId_, acked_, error_, requests_);

    if (ackedRings_.size() > 0)
    {
        w.Write(" Acked Rings: {0}", ackedRings_);
    }
}
