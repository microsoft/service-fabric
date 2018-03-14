// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Federation/ArbitrationOperation.h"

using namespace Common;
using namespace std;
using namespace Federation;
using namespace LeaseWrapper;

static StringLiteral const ArbitrationTimerTag("Arbitration");
static StringLiteral const TraceVoter("voter");

int const MaxRequestPerVoter = 3;

ArbitrationOperation::ArbitrationVote::ArbitrationVote(VoteProxySPtr && proxy)
    :   proxy_(std::move(proxy)),
        replyReceivedTime_(StopwatchTime::Zero),
        count_(0),
        pending_(false),
        reverted_(false)
{
}

bool ArbitrationOperation::ArbitrationVote::HasResult() const
{
    return (replyReceivedTime_ != StopwatchTime::Zero);
}

bool ArbitrationOperation::ArbitrationVote::CanRevert(ArbitrationType::Enum type) const
{
    if (!HasResult() || result_.SubjectTTL == TimeSpan::MaxValue || reverted_ || pending_)
    {
        return false;
    }

    return (type == ArbitrationType::RevertToReject || result_.IsExtended());
}

void ArbitrationOperation::ArbitrationVote::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.Write("{0} {1}", proxy_->VoteId, result_);
}

ArbitrationOperation::ArbitrationOperation(
    SiteNodeSPtr const & siteNode,
    vector<VoteProxySPtr> && votes,
    LeaseAgentInstance const & local,
    LeaseAgentInstance const & remote,
    TimeSpan remoteTTL,
    TimeSpan historyNeeded,
    int64 monitorLeaseInstance,
    int64 subjectLeaseInstance,
    int64 extraData,
    ArbitrationType::Enum type)
    :   siteNode_(siteNode),
        local_(local),
        remote_(remote),
        startTime_(Stopwatch::Now()),
        remoteExpireTime_(startTime_ + remoteTTL),
        historyNeeded_(historyNeeded),
        monitorLeaseInstance_(monitorLeaseInstance),
        subjectLeaseInstance_(subjectLeaseInstance),
        extraData_(extraData),
        type_(type),
        expireTime_(startTime_ + FederationConfig::GetConfig().ArbitrationTimeout),
        localResult_(TimeSpan::Zero),
        remoteResult_(TimeSpan::MaxValue),
        revertType_(type),
        nextCheck_(StopwatchTime::MaxValue),
        lastResponseTime_(StopwatchTime::Zero),
        checkRevertTime_(StopwatchTime::MaxValue),
        canRetry_(false),
        partialCompleted_(false),
        completed_(false),
        isLocalPreferred_(IsLocalPreferred(local_, remote_))
{
    for (size_t i = 0; i < votes.size(); i++)
    {
        votes_.push_back(make_unique<ArbitrationVote>(move(votes[i])));
    }
}

bool ArbitrationOperation::HasReply(ArbitrationType::Enum type)
{
    return (type != ArbitrationType::RevertToReject &&
        type != ArbitrationType::RevertToNeutral &&
        type != ArbitrationType::KeepAlive);
}

ArbitrationOperation::ArbitrationVote* ArbitrationOperation::GetVoter(NodeInstance const & voteInstance)
{
    for (ArbitrationVoteUPtr const & vote : votes_)
    {
        if (vote->proxy_->VoteId == voteInstance.Id)
        {
            return vote.get();
        }
    }

    return nullptr;
}

void ArbitrationOperation::Start(ArbitrationOperationSPtr const & thisSPtr)
{
    WriteInfo("Start", siteNode_->IdString, "{0} started, remote expiration {1}, history {2}, extra {3}",
        *this, remoteExpireTime_, historyNeeded_, extraData_);

    nextCheck_ = expireTime_;

    if (HasReply(type_))
    {
        timer_ = Timer::Create(
            ArbitrationTimerTag,
            [this, thisSPtr](TimerSPtr const &) { this->OnTimer(thisSPtr); },
            true);
        timer_->Change(expireTime_.RemainingDuration());
    }

    StartArbitration(thisSPtr);
}

void ArbitrationOperation::StartArbitration(ArbitrationOperationSPtr const & thisSPtr)
{
    AcquireExclusiveLock grab(lock_);

    for (ArbitrationVoteUPtr const & vote : votes_)
    {
        StartVoteArbitration(thisSPtr, *vote, type_);
    }
}

void ArbitrationOperation::OnTimer(ArbitrationOperationSPtr const & thisSPtr)
{
    AcquireExclusiveLock grab(lock_);
    nextCheck_ = StopwatchTime::MaxValue;
    RunStateMachine(thisSPtr);
}

void ArbitrationOperation::CleanupTimer()
{
    if (timer_)
    {
        timer_->Cancel();
        timer_.reset();
    }

    nextCheck_ = StopwatchTime::MaxValue;
}

void ArbitrationOperation::StartVoteArbitration(
    ArbitrationOperationSPtr const & thisSPtr,
    ArbitrationVote & vote,
    ArbitrationType::Enum type)
{
    TimeSpan timeout;
    TimeSpan subjectTTL;
    TimeSpan historyNeeded;
    FederationConfig const & config = FederationConfig::GetConfig();

    if (type == ArbitrationType::RevertToReject || type == ArbitrationType::RevertToNeutral)
    {
        timeout = config.MessageTimeout;
        subjectTTL = TimeSpan::MinValue;
        historyNeeded = TimeSpan::Zero;
        vote.reverted_ = true;
        vote.result_.SubjectTTL = TimeSpan::MaxValue;
        if (type == ArbitrationType::RevertToReject)
        {
            vote.result_.MonitorTTL = TimeSpan::Zero;
        }
    }
    else
    {
        timeout = expireTime_.RemainingDuration();
        if (timeout <= TimeSpan::Zero)
        {
            return;
        }

        historyNeeded = historyNeeded_;

        subjectTTL = (type == ArbitrationType::OneWay ? config.LeaseDuration : remoteExpireTime_.RemainingDuration());
        vote.pending_ = true;
    }

    vote.count_++;

    WriteInfo(TraceVoter, siteNode_->IdString,
        "{0} starting arbitration to {1}, type {2}",
        *this, vote.proxy_->VoteId, type);

    vote.proxy_->BeginArbitrate(
        *siteNode_,
        ArbitrationRequestBody(local_, remote_, subjectTTL, historyNeeded, monitorLeaseInstance_, subjectLeaseInstance_, extraData_, type),
        timeout,
        [thisSPtr, &vote, type](AsyncOperationSPtr const & operation)
        {
            thisSPtr->ProcessResult(thisSPtr, operation, vote);
        },
        siteNode_->CreateAsyncOperationRoot());
}

void ArbitrationOperation::ProcessResult(ArbitrationOperationSPtr const & thisSPtr, AsyncOperationSPtr const & operation, ArbitrationVote & vote)
{
    ArbitrationReplyBody result;
    ErrorCode error = vote.proxy_->EndArbitrate(operation, *siteNode_, result);
    if (!HasReply(type_))
    {
        ASSERT_IF(!error.IsSuccess(), "Unexpected error {0}", error);
        return;
    }

    result.Normalize();

    if (!operation->CompletedSynchronously)
    {
        ProcessResult(thisSPtr, vote, error, result);
    }
    else
    {
        Threadpool::Post([thisSPtr, &vote, error, result, this] { ProcessResult(thisSPtr, vote, error, result); });
    }
}

void ArbitrationOperation::ProcessResult(
    ArbitrationOperationSPtr const & thisSPtr,
    ArbitrationVote & vote,
    ErrorCode error,
    ArbitrationReplyBody const & result)
{
    AcquireExclusiveLock grab(lock_);

    if (!vote.pending_)
    {
        return;
    }

    vote.pending_ = false;
    lastResponseTime_ = Stopwatch::Now();

    if (error.IsSuccess() && lastResponseTime_ > expireTime_)
    {
        error = ErrorCodeValue::Timeout;
    }

    if (error.IsSuccess())
    {
        if (!vote.reverted_)
        {
            vote.result_ = result;
            vote.replyReceivedTime_ = lastResponseTime_;

            WriteInfo(TraceVoter, siteNode_->IdString,
                "{0} updated voter {1}",
                *this, vote);
        }
        else
        {
            WriteWarning(TraceVoter, siteNode_->IdString,
                "{0} voter {1} ignored result {2}",
                *this, vote, result);
        }
    }
    else
    {
        WriteWarning(TraceVoter, siteNode_->IdString,
            "{0} arbitration for voter {1} failed: {2}",
            *this, vote.proxy_->VoteId, error);
    }

    RunStateMachine(thisSPtr);
}

bool ArbitrationOperation::Matches(DelayedArbitrationReplyBody const & body)
{
    return (local_ == body.Monitor &&
            remote_ == body.Subject &&
            monitorLeaseInstance_ == body.MonitorLeaseInstance &&
            subjectLeaseInstance_ == body.SubjectLeaseInstance);
}

void ArbitrationOperation::ProcessDelayedReply(
    ArbitrationOperationSPtr const & thisSPtr,
    DelayedArbitrationReplyBody const & body, 
    NodeInstance const & voteInstance)
{
    bool isGrant = (body.Decision == ArbitrationDecision::Grant);

    AcquireExclusiveLock grab(lock_);
    ArbitrationVote* vote = GetVoter(voteInstance);
    if (vote && !vote->reverted_)
    {
        if (vote->HasResult())
        {
            if (isGrant)
            {
                if (vote->result_.SubjectTTL == TimeSpan::MaxValue)
                {
                    vote->replyReceivedTime_ = Stopwatch::Now();
                    vote->result_.SubjectTTL = body.SubjectTTL;
                    vote->result_.MonitorTTL = TimeSpan::MaxValue;
                    vote->result_.Flags = body.Flags;
                }
            }
            else
            {
                if (vote->result_.MonitorTTL != TimeSpan::MaxValue)
                {
                    vote->replyReceivedTime_ = Stopwatch::Now();
                    vote->result_.MonitorTTL = TimeSpan::MaxValue;
                    vote->result_.Flags = body.Flags;
                }
            }

            if (!vote->result_.SubjectReported)
            {
                vote->result_.SubjectReported = body.SubjectReported;
            }
            if (vote->result_.IsDelayed())
            {
                vote->result_.Flags &= (~ArbitrationReplyBody::Delayed);
            }
        }
        else
        {
            vote->replyReceivedTime_ = Stopwatch::Now();
            vote->pending_ = false;
            vote->result_ = ArbitrationReplyBody(
                isGrant ? body.SubjectTTL : TimeSpan::MaxValue,
                TimeSpan::MaxValue, 
                body.SubjectReported,
                body.Flags);
        }

        WriteInfo(TraceVoter, siteNode_->IdString,
            "{0} updated voter {1} with delayed response {2}",
            *this, *vote, body);
    }

    RunStateMachine(thisSPtr);
}

void ArbitrationOperation::RunStateMachine(shared_ptr<ArbitrationOperation> const & thisSPtr)
{
    bool partialCompleted = partialCompleted_;
    bool completed = completed_;

    CheckState();

    GenerationActions(thisSPtr, partialCompleted, completed);
}

void ArbitrationOperation::CheckState()
{
    if (completed_)
    {
        return;
    }

    int quorumCount = static_cast<int>(votes_.size() / 2 + 1);
    int resultCount = 0;
    int retryableCount = 0;

    int grant = 0;
    int strongGrant = 0;
    int reject = 0;
    int strongReject = 0;
    int neutral = 0;
    int delay = 0;
    bool remoteReported = false;
    bool continuous = false;

    for (ArbitrationVoteUPtr const & vote : votes_)
    {
        if (vote->HasResult())
        {
            resultCount++;

            if (vote->result_.SubjectReported)
            {
                remoteReported = true;
            }

            if (vote->result_.IsContinuous())
            {
                continuous = true;
            }

            if (vote->result_.MonitorTTL == TimeSpan::MaxValue)
            {
                if (type_ == ArbitrationType::Implicit || 
                    type_ == ArbitrationType::Query ||
                    vote->result_.SubjectTTL == TimeSpan::MaxValue)
                {
                    neutral++;
                    if (vote->result_.IsDelayed())
                    {
                        delay++;
                    }
                }
                else
                {
                    grant++;
                    if (vote->result_.IsStrong())
                    {
                        strongGrant++;
                    }
                }
            }
            else
            {
                reject++;
                if (vote->result_.IsStrong())
                {
                    strongReject++;
                }

                if (vote->count_ < MaxRequestPerVoter)
                {
                    retryableCount++;
                }
            }
        }
        else if (vote->count_ >= MaxRequestPerVoter)
        {
            reject++;
        }
    }

    bool localGranted = (grant + neutral >= quorumCount);
    ASSERT_IF(!localGranted && partialCompleted_, "Unexpected partial completed {0}", *this);

    bool isPreferred;
    if (type_ == ArbitrationType::TwoWaySimple)
    {
        isPreferred = isLocalPreferred_;
    }
    else
    {
        isPreferred = (strongGrant > strongReject) || (strongGrant == strongReject && isLocalPreferred_);
    }

    bool canCoexist = (type_ == ArbitrationType::TwoWayExtended && !continuous);

    checkRevertTime_ = StopwatchTime::MaxValue;
    if (revertType_ == type_ && grant > 0 && resultCount >= quorumCount && remoteReported && (type_ == ArbitrationType::TwoWaySimple || type_ == ArbitrationType::TwoWayExtended))
    {
        if (canCoexist)
        {
            revertType_ = ArbitrationType::RevertToNeutral;
        }
        else if (!localGranted && !isPreferred)
        {
            if (resultCount == votes_.size())
            {
                revertType_ = ArbitrationType::RevertToReject;
            }
            else
            {
                StopwatchTime checkRevertTime = max(startTime_ + FederationConfig::GetConfig().LeaseSuspendTimeout, lastResponseTime_ + TimeSpan::FromSeconds(1));
                if (Stopwatch::Now() > checkRevertTime)
                {
                    revertType_ = ArbitrationType::RevertToReject;
                }
                else
                {
                    checkRevertTime_ = checkRevertTime;
                }
            }
        }
    }

    canRetry_ = !localGranted && (reject > 0) && (grant + neutral > 0) && (type_ == ArbitrationType::TwoWaySimple || type_ == ArbitrationType::TwoWayExtended);
    if (localGranted)
    {
        localResult_ = TimeSpan::MaxValue;

        if (type_ == ArbitrationType::Query && resultCount < votes_.size())
        {
            int count = 0;
            FederationConfig const & config = FederationConfig::GetConfig();
            TimeSpan queryInterval = config.MaxImplicitLeaseInterval + config.MaxArbitrationTimeout + config.MaxArbitrationTimeout + config.MaxLeaseDuration;
            for (ArbitrationVoteUPtr const & vote : votes_)
            {
                if (vote->HasResult() && vote->result_.SubjectTTL > queryInterval)
                {
                    count++;
                }
            }
            
            partialCompleted_ = completed_ = (count >= quorumCount);
        }
        else
        {
            partialCompleted_ = true;
            
            if (revertType_ != type_ || reject + neutral - delay >= quorumCount)
            {
                completed_ = true;
            }
            else if (grant >= quorumCount)
            {
                remoteResult_ = GetRemoteTTL();
                completed_ = true;
            }
        }
    }
    else
    {
        if (!canRetry_)
        {
            retryableCount = 0;
        }

        if (reject - retryableCount >= quorumCount)
        {
            completed_ = true;
            if (revertType_ == ArbitrationType::TwoWayExtended || revertType_ == ArbitrationType::TwoWaySimple)
            {
                revertType_ = ArbitrationType::RevertToReject;
            }
        }
    }

    if (!completed_)
    {
        if (siteNode_->IsShutdown)
        {
            completed_ = true;
        }
        else if (Stopwatch::Now() + StopwatchTime::TimerMargin > expireTime_)
        {
            if (!partialCompleted_)
            {
                WriteWarning("Timeout", siteNode_->IdString, "{0} timed out", *this);
            }

            completed_ = true;
        }
    }

    string desc;
    StringWriterA w(desc);
    if (revertType_ != type_)
    {
        w.Write(" ");
        w.Write(revertType_);
    }

    if (canRetry_)
    {
        w.Write(" canRetry");
    }

    if (completed_)
    {
        w.Write(" complete");
    }
    else if (partialCompleted_)
    {
        w.Write(" partial");
    }

    WriteInfo("State", siteNode_->IdString, "{0} count={1}/{2}/{3}/{4}{5}",
        *this, grant, reject, neutral, delay, desc);
}

void ArbitrationOperation::GenerationActions(shared_ptr<ArbitrationOperation> const & thisSPtr, bool partialCompleted, bool completed)
{
    FederationConfig const & config = FederationConfig::GetConfig();

    if (completed_)
    {
        if (!completed)
        {
            CleanupTimer();
            ReportResult(thisSPtr, partialCompleted);
            Threadpool::Post([thisSPtr] { thisSPtr->siteNode_->GetVoteManager().RemoveArbitrationOperation(thisSPtr); });
        }
    }
    else if (partialCompleted_)
    {
        if (!partialCompleted)
        {
            ReportResult(thisSPtr, false);
        }
    }

    StopwatchTime nextCheck = min(expireTime_, checkRevertTime_);
    StopwatchTime now = Stopwatch::Now();

    for (ArbitrationVoteUPtr const & vote : votes_)
    {
        if (revertType_ != type_ && vote->CanRevert(revertType_))
        {
            StartVoteArbitration(thisSPtr, *vote, revertType_);
        }
        else if (!completed_ && !vote->pending_ && vote->count_ < MaxRequestPerVoter)
        {
            if (!vote->HasResult())
            {
                StartVoteArbitration(thisSPtr, *vote, type_);
            }
            else if (vote->result_.MonitorTTL != TimeSpan::MaxValue && canRetry_)
            {
                TimeSpan delay = config.ArbitrationRetryInterval;
                if (vote->count_ == MaxRequestPerVoter - 1)
                {
                    delay = TimeSpan::FromTicks(delay.Ticks * 3);
                }

                if (CheckDueTime(vote->replyReceivedTime_ + delay, now, nextCheck))
                {
                    StartVoteArbitration(thisSPtr, *vote, type_);
                }
            }
        }
    }

    if (!completed_ && nextCheck + StopwatchTime::TimerMargin < nextCheck_)
    {
        nextCheck_ = nextCheck;
        timer_->ChangeWithLowerBoundDelay(nextCheck_ - now);
    }
}

TimeSpan ArbitrationOperation::GetRemoteTTL() const
{
    vector<StopwatchTime> values;
    for (ArbitrationVoteUPtr const & vote : votes_)
    {
        if (vote->HasResult() && vote->result_.SubjectTTL != TimeSpan::MaxValue)
        {
            values.push_back(vote->replyReceivedTime_ + vote->result_.SubjectTTL);
        }
    }

    size_t quorumCount = votes_.size() / 2 + 1;
    ASSERT_IF(values.size() < quorumCount, "Not enough grants");
    sort(values.begin(), values.end());

    return max(values[quorumCount - 1] - Stopwatch::Now(), TimeSpan::Zero);
}

bool ArbitrationOperation::CheckDueTime(StopwatchTime dueTime, StopwatchTime now, StopwatchTime & nextCheck)
{
    if (dueTime < now + StopwatchTime::TimerMargin)
    {
        return true;
    }

    if (dueTime < nextCheck)
    {
        nextCheck = dueTime;
    }

    return false;
}

void ArbitrationOperation::ReportResult(shared_ptr<ArbitrationOperation> const & thisSPtr, bool isDelayed)
{
    WriteInfo("Result", siteNode_->IdString,
        "{0} result {1}/{2}{3}",
        *this, localResult_, remoteResult_, isDelayed ? L" delayed" : L"");

    if (type_ == ArbitrationType::Implicit)
    {
        siteNode_->Table.ReportImplicitArbitrationResult(localResult_ == TimeSpan::MaxValue);
    }
    else if (type_ == ArbitrationType::Query)
    {
        vector<StopwatchTime> values;
        for (ArbitrationVoteUPtr const & vote : votes_)
        {
            if (vote->HasResult())
            {
                values.push_back(vote->replyReceivedTime_ - vote->result_.SubjectTTL);
            }
        }

        StopwatchTime result;
        size_t quorumCount = votes_.size() / 2 + 1;
        if (values.size() >= quorumCount)
        {
            sort(values.begin(), values.end());
            result = values[quorumCount - 1];
        }
        else
        {
            result = StopwatchTime::MaxValue;
        }

        siteNode_->Table.ReportArbitrationQueryResult(remote_.Id, remote_.InstanceId, result);
    }
    else
    {
        if (localResult_ == TimeSpan::Zero)
        {
            siteNode_->ReportArbitrationFailure(local_, remote_, monitorLeaseInstance_, subjectLeaseInstance_, formatString(type_));
            if (revertType_ == ArbitrationType::RevertToReject)
            {
                Threadpool::Post([thisSPtr] { thisSPtr->ReportResultToLeaseAgent(false); }, FederationConfig::GetConfig().ArbitrationRetryInterval);
                return;
            }
        }

        if (!isDelayed || remoteResult_ != TimeSpan::MaxValue)
        {
            ReportResultToLeaseAgent(isDelayed);
        }
    }
}

void ArbitrationOperation::ReportResultToLeaseAgent(bool isDelayed)
{
    siteNode_->GetLeaseAgent().CompleteArbitrationSuccessProcessing(local_.InstanceId, remote_.Id.c_str(), remote_.InstanceId, localResult_, remoteResult_, isDelayed);
}

void ArbitrationOperation::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.Write("{0}-{1} {2} {3}/{4}",
        local_, remote_, type_, monitorLeaseInstance_, subjectLeaseInstance_);
}

bool ArbitrationOperation::IsLocalPreferred(LeaseAgentInstance const & local, LeaseAgentInstance const & remote)
{
    // We could use a statistically more fair logic, but it is not clear
    // that it will be better.
    return (local.InstanceId < remote.InstanceId);
}
