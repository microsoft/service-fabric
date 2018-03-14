// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceComponent("RollingUpgradeUpdateDescription");

RollingUpgradeUpdateDescription::RollingUpgradeUpdateDescription() 
    : rollingUpgradeMode_()
    , forceRestart_()
    , replicaSetCheckTimeout_()
    , failureAction_()
    , healthCheckWait_()
    , healthCheckRetry_()
    , healthCheckStable_()
    , upgradeTimeout_()
    , upgradeDomainTimeout_()
{ 
}

RollingUpgradeUpdateDescription::RollingUpgradeUpdateDescription(RollingUpgradeUpdateDescription && other) 
    : rollingUpgradeMode_(move(other.rollingUpgradeMode_))
    , forceRestart_(move(other.forceRestart_))
    , replicaSetCheckTimeout_(move(other.replicaSetCheckTimeout_))
    , failureAction_(move(other.failureAction_))
    , healthCheckWait_(move(other.healthCheckWait_))
    , healthCheckRetry_(move(other.healthCheckRetry_))
    , healthCheckStable_(move(other.healthCheckStable_))
    , upgradeTimeout_(move(other.upgradeTimeout_))
    , upgradeDomainTimeout_(move(other.upgradeDomainTimeout_))
{ 
}

bool RollingUpgradeUpdateDescription::operator == (RollingUpgradeUpdateDescription const & other) const
{

    CHECK_EQUALS_IF_NON_NULL( rollingUpgradeMode_ )
    CHECK_EQUALS_IF_NON_NULL( forceRestart_ )
    CHECK_EQUALS_IF_NON_NULL( replicaSetCheckTimeout_ )
    CHECK_EQUALS_IF_NON_NULL( failureAction_ )
    CHECK_EQUALS_IF_NON_NULL( healthCheckWait_ )
    CHECK_EQUALS_IF_NON_NULL( healthCheckRetry_ )
    CHECK_EQUALS_IF_NON_NULL( healthCheckStable_ )
    CHECK_EQUALS_IF_NON_NULL( upgradeTimeout_ )
    CHECK_EQUALS_IF_NON_NULL( upgradeDomainTimeout_ )

    return true;
}

bool RollingUpgradeUpdateDescription::operator != (RollingUpgradeUpdateDescription const & other) const
{
    return !(*this == other);
}

void RollingUpgradeUpdateDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w << "[ ";

    if (rollingUpgradeMode_) { w << "rollingUpgradeMode=" << *rollingUpgradeMode_ << " "; }
    if (forceRestart_) { w << "forceRestart=" << *forceRestart_ << " "; }
    if (replicaSetCheckTimeout_) { w << "replicaSetCheckTimeout=" << *replicaSetCheckTimeout_ << " "; }
    if (failureAction_) { w << "failureAction=" << *failureAction_ << " "; }
    if (healthCheckWait_) { w << "healthCheckWait=" << *healthCheckWait_ << " "; }
    if (healthCheckRetry_) { w << "healthCheckRetry=" << *healthCheckRetry_ << " "; }
    if (healthCheckStable_) { w << "healthCheckStable=" << *healthCheckStable_ << " "; }
    if (upgradeTimeout_) { w << "upgradeTimeout=" << *upgradeTimeout_ << " "; }
    if (upgradeDomainTimeout_) { w << "upgradeDomainTimeout=" << *upgradeDomainTimeout_ << " "; }

    w << "]";
}

bool RollingUpgradeUpdateDescription::HasRollingUpgradeUpdateDescriptionFlags(DWORD flags)
{
    return 
        (flags & FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_MODE) ||
        (flags & FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_FORCE_RESTART) ||
        (flags & FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_REPLICA_SET_CHECK_TIMEOUT) ||
        (flags & FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_FAILURE_ACTION) ||
        (flags & FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_CHECK_WAIT) ||
        (flags & FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_CHECK_RETRY) ||
        (flags & FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_TIMEOUT) ||
        (flags & FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_DOMAIN_TIMEOUT) ||
        (flags & FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_CHECK_STABLE);            
}

bool RollingUpgradeUpdateDescription::IsValidForRollback()
{
    if (this->RollingUpgradeMode)
    {
        switch (*this->RollingUpgradeMode)
        {
            case RollingUpgradeMode::UnmonitoredAuto:
            case RollingUpgradeMode::UnmonitoredManual:
                break;

            default:
                return false;
        }
    }

    if (this->FailureAction) { return false; }
    if (this->HealthCheckWaitDuration) { return false; }
    if (this->HealthCheckStableDuration) { return false; }
    if (this->HealthCheckRetryTimeout) { return false; }
    if (this->UpgradeDomainTimeout) { return false; }
    if (this->UpgradeTimeout) { return false; }

    return true;
}

Common::ErrorCode RollingUpgradeUpdateDescription::FromPublicApi(
    FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS const & flags,
    FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION const & publicDescription)
{
    auto dwordFlags = static_cast<DWORD>(flags);

    if (dwordFlags & FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_MODE)        
    {
        auto mode = RollingUpgradeMode::FromPublicApi(publicDescription.RollingUpgradeMode);
        if (mode == RollingUpgradeMode::Invalid) 
        { 
            return ErrorCode(
                ErrorCodeValue::InvalidArgument, 
                wformatString("{0} {1}", GET_CM_RC( Invalid_Upgrade_Mode2 ), static_cast<int>(publicDescription.RollingUpgradeMode)));
        }

        rollingUpgradeMode_ = make_shared<RollingUpgradeMode::Enum>(mode);

        dwordFlags &= ~FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_MODE;
    }        

    if (dwordFlags & FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_FORCE_RESTART)        
    {
        forceRestart_ = make_shared<bool>(publicDescription.ForceRestart == TRUE);

        dwordFlags &= ~FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_FORCE_RESTART;
    }        

    if (dwordFlags & FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_REPLICA_SET_CHECK_TIMEOUT)        
    {
        replicaSetCheckTimeout_ = make_shared<TimeSpan>(
            TimeSpan::FromSeconds(publicDescription.UpgradeReplicaSetCheckTimeoutInSeconds));

        dwordFlags &= ~FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_REPLICA_SET_CHECK_TIMEOUT;
    }

    // The first few flags were cleared to decide whether or not the _EX structure 
    // (Reserved pointer) is expected
    //
    if (dwordFlags == FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_NONE)
    {
        return ErrorCodeValue::Success;
    }
    else if (publicDescription.Reserved == NULL)
    {
        return ErrorCode(ErrorCodeValue::ArgumentNull, wformatString("{0} {1}", GET_CM_RC( Invalid_Null_Pointer ), "Reserved"));
    }

    auto ex1Ptr = reinterpret_cast<FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION_EX1*>(publicDescription.Reserved);
    auto monitoringPtr = ex1Ptr->MonitoringPolicy;

    if (monitoringPtr == NULL)
    {
        return ErrorCode(ErrorCodeValue::ArgumentNull, wformatString("{0} {1}", GET_CM_RC( Invalid_Null_Pointer ), "MonitoringPolicy"));
    }

    if (dwordFlags & FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_FAILURE_ACTION)
    {
        failureAction_ = make_shared<MonitoredUpgradeFailureAction::Enum>(
            MonitoredUpgradeFailureAction::FromPublicApi(monitoringPtr->FailureAction));
    }        

    if (dwordFlags & FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_CHECK_WAIT)        
    {
        healthCheckWait_ = make_shared<TimeSpan>(
            TimeSpan::FromSeconds(monitoringPtr->HealthCheckWaitDurationInSeconds));
    }        

    if (dwordFlags & FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_CHECK_RETRY)        
    {
        healthCheckRetry_ = make_shared<TimeSpan>(
            TimeSpan::FromSeconds(monitoringPtr->HealthCheckRetryTimeoutInSeconds));
    }        

    if (dwordFlags & FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_TIMEOUT)        
    {
        upgradeTimeout_ = make_shared<TimeSpan>(
            TimeSpan::FromSeconds(monitoringPtr->UpgradeTimeoutInSeconds));
    }        

    if (dwordFlags & FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_UPGRADE_DOMAIN_TIMEOUT)        
    {
        upgradeDomainTimeout_ = make_shared<TimeSpan>(
            TimeSpan::FromSeconds(monitoringPtr->UpgradeDomainTimeoutInSeconds));
    }        

    if (dwordFlags & FABRIC_ROLLING_UPGRADE_UPDATE_FLAGS_HEALTH_CHECK_STABLE)        
    {
        if (monitoringPtr->Reserved != NULL)
        {
            auto monitoringEx1Ptr = reinterpret_cast<FABRIC_ROLLING_UPGRADE_MONITORING_POLICY_EX1*>(monitoringPtr->Reserved);

            healthCheckStable_ = make_shared<TimeSpan>(
                TimeSpan::FromSeconds(monitoringEx1Ptr->HealthCheckStableDurationInSeconds));
        }
    }        

    return ErrorCodeValue::Success;
}
