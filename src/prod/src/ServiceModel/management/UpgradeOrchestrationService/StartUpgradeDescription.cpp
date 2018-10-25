// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::UpgradeOrchestrationService;

StringLiteral const TraceComponent("StartUpgradeDescription");
static const size_t MaxJsonConfigSize = 4 * 1024 * 1024;

StartUpgradeDescription::StartUpgradeDescription()
    : clusterConfig_()
    , healthCheckRetryTimeout_(TimeSpan::FromSeconds(600))
    , healthCheckWaitDuration_(TimeSpan::Zero)
    , healthCheckStableDuration_(TimeSpan::Zero)
    , upgradeDomainTimeout_(TimeSpan::MaxValue)
    , upgradeTimeout_(TimeSpan::MaxValue)
    , maxPercentUnhealthyApplications_(0)
    , maxPercentUnhealthyNodes_(0)
    , maxPercentDeltaUnhealthyNodes_(0)
    , maxPercentUpgradeDomainDeltaUnhealthyNodes_(0)
    , applicationHealthPolicies_()
{
}
                         
StartUpgradeDescription::StartUpgradeDescription(
    std::wstring  &clusterConfig,
    Common::TimeSpan const &healthCheckRetryTimeout,
    Common::TimeSpan const &healthCheckWaitDuration,
    Common::TimeSpan const &healthCheckStableDuration,
    Common::TimeSpan const &upgradeDomainTimeout,
    Common::TimeSpan const &upgradeTimeout,
    BYTE &maxPercentUnhealthyApplications,
    BYTE &maxPercentUnhealthyNodes,
    BYTE &maxPercentDeltaUnhealthyNodes,
    BYTE &maxPercentUpgradeDomainDeltaUnhealthyNodes,
    ApplicationHealthPolicyMap && applicationHealthPolicies)
    : clusterConfig_(clusterConfig)
    , healthCheckRetryTimeout_(healthCheckRetryTimeout)
    , healthCheckWaitDuration_(healthCheckWaitDuration)
    , healthCheckStableDuration_(healthCheckStableDuration)
    , upgradeDomainTimeout_(upgradeDomainTimeout)
    , upgradeTimeout_(upgradeTimeout)
    , maxPercentUnhealthyApplications_(maxPercentUnhealthyApplications)
    , maxPercentUnhealthyNodes_(maxPercentUnhealthyNodes)
    , maxPercentDeltaUnhealthyNodes_(maxPercentDeltaUnhealthyNodes)
    , maxPercentUpgradeDomainDeltaUnhealthyNodes_(maxPercentUpgradeDomainDeltaUnhealthyNodes)
    , applicationHealthPolicies_(move(applicationHealthPolicies))
{
}

StartUpgradeDescription::StartUpgradeDescription(StartUpgradeDescription const & other)
    : clusterConfig_(other.clusterConfig_)
    , healthCheckRetryTimeout_(other.healthCheckRetryTimeout_)
    , healthCheckWaitDuration_(other.healthCheckWaitDuration_)
    , healthCheckStableDuration_(other.healthCheckStableDuration_)
    , upgradeDomainTimeout_(other.upgradeDomainTimeout_)
    , upgradeTimeout_(other.upgradeTimeout_)
    , maxPercentUnhealthyApplications_(other.maxPercentUnhealthyApplications_)
    , maxPercentUnhealthyNodes_(other.maxPercentUnhealthyNodes_)
    , maxPercentDeltaUnhealthyNodes_(other.maxPercentDeltaUnhealthyNodes_)
    , maxPercentUpgradeDomainDeltaUnhealthyNodes_(other.maxPercentUpgradeDomainDeltaUnhealthyNodes_)
    , applicationHealthPolicies_(other.applicationHealthPolicies_)
{
}

StartUpgradeDescription::StartUpgradeDescription(StartUpgradeDescription && other)
    : clusterConfig_(move(other.clusterConfig_))
    , healthCheckRetryTimeout_(move(other.healthCheckRetryTimeout_))
    , healthCheckWaitDuration_(move(other.healthCheckWaitDuration_))
    , healthCheckStableDuration_(move(other.healthCheckStableDuration_))
    , upgradeDomainTimeout_(move(other.upgradeDomainTimeout_))
    , upgradeTimeout_(move(other.upgradeTimeout_))
    , maxPercentUnhealthyApplications_(move(other.maxPercentUnhealthyApplications_))
    , maxPercentUnhealthyNodes_(move(other.maxPercentUnhealthyNodes_))
    , maxPercentDeltaUnhealthyNodes_(move(other.maxPercentDeltaUnhealthyNodes_))
    , maxPercentUpgradeDomainDeltaUnhealthyNodes_(move(other.maxPercentUpgradeDomainDeltaUnhealthyNodes_))
    , applicationHealthPolicies_(move(other.applicationHealthPolicies_))
{
}

bool StartUpgradeDescription::operator == (StartUpgradeDescription const & other) const
{
    return clusterConfig_ == other.clusterConfig_ &&
        healthCheckRetryTimeout_ == other.healthCheckRetryTimeout_ &&
        healthCheckWaitDuration_ == other.healthCheckWaitDuration_ &&
        healthCheckStableDuration_ == other.healthCheckStableDuration_ &&
        upgradeDomainTimeout_ == other.upgradeDomainTimeout_ &&
        upgradeTimeout_ == other.upgradeTimeout_ &&
        maxPercentUnhealthyApplications_ == other.maxPercentUnhealthyApplications_ &&
        maxPercentUnhealthyNodes_ == other.maxPercentUnhealthyNodes_ &&
        maxPercentDeltaUnhealthyNodes_ == other.maxPercentDeltaUnhealthyNodes_ &&
        maxPercentUpgradeDomainDeltaUnhealthyNodes_ == other.maxPercentUpgradeDomainDeltaUnhealthyNodes_ &&
        applicationHealthPolicies_ == other.applicationHealthPolicies_;
}

bool StartUpgradeDescription::operator != (StartUpgradeDescription const & other) const
{
    return !(*this == other);
}

void StartUpgradeDescription::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ClusterConfig: {0}, healthCheckRetryTimeout: {1}, healthCheckWaitDuration: {2}, healthCheckStableDuration: {3}, upgradeDomainTimeout: {4}, upgradeTimeout: {5}, maxPercentUnhealthyApplications: {6}, maxPercentUnhealthyNodes: {7}, maxPercentDeltaUnhealthyNodes: {8}, maxPercentUpgradeDomainDeltaUnhealthyNodes: {9}",
        clusterConfig_,
        healthCheckRetryTimeout_,
        healthCheckWaitDuration_,
        healthCheckStableDuration_,
        upgradeDomainTimeout_,
        upgradeTimeout_,
        maxPercentUnhealthyApplications_,
        maxPercentUnhealthyNodes_,
        maxPercentDeltaUnhealthyNodes_,
        maxPercentUpgradeDomainDeltaUnhealthyNodes_);

    if (!applicationHealthPolicies_.empty())
    {
        w.Write(", {0}", applicationHealthPolicies_.ToString());
    }
}

Common::ErrorCode StartUpgradeDescription::FromPublicApi(
    FABRIC_START_UPGRADE_DESCRIPTION const & publicDescription)
{
    HRESULT hr = StringUtility::LpcwstrToWstring2(publicDescription.ClusterConfig, false, ParameterValidator::MinStringSize, MaxJsonConfigSize, clusterConfig_).ToHResult();
    if (FAILED(hr))
    {
        Trace.WriteError(TraceComponent, "StartUpgradeDescription::FromPublicApi/Failed to parse ClusterConfig: {0}", hr);
        return ErrorCode::FromHResult(hr);
    }

    healthCheckWaitDuration_ = FromPublicTimeInSeconds(publicDescription.HealthCheckWaitDurationInSeconds);
    healthCheckRetryTimeout_ = FromPublicTimeInSeconds(publicDescription.HealthCheckRetryTimeoutInSeconds);
    healthCheckStableDuration_ = FromPublicTimeInSeconds(publicDescription.HealthCheckStableDurationInSeconds);
    upgradeDomainTimeout_ = FromPublicTimeInSeconds(publicDescription.UpgradeDomainTimeoutInSeconds);
    upgradeTimeout_ = FromPublicTimeInSeconds(publicDescription.UpgradeTimeoutInSeconds);

    auto error = ParameterValidator::ValidatePercentValue(publicDescription.MaxPercentUnhealthyApplications, Constants::MaxPercentUnhealthyApplications);
    if (!error.IsSuccess()) { return error; }
    maxPercentUnhealthyApplications_ = publicDescription.MaxPercentUnhealthyApplications;
    
    error = ParameterValidator::ValidatePercentValue(publicDescription.MaxPercentUnhealthyNodes, Constants::MaxPercentUnhealthyNodes);
    if (!error.IsSuccess()) { return error; }
    maxPercentUnhealthyNodes_ = publicDescription.MaxPercentUnhealthyNodes;
    
    error = ParameterValidator::ValidatePercentValue(publicDescription.MaxPercentDeltaUnhealthyNodes, Constants::MaxPercentDeltaUnhealthyNodes);
    if (!error.IsSuccess()) { return error; }
    maxPercentDeltaUnhealthyNodes_ = publicDescription.MaxPercentDeltaUnhealthyNodes;
    
    error = ParameterValidator::ValidatePercentValue(publicDescription.MaxPercentUpgradeDomainDeltaUnhealthyNodes, Constants::MaxPercentUpgradeDomainDeltaUnhealthyNodes);
    if (!error.IsSuccess()) { return error; }
    maxPercentUpgradeDomainDeltaUnhealthyNodes_ = publicDescription.MaxPercentUpgradeDomainDeltaUnhealthyNodes;

    if (publicDescription.Reserved != nullptr)
    {
        auto ex1Ptr = reinterpret_cast<FABRIC_START_UPGRADE_DESCRIPTION_EX1*>(publicDescription.Reserved);
        if (ex1Ptr->ApplicationHealthPolicyMap != nullptr)
        {
            error = applicationHealthPolicies_.FromPublicApi(*ex1Ptr->ApplicationHealthPolicyMap);
            if (!error.IsSuccess())
            {
                return error;
            }
        }

    }

    return ErrorCodeValue::Success;
}

void StartUpgradeDescription::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_START_UPGRADE_DESCRIPTION & result) const
{
    result.ClusterConfig = heap.AddString(clusterConfig_);
    result.HealthCheckWaitDurationInSeconds = ToPublicTimeInSeconds(healthCheckWaitDuration_);
    result.HealthCheckRetryTimeoutInSeconds = ToPublicTimeInSeconds(healthCheckRetryTimeout_);
    result.HealthCheckStableDurationInSeconds = ToPublicTimeInSeconds(healthCheckStableDuration_);
    result.UpgradeDomainTimeoutInSeconds = ToPublicTimeInSeconds(upgradeDomainTimeout_);
    result.UpgradeTimeoutInSeconds = ToPublicTimeInSeconds(upgradeTimeout_);
    result.MaxPercentUnhealthyNodes = maxPercentUnhealthyNodes_;
    result.MaxPercentUnhealthyApplications = maxPercentUnhealthyApplications_;
    result.MaxPercentDeltaUnhealthyNodes = maxPercentDeltaUnhealthyNodes_;
    result.MaxPercentUpgradeDomainDeltaUnhealthyNodes = maxPercentUpgradeDomainDeltaUnhealthyNodes_;

    if (!applicationHealthPolicies_.empty())
    {
        auto policyDescriptionEx = heap.AddItem<FABRIC_START_UPGRADE_DESCRIPTION_EX1>();

        auto publicAppHealthPolicyMap = heap.AddItem<FABRIC_APPLICATION_HEALTH_POLICY_MAP>();
        policyDescriptionEx->ApplicationHealthPolicyMap = publicAppHealthPolicyMap.GetRawPointer();
        applicationHealthPolicies_.ToPublicApi(heap, *publicAppHealthPolicyMap);

        result.Reserved = policyDescriptionEx.GetRawPointer();
    }
}

DWORD StartUpgradeDescription::ToPublicTimeInSeconds(TimeSpan const & time) const
{
    if (time == TimeSpan::MaxValue)
    {
        return FABRIC_INFINITE_DURATION;
    }

    return static_cast<DWORD>(time.TotalSeconds());
}

TimeSpan StartUpgradeDescription::FromPublicTimeInSeconds(DWORD time)
{
    if (time == FABRIC_INFINITE_DURATION)
    {
        return TimeSpan::MaxValue;
    }

    return TimeSpan::FromSeconds(time);
}
