// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace Management::ClusterManager;
using namespace std;

ComposeDeploymentUpgradeProgress::ComposeDeploymentUpgradeProgress()
    : deploymentName_()
    , applicationName_()
    , upgradeType_(UpgradeType::Enum::Invalid)
    , rollingUpgradeMode_(RollingUpgradeMode::Enum::Invalid)
    , forceRestart_(false)
    , replicaSetCheckTimeoutInSeconds_()
    , monitoringPolicy_()
    , healthPolicy_()
    , targetAppTypeVersion_()
    , composeDeploymentUpgradeState_(ComposeDeploymentUpgradeState::Enum::Invalid)
    , inProgressUpgradeDomain_()
    , nextUpgradeDomain_()
    , completedUpgradeDomains_()
    , pendingUpgradeDomains_()
    , upgradeDuration_()
    , currentUpgradeDomainDuration_()
    , unhealthyEvaluations_()
    , currentUpgradeDomainProgress_()
    , commonUpgradeContextData_()
{
    publicUpgradeState_ = this->ToPublicUpgradeState();
    startTime_ = commonUpgradeContextData_.StartTime;
    failureTime_ = commonUpgradeContextData_.FailureTime;
    applicationStatusDetails_ = commonUpgradeContextData_.UpgradeStatusDetails;
}

ComposeDeploymentUpgradeProgress::ComposeDeploymentUpgradeProgress(
    wstring deploymentName,
    NamingUri && applicationName,
    RollingUpgradeMode::Enum rollingUpgradeMode,
    ULONG replicaSetCheckTimeoutInSeconds,
    RollingUpgradeMonitoringPolicy const & monitoringPolicy,
    shared_ptr<ApplicationHealthPolicy> healthPolicy,
    wstring const & targetAppTypeVersion,
    ComposeDeploymentUpgradeState::Enum upgradeState,
    wstring && statusDetails)
    : deploymentName_(deploymentName)
    , applicationName_(move(applicationName))
    , upgradeType_(UpgradeType::Enum::Invalid)
    , rollingUpgradeMode_(rollingUpgradeMode)
    , forceRestart_(false)
    , replicaSetCheckTimeoutInSeconds_(replicaSetCheckTimeoutInSeconds)
    , monitoringPolicy_(move(monitoringPolicy))
    , healthPolicy_(healthPolicy)
    , targetAppTypeVersion_(move(targetAppTypeVersion))
    , composeDeploymentUpgradeState_(upgradeState)
    , inProgressUpgradeDomain_()
    , nextUpgradeDomain_()
    , completedUpgradeDomains_()
    , pendingUpgradeDomains_()
    , upgradeInstance_()
    , upgradeDuration_()
    , currentUpgradeDomainDuration_()
    , unhealthyEvaluations_()
    , currentUpgradeDomainProgress_()
    , statusDetails_(move(statusDetails))
    , commonUpgradeContextData_()
{
    publicUpgradeState_ = this->ToPublicUpgradeState();
    startTime_ = commonUpgradeContextData_.StartTime;
    failureTime_ = commonUpgradeContextData_.FailureTime;
    applicationStatusDetails_ = commonUpgradeContextData_.UpgradeStatusDetails;
}

ComposeDeploymentUpgradeProgress::ComposeDeploymentUpgradeProgress(
    wstring const & deploymentName,
    NamingUri && applicationName,
    UpgradeType::Enum const upgradeType,
    RollingUpgradeMode::Enum const rollingUpgradeMode,
    ULONG const replicaSetCheckTimeoutInSeconds,
    RollingUpgradeMonitoringPolicy const & monitoringPolicy,
    shared_ptr<ApplicationHealthPolicy> healthPolicy,
    wstring const & targetAppTypeVersion,
    ComposeDeploymentUpgradeState::Enum const upgradeState,
    wstring && inProgressUpgradeDomain,
    vector<wstring> completedUpgradeDomains,
    vector<wstring> pendingUpgradeDomains,
    uint64 upgradeInstance,
    TimeSpan upgradeDuration,
    TimeSpan currentUpgradeDomainDuration,
    vector<HealthEvaluation> && unhealthyEvaluations,
    Reliability::UpgradeDomainProgress && currentUpgradeDomainProgress,
    wstring && statusDetails,
    CommonUpgradeContextData && commonUpgradeContextData)
    : deploymentName_(deploymentName)
    , applicationName_(move(applicationName))
    , upgradeType_(upgradeType)
    , rollingUpgradeMode_(rollingUpgradeMode)
    , replicaSetCheckTimeoutInSeconds_(replicaSetCheckTimeoutInSeconds)
    , monitoringPolicy_(move(monitoringPolicy))
    , healthPolicy_(healthPolicy)
    , targetAppTypeVersion_(move(targetAppTypeVersion))
    , composeDeploymentUpgradeState_(upgradeState)
    , inProgressUpgradeDomain_(move(inProgressUpgradeDomain))
    , nextUpgradeDomain_(pendingUpgradeDomains.empty() ? L"" : pendingUpgradeDomains.front())
    , completedUpgradeDomains_(move(completedUpgradeDomains))
    , pendingUpgradeDomains_(move(pendingUpgradeDomains))
    , upgradeInstance_(upgradeInstance)
    , upgradeDuration_(upgradeDuration)
    , currentUpgradeDomainDuration_(currentUpgradeDomainDuration)
    , unhealthyEvaluations_(move(unhealthyEvaluations))
    , currentUpgradeDomainProgress_(move(currentUpgradeDomainProgress))
    , statusDetails_(move(statusDetails))
    , commonUpgradeContextData_(move(commonUpgradeContextData))
{
    publicUpgradeState_ = this->ToPublicUpgradeState();
    forceRestart_ = (upgradeType == UpgradeType::Rolling_ForceRestart ? true: false);
    startTime_ = commonUpgradeContextData_.StartTime;
    failureTime_ = commonUpgradeContextData_.FailureTime;
    applicationStatusDetails_ = commonUpgradeContextData_.UpgradeStatusDetails;
}

FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE ComposeDeploymentUpgradeProgress::ToPublicUpgradeState() const
{
    switch (composeDeploymentUpgradeState_)
    {
        case ComposeDeploymentUpgradeState::Enum::ProvisioningTarget:
            return FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_PROVISIONING_TARGET;

        case ComposeDeploymentUpgradeState::Enum::RollingForward:
            // UnmonitoredAuto mode always returns IN_PROGRESS since there is no waiting or pending health check.
            // For UnmonitoredManual, "pending" indicates that the system is 
            // waiting for an explicit move next.
            //
            // For Monitored, "pending" indicates that a health check is pending.
            //
            if (inProgressUpgradeDomain_.empty() && !pendingUpgradeDomains_.empty() && rollingUpgradeMode_ != RollingUpgradeMode::Enum::UnmonitoredAuto)
            {
                return FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_ROLLING_FORWARD_PENDING;
            }
            else
            {
                return FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_ROLLING_FORWARD_IN_PROGRESS;
            }

        case ComposeDeploymentUpgradeState::Enum::UnprovisioningCurrent:
            return FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_UNPROVISIONING_CURRENT;

        case ComposeDeploymentUpgradeState::Enum::CompletedRollforward:
            return FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_ROLLING_FORWARD_COMPLETED;
        
        case ComposeDeploymentUpgradeState::Enum::RollingBack:
            return FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_ROLLING_BACK_IN_PROGRESS;

        case ComposeDeploymentUpgradeState::Enum::UnprovisioningTarget:
            return FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_UNPROVISIONING_TARGET;

        case ComposeDeploymentUpgradeState::Enum::CompletedRollback:
            return FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_ROLLING_BACK_COMPLETED;

        case ComposeDeploymentUpgradeState::Enum::Failed:
            return FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_FAILED;

        default:
            return FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_STATE_INVALID;
    }
}

void ComposeDeploymentUpgradeProgress::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_PROGRESS & publicResult) const
{
    publicResult.DeploymentName = heap.AddString(deploymentName_);
    publicResult.ApplicationName = heap.AddString(applicationName_.ToString());

    switch (upgradeType_)
    {
        case UpgradeType::Rolling:
        case UpgradeType::Rolling_NotificationOnly:
        case UpgradeType::Rolling_ForceRestart:
            publicResult.UpgradeKind = FABRIC_APPLICATION_UPGRADE_KIND_ROLLING;
            break;
        default:
            publicResult.UpgradeKind = FABRIC_APPLICATION_UPGRADE_KIND_INVALID;
    }

    if (publicResult.UpgradeKind == FABRIC_APPLICATION_UPGRADE_KIND_ROLLING)
    {
        auto policyDescription = heap.AddItem<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION>();

        switch (rollingUpgradeMode_)
        {
            case RollingUpgradeMode::UnmonitoredAuto:
                policyDescription->RollingUpgradeMode = FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_AUTO;
                break;

            case RollingUpgradeMode::UnmonitoredManual:
                policyDescription->RollingUpgradeMode = FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_MANUAL;
                break;

            case RollingUpgradeMode::Monitored:
            {
                policyDescription->RollingUpgradeMode = FABRIC_ROLLING_UPGRADE_MODE_MONITORED;

                auto policyDescriptionEx = heap.AddItem<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX1>();

                auto monitoringPolicy = heap.AddItem<FABRIC_ROLLING_UPGRADE_MONITORING_POLICY>();
                policyDescriptionEx->MonitoringPolicy = monitoringPolicy.GetRawPointer();
                monitoringPolicy_.ToPublicApi(heap, *monitoringPolicy);

                if (healthPolicy_)
                {
                    auto healthPolicy = heap.AddItem<FABRIC_APPLICATION_HEALTH_POLICY>();
                    policyDescriptionEx->HealthPolicy = healthPolicy.GetRawPointer();
                    healthPolicy_->ToPublicApi(heap, *healthPolicy);
                }
                else
                {
                    policyDescriptionEx->HealthPolicy = nullptr;
                }

                policyDescription->Reserved = policyDescriptionEx.GetRawPointer();

                break;
            }

            default:
                policyDescription->RollingUpgradeMode = FABRIC_ROLLING_UPGRADE_MODE_INVALID;
        }
        policyDescription->ForceRestart = forceRestart_? TRUE : FALSE;
        policyDescription->UpgradeReplicaSetCheckTimeoutInSeconds = replicaSetCheckTimeoutInSeconds_;
        publicResult.UpgradePolicyDescription = policyDescription.GetRawPointer();
    }
    else
    {
        publicResult.UpgradePolicyDescription = nullptr;
    }

    publicResult.TargetApplicationTypeVersion = heap.AddString(targetAppTypeVersion_);
    publicResult.UpgradeState = publicUpgradeState_;
    publicResult.NextUpgradeDomain = nextUpgradeDomain_.empty() ? nullptr : heap.AddString(nextUpgradeDomain_);

    ReferenceArray<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION> domains;
    UpgradeHelper::ToPublicUpgradeDomains(
        heap,
        inProgressUpgradeDomain_,
        pendingUpgradeDomains_,
        completedUpgradeDomains_,
        domains);
    auto domainListPtr = heap.AddItem<FABRIC_UPGRADE_DOMAIN_STATUS_DESCRIPTION_LIST>();
    domainListPtr->Count = static_cast<ULONG>(domains.GetCount());
    domainListPtr->Items = domains.GetRawArray();
    publicResult.UpgradeDomains = domainListPtr.GetRawPointer();

    publicResult.UpgradeDurationInSeconds = static_cast<DWORD>(upgradeDuration_.TotalSeconds());
    publicResult.CurrentUpgradeDomainDurationInSeconds = static_cast<DWORD>(currentUpgradeDomainDuration_.TotalSeconds());
    
    auto publicHealthEvaluationsPtr = heap.AddItem<FABRIC_HEALTH_EVALUATION_LIST>();
    auto error = PublicApiHelper::ToPublicApiList<HealthEvaluation, FABRIC_HEALTH_EVALUATION, FABRIC_HEALTH_EVALUATION_LIST>(
        heap, 
        unhealthyEvaluations_, 
        *publicHealthEvaluationsPtr); 
    if (error.IsSuccess())
    {
        publicResult.ApplicationUnhealthyEvaluations = publicHealthEvaluationsPtr.GetRawPointer();
    }
    else
    {
        Trace.WriteError("ComposeDeploymentUpgradeStatusDescription", "Unhealthy evaluations to public API failed: error={0}", error);

        publicResult.ApplicationUnhealthyEvaluations = nullptr;
    }

    auto upgradeDomainProgressPtr = heap.AddItem<FABRIC_UPGRADE_DOMAIN_PROGRESS>();
    currentUpgradeDomainProgress_.ToPublicApi(heap, *upgradeDomainProgressPtr);
    publicResult.CurrentUpgradeDomainProgress = upgradeDomainProgressPtr.GetRawPointer();
    publicResult.UpgradeStatusDetails = heap.AddString(statusDetails_);

    publicResult.StartTimestampUtc = commonUpgradeContextData_.StartTime.AsFileTime;
    publicResult.FailureTimestampUtc = commonUpgradeContextData_.FailureTime.AsFileTime;
    publicResult.FailureReason = Management::ClusterManager::UpgradeFailureReason::ToPublicApi(commonUpgradeContextData_.FailureReason);
    if (commonUpgradeContextData_.FailureTime != DateTime::Zero)
    {
        auto upgradeProgressAtFailurePtr = heap.AddItem<FABRIC_UPGRADE_DOMAIN_PROGRESS>();
        commonUpgradeContextData_.UpgradeProgressAtFailure.ToPublicApi(heap, *upgradeProgressAtFailurePtr);
        publicResult.UpgradeDomainProgressAtFailure = upgradeProgressAtFailurePtr.GetRawPointer();
    }

    publicResult.ApplicationUpgradeStatusDetails = commonUpgradeContextData_.UpgradeStatusDetails.empty() ? nullptr : heap.AddString(commonUpgradeContextData_.UpgradeStatusDetails);
}
