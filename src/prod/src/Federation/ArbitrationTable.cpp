// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Federation/ArbitrationTable.h"
#include "Federation/VoteRequest.h"

using namespace std;
using namespace Common;
using namespace LeaseWrapper;
using namespace Transport;
using namespace Federation;

static StringLiteral const TraceResult("Reply");
static StringLiteral const TraceDecision("Decision");

void ArbitrationDecision::WriteToTextWriter(TextWriter & w, Enum const & val)
{
    switch (val)
    {
    case Grant:
        w << "G";
        return;
    case Reject:
        w << "R";
        return;
    case Neutral:
        w << "N";
        return;
    case Delay:
        w << "D";
        return;
    case None:
        w << "?";
        return;
    default:
        w << "InvalidValue(" << static_cast<int>(val) << ')';
        return;
    }
}

void Federation::WriteToTextWriter(TextWriter & w, ArbitrationTable::RecordState::Enum const & val)
{
    switch (val)
    {
    case ArbitrationTable::RecordState::None:
        w << "None";
        return;
    case ArbitrationTable::RecordState::Pending:
        w << "Pending";
        return;
    case ArbitrationTable::RecordState::Confirmed:
        w << "Confirmed";
        return;
    case ArbitrationTable::RecordState::Reported:
        w << "Reported";
        return;
    case ArbitrationTable::RecordState::Reported2:
        w << "Reported2";
        return;
    case ArbitrationTable::RecordState::Rejected:
        w << "Rejected";
        return;
    case ArbitrationTable::RecordState::Expired:
        w << "Expired";
        return;
    case ArbitrationTable::RecordState::Implicit:
        w << "Implicit";
        return;
    default:
        w << "InvalidValue(" << static_cast<int>(val) << ')';
        return;
    }
}

ArbitrationTable::ArbitrationRecord::ArbitrationRecord(
    NodeHistory & node,
    LeaseAgentInstance const & monitor,
    LeaseAgentInstance const & subject,
    int64 monitorLeaseInstance,
    int64 subjectLeaseInstance)
    :   node_(node),
        monitorInstance_(monitor.InstanceId),
        subject_(subject),
        monitorLeaseInstance_(monitorLeaseInstance),
        subjectLeaseInstance_(subjectLeaseInstance),
        processTime_(Stopwatch::Now()),
        subjectExpireTime_(StopwatchTime::MaxValue),
        decision_(ArbitrationDecision::None),
        flags_(ArbitrationReplyBody::Extended),
        implicitArbitration1_(0),
        implicitArbitration2_(0),
        state_(RecordState::None),
        prev_(nullptr),
        next_(nullptr),
        peer_(nullptr)
{
}

void ArbitrationTable::ArbitrationRecord::UpdateState(ArbitrationTable::RecordState::Enum state)
{
    if (state == RecordState::Reported &&
        peer_->node_.GetReportedCount() >= FederationConfig::GetConfig().SuspicionReportThreshold)
    {
        state = RecordState::Reported2;
    }

    if (state != state_)
    {
        node_.UpdateRecordState(state_, state);
        state_ = state;
    }
}

void ArbitrationTable::ArbitrationRecord::set_Decision(ArbitrationDecision::Enum value)
{
    if (state_ != RecordState::Implicit)
    {
        if (value == ArbitrationDecision::Reject)
        {
            UpdateState(RecordState::Rejected);
        }
        else
        {
            if (state_ == RecordState::Rejected)
            {
                UpdateState(RecordState::Reported);
            }

            if (value == ArbitrationDecision::Grant && peer_ != nullptr && !peer_->IsPresent())
            {
                peer_->UpdateState(RecordState::Rejected);
            }
        }
    }

    decision_ = value;
}

void ArbitrationTable::ArbitrationRecord::UpdateRequest(ArbitrationRequestBody const & request)
{
    processTime_ = Stopwatch::Now();
    subjectExpireTime_ = processTime_ + request.SubjectTTL;
    if (!peer_->IsPresent())
    {
        UpdateState(request.SubjectTTL > TimeSpan::Zero ? RecordState::Pending : RecordState::Confirmed);
        peer_->UpdateState(RecordState::Reported);
    }
    else if (peer_->state_ == RecordState::Pending)
    {
        peer_->UpdateState(RecordState::Reported);
    }
}

void ArbitrationTable::ArbitrationRecord::WriteTo(TextWriter& w, FormatOptions const&) const
{
    if (state_ == RecordState::Implicit)
    {
        w.Write("{0}->{1} {2} {3} {4}/{5}",
            monitorInstance_, subject_, decision_, flags_, implicitArbitration1_, implicitArbitration2_);
    }
    else
    {
        w.Write("{0}->{1} {2}/{3} {4} {5} {6}",
            monitorInstance_, subject_, monitorLeaseInstance_, subjectLeaseInstance_, decision_, flags_, state_);
    }
}

ArbitrationTable::NodeHistory::NodeHistory(LeaseAgentInstance const & leaseAgentInstance)
    :   id_(leaseAgentInstance.Id),
        instance_(leaseAgentInstance.InstanceId),
        lastReceived_(StopwatchTime::Zero),
        head_(nullptr),
        tail_(nullptr),
        weakCount_(0),
        strongCount_(0),
        rejectCount_(0),
        reportedCount_(0),
        suspicionLevel_(0)
{
}

void ArbitrationTable::NodeHistory::UpdateInstance(int64 instance)
{
    if (instance > instance_)
    {
        for (ArbitrationRecord* record = head_; record != nullptr; record = record->next_)
        {
            if (record->state_ == RecordState::Rejected)
            {
                record->UpdateState(RecordState::Reported);
            }
        }
        ASSERT_IF(rejectCount_ > 0, "Reject count not 0: {0}", *this);

        instance_ = instance;
    }
}

void ArbitrationTable::NodeHistory::ReceiveRequest(PartnerNodeSPtr const & from)
{
    if (!from_ || from_->Instance < from->Instance)
    {
        from_ = from;
    }

    lastReceived_ = Stopwatch::Now();
}

void ArbitrationTable::NodeHistory::UpdateRecordState(RecordState::Enum oldState, RecordState::Enum newState)
{
    FederationConfig const & config = FederationConfig::GetConfig();

    int oldReportedCount = reportedCount_;

    if (IsReported(oldState))
    {
        reportedCount_--;
    }

    if (IsReported(newState))
    {
        reportedCount_++;
    }

    if (oldReportedCount < config.SuspicionReportThreshold && reportedCount_ >= config.SuspicionReportThreshold)
    {
        for (ArbitrationRecord* record = head_; record != nullptr; record = record->next_)
        {
            if (record->peer_ && record->peer_->state_ == RecordState::Reported)
            {
                record->peer_->UpdateState(RecordState::Reported2);
            }
        }
    }

    if (IsWeaklySuspicious(oldState))
    {
        weakCount_--;
    }
    else if (oldState == RecordState::Reported)
    {
        strongCount_--;
    }
    else if (oldState == RecordState::Rejected)
    {
        rejectCount_--;
    }

    if (IsWeaklySuspicious(newState))
    {
        weakCount_++;
    }
    else if (newState == RecordState::Reported)
    {
        strongCount_++;
    }
    else if (newState == RecordState::Rejected)
    {
        rejectCount_++;
    }

    suspicionLevel_ = weakCount_ * config.ArbitrationWeakSuspicionLevel +
        strongCount_ * config.ArbitrationStrongSuspicionLevel +
        rejectCount_ * config.ArbitrationRejectSuspicionLevel;
}

void ArbitrationTable::NodeHistory::Add(ArbitrationRecord* record)
{
    if (head_)
    {
        head_->prev_ = record;
        record->next_ = head_;
    }

    head_ = record;

    if (!tail_)
    {
        tail_ = record;
    }
}

bool LeaseInstanceMatched(int64 instance1, int64 instance2)
{
    return (instance1 == 0 || instance2 == 0 || instance1 == instance2);
}

ArbitrationTable::ArbitrationRecord* ArbitrationTable::NodeHistory::GetRecord(
    LeaseAgentInstance const & monitor,
    LeaseAgentInstance const & subject,
    int64 monitorLeaseInstance,
    int64 subjectLeaseInstance,
    StopwatchTime timeBound,
    bool & foundHistory)
{
    StopwatchTime searchBound = timeBound - FederationConfig::GetConfig().ArbitrationRecordSuspicionExpireInterval;

    for (ArbitrationRecord* record = head_; record != nullptr && record->processTime_ >= searchBound; record = record->next_)
    {
        if (record->monitorInstance_ == monitor.InstanceId && record->subject_ == subject && record->state_ != RecordState::Implicit)
        {
            if (LeaseInstanceMatched(record->monitorLeaseInstance_, monitorLeaseInstance) &&
                LeaseInstanceMatched(record->subjectLeaseInstance_, subjectLeaseInstance))
            {
                foundHistory = ((record->flags_ & ArbitrationReplyBody::Continuous) != 0);
                return record;
            }
            else
            {
                foundHistory = true;
            }
        }
    }

    return nullptr;
}

ArbitrationTable::ArbitrationRecord* ArbitrationTable::NodeHistory::GetSuccessRecord()
{
    for (ArbitrationRecord* record = head_; record; record = record->next_)
    {
        if (record->IsPresent() && record->Decision != ArbitrationDecision::Reject)
        {
            return record;
        }
    }

    return nullptr;
}

ArbitrationTable::ArbitrationRecord* ArbitrationTable::NodeHistory::GetImplicitRecord()
{
    for (ArbitrationRecord* record = head_; record; record = record->next_)
    {
        if (record->state_ == RecordState::Implicit)
        {
            return record;
        }
    }

    return nullptr;
}

void ArbitrationTable::NodeHistory::Remove(ArbitrationRecord* record)
{
    record->UpdateState(RecordState::None);

    if (record == head_)
    {
        head_ = record->next_;
    }
    else
    {
        record->prev_->next_ = record->next_;
    }

    if (record == tail_)
    {
        tail_ = record->prev_;
    }
    else
    {
        record->next_->prev_ = record->prev_;
    }

    delete record;
}

bool ArbitrationTable::NodeHistory::Compact(StopwatchTime recordTimeBound)
{
    while (tail_ && tail_->processTime_ < recordTimeBound)
    {
        if (tail_->peer_)
        {
            tail_->peer_->node_.Remove(tail_->peer_);
        }

        Remove(tail_);
    }

    return (!tail_ && lastReceived_ < recordTimeBound);
}

int ArbitrationTable::NodeHistory::GetSuspicionLevel()
{
    StopwatchTime now = Stopwatch::Now();
    FederationConfig const & config = FederationConfig::GetConfig();
    StopwatchTime expireTime = now - config.ArbitrationRecordSuspicionExpireInterval;
    StopwatchTime rejectExpireTime = now - config.MaxArbitrationTimeout;

    for (ArbitrationRecord* record = head_; record != nullptr; record = record->next_)
    {
        if (record->state_ != RecordState::Expired && record->processTime_ < expireTime)
        {
            record->UpdateState(RecordState::Expired);
        }
        else if (record->state_ == RecordState::Pending)
        {
            if (record->subjectExpireTime_ < now)
            {
                record->UpdateState(RecordState::Confirmed);
            }
        }
        else if (record->state_ == RecordState::Rejected && record->processTime_ < rejectExpireTime)
        {
            record->UpdateState(RecordState::Reported);
        }
    }

    return suspicionLevel_;
}

bool ArbitrationTable::NodeHistory::IsSuspicious(bool withKeepAlive)
{
    FederationConfig const & config = FederationConfig::GetConfig();
    int threshold = (withKeepAlive ? config.KeepAliveSuspicionLevelThreshold : config.SuspicionLevelThreshold);
    if (suspicionLevel_ < threshold)
    {
        return false;
    }

    return (GetSuspicionLevel() >= threshold);
}

void ArbitrationTable::NodeHistory::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.WriteLine("{0}:{1} {2}/{3}/{4}/{5} {6}", id_, instance_, weakCount_, strongCount_, rejectCount_, reportedCount_, suspicionLevel_);

    for (ArbitrationRecord* record = head_; record != nullptr; record = record->next_)
    {
        w.WriteLine(*record);
    }
}

void ArbitrationTable::DelayedResponse::Add(MessageUPtr && message, PartnerNodeSPtr const & target)
{
    message_ = move(message);
    target_ = target;
}

void ArbitrationTable::DelayedResponse::Send(SiteNode & site)
{
    if (message_)
    {
        site.Send(move(message_), target_);
    }
}

ArbitrationTable::ArbitrationTable(NodeId voteId)
    : voteId_(voteId), historyStartTime_(Stopwatch::Now()), lastCompactTime_(historyStartTime_)
{
}

ArbitrationTable::NodeHistory* ArbitrationTable::GetNodeHistory(LeaseAgentInstance const & leaseAgentInstance, bool create)
{
    auto it = nodes_.find(leaseAgentInstance.Id);
    if (it != nodes_.end())
    {
        it->second->UpdateInstance(leaseAgentInstance.InstanceId);
        return it->second.get();
    }

    if (!create)
    {
        return nullptr;
    }

    NodeHistoryUPtr node = make_unique<NodeHistory>(leaseAgentInstance);
    NodeHistory* result = node.get();

    nodes_.insert(make_pair(leaseAgentInstance.Id, move(node)));

    return result;
}

void ArbitrationTable::MakeDecision(
    ArbitrationRequestBody const & request,
    ArbitrationRecord* monitorRecord,
    ArbitrationRecord* subjectRecord,
    bool continuous,
    DelayedResponse & delayedResponse)
{
    if (subjectRecord->IsPresent())
    {
        if (subjectRecord->flags_ & ArbitrationReplyBody::Strong)
        {
            monitorRecord->flags_ |= ArbitrationReplyBody::Strong;
        }

        if (subjectRecord->Decision == ArbitrationDecision::Grant)
        {
            monitorRecord->Decision = ArbitrationDecision::Reject;
        }
        else if (subjectRecord->Decision == ArbitrationDecision::Reject && request.Type != ArbitrationType::OneWay)
        {
            monitorRecord->Decision = ArbitrationDecision::Grant;
        }
        else
        {
            ASSERT_IF(request.Type == ArbitrationType::TwoWaySimple, "{0} subject record {1} request {2}",
                voteId_, *subjectRecord, request);
            if (subjectRecord->Decision == ArbitrationDecision::Delay)
            {
                subjectRecord->Decision = ArbitrationDecision::Neutral;
                subjectRecord->flags_ &= (~ArbitrationReplyBody::Delayed);
                delayedResponse.Add(FederationMessage::GetDelayedArbitrateReply().CreateMessage(DelayedArbitrationReplyBody(request, ArbitrationDecision::Neutral, TimeSpan::MaxValue, true, subjectRecord->flags_)), subjectRecord->node_.From);
            }

            monitorRecord->Decision = ArbitrationDecision::Neutral;
        }

        return;
    }

    if (request.Type == ArbitrationType::OneWay)
    {
        monitorRecord->Decision = ArbitrationDecision::Neutral;
        return;
    }

    if (monitorRecord->IsPresent())
    {
        return;
    }

    FederationConfig const & config = FederationConfig::GetConfig();
    StopwatchTime now = Stopwatch::Now();
    bool subjectReceived = (now < subjectRecord->node_.LastReceived + config.LeaseDuration);

    if (continuous || request.Type == ArbitrationType::TwoWaySimple)
    {
        int monitorLevel = monitorRecord->node_.GetSuspicionLevel();
        int subjectLevel = subjectRecord->node_.GetSuspicionLevel();
        bool monitorReceived = (now < monitorRecord->node_.LastReceived + config.LeaseDuration);
        
        if (monitorLevel >= subjectLevel + config.SuspicionLevelComparisonThreshold && (!monitorReceived || subjectReceived))
        {
            if (now >= historyStartTime_ + config.ArbitrationRecordSuspicionExpireInterval)
            {
                monitorRecord->flags_ |= ArbitrationReplyBody::Strong;
            }

            monitorRecord->Decision = ArbitrationDecision::Reject;
        }
        else if (!monitorReceived && subjectReceived)
        {
            monitorRecord->Decision = ArbitrationDecision::Reject;
        }
        else
        {
            if (subjectLevel >= monitorLevel + config.SuspicionLevelComparisonThreshold &&
                now >= historyStartTime_ + config.ArbitrationRecordSuspicionExpireInterval)
            {
                monitorRecord->flags_ |= ArbitrationReplyBody::Strong;
            }

            monitorRecord->Decision = ArbitrationDecision::Grant;
        }
    }
    else
    {
        if ((subjectReceived && !subjectRecord->node_.IsSuspicious(true)) || monitorRecord->node_.IsSuspicious(false))
        {
            delayed_.push_back(monitorRecord);
            monitorRecord->flags_ |= ArbitrationReplyBody::Delayed;
            monitorRecord->Decision = ArbitrationDecision::Delay;
        }
        else
        {
            monitorRecord->Decision = ArbitrationDecision::Grant;
        }
    }
}

void ArbitrationTable::ProcessRevertRequest(ArbitrationRequestBody const & request, NodeHistory* monitor, StopwatchTime timeBound, DelayedResponse & delayedResponse)
{
    bool foundHistory = false;
    ArbitrationRecord* monitorRecord = monitor->GetRecord(request.Monitor, request.Subject, request.MonitorLeaseInstance, request.SubjectLeaseInstance, timeBound, foundHistory);
    if (monitorRecord && monitorRecord->Decision == ArbitrationDecision::Grant)
    {
        monitorRecord->Decision = (request.Type == ArbitrationType::RevertToNeutral ? ArbitrationDecision::Neutral : ArbitrationDecision::Reject);

        ArbitrationRecord* subjectRecord = monitorRecord->peer_;
        if (subjectRecord->IsPresent())
        {
            subjectRecord->Decision = (request.Type == ArbitrationType::RevertToNeutral ? ArbitrationDecision::Neutral : ArbitrationDecision::Grant);
            if (subjectRecord->IsExtended())
            {
                delayedResponse.Add(FederationMessage::GetDelayedArbitrateReply().CreateMessage(DelayedArbitrationReplyBody(request, subjectRecord->Decision, subjectRecord->subjectExpireTime_.RemainingDuration(), true, subjectRecord->flags_)), subjectRecord->node_.From);
            }
        }
        else if (subjectRecord->state_ == RecordState::Rejected)
        {
            subjectRecord->UpdateState(RecordState::Reported);
        }

        WriteInfo(TraceResult, "{0} reverted {1} to {2}", voteId_, request, monitorRecord->Decision);
    }
    else
    {
        WriteWarning(TraceResult, "{0} ignored revert request {1}", voteId_, request);
    }
}

MessageUPtr ArbitrationTable::ProcessQuery(ArbitrationRequestBody const & request)
{
    StopwatchTime result = historyStartTime_;
    StopwatchTime now = Stopwatch::Now();

    NodeHistory* subject = GetNodeHistory(request.Subject, false);
    if (subject)
    {
        ArbitrationRecord* record = subject->GetSuccessRecord();
        if (record)
        {
            result = record->processTime_;
        }
    }

    WriteInfo(TraceResult,
        "{0} processed query {1} with result {2}",
        voteId_, request, result);

    return FederationMessage::GetArbitrateReply().CreateMessage(ArbitrationReplyBody(now - result, TimeSpan::MaxValue, false, ArbitrationReplyBody::Extended));
}

MessageUPtr ArbitrationTable::ProcessImplicitRequest(ArbitrationRequestBody const & request, NodeHistory* monitor)
{
    NodeHistory* subject = GetNodeHistory(request.Subject, false);
    ArbitrationRecord* subjectRecord = (subject ? subject->GetImplicitRecord() : nullptr);

    FederationConfig const & config = FederationConfig::GetConfig();
    TimeSpan recordKeepInterval = config.MaxImplicitLeaseInterval + config.MaxLeaseDuration + config.MaxLeaseDuration + config.MaxArbitrationTimeout + config.MaxArbitrationTimeout;
    StopwatchTime timeBound = Stopwatch::Now() - recordKeepInterval;

    ArbitrationDecision::Enum decision;
    if (subjectRecord && subjectRecord->processTime_ > timeBound)
    {
        if (subjectRecord->Decision == ArbitrationDecision::Reject)
        {
            decision = ArbitrationDecision::Grant;
        }
        else if (subjectRecord->Decision == ArbitrationDecision::Grant && subjectRecord->subject_ == request.Monitor)
        {
            decision = ArbitrationDecision::Reject;
        }
        else
        {
            if (request.ImplicitArbitration2 == 0 && subjectRecord->implicitArbitration2_ > 0)
            {
                decision = ArbitrationDecision::Grant;
            }
            else if ((request.ImplicitArbitration2 > 0 && subjectRecord->implicitArbitration2_ == 0) ||
                     (request.ImplicitArbitration1 > subjectRecord->implicitArbitration1_ + 1))
            {
                decision = ArbitrationDecision::Reject;
            }
            else if (subjectRecord->implicitArbitration1_ > request.ImplicitArbitration1 + 1)
            {
                decision = ArbitrationDecision::Grant;
            }
            else
            {
                int monitorLevel = monitor->GetSuspicionLevel();
                int subjectLevel = subject->GetSuspicionLevel();
                if (subjectLevel >= monitorLevel + config.SuspicionLevelComparisonThreshold)
                {
                    decision = ArbitrationDecision::Grant;
                }
                else
                {
                    decision = ArbitrationDecision::Reject;
                }
            }
        }
    }
    else
    {
        decision = ArbitrationDecision::Neutral;
    }

    ArbitrationRecord* monitorRecord = new ArbitrationRecord(*monitor, request.Monitor, request.Subject, request.MonitorLeaseInstance, request.SubjectLeaseInstance);
    monitorRecord->state_ = RecordState::Implicit;
    monitorRecord->implicitArbitration1_ = request.ImplicitArbitration1;
    monitorRecord->implicitArbitration2_ = request.ImplicitArbitration2;
    monitorRecord->Decision = decision;
    monitor->Add(monitorRecord);

    WriteInfo(TraceResult,
        "{0} processed implicit request {1} decision {2} history start {3} subject {4}",
        voteId_, request, decision, historyStartTime_, subjectRecord ? wformatString(*subjectRecord) : L"");

    return FederationMessage::GetArbitrateReply().CreateMessage(ArbitrationReplyBody(TimeSpan::MaxValue, decision != ArbitrationDecision::Reject ? TimeSpan::MaxValue : TimeSpan::Zero, false, ArbitrationReplyBody::Extended));
}

MessageUPtr ArbitrationTable::ProcessRequest(SiteNode & site, ArbitrationRequestBody const & request, PartnerNodeSPtr const & from)
{
    MessageUPtr response;
    DelayedResponse delayedResponse;
    {
        AcquireExclusiveLock grab(lock_);
        response = InternalProcessRequest(request, from, delayedResponse);
    }

    delayedResponse.Send(site);

    return response;
}

MessageUPtr ArbitrationTable::InternalProcessRequest(ArbitrationRequestBody const & request, PartnerNodeSPtr const & from, DelayedResponse & delayedResponse)
{
    if (request.Type == ArbitrationType::Query)
    {
        return ProcessQuery(request);
    }

    NodeHistory* monitor = GetNodeHistory(request.Monitor, true);
    if (monitor->Instance > request.Monitor.InstanceId)
    {
        WriteInfo(TraceResult,
            "{0} ignored request {1} with known instance {2}",
            voteId_, request, monitor->Instance);
        return nullptr;
    }

    if (from->IsShutdown)
    {
        WriteInfo(TraceResult,
            "{0} rejected request {1} from down node {2}",
            voteId_, request, from->Instance);
        return FederationMessage::GetArbitrateReply().CreateMessage(ArbitrationReplyBody(TimeSpan::MaxValue, TimeSpan::Zero, true, ArbitrationReplyBody::Strong));
    }

    monitor->ReceiveRequest(from);

    if (request.Type == ArbitrationType::KeepAlive)
    {
        WriteInfo(TraceResult,
            "{0} processed keep alive request from {1}:{2}",
            voteId_, monitor->Id, monitor->Instance);
        return nullptr;
    }

    if (request.Type == ArbitrationType::Implicit)
    {
        return ProcessImplicitRequest(request, monitor);
    }

    StopwatchTime now = Stopwatch::Now();

    FederationConfig & config = FederationConfig::GetConfig();
    TimeSpan historyNeeded = request.HistoryNeeded;
    if (historyNeeded == TimeSpan::Zero)
    {
        historyNeeded = config.MaxLeaseDuration + config.MaxArbitrationTimeout;
    }
    StopwatchTime timeBound = now - historyNeeded;

    if (request.Type == ArbitrationType::RevertToReject || request.Type == ArbitrationType::RevertToNeutral)
    {
        ProcessRevertRequest(request, monitor, timeBound, delayedResponse);
        return nullptr;
    }

    bool foundHistory = false;
    ArbitrationRecord* subjectRecord;
    ArbitrationRecord* monitorRecord = monitor->GetRecord(request.Monitor, request.Subject, request.MonitorLeaseInstance, request.SubjectLeaseInstance, timeBound, foundHistory);
    if (!monitorRecord)
    {
        monitorRecord = new ArbitrationRecord(*monitor, request.Monitor, request.Subject, request.MonitorLeaseInstance, request.SubjectLeaseInstance);

        NodeHistory* subject = GetNodeHistory(request.Subject, true);
        subjectRecord = new ArbitrationRecord(*subject, request.Subject, request.Monitor, request.SubjectLeaseInstance, request.MonitorLeaseInstance);
        if (foundHistory)
        {
            subjectRecord->flags_ |= ArbitrationReplyBody::Continuous;
        }

        monitorRecord->peer_ = subjectRecord;
        subjectRecord->peer_ = monitorRecord;

        monitor->Add(monitorRecord);
        subject->Add(subjectRecord);
    }
    else
    {
        subjectRecord = monitorRecord->peer_;
    }

    if (!monitorRecord->IsPresent())
    {
        monitorRecord->UpdateRequest(request);
    }

    if (foundHistory)
    {
        monitorRecord->flags_ |= ArbitrationReplyBody::Continuous;
    }

    if (historyStartTime_ > timeBound)
    {
        WriteWarning("Reject",
            "{0} rejected arbitration request as history required {1} but only {2} onwards available",
            voteId_, historyNeeded, historyStartTime_);

        monitorRecord->Decision = ArbitrationDecision::Reject;
    }
    else
    {
        MakeDecision(request, monitorRecord, subjectRecord, foundHistory, delayedResponse);
    }

    WriteInfo(TraceResult,
        "{0} processed request {1} with result {2}/{3}, monitor {4} level {5}, subject {6} level {7} history={8}",
        voteId_,
        request,
        monitorRecord->Decision,
        monitorRecord->flags_,
        monitorRecord->state_,
        monitor->SuspicionLevel,
        wformatString("{0} {1}", subjectRecord->Decision, subjectRecord->state_),
        subjectRecord->node_.GetRawSuspicionLevel(),
        foundHistory);

    return FederationMessage::GetArbitrateReply().CreateMessage(
        ArbitrationReplyBody(
            monitorRecord->Decision == ArbitrationDecision::Grant ? request.SubjectTTL : TimeSpan::MaxValue,
            monitorRecord->Decision == ArbitrationDecision::Reject ? TimeSpan::Zero : TimeSpan::MaxValue,
            subjectRecord->IsPresent(),
            monitorRecord->flags_));
}

TimeSpan ArbitrationTable::Compact(SiteNode & site)
{
    FederationConfig const & config = FederationConfig::GetConfig();
    TimeSpan result = config.ArbitrationDelayInterval;
    StopwatchTime now = Stopwatch::Now();
    StopwatchTime delayThreshold = now - result;

    vector<PartnerNodeSPtr> targets;
    vector<DelayedArbitrationReplyBody> replies;
    {
        AcquireExclusiveLock grab(lock_);

        for (auto it = delayed_.begin(); it != delayed_.end();)
        {
            ArbitrationRecord* record = *it;
            if (record->Decision != ArbitrationDecision::Delay)
            {
                it = delayed_.erase(it);
            }
            else if (record->processTime_ <= delayThreshold)
            {
                record->Decision = ArbitrationDecision::Grant;
                record->flags_ &= (~ArbitrationReplyBody::Delayed);
                if (record->state_ == RecordState::Pending)
                {
                    record->UpdateState(RecordState::Confirmed);
                }

                targets.push_back(record->node_.From);
                replies.push_back(DelayedArbitrationReplyBody(
                    LeaseAgentInstance(record->node_.Id, record->monitorInstance_),
                    record->subject_,
                    record->monitorLeaseInstance_,
                    record->subjectLeaseInstance_,
                    ArbitrationDecision::Grant,
                    record->subjectExpireTime_.RemainingDuration(),
                    false,
                    record->flags_));

                WriteInfo(TraceResult,
                    "{0} delayed grant for {1}:{2}-{3} {4}/{5} {6}",
                    voteId_, record->node_.Id, record->monitorInstance_, record->subject_, record->monitorLeaseInstance_, record->subjectLeaseInstance_, record->processTime_);

                it = delayed_.erase(it);
            }
            else
            {
                WriteInfo("Delay",
                    "{0} delayed for {1}:{2}-{3} {4}/{5} {6}",
                    voteId_, record->node_.Id, record->monitorInstance_, record->subject_, record->monitorLeaseInstance_, record->subjectLeaseInstance_, record->processTime_);
                it++;
            }
        }

        if (now > lastCompactTime_ + config.ArbitrationCleanupInterval)
        {
            CompactInternal(now);
            lastCompactTime_ = now;
        }
    }

    for (size_t i = 0; i < targets.size(); i++)
    {
        auto message = FederationMessage::GetDelayedArbitrateReply().CreateMessage(replies[i]);
        site.Send(move(message), targets[i]);
    }

    return (delayed_.size() == 0 ? result : delayed_[0]->processTime_ - delayThreshold);
}

void ArbitrationTable::CompactInternal(StopwatchTime now)
{
    FederationConfig const & config = FederationConfig::GetConfig();

    TimeSpan recordKeepInterval = config.MaxImplicitLeaseInterval + config.MaxLeaseDuration + config.MaxLeaseDuration + config.MaxArbitrationTimeout + config.MaxArbitrationTimeout;
    StopwatchTime timeBound = now - recordKeepInterval;

    TimeoutHelper timeout(TimeSpan::FromMilliseconds(100));
    for (auto it = nodes_.begin(); it != nodes_.end() && !timeout.IsExpired;)
    {
        if (it->second->Compact(timeBound))
        {
            it = nodes_.erase(it);
        }
        else
        {
            ++it;
        }
    }

    historyStartTime_ = max(historyStartTime_, timeBound);
}

void ArbitrationTable::Dump(TextWriter& w, FormatOptions const& option)
{
    AcquireExclusiveLock grab(lock_);
    WriteTo(w, option);
}

void ArbitrationTable::WriteTo(TextWriter& w, FormatOptions const&) const
{
    for (auto it = nodes_.begin(); it != nodes_.end(); ++it)
    {
        it->second->GetSuspicionLevel();
        w.WriteLine(*(it->second));
    }
}
