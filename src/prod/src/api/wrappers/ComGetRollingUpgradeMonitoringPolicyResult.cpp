// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace ServiceModel;

using namespace std;

ComGetRollingUpgradeMonitoringPolicyResult::ComGetRollingUpgradeMonitoringPolicyResult(
    RollingUpgradeMonitoringPolicy const & policy)
    : IFabricGetRollingUpgradeMonitoringPolicyResult()
    , ComUnknownBase()
    , heap_()
    , policy_()
{
    policy_ = heap_.AddItem<FABRIC_ROLLING_UPGRADE_MONITORING_POLICY>();
    policy.ToPublicApi(heap_, *policy_);
}

ComGetRollingUpgradeMonitoringPolicyResult::~ComGetRollingUpgradeMonitoringPolicyResult()
{
}

const FABRIC_ROLLING_UPGRADE_MONITORING_POLICY * STDMETHODCALLTYPE ComGetRollingUpgradeMonitoringPolicyResult::get_Policy()
{
    return policy_.GetRawPointer();
}
