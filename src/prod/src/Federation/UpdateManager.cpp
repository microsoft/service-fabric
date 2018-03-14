// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace std;
using namespace Transport;

StringLiteral const TraceUpdate("Update");
static StringLiteral const UpdateTimerTag("Update");

UpdateManager::UpdateManager(SiteNode & siteNode)
    : siteNode_(siteNode),
    stopped_(false),
    lastExponential_(false),
    exponentialIndex_(0),
    lastSnapshot_(StopwatchTime::Zero),
    snapshotInterval_(FederationConfig::GetConfig().NeighborhoodExchangeTimeout)
{
    targets_.reserve(FederationConfig::GetConfig().MaxUpdateTarget);
    targets_.push_back(siteNode_.Id); // dummy entry for local node

    LargeInteger stepSize = LargeInteger::MaxValue >> 1;
    stepSize += LargeInteger::One; // half the number space size
    targets_.push_back(siteNode_.Id + stepSize);

    while ((targets_.size() + 2) <= static_cast<size_t>(FederationConfig::GetConfig().MaxUpdateTarget) && (stepSize > LargeInteger::Zero))
    {
        stepSize >>= 1;
        targets_.push_back(siteNode_.Id + stepSize);
        targets_.push_back(siteNode_.Id - stepSize);
    }

    UpdateManager::WriteInfo(
        TraceUpdate,
        siteNode_.IdString,
        "config: UpdateInterval={0}, MaxUpdateTarget={1}",
        FederationConfig::GetConfig().UpdateInterval,
        FederationConfig::GetConfig().MaxUpdateTarget);
}

void UpdateManager::Start()
{
    AcquireWriteLock writeLock(thisLock_);
    if (!stopped_)
    {
        auto root = siteNode_.CreateComponentRoot();
        timerSPtr_ = Common::Timer::Create(
            UpdateTimerTag,
            [this, root] (Common::TimerSPtr const &) { this->TimerCallback();}, false);

        TimeSpan updateInterval(FederationConfig::GetConfig().UpdateInterval);
        timerSPtr_->Change(TimeSpan::Zero, updateInterval);

        WriteInfo(TraceUpdate, siteNode_.IdString, "UpdateManager started at site {0}", siteNode_.Id);
    }
}

void UpdateManager::Stop()
{
    bool needToStop;
    {
        AcquireWriteLock writeLock(thisLock_);
        needToStop = (timerSPtr_ != nullptr);
        stopped_ = true;
    }

    if (needToStop)
    {
        timerSPtr_->Cancel();
    }

    WriteInfo(TraceUpdate, siteNode_.IdString, "UpdateManager stopped at site {0}, stop timer {1}", siteNode_.Id, needToStop);
}

_Use_decl_annotations_
void UpdateManager::ProcessUpdateReply(Message & message)
{
    auto body = message.GetBodyStream();
    UpdateReplyBody updateReply;

    NTSTATUS status = body->ReadSerializable(&updateReply);
    if (!NT_SUCCESS(status))
    {
        return;
    }

    siteNode_.GetVoteManager().UpdateGlobalTickets(updateReply.GlobalLease, siteNode_.Instance);

    ProcessIncomingRange(updateReply.UpdateRange);

    SerializableWithActivationList globalStates;
    status = body->ReadSerializable(&globalStates);
    if (NT_SUCCESS(status))
    {
        NodeInstance from;
        if (PointToPointManager::GetFromInstance(message, from))
        {
            siteNode_.Table.ProcessGlobalTimeExchangeHeader(message, from);
            siteNode_.GetGlobalStore().ProcessInputState(globalStates, from);
        }
    }
}

_Use_decl_annotations_
void UpdateManager::ProcessUpdateRequestMessage(Message & message, RequestReceiverContext & requestReceiverContext)
{
    auto body = message.GetBodyStream();

    UpdateRequestBody updateRequest;
    NTSTATUS status = body->ReadSerializable(&updateRequest);
    if (!NT_SUCCESS(status))
    {
        requestReceiverContext.InternalReject(message.FaultErrorCodeValue);
        return;
    }

    SerializableWithActivationList globalStates;
    status = body->ReadSerializable(&globalStates);
    if (NT_SUCCESS(status))
    {
        siteNode_.GetGlobalStore().ProcessInputState(globalStates, requestReceiverContext.From->Instance);
    }

    TimeSpan delta = Stopwatch::Now() - updateRequest.RequestTime;

    vector<VoteTicket> tickets;
    siteNode_.GetVoteManager().GenerateGlobalLease(tickets);

    siteNode_.Table.ProcessGlobalTimeExchangeHeader(message, requestReceiverContext.FromWithSameInstance);

    ProcessIncomingRange(updateRequest.Range);

    UpdateReplyBody updateMessageBody(
        GlobalLease(move(tickets), delta),
        siteNode_.Table.GetCombinedNeighborHoodTokenRange(),
        updateRequest.IsToExponentialTarget);

    MessageUPtr reply = FederationMessage::GetUpdateReply().CreateMessage(updateMessageBody, false);
    reply->AddBody(siteNode_.GetGlobalStore().AddOutputState(&(requestReceiverContext.From->Instance)), true);

    siteNode_.Table.AddGlobalTimeExchangeHeader(*reply, requestReceiverContext.From);
    requestReceiverContext.Reply(move(reply));
}

void UpdateManager::ProcessIncomingRange(NodeIdRange const & range)
{
    AcquireWriteLock writeLock(thisLock_);
    AddRange(range, true);
    WriteInfo(TraceUpdate, siteNode_.IdString, "Add range {0} gaps {1}", range, gaps_.size());
}

bool UpdateManager::TryGetNonExponentialTarget(NodeIdRange const & neighborhoodRange, NodeId & target)
{
    StopwatchTime now = Stopwatch::Now();

    if (gaps_.size() == 0)
    {
        if (ranges_.size() > 0)
        {
            TimeSpan interval = now - ranges_[0].ReceiveTime;
            StopwatchTime expireTime = now - TimeSpan::FromTicks(interval.Ticks / 2);

            size_t i;
            for (i = 0; i < ranges_.size() && ranges_[i].ReceiveTime < expireTime; i++);
            ranges_.erase(ranges_.begin(), ranges_.begin() + i);
        }

        if (!neighborhoodRange.IsFull)
        {
            gaps_.push_back(NodeIdRange(neighborhoodRange.End, neighborhoodRange.Begin));

            for (TimedRange const & range : ranges_)
            {
                AddRange(range.Range, false);
            }
        }
    }

    if (gaps_.size() == 0)
    {
        return false;
    }

    LargeInteger maxGap = LargeInteger::Zero;
    size_t offset = static_cast<size_t>(now.Ticks % gaps_.size());
    for (size_t i = 0; i < gaps_.size(); i++)
    {
        NodeIdRange const & gap = gaps_[offset];
        LargeInteger size = gap.Begin.SuccDist(gap.End);
        if (size > maxGap)
        {
            target = gap.Begin.GetSuccMidPoint(gap.End);
            maxGap = size;
        }

        offset++;
        if (offset == gaps_.size())
        {
            offset = 0;
        }
    }

    WriteInfo(TraceUpdate, siteNode_.IdString, "Gaps {0}, max {1}, target {2}", gaps_.size(), maxGap, target);

    return true;
}

void UpdateManager::TimerCallback()
{
    if (siteNode_.IsShutdown)
    {
        return;
    }

    NodeIdRange range = siteNode_.Table.GetCombinedNeighborHoodTokenRange();
    if (range.IsFull)
    {
        return;
    }

    NodeId updateTarget;
    {
        AcquireWriteLock writeLock(thisLock_);

        if (lastExponential_ && TryGetNonExponentialTarget(range, updateTarget))
        {
            lastExponential_ = false;
        }
        else
        {
            exponentialIndex_++;
            if (exponentialIndex_ == targets_.size())
            {
                exponentialIndex_ = 1;
            }

            updateTarget = targets_[exponentialIndex_];
            lastExponential_ = true;
        }
    }

    UpdateRequestBody updateRequestBody(Stopwatch::Now(), lastExponential_, range);
    auto updateRequestMessage = FederationMessage::GetUpdateRequest().CreateMessage(updateRequestBody, false);
    updateRequestMessage->Idempotent = true;
    updateRequestMessage->AddBody(siteNode_.GetGlobalStore().AddOutputState(nullptr), true);

    siteNode_.Table.AddGlobalTimeExchangeHeader(*updateRequestMessage, nullptr);
    RouteUpdateRequest(std::move(updateRequestMessage), updateTarget);
}

void UpdateManager::RouteUpdateRequest(
    MessageUPtr && updateRequestMessage,
    NodeId targetLocation)
{
    TimeSpan messageTimeout = FederationConfig::GetConfig().MessageTimeout;
    TimeSpan updateRequestTimeout = messageTimeout + messageTimeout;

    siteNode_.BeginRouteRequest(
        std::move(updateRequestMessage),
        targetLocation,
        0,
        false,
        updateRequestTimeout,
        [this, targetLocation](AsyncOperationSPtr const & contextSPtr) -> void
        {
            this->RouteCallback(contextSPtr, targetLocation);
        },
        siteNode_.CreateAsyncOperationRoot());
}

void UpdateManager::RouteCallback(AsyncOperationSPtr const & contextSPtr, NodeId targetLocation)
{
    MessageUPtr reply;
    ErrorCode error = siteNode_.EndRouteRequest(contextSPtr, reply);
    if (!error.IsSuccess())
    {
        SiteNode::WriteWarning(
            TraceUpdate,
            siteNode_.IdString,
            "Update to {0} failed with ErrorCode {1}.",
            targetLocation, error.ReadValue());
    }
    else if (siteNode_.IsAvailable)
    {
        ProcessUpdateReply(*reply);
    }
}

size_t UpdateManager::FindGapEndGreaterThan(NodeId target)
{
    size_t lower = 0;
    size_t upper = gaps_.size();
    while (lower < upper)
    {
        size_t mid = (lower + upper) / 2;
        if (gaps_[mid].End <= target)
        {
            lower = mid + 1;
        }
        else
        {
            upper = mid;
        }
    }

    return lower;
}

void UpdateManager::AddGap(NodeIdRange const & gap)
{
    size_t index = FindGapEndGreaterThan(gap.End);
    gaps_.insert(gaps_.begin() + index, gap);
}

void UpdateManager::AddRange(NodeIdRange const & range, bool newRange)
{
    if (range.IsEmpty)
    {
        return;
    }

    StopwatchTime now = Stopwatch::Now();
    if (newRange)
    {
        ranges_.push_back(TimedRange(range, now));
    }

    vector<NodeIdRange> newGaps;

    size_t index = FindGapEndGreaterThan(range.Begin);
    while (gaps_.size() > 0)
    {
        if (index == gaps_.size())
        {
            index = 0;
        }

        NodeIdRange range1, range2;
        gaps_[index].Subtract(range, range1, range2);
        if (range1 == gaps_[index])
        {
            break;
        }

        bool remove;
        if (range1.IsEmpty || range1.Begin == range1.End)
        {
            remove = true;
        }
        else if (range1.Begin >= range.Begin)
        {
            gaps_[index] = range1;
            remove = false;
        }
        else
        {
            newGaps.push_back(range1);
            remove = true;
        }

        if (!range2.IsEmpty && range2.Begin != range2.End)
        {
            newGaps.push_back(range2);
        }

        if (remove)
        {
            gaps_.erase(gaps_.begin() + index);
        }
        else
        {
            index++;
        }
    }

    for (NodeIdRange const & newGap : newGaps)
    {
        AddGap(newGap);
    }

    TimeSpan minInterval = FederationConfig::GetConfig().NeighborhoodExchangeTimeout;
    if (newRange && gaps_.size() == 0 && ranges_[0].ReceiveTime > lastSnapshot_ && now > lastSnapshot_ + minInterval)
    {
        lastSnapshot_ = now;
        snapshotInterval_ = now - ranges_[0].ReceiveTime;
        snapshotInterval_ = snapshotInterval_ + snapshotInterval_;
        if (snapshotInterval_ < minInterval)
        {
            snapshotInterval_ = minInterval;
        }

        siteNode_.Table.UpdateNeighborhoodExchangeInterval(snapshotInterval_);

        SiteNode::WriteInfo(
            TraceUpdate,
            siteNode_.IdString,
            "Snapshot interval {0}, first range {1}",
            snapshotInterval_, ranges_[0].ReceiveTime);
    }
}

void UpdateManager::TimedRange::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write(Range);
}
