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

StringLiteral const TraceComponent("ApplicationUpgradeDescription");

ApplicationUpgradeDescription::ApplicationUpgradeDescription() 
    : applicationName_()
    , targetApplicationTypeVersion_()
    , applicationParameters_()
    , upgradeType_(UpgradeType::Rolling)
    , rollingUpgradeMode_(RollingUpgradeMode::UnmonitoredAuto)
    , replicaSetCheckTimeout_(TimeSpan::Zero)
    , monitoringPolicy_()
    , healthPolicy_()
    , isHealthPolicyValid_(false)
{ 
}

ApplicationUpgradeDescription::ApplicationUpgradeDescription(NamingUri const & appName)
    : applicationName_(appName)
    , targetApplicationTypeVersion_()
    , applicationParameters_()
    , upgradeType_(UpgradeType::Rolling)
    , rollingUpgradeMode_(RollingUpgradeMode::UnmonitoredAuto)
    , replicaSetCheckTimeout_(TimeSpan::Zero)
    , monitoringPolicy_()
    , healthPolicy_()
    , isHealthPolicyValid_(false)
{
}

ApplicationUpgradeDescription::ApplicationUpgradeDescription(
    NamingUri const & appName,
    std::wstring const & targetApplicationTypeVersion,
    std::map<std::wstring, std::wstring> const & applicationParameters,
    UpgradeType::Enum upgradeType,
    RollingUpgradeMode::Enum rollingUpgradeMode,
    TimeSpan replicaSetCheckTimeout,
    RollingUpgradeMonitoringPolicy const & monitoringPolicy,
    ApplicationHealthPolicy const & healthPolicy,
    bool isHealthPolicyValid)
    : applicationName_(appName)
    , targetApplicationTypeVersion_(targetApplicationTypeVersion)
    , applicationParameters_(applicationParameters)
    , upgradeType_(upgradeType)
    , rollingUpgradeMode_(rollingUpgradeMode)
    , replicaSetCheckTimeout_(replicaSetCheckTimeout)
    , monitoringPolicy_(monitoringPolicy)
    , healthPolicy_(healthPolicy)
    , isHealthPolicyValid_(isHealthPolicyValid)
{
}

ApplicationUpgradeDescription::ApplicationUpgradeDescription(ApplicationUpgradeDescription && other) 
    : applicationName_(move(other.applicationName_))
    , targetApplicationTypeVersion_(move(other.targetApplicationTypeVersion_))
    , applicationParameters_(move(other.applicationParameters_))
    , upgradeType_(move(other.upgradeType_))
    , rollingUpgradeMode_(move(other.rollingUpgradeMode_))
    , replicaSetCheckTimeout_(move(other.replicaSetCheckTimeout_))
    , monitoringPolicy_(move(other.monitoringPolicy_))
    , healthPolicy_(move(other.healthPolicy_))
    , isHealthPolicyValid_(move(other.IsHealthPolicyValid))
{ 
}

// The distinction between UnmonitoredManual and Monitored only exists at the CM. In both cases,
// the CM will send a message to the FM reporting the health of completed upgrade domains.
// Within the FM, there is no distinction between these two modes.
//
bool ApplicationUpgradeDescription::get_IsInternalMonitored() const
{
    return (this->IsUnmonitoredManual || this->IsHealthMonitored);
}

bool ApplicationUpgradeDescription::get_IsUnmonitoredManual() const
{
    return rollingUpgradeMode_ == RollingUpgradeMode::UnmonitoredManual;
}

bool ApplicationUpgradeDescription::get_IsHealthMonitored() const
{
    return rollingUpgradeMode_ == RollingUpgradeMode::Monitored;
}

Common::ErrorCode ApplicationUpgradeDescription::FromPublicApi(FABRIC_APPLICATION_UPGRADE_DESCRIPTION const & publicDescription)
{
    ApplicationUpgradeDescriptionWrapper wrapper;
    auto error = wrapper.FromPublicApi(publicDescription);

    if (!error.IsSuccess()) { return error; }

    return this->FromWrapper(wrapper);
}

void ApplicationUpgradeDescription::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_APPLICATION_UPGRADE_DESCRIPTION & publicDescription) const
{
    ApplicationUpgradeDescriptionWrapper wrapper;
    this->ToWrapper(wrapper);

    wrapper.ToPublicApi(heap, publicDescription);
}

bool ApplicationUpgradeDescription::TryValidate(__out wstring & validationErrorMessage) const
{
    return monitoringPolicy_.TryValidate(validationErrorMessage) && healthPolicy_.TryValidate(validationErrorMessage);
}

bool ApplicationUpgradeDescription::TryModify(
    ApplicationUpgradeUpdateDescription const & update,
    __out wstring & validationErrorMessage)
{
    return ModifyUpgradeHelper::TryModifyUpgrade(*this, update, validationErrorMessage);
}

Common::ErrorCode ApplicationUpgradeDescription::FromWrapper(ServiceModel::ApplicationUpgradeDescriptionWrapper const &descWrapper)
{
    if (!NamingUri::TryParse(descWrapper.ApplicationName, applicationName_))
    {
        Trace.WriteWarning(TraceComponent, "Cannot parse '{0}' as Naming Uri", wstring(descWrapper.ApplicationName));
        return ErrorCodeValue::InvalidNameUri;
    }
    targetApplicationTypeVersion_ = descWrapper.TargetApplicationTypeVersion;

    applicationParameters_ = descWrapper.Parameters;
    upgradeType_ = descWrapper.UpgradeKind;
    
    switch (upgradeType_)
    {
        case UpgradeType::Rolling:
        case UpgradeType::Rolling_NotificationOnly:
        case UpgradeType::Rolling_ForceRestart:
            replicaSetCheckTimeout_ = UpgradeHelper::GetReplicaSetCheckTimeoutForInitialUpgrade(descWrapper.ReplicaSetTimeoutInSec);
            rollingUpgradeMode_ = descWrapper.UpgradeMode;
            break;

        default:
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString("{0} {1}", GET_CM_RC( Invalid_Upgrade_Kind ), upgradeType_));
    }

    if (descWrapper.ForceRestart)
    {
        // overrides type
        upgradeType_ = UpgradeType::Rolling_ForceRestart;
    }

    monitoringPolicy_ = descWrapper.MonitoringPolicy;

    if (descWrapper.HealthPolicy)
    {
        healthPolicy_ = *descWrapper.HealthPolicy;
        isHealthPolicyValid_ = true;
    }
    else
    {
        isHealthPolicyValid_ = false;
    }

    return ErrorCodeValue::Success;
}

void ApplicationUpgradeDescription::ToWrapper(__out ApplicationUpgradeDescriptionWrapper &descWrapper) const
{
    descWrapper.ApplicationName = applicationName_.ToString();
    descWrapper.TargetApplicationTypeVersion = targetApplicationTypeVersion_;

    auto parameters = applicationParameters_;
    descWrapper.Parameters = move(parameters);

    switch (upgradeType_)
    {
        case UpgradeType::Rolling:
        case UpgradeType::Rolling_ForceRestart:
        case UpgradeType::Rolling_NotificationOnly:
            descWrapper.UpgradeKind = UpgradeType::Rolling;
            descWrapper.UpgradeMode = rollingUpgradeMode_;
            descWrapper.ReplicaSetTimeoutInSec = UpgradeHelper::ToPublicReplicaSetCheckTimeoutInSeconds(ReplicaSetCheckTimeout);
            descWrapper.ForceRestart = (upgradeType_ == UpgradeType::Rolling_ForceRestart ? true : false);
            break;

        default:
            descWrapper.UpgradeMode = RollingUpgradeMode::Invalid;
    }

    descWrapper.MonitoringPolicy = monitoringPolicy_;

    if (isHealthPolicyValid_)
    {
        descWrapper.HealthPolicy = make_shared<ApplicationHealthPolicy>(healthPolicy_);
    }
}

bool ApplicationUpgradeDescription::EqualsIgnoreHealthPolicies(ApplicationUpgradeDescription const & other) const
{
    return applicationName_ == other.applicationName_
        && targetApplicationTypeVersion_ == other.targetApplicationTypeVersion_
        && applicationParameters_ == other.applicationParameters_
        && upgradeType_ == other.upgradeType_
        && replicaSetCheckTimeout_ == other.replicaSetCheckTimeout_;
}

bool ApplicationUpgradeDescription::operator == (ApplicationUpgradeDescription const & other) const
{
    return this->EqualsIgnoreHealthPolicies(other)
        && rollingUpgradeMode_ == other.rollingUpgradeMode_
        && monitoringPolicy_ == other.monitoringPolicy_
        && healthPolicy_ == other.healthPolicy_
        && isHealthPolicyValid_ == other.isHealthPolicyValid_;
}

bool ApplicationUpgradeDescription::operator != (ApplicationUpgradeDescription const & other) const
{
    return !(*this == other);
}

void ApplicationUpgradeDescription::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    // Don't trace application parameters - SQL puts plaintext passwords in them (although they shouldn't)
    //
    w.Write("{0}, {1}, {2}, {3}, ReplicaSetCheckTimeout = {4}, Parameters = [removed]",
        applicationName_,
        targetApplicationTypeVersion_,
        upgradeType_,
        rollingUpgradeMode_,
        replicaSetCheckTimeout_);

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
    }
}
