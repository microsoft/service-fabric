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

StringLiteral const TraceComponent("CleanupEntityExpiredTransientEventsJI");

CleanupEntityExpiredTransientEventsJobItem::CleanupEntityExpiredTransientEventsJobItem(
    std::shared_ptr<HealthEntity> const & entity,
    Common::ActivityId const & activityId)
    : CleanupEntityJobItemBase(entity, activityId)
{
}

CleanupEntityExpiredTransientEventsJobItem::~CleanupEntityExpiredTransientEventsJobItem()
{
}

void CleanupEntityExpiredTransientEventsJobItem::ProcessInternal(IHealthJobItemSPtr const & thisSPtr)
{
    this->Entity->StartCleanupExpiredTransientEvents(thisSPtr);
}

void CleanupEntityExpiredTransientEventsJobItem::FinishAsyncWork()
{
    this->Entity->EndCleanupExpiredTransientEvents(*this, this->PendingAsyncWork);
}
