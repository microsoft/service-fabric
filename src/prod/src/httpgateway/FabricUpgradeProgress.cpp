// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Api;
using namespace Management::ClusterManager;
using namespace ServiceModel;
using namespace Naming;
using namespace HttpGateway;

FabricUpgradeProgress::FabricUpgradeProgress()
    : codeVersion_()
    , configVersion_()
    , upgradeDomainStatus_()
    , upgradeState_(FABRIC_UPGRADE_STATE_INVALID)
    , nextUpgradeDomain_()
    , rollingUpgradeMode_(FABRIC_ROLLING_UPGRADE_MODE_INVALID)
    , upgradeDescription_()
    , upgradeDuration_(TimeSpan::Zero)
    , upgradeDomainDuration_(TimeSpan::Zero)
    , unhealthyEvaluations_()
    , currentUpgradeDomainProgress_()
    , startTime_(DateTime::Zero)
    , failureTime_(DateTime::Zero)
    , failureReason_(FABRIC_UPGRADE_FAILURE_REASON_NONE)
    , upgradeDomainProgressAtFailure_()
{
}

ErrorCode FabricUpgradeProgress::FromInternalInterface(IUpgradeProgressResultPtr &resultPtr)
{
    codeVersion_ = resultPtr->GetTargetCodeVersion().ToString();
    configVersion_ = resultPtr->GetTargetConfigVersion().ToString();

    wstring inProgressDomain;
    vector<wstring> pendingDomains;
    vector<wstring> completedDomains;

    auto err = resultPtr->GetUpgradeDomains(inProgressDomain, pendingDomains, completedDomains);
    if (!err.IsSuccess()) { return err; }

    upgradeState_ = resultPtr->ToPublicUpgradeState();

    if (!inProgressDomain.empty())
    {
        upgradeDomainStatus_.push_back(move(UpgradeDomainStatus(inProgressDomain, FABRIC_UPGRADE_DOMAIN_STATE_IN_PROGRESS)));
    }

    for (auto itr = pendingDomains.begin(); itr != pendingDomains.end(); itr++)
    {
        upgradeDomainStatus_.push_back(move(UpgradeDomainStatus(*itr, FABRIC_UPGRADE_DOMAIN_STATE_PENDING)));
    }

    for (auto itr = completedDomains.begin(); itr != completedDomains.end(); itr++)
    {
        upgradeDomainStatus_.push_back(move(UpgradeDomainStatus(*itr, FABRIC_UPGRADE_DOMAIN_STATE_COMPLETED)));
    }

    nextUpgradeDomain_ = resultPtr->GetNextUpgradeDomain();

    rollingUpgradeMode_ = RollingUpgradeMode::ToPublicApi(resultPtr->GetRollingUpgradeMode());

    if (resultPtr->GetUpgradeDescription())
    {
        upgradeDescription_ = make_shared<FabricUpgradeDescriptionWrapper>();

        resultPtr->GetUpgradeDescription()->ToWrapper(*upgradeDescription_);
    }

    upgradeDuration_ = resultPtr->GetUpgradeDuration();

    upgradeDomainDuration_ = resultPtr->GetCurrentUpgradeDomainDuration();

    unhealthyEvaluations_ = resultPtr->GetHealthEvaluations();

    currentUpgradeDomainProgress_ = resultPtr->GetCurrentUpgradeDomainProgress();

    startTime_ = resultPtr->GetStartTime();
    
    failureTime_ = resultPtr->GetFailureTime();

    failureReason_ = UpgradeFailureReason::ToPublicApi(resultPtr->GetFailureReason());

    upgradeDomainProgressAtFailure_ = resultPtr->GetUpgradeDomainProgressAtFailure();

    return ErrorCode::Success();
}
