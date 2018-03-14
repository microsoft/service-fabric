// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Management::RepairManager;

UpdateRepairTaskHealthPolicyMessageBody::UpdateRepairTaskHealthPolicyMessageBody()
    : taskId_()
    , version_(0)
    , scope_()
    , performPreparingHealthCheckSPtr_()
    , performRestoringHealthCheckSPtr_()
{
}

UpdateRepairTaskHealthPolicyMessageBody::UpdateRepairTaskHealthPolicyMessageBody(
    RepairScopeIdentifierBaseSPtr const & scope,
    wstring const & taskId,
    int64 version,
    shared_ptr<bool> const & performPreparingHealthCheckSPtr,
    shared_ptr<bool> const & performRestoringHealthCheckSPtr)
    : taskId_(taskId)
    , version_(version)
    , scope_(scope)
    , performPreparingHealthCheckSPtr_(move(performPreparingHealthCheckSPtr))
    , performRestoringHealthCheckSPtr_(move(performRestoringHealthCheckSPtr))
{
}

void UpdateRepairTaskHealthPolicyMessageBody::ReplaceScope(RepairScopeIdentifierBaseSPtr const & newScope)
{
    scope_ = newScope;
}

ErrorCode UpdateRepairTaskHealthPolicyMessageBody::FromPublicApi(FABRIC_REPAIR_TASK_HEALTH_POLICY_UPDATE_DESCRIPTION const & publicDescription)
{
    if (publicDescription.Scope == NULL) { return ErrorCodeValue::InvalidArgument; }
    auto error = RepairScopeIdentifierBase::CreateSPtrFromPublicApi(*publicDescription.Scope, scope_);
    if (!error.IsSuccess()) { return error; }

    HRESULT hr = StringUtility::LpcwstrToWstring(
        publicDescription.RepairTaskId,
        false,
        taskId_);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    version_ = static_cast<uint64>(publicDescription.Version);

    if (publicDescription.Flags & FABRIC_REPAIR_TASK_HEALTH_POLICY_UPDATE_SETTINGS_FLAGS::FABRIC_REPAIR_TASK_HEALTH_POLICY_UPDATE_SETTINGS_HONOR_PERFORM_PREPARING_HEALTH_CHECK)
    {
        performPreparingHealthCheckSPtr_ = make_shared<bool>(publicDescription.PerformPreparingHealthCheck == TRUE);
    }
    if (publicDescription.Flags & FABRIC_REPAIR_TASK_HEALTH_POLICY_UPDATE_SETTINGS_FLAGS::FABRIC_REPAIR_TASK_HEALTH_POLICY_UPDATE_SETTINGS_HONOR_PERFORM_RESTORING_HEALTH_CHECK)
    {
        performRestoringHealthCheckSPtr_ = make_shared<bool>(publicDescription.PerformRestoringHealthCheck == TRUE);
    }

    return ErrorCode::Success();
}

