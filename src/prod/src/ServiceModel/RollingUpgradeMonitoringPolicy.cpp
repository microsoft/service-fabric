// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

Global<RollingUpgradeMonitoringPolicy> RollingUpgradeMonitoringPolicy::Default = make_global<RollingUpgradeMonitoringPolicy>();

RollingUpgradeMonitoringPolicy::RollingUpgradeMonitoringPolicy()
    : failureAction_(MonitoredUpgradeFailureAction::Manual)
    , healthCheckWaitDuration_(TimeSpan::Zero)
    , healthCheckStableDuration_(TimeSpan::FromMinutes(2))
    , healthCheckRetryTimeout_(TimeSpan::FromSeconds(600))
    , upgradeTimeout_(TimeSpan::MaxValue)
    , upgradeDomainTimeout_(TimeSpan::MaxValue)
{
}

RollingUpgradeMonitoringPolicy::RollingUpgradeMonitoringPolicy(
    MonitoredUpgradeFailureAction::Enum failureAction,
    Common::TimeSpan const healthCheckWaitDuration,
    Common::TimeSpan const healthCheckStableDuration,
    Common::TimeSpan const healthCheckRetryTimeout,
    Common::TimeSpan const upgradeTimeout,
    Common::TimeSpan const upgradeDomainTimeout)
    : failureAction_(failureAction)
    , healthCheckWaitDuration_(healthCheckWaitDuration)
    , healthCheckStableDuration_(healthCheckStableDuration)
    , healthCheckRetryTimeout_(healthCheckRetryTimeout)
    , upgradeTimeout_(upgradeTimeout)
    , upgradeDomainTimeout_(upgradeDomainTimeout)
{
}

RollingUpgradeMonitoringPolicy::~RollingUpgradeMonitoringPolicy()
{
}

ErrorCode RollingUpgradeMonitoringPolicy::FromPublicApi(FABRIC_ROLLING_UPGRADE_MONITORING_POLICY const & description)
{
    failureAction_ = MonitoredUpgradeFailureAction::FromPublicApi(description.FailureAction);
    healthCheckWaitDuration_ = FromPublicTimeInSeconds(description.HealthCheckWaitDurationInSeconds);
    healthCheckRetryTimeout_ = FromPublicTimeInSeconds(description.HealthCheckRetryTimeoutInSeconds);
    upgradeTimeout_ = FromPublicTimeInSeconds(description.UpgradeTimeoutInSeconds);
    upgradeDomainTimeout_ = FromPublicTimeInSeconds(description.UpgradeDomainTimeoutInSeconds);

    if (description.Reserved != NULL)
    {
        auto ex1Ptr = static_cast<FABRIC_ROLLING_UPGRADE_MONITORING_POLICY_EX1*>(description.Reserved);

        healthCheckStableDuration_ = FromPublicTimeInSeconds(ex1Ptr->HealthCheckStableDurationInSeconds);
    }

    return ErrorCodeValue::Success;
}

void RollingUpgradeMonitoringPolicy::ToPublicApi(
    __in ScopedHeap & heap, 
    __out FABRIC_ROLLING_UPGRADE_MONITORING_POLICY & description) const
{
    UNREFERENCED_PARAMETER(heap);

    description.FailureAction = MonitoredUpgradeFailureAction::ToPublicApi(failureAction_);
    description.HealthCheckWaitDurationInSeconds = ToPublicTimeInSeconds(healthCheckWaitDuration_);
    description.HealthCheckRetryTimeoutInSeconds = ToPublicTimeInSeconds(healthCheckRetryTimeout_);
    description.UpgradeTimeoutInSeconds = ToPublicTimeInSeconds(upgradeTimeout_);
    description.UpgradeDomainTimeoutInSeconds = ToPublicTimeInSeconds(upgradeDomainTimeout_);

    auto ex1Ptr = heap.AddItem<FABRIC_ROLLING_UPGRADE_MONITORING_POLICY_EX1>().GetRawPointer();

    ex1Ptr->HealthCheckStableDurationInSeconds = ToPublicTimeInSeconds(healthCheckStableDuration_);

    description.Reserved = ex1Ptr;
}

void RollingUpgradeMonitoringPolicy::Modify(RollingUpgradeUpdateDescription const & updateDescription)
{
    if (updateDescription.FailureAction)
    {
        failureAction_ = *updateDescription.FailureAction;
    }

    if (updateDescription.HealthCheckWaitDuration)
    {
        healthCheckWaitDuration_ = *updateDescription.HealthCheckWaitDuration;
    }

    if (updateDescription.HealthCheckStableDuration)
    {
        healthCheckStableDuration_ = *updateDescription.HealthCheckStableDuration;
    }

    if (updateDescription.HealthCheckRetryTimeout)
    {
        healthCheckRetryTimeout_ = *updateDescription.HealthCheckRetryTimeout;
    }

    if (updateDescription.UpgradeTimeout)
    {
        upgradeTimeout_ = *updateDescription.UpgradeTimeout;
    }

    if (updateDescription.UpgradeDomainTimeout)
    {
        upgradeDomainTimeout_ = *updateDescription.UpgradeDomainTimeout;
    }
}

bool RollingUpgradeMonitoringPolicy::TryValidate(__out std::wstring & validationErrorMessage) const
{
    // Check here instead of FromPublicApi() in order to get an error message in the traces
    if (failureAction_ == MonitoredUpgradeFailureAction::Invalid)
    {
        validationErrorMessage = GET_CM_RC( Invalid_Upgrade_Failure_Action );
        return false;
    }

    if (upgradeTimeout_ <= healthCheckWaitDuration_.AddWithMaxAndMinValueCheck(healthCheckRetryTimeout_))
    {
        validationErrorMessage = wformatString("{0} ({1}, {2}, {3})", GET_CM_RC( Invalid_Overall_Timeout ),
            upgradeTimeout_,
            healthCheckWaitDuration_,
            healthCheckRetryTimeout_);
        return false;
    }

    if (upgradeDomainTimeout_ <= healthCheckWaitDuration_.AddWithMaxAndMinValueCheck(healthCheckRetryTimeout_))
    {
        validationErrorMessage = wformatString("{0} ({1}, {2}, {3})", GET_CM_RC( Invalid_Domain_Timeout ),
            upgradeDomainTimeout_,
            healthCheckWaitDuration_,
            healthCheckRetryTimeout_);
        return false;
    }

    if (upgradeTimeout_ <= healthCheckWaitDuration_.AddWithMaxAndMinValueCheck(healthCheckStableDuration_))
    {
        validationErrorMessage = wformatString("{0} ({1}, {2}, {3})", GET_CM_RC( Invalid_Overall_Timeout3 ),
            upgradeTimeout_,
            healthCheckWaitDuration_,
            healthCheckStableDuration_);
        return false;
    }

    if (upgradeDomainTimeout_ <= healthCheckWaitDuration_.AddWithMaxAndMinValueCheck(healthCheckStableDuration_))
    {
        validationErrorMessage = wformatString("{0} ({1}, {2}, {3})", GET_CM_RC( Invalid_Domain_Timeout2 ),
            upgradeDomainTimeout_,
            healthCheckWaitDuration_,
            healthCheckStableDuration_);
        return false;
    }

    if (upgradeTimeout_ < upgradeDomainTimeout_)
    {
        validationErrorMessage = wformatString("{0} ({1}, {2})", GET_CM_RC( Invalid_Overall_Timeout2 ),
            upgradeTimeout_,
            upgradeDomainTimeout_);
        return false;
    }

    return true;
}

bool RollingUpgradeMonitoringPolicy::operator == (RollingUpgradeMonitoringPolicy const & other) const
{
    return failureAction_ == other.failureAction_ &&
        healthCheckWaitDuration_ == other.healthCheckWaitDuration_ &&
        healthCheckStableDuration_ == other.healthCheckStableDuration_ &&
        healthCheckRetryTimeout_ == other.healthCheckRetryTimeout_ &&
        upgradeTimeout_ == other.upgradeTimeout_ &&
        upgradeDomainTimeout_ == other.upgradeDomainTimeout_;
}

bool RollingUpgradeMonitoringPolicy::operator != (RollingUpgradeMonitoringPolicy const & other) const
{
    return !(*this == other);
}

void RollingUpgradeMonitoringPolicy::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write(
        "action:{0}, wait:{1}, stable:{2}, retry:{3}, ud:{4}, overall:{5}", 
        failureAction_, 
        healthCheckWaitDuration_, 
        healthCheckStableDuration_, 
        healthCheckRetryTimeout_, 
        upgradeDomainTimeout_, 
        upgradeTimeout_);
}

DWORD RollingUpgradeMonitoringPolicy::ToPublicTimeInSeconds(TimeSpan const & time)
{
    if (time == TimeSpan::MaxValue)
    {
        return FABRIC_INFINITE_DURATION;
    }

    return static_cast<DWORD>(time.TotalSeconds());
}

TimeSpan RollingUpgradeMonitoringPolicy::FromPublicTimeInSeconds(DWORD time)
{
    if (time == FABRIC_INFINITE_DURATION)
    {
        return TimeSpan::MaxValue;
    }

    return TimeSpan::FromSeconds(time);
}
