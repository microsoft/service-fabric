// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Constants.h"
#include "ServiceDescription.h"
#include "PlacementAndLoadBalancing.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

GlobalWString const ServiceDescription::FormatHeader = make_global<wstring>(L"Service descriptions");

ServiceDescription::ServiceDescription(
    wstring && serviceName, 
    wstring && serviceTypeName,
    wstring && applicationName,
    bool isStateful,
    wstring && placementConstraints,
    wstring && affinitizedService,
    bool alignedAffinity,
    vector<ServiceMetric> && metrics,
    uint defaultMoveCost,
    bool onEveryNode,
    int partitionCount,
    int targetReplicaSetSize,
    bool hasPersistedState,
    ServiceModel::ServicePackageIdentifier && servicePackageIdentifier,
    ServiceModel::ServicePackageActivationMode::Enum servicePackageActivationMode,
    uint64 serviceInstance,
    vector<Reliability::ServiceScalingPolicyDescription> && scalingPolicies)
    : serviceName_(move(serviceName)),
    serviceTypeName_(move(serviceTypeName)),
    applicationName_(move(applicationName)),
    isStateful_(isStateful),
    placementConstraints_(move(placementConstraints)),
    affinitizedService_(move(affinitizedService)),
    alignedAffinity_(alignedAffinity),
    metrics_(move(metrics)),
    defaultMoveCost_(defaultMoveCost),
    onEveryNode_(onEveryNode),
    partitionCount_(partitionCount),
    targetReplicaSetSize_(targetReplicaSetSize),
    hasPersistedState_(hasPersistedState),
    serviceId_(0),
    applicationId_(0),
    servicePackageId_(0),
    servicePackageIdentifier_(move(servicePackageIdentifier)),
    servicePackageActivationMode_(servicePackageActivationMode),
    serviceInstance_(serviceInstance),
    scalingPolicies_(move(scalingPolicies))
{
}

ServiceDescription::ServiceDescription(ServiceDescription const & other)
    : serviceName_(other.serviceName_),
    serviceTypeName_(other.serviceTypeName_),
    applicationName_(other.applicationName_),
    isStateful_(other.isStateful_),
    placementConstraints_(other.placementConstraints_),
    affinitizedService_(other.affinitizedService_),
    alignedAffinity_(other.alignedAffinity_),
    metrics_(other.metrics_),
    defaultMoveCost_(other.defaultMoveCost_),
    onEveryNode_(other.onEveryNode_),
    partitionCount_(other.partitionCount_),
    targetReplicaSetSize_(other.targetReplicaSetSize_),
    hasPersistedState_(other.hasPersistedState_),
    serviceId_(other.serviceId_),
    applicationId_(other.applicationId_),
    servicePackageId_(other.servicePackageId_),
    servicePackageIdentifier_(other.servicePackageIdentifier_),
    servicePackageActivationMode_(other.servicePackageActivationMode_),
    serviceInstance_(other.serviceInstance_),
    scalingPolicies_(other.scalingPolicies_)
{
}

ServiceDescription::ServiceDescription(ServiceDescription && other)
    : serviceName_(move(other.serviceName_)),
    serviceTypeName_(move(other.serviceTypeName_)),
    applicationName_(move(other.applicationName_)),
    isStateful_(other.isStateful_),
    placementConstraints_(move(other.placementConstraints_)),
    affinitizedService_(move(other.affinitizedService_)),
    alignedAffinity_(other.alignedAffinity_),
    metrics_(move(other.metrics_)),
    defaultMoveCost_(other.defaultMoveCost_),
    onEveryNode_(other.onEveryNode_),
    partitionCount_(other.partitionCount_),
    targetReplicaSetSize_(other.targetReplicaSetSize_),
    hasPersistedState_(other.hasPersistedState_),
    serviceId_(other.serviceId_),
    applicationId_(other.applicationId_),
    servicePackageId_(other.servicePackageId_),
    servicePackageIdentifier_(move(other.servicePackageIdentifier_)),
    servicePackageActivationMode_(other.servicePackageActivationMode_),
    serviceInstance_(other.serviceInstance_),
    scalingPolicies_(move(other.scalingPolicies_))
{
}

ServiceDescription & ServiceDescription::operator = (ServiceDescription && other)
{
    if (this != &other)
    {
        serviceName_ = move(other.serviceName_);
        serviceTypeName_ = move(other.serviceTypeName_);
        applicationName_ = move(other.applicationName_);
        isStateful_ = other.isStateful_;
        placementConstraints_ = move(other.placementConstraints_);
        affinitizedService_ = move(other.affinitizedService_);
        alignedAffinity_ = other.alignedAffinity_;
        metrics_ = move(other.metrics_);
        defaultMoveCost_ = other.defaultMoveCost_;
        onEveryNode_ = other.onEveryNode_;
        partitionCount_ = other.partitionCount_;
        targetReplicaSetSize_ = other.targetReplicaSetSize_;
        hasPersistedState_ = other.hasPersistedState_;
        serviceId_ = other.serviceId_;
        applicationId_ = other.applicationId_;
        servicePackageId_ = other.servicePackageId_;
        servicePackageIdentifier_ = move(other.servicePackageIdentifier_);
        servicePackageActivationMode_ = other.servicePackageActivationMode_;
        serviceInstance_ = other.serviceInstance_;
        scalingPolicies_ = move(other.scalingPolicies_);
    }

    return *this;
}

bool ServiceDescription::operator == (ServiceDescription const & other ) const
{
    // it is only used when we update the service description
    ASSERT_IFNOT(serviceName_ == other.serviceName_, "Comparison between two different service");

    if (scalingPolicies_.size() == other.scalingPolicies_.size())
    {
        for (auto index = 0; index < scalingPolicies_.size(); ++index)
        {
            if (!scalingPolicies_[index].Equals(other.scalingPolicies_[index], true).IsSuccess())
            {
                return false;
            }
        }
    }
    else
    {
        return false;
    }

    return (
        serviceTypeName_ == other.serviceTypeName_ &&
        applicationName_ == other.applicationName_ &&
        isStateful_ == other.isStateful_ &&
        placementConstraints_ == other.placementConstraints_ &&
        affinitizedService_ == other.affinitizedService_ &&
        alignedAffinity_ == other.alignedAffinity_ &&
        metrics_ == other.metrics_ &&
        defaultMoveCost_ == other.defaultMoveCost_ &&
        onEveryNode_ == other.onEveryNode_ &&
        partitionCount_ == other.partitionCount_ &&
        targetReplicaSetSize_ == other.targetReplicaSetSize_ &&
        hasPersistedState_ == other.hasPersistedState_ &&
        serviceId_ == other.serviceId_ &&
        applicationId_ == other.applicationId_ &&
        servicePackageId_ == other.servicePackageId_ &&
        servicePackageIdentifier_ == other.servicePackageIdentifier_ &&
        servicePackageActivationMode_ == other.servicePackageActivationMode_
        );
}

bool ServiceDescription::operator != (ServiceDescription const & other ) const
{
    return !(*this == other);
}

bool ServiceDescription::AddRGMetric(wstring metricName, double weight, uint primaryLoad, uint secondaryLoad)
{
    for (auto & metric : metrics_)
    {
        if (metric.Name == metricName)
        {
            return false;
        }
    }
    metrics_.push_back(ServiceMetric(move(metricName), weight, primaryLoad, secondaryLoad, true));
    return true;
}

bool ServiceDescription::HasRGMetric() const
{
    for (auto & metric : metrics_)
    {
        if (metric.IsRGMetric)
        {
            return true;
        }
    }
    return false;
}

void ServiceDescription::UpdateRGMetrics(std::map<std::wstring, uint> const & resources)
{
    vector<ServiceMetric> metrics;
    PLBConfig const& config = PLBConfig::GetConfig();
    for (auto itResource = resources.begin(); itResource != resources.end(); ++itResource)
    {
        uint primaryLoad = 0;
        uint secondaryLoad = 0;
        if (servicePackageActivationMode_ == ServiceModel::ServicePackageActivationMode::ExclusiveProcess)
        {
            primaryLoad = itResource->second;
            secondaryLoad = itResource->second;
        }
        double rgMetricWeight = (itResource->first == ServiceModel::Constants::SystemMetricNameCpuCores) ?
            config.CpuCoresMetricWeight :
            config.MemoryInMBMetricWeight;
        metrics.push_back(ServiceMetric(wstring(itResource->first), rgMetricWeight, primaryLoad, secondaryLoad, true));
    }

    //add the non-rg metrics
    for (auto itMetric = metrics_.begin(); itMetric != metrics_.end(); ++itMetric)
    {
        if (!itMetric->IsRGMetric)
        {
            metrics.push_back(move(*itMetric));
        }
    }
    metrics_ = move(metrics);
}

bool ServiceDescription::AddDefaultMetrics()
{
    if (metrics_.empty())
    {
        metrics_.push_back(ServiceMetric(wstring(ServiceMetric::PrimaryCountName), Constants::MetricWeightHigh, 0, 0));
        metrics_.push_back(ServiceMetric(wstring(ServiceMetric::ReplicaCountName), Constants::MetricWeightMedium, 0, 0));
        metrics_.push_back(ServiceMetric(wstring(ServiceMetric::CountName), Constants::MetricWeightLow, 0, 0));
        return true;
    }
    return false;
}

void ServiceDescription::AlignSinletonReplicaDefLoads()
{
    if (this->TargetReplicaSetSize == 1)
    {
        for (auto itMetric = metrics_.begin(); itMetric != metrics_.end(); ++itMetric)
        {
            itMetric->SecondaryDefaultLoad = itMetric->PrimaryDefaultLoad;
        }
    }
}

void ServiceDescription::ClearAffinitizedService()
{
    affinitizedService_ = L"";
}

Reliability::ServiceScalingPolicyDescription ServiceDescription::get_FirstAutoScalingPolicy() const
{
    if (scalingPolicies_.size() != 1)
    {
        // This should not happen, ignore in production
        Common::Assert::TestAssert(
            "Attempting to get scaling policy when no policy defined. Service={0}",
            serviceName_);
        return Reliability::ServiceScalingPolicyDescription();
    }
    return scalingPolicies_[0];
}


void ServiceDescription::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.Write("{0} type:{1} application:{2} stateful:{3} placementConstraints:{4} affinity:{5} alignedAffinity:{6} metrics:{7} defaultMoveCost:{8} everyNode:{9} partitionCount:{10} targetReplicaSetSize:{11} servicePackageIdentifier:{12} servicePackageActivationMode:{13} serviceInstance:{14} scalingPolicies:{15}", 
        serviceName_, serviceTypeName_, applicationName_, isStateful_, placementConstraints_, affinitizedService_, alignedAffinity_,
        metrics_, defaultMoveCost_, onEveryNode_, partitionCount_, targetReplicaSetSize_, servicePackageIdentifier_, servicePackageActivationMode_, serviceInstance_, scalingPolicies_);
}

void ServiceDescription::WriteToEtw(uint16 contextSequenceId) const
{
    PlacementAndLoadBalancing::PLBTrace->PLBDump(contextSequenceId, wformatString(*this));
}
