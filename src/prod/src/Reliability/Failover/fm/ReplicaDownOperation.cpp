// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Federation;
using namespace ServiceModel;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;

ReplicaDownOperation::ReplicaDownOperation(
    FailoverManager & fm,
    NodeInstance const& from)
    : root_(fm, fm.CreateComponentRoot())
    , from_(from)
    , isCompleted_(false)
{
}

ReplicaDownOperation::~ReplicaDownOperation()
{
}

void ReplicaDownOperation::Start(
    ReplicaDownOperationSPtr const& thisSPtr,
    FailoverManager & fm,
    ReplicaListMessageBody && body)
{
    fm.MessageEvents.ReplicaDown(wformatString(from_.Id), from_, body);

    timer_ = Timer::Create(TimerTagDefault, [thisSPtr](TimerSPtr const&) { thisSPtr->OnTimer(); });
    timer_->Change(max(FailoverConfig::GetConfig().FMMessageRetryInterval - TimeSpan::FromSeconds(5.0), TimeSpan::FromSeconds(5.0)));

    for (auto const& pair : body.Replicas)
    {
        pending_.insert(pair.first);
    }

    for (auto const& pair : body.Replicas)
    {
        FailoverUnitId const& failoverUnitId = pair.first;
        ReplicaDescription replicaDescription = move(pair.second);

        fm.MessageEvents.FTReplicaDownReceive(failoverUnitId.Guid, replicaDescription);

        ReplicaDownTask * pTask = new ReplicaDownTask(thisSPtr, fm, failoverUnitId, move(replicaDescription), from_);
        DynamicStateMachineTaskUPtr task(pTask);

        bool result = fm.FailoverUnitCacheObj.TryProcessTaskAsync(failoverUnitId, task, from_, false);
        if (!result)
        {
            fm.WriteWarning(
                "ReplicaDown", wformatString(failoverUnitId),
                "FailoverUnit not found: {0}", failoverUnitId);
        }
    }

    AcquireWriteLock lock(lock_);

    if (!isCompleted_ && pending_.empty())
    {
        CompleteCallerHoldsLock(fm);
    }
}

void ReplicaDownOperation::AddResult(
    FailoverManager & fm,
    FailoverUnitId const& failoverUnitId,
    ReplicaDescription && replicaDescription)
{
    AcquireWriteLock lock(lock_);

    if (isCompleted_)
    {
        return;
    }

    processed_.insert(make_pair(failoverUnitId, move(replicaDescription)));

    auto result = pending_.erase(failoverUnitId);

    ASSERT_IF(result == 0, "ReplicaDownOperation: FailoverUnit not found for {0}", failoverUnitId);

    if (pending_.empty())
    {
        CompleteCallerHoldsLock(fm);
    }
}

void ReplicaDownOperation::OnTimer()
{
    auto holder = root_.lock();
    if (holder)
    {
        FailoverManager & fm = holder->RootedObject;

        AcquireWriteLock lock(lock_);

        if (isCompleted_)
        {
            return;
        }

        if (pending_.size() > 0)
        {
            fm.MessageEvents.ReplicaDownTimeout(wformatString(from_.Id), from_, pending_.size(), *pending_.begin());
        }

        CompleteCallerHoldsLock(fm);
    }
}

void ReplicaDownOperation::CompleteCallerHoldsLock(FailoverManager & fm)
{
    isCompleted_ = true;

    if (timer_)
    {
        timer_->Cancel();
        timer_ = nullptr;
    }

    if (processed_.size() > 0)
    {
        ReplicaListMessageBody replyBody(move(processed_));

        fm.MessageEvents.ReplicaDownReply(wformatString(from_.Id), from_, replyBody);

        MessageUPtr reply = RSMessage::GetReplicaDownReply().CreateMessage<ReplicaListMessageBody>(replyBody);
        reply->Headers.Add(GenerationHeader(fm.Generation, fm.IsMaster));
        fm.SendToNodeAsync(move(reply), from_);
    }
}
