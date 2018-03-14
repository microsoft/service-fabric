// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::ClusterManager;

template bool ModifyUpgradeHelper::TryModifyUpgrade
<ApplicationUpgradeDescription, ApplicationUpgradeUpdateDescription>(
    __in ApplicationUpgradeDescription &,
    ApplicationUpgradeUpdateDescription const &,
    __out std::wstring &);

template bool ModifyUpgradeHelper::TryModifyUpgrade
<FabricUpgradeDescription, FabricUpgradeUpdateDescription>(
    __in FabricUpgradeDescription &,
    FabricUpgradeUpdateDescription const &,
    __out std::wstring &);

// use template instead of base class for version compatibility
// (fields can only be extended now in derived classes)
//
template <class TUpgradeDescription, class TUpdateDescription>
bool ModifyUpgradeHelper::TryModifyUpgrade(
    __in TUpgradeDescription & upgrade,
    TUpdateDescription const & update,
    __out std::wstring & validationErrorMessage)
{
    shared_ptr<RollingUpgradeUpdateDescription> updateDescription = update.UpdateDescription;

    if (updateDescription)
    {
        if (updateDescription->ForceRestart)
        {
            upgrade.upgradeType_ = (*(updateDescription->ForceRestart)
                ? UpgradeType::Rolling_ForceRestart
                : UpgradeType::Rolling);
        }

        if (updateDescription->RollingUpgradeMode)
        {
            upgrade.rollingUpgradeMode_ = *(updateDescription->RollingUpgradeMode);
        }

        if (updateDescription->ReplicaSetCheckTimeout)
        {
            upgrade.replicaSetCheckTimeout_ = UpgradeHelper::GetReplicaSetCheckTimeoutForModify(
                *(updateDescription->ReplicaSetCheckTimeout));
        }

        if (upgrade.rollingUpgradeMode_ == RollingUpgradeMode::Monitored)
        {
            upgrade.monitoringPolicy_.Modify(*updateDescription);
        }
        else if (
            updateDescription->FailureAction ||
            updateDescription->HealthCheckWaitDuration ||
            updateDescription->HealthCheckStableDuration ||
            updateDescription->HealthCheckRetryTimeout ||
            updateDescription->UpgradeTimeout ||
            updateDescription->UpgradeDomainTimeout)
        {
            validationErrorMessage = wformatString("{0} {1}", GET_CM_RC( Invalid_Upgrade_Mode2 ), upgrade.rollingUpgradeMode_);
            return false;
        }
    }

    if (upgrade.rollingUpgradeMode_ == RollingUpgradeMode::Monitored)
    {
        if (update.HealthPolicy)
        {
            upgrade.healthPolicy_ = *update.HealthPolicy;

            upgrade.isHealthPolicyValid_ = true;
        }

        if (!upgrade.monitoringPolicy_.TryValidate(validationErrorMessage) ||
            !upgrade.healthPolicy_.TryValidate(validationErrorMessage))
        {
            return false;
        }

        __if_not_exists(TUpdateDescription::ApplicationName)
        {
            if (update.EnableDeltaHealthEvaluation)
            {
                upgrade.enableDeltaHealthEvaluation_ = *update.EnableDeltaHealthEvaluation;
            }

            if (update.UpgradeHealthPolicy)
            {
                upgrade.upgradeHealthPolicy_ = *update.UpgradeHealthPolicy;

                upgrade.isUpgradeHealthPolicyValid_ = true;
            }

            if (!upgrade.upgradeHealthPolicy_.TryValidate(validationErrorMessage))
            {
                return false;
            }

            if (update.ApplicationHealthPolicies)
            {
                upgrade.applicationHealthPolicies_ = update.ApplicationHealthPolicies;
                if (!upgrade.ApplicationHealthPolicies->TryValidate(validationErrorMessage))
                {
                    return false;
                }
            }
        }
    }
    else if (update.HealthPolicy)
    {
        validationErrorMessage = wformatString("{0} {1}", GET_CM_RC( Invalid_Upgrade_Mode2 ), upgrade.rollingUpgradeMode_);
        return false;
    }
    else
    {
        __if_not_exists(TUpdateDescription::ApplicationName)
        {
            if (update.UpgradeHealthPolicy || update.EnableDeltaHealthEvaluation || update.ApplicationHealthPolicies)
            {
                validationErrorMessage = wformatString("{0} {1}", GET_CM_RC( Invalid_Upgrade_Mode2 ), upgrade.rollingUpgradeMode_);
                return false;
            }
        }
    }

    return true;
}

template ErrorCode ModifyUpgradeHelper::FromPublicUpdateDescription
<ApplicationUpgradeUpdateDescription, FABRIC_APPLICATION_UPGRADE_UPDATE_DESCRIPTION, ApplicationHealthPolicy, FABRIC_APPLICATION_HEALTH_POLICY>(
    __in ApplicationUpgradeUpdateDescription & internalDescription,
    FABRIC_APPLICATION_UPGRADE_UPDATE_DESCRIPTION const & publicDescription);

template ErrorCode ModifyUpgradeHelper::FromPublicUpdateDescription
<FabricUpgradeUpdateDescription, FABRIC_UPGRADE_UPDATE_DESCRIPTION, ClusterHealthPolicy, FABRIC_CLUSTER_HEALTH_POLICY>(
    __in FabricUpgradeUpdateDescription & internalDescription,
    FABRIC_UPGRADE_UPDATE_DESCRIPTION const & publicDescription);

template <
    class TInternalDescription, 
    class TPublicDescription, 
    class TInternalHealthPolicy, 
    class TPublicHealthPolicy>
ErrorCode ModifyUpgradeHelper::FromPublicUpdateDescription(
    __in TInternalDescription & internalDescription,
    TPublicDescription const & publicDescription)
{
    int expectedKind = FABRIC_UPGRADE_KIND_ROLLING;
    __if_exists (TPublicDescription::ApplicationName)
    {
        expectedKind = FABRIC_APPLICATION_UPGRADE_KIND_ROLLING;
    }

    if (static_cast<int>(publicDescription.UpgradeKind) != expectedKind) 
    { 
        return ErrorCode(
            ErrorCodeValue::InvalidArgument, 
            wformatString("{0} {1}", GET_CM_RC( Invalid_Upgrade_Kind ), static_cast<int>(publicDescription.UpgradeKind)));
    }

    if (publicDescription.UpdateFlags == FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_NONE)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument, 
            wformatString("{0} {1}", GET_CM_RC( Invalid_Flags ), static_cast<int>(publicDescription.UpdateFlags)));
    }

    if (publicDescription.UpgradePolicyDescription == NULL)
    {
        return ErrorCode(
            ErrorCodeValue::ArgumentNull,
            wformatString("{0} {1}", GET_CM_RC( Invalid_Null_Pointer ), "UpgradePolicyDescription"));
    }

    auto flags = publicDescription.UpdateFlags;

    auto policyPtr = reinterpret_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION*>(publicDescription.UpgradePolicyDescription);

    if (RollingUpgradeUpdateDescription::HasRollingUpgradeUpdateDescriptionFlags(flags))
    {
        auto description = make_shared<RollingUpgradeUpdateDescription>();
        
        auto error = description->FromPublicApi(
            static_cast<FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS>(flags),
            *policyPtr);

        if (!error.IsSuccess()) { return error; }

        internalDescription.updateDescription_ = std::move(description);
    }
    
    auto healthPolicyFlagSet = flags & FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_POLICY;
    auto enableDeltasFlagSet = flags & FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_ENABLE_DELTAS;
    auto upgradeHealthPolicyFlagSet = flags & FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_HEALTH_POLICY;
    auto applicationHealthPoliciesFlagSet = flags & FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_APPLICATION_HEALTH_POLICY_MAP;

    if (healthPolicyFlagSet || enableDeltasFlagSet || upgradeHealthPolicyFlagSet || applicationHealthPoliciesFlagSet)
    {
        if (policyPtr->Reserved == NULL) { return ErrorCode::FromHResult(E_POINTER); }

        auto ex1Ptr = reinterpret_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX1*>(policyPtr->Reserved);

        if (healthPolicyFlagSet)
        {
            if (ex1Ptr->HealthPolicy == NULL) { return ErrorCode::FromHResult(E_POINTER); }

            auto healthPtr = reinterpret_cast<TPublicHealthPolicy*>(ex1Ptr->HealthPolicy);

            auto policy = make_shared<TInternalHealthPolicy>();

            auto error = policy->FromPublicApi(*healthPtr);
            
            if (!error.IsSuccess()) { return error; }

            internalDescription.healthPolicy_ = std::move(policy);
        }
        
        // Upgrade health policy is only available for fabric upgrades
        //
        __if_not_exists (TPublicDescription::ApplicationName)
        {
            if (enableDeltasFlagSet || upgradeHealthPolicyFlagSet || applicationHealthPoliciesFlagSet)
            {
                if (ex1Ptr->Reserved == NULL) { return ErrorCode::FromHResult(E_POINTER); }

                auto ex2Ptr = reinterpret_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX2*>(ex1Ptr->Reserved);

                if (applicationHealthPoliciesFlagSet)
                {
                    if (ex2Ptr->Reserved == NULL) { return ErrorCode::FromHResult(E_POINTER); }

                    auto ex3Ptr = static_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX3*>(ex2Ptr->Reserved);
                    
                    if (ex3Ptr->ApplicationHealthPolicyMap == NULL) { return ErrorCode::FromHResult(E_POINTER); }
                    
                    auto policies = make_shared<ApplicationHealthPolicyMap>();
                    auto error = policies->FromPublicApi(*ex3Ptr->ApplicationHealthPolicyMap);
                    if (!error.IsSuccess()) { return error; }

                    internalDescription.applicationHealthPolicies_ = move(policies);
                }

                if (enableDeltasFlagSet)
                {
                    internalDescription.enableDeltaHealthEvaluation_ = make_shared<bool>(ex2Ptr->EnableDeltaHealthEvaluation == TRUE);
                }

                if (upgradeHealthPolicyFlagSet)
                {
                    if (ex2Ptr->UpgradeHealthPolicy == NULL) { return ErrorCode::FromHResult(E_POINTER); }

                    auto upgradeHealthPtr = reinterpret_cast<FABRIC_CLUSTER_UPGRADE_HEALTH_POLICY*>(ex2Ptr->UpgradeHealthPolicy);

                    auto upgradeHealthPolicy = make_shared<ClusterUpgradeHealthPolicy>();

                    auto error = upgradeHealthPolicy->FromPublicApi(*upgradeHealthPtr);

                    if (!error.IsSuccess()) { return error; }

                    internalDescription.upgradeHealthPolicy_ = std::move(upgradeHealthPolicy);
                }
            }
        }
    }

    return ErrorCodeValue::Success;
}
