//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace std;
using namespace Store;
using namespace Management::HealthManager;

StringLiteral const TraceComponent("AutoCleanupEventsJI");

AutoCleanupEventsJobItem::AutoCleanupEventsJobItem(
    std::shared_ptr<HealthEntity> const & entity,
    Common::ActivityId const & activityId)
    : CleanupEntityJobItemBase(entity, activityId)
{
}

AutoCleanupEventsJobItem::~AutoCleanupEventsJobItem()
{
}

void AutoCleanupEventsJobItem::ProcessInternal(IHealthJobItemSPtr const & thisSPtr)
{
    this->Entity->StartAutoCleanupEvents(thisSPtr);
}

void AutoCleanupEventsJobItem::FinishAsyncWork()
{
    this->Entity->EndAutoCleanupEvents(*this, this->PendingAsyncWork);
}
