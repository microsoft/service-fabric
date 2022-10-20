// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;

INITIALIZE_SIZE_ESTIMATION(ServiceDescription)

void ServiceDescription::AddDefaultMetrics()
{
    if (metrics_.empty())
    {
        if (isStateful_) 
        {
            metrics_.push_back(ServiceLoadMetricDescription(L"PrimaryCount", FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH, 0, 0));
        }
        metrics_.push_back(ServiceLoadMetricDescription(L"ReplicaCount", FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM, 0, 0));
        metrics_.push_back(ServiceLoadMetricDescription(L"Count", FABRIC_SERVICE_LOAD_METRIC_WEIGHT_LOW, 0, 0));
    }
}

ServiceDescription::ServiceDescription()
    : name_(),
      instance_(0),
      updateVersion_(0),
      applicationName_(),
      partitionCount_(0),
      targetReplicaSetSize_(0),
      minReplicaSetSize_(0),
      isStateful_(false),
      hasPersistedState_(false),
      replicaRestartWaitDuration_(TimeSpan::MinValue),
      quorumLossWaitDuration_(TimeSpan::MinValue),
      standByReplicaKeepDuration_(TimeSpan::MinValue),
      isServiceGroup_(false),
      scaleoutCount_(0),
      defaultMoveCost_(FABRIC_MOVE_COST_LOW),
      servicePackageActivationMode_(ServiceModel::ServicePackageActivationMode::SharedProcess),
      serviceDnsName_(),
      scalingPolicies_()
{
}

ServiceDescription::ServiceDescription(
    ServiceDescription const & other,
    std::wstring && nameOverride)
    : name_(std::move(nameOverride)),
      instance_(other.instance_),
      updateVersion_(other.updateVersion_),
      type_(other.type_),
      applicationName_(other.applicationName_),
      packageVersionInstance_(other.packageVersionInstance_),
      partitionCount_(other.partitionCount_),
      targetReplicaSetSize_(other.targetReplicaSetSize_),
      minReplicaSetSize_(other.minReplicaSetSize_),
      isStateful_(other.isStateful_),
      hasPersistedState_(other.hasPersistedState_),
      replicaRestartWaitDuration_(other.replicaRestartWaitDuration_),
      quorumLossWaitDuration_(other.quorumLossWaitDuration_),
      standByReplicaKeepDuration_(other.standByReplicaKeepDuration_),
      initializationData_(other.initializationData_),
      isServiceGroup_(other.isServiceGroup_),
      serviceCorrelations_(other.serviceCorrelations_),
      placementConstraints_(other.placementConstraints_),
      scaleoutCount_(other.scaleoutCount_),
      metrics_(other.metrics_),
      placementPolicies_(other.placementPolicies_),
      defaultMoveCost_(other.defaultMoveCost_),
      servicePackageActivationMode_(other.servicePackageActivationMode_),
      serviceDnsName_(other.serviceDnsName_),
      scalingPolicies_(other.scalingPolicies_)
{
}

ServiceDescription::ServiceDescription(
    ServiceDescription const & other,
    std::vector<byte> && initializationDataOverride)
  : initializationData_(move(initializationDataOverride)),
    name_(other.name_),
    instance_(other.instance_),
    updateVersion_(other.updateVersion_),
    type_(other.type_),
    applicationName_(other.applicationName_),
    packageVersionInstance_(other.packageVersionInstance_),
    partitionCount_(other.partitionCount_),
    targetReplicaSetSize_(other.targetReplicaSetSize_),
    minReplicaSetSize_(other.minReplicaSetSize_),
    isStateful_(other.isStateful_),
    hasPersistedState_(other.hasPersistedState_),
    replicaRestartWaitDuration_(other.replicaRestartWaitDuration_),
    quorumLossWaitDuration_(other.quorumLossWaitDuration_),
    standByReplicaKeepDuration_(other.standByReplicaKeepDuration_),
    isServiceGroup_(other.isServiceGroup_),
    serviceCorrelations_(other.serviceCorrelations_),
    placementConstraints_(other.placementConstraints_),
    scaleoutCount_(other.scaleoutCount_),
    metrics_(other.metrics_),
    placementPolicies_(other.placementPolicies_),
    defaultMoveCost_(other.defaultMoveCost_),
    servicePackageActivationMode_(other.servicePackageActivationMode_),
    serviceDnsName_(other.serviceDnsName_),
    scalingPolicies_(other.scalingPolicies_)
{
}

 ServiceDescription::ServiceDescription(
    ServiceDescription const & other,
    std::wstring && nameOverride,
    ServiceModel::ServiceTypeIdentifier && typeOverride,
    std::vector<byte> && initializationDataOverride,
    std::vector<ServiceLoadMetricDescription> && loadMetricOverride,
    uint defaultMoveCostOverride)
  : initializationData_(move(initializationDataOverride)),
    name_(move(nameOverride)),
    instance_(other.instance_),
    updateVersion_(other.updateVersion_),
    type_(move(typeOverride)),
    applicationName_(other.applicationName_),
    packageVersionInstance_(other.packageVersionInstance_),
    partitionCount_(other.partitionCount_),
    targetReplicaSetSize_(other.targetReplicaSetSize_),
    minReplicaSetSize_(other.minReplicaSetSize_),
    isStateful_(other.isStateful_),
    hasPersistedState_(other.hasPersistedState_),
    replicaRestartWaitDuration_(other.replicaRestartWaitDuration_),
    quorumLossWaitDuration_(other.quorumLossWaitDuration_),
    standByReplicaKeepDuration_(other.standByReplicaKeepDuration_),
    isServiceGroup_(other.isServiceGroup_),
    serviceCorrelations_(other.serviceCorrelations_),
    placementConstraints_(other.placementConstraints_),
    scaleoutCount_(other.scaleoutCount_),
    metrics_(move(loadMetricOverride)),
    placementPolicies_(other.placementPolicies_),
    defaultMoveCost_(defaultMoveCostOverride),
    servicePackageActivationMode_(other.servicePackageActivationMode_),
    serviceDnsName_(other.serviceDnsName_),
    scalingPolicies_(other.scalingPolicies_)
{
}

ServiceDescription::ServiceDescription(
    ServiceDescription const & other,
    ServiceModel::ServicePackageVersionInstance const & versionOverride)
    :   name_(other.name_),
        instance_(other.instance_),
        updateVersion_(other.updateVersion_),
        type_(other.type_),
        applicationName_(other.applicationName_),
        packageVersionInstance_(versionOverride),
        partitionCount_(other.partitionCount_),
        targetReplicaSetSize_(other.targetReplicaSetSize_),
        minReplicaSetSize_(other.minReplicaSetSize_),
        isStateful_(other.isStateful_),
        hasPersistedState_(other.hasPersistedState_),
        replicaRestartWaitDuration_(other.replicaRestartWaitDuration_),
        quorumLossWaitDuration_(other.quorumLossWaitDuration_),
        standByReplicaKeepDuration_(other.standByReplicaKeepDuration_),
        isServiceGroup_(other.isServiceGroup_),
        serviceCorrelations_(other.serviceCorrelations_),
        placementConstraints_(other.placementConstraints_),
        scaleoutCount_(other.scaleoutCount_),
        metrics_(other.metrics_),
        placementPolicies_(other.placementPolicies_),
        defaultMoveCost_(other.defaultMoveCost_),
        initializationData_(other.initializationData_),
        servicePackageActivationMode_(other.servicePackageActivationMode_),
        serviceDnsName_(other.serviceDnsName_),
        scalingPolicies_(other.scalingPolicies_)
{
}

ServiceDescription::ServiceDescription(
    std::wstring const & name,
    uint64 instance,
    uint64 updateVersion,
    int partitionCount,
    int targetReplicaSetSize,
    int minReplicaSetSize,
    bool isStateful,
    bool hasPersistedState,
    TimeSpan replicaRestartWaitDuration,
    TimeSpan quorumLossWaitDuration,
    TimeSpan standByReplicaKeepDuration,
    ServiceModel::ServiceTypeIdentifier const& type,
    vector<ServiceCorrelationDescription> const& serviceCorrelations,
    wstring const& placementConstraints,
    int scaleoutCount,
    vector<ServiceLoadMetricDescription> const& metrics,
    uint defaultMoveCost,
    vector<byte> const & initializationData,
    std::wstring const & applicationName,
    vector<ServiceModel::ServicePlacementPolicyDescription> const& placementPolicies,
    ServiceModel::ServicePackageActivationMode::Enum servicePackageActivationMode,
    std::wstring const & serviceDnsName,
    std::vector<Reliability::ServiceScalingPolicyDescription> const & scalingPolicies) :
      name_(name),
      instance_(instance),
      updateVersion_(updateVersion),
      type_(type),
      applicationName_(applicationName),
      packageVersionInstance_(),    // only relevant for managed services
      partitionCount_(partitionCount),
      targetReplicaSetSize_(targetReplicaSetSize),
      minReplicaSetSize_(minReplicaSetSize),
      isStateful_(isStateful),
      hasPersistedState_(hasPersistedState),
      replicaRestartWaitDuration_(replicaRestartWaitDuration),
      quorumLossWaitDuration_(quorumLossWaitDuration),
      standByReplicaKeepDuration_(standByReplicaKeepDuration),
      isServiceGroup_(false),
      serviceCorrelations_(serviceCorrelations),
      placementConstraints_(placementConstraints),
      scaleoutCount_(scaleoutCount),
      metrics_(metrics),
      defaultMoveCost_(defaultMoveCost),
      initializationData_(std::move(initializationData)),
      placementPolicies_(placementPolicies),
      servicePackageActivationMode_(servicePackageActivationMode),
      serviceDnsName_(serviceDnsName),
      scalingPolicies_(scalingPolicies)
{
    StringUtility::ToLower(serviceDnsName_);

    // Assert int parameters are large enough and correct with regards to each other

    ASSERT_IFNOT(
        Expression::Build(placementConstraints_),
        "Expression::Build returned null for {0}",
        placementConstraints_);
    ASSERT_IF(
        partitionCount < 1,
        "Partition count cannot be less than 1; provided value is {0}",
        partitionCount);
    ASSERT_IF(
        targetReplicaSetSize < 1 && targetReplicaSetSize != -1,
        "Required replica count cannot be less than 1; provided value is {0}",
        targetReplicaSetSize);
    ASSERT_IF(
        scaleoutCount < 0,
        "Scaleout count cannot be less than 0; provided value is {0}",
        scaleoutCount);

    if (isStateful)
    {
        ASSERT_IF(
            minReplicaSetSize < 1,
            "MinReplicaSetSize cannot be less than 1; provided value is {0}",
            minReplicaSetSize);
        ASSERT_IF(
            minReplicaSetSize > targetReplicaSetSize,
            "MinReplicaSetSize cannot be less than or equal to required replica count; provided value for MinReplicaSetSize is {0} and target count is {1}",
            minReplicaSetSize,
            targetReplicaSetSize);
    }
}

ServiceDescription & ServiceDescription::operator = (ServiceDescription const& other)
{
    if (this != &other)
    {
        name_ = other.name_;
        instance_ = other.instance_;
        updateVersion_ = other.updateVersion_;
        type_ = other.type_;
        applicationName_ = other.applicationName_;
        packageVersionInstance_ = other.packageVersionInstance_;
        partitionCount_ = other.partitionCount_;
        targetReplicaSetSize_ = other.targetReplicaSetSize_;
        minReplicaSetSize_ = other.minReplicaSetSize_;
        isStateful_ = other.isStateful_;
        hasPersistedState_ = other.hasPersistedState_;
        replicaRestartWaitDuration_ = other.replicaRestartWaitDuration_;
        quorumLossWaitDuration_ = other.quorumLossWaitDuration_;
        standByReplicaKeepDuration_ = other.standByReplicaKeepDuration_;
        isServiceGroup_ = other.isServiceGroup_;
        serviceCorrelations_ = other.serviceCorrelations_;
        placementConstraints_ = other.placementConstraints_;
        scaleoutCount_ = other.scaleoutCount_;
        metrics_ = other.metrics_;
        defaultMoveCost_ = other.defaultMoveCost_;
        placementPolicies_ = other.placementPolicies_;

        initializationData_ = other.initializationData_;
        servicePackageActivationMode_ = other.servicePackageActivationMode_;
        serviceDnsName_ = other.serviceDnsName_;
        scalingPolicies_ = other.scalingPolicies_;
    }

    return *this;
}

ServiceDescription & ServiceDescription::operator = (ServiceDescription && other)
{
    if (this != &other)
    {
        name_ = std::move(other.name_);
        instance_ = other.instance_;
        updateVersion_ = other.updateVersion_;
        type_ = std::move(other.type_);
        applicationName_ = std::move(other.applicationName_);
        packageVersionInstance_ = other.packageVersionInstance_;
        partitionCount_ = other.partitionCount_;
        targetReplicaSetSize_ = other.targetReplicaSetSize_;
        minReplicaSetSize_ = other.minReplicaSetSize_;
        isStateful_ = other.isStateful_;
        hasPersistedState_ = other.hasPersistedState_;
        replicaRestartWaitDuration_ = other.replicaRestartWaitDuration_;
        quorumLossWaitDuration_ = other.quorumLossWaitDuration_;
        standByReplicaKeepDuration_ = other.standByReplicaKeepDuration_;
        isServiceGroup_ = other.isServiceGroup_;
        serviceCorrelations_ = std::move(other.serviceCorrelations_);
        placementConstraints_ = std::move(other.placementConstraints_);
        scaleoutCount_ = other.scaleoutCount_;
        metrics_ = std::move(other.metrics_);
        initializationData_ = std::move(other.initializationData_);
        defaultMoveCost_ = other.defaultMoveCost_;
        placementPolicies_ = std::move(other.placementPolicies_);
        servicePackageActivationMode_ = std::move(other.servicePackageActivationMode_);
        serviceDnsName_ = std::move(other.serviceDnsName_);
        scalingPolicies_ = std::move(other.scalingPolicies_);
    }

    return *this;
}

TimeSpan ServiceDescription::get_ReplicaRestartWaitDuration() const
{
    if (replicaRestartWaitDuration_ < TimeSpan::Zero)
    {
        if (type_.IsSystemServiceType())
        {
            return ServiceModel::ServiceModelConfig::GetConfig().SystemReplicaRestartWaitDuration;
        }
        else
        {
            return ServiceModel::ServiceModelConfig::GetConfig().UserReplicaRestartWaitDuration;
        }
    }

    return replicaRestartWaitDuration_;
}

TimeSpan ServiceDescription::get_QuorumLossWaitDuration() const
{
    if (quorumLossWaitDuration_ < TimeSpan::Zero)
    {
        return ServiceModel::ServiceModelConfig::GetConfig().QuorumLossWaitDuration;
    }

    return quorumLossWaitDuration_;
}

TimeSpan ServiceDescription::get_StandByReplicaKeepDuration() const
{
    if (standByReplicaKeepDuration_ < TimeSpan::Zero)
    {
        if (type_.IsSystemServiceType())
        {
            return ServiceModel::ServiceModelConfig::GetConfig().SystemStandByReplicaKeepDuration;
        }
        else
        {
            return ServiceModel::ServiceModelConfig::GetConfig().UserStandByReplicaKeepDuration;
        }
    }

    return standByReplicaKeepDuration_;
}

void ServiceDescription::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.Write("{0} ({1}):{2}@{3} [{4}] ({5}) [{6}]/{7}/{8}",
        name_,
        serviceDnsName_,
        type_,
        applicationName_,
        servicePackageActivationMode_,
        packageVersionInstance_,
        partitionCount_,
        targetReplicaSetSize_,
        minReplicaSetSize_);

    writer.Write(" {0}{1} {2}:{3} {4} {5} {6}",
        isStateful_ ? "S" : "",
        hasPersistedState_ ? "P" : "",
        instance_,
        updateVersion_,
        hasPersistedState_ && replicaRestartWaitDuration_ >= TimeSpan::Zero ? formatString(replicaRestartWaitDuration_) : "",
        hasPersistedState_ && quorumLossWaitDuration_ >= TimeSpan::Zero ? formatString(quorumLossWaitDuration_) : "",
        hasPersistedState_ && standByReplicaKeepDuration_ >= TimeSpan::Zero ? formatString(standByReplicaKeepDuration_) : "");

    writer.Write(" {0} {1} {2} {3} {4} {5} {6} init={7}bytes",
        serviceCorrelations_,
        placementConstraints_,
        scaleoutCount_,
        metrics_,
        defaultMoveCost_,
        placementPolicies_,
        isServiceGroup_,
        initializationData_.size());
}

void ServiceDescription::WriteToEtw(uint16 contextSequenceId) const
{
    std::wstring details = wformatString(" {0}{1} {2}:{3} {4} {5} {6}",
        isStateful_ ? "S" : "",
        hasPersistedState_ ? "P" : "",
        instance_,
        updateVersion_,
        hasPersistedState_ ? formatString(ReplicaRestartWaitDuration) : "",
        hasPersistedState_ ? formatString(QuorumLossWaitDuration) : "",
        hasPersistedState_ ? formatString(StandByReplicaKeepDuration) : "");

    std::wstring details2 = wformatString(" {0} {1} {2} {3} {4} {5} {6} init={7}bytes",
        serviceCorrelations_,
        placementConstraints_,
        scaleoutCount_,
        metrics_,
        defaultMoveCost_,
        placementPolicies_,
        isServiceGroup_,
        initializationData_.size());

    details.append(details2);

    ServiceModel::ServiceModelEventSource::Trace->ServiceDescription(
        contextSequenceId,
        name_,
        serviceDnsName_,
        wformatString(type_),
        applicationName_,
        servicePackageActivationMode_,
        packageVersionInstance_,
        partitionCount_,
        targetReplicaSetSize_,
        minReplicaSetSize_,
        details);
}

bool ServiceDescription::operator == (ServiceDescription const & other) const
{
    auto error = Equals(other);

    return error.IsSuccess();
}

bool ServiceDescription::operator != (ServiceDescription const & other) const
{
    return !(*this == other);
}

Common::ErrorCode ServiceDescription::Equals(ServiceDescription const& other) const
{
    if (initializationData_.size() != other.initializationData_.size())
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_FM_RC(ServiceDescription_InitializationData_Size_Changed), initializationData_.size(), other.initializationData_.size()));
    }

    for (size_t ix = 0; ix < initializationData_.size(); ++ix)
    {
        if (initializationData_[ix] != other.initializationData_[ix])
        {
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(
                    GET_FM_RC(ServiceDescription_InitializationData_Changed), ix, initializationData_[ix], other.initializationData_[ix]));
        }
    }

    if (metrics_.size() != other.metrics_.size())
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_FM_RC(ServiceDescription_Load_Metric_Description_Count_Changed), metrics_.size(), other.metrics_.size()));
    }

    for (auto iter = metrics_.begin(); iter != metrics_.end(); ++iter)
    {
        auto findIter = find(other.metrics_.begin(), other.metrics_.end(), *iter);
        if (findIter == other.metrics_.end())
        {
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(
                    GET_FM_RC(ServiceDescription_Load_Metric_Removed), *iter));
        }
    }

    for (auto iter = other.metrics_.begin(); iter != other.metrics_.end(); ++iter)
    {
        auto findIter = find(metrics_.begin(), metrics_.end(), *iter);
        if (findIter == metrics_.end())
        {
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                wformatString(
                    GET_FM_RC(ServiceDescription_Load_Metric_Added), *iter));
        }
    }

    if (scalingPolicies_.size() != other.scalingPolicies_.size())
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_FM_RC(ServiceDescription_ScalingPolicies_Size_Changed), scalingPolicies_.size(), other.scalingPolicies_.size()));
    }

    for (size_t index = 0; index < scalingPolicies_.size(); ++index)
    {
        auto error = scalingPolicies_[index].Equals(other.scalingPolicies_[index], true);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    if (name_ != other.name_)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_FM_RC(ServiceDescription_ServiceName_Changed), name_, other.name_));
    }

    if (type_ != other.type_)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_FM_RC(ServiceDescription_ServiceType_Changed), type_, other.type_));
    }

    if (applicationName_ != other.applicationName_)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_FM_RC(ServiceDescription_ApplicationName_Changed), applicationName_, other.applicationName_));
    }

    if (partitionCount_ != other.partitionCount_)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_FM_RC(ServiceDescription_Partition_Count_Changed), partitionCount_, other.partitionCount_));
    }

    if (targetReplicaSetSize_ != other.targetReplicaSetSize_)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_FM_RC(ServiceDescription_TargetReplicaSetSize_Changed), targetReplicaSetSize_, other.targetReplicaSetSize_));
    }

    if (isStateful_ != other.isStateful_)
    {
        if (isStateful_)
        {
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                GET_FM_RC(ServiceDescription_Changed_To_Stateless));
        }
        else
        {
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                GET_FM_RC(ServiceDescription_Changed_To_Stateful));
        }
    }

    if (hasPersistedState_ != other.hasPersistedState_)
    {
        if (hasPersistedState_)
        {
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                GET_FM_RC(ServiceDescription_Changed_To_NonPersisted));
        }
        else
        {
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                GET_FM_RC(ServiceDescription_Changed_To_Persisted));
        }
    }

    if (replicaRestartWaitDuration_ != other.replicaRestartWaitDuration_)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_FM_RC(ServiceDescription_ReplicaRestartWaitDuration_Changed), replicaRestartWaitDuration_, other.replicaRestartWaitDuration_));
    }

    if (quorumLossWaitDuration_ != other.quorumLossWaitDuration_)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_FM_RC(ServiceDescription_QuorumLossWaitDuration_Changed), quorumLossWaitDuration_, other.quorumLossWaitDuration_));
    }

    if (standByReplicaKeepDuration_ != other.standByReplicaKeepDuration_)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_FM_RC(ServiceDescription_StandByReplicaKeepDuration_Changed), standByReplicaKeepDuration_, other.standByReplicaKeepDuration_));
    }

    if (placementConstraints_ != other.placementConstraints_)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_FM_RC(ServiceDescription_PlacementConstraints_Changed), placementConstraints_, other.placementConstraints_));
    }

    if (placementPolicies_.size() != other.placementPolicies_.size())
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_FM_RC(ServiceDescription_PlacementPolicy_Count_Changed), placementPolicies_.size(), other.placementPolicies_.size()));
    }

    for (size_t ix = 0; ix < placementPolicies_.size(); ++ix)
    {
        auto error = placementPolicies_[ix].Equals(other.placementPolicies_[ix]);
 
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    if (scaleoutCount_ != other.scaleoutCount_)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_FM_RC(ServiceDescription_ScaleoutCount_Changed), scaleoutCount_, other.scaleoutCount_));
    }

    if (defaultMoveCost_ != other.defaultMoveCost_)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_FM_RC(ServiceDescription_DefaultMoveCost_Changed), defaultMoveCost_, other.defaultMoveCost_));
    }

    if (servicePackageActivationMode_ != other.servicePackageActivationMode_)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_FM_RC(ServiceDescription_ServicePackageActivationMode_Changed), servicePackageActivationMode_, other.servicePackageActivationMode_));
    }

    if (!StringUtility::AreEqualCaseInsensitive(serviceDnsName_, other.serviceDnsName_))
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_FM_RC(ServiceDescription_ServiceDnsName_Changed), serviceDnsName_, other.serviceDnsName_));
    }

    if (packageVersionInstance_ != other.packageVersionInstance_)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_FM_RC(ServiceDescription_PackageVersionInstance_Changed), packageVersionInstance_, other.packageVersionInstance_));
    }

    if (minReplicaSetSize_ != other.minReplicaSetSize_)
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(
                GET_FM_RC(ServiceDescription_MinReplicaSetSize_Changed), minReplicaSetSize_, other.minReplicaSetSize_));
    }

    if (isServiceGroup_ != other.isServiceGroup_)
    {
        if (isServiceGroup_)
        {
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                GET_FM_RC(ServiceDescription_Changed_To_Lone_Service));
        }
        else
        {
            return ErrorCode(
                ErrorCodeValue::InvalidArgument,
                GET_FM_RC(ServiceDescription_Changed_To_ServiceGroup));
        }
    }

    return ErrorCode::Success();
}

void ServiceDescription::put_ServiceDnsName(std::wstring const & value)
{
    serviceDnsName_ = value;
    StringUtility::ToLower(serviceDnsName_);
}
