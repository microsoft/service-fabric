// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::ClusterManager;

StringLiteral const TraceComponent("FabricUpgradeDescription");

FabricUpgradeDescription::FabricUpgradeDescription() 
    : version_()
    , upgradeType_(UpgradeType::Rolling)
    , rollingUpgradeMode_(RollingUpgradeMode::UnmonitoredAuto)
    , replicaSetCheckTimeout_(TimeSpan::Zero)
    , monitoringPolicy_()
    , healthPolicy_()
    , isHealthPolicyValid_(false)
    , upgradeHealthPolicy_()
    , isUpgradeHealthPolicyValid_(false)
    , applicationHealthPolicies_()
{ 
}

FabricUpgradeDescription::FabricUpgradeDescription(
    FabricVersion const & version,
    ServiceModel::UpgradeType::Enum upgradeType,
    RollingUpgradeMode::Enum rollingUpgradeMode,
    TimeSpan replicaSetCheckTimeout,
    RollingUpgradeMonitoringPolicy const & monitoringPolicy,
    ClusterHealthPolicy const & healthPolicy,
    bool isHealthPolicyValid,
    bool enableDeltaHealthEvaluation,
    ClusterUpgradeHealthPolicy const & upgradeHealthPolicy,
    bool isUpgradeHealthPolicyValid,
    ApplicationHealthPolicyMapSPtr const & applicationHealthPolicies)
    : version_(version)
    , upgradeType_(upgradeType)
    , rollingUpgradeMode_(rollingUpgradeMode)
    , replicaSetCheckTimeout_(replicaSetCheckTimeout)
    , monitoringPolicy_(monitoringPolicy)
    , healthPolicy_(healthPolicy)
    , isHealthPolicyValid_(isHealthPolicyValid)
    , enableDeltaHealthEvaluation_(enableDeltaHealthEvaluation)
    , upgradeHealthPolicy_(upgradeHealthPolicy)
    , isUpgradeHealthPolicyValid_(isUpgradeHealthPolicyValid)
    , applicationHealthPolicies_(applicationHealthPolicies)
{
}

FabricUpgradeDescription::FabricUpgradeDescription(FabricUpgradeDescription && other) 
    : version_(move(other.version_))
    , upgradeType_(move(other.upgradeType_))
    , rollingUpgradeMode_(move(other.rollingUpgradeMode_))
    , replicaSetCheckTimeout_(move(other.replicaSetCheckTimeout_))
    , monitoringPolicy_(move(other.monitoringPolicy_))
    , healthPolicy_(move(other.healthPolicy_))
    , isHealthPolicyValid_(move(other.isHealthPolicyValid_))
    , enableDeltaHealthEvaluation_(move(other.enableDeltaHealthEvaluation_))
    , upgradeHealthPolicy_(move(other.upgradeHealthPolicy_))
    , isUpgradeHealthPolicyValid_(move(other.isUpgradeHealthPolicyValid_))
    , applicationHealthPolicies_(move(other.applicationHealthPolicies_))
{ 
}

// The distinction between UnmonitoredManual and Monitored only exists at the CM. In both cases,
// the CM will send a message to the FM reporting the health of completed upgrade domains.
// Within the FM, there is no distinction between these two modes.
//
bool FabricUpgradeDescription::get_IsInternalMonitored() const
{
    return (this->IsUnmonitoredManual || this->IsHealthMonitored);
}

bool FabricUpgradeDescription::get_IsUnmonitoredManual() const
{
    return rollingUpgradeMode_ == RollingUpgradeMode::UnmonitoredManual;
}

bool FabricUpgradeDescription::get_IsHealthMonitored() const
{
    return rollingUpgradeMode_ == RollingUpgradeMode::Monitored;
}

ErrorCode FabricUpgradeDescription::FromWrapper(FabricUpgradeDescriptionWrapper const& wrapper)
{
    if (wrapper.CodeVersion.empty() && wrapper.ConfigVersion.empty())
    {
        return TraceAndGetError(
            ErrorCodeValue::InvalidArgument,
            wformatString("{0} ('{1}','{2}')", GET_CM_RC( Invalid_Fabric_Upgrade ), 
                wrapper.CodeVersion,
                wrapper.ConfigVersion));
    }

    FabricCodeVersion codeVersion;
    if (!wrapper.CodeVersion.empty())
    {
        if (wrapper.CodeVersion.length() > ParameterValidator::MaxStringSize) 
        { 
            return TraceAndGetError(
                ErrorCodeValue::InvalidArgument,
                wformatString("{0} ({1},{2},{3})'", GET_COM_RC( String_Too_Long ), 
                    L"CodeVersion",
                    wrapper.CodeVersion.length(),
                    ParameterValidator::MaxStringSize));
        }
        
        if (!FabricCodeVersion::TryParse(wrapper.CodeVersion, codeVersion))
        {
            return TraceAndGetError(
                ErrorCodeValue::InvalidArgument,
                wformatString("{0} {1}'", GET_CM_RC( Invalid_Fabric_Code_Version ), 
                    wrapper.CodeVersion));
        }
    }

    FabricConfigVersion configVersion;
    if (!wrapper.ConfigVersion.empty())
    {
        if (wrapper.ConfigVersion.length() > ParameterValidator::MaxStringSize) 
        { 
            return TraceAndGetError(
                ErrorCodeValue::InvalidArgument,
                wformatString("{0} ({1},{2},{3})'", GET_COM_RC( String_Too_Long ), 
                    L"ConfigVersion",
                    wrapper.ConfigVersion.length(),
                    ParameterValidator::MaxStringSize));
        }

        if (!FabricConfigVersion::TryParse(wrapper.ConfigVersion, configVersion))
        {
            return TraceAndGetError(
                ErrorCodeValue::InvalidArgument,
                wformatString("{0} {1}'", GET_CM_RC( Invalid_Fabric_Config_Version ), 
                    wrapper.ConfigVersion));
        }
    }

    version_ = FabricVersion(codeVersion, configVersion);

    if (wrapper.UpgradeKind != FABRIC_UPGRADE_KIND_ROLLING)
    {
        return TraceAndGetError(
            ErrorCodeValue::InvalidArgument,
            wformatString("{0} {1}'", GET_CM_RC( Invalid_Upgrade_Kind ), 
                static_cast<int>(wrapper.UpgradeKind)));
    }
    
    upgradeType_ = UpgradeType::Rolling;
    if (wrapper.ForceRestart == true)
    {
        upgradeType_ = UpgradeType::Rolling_ForceRestart;
    }

    switch (wrapper.UpgradeMode)
    {
        case FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_AUTO: 
            rollingUpgradeMode_ = RollingUpgradeMode::UnmonitoredAuto;
            break;
        case FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_MANUAL:
            rollingUpgradeMode_ = RollingUpgradeMode::UnmonitoredManual;
            break;
        case FABRIC_ROLLING_UPGRADE_MODE_MONITORED:
            rollingUpgradeMode_ = RollingUpgradeMode::Monitored;

            monitoringPolicy_ = wrapper.MonitoringPolicy;

            if (wrapper.HealthPolicy)
            {
                healthPolicy_ = *wrapper.HealthPolicy;
                isHealthPolicyValid_ = true;
            }
            else
            {
                isHealthPolicyValid_ = false;
            }

            if (wrapper.UpgradeHealthPolicy)
            {
                upgradeHealthPolicy_ = *wrapper.UpgradeHealthPolicy;
                isUpgradeHealthPolicyValid_ = true;
            }
            else
            {
                isUpgradeHealthPolicyValid_ = false;
            }

            applicationHealthPolicies_ = wrapper.ApplicationHealthPolicies;

            break;

        default:
            return TraceAndGetError(
                ErrorCodeValue::InvalidArgument,
                wformatString("{0} {1}'", GET_CM_RC( Invalid_Upgrade_Mode2 ), 
                    static_cast<int>(wrapper.UpgradeMode)));
    }

    enableDeltaHealthEvaluation_ = wrapper.EnableDeltaHealthEvaluation;

    replicaSetCheckTimeout_ = UpgradeHelper::GetReplicaSetCheckTimeoutForInitialUpgrade(
        wrapper.ReplicaSetTimeoutInSec);

    return ErrorCodeValue::Success;
}

void FabricUpgradeDescription::ToWrapper(__out ServiceModel::FabricUpgradeDescriptionWrapper & wrapper) const
{
    wrapper.CodeVersion = version_.CodeVersion.ToString();
    wrapper.ConfigVersion = version_.ConfigVersion.ToString();

    switch (upgradeType_)
    {
        case UpgradeType::Rolling_ForceRestart:
            wrapper.ForceRestart = true;
            // intentional fall-through

        case UpgradeType::Rolling:
        case UpgradeType::Rolling_NotificationOnly:
            wrapper.UpgradeKind = FABRIC_UPGRADE_KIND_ROLLING;
            break;

        default:
            wrapper.UpgradeKind = FABRIC_UPGRADE_KIND_INVALID;
    }

    switch (rollingUpgradeMode_)
    {
        case RollingUpgradeMode::UnmonitoredAuto:
            wrapper.UpgradeMode = FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_AUTO;
            break;
        case RollingUpgradeMode::UnmonitoredManual:
            wrapper.UpgradeMode = FABRIC_ROLLING_UPGRADE_MODE_UNMONITORED_MANUAL;
            break;
        case RollingUpgradeMode::Monitored:
            wrapper.UpgradeMode = FABRIC_ROLLING_UPGRADE_MODE_MONITORED;

            wrapper.MonitoringPolicy = monitoringPolicy_;

            if (isHealthPolicyValid_)
            {
                wrapper.HealthPolicy = make_shared<ClusterHealthPolicy>(healthPolicy_);
            }

            if (isUpgradeHealthPolicyValid_)
            {
                wrapper.UpgradeHealthPolicy = make_shared<ClusterUpgradeHealthPolicy>(upgradeHealthPolicy_);
            }

            wrapper.ApplicationHealthPolicies = applicationHealthPolicies_;

            break;

        default:
            wrapper.UpgradeMode = FABRIC_ROLLING_UPGRADE_MODE_INVALID;
    }

    wrapper.EnableDeltaHealthEvaluation = enableDeltaHealthEvaluation_;

    wrapper.ReplicaSetTimeoutInSec = UpgradeHelper::ToPublicReplicaSetCheckTimeoutInSeconds(ReplicaSetCheckTimeout);
}

ErrorCode FabricUpgradeDescription::FromPublicApi(FABRIC_UPGRADE_DESCRIPTION const & publicDescription)
{
    FabricUpgradeDescriptionWrapper wrapper;
    auto error = wrapper.FromPublicApi(publicDescription);
    if (!error.IsSuccess()) { return error; }

    return this->FromWrapper(wrapper);
}

void FabricUpgradeDescription::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_UPGRADE_DESCRIPTION & publicDescription) const
{
    FabricUpgradeDescriptionWrapper wrapper;
    this->ToWrapper(wrapper);

    wrapper.ToPublicApi(heap, publicDescription);
}

bool FabricUpgradeDescription::TryValidate(__out wstring & validationErrorMessage) const
{
    if (applicationHealthPolicies_ && !applicationHealthPolicies_->TryValidate(validationErrorMessage))
    {
        return false;
    }

    return monitoringPolicy_.TryValidate(validationErrorMessage) && 
           healthPolicy_.TryValidate(validationErrorMessage) &&
           upgradeHealthPolicy_.TryValidate(validationErrorMessage);
}

bool FabricUpgradeDescription::TryModify(
    FabricUpgradeUpdateDescription const & update,
    __out wstring & validationErrorMessage)
{
    return ModifyUpgradeHelper::TryModifyUpgrade(*this, update, validationErrorMessage);
}

bool FabricUpgradeDescription::EqualsIgnoreHealthPolicies(FabricUpgradeDescription const & other) const
{
    return (version_ == other.version_)
        && (upgradeType_ == other.upgradeType_)
        && (replicaSetCheckTimeout_ == other.replicaSetCheckTimeout_);
}

bool FabricUpgradeDescription::operator == (FabricUpgradeDescription const & other) const
{
    CHECK_EQUALS_IF_NON_NULL( applicationHealthPolicies_ )

    return this->EqualsIgnoreHealthPolicies(other)
        && (rollingUpgradeMode_ == other.rollingUpgradeMode_)
        && (monitoringPolicy_ == other.monitoringPolicy_)
        && (healthPolicy_ == other.healthPolicy_)
        && (enableDeltaHealthEvaluation_ == other.enableDeltaHealthEvaluation_)
        && (upgradeHealthPolicy_ == other.upgradeHealthPolicy_);
}

bool FabricUpgradeDescription::operator != (FabricUpgradeDescription const & other) const
{
    return !(*this == other);
}

void FabricUpgradeDescription::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write("{0}, {1}, {2}, ReplicaSetCheckTimeout = {3}, EnableDelta = {4}",
        version_,
        upgradeType_,
        rollingUpgradeMode_,
        replicaSetCheckTimeout_,
        enableDeltaHealthEvaluation_);

    if (this->IsHealthMonitored)
    {
        w.Write(", Monitoring = {0}", monitoringPolicy_);

        if (isHealthPolicyValid_)
        {
            w.Write(", Health = {0}", healthPolicy_);
        }
        else
        {
            w.Write(", InvalidHealth");
        }

        if (isUpgradeHealthPolicyValid_)
        {
            w.Write(", UpgradeHealth = {0}", upgradeHealthPolicy_);
        }
        else
        {
            w.Write(", InvalidUpgradeHealth");
        }

        if (applicationHealthPolicies_ && applicationHealthPolicies_->size() > 0)
        {
            w.Write(", {0}", applicationHealthPolicies_->ToString());
        }
    }
}

ErrorCode FabricUpgradeDescription::TraceAndGetError(
    ErrorCodeValue::Enum errorValue,
    wstring && message)
{
    Trace.WriteWarning(TraceComponent, "{0}", message);
    return ErrorCode(errorValue, move(message));
}
