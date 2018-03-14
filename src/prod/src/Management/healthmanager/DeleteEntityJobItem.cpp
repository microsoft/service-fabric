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

StringLiteral const TraceComponent("DeleteEntityJI");

DeleteEntityJobItem::DeleteEntityJobItem(
    std::shared_ptr<HealthEntity> const & entity,
    Common::ActivityId const & activityId)
    : CleanupEntityJobItemBase(entity, activityId)
{
}

DeleteEntityJobItem::~DeleteEntityJobItem()
{
}

void DeleteEntityJobItem::ProcessInternal(IHealthJobItemSPtr const & thisSPtr)
{
    this->Entity->StartDeleteEntity(thisSPtr);
}

void DeleteEntityJobItem::FinishAsyncWork()
{
    this->Entity->EndDeleteEntity(*this, this->PendingAsyncWork);
}
