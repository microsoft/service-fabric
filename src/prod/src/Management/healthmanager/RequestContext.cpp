// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace std;
using namespace Store;
using namespace ServiceModel;
using namespace Management::HealthManager;

RequestContext::RequestContext(
    Store::ReplicaActivityId const & replicaActivityId,
    Common::AsyncOperationSPtr const & owner)
    : ReplicaActivityTraceComponent(replicaActivityId)
    , owner_(owner)
#ifdef DBG
    , timedOwner_(dynamic_cast<TimedAsyncOperation*>(owner.get()))
#else
    , timedOwner_(static_cast<TimedAsyncOperation*>(owner.get()))
#endif
    , startTime_(Stopwatch::GetTimestamp())
    , error_(ErrorCodeValue::NotReady)
    , estimatedSize_(0)
    , isAccepted_(false)
{
}

RequestContext::RequestContext(RequestContext && other)
    : ReplicaActivityTraceComponent(move(other))
    , owner_(move(other.owner_))
    , timedOwner_(other.timedOwner_)
    , startTime_(move(other.startTime_))
    , error_(move(other.error_))
    , estimatedSize_(move(other.estimatedSize_))
    , isAccepted_(move(other.isAccepted_))
{
}

RequestContext & RequestContext::operator = (RequestContext && other)
{
    if (this != &other)
    {
        owner_ = move(other.owner_);
        timedOwner_ = other.timedOwner_;
        startTime_ = move(other.startTime_);
        error_ = move(other.error_);
        estimatedSize_ = move(other.estimatedSize_);
        isAccepted_ = move(other.isAccepted_);
    }

    ReplicaActivityTraceComponent::operator=(move(other));
    return *this;
}

RequestContext::~RequestContext()
{
}

Common::TimeSpan RequestContext::get_OwnerRemainingTime() const
{
#ifdef DBG
    ASSERT_IF(!timedOwner_, "{0}: timed owner not set");
#endif

    return timedOwner_->RemainingTime;
}

void RequestContext::SetError(ErrorCode const & error) const
{
    if (!IsErrorSet())
    {
        error_ = error;
    }
}

void RequestContext::SetError(ErrorCode && error) const
{
    if (!IsErrorSet())
    {
        error_ = move(error);
    }
}

uint64 RequestContext::GetEstimatedSize() const
{
    if (estimatedSize_ == 0)
    {
        estimatedSize_ = static_cast<uint64>(EstimateSize());
    }

    return estimatedSize_;
}
