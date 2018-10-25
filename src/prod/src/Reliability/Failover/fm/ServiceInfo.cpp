// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace Federation;
using namespace ServiceModel;

ServiceInfo::ServiceInfo()
    : StoreData(),
    isServiceUpdateNeeded_(false),
    healthSequence_(FABRIC_INVALID_SEQUENCE_NUMBER),
    isToBeDeleted_(false),
    isForceDelete_(false),
    isDeleted_(false),
    failoverUnitIds_(),
    deleteOperation_(),
    repartitionInfo_(nullptr),
    isStale_(false)
{
}

ServiceInfo::ServiceInfo(ServiceInfo const & other)
    : StoreData(other.OperationLSN, PersistenceState::ToBeUpdated),
    serviceDesc_(other.serviceDesc_),
    serviceType_(other.serviceType_),
    isServiceUpdateNeeded_(other.isServiceUpdateNeeded_),
    isToBeDeleted_(other.isToBeDeleted_),
    isForceDelete_(other.isForceDelete_),
    isDeleted_(other.isDeleted_),
    healthSequence_(other.healthSequence_),
    lastUpdated_(other.lastUpdated_),
    failoverUnitIds_(other.failoverUnitIds_),
    deleteOperation_(other.deleteOperation_),
    repartitionInfo_(other.repartitionInfo_ ? make_unique<FailoverManagerComponent::RepartitionInfo>(*other.repartitionInfo_) : nullptr),
    isStale_(false)
{
}

ServiceInfo::ServiceInfo(ServiceInfo const & other, Reliability::ServiceDescription && serviceDescription)
    : StoreData(other.OperationLSN, PersistenceState::ToBeUpdated),
    serviceDesc_(move(serviceDescription)),
    serviceType_(other.serviceType_),
    isServiceUpdateNeeded_(other.isServiceUpdateNeeded_),
    isToBeDeleted_(other.isToBeDeleted_),
    isForceDelete_(other.isForceDelete_),
    isDeleted_(other.isDeleted_),
    healthSequence_(other.healthSequence_),
    lastUpdated_(other.lastUpdated_),
    failoverUnitIds_(other.failoverUnitIds_),
    deleteOperation_(other.deleteOperation_),
    repartitionInfo_(other.repartitionInfo_ ? make_unique<FailoverManagerComponent::RepartitionInfo>(*other.repartitionInfo_) : nullptr),
    isStale_(false)
{
}

ServiceInfo::ServiceInfo(
    Reliability::ServiceDescription const& serviceDesc,
    ServiceTypeSPtr const& serviceType,
    FABRIC_SEQUENCE_NUMBER healthSequence,
    bool isServiceUpdateNeeded)
    : StoreData(PersistenceState::ToBeInserted),
    serviceDesc_(serviceDesc),
    serviceType_(serviceType),
    isServiceUpdateNeeded_(isServiceUpdateNeeded),
    lastUpdated_(DateTime::Now()),
    isToBeDeleted_(false),
    isForceDelete_(false),
    isDeleted_(false),
    healthSequence_(healthSequence),
    failoverUnitIds_(),
    deleteOperation_(),
    repartitionInfo_(nullptr),
    isStale_(false)
{
}

wstring const& ServiceInfo::GetStoreType()
{
    return FailoverManagerStore::ServiceInfoType;
}

wstring const& ServiceInfo::GetStoreKey() const
{
    return Name;
}

ServiceModel::ServicePackageVersionInstance ServiceInfo::GetTargetVersion(std::wstring const & upgradeDomain) const
{
    ServiceModel::ServicePackageVersionInstance result;
    if (!serviceType_->Application->GetUpgradeVersionForServiceType(ServiceType->Type, result, upgradeDomain))
    {
        return ServiceDescription.PackageVersionInstance;
    }

    return result;
}

void ServiceInfo::AddUpdatedNode(NodeId nodeId)
{
    AcquireExclusiveLock lock(lock_);

    if (updatedNodes_.find(nodeId) == updatedNodes_.end())
    {
        updatedNodes_.insert(nodeId);
    }
}

void ServiceInfo::ClearUpdatedNodes()
{
    AcquireExclusiveLock lock(lock_);
    updatedNodes_.clear();
}

void ServiceInfo::AddDeleteOperation(Common::AsyncOperationSPtr const& deleteOperation)
{
    AcquireExclusiveLock lock(lock_);

    if (deleteOperation_)
    {
        deleteOperation_->TryComplete(deleteOperation_, ErrorCodeValue::FMServiceDeleteInProgress);
    }

    deleteOperation_ = deleteOperation;
}

void ServiceInfo::CompleteDeleteOperation(ErrorCode const& error)
{
    AcquireExclusiveLock lock(lock_);

    if (deleteOperation_)
    {
        deleteOperation_->TryComplete(deleteOperation_, error);
    }

    deleteOperation_ = nullptr;
}

void ServiceInfo::PostUpdate(DateTime updatedTime)
{
    lastUpdated_ = updatedTime;
}

void ServiceInfo::PostRead(int64 operationLSN)
{
    StoreData::PostRead(operationLSN);
}

LoadBalancingComponent::ServiceDescription ServiceInfo::GetPLBServiceDescription() const
{
    vector<LoadBalancingComponent::ServiceMetric> metrics;
    for (auto itMetric = ServiceDescription.Metrics.begin(); itMetric != ServiceDescription.Metrics.end(); ++itMetric)
    {
        double weight;
        switch (itMetric->Weight)
        {
        case FABRIC_SERVICE_LOAD_METRIC_WEIGHT_ZERO:
            weight = LoadBalancingComponent::Constants::MetricWeightZero;
            break;
        case FABRIC_SERVICE_LOAD_METRIC_WEIGHT_LOW:
            weight = LoadBalancingComponent::Constants::MetricWeightLow;
            break;
        case FABRIC_SERVICE_LOAD_METRIC_WEIGHT_MEDIUM:
            weight = LoadBalancingComponent::Constants::MetricWeightMedium;
            break;
        case FABRIC_SERVICE_LOAD_METRIC_WEIGHT_HIGH:
            weight = LoadBalancingComponent::Constants::MetricWeightHigh;
            break;
        default:
            Assert::CodingError("Invalid service metric weight {0}", static_cast<int>(itMetric->Weight));
        }

        metrics.push_back(LoadBalancingComponent::ServiceMetric(
            wstring(itMetric->Name),
            weight,
            itMetric->PrimaryDefaultLoad,
            itMetric->SecondaryDefaultLoad));
    }

    wstring affinitizedService;
    bool alignedAffinity = true;
    if (!ServiceDescription.Correlations.empty())
    {
        ASSERT_IFNOT(ServiceDescription.Correlations.size() == 1, "Only one affinity correlationship is supported");

        // If it is non-internal service, the original name will be returned
        affinitizedService = SystemServiceApplicationNameHelper::GetInternalServiceName(ServiceDescription.Correlations[0].ServiceName);

        switch (ServiceDescription.Correlations[0].Scheme)
        {
        case FABRIC_SERVICE_CORRELATION_SCHEME_AFFINITY:
        case FABRIC_SERVICE_CORRELATION_SCHEME_ALIGNED_AFFINITY:
            break;
        case FABRIC_SERVICE_CORRELATION_SCHEME_NONALIGNED_AFFINITY:
            alignedAffinity = false;
            break;
        default:
            Assert::CodingError("Invalid service affinity type {0}", static_cast<int>(ServiceDescription.Correlations[0].Scheme));
        }
    }

    // if stateless and relica set size is -1, set flag onEveryNode to true
    bool onEveryNode = false;
    if (!ServiceDescription.IsStateful && ServiceDescription.TargetReplicaSetSize == -1)
    {
        onEveryNode = true;
    }

    // Append the policies to the placement constraints
    wstring placementConstraints = ServiceDescription.PlacementConstraints;
    ServiceModel::ApplicationServiceDescription::PlacementPoliciesToPlacementConstraints(
        ServiceDescription.PlacementPolicies,
        placementConstraints);

    return LoadBalancingComponent::ServiceDescription(
        wstring(ServiceDescription.Name),
        wstring(ServiceDescription.Type.QualifiedServiceTypeName),
        wstring(ServiceDescription.ApplicationName),
        ServiceDescription.IsStateful,
        move(placementConstraints),
        move(affinitizedService),
        alignedAffinity,
        move(metrics),
        ServiceDescription.DefaultMoveCost,
        onEveryNode,
        ServiceDescription.PartitionCount,
        ServiceDescription.TargetReplicaSetSize,
        ServiceDescription.HasPersistedState,
        ServiceModel::ServicePackageIdentifier(ServiceDescription.Type.ServicePackageId),
        ServiceDescription.PackageActivationMode,
        ServiceDescription.Instance,
        vector<Reliability::ServiceScalingPolicyDescription>(ServiceDescription.ScalingPolicies));
}

std::string ServiceInfo::MakeFlags() const
{
    std::string flags = "";

    if (isDeleted_)
    {
        flags += 'D';
    }
    if (isToBeDeleted_)
    {
        flags += 'T';
    }
    if (isForceDelete_)
    {
        flags += 'F';
    }
    if (isServiceUpdateNeeded_)
    {
        flags += 'U';
    }
    if (isStale_)
    {
        flags += 'S';
    }
    if (flags.empty())
    {
        flags += '-';
    }

    if (repartitionInfo_)
    {
        flags += formatString(" {0}", repartitionInfo_->RepartitionType);
    }

    return flags;
}

void ServiceInfo::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.WriteLine("{0} {1} {2} {3} {4}",
        serviceDesc_,
        MakeFlags(),
        healthSequence_,
        this->OperationLSN,
        lastUpdated_);

    writer.WriteLine("{0:5}", failoverUnitIds_);
}

std::string ServiceInfo::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{
    std::string format = "{0} {1} {2} {3} {4}\r\n{5}";
    size_t index = 0;
    traceEvent.AddEventField<Reliability::ServiceDescription>(format, name + ".serviceDescription", index);
    traceEvent.AddEventField<std::string>(format, name + ".flags", index);
    traceEvent.AddEventField<int64>(format, name + ".healthSequence", index);
    traceEvent.AddEventField<int64>(format, name + ".lsn", index);
    traceEvent.AddEventField<DateTime>(format, name + ".lastUpdated", index);
    traceEvent.AddEventField<wstring>(format, name + ".failoverUnitIds", index);
    return format;
}

void ServiceInfo::FillEventData(Common::TraceEventContext & context) const
{
    context.Write<Reliability::ServiceDescription>(serviceDesc_);
    context.WriteCopy<std::string>(MakeFlags());
    context.Write(healthSequence_);
    context.Write(this->OperationLSN);
    context.Write(lastUpdated_);
    context.Write(wformatString("{0:5}", failoverUnitIds_));
}

ServiceModel::ServiceQueryResult ServiceInfo::ToServiceQueryResult(FabricCodeVersion const& fabricCodeVersion) const
{
    Uri serviceName;
    Uri::TryParse(SystemServiceApplicationNameHelper::GetPublicServiceName(Name), serviceName);

    wstring version;
    if (serviceType_->Type.IsSystemServiceType())
    {
        version = fabricCodeVersion.ToString();
    }
    else
    {
        version = ServiceDescription.PackageVersionInstance.Version.ToString();
    }

    if (ServiceDescription.IsStateful)
    {
        return ServiceModel::ServiceQueryResult::CreateStatefulServiceQueryResult(
            serviceName,
            ServiceType->Type.ServiceTypeName,
            version,
            ServiceDescription.HasPersistedState,
            Status,
            ServiceDescription.IsServiceGroup);
    }
    else
    {
        return ServiceModel::ServiceQueryResult::CreateStatelessServiceQueryResult(
            serviceName,
            ServiceType->Type.ServiceTypeName,
            version,
            Status,
            ServiceDescription.IsServiceGroup);
    }
}

ServiceModel::ServiceGroupMemberQueryResult ServiceInfo::ToServiceGroupMemberQueryResult(FabricCodeVersion const& fabricCodeVersion) const
{
    Uri serviceName;
    Uri::TryParse(SystemServiceApplicationNameHelper::GetPublicServiceName(Name), serviceName);

    wstring version;
    if (serviceType_->Type.IsSystemServiceType())
    {
        version = fabricCodeVersion.ToString();
    }
    else
    {
        version = ServiceDescription.PackageVersionInstance.Version.ToString();
    }

    CServiceGroupDescription cServiceGroupDescription;
    std::vector<byte> serializedInitializationData(serviceDesc_.get_InitializationData());
    Common::FabricSerializer::Deserialize(cServiceGroupDescription, serializedInitializationData);

    return ServiceModel::ServiceGroupMemberQueryResult::CreateServiceGroupMemberQueryResult(
        serviceName,
        cServiceGroupDescription.ServiceGroupMemberData);
}

FABRIC_QUERY_SERVICE_STATUS ServiceInfo::get_Status() const
{
    if (IsToBeDeleted)
    {
        return FABRIC_QUERY_SERVICE_STATUS_DELETING;
    }

    ServiceModel::ServicePackageVersionInstance versionInstance;
    if (serviceType_->Application->GetUpgradeVersionForServiceType(serviceType_->Type, versionInstance))
    {
        return FABRIC_QUERY_SERVICE_STATUS_UPGRADING;
    }

    return FABRIC_QUERY_SERVICE_STATUS_ACTIVE;
}

void ServiceInfo::AddFailoverUnitId(FailoverUnitId const& failoverUnitId)
{
    failoverUnitIds_.insert(failoverUnitId);
}

bool ServiceInfo::SetFailoverUnitIds(set<FailoverUnitId> && failoverUnitIds)
{
    size_t oldCount = failoverUnitIds_.size();

    if (oldCount > serviceDesc_.PartitionCount)
    {
        failoverUnitIds_.clear();
    }

    failoverUnitIds_.insert(failoverUnitIds.begin(), failoverUnitIds.end());

    size_t newCount = failoverUnitIds_.size();

    return (newCount != oldCount);
}

void ServiceInfo::ClearFailoverUnitIds()
{
    failoverUnitIds_.clear();
}
