// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace ServiceModel;
using namespace Store;
using namespace Management::HealthManager;

AttributesStoreData::AttributesStoreData()
    : Store::StoreData()
    , stateFlags_()
{
}

AttributesStoreData::AttributesStoreData(
    Store::ReplicaActivityId const & activityId)
    : Store::StoreData(activityId)
    , stateFlags_()
{
}

AttributesStoreData::~AttributesStoreData()
{
}

bool AttributesStoreData::ShouldReplaceAttributes(AttributesStoreData const & other) const
{
    if (this->UseInstance != other.UseInstance)
    {
        return true;
    }

    if (this->UseInstance)
    {
        return this->InstanceId < other.InstanceId;
    }
    else
    {
        return false;
    }
}

Common::ErrorCode AttributesStoreData::TryAcceptHealthReport(
    ServiceModel::HealthReport const & healthReport,
    __inout bool & skipLsnCheck,
    __inout bool & replaceAttributesMetadata) const
{
    replaceAttributesMetadata = false;

    if (this->IsMarkedForDeletion &&
        (!this->UseInstance || this->InstanceId >= healthReport.EntityInformation->EntityInstance))
    {
        // If current entity is marked for delete and the new report is for the same instance,
        // return special error code to let caller know that the entity is marked for deletion.
        // If entity is currently deleted, only reports for higher instance are accepted.
        return ErrorCode(ErrorCodeValue::HealthEntityDeleted);
    }

    if (!this->UseInstance)
    {
        skipLsnCheck = false;
        return ErrorCode::Success();
    }

    return healthReport.EntityInformation->CheckAgainstInstance(this->InstanceId, healthReport.IsForDelete, skipLsnCheck);
}
