// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Federation;

StringLiteral const TraceGrant("Grant");
StringLiteral const TraceReject("Reject");
StringLiteral const TraceRenew("Renew");
StringLiteral const TraceRelease("Release");
StringLiteral const TraceThrottle("Throttle");

JoinLockManager::JoinLockManager(SiteNode & site)
    :   site_(site),
        owner_(site.Instance),
        lockId_(DateTime::Now().Ticks)
{
}

void JoinLockManager::LockRequestHandler(__in Message &, OneWayReceiverContext & oneWayReceiverContext)
{
    NodeInstance from = oneWayReceiverContext.From->Instance;
    oneWayReceiverContext.Accept();

    if (!site_.IsRouting || oneWayReceiverContext.From->Phase != JoiningPhase::Locking)
    {
        return;
    }

    NodeInstance owner;
    FederationPartnerNodeHeader ownerHeader;
    int64 lockId;
    NodeIdRange hoodRange;

    bool result;
    {
        AcquireExclusiveLock grab(lock_);

        FederationConfig const & config = FederationConfig::GetConfig();
        StopwatchTime lockExpiry = Stopwatch::Now() + config.JoinLockDuration;
        result = site_.Table.LockRange(owner_, oneWayReceiverContext.From, lockExpiry, hoodRange, ownerHeader);
        if (result)
        {
            lockId_++;
            owner_ = from;
        }

        owner = owner_;
        lockId = lockId_;

        AddCompeting(from.Id, config.JoinThrottleActiveInterval);
    }
    
    if (result)
    {
        WriteInfo(TraceGrant, "Lock:{0} {1} granted to {2}",
            site_.Id, lockId, from);

        JoinLock lock(lockId, hoodRange);

        site_.Send(FederationMessage::GetLockGrant().CreateMessage(lock), oneWayReceiverContext.From);
    }
    else
    {
        WriteInfo(TraceReject, "Lock:{0} request from {1} denied as it is granted to {2}",
            site_.Id, oneWayReceiverContext.From->Instance, owner);

        site_.Send(FederationMessage::GetLockDeny().CreateMessage(ownerHeader), oneWayReceiverContext.From);
    }
}

void JoinLockManager::LockRenewHandler(__in Message & message, OneWayReceiverContext & oneWayReceiverContext)
{
    oneWayReceiverContext.Accept();

    LockIdBody body;
    if (!message.GetBody<LockIdBody>(body))
    {
        return;
    }

    if (oneWayReceiverContext.From->Phase != JoiningPhase::Locking)
    {
        return;
    }

    int64 lockId = body.get_Id();
    NodeIdRange hoodRange;

    {
        NodeInstance const & from = oneWayReceiverContext.From->Instance;
        FederationConfig const & config = FederationConfig::GetConfig();

        AcquireExclusiveLock grab(lock_);

        AddCompeting(from.Id, config.JoinThrottleActiveInterval);

        if (lockId != lockId_ || owner_.Id == site_.Id)
        {
            WriteInfo(TraceRenew,
                "Lock:{0} {1} renew {2} from node {3} ignored",
                site_.Id, lockId_, lockId, from);

            return;
        }

        FederationPartnerNodeHeader ownerHeader;
        StopwatchTime lockExpiry = Stopwatch::Now() + config.JoinLockDuration;
        if (!site_.Table.LockRange(site_.Instance, oneWayReceiverContext.From, lockExpiry, hoodRange, ownerHeader))
        {
            WriteInfo(TraceRenew,
                "Lock:{0} {1} renew {2} from node {3} rejected",
                site_.Id, lockId_, lockId, from);

            return;
        }

        owner_ = from;
    }

    WriteInfo(TraceRenew,
        "Lock:{0} {1} renewed to node {2}",
        site_.Id, lockId, owner_);

    JoinLock lock(lockId, hoodRange, true);
    site_.Send(FederationMessage::GetLockGrant().CreateMessage(lock), oneWayReceiverContext.From);
}

void JoinLockManager::LockReleaseHandler(__in Message & message, OneWayReceiverContext & oneWayReceiverContext)
{
    oneWayReceiverContext.Accept();

    LockIdBody body;
    if (!message.GetBody<LockIdBody>(body))
    {
        return;
    }

    int64 lockId = body.get_Id();

    ReleaseLock(lockId, oneWayReceiverContext.From->Instance);
}

void JoinLockManager::UnlockHandler(__in Message & message, OneWayReceiverContext & oneWayReceiverContext)
{
    oneWayReceiverContext.Accept();

    LockIdBody body;
    if (!message.GetBody<LockIdBody>(body))
    {
        return;
    }

    int64 lockId = body.get_Id();

    bool result = ReleaseLock(lockId, oneWayReceiverContext.From->Instance);

    MessageUPtr reply = (result ? FederationMessage::GetUnlockGrant().CreateMessage(LockIdBody(lockId)) : FederationMessage::GetUnlockDeny().CreateMessage(LockIdBody(lockId)));
    site_.Send(move(reply), oneWayReceiverContext.From);
}

bool JoinLockManager::ReleaseLock(int64 lockId, NodeInstance const & from)
{
    AcquireExclusiveLock grab(lock_);    

    RemoveCompeting(from.Id);

    if (lockId != lockId_)
    {
        WriteInfo(TraceRelease,
            "Lock:{0} {1} release {2} from {3} release ignored",
            site_.Id, lockId_, lockId, from);

        return false;
    }

    owner_ = site_.Instance;
    
    site_.Table.ReleaseRangeLock();

    WriteInfo(TraceRelease,
        "Lock:{0} {1} from {2} released",
        site_.Id, lockId, from);

    return true;
}

void JoinLockManager::ResetLock()
{
    AcquireExclusiveLock grab(lock_);    

    if (owner_ != site_.Instance)
    {
        WriteInfo(TraceRelease,
            "Lock:{0} {1} force released",
            site_.Id, lockId_);

        owner_ = site_.Instance;
        site_.Table.ReleaseRangeLock();
    }
}

void JoinLockManager::AddCompeting(NodeId id, TimeSpan timeout)
{
    competing_[id] = Stopwatch::Now() + timeout;
    waiting_.erase(id);
}

void JoinLockManager::RemoveCompeting(NodeId id)
{
    competing_.erase(id);
    waiting_.erase(id);

    if (waiting_.size() > 0 && competing_.size() < FederationConfig::GetConfig().JoinThrottleLowThreshold)
    {
        auto rootSPtr = site_.GetSiteNodeSPtr();
        Threadpool::Post([this, rootSPtr]() { this->ProcessWaitingList(); });
    }
}

void JoinLockManager::RefreshCompeting(NodeIdRange const & range)
{
    StopwatchTime now = Stopwatch::Now();

    for (auto it = competing_.begin(); it != competing_.end();)
    {
        if (!range.Contains(it->first) || it->second < now)
        {
            it = competing_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void JoinLockManager::AddThrottleHeaderIfNeeded(__in Transport::Message & message, NodeId id)
{
    FederationConfig const & config = FederationConfig::GetConfig();
    NodeIdRange range = site_.Table.GetHoodRange();

    AcquireExclusiveLock grab(lock_);

    if (competing_.find(id) != competing_.end())
    {
        return;
    }

    RefreshCompeting(range);

    if (competing_.size() <= config.JoinThrottleHighThreshold && waiting_.find(id) == waiting_.end())
    {
        AddCompeting(id, config.NeighborhoodQueryRetryInterval);
        return;
    }

    if (waiting_.size() == 0)
    {
        ScheduleNextCheck();
    }

    StopwatchTime expire = Stopwatch::Now() + config.JoinThrottleTimeout;
    waiting_[id] = expire;
    message.Headers.Add(JoinThrottleHeader(expire.Ticks, true));

    WriteInfo(TraceThrottle, site_.IdString, "Add {0}, competing {1}, waiting {2}", id, competing_.size(), waiting_.size());
}

void JoinLockManager::ScheduleNextCheck()
{
    auto rootSPtr = site_.GetSiteNodeSPtr();
    Threadpool::Post([this, rootSPtr]() { this->CheckWaitingList(); }, FederationConfig::GetConfig().JoinThrottleCheckInterval);
}

void JoinLockManager::CheckWaitingList()
{
    if (ProcessWaitingList())
    {
        ScheduleNextCheck();
    }
}

bool JoinLockManager::ProcessWaitingList()
{
    FederationConfig const & config = FederationConfig::GetConfig();

    StopwatchTime now = Stopwatch::Now();
    NodeIdRange range = site_.Table.GetHoodRange();
    bool result;

    map<NodeId, StopwatchTime> targets;
    {
        AcquireExclusiveLock grab(lock_);

        RefreshCompeting(range);

        while (waiting_.size() > 0 && competing_.size() < config.JoinThrottleLowThreshold)
        {
			auto it = waiting_.begin();
            NodeId id = it->first;
            if (it->second > now)
            {
                targets[id] = it->second;
                if (range.Contains(id))
                {
                    competing_[id] = now + config.NeighborhoodQueryRetryInterval;
                }

                WriteInfo(TraceThrottle, site_.IdString, "Resume {0}", id);
            }
            else
            {
                WriteInfo(TraceThrottle, site_.IdString, "Remove {0}", id);
            }

            waiting_.erase(it);
        }

        WriteInfo(TraceThrottle, site_.IdString, "Process competing {0}, waiting {1}, range {2}", competing_.size(), waiting_.size(), range);

        result = (waiting_.size() > 0);
    }

    for (auto it2 = targets.begin(); it2 != targets.end(); ++it2)
    {
        PartnerNodeSPtr target = site_.Table.Get(it2->first);
        if (target && target->Phase == NodePhase::Joining)
        {
            MessageUPtr msg = FederationMessage::GetResumeJoin().CreateMessage();
            msg->Headers.Add(JoinThrottleHeader(it2->second.Ticks, !range.Contains(it2->first)));
            site_.Send(move(msg), target);
        }
    }

    return result;
}
