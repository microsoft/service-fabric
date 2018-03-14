// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace std;
using namespace Store;
using namespace Management::HealthManager;

StringLiteral const TraceComponent("CleanupEntityJobItem");

CleanupEntityJobItemBase::CleanupEntityJobItemBase(
    std::shared_ptr<HealthEntity> const & entity,
    Common::ActivityId const & activityId)
    : EntityJobItem(entity)
    , replicaActivityId_(entity->PartitionedReplicaId, activityId)
{
}

CleanupEntityJobItemBase::~CleanupEntityJobItemBase()
{
}

void CleanupEntityJobItemBase::OnComplete(Common::ErrorCode const & error)
{
    if (!error.IsSuccess())
    {
        HealthManagerReplica::WriteInfo(
            TraceComponent,
            this->JobId,
            "{0}: {1} failed with {2}",
            this->ReplicaActivityId.TraceId,
            this->TypeString,
            error);
    }

    this->Entity->OnCleanupJobCompleted();
}

bool CleanupEntityJobItemBase::CanCombine(IHealthJobItemSPtr const & other) const
{
    // Can combine only with other cleanup entity job items of the same type
    return (other->Type == this->Type);
}

void CleanupEntityJobItemBase::Append(IHealthJobItemSPtr && other)
{
    // Both requests execute the same work, so simply ignore the second one
    ASSERT_IFNOT(other->Type == this->Type, "{0}: can't append {1}", *this, *other);
}
