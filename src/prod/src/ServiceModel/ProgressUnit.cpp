// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace ServiceModel
{
    ProgressUnit::ProgressUnit(ServiceModel::ProgressUnitType::Enum itemType)
        : itemType_(itemType)
        , completedItems_(0)
        , totalItems_(0)
        , lastModifiedTimeInTicks_(Common::DateTime::get_Now().Ticks)
    {
    }

    void ProgressUnit::IncrementCompletedItems(size_t value)
    {
        completedItems_ += value;
        lastModifiedTimeInTicks_.store(Common::DateTime::get_Now().Ticks);
    }

    void ProgressUnit::IncrementTotalItems(size_t value)
    {
        totalItems_ += value;
        lastModifiedTimeInTicks_.store(Common::DateTime::get_Now().Ticks);
    }
}

