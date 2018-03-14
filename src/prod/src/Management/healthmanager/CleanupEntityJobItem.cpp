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

StringLiteral const TraceComponent("CleanupEntityJI");

CleanupEntityJobItem::CleanupEntityJobItem(
    std::shared_ptr<HealthEntity> const & entity,
    wstring const & invalidateSource,
    FABRIC_SEQUENCE_NUMBER invalidateLsn,
    Common::ActivityId const & activityId)
    : CleanupEntityJobItemBase(entity, activityId)
    , invalidateSource_(invalidateSource)
    , invalidateLsn_(invalidateLsn)
{
}

CleanupEntityJobItem::~CleanupEntityJobItem()
{
}

void CleanupEntityJobItem::ProcessInternal(IHealthJobItemSPtr const & thisSPtr)
{
    this->Entity->StartCleanupEntity(thisSPtr, invalidateSource_, invalidateLsn_);
}

void CleanupEntityJobItem::FinishAsyncWork()
{
    this->Entity->EndCleanupEntity(*this, this->PendingAsyncWork);
}
