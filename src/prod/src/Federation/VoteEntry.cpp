// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Federation/VoteEntry.h"
#include "Federation/VoteRequest.h"

using namespace Common;
using namespace std;
using namespace Transport;
using namespace Federation;

StringLiteral const TraceGap("Gap");

VoteEntry::VoteEntry(wstring const & type, wstring const & connectionString, VoteProxySPtr && proxy, SiteNode & siteNode)
    :   type_(type),
        connectionString_(connectionString),
        proxy_(move(proxy)),
        siteNode_(siteNode),
        globalTicket_(0),
        superTicket_(0),
        isTokenOwned_(false),
        isUpdatingGlobalTicket_(false),
        lastRenew_(0),
        lastFailedAcquire_(0),
        lastSend_(0),
        lastReceive_(0)
{
}

VoteEntry::~VoteEntry()
{
    Dispose();
}

ErrorCode VoteEntry::Initialize(bool acquireSharedVoter)
{
    ErrorCode proxyOpenResult = proxy_->Open();
    if (!proxyOpenResult.IsSuccess())
    {
        return proxyOpenResult;
    }

    if (!proxy_->IsSharable)
    {
        ErrorCode error = proxy_->Acquire(siteNode_, false);
        if (VoteId == siteNode_.Id)
        {
            if (!error.IsSuccess())
            {
                return error;
            }

            globalTicket_ = proxy_->GetGlobalTicket();
        }
        else
        {
            ASSERT_IF(error.ReadValue() != ErrorCodeValue::OwnerExists,
                "{0} acquire seed vote unexpected error {1}",
                *this, error);
        }
    }
    else if (acquireSharedVoter)
    {
        Acquire();
    }

    return ErrorCodeValue::Success;
}

void VoteEntry::Dispose()
{
    proxy_->Close();
}

void VoteEntry::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.Write("{0}@{1}", VoteId, siteNode_.Id);
}

void VoteEntry::WriteToEtw(uint16 contextSequenceId) const
{
    VoteManagerEventSource::Events->VoteEntry(
        contextSequenceId,
        VoteId,
        siteNode_.Id,
        IsAcquired,
        wformatString(GlobalTicket),
        GlobalTicket.Ticks);
}

bool VoteEntry::IsSuperTicketOwned() const
{
    if (superTicket_ > Stopwatch::Now())
    {
        return true;
    }

    return (IsAcquired && proxy_->GetSuperTicket() < Stopwatch::Now());
}

bool VoteEntry::IsSuperTicketEffective() const
{
    if (superTicket_ > Stopwatch::Now() + FederationConfig::GetConfig().BootstrapTicketAcquireLimit)
    {
        return true;
    }

    return (IsAcquired && proxy_->GetSuperTicket() < Stopwatch::Now());
}

bool VoteEntry::IsVoteOwned() const
{
    return IsSuperTicketEffective() && globalTicket_ < Stopwatch::Now();
}

bool VoteEntry::CanSendBootstrapRequest() const
{
    return (!IsAcquired &&
            Owner != nullptr &&
            lastSend_ + FederationConfig::GetConfig().BootstrapVoteRequestInterval < Stopwatch::Now());
}

bool VoteEntry::CanSendRenewRequest() const
{
    StopwatchTime now = Stopwatch::Now();
    FederationConfig const & config = FederationConfig::GetConfig();

    return (superTicket_ > now + config.BootstrapVoteRequestInterval &&
            superTicket_ < now + TimeSpan::FromTicks(config.BootstrapTicketLeaseDuration.Ticks / 2));
}

bool VoteEntry::CanSendVoteTransferRequest() const
{
    return (proxy_->IsSharable || lastReceive_ != StopwatchTime::Zero);
}

bool VoteEntry::CanRemove() const
{
    return !isUpdatingGlobalTicket_;
}

bool VoteEntry::SetGlobalTicket(StopwatchTime globalTicket)
{
    ErrorCode error = proxy_->SetGlobalTicket(globalTicket);
    bool result = error.IsSuccess();

    PostUpdateGlobalTicket(globalTicket, result);

    return result;
}

bool VoteEntry::SetSuperTicket(StopwatchTime superTicket)
{
    ErrorCode error = proxy_->SetSuperTicket(superTicket);
    return error.IsSuccess();
}

void VoteEntry::CompleteBootstrap(SiteNode & siteNode, bool notifySeedNodes)
{
    if (notifySeedNodes && Owner->Id != siteNode.Id)
    {
        siteNode.Send(FederationMessage::GetVotePingReply().CreateMessage(), Owner);
    }

    proxy_->ResetOwner();
}

void VoteEntry::UpdateLastSend()
{
    lastSend_ = Stopwatch::Now();
}

bool VoteEntry::UpdateLastReceived(NodeInstance const & from)
{
    bool result = false;

    if (Owner && Owner->Id == from.Id)
    {
        if (lastReceive_ == StopwatchTime::Zero)
        {
            lastSend_ = StopwatchTime::Zero;
            result = true;
        }

        lastReceive_ = Stopwatch::Now();
    }

    return result;
}

void VoteEntry::UpdateGlobalTicket(VoteTicket const & ticket)
{
    FederationConfig & config = FederationConfig::GetConfig();
    StopwatchTime expireTime = ticket.ExpireTime;

    StopwatchTime now = Stopwatch::Now();
    if (IsAcquired)
    {
        if (expireTime <= proxy_->GetGlobalTicket())
        {
            return;
        }

        TraceInfo(TraceTaskCodes::Bootstrap, "Consume",
            "{0} super ticket {1} consumed, global ticket {2}/{3}",
            *this, proxy_->GetSuperTicket(), expireTime, proxy_->GetGlobalTicket());

        StopwatchTime newGlobalTicket = max(proxy_->GetSuperTicket(), now + config.GlobalTicketLeaseDuration);

        bool updated = SetGlobalTicket(newGlobalTicket);
        // If update failed, we can still update in-memory ticket based on propagation.
        if (!updated)
        {
            globalTicket_ = expireTime;
        }
    }
    else
    {
        if (expireTime <= globalTicket_)
        {
            return;
        }

        globalTicket_ = expireTime;

        for (TicketGap const & gap : ticket.GetGaps())
        {
            ProcessTicketGap(gap);
        }
    }
}

StopwatchTime VoteEntry::PreUpdateGlobalTicket()
{
    StopwatchTime now = Stopwatch::Now();
    StopwatchTime issuedSuperTicket = proxy_->GetSuperTicket();
    if (!isUpdatingGlobalTicket_ && IsAcquired && (issuedSuperTicket <= globalTicket_ || issuedSuperTicket < now))
    {
        FederationConfig const & config = FederationConfig::GetConfig();
        StopwatchTime result = now + config.GlobalTicketLeaseDuration;
        if (result > globalTicket_ + config.GlobalTicketRenewInterval)
        {
            isUpdatingGlobalTicket_ = true;
            return result;
        }
    }

    return StopwatchTime::Zero;
}

void VoteEntry::PostUpdateGlobalTicket(StopwatchTime value, bool updated)
{
    if (updated)
    {
        proxy_->PostSetGlobalTicket(value);
        globalTicket_ = value;
    }

    isUpdatingGlobalTicket_ = false;
}

void VoteEntry::IssueGlobalTicket()
{
    StopwatchTime newTicket = PreUpdateGlobalTicket();
    if (newTicket != StopwatchTime::Zero)
    {
        SetGlobalTicket(newTicket);
    }
}

void VoteEntry::UpdateGaps()
{
    auto end = remove_if(gaps_.begin(), gaps_.end(), [](TicketGap const & gap) { return gap.IsExpired(); });
    if (end != gaps_.end())
    {
        for (auto it = end; it != gaps_.end(); it++)
        {
            VoteManager::WriteInfo(TraceGap,
                "{0} removing gap {1}",
                *this, *it);
        }

        gaps_.erase(end, gaps_.end());
    }

    if (IsAcquired)
    {
        for (auto it = gaps_.begin(); it != gaps_.end(); it++)
        {
            it->UpdateInterval();
        }
    }
}

VoteTicket VoteEntry::IssueVoteTicket()
{
    VoteTicket ticket(VoteId, globalTicket_);

    for (TicketGap const & gap : gaps_)
    {
        ticket.AddGap(gap);
    }

    return ticket;
}

void VoteEntry::ConvertSuperTicket()
{
    if (!IsAcquired && IsVoteOwned() && globalTicket_ < superTicket_)
    {
        globalTicket_ = superTicket_;
        TraceInfo(TraceTaskCodes::Bootstrap, "Convert",
            "{0} converted super ticket {0} to global ticket",
            *this, superTicket_);
    }
}

void VoteEntry::AcceptTransfer(TicketTransfer const & transfer)
{
    if (transfer.GlobalTicket > globalTicket_)
    {
        globalTicket_ = transfer.GlobalTicket;
    }

    if (transfer.SuperTicket > superTicket_)
    {
        superTicket_ = transfer.SuperTicket;

        TraceInfo(TraceTaskCodes::Bootstrap, "Transfer",
            "{0} accepted super ticket {1} with global ticket {2}",
            *this, superTicket_, globalTicket_);
    }
}

void VoteEntry::TransferSuperTicket(std::vector<TicketTransfer> & transfers, TimeSpan delta)
{
    FederationConfig & config = FederationConfig::GetConfig();

    StopwatchTime superTicket;
    if (IsAcquired)
    {
        superTicket = Stopwatch::Now() + config.BootstrapTicketLeaseDuration;
        TimeSpan remainingTTL = superTicket - globalTicket_;
        if (remainingTTL < config.BootstrapTicketAcquireLimit ||
            !SetSuperTicket(superTicket))
        {
            return;
        }
    }
    else
    {
        superTicket = superTicket_;
        superTicket_ = StopwatchTime(0);
    }

    transfers.push_back(TicketTransfer(VoteId, superTicket, globalTicket_, delta));
}

MessageUPtr VoteEntry::ExtendSuperTicket(TimeSpan delta)
{
    if (!IsAcquired || IsSuperTicketOwned())
    {
        return nullptr;
    }

    StopwatchTime superTicket = Stopwatch::Now() + FederationConfig::GetConfig().BootstrapTicketLeaseDuration;
    if (!SetSuperTicket(superTicket))
    {
        return nullptr;
    }

    TraceInfo(TraceTaskCodes::Bootstrap, "Extend",
        "{0} extended super ticket to {1}",
        *this, superTicket_);

    return FederationMessage::GetVoteRenewReply().CreateMessage(TicketTransfer(VoteId, superTicket, globalTicket_, delta));
}

void VoteEntry::Acquire()
{
    ErrorCode error = proxy_->Acquire(siteNode_, siteNode_.Phase != NodePhase::Booting);
    if (error.IsSuccess())
    {
        TraceInfo(TraceTaskCodes::Bootstrap, "Acquire",
            "Proxy {0} has been acquired and is owned by self",
            proxy_->ProxyId);

        lastRenew_ = Stopwatch::Now();
        lastFailedAcquire_ = StopwatchTime(0);
    }
    else
    {
        if (error.IsError(ErrorCodeValue::OwnerExists))
        {
            TraceInfo(TraceTaskCodes::Bootstrap, "Acquire",
                "Proxy {0} has been acquired and is owned by {1}",
                proxy_->VoteId, proxy_->Owner->Id);
        }
        else
        {
            TraceWarning(TraceTaskCodes::Bootstrap, "Acquire",
                "Proxy {0} acquire failed: {1}",
                proxy_->ProxyId, error);
        }

        lastFailedAcquire_ = Stopwatch::Now();
    }

    if (globalTicket_ < proxy_->GetGlobalTicket())
    {
        globalTicket_ = proxy_->GetGlobalTicket();
    }
    // If it is the other way around, we could execute the consume code path
    // as an optimization.
}

void VoteEntry::CheckOwnershipInBootstrap()
{
    FederationConfig & config = FederationConfig::GetConfig();
    StopwatchTime now = Stopwatch::Now();

    bool acquireNeeded;
    if (IsAcquired)
    {
        TimeSpan elapsed = now - lastRenew_;
        acquireNeeded = (elapsed.Ticks > config.VoteOwnershipLeaseInterval.Ticks / 2);
    }
    else
    {
        acquireNeeded = ((now > lastFailedAcquire_ + config.BootstrapVoteAcquireRetryInterval) &&
                         (!this->IsSuperTicketEffective()) &&
                         ((Owner == nullptr) ||
                          ((lastReceive_ < lastSend_) &&
                           (now - lastSend_ > config.BootstrapVoteRequestInterval))));
    }

    // TODO: consider to put this into an action
    if (acquireNeeded)
    {
        Acquire();
    }
}

TimeSpan VoteEntry::CheckOwnershipAfterBootstrap(RoutingToken const & token)
{
    bool isOwner = token.Contains(proxy_->VoteId);
    if (isOwner != isTokenOwned_)
    {
        lastFailedAcquire_ = StopwatchTime(0);
        isTokenOwned_ = isOwner;
    }

    if (isOwner && !IsAcquired)
    {
        FederationConfig & config = FederationConfig::GetConfig();
        TimeSpan result = lastFailedAcquire_ + config.BootstrapVoteAcquireRetryInterval - Stopwatch::Now();
        if (result > TimeSpan::Zero)
        {
            return result;
        }

        Acquire();

        if (!IsAcquired)
        {
            return config.BootstrapVoteAcquireRetryInterval;
        }
    }

    return TimeSpan::MaxValue;
}

void VoteEntry::CreateTicketGap(NodeIdRange const & requestRange)
{
    NodeIdRange range = requestRange;
    for (auto it = gaps_.begin(); it != gaps_.end();)
    {
        if (it->Range.Contains(range))
        {
            VoteManager::WriteWarning(TraceGap,
                "{0} rejected new gap {1}, current {2}",
                *this, range, it->Range);
            return;
        }

        if (it->Range.ProperDisjoint(range))
        {
            it++;
        }
        else
        {
            VoteManager::WriteInfo(TraceGap,
                "{0} removing overlapped gap {1} with new gap {2}",
                *this, it->Range, range);

            range = NodeIdRange::Merge(range, it->Range);
            it = gaps_.erase(it);
        }
    }

    if (static_cast<int>(gaps_.size()) < FederationConfig::GetConfig().MaxGapsInCluster)
    {
        gaps_.push_back(TicketGap(range, TimeRange(globalTicket_, Stopwatch::Now())));

        VoteManager::WriteInfo(TraceGap,
            "{0} created new gap {1} from {2}, count {3}",
            *this, range, globalTicket_, gaps_.size());
    }
    else
    {
        VoteManager::WriteWarning(TraceGap,
            "{0} rejected new gap {1}, count {2}",
            *this, range, gaps_.size());
    }
}

void VoteEntry::ProcessTicketGap(TicketGap const & gap)
{
    for (auto it = gaps_.begin(); it != gaps_.end();)
    {
        if (it->Merge(gap))
        {
            return;
        }

        if (it->Range.Contains(gap.Range) || it->Range.ProperDisjoint(gap.Range))
        {
            it++;
        }
        else
        {
            it = gaps_.erase(it);
        }
    }

    gaps_.push_back(gap);

    VoteManager::WriteInfo(TraceGap,
        "{0} added new gap {1}",
        *this, gap.Range);
}

const TicketGap* VoteEntry::GetGap(NodeIdRange range)
{
    for (TicketGap const & gap : gaps_)
    {
        if (gap.Range == range)
        {
            return &gap;
        }
    }

    return nullptr;
}
