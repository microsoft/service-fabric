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

StringLiteral const TraceComponent("CheckInMemoryEntityDataJI");

CheckInMemoryEntityDataJobItem::CheckInMemoryEntityDataJobItem(
    std::shared_ptr<HealthEntity> const & entity,
    Common::ActivityId const & activityId)
    : CleanupEntityJobItemBase(entity, activityId)
{
}

CheckInMemoryEntityDataJobItem::~CheckInMemoryEntityDataJobItem()
{
}

void CheckInMemoryEntityDataJobItem::ProcessInternal(IHealthJobItemSPtr const & thisSPtr)
{
    this->Entity->CheckInMemoryAgainstStoreData(thisSPtr);
}

void CheckInMemoryEntityDataJobItem::FinishAsyncWork()
{
    Assert::CodingError("{0}: CheckInMemoryEntityDataJobItem should never be called async", *this);
}
