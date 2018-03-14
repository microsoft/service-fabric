// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Federation/VoteProxy.h"
#include "Federation/SeedNodeProxy.h"
#include "Federation/SharableProxy.h"
#include "Federation/VoteEntry.h"
#include "Federation/VoteManager.h"
#include "Federation/ArbitrationOperation.h"
#include "Federation/TicketGapRequest.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Federation;
using namespace LeaseWrapper;
using namespace Store;

StringLiteral const TraceConfig("Config");
StringLiteral const TraceGlobalTickets("Tickets");
StringLiteral const TraceTTL("TTL");
StringLiteral const VoteTimerTag("Vote");

StringLiteral const GlobalLeaseStateExpiredTag("expired");
StringLiteral const GlobalLeaseStateAbandonedTag("abandoned");
StringLiteral const EmptyTag("");

struct CompareEntryId
{
public:
    bool operator()(VoteEntryUPtr const & entry, NodeId voteId)
    {
        return (entry->VoteId < voteId);
    }
};

VoteManager::VoteManager(SiteNode & siteNode)
    :   siteNode_(siteNode),
        entries_(),
        count_(0),
        quorumCount_(0),
        globalLeaseExpiration_(0),
        globalLeaseState_(GlobalLeaseValid),
        arbitrationTable_(siteNode.Id),
        globalLeaseExpirationWrittenToLeaseAgent_(StopwatchTime::Zero),
        configChangeHandler_(0),
        lastArbitrationTime_(StopwatchTime::Zero),
        lastTwoWayArbitrationTime_(StopwatchTime::Zero),
        continousArbitrations_(0),
        started_(false),
        lastDeltaTableCompact_(StopwatchTime::Zero)
{
}

VoteEntry* VoteManager::GetVote(NodeId voteId) const
{
    auto it = lower_bound(entries_.begin(), entries_.end(), voteId, CompareEntryId());
    if (it != entries_.end() && (*it)->VoteId == voteId)
    {
        return it->get();
    }

    return nullptr;
}

bool VoteManager::AddVote(VoteEntryUPtr && vote, bool isUpdate)
{
    ErrorCode error = vote->Initialize(!isUpdate);
    if (!error.IsSuccess())
    {
        return false;
    }

    auto it = lower_bound(entries_.begin(), entries_.end(), vote->VoteId, CompareEntryId());
    entries_.insert(it, move(vote));

    return true;
}

void VoteManager::RemoveVote(NodeId voteId)
{
    auto it = lower_bound(entries_.begin(), entries_.end(), voteId, CompareEntryId());
    if (!(*it)->CanRemove())
    {
        staleEntries_.push_back(move(*it));
    }

    entries_.erase(it);
}

bool VoteManager::LoadConfig(bool isUpdate)
{
    auto votes = FederationConfig::GetConfig().Votes;

    {
        AcquireExclusiveLock grab(lock_);

        if (isUpdate && entries_.size() == 0)
        {
            return true;
        }

        if (isUpdate && siteNode_.Phase == NodePhase::Booting)
        {
            WriteError(TraceConfig, "{0} can't update vote config during booting phase", siteNode_.Instance);
            return false;
        }

        int addCount = 0;
        for (VoteEntryConfig const & vote : votes)
        {
            auto voteId = vote.Id;
            wstring const & type = vote.Type;
            wstring const & connectionString = vote.ConnectionString;
            wstring const & ringName = vote.RingName;

            WriteInfo(TraceConfig, "load vote: {0} {1} {2} {3}", voteId, ringName, type, connectionString);

            if (ringName != siteNode_.RingName)
            {
                ASSERT_IF(ringName.empty() || siteNode_.RingName.empty(),
                    "Unexpected vote {0} for ring {1}",
                    ringName, siteNode_.RingName);
                continue;
            }

            auto existing = GetVote(voteId);
            if (isUpdate)
            {
                if (existing)
                {
                    if (type != existing->Type)
                    {
                        WriteError(TraceConfig, "Can't update vote {0} from type {1} to {2}",
                            voteId, existing->Type, type);
                        return false;
                    }
                    else if (connectionString != existing->ConnectionString)
                    {
                        RemoveVote(voteId);
                    }
                    else
                    {
                        continue;
                    }
                }
                else if (addCount >= 1)
                {
                    WriteError(TraceConfig, "Can't add more than one voter");
                    return false;
                }
                else
                {
                    addCount++;
                }
            }
            else if (existing)
            {
                WriteError(TraceConfig, "Duplicate vote configuration for {0}",
                    voteId);
                return false;
            }

            VoteProxySPtr proxy;
            if (type == Constants::SeedNodeVoteType)
            {
                proxy = make_shared<SeedNodeProxy>(NodeConfig(voteId, connectionString, L"", siteNode_.WorkingDir, ringName), siteNode_.Id);
            }
            else if (type == Constants::SqlServerVoteType)
            {
                Store::StoreFactoryParameters parameters;
                parameters.Type = Store::StoreType::Sql;
                parameters.ConnectionString = connectionString;

                auto factory = siteNode_.StoreFactory;
                ASSERT_IF(factory == nullptr, "Using SqlStore but no sql store factory provided");
                
                Store::ILocalStoreSPtr store;
                auto error = factory->CreateLocalStore(parameters, store);
                ASSERT_IF(!error.IsSuccess(), "Create of sql store cannot fail {0}", error);

                proxy = make_shared<SharableProxy>(move(store), ringName, voteId, siteNode_.Id);
            }
            else if (type == Constants::WindowsAzureVoteType)
            {
                proxy = make_shared<WindowsAzureProxy>(voteId, connectionString, ringName, siteNode_.Id);
            }
            else 
            {
                WriteError(TraceConfig, "Vote config for {0} with unknown type {1}",
                    voteId, type);
                return false;
            }

            if (!AddVote(make_unique<VoteEntry>(type, connectionString, move(proxy), siteNode_), isUpdate))
            {
                return false;
            }
        }

        int removeCount = 0;
        if (isUpdate)
        {
            for (auto it = entries_.begin(); it != entries_.end();)
            {
                if (votes.find((*it)->VoteId, siteNode_.RingName) == votes.end())
                {
                    if ((*it)->VoteId == siteNode_.Id)
                    {
                        WriteError(TraceConfig, "{0} must go down as it is no longer configured as seed node", siteNode_.Id);
                        return false;
                    }

                    if (removeCount > 0)
                    {
                        WriteError(TraceConfig, "Can't remove more than one voter");
                        return false;
                    }

                    if (!(*it)->CanRemove())
                    {
                        staleEntries_.push_back(move(*it));
                    }

                    it = entries_.erase(it);
                    removeCount++;
                }
                else
                {
                    ++it;
                }
            }
        }

        siteNode_.Table.UpdateExternalRingVotes();

        count_ = static_cast<int>(entries_.size());
        quorumCount_ = count_ / 2 + 1;

        ASSERT_IF(entries_.size() == 0,
            "Invalid vote configuration for {0}",
            this->siteNode_.Id);
    }

    if (isUpdate)
    {
        siteNode_.GetVoterStore().UpdateVoterConfig();
    }

    WriteInfo(TraceConfig,
        "{0} VoteManager loaded votes: {1}isUpdate={2}",
        siteNode_.Id, votes, isUpdate);

    RunStateMachine();

    return true;
}

bool VoteManager::Initialize()
{
    auto rootSPtr = siteNode_.GetSiteNodeSPtr();
    timer_ = Timer::Create(
        VoteTimerTag,
        [this, rootSPtr](TimerSPtr const &) { this->RunStateMachine(); },
        true);

    {
        AcquireExclusiveLock grab(lock_);
        configChangeHandler_ = FederationConfig::GetConfig().VotesEntry.AddHandler([this, rootSPtr](EventArgs const&)
        {
            bool result = this->LoadConfig(true);
            ASSERT_IF(!result, "{0} unable to apply dynamic voter configuration update", this->siteNode_.Instance);
        });
    }

    return LoadConfig(false);
}

void VoteManager::Start()
{
    {
        AcquireExclusiveLock grab(lock_);
        started_ = true;
    }

    RunStateMachine();
}

void VoteManager::Stop()
{
    if (timer_)
    {
        timer_->Cancel();
    }

    AcquireExclusiveLock grab(lock_);

    if (configChangeHandler_)
    {
        FederationConfig::GetConfig().VotesEntry.RemoveHandler(configChangeHandler_);
        configChangeHandler_ = 0;
    }

    for (VoteEntryUPtr const & entry : entries_)
    {
        entry->Dispose();
    }
}

void VoteManager::CompleteBootstrap(bool notifySeedNodes)
{
    for (VoteEntryUPtr const & entry : entries_)
    {
        entry->CompleteBootstrap(siteNode_, notifySeedNodes);
    }
}

void VoteManager::OnGlobalLeaseLost()
{
    auto rootSPtr = siteNode_.GetSiteNodeSPtr();
    Threadpool::Post([rootSPtr] { rootSPtr->OnGlobalLeaseQuorumLost(); });
}

void VoteManager::WriteGlobalLeaseExpirationToLeaseAgentCallerHoldingLock()
{
    LeaseAgent & leaseAgent = siteNode_.GetLeaseAgent();
    LONG ttlInMilliseconds;
    LONGLONG kernelCurrentTime;
    if (!leaseAgent.GetLeasingApplicationExpirationTime(&ttlInMilliseconds, &kernelCurrentTime))
    {
        WriteError(TraceGlobalTickets, "{0}: GetLeasingApplicationExpirationTime() failed with {1}", siteNode_.Id, ::GetLastError());
        return;
    }

    // !!! For correctness, globalLeaseTtl has to be calculated after reading kernelCurrentTime
    TimeSpan globalLeaseTtl = globalLeaseExpiration_ - Stopwatch::Now();
    if (globalLeaseTtl <= TimeSpan::Zero)
    {
        WriteNoise(TraceTTL,
            "{0}: will not write to lease agent with negative TTL {1}",
            siteNode_.IdString, globalLeaseTtl);
        return;
    }

    LONGLONG globalLeaseExpirationToWrite = kernelCurrentTime + globalLeaseTtl.TotalPositiveMilliseconds();
    WriteNoise(
        TraceTTL,
        "{0}: KernelCurrentTime = {1}, globalLeaseExpirationToWrite = {2}, TTL = {3}",
        siteNode_.IdString,
        kernelCurrentTime,
        globalLeaseExpirationToWrite,
        globalLeaseTtl);

    if (!leaseAgent.SetGlobalLeaseExpirationTime(globalLeaseExpirationToWrite))
    {
        WriteError(
            TraceTTL,
            "{0}: SetLeasingApplicationExpirationTime() failed with {1}",
            siteNode_.Id,
            ::GetLastError());
    }
    else
    {
        globalLeaseExpirationWrittenToLeaseAgent_ = globalLeaseExpiration_;
    }
}

void VoteManager::UpdateGlobalLeaseExpirationCallerHoldingLock()
{
    vector<StopwatchTime> expirations;
    for (VoteEntryUPtr const & entry : entries_)
    {
        expirations.push_back(entry->GlobalTicket);
    }

    sort(expirations.begin(), expirations.end());
    globalLeaseExpiration_ = expirations[expirations.size() - quorumCount_];
    VoteManagerEventSource::Events->TTL(
        siteNode_.Id,
        wformatString(globalLeaseExpiration_),
        quorumCount_,
        expirations.size());

    if (globalLeaseExpiration_ > globalLeaseExpirationWrittenToLeaseAgent_)
    {
        WriteGlobalLeaseExpirationToLeaseAgentCallerHoldingLock();
    }
}

int VoteManager::GetGlobalLeaseLevel() const
{
    FederationConfig const & config = FederationConfig::GetConfig();
    TimeSpan ttl = globalLeaseExpiration_ - Stopwatch::Now();
    if (ttl > config.BootstrapTicketLeaseDuration)
    {
        return 2;
    }
    else if (ttl > config.BootstrapTicketLeaseDuration / 3)
    {
        return 1;
    }
    else if (ttl > TimeSpan::Zero)
    {
        return 0;
    }

    return -1;
}

bool VoteManager::IsGlobalLeaseAbandoned() const
{
    return (globalLeaseState_ == GlobalLeaseAbandoned);
}

void VoteManager::UpdateGlobalTickets(__in Message & message, NodeInstance const & from)
{
    GlobalLease globalLease;
    if (message.SerializedBodySize() > 0 && message.GetBody(globalLease))
    {
        UpdateGlobalTickets(globalLease, from);
    }
}

bool VoteManager::UpdateGlobalTickets(GlobalLease & globalLease, NodeInstance const & from)
{
    bool result = false;
    StopwatchTime now = Stopwatch::Now();
    globalLease.AdjustTime();

    AcquireExclusiveLock grab(lock_);

    if (from.Id != siteNode_.Id)
    {
        result = UpdateDeltaTable(globalLease.BaseTime, now, from);
    }

    size_t i = 0;
    size_t j = 0;

    vector<VoteTicket> const & tickets = globalLease.Tickets;
    while (i < entries_.size() && j < tickets.size())
    {
        if (entries_[i]->VoteId == tickets[j].VoteId)
        {
            for (TicketGap const & gap : tickets[j].GetGaps())
            {
                if (gap.Range.ProperContains(siteNode_.Id))
                {
                    globalLeaseState_ = GlobalLeaseAbandoned;

                    WriteError(TraceGlobalTickets,
                        "{0} lost global lease on the local node with gap: {1}",
                        siteNode_.Id, gap);

                    if (siteNode_.Phase != NodePhase::Joining)
                    {
                        OnGlobalLeaseLost();
                    }

                    return result;
                }
            }

            entries_[i]->UpdateGlobalTicket(tickets[j]);
            i++;
            j++;
        }
        else if (entries_[i]->VoteId < tickets[j].VoteId)
        {
            i++;
        }
        else
        {
            j++;
        }
    }

    VoteManagerEventSource::Events->UpdateGlobalTickets(
        siteNode_.Id,
        *this,
        now.Ticks);

    UpdateGlobalLeaseExpirationCallerHoldingLock();

    return result;
}

GlobalLease VoteManager::GenerateGlobalLease(NodeInstance const & target)
{
    vector<VoteTicket> tickets;

    AcquireExclusiveLock grab(lock_);

    TimeSpan delta = InternalGetDelta(target);
    if (delta != TimeSpan::MaxValue)
    {
        InternalGenerateGlobalLease(tickets);
    }

    return GlobalLease(move(tickets), delta);
}

void VoteManager::GenerateGlobalLease(vector<VoteTicket> & tickets)
{
    AcquireExclusiveLock grab(lock_);
    InternalGenerateGlobalLease(tickets);
}

void VoteManager::InternalGenerateGlobalLease(vector<VoteTicket> & tickets)
{
    StopwatchTime now = Stopwatch::Now();

    for (VoteEntryUPtr const & entry : entries_)
    {
        entry->UpdateGaps();

        if (entry->GlobalTicket > now)
        {
            tickets.push_back(entry->IssueVoteTicket());
        }
    }
}

void VoteManager::ConvertSuperTickets()
{
    for (VoteEntryUPtr const & entry : entries_)
    {
        entry->ConvertSuperTicket();
    }
}

bool VoteManager::OnMessageReceived(NodeInstance const & from)
{
    bool result = false;

    for (VoteEntryUPtr const & entry : entries_)
    {
        if (entry->UpdateLastReceived(from))
        {
            result = true;
        }
    }

    return result;
}

void VoteManager::VotePingHandler(__in Message &, OneWayReceiverContext & oneWayReceiverContext)
{
    bool newNodeFound;
    {
        AcquireExclusiveLock grab(lock_);
        newNodeFound = OnMessageReceived(oneWayReceiverContext.From->Instance);
    }

    if (siteNode_.Phase != NodePhase::Joining)
    {
        siteNode_.Send(FederationMessage::GetVotePingReply().CreateMessage(), oneWayReceiverContext.From);
    }
    oneWayReceiverContext.Accept();

    if (newNodeFound)
    {
        RunStateMachine();
    }
}

void VoteManager::VotePingReplyHandler(__in Message &, OneWayReceiverContext & oneWayReceiverContext)
{
    {
        AcquireExclusiveLock grab(lock_);
        OnMessageReceived(oneWayReceiverContext.From->Instance);
    }

    oneWayReceiverContext.Accept();

    RunStateMachine();
}

void VoteManager::PrepareTicketsForTransfer(VoteRequest const & request, __out vector<TicketTransfer> & transfers, __in NodeInstance const & destination)
{
    TimeSpan delta = Stopwatch::Now() - request.RequestTime;
    AcquireExclusiveLock grab(lock_);
    OnMessageReceived(destination);

    if (siteNode_.Phase == NodePhase::Booting)
    {
        for (VoteEntryUPtr const & entry : entries_)
        {
            if (entry->IsSuperTicketEffective())
            {
                if (entry->VoteId < request.VoteId)
                {
                    return;
                }

                entry->TransferSuperTicket(transfers, delta);
            }
        }
    }
}

void VoteManager::VoteTransferRequestHandler(__in Message & message, OneWayReceiverContext & oneWayReceiverContext)
{
    oneWayReceiverContext.Accept();

    VoteRequest request;
    if (message.GetBody(request))
    {
        vector<TicketTransfer> transfers;
        PrepareTicketsForTransfer(request, transfers, oneWayReceiverContext.From->Instance);
        TicketTransferBody body(std::move(transfers));
        siteNode_.Send(FederationMessage::GetVoteTransferReply().CreateMessage(body), oneWayReceiverContext.From);
    }
}

void VoteManager::VoteRenewRequestHandler(__in Message & message, OneWayReceiverContext & oneWayReceiverContext)
{
	oneWayReceiverContext.Accept();

	VoteRequest request;
	if (message.GetBody(request))
	{
		VoteEntry* entry = GetVote(request.VoteId);
		if (entry != nullptr)
		{
            TimeSpan delta = Stopwatch::Now() - request.RequestTime;
            MessageUPtr reply = entry->ExtendSuperTicket(delta);
            if (reply)
            {
                siteNode_.Send(move(reply), oneWayReceiverContext.From);
            }
		}
	}
}

void VoteManager::AcceptTransferredTickets(vector<TicketTransfer> transfers, __in NodeInstance const & from)
{
    AcquireExclusiveLock grab(lock_);
    OnMessageReceived(from);

    for (TicketTransfer const & transfer : transfers)
    {
        VoteEntry* entry = GetVote(transfer.VoteId);
        if (entry)
        {
            entry->AcceptTransfer(transfer);
        }
    }

    if (siteNode_.Phase > NodePhase::Booting)
    {
        ConvertSuperTickets();
    }
}

void VoteManager::VoteTransferReplyHandler(__in Message & message, OneWayReceiverContext & oneWayReceiverContext)
{
    oneWayReceiverContext.Accept();

    vector<TicketTransfer> transfers;
    if (TicketTransferBody::ReadFromMessage(message, transfers))
    {
        AcceptTransferredTickets(transfers, oneWayReceiverContext.From->Instance);

        RunStateMachine();
    }
}

void VoteManager::VoteRenewReplyHandler(__in Message & message, OneWayReceiverContext & oneWayReceiverContext)
{
	oneWayReceiverContext.Accept();

    TicketTransfer body;
    if (message.GetBody<TicketTransfer>(body))
    {
        VoteEntry* entry = GetVote(body.VoteId);
        if (entry && entry->IsSuperTicketOwned())
        {
            body.AdjustTime();
            entry->AcceptTransfer(body);

            RunStateMachine();
        }
    }
}

Common::TimeSpan VoteManager::CheckBootstrapState(StateMachineActionCollection & actions, NodePhase::Enum & phase)
{
    int ownerCount = 0;
    int routingNodeCount = siteNode_.Table.GetRoutingNodeCount();

    if (routingNodeCount > 0)
    {
        CompleteBootstrap(false);
        phase = NodePhase::Joining;

        TraceInfo(TraceTaskCodes::Bootstrap, "Complete",
            "{0} completed bootstrap phase with {1} routing nodes",
            siteNode_.Id, routingNodeCount);

        return TimeSpan::MaxValue;
    }

    int firstOwnerIndex = -1;
    for (int i = 0; i < count_; i++)
    {
        VoteEntryUPtr const & entry = entries_[i];
        if (entry->IsSharable)
        {
            entry->CheckOwnershipInBootstrap();
        }
        if (entry->IsVoteOwned())
        {
            if (firstOwnerIndex < 0)
            {
                firstOwnerIndex = i;
            }

            ownerCount++;
        }
    }

    TraceInfo(TraceTaskCodes::Bootstrap, "State",
        "{0} owned {1} votes, firstIndex {2}: {3}",
        siteNode_.Id, ownerCount, firstOwnerIndex, *this);

    if (ownerCount >= quorumCount_)
    {
        ConvertSuperTickets();
        for (VoteEntryUPtr const & entry : entries_)
        {
            entry->IssueGlobalTicket();
        }
        UpdateGlobalLeaseExpirationCallerHoldingLock();

        phase = NodePhase::Routing;

        TraceInfo(TraceTaskCodes::Bootstrap, "Complete",
            "{0} bootstrapped the ring with {1} votes: {2}",
            siteNode_.Id, ownerCount, *this);

        return TimeSpan::MaxValue;
    }

    bool tryFormRing = (ownerCount > 0 && firstOwnerIndex <= (count_ - 1) / 2);
    if (tryFormRing)
    {
        NodeId requestId = entries_[firstOwnerIndex]->VoteId;
        for (int i = firstOwnerIndex + 1; i < count_; i++)
        {
            VoteEntryUPtr const & entry = entries_[i];
            if (entry->CanSendBootstrapRequest())
            {
                if (entry->IsSuperTicketOwned())
                {
                    if (entry->CanSendRenewRequest())
                    {
                        actions.Add(make_unique<SendMessageAction>(FederationMessage::GetVoteRenewRequest().CreateMessage(VoteRequest(entry->VoteId)), entry->Owner));
                        entry->UpdateLastSend();
                    }
                    else if (entry->CanSendVoteTransferRequest())
                    {
                        actions.Add(make_unique<SendMessageAction>(FederationMessage::GetVoteTransferRequest().CreateMessage(VoteRequest(requestId)), entry->Owner));
                        entry->UpdateLastSend();
                    }
                }
                else
                {
                    if (entry->CanSendVoteTransferRequest())
                    {
                        actions.Add(make_unique<SendMessageAction>(FederationMessage::GetVoteTransferRequest().CreateMessage(VoteRequest(requestId)), entry->Owner));
                    }
                    else
                    {
                        actions.Add(make_unique<SendMessageAction>(FederationMessage::GetVotePing().CreateMessage(), entry->Owner));
                    }

                    entry->UpdateLastSend();
                }
            }
        }
    }

    VoteEntry* candidate = nullptr;
    int startIndex = Random().Next(count_);
    for (int i = 0; i < count_; i++)
    {
        int index = (startIndex + i) % count_;
        VoteEntryUPtr const & entry = entries_[index];
        if ((!tryFormRing || index < firstOwnerIndex) &&
            entry->CanSendBootstrapRequest() &&
            (!candidate || candidate->LastSend > entry->LastSend))
        {
            candidate = entry.get();
        }
    }

    if (candidate)
    {
        actions.Add(make_unique<SendMessageAction>(FederationMessage::GetVotePing().CreateMessage(), candidate->Owner));
        candidate->UpdateLastSend();
    }

    return FederationConfig::GetConfig().BootstrapStateMachineInterval;
}

TimeSpan VoteManager::CheckRoutingState(StateMachineActionCollection &)
{
    RoutingToken token = siteNode_.Table.GetRoutingToken();
    TimeSpan interval = max(TimeSpan::Zero, globalLeaseExpiration_ - Stopwatch::Now());
    
    for (VoteEntryUPtr const & entry : entries_)
    {
        if (entry->IsSharable)
        {
            TimeSpan result = entry->CheckOwnershipAfterBootstrap(token);
            if (result < interval)
            {
                interval = result;
            }
        }
    }

    return interval;
}

struct TicketUpdate
{
    VoteEntry* m_pEntry;
    StopwatchTime m_value;
    bool m_result;

    TicketUpdate(VoteEntry* pEntry, StopwatchTime value)
        : m_pEntry(pEntry), m_value(value), m_result(false)
    {
    }
};

void VoteManager::RunStateMachine()
{
    StateMachineActionCollection actions;
    vector<TicketUpdate> updates;
    {
        TimeSpan interval;

        AcquireExclusiveLock grab(lock_);
        if (siteNode_.IsShutdown || globalLeaseState_ != GlobalLeaseValid || !started_)
        {
            return;
        }

        StopwatchTime now = Stopwatch::Now();
        if (siteNode_.Phase >= NodePhase::Inserting && now >= globalLeaseExpiration_)
        {
            globalLeaseState_ = GlobalLeaseExpired;

            WriteError(TraceGlobalTickets,
                "{0} lost global lease on the local node: {1}, Ticks {2}, phase {3}",
                siteNode_.Id, *this, now.Ticks, siteNode_.Phase);

            OnGlobalLeaseLost();
            return;
        }

        if (siteNode_.Phase == NodePhase::Booting)
        {
            NodePhase::Enum newPhase = NodePhase::Booting;
            interval = CheckBootstrapState(actions, newPhase);
            if (newPhase == NodePhase::Routing)
            {
                siteNode_.Table.Bootstrap();
                CompleteBootstrap(true);
            }
            else if (newPhase != NodePhase::Booting)
            {
                siteNode_.ChangePhase(newPhase);
            }
        }
        else
        {
            interval = CheckRoutingState(actions);
            if (siteNode_.IsRouting)
            {
                for (auto it = entries_.begin(); it != entries_.end(); ++it)
                {
                    StopwatchTime newTicket = (*it)->PreUpdateGlobalTicket();
                    if (newTicket != StopwatchTime::Zero)
                    {
                        updates.push_back(TicketUpdate(it->get(), newTicket));
                    }
                }
            }
        }

        for (auto it = staleEntries_.begin(); it != staleEntries_.end();)
        {
            if ((*it)->CanRemove())
            {
                it = staleEntries_.erase(it);
            }
            else
            {
                ++it;
            }
        }

        interval = min(interval, arbitrationTable_.Compact(siteNode_));
        interval = min(interval, FederationConfig::GetConfig().GlobalTicketRenewInterval);

        timer_->ChangeWithLowerBoundDelay(interval);
    }

    actions.Execute(siteNode_);

    for (auto it = updates.begin(); it != updates.end(); ++it)
    {
        if (it->m_pEntry->Proxy->SetGlobalTicket(it->m_value).IsSuccess())
        {
            it->m_result = true;
        }
    }

    if (updates.size() > 0)
    {
        AcquireExclusiveLock grab(lock_);
        for (auto it = updates.begin(); it != updates.end(); ++it)
        {
            it->m_pEntry->PostUpdateGlobalTicket(it->m_value, it->m_result);
        }

        UpdateGlobalLeaseExpirationCallerHoldingLock();
    }
}

void VoteManager::OnRoutingTokenChanged()
{
    RunStateMachine();
}

void VoteManager::ArbitrateRequestHandler(__in Message & message, RequestReceiverContext & requestReceiverContext)
{
    ArbitrationRequestBody request;
    if (message.GetBody(request))
    {
        request.Normalize();

        if (requestReceiverContext.From && requestReceiverContext.FromInstance < requestReceiverContext.From->Instance)
        {
            ArbitrationTable::WriteInfo("Ignore", "{0} ignored request {1} from old instance {2}",
                siteNode_.IdString, request, requestReceiverContext.FromInstance);

            requestReceiverContext.Ignore();
            return;
        }

        MessageUPtr reply = arbitrationTable_.ProcessRequest(siteNode_, request, requestReceiverContext.From);
        if (reply)
        {
            requestReceiverContext.Reply(move(reply));
        }
        else
        {
            requestReceiverContext.Ignore();
        }
    }
    else
    {
        requestReceiverContext.InternalReject(message.FaultErrorCodeValue);
    }
}

void VoteManager::ArbitrateOneWayMessageHandler(__in Message & message, OneWayReceiverContext & oneWayReceiverContext)
{
    oneWayReceiverContext.Accept();

    ArbitrationRequestBody request;
    if (message.GetBody(request))
    {
        arbitrationTable_.ProcessRequest(siteNode_, request, oneWayReceiverContext.From);
    }
}

void VoteManager::Arbitrate(
    LeaseAgentInstance const & local,
    LeaseAgentInstance const & remote,
    TimeSpan remoteTTL,
    TimeSpan historyNeeded,
    int64 monitorLeaseInstance,
    int64 subjectLeaseInstance,
    int64 extraData,
    ArbitrationType::Enum type)
{
    FederationConfig const & config = FederationConfig::GetConfig();
    TimeSpan delay = TimeSpan::Zero;
    ArbitrationOperationSPtr operation;
    vector<VoteProxySPtr> votes;
    {
        AcquireExclusiveLock grab(lock_);

        StopwatchTime now = Stopwatch::Now();
        if (type == ArbitrationType::KeepAlive)
        {
            if (now < lastArbitrationTime_ + config.LeaseDuration - TimeSpan::FromTicks(config.LeaseSuspendTimeout.Ticks * 3 / 2))
            {
                WriteInfo("Arbitration", siteNode_.IdString,
                    "Ignore keep alive request, last arbitration {0}",
                    lastArbitrationTime_);
                return;
            }
        }
        else if (type == ArbitrationType::TwoWaySimple || type == ArbitrationType::TwoWayExtended)
        {
            if (now - lastTwoWayArbitrationTime_ > config.LeaseDuration)
            {
                continousArbitrations_ = 1;
            }
            else
            {
                continousArbitrations_++;
            }

            lastTwoWayArbitrationTime_ = now;

            if (continousArbitrations_ == 1 || type == ArbitrationType::TwoWayExtended)
            {
                delay = TimeSpan::Zero;
            }
            else if (continousArbitrations_ == 2)
            {
                delay = TimeSpan::FromTicks(config.LeaseSuspendTimeout.Ticks / 2);
            }
            else if (continousArbitrations_ == 3)
            {
                delay = config.LeaseSuspendTimeout - TimeSpan::FromMilliseconds(100.0);
            }
            else
            {
                delay = min(config.LeaseSuspendTimeout + TimeSpan::FromTicks(config.ArbitrationRequestDelay.Ticks * (continousArbitrations_ - 3)), config.ArbitrationTimeout - config.ArbitrationRequestDelay);
            }
        }

        lastArbitrationTime_ = now;

        for (VoteEntryUPtr const & entry : entries_)
        {
            votes.push_back(entry->Proxy);
        }

        operation = make_shared<ArbitrationOperation>(
            siteNode_.GetSiteNodeSPtr(),
            move(votes),
            local,
            remote,
            remoteTTL,
            historyNeeded,
            monitorLeaseInstance,
            subjectLeaseInstance,
            extraData,
            type);

        if (ArbitrationOperation::HasReply(type))
        {
            arbitrationOperations_.push_back(operation);
        }
    }

    if (delay > TimeSpan::Zero)
    {
        WriteInfo("Arbitration", siteNode_.IdString, "Delay arbitration {0}-{1} for {2}", local, remote, delay);
        Threadpool::Post([operation] { operation->Start(operation); }, delay);
    }
    else
    {
        operation->Start(operation);
    }
}

void VoteManager::TicketGapRequestHandler(__in Message & message, RequestReceiverContext & requestReceiverContext)
{
    TicketGapRequest request;
    if (!message.GetBody<TicketGapRequest>(request))
    {
        requestReceiverContext.InternalReject(message.FaultErrorCodeValue);
        return;
    }

    {
        AcquireExclusiveLock grab(lock_);

        if (request.Range.ProperContains(siteNode_.Id))
        {
            globalLeaseState_ = GlobalLeaseAbandoned;

            WriteError(TraceGlobalTickets,
                "{0} lost global lease on the local node with gap request {1} from {2}",
                siteNode_.Id, request, requestReceiverContext.From);

            requestReceiverContext.InternalReject(ErrorCodeValue::OperationFailed);

            OnGlobalLeaseLost();

            return;
        }

        for (VoteEntryUPtr const & entry : entries_)
        {
            if (entry->IsAcquired)
            {
                entry->CreateTicketGap(request.Range);
            }
        }
    }

    vector<VoteTicket> tickets;
    GenerateGlobalLease(tickets);

    TimeSpan delta = Stopwatch::Now() - request.RequestTime;
    requestReceiverContext.Reply(FederationMessage::GetTicketGapReply().CreateMessage(GlobalLease(move(tickets), delta)));
}

bool VoteManager::CheckGapRecovery(NodeIdRange const & range, vector<NodeId> & pendingVotes)
{
    vector<TimeRange> effectiveGaps;
	wstring gaps;
	StringWriter w(gaps);

    AcquireExclusiveLock grab(lock_);

    for (VoteEntryUPtr const & entry : entries_)
    {
        const TicketGap* gap = entry->GetGap(range);
        if (gap)
        {
			w.Write("{0}: {1} {2}\r\n", *entry, *gap, gap->IsEffective());
            if (gap->IsEffective())
            {
                effectiveGaps.push_back(gap->Interval);
            }
        }
        else
        {
            pendingVotes.push_back(entry->VoteId);
        }
    }

	WriteInfo("TicketGap", "{0}", gaps);

    if (static_cast<int>(effectiveGaps.size()) < quorumCount_)
    {
        return false;
    }

    vector<StopwatchTime> timeRanges;
    vector<int> counts;

    for (TimeRange const & gap : effectiveGaps)
    {
        timeRanges.push_back(gap.Begin);
        timeRanges.push_back(gap.End);
        counts.push_back(0);
        counts.push_back(0);
    }

    sort(timeRanges.begin(), timeRanges.end());

    for (TimeRange const & gap : effectiveGaps)
    {
        int startIndex = static_cast<int>(find(timeRanges.begin(), timeRanges.end(), gap.Begin) - timeRanges.begin());
        int endIndex = static_cast<int>(find(timeRanges.begin(), timeRanges.end(), gap.End) - timeRanges.begin());

        for (int i = startIndex; i < endIndex; i++)
        {
            if (++counts[i] >= quorumCount_)
            {
                return true;
            }
        }
    }

    return false;
}

void VoteManager::DelayedArbitrateReplyHandler(__in Message & message, OneWayReceiverContext & oneWayReceiverContext)
{
    oneWayReceiverContext.Accept();

    ArbitrationOperationSPtr operation = nullptr;
    DelayedArbitrationReplyBody body;
    if (message.GetBody<DelayedArbitrationReplyBody>(body))
    {
        AcquireReadLock grab(lock_);

        for (ArbitrationOperationSPtr const & op : arbitrationOperations_)
        {
            if (op->Matches(body))
            {
                operation = op;
            }
        }
    }

    if (operation)
    {
        operation->ProcessDelayedReply(operation, body, oneWayReceiverContext.FromInstance);
    }
}

void VoteManager::RemoveArbitrationOperation(ArbitrationOperationSPtr const & operation)
{
    AcquireWriteLock grab(lock_);
    for (auto it = arbitrationOperations_.begin(); it != arbitrationOperations_.end(); ++it)
    {
        if (it->get() == operation.get())
        {
            arbitrationOperations_.erase(it);
            return;
        }
    }
}

void VoteManager::WriteTo(TextWriter& w, FormatOptions const & option) const
{
    if (option.formatString == "a")
    {
        arbitrationTable_.Dump(w, option);
        return;
    }

    if (globalLeaseState_ == GlobalLeaseExpired)
    {
        w.Write("expired\r\n");
    }
    else if (globalLeaseState_ == GlobalLeaseAbandoned)
    {
        w.Write("abandoned\r\n");
    }
    else
    {
        w.Write("\r\n");
    }

    for (VoteEntryUPtr const & entry : entries_)
    {
        if (siteNode_.Phase == NodePhase::Booting)
        {
            w.Write("{0}:{1},{2},{3}\r\n",
                *entry, entry->IsAcquired, entry->SuperTicket, entry->GlobalTicket);
        }
        else
        {
            w.Write("{0}:{1},{2}/{3}\r\n",
                *entry, entry->IsAcquired, entry->GlobalTicket, entry->GlobalTicket.Ticks);
        }
    }
}

void VoteManager::WriteToEtw(uint16 contextSequenceId) const 
{
    StringLiteral const * tag;
    if (globalLeaseState_ == GlobalLeaseExpired)
    {
        tag = &GlobalLeaseStateExpiredTag;
    }
    else if (globalLeaseState_ == GlobalLeaseAbandoned)
    {
        tag = &GlobalLeaseStateAbandonedTag;
    }
    else
    {
        tag = &EmptyTag;
    }

    VoteManagerEventSource::Events->VoteManager(
        contextSequenceId,
        *tag,
        entries_);
}

int VoteManager::GetSeedNodes(vector<NodeId> & seedNodes) const
{
    AcquireReadLock grab(lock_);
    return GetSeedNodesCallerHoldingLock(seedNodes);
}

int VoteManager::GetSeedNodesCallerHoldingLock(vector<NodeId> & seedNodes) const
{
    for (VoteEntryUPtr const & entry : entries_)
    {
        if (!entry->IsSharable)
        {
            seedNodes.push_back(entry->VoteId);
        }
    }

    return count_;
}

bool VoteManager::IsSeedNode(NodeId nodeId, wstring const & ringName)
{
    auto votes = FederationConfig::GetConfig().Votes;
    for (auto it = votes.begin(); it != votes.end(); ++it)
    {
        if ((nodeId == it->Id) &&
            (ringName == it->RingName) &&
            (it->Type == Constants::SeedNodeVoteType || it->Type == Constants::WindowsAzureVoteType))
        {
            return true;
        }
    }

    return false;
}

TimeSpan VoteManager::GetDelta(NodeInstance const & target)
{
    AcquireWriteLock grab(lock_);
    return InternalGetDelta(target);
}

TimeSpan VoteManager::InternalGetDelta(NodeInstance const & target)
{
    TimeSpan result = TimeSpan::MaxValue;

    StopwatchTime now = Stopwatch::Now();
    StopwatchTime bound = now - FederationConfig::GetConfig().GlobalTicketLeaseDuration;
    if (lastDeltaTableCompact_ < bound)
    {
        for (auto it = deltaTable_.begin(); it != deltaTable_.end();)
        {
            if (it->second.LastUpdated < bound)
            {
                it = deltaTable_.erase(it);
            }
            else
            {
                if (it->first == target.Id && it->second.Instance == target.InstanceId)
                {
                    result = it->second.Delta;
                }

                ++it;
            }
        }

        lastDeltaTableCompact_ = now;
    }
    else
    {
        auto it = deltaTable_.find(target.Id);
        if (it != deltaTable_.end() && it->second.Instance == target.InstanceId)
        {
            result = it->second.Delta;
        }
    }

    return result;
}

bool VoteManager::UpdateDeltaTable(StopwatchTime requestTime, StopwatchTime now, NodeInstance const & from)
{
    bool result;
    TimeSpan adjustedTime = now - requestTime;

    auto it = deltaTable_.find(from.Id);
    if (it == deltaTable_.end())
    {
        deltaTable_.insert(make_pair(from.Id, DeltaTableEntry(from.InstanceId, adjustedTime, now)));
        result = true;
    }
    else if (it->second.Instance > from.InstanceId)
    {
        return false;
    }
    else
    {
        result = (it->second.Instance < from.InstanceId);

        it->second.Instance = from.InstanceId;
        it->second.Delta = adjustedTime;
        it->second.LastUpdated = now;
    }

    return result;
}
