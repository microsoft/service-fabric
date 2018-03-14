// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Federation;
using namespace LeaseWrapper;

StringLiteral const TraceNeighborhoodQuery("NeighborhoodQuery");
StringLiteral const TraceLock("Lock");
StringLiteral const TraceEstablish("Establish");
StringLiteral const TraceUnlock("Unlock");
StringLiteral const TraceEntry("Entry");
StringLiteral const TraceState("State");
StringLiteral const TracePhase("Phase");
StringLiteral const TraceAction("Action");
StringLiteral const TraceThrottle("Throttle");

static const StringLiteral JoinTimerTag("Join");

void JoiningPhase::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    switch (val)
    {
        case QueryingNeighborhood:
            w << "Q";
            return;
        case Locking:
            w << "L";
            return;
        case EstablishingLease:
            w << "E";
            return;
        case UnLocking:
            w << "U";
            return;
        case Routing:
            w << "R";
            return;
        default: 
            coding_error("Unknown Phase");
            return;
    }
}

void JoinLockTransfer::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write("Lock:{0} {1} {2}", origin_.Id, lock_, held_);
}

JoinLockInfo::JoinLockInfo(PartnerNodeSPtr const & partner, bool isLocking)
    :   partner_(partner),
        lock_(),
        isPhaseCompleted_(false),
        isKnown_(!isLocking),
        lastSent_(StopwatchTime::Zero),
        acquiredTime_(StopwatchTime::Zero),
        retryCount_(0)
{
}

void JoinLockInfo::Reset()
{
    isPhaseCompleted_ = false;
    acquiredTime_ = lastSent_ = StopwatchTime::Zero;
}

bool JoinLockInfo::AcquireLock(JoinLock const & lock)
{
    acquiredTime_ = Stopwatch::Now();

    if (isPhaseCompleted_)
    {
        return false;
    }

    lock_ = lock;
    isPhaseCompleted_ = true;
    lastSent_ = acquiredTime_;

    return true;
}

void JoinLockInfo::SetKnown()
{
    isKnown_ = true;
}

void JoinLockInfo::GenerateActions(JoiningPhase::Enum phase, StateMachineActionCollection & actions)
{
    FederationConfig & config = FederationConfig::GetConfig();
    StopwatchTime now = Stopwatch::Now();
    TimeSpan duration = now - lastSent_;
    TimeSpan renewDuration = TimeSpan::FromTicks(config.JoinLockDuration.Ticks * 2 / 3); 
    MessageUPtr message = nullptr;

    if (phase == JoiningPhase::Locking)
    {
        if (isPhaseCompleted_)
        {
            if (now - acquiredTime_ > config.JoinLockDuration)
            {
                Reset();
                message = FederationMessage::GetLockRequest().CreateMessage();
            }
            else if (duration > (isKnown_ ? renewDuration : config.LockRequestTimeout))
            {
                message = FederationMessage::GetLockRenew().CreateMessage(LockIdBody(lock_.Id));
            }
        }
        else
        {
            if (duration > config.LockRequestTimeout)
            {
                message = FederationMessage::GetLockRequest().CreateMessage();
            }
        }
    }
    else if (phase == JoiningPhase::UnLocking && !IsCompleted() && duration > config.LockRequestTimeout)
    {
        message = FederationMessage::GetUnlockRequest().CreateMessage(LockIdBody(lock_.Id));
    }

    if (message)
    {
        lastSent_ = now;
        retryCount_++;
        actions.Add(make_unique<SendMessageAction>(move(message), partner_));
    }
}

void JoinLockInfo::Release(StateMachineActionCollection & actions)
{
    if (isPhaseCompleted_)
    {
        MessageUPtr message = FederationMessage::GetLockRelease().CreateMessage(LockIdBody(lock_.Id));
        actions.Add(make_unique<SendMessageAction>(move(message), partner_));
    }
}

void JoinLockInfo::WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
{
    w.Write(partner_->Instance);
    if (isPhaseCompleted_)
    {
        w.Write(L" ");
        w.Write(lock_);
    }
}

JoinManager::JoinManager(SiteNode & siteNode)
    :   site_(siteNode),
        timeoutHelper_(TimeSpan::MaxValue),
        phase_(JoiningPhase::QueryingNeighborhood),
        insertStartTime_(StopwatchTime::Zero),
        entries_(),
        lastStateDump_(DateTime::Now()),
        waitRoutingNodeCount_(0),
        throttleSequence_(0),
        throttled_(false),
        resumed_(false),
        queryAfterResume_(false)
{
}

bool JoinManager::Start(TimeSpan timeout)
{
    timeoutHelper_.SetRemainingTime(timeout);

    auto rootSPtr = site_.GetSiteNodeSPtr();
    timer_ = Timer::Create(
        JoinTimerTag,
        [this, rootSPtr] (TimerSPtr const &) { this->RunStateMachine(); },
        true);

    WriteInfo("Start", site_.IdString, "Join Manager started");

    SendNeighborhoodQuery();
    
    return true;
}

void JoinManager::Stop()
{
    if (timer_)
    {
        timer_->Cancel();
    }

    AcquireExclusiveLock grab(stateLock_);
    entries_.clear();

    WriteInfo("Stop", site_.IdString, "Join Manager stopped");
}

void JoinManager::OnThrottleTimeout(int64 throttleSequence)
{
    {
        AcquireExclusiveLock grab(stateLock_);

        if (!throttled_ || throttleSequence_ != throttleSequence)
        {
            return;
        }

        throttled_ = false;
    }

    SendNeighborhoodQuery();
}

void JoinManager::SendNeighborhoodQuery()
{
    TimeSpan timeout = timeoutHelper_.GetRemainingTime();
    if (timeout <= TimeSpan::Zero)
    {
        site_.CompleteOpenAsync(ErrorCodeValue::Timeout);
    }
    else if (site_.Table.UpNodes == 0)
    {
        site_.CompleteOpenAsync(ErrorCodeValue::OperationFailed);
    }
    else
    {
        WriteInfo(TraceNeighborhoodQuery, site_.IdString, "Start request");

        MessageUPtr message = FederationMessage::GetNeighborhoodQueryRequest().CreateMessage(NeighborhoodQueryRequestBody(Stopwatch::Now()));
        message->Idempotent = true;
        message->Headers.Add(FabricCodeVersionHeader(site_.CodeVersion));
        site_.Table.AddGlobalTimeExchangeHeader(*message, nullptr);

        auto operation = site_.BeginRouteRequest(
            move(message),
            site_.Id,
            0,
            false,
            timeout,
            [this](AsyncOperationSPtr const & operation) { this->OnNeighborhoodQueryRequestCompleted(operation); },
            site_.CreateAsyncOperationRoot());
    }
}

bool JoinManager::IsSafeToCompleteJoinQuery()
{
    bool isSeed = VoteManager::IsSeedNode(site_.Id, site_.RingName);

    int result = site_.GetVoteManager().GetGlobalLeaseLevel();
    if (result < 0 || (result == 0 && !isSeed))
    {
        WriteInfo(TraceNeighborhoodQuery, site_.IdString, "global lease expiration is too close, retry neighborhood query");
        return false;
    }

    if (result > 1 || isSeed)
    {
        return true;
    }

    FederationConfig const & config = FederationConfig::GetConfig();

    int routingNodes = site_.Table.GetRoutingNodeCount();
    if (routingNodes >= config.Votes.size() / 2 + 1 || 
        routingNodes >= config.NeighborhoodSize * 2 ||
        config.NeighborhoodQueryRetryInterval.Ticks * waitRoutingNodeCount_ >= config.NonSeedNodeJoinWaitInterval.Ticks)
    {
        return true;
    }

    waitRoutingNodeCount_++;

    WriteInfo(
        TraceNeighborhoodQuery,
        site_.IdString,
        "Only {0} routing nodes, retry neighborhood query {1}",
        routingNodes, waitRoutingNodeCount_);

    return false;
}

void JoinManager::CompleteNeighborhoodQuery()
{
    {
        AcquireExclusiveLock grab(stateLock_);
        if (phase_ != JoiningPhase::QueryingNeighborhood)
        {
            return;
        }

        phase_ = JoiningPhase::Locking;
        WriteInfo(TracePhase, site_.IdString, "Joining phase changed from {0} to {1}", JoiningPhase::QueryingNeighborhood, phase_);
    }

    // !!! It is assumed neighborhood query never completes on the same thread. If this ever becomes false, we will need to call
    // StartNewPhase() on a different thread, because it acquires SiteNode::stateLock_, under which JoinManager is being started.
    StartNewPhase();
}

void JoinManager::OnNeighborhoodQueryRequestCompleted(AsyncOperationSPtr const& sendRequestOperation)
{
    MessageUPtr reply;
    ErrorCode error = site_.EndRouteRequest(sendRequestOperation, reply);

    WriteInfo(TraceNeighborhoodQuery, site_.IdString, "NeighborhoodQueryRequest completed {0}", error);

    int64 throttleSequence = 0;
    bool complete = false;
    bool sendQuery = false;
    NodeInstance fromInstance;

    if (error.IsSuccess())
    {
        FabricCodeVersion version;
        FabricCodeVersionHeader::ReadFromMessage(*reply, version);

        if (version < *FabricCodeVersion::MinCompatible)
        {
            Trace.WriteError(Constants::VersionTraceType, "{0} unable to join the ring with version {1}, min acceptable {2}",
                site_.Instance, version, *FabricCodeVersion::MinCompatible);
            site_.CompleteOpenAsync(ErrorCodeValue::IncompatibleVersion);

            return;
        }

        site_.GetVoteManager().UpdateGlobalTickets(*reply, site_.Instance);
        if (site_.GetVoteManager().IsGlobalLeaseAbandoned())
        {
            Trace.WriteInfo(TracePhase, site_.IdString, "Stop joining process, wait for open to timeout");
            return;
        }

        if (PointToPointManager::GetFromInstance(*reply, fromInstance))
        {
            site_.Table.ProcessGlobalTimeExchangeHeader(*reply, fromInstance);
        }

        JoinThrottleHeader throttleHeader;
        if (!VoteManager::IsSeedNode(site_.Id, site_.RingName) && reply->Headers.TryReadFirst(throttleHeader))
        {
            throttleSequence = throttleHeader.Sequence;
        }
        else
        {
            complete = IsSafeToCompleteJoinQuery();
        }
    }
    else if (error.IsError(ErrorCodeValue::IncompatibleVersion) || error.IsError(ErrorCodeValue::OperationCanceled) || site_.IsShutdown)
    {
        site_.CompleteOpenAsync(error);
        return;
    }

    {
        AcquireExclusiveLock grab(stateLock_);

        ASSERT_IF(throttled_, "{0} received NeighborhoodQuery while being throttled", site_.Instance);
        if (resumed_ && throttleSequence_ == throttleSequence)
        {
            if (queryAfterResume_)
            {
                sendQuery = true;
            }
            else
            {
                complete = true;
            }
        }
        else if (throttleSequence > 0 && fromInstance.InstanceId > 0)
        {
            throttleFrom_ = fromInstance;
            throttleSequence_ = throttleSequence;
            throttled_ = true;
        }

        resumed_ = false;
    }

    if (complete)
    {
        CompleteNeighborhoodQuery();
    }
    else if (sendQuery)
    {
        SendNeighborhoodQuery();
    }
    else
    {
        auto rootSPtr = site_.GetSiteNodeSPtr();
        if (throttleSequence == 0)
        {
            Threadpool::Post([this, throttleSequence, rootSPtr]() { this->SendNeighborhoodQuery(); }, FederationConfig::GetConfig().NeighborhoodQueryRetryInterval);
        }
        else
        {
            WriteInfo(TraceThrottle, site_.IdString, "Start throttle {0}", throttleSequence);
            Threadpool::Post([this, throttleSequence, rootSPtr]() { this->OnThrottleTimeout(throttleSequence); }, FederationConfig::GetConfig().JoinThrottleTimeout);
        }
    }
}

void JoinManager::ResetStateForJoining()
{
    entries_.clear();
    
    if (phase_ ==  JoiningPhase::Routing)
    {
        return;
    }

    site_.Table.SetTokenAcquireDeadline(StopwatchTime::MaxValue);
    phase_ = JoiningPhase::QueryingNeighborhood;
}

void JoinManager::RestartJoining()
{
    ErrorCode error = site_.RestartInstance();
    if (error.IsSuccess())
    {
        SendNeighborhoodQuery();
    }
    else
    {
        site_.CompleteOpen(error);
    }
}

void JoinManager::LockGrantHandler(__in Message & message, OneWayReceiverContext & oneWayReceiverContext)
{
    oneWayReceiverContext.Accept();

    JoinLock lock;
    if (!message.GetBody<JoinLock>(lock))
    {
        return;
    }

    bool acquired = false;
    bool completed = false;
    bool runStateMachine = false;
    StateMachineActionCollection actions;
    {
        AcquireExclusiveLock grab(stateLock_);
        
        if (phase_ == JoiningPhase::Locking)
        {
            UpdateNeighborhood(actions);

            JoinLockInfo* existing = GetLock(oneWayReceiverContext.From->Instance);
            if (existing)
            {
                existing->SetKnown();
                if (!lock.IsRenew || existing->IsCompleted())
                {
                    existing->RetryCount = 0;

                    bool firstAcquire = existing->AcquireLock(lock);
                    WriteInfo(TraceLock, site_.IdString, "Lock:{0} {1} {2}",
                        oneWayReceiverContext.From->Id,
                        firstAcquire ? "acquired" : "renewed",
                        lock);

                    acquired = true;
                    runStateMachine = true;
                }
            }

            completed = CheckCompletion();
        }
        else if (phase_ == JoiningPhase::EstablishingLease || phase_ == JoiningPhase::UnLocking)
        {
            JoinLockInfo* existing = GetLock(oneWayReceiverContext.From->Instance);
            acquired = (existing != nullptr);
        }
    }
    
    if (!acquired && !lock.IsRenew)
    {
        MessageUPtr msg = FederationMessage::GetLockRelease().CreateMessage(LockIdBody(lock.Id));
        actions.Add(make_unique<SendMessageAction>(move(msg), oneWayReceiverContext.From));
    }

    actions.Execute(site_);

    if (completed)
    {
        StartNewPhase();
    }
    else if (runStateMachine)
    {
        RunStateMachine();
    }
}

void JoinManager::LockDenyHandler(__in Message & message,  OneWayReceiverContext & oneWayReceiverContext)
{
    oneWayReceiverContext.Accept();

    FederationPartnerNodeHeader owner;
    if (!message.GetBody(owner))
    {
        return;
    }

    WriteInfo(TraceLock, site_.IdString, "Lock request denied from {0} owner {1}", 
        oneWayReceiverContext.From->Id, owner.Instance);

    StateMachineActionCollection actions;
    vector<JoinLockTransfer> locks;

    int count = 0;
    {
        AcquireExclusiveLock grab(stateLock_);

        if (phase_ != JoiningPhase::Locking)
        {
            return;
        }

        UpdateNeighborhood(actions);

        JoinLockInfo* existing = GetLock(oneWayReceiverContext.From->Instance);
		if (!existing ||
            existing->IsCompleted() ||
            owner.Instance.Id == oneWayReceiverContext.From->Id ||
            owner.Instance.Id == site_.Id)
        {
            return;
        }
         
        existing->RetryCount = 0;
        existing->SetKnown();

        for (JoinLockInfoUPtr const & entry : entries_)
        {
            if (entry->IsCompleted())
            {
                count++;
            }
            locks.push_back(entry->CreateTransfer());
        }
    }

    actions.Execute(site_);

    PartnerNodeSPtr ownerSPtr = site_.Table.ConsiderAndReturn(owner, false);
    if (count > 0)
    {
        if (ownerSPtr)
        {
            WriteInfo(TraceLock, site_.IdString, "Sending transfer request to {0}", owner.Instance);

            JoinLockTransferBody body(std::move(locks));

            site_.Send(FederationMessage::GetLockTransferRequest().CreateMessage(body), ownerSPtr);
        }
        else
        {
            WriteWarning(TraceAction, site_.IdString,
                "Skip transfer request to {0} since it is not in routing table",
                owner.Instance);
        }
    }
}

void JoinManager::UnLockGrantHandler(__in Message & message, OneWayReceiverContext & oneWayReceiverContext)
{
    oneWayReceiverContext.Accept();

    LockIdBody body;
    if (!message.GetBody<LockIdBody>(body))
    {
        return;
    }

    int64 lockId = body.get_Id();

    bool completed = false;
    {
        AcquireExclusiveLock grab(stateLock_);

        JoinLockInfo* lock = GetLock(oneWayReceiverContext.From->Instance);
        if (lock && lock->GetLockId() == lockId && !lock->IsCompleted() && phase_ == JoiningPhase::UnLocking)
        {
            lock->Complete();
            WriteInfo(TraceUnlock, site_.IdString, "Unlock {0} granted from {1}", lockId, lock->Instance);
    
            completed = CheckCompletion();
        }
    }

    if (completed)
    {
        StartNewPhase();
    }
}

void JoinManager::UnLockDenyHandler(__in Message & message, OneWayReceiverContext & oneWayReceiverContext)
{
    oneWayReceiverContext.Accept();

    LockIdBody body;
    if (!message.GetBody<LockIdBody>(body))
    {
        return;
    }

    int64 lockId = body.get_Id();

    bool restart;
    {
        AcquireExclusiveLock grab(stateLock_);

        JoinLockInfo* lock = GetLock(oneWayReceiverContext.From->Instance);
        restart = (lock && lock->GetLockId() == lockId && !lock->IsCompleted() && phase_ == JoiningPhase::UnLocking);
        if (restart)
        {
            ResetStateForJoining();
        }
    }

    if (restart)
    {
        WriteWarning(TraceUnlock, site_.IdString, "{0} restarting as unlock denied by {1}",
            site_.Id, oneWayReceiverContext.From->Instance);

        RestartJoining();
    }
}

void JoinManager::LockTransferRequestHandler(__in Message & message, OneWayReceiverContext & oneWayReceiverContext)
{
    oneWayReceiverContext.Accept();

    JoinLockTransferBody body;
    if (!message.GetBody(body))
    {
        return;
    }

    vector<JoinLockTransfer> request = body.TakeTransfers();

    vector<JoinLockTransfer> response;

    {
        AcquireExclusiveLock grab(stateLock_);

        if (phase_ == JoiningPhase::Locking)
        {
            ProcessTransferReuqest(request, oneWayReceiverContext.From->Id, response);
        }
    }

    body = JoinLockTransferBody(std::move(response));
    site_.Send(FederationMessage::GetLockTransferReply().CreateMessage(body), oneWayReceiverContext.From);
}

void JoinManager::LockTransferReplyHandler(__in Message & message, OneWayReceiverContext & oneWayReceiverContext)
{
    oneWayReceiverContext.Accept();

    JoinLockTransferBody body;
    if (!message.GetBody(body))
    {
        return;
    }

    vector<JoinLockTransfer> transfers = body.TakeTransfers();

    if (transfers.size() == 0)
    {
        WriteInfo(TraceLock, site_.IdString, "No lock transferred from {0}", oneWayReceiverContext.From->Id);
        return;
    }

    vector<bool> acquired;
    bool completed = false;
    StateMachineActionCollection actions;
    {
        AcquireExclusiveLock grab(stateLock_);
        if (phase_ == JoiningPhase::Locking)
        {
            UpdateNeighborhood(actions);

            for (JoinLockTransfer const & transfer : transfers)
            {
                JoinLockInfo* lock = GetLock(transfer.Origin);
                if (lock && lock->AcquireLock(transfer.Lock))
                {
                    WriteInfo(TraceLock, site_.IdString, "Lock:{0} {1} acquired from {2}",
                        transfer.Origin.Id, transfer.Lock, oneWayReceiverContext.From->Id);
                }
                acquired.push_back(lock != nullptr);
            }

            completed = CheckCompletion();
        }
    }

    actions.Execute(site_);

    for (size_t i = 0; i < transfers.size(); i++)
    {
        PartnerNodeSPtr partner = site_.Table.Get(transfers[i].Origin);
        if (partner)
        {
            FederationMessage msg = (i < acquired.size() && acquired[i] ? FederationMessage::GetLockRenew() : FederationMessage::GetLockRelease());
            site_.Send(msg.CreateMessage(LockIdBody(transfers[i].Lock.Id)), partner);
        }
    }

    if (completed)
    {
        StartNewPhase();
    }
    else
    {
        RunStateMachine();
    }
}

void JoinManager::ResumeJoinHandler(__in Message & message, OneWayReceiverContext & oneWayReceiverContext)
{
    oneWayReceiverContext.Accept();

    JoinThrottleHeader throttleHeader;
    message.Headers.TryReadFirst(throttleHeader);
    WriteInfo(TraceThrottle, site_.IdString, "Receive resume with seq {0} {1}", throttleHeader.Sequence, throttleHeader.QueryNeeded);

    bool canComplete = !throttleHeader.QueryNeeded && (site_.GetVoteManager().GetGlobalLeaseLevel() > 0);
    bool complete = false;
    bool sendQuery = false;
    {
        AcquireExclusiveLock grab(stateLock_);

        if (throttled_)
        {
            if (oneWayReceiverContext.From->Instance == throttleFrom_ && throttleHeader.Sequence >= throttleSequence_)
            {
                if (canComplete)
                {
                    complete = true;
                }
                else
                {
                    sendQuery = true;
                }

                throttled_ = false;
            }
            else
            {
                WriteInfo(TraceThrottle, site_.IdString, "Ignore resume, current sequence {0}", throttleSequence_);
            }
        }
        else
        {
            throttleSequence_ = throttleHeader.Sequence;
            resumed_ = true;
            queryAfterResume_ = throttleHeader.QueryNeeded;

            WriteInfo(TraceThrottle, site_.IdString, "Out of order or stale resume");
        }
    }

    if (complete)
    {
        CompleteNeighborhoodQuery();
    }
    else if (sendQuery)
    {
        SendNeighborhoodQuery();
    }
}

void JoinManager::StartLeaseEstablish()
{
    TimeSpan timeout = timeoutHelper_.GetRemainingTime();
    if (timeout <= TimeSpan::Zero)
    {
        site_.CompleteOpenAsync(ErrorCodeValue::Timeout);
        return;
    }

    FederationConfig & config = FederationConfig::GetConfig();
    timeout = min(timeout, config.JoinLockDuration);
    timeout = min(timeout, config.TokenAcquireTimeout);

    insertStartTime_ = Stopwatch::Now();
    site_.Table.SetTokenAcquireDeadline(insertStartTime_ + config.TokenAcquireTimeout);
    site_.ChangePhase(NodePhase::Inserting);

    vector<PartnerNodeSPtr> hood;
    {
        AcquireExclusiveLock grab(stateLock_);

        for (JoinLockInfoUPtr const & entry : entries_)
        {
            hood.push_back(entry->GetPartner());
        }
    }

    site_.Table.SetInitialLeasePartners(hood);
    auto root = site_.CreateComponentRoot();

    for (PartnerNodeSPtr const & partner : hood)
    {
        NodeInstance thisInstance = site_.Instance;
        NodeInstance remoteInstance = partner->Instance;
        LEASE_DURATION_TYPE leaseTimeoutType;
        Common::TimeSpan leaseTimeout = site_.GetLeaseDuration(*partner, leaseTimeoutType);

        WriteInfo(TraceEstablish, site_.IdString, "{0} EstablishLease for {1}", thisInstance, remoteInstance);

        site_.GetLeaseAgent().BeginEstablish(
            remoteInstance.ToString(),
            partner->NodeFaultDomainId.ToString(),
            partner->LeaseAgentAddress,
            partner->LeaseAgentInstanceId,
            leaseTimeoutType,
            min(timeout, leaseTimeout + TimeSpan::FromSeconds(5)),
            [root, this, thisInstance, remoteInstance] (AsyncOperationSPtr const & operation) 
            { Threadpool::Post([root, this, thisInstance, remoteInstance, operation] { this->LeaseEstablishHandler(operation, thisInstance, remoteInstance); }); },
            site_.CreateAsyncOperationRoot());
    }
}

void JoinManager::LeaseEstablishHandler(AsyncOperationSPtr const & operation, NodeInstance const & thisInstance, NodeInstance const & partner)
{
    ErrorCode error =  LeaseAgent::EndEstablish(operation);
    WriteInfo(TraceEstablish, site_.IdString, "{0} lease with {1} estabish result {2}", thisInstance, partner, error);

    if (!error.IsSuccess())
    {
        bool restart;
        {
            AcquireExclusiveLock grab(stateLock_);

            JoinLockInfo* lock = GetLock(partner);
            restart = (lock && !lock->IsCompleted() && phase_ == JoiningPhase::EstablishingLease && site_.Instance == thisInstance);
            if (restart)
            {
                ResetStateForJoining();
            }
        }

        if (restart)
        {
            WriteWarning(TraceEstablish, site_.IdString, "Restarting as lease establish with {0} failed {1}",
                partner, error);

            RestartJoining();
        }
        return;
    }

    bool completed = false;
    {
        AcquireExclusiveLock grab(stateLock_);

        JoinLockInfo* lock = GetLock(partner);
        if (lock && !lock->IsCompleted() && phase_ == JoiningPhase::EstablishingLease && site_.Instance == thisInstance)
        {
            lock->Complete();

            completed = CheckCompletion();
        }
    }

    if (completed)
    {
        StartNewPhase();
    }
}

JoinLockInfo* JoinManager::GetLock(NodeId nodeId)
{
    for (JoinLockInfoUPtr const & entry : entries_)
    {
        if (entry->Id == nodeId)
        {
            return entry.get();
        }
    }

    return nullptr;
}

JoinLockInfo* JoinManager::GetLock(NodeInstance const & nodeInstance)
{
    JoinLockInfo* result = GetLock(nodeInstance.Id);
    if (result && result->Instance == nodeInstance)
    {
        return result;
    }

    return nullptr;
}

void JoinManager::RemoveLock(NodeId nodeId)
{
    auto end = remove_if(entries_.begin(), entries_.end(), [nodeId](JoinLockInfoUPtr const & x) { return (x->Id == nodeId); });
    entries_.erase(end, entries_.end());
}

void JoinManager::UpdateNeighborhood(StateMachineActionCollection & actions)
{
    vector<PartnerNodeSPtr> hood;
    site_.Table.GetExtendedHood(hood);

    for (PartnerNodeSPtr const & partner : hood)
    {
        JoinLockInfo* lock = GetLock(partner->Id);
        if (lock)
        {
            if (lock->Instance < partner->Instance)
            {
                RemoveLock(partner->Id);
                entries_.push_back(make_unique<JoinLockInfo>(partner, phase_ == JoiningPhase::Locking));
            }
        }
        else
        {
            entries_.push_back(make_unique<JoinLockInfo>(partner, phase_ == JoiningPhase::Locking));
            WriteInfo(TraceEntry, site_.IdString, "adding entry for {0}", partner->Instance);
        }
    }

    for (auto it = entries_.begin(); it != entries_.end(); )
    {
        NodeId id = (*it)->Id;
        auto partner = find_if(
            hood.begin(),
            hood.end(),
            [id] (PartnerNodeSPtr const & value) {return value->Id == id ;} );
        if (partner == hood.end())
        {
            WriteInfo(TraceEntry, site_.IdString, "removing entry for {0}", (*it)->Instance);
            (*it)->Release(actions);
            it = entries_.erase(it);
        }
        else
        {
            it++;
        }
    }
}

void JoinManager::ProcessTransferReuqest(vector<JoinLockTransfer> const & request, NodeId requestorId, vector<JoinLockTransfer> & response)
{
    int remoteLocksCount = 0, localLocksCount = 0;
    vector<JoinLockInfo*> transfers;

    for (JoinLockTransfer const & transfer : request)
    {
        if (transfer.IsHeld)
        {
            remoteLocksCount++;
        }
        else
        {
            JoinLockInfo* local = GetLock(transfer.Origin);
            if (local && local->IsCompleted())
            {
                transfers.push_back(local);
                remoteLocksCount++;
            }
        }
    }

    int localIncrease = 0;
    for (auto it = entries_.begin(); it != entries_.end(); it++)
    {
        if ((*it)->IsCompleted())
        {
            localLocksCount++;
        }
        else
        {
            NodeInstance instance = (*it)->Instance;
            auto it1 = find_if(
                request.begin(),
                request.end(),
                [instance] (JoinLockTransfer const & lock) { return instance == lock.Origin && lock.IsHeld; });

            if (it1 != request.end())
            {
                localLocksCount++;
                localIncrease++;
            }
        }
    }

    if ((localLocksCount < remoteLocksCount) ||
        ((localLocksCount == remoteLocksCount) && (localIncrease > 0) && (requestorId < site_.Id)))
    {
        for (JoinLockInfo* transfer : transfers)
        {
            response.push_back(transfer->CreateTransfer());
            transfer->Reset();
        }
    }

    WriteInfo(TraceLock, site_.IdString, 
        "{0} Locks being transferred to {1}: remote/local: {2}/{3}",
        response.size(), requestorId, remoteLocksCount, localLocksCount);
}

bool JoinManager::IsRangeCovered()
{
    int count = static_cast<int>(entries_.size());
    if (count == 0)
    {
        return false;
    }

    sort(entries_.begin(), entries_.end(), [] (JoinLockInfoUPtr const & left, JoinLockInfoUPtr const & right) { return left->Id < right->Id; } );

    bool isFull = (count == FederationConfig::GetConfig().NeighborhoodSize * 2);
    if (isFull)
    {
        int index = count / 2;
        while (site_.Id.SuccDist(entries_[index]->Id) > site_.Id.SuccDist(entries_[index - 1]->Id))
        {
            JoinLockInfoUPtr temp = move(entries_[0]);
            entries_.erase(entries_.begin());
            entries_.push_back(move(temp));
        }
    }

    for (int i = 0; i < count - 1; ++i)
    {
        if (entries_[i]->GetRange().Disjoint(entries_[i + 1]->GetRange()))
        {
            return false;
        }
    }

    if (!isFull)
    {
        if (entries_[count - 1]->GetRange().Disjoint(entries_[0]->GetRange()))
        {
            return false;
        }
    }

    return true;
}

int JoinManager::GetPendingCount() const
{
    int pendingCount = 0;
    for (JoinLockInfoUPtr const & entry : entries_)
    {
        if (!entry->IsCompleted())
        {
            pendingCount++;
        }
    }

    return pendingCount;
}

void JoinManager::DumpState() const
{
    for (JoinLockInfoUPtr const & entry : entries_)
    {
        WriteInfo(TraceState, site_.IdString,
            "{0}: lastSent={1}, acquiredTime={2}, retry={3}, range={4}",
            *entry, entry->GetLastSent(), entry->GetAcquiredTime(), entry->getRetryCount(), entry->GetRange());
    }
}

bool JoinManager::CheckCompletion()
{
    int pendingCount = GetPendingCount();
    if (pendingCount > 0)
    {
        WriteInfo(TraceState, site_.IdString, "{0} pending at {1} stage", pendingCount, phase_);
        if (DateTime::Now() > lastStateDump_ + TimeSpan::FromSeconds(5))
        {
            DumpState();
            lastStateDump_ = DateTime::Now();
        }

        return false;
    }

    for (JoinLockInfoUPtr const & entry : entries_)
    {
        entry->Reset();
    }

    if (phase_ == JoiningPhase::Locking && !IsRangeCovered())
    {
        DumpState();
        WriteWarning(TraceState, site_.IdString, "Locking phase range not covered");
        return false;
    }

    JoiningPhase::Enum oldPhase = phase_;
    if (phase_ == JoiningPhase::Locking)
    {
        phase_ = JoiningPhase::EstablishingLease;
    }
    else if (phase_ == JoiningPhase::EstablishingLease)
    {
        phase_ = JoiningPhase::UnLocking;
    }
    else if (phase_ == JoiningPhase::UnLocking)
    {
        phase_ = JoiningPhase::Routing;
        entries_.clear();
    }

    WriteInfo(TracePhase, site_.IdString, "Joining phase changed from {0} to {1}", oldPhase, phase_);

    return true;
}

void JoinManager::StartNewPhase()
{
    if (phase_ == JoiningPhase::Routing)
    {
        site_.ChangePhase(NodePhase::Routing);
        timer_->Cancel();
    }
    else if (phase_ == JoiningPhase::EstablishingLease)
    {
        StartLeaseEstablish();
    }
    else
    {
        RunStateMachine();
    }
}

void JoinManager::RunStateMachine()
{
    if (site_.IsShutdown)
    {
        return;
    }

    WriteInfo(TraceAction, site_.IdString, "Executing statemachine");

    StateMachineActionCollection actions;
    bool phaseCompleted;
    {
        // Ensuring that the new nodes in the hood are transferred to local state.
        AcquireExclusiveLock grab(stateLock_);

        JoinLockInfo* lock = nullptr;
        if (phase_ == JoiningPhase::Locking)
        {
            UpdateNeighborhood(actions);

            if (entries_.size() > 0)
            {
                lock = entries_[0].get();
                if (lock->IsCompleted() || lock->GetLastSent() == StopwatchTime::Zero || (lock->RetryCount % 10) == 5)
                {
                    lock = nullptr;
                }
            }
        }
        else if (phase_ != JoiningPhase::UnLocking)
        {
            return;
        }

        if (lock)
        {
            lock->GenerateActions(phase_, actions);
        }
        else
        {
            for (JoinLockInfoUPtr const & entry : entries_)
            {
                entry->GenerateActions(phase_, actions);
            }
        }

        phaseCompleted = CheckCompletion();
    }

    // Executing actions.
    if (!actions.IsEmpty())
    {
        WriteInfo(TraceAction, site_.IdString, "Executing actions {0}", actions);
        actions.Execute(site_);
    }
    else
    {
        WriteInfo(TraceAction, site_.IdString, "No actions");
    }

    if (phaseCompleted)
    {
        StartNewPhase();
    }

    FederationConfig & config = FederationConfig::GetConfig();
    if (phase_ == JoiningPhase::UnLocking)
    {
        bool restart = (Stopwatch::Now() > insertStartTime_ + min(config.JoinLockDuration, config.TokenAcquireTimeout));
        {
            AcquireExclusiveLock grab(stateLock_);
            if (!restart)
            {
                for (JoinLockInfoUPtr const & entry : entries_)
                {
                    if (site_.Table.IsDown(entry->Instance))
                    {
                        restart = true;
                        WriteInfo(TracePhase, site_.IdString, "Restarting as neighbor went down during unlocking phase");
                        break;
                    }
                }
            }
            else
            {
                WriteWarning(TracePhase, site_.IdString, "Restarting as unlock phase timed out");
            }

            if (restart)
            {
                ResetStateForJoining();
            }
        }

        if (restart)
        {
            RestartJoining();
            return;
        }
    }
    
    if (phase_ != JoiningPhase::Routing)
    {
        timer_->Change(config.JoinStateMachineInterval);
    }
}
