// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ServiceDomain.h"
#include "PlacementAndLoadBalancing.h"
#include "PlacementCreator.h"
#include "PLBEventSource.h"
#include "LoadOrMoveCostDescription.h"
#include "SystemState.h"
#include "BalanceCheckerCreator.h"
#include "FailoverUnitMovement.h"
#include "PLBDiagnostics.h"
#include "ServicePackageNode.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

ServiceDomain::ServiceDomain(DomainId && id, PlacementAndLoadBalancing & plb)
    : domainId_(move(id)),
    plb_(plb),
    serviceTable_(),
    metricTable_(),
    failoverUnitTable_(),
    movePlan_(),
    scheduler_(
        domainId_,
        plb.Trace,
        plb.constraintCheckEnabled_.load(),
        plb.balancingEnabled_.load() && !plb.IsMaster,
        StopwatchTime::Zero,
        Stopwatch::Now(),
        StopwatchTime::Zero),
    changedNodes_(),
    changedServiceTypes_(),
    changedServices_(),
    lastNodeLoadsTrace_(StopwatchTime::Zero),
    lastApplicationLoadsTrace_(StopwatchTime::Zero),
    lastTracedApplication_(0),
    applicationTraceIterationNumber_(0),
    inTransitionPartitionCount_(0),
    movableReplicaCount_(0),
    existingReplicaCount_(0),
    newReplicaCount_(0),
    partitionsWithNewReplicas_(),
    partitionsWithExtraReplicas_(),
    partitionsWithInUpgradeReplicas_(),
    constraintCheckClosure_(),
    fullConstraintCheck_(false),
    partitionsPerNode_(),
    inBuildCountPerNode_(),
    childServices_(),
    applicationPartitions_(),
    applicationLoadTable_(),
    reservationLoadTable_(),
    servicePackageReplicaCountPerNode_(),
    partitionsInAppUpgrade_(0),
    autoScaler_()
{
    if (plb_.applicationTable_.size() > 0)
    {
        lastTracedApplication_ = plb_.applicationTable_.begin()->first;
    }
}

ServiceDomain::ServiceDomain(ServiceDomain && other)
    : domainId_(move(other.domainId_)),
    plb_(other.plb_),
    serviceTable_(move(other.serviceTable_)),
    metricTable_(move(other.metricTable_)),
    failoverUnitTable_(move(other.failoverUnitTable_)),
    movePlan_(move(other.movePlan_)),
    scheduler_(move(other.scheduler_)),
    changedNodes_(move(other.changedNodes_)),
    changedServiceTypes_(move(other.changedServiceTypes_)),
    changedServices_(move(other.changedServices_)),
    lastNodeLoadsTrace_(other.lastNodeLoadsTrace_),
    lastApplicationLoadsTrace_(other.lastNodeLoadsTrace_),
    lastTracedApplication_(other.lastTracedApplication_),
    applicationTraceIterationNumber_(other.applicationTraceIterationNumber_),
    inTransitionPartitionCount_(other.inTransitionPartitionCount_),
    movableReplicaCount_(other.movableReplicaCount_),
    existingReplicaCount_(other.existingReplicaCount_),
    newReplicaCount_(other.newReplicaCount_),
    partitionsWithNewReplicas_(move(other.partitionsWithNewReplicas_)),
    partitionsWithExtraReplicas_(move(other.partitionsWithExtraReplicas_)),
    partitionsWithInUpgradeReplicas_(move(other.partitionsWithInUpgradeReplicas_)),
    constraintCheckClosure_(move(other.constraintCheckClosure_)),
    fullConstraintCheck_(other.fullConstraintCheck_),
    partitionsPerNode_(move(other.partitionsPerNode_)),
    inBuildCountPerNode_(move(other.inBuildCountPerNode_)),
    childServices_(move(other.childServices_)),
    applicationPartitions_(move(other.applicationPartitions_)),
    reservationLoadTable_(move(other.reservationLoadTable_)),
    applicationLoadTable_(move(other.applicationLoadTable_)),
    servicePackageReplicaCountPerNode_(move(other.servicePackageReplicaCountPerNode_)),
    partitionsInAppUpgrade_(other.partitionsInAppUpgrade_),
    autoScaler_(other.autoScaler_)
{

}
/// <summary>
/// Adds the metric to this ServiceDomain. Can also increment ApplicationCount for a metric.
/// </summary>
/// <param name="metricName">Name of the metric.</param>
/// <param name="incrementAppCount">if set to <c>true</c> [increment application count].</param>
void ServiceDomain::AddMetric(std::wstring metricName,bool incrementAppCount)
{
    auto metricIt = metricTable_.find(metricName);
    if (metricIt != metricTable_.end())
    {
        if (incrementAppCount)
        {
            metricIt->second.ApplicationCount++;
        }
        return;
    }

    ServiceDomainMetric domainMetric(move(wstring(metricName)));
    if (incrementAppCount)
    {
        domainMetric.ApplicationCount++;
    }
    metricTable_.insert(make_pair(metricName, move(domainMetric)));
}
/// <summary>
/// Removes the metric, if it is not being referenced. Can also decrement Application count.
/// </summary>
/// <param name="metricName">Name of the metric.</param>
/// <param name="decrementAppCount">if set to <c>true</c> [decrement application count].</param>
bool ServiceDomain::RemoveMetric(std::wstring metricName,bool decrementAppCount)
{
    auto metricIt = metricTable_.find(metricName);
    if (metricIt == metricTable_.end())
    {
        return true;
    }
    if (decrementAppCount && metricIt->second.ApplicationCount > 0)
    {
        metricIt->second.ApplicationCount--;
    }
    if (metricIt->second.ApplicationCount == 0u && metricIt->second.ServiceCount == 0u)
    {
        metricTable_.erase(metricIt);
        return true;
    }
    return false;
}

/// <summary>
/// Checks for empty metrics.
/// </summary>
/// <returns></returns>
bool ServiceDomain::CheckMetrics()
{
    for (auto metricIt = metricTable_.begin(); metricIt != metricTable_.end(); metricIt++)
    {
        if (metricIt->second.ApplicationCount == 0 && metricIt->second.ServiceCount == 0)
            return true;
    }
    return false;
}

void ServiceDomain::AddService(ServiceDescription && serviceDescription)
{
    ASSERT_IF(serviceTable_.find(serviceDescription.ServiceId) != serviceTable_.end(), "Service {0} already exists", serviceDescription.Name);

    auto itInserted = serviceTable_.insert(make_pair(serviceDescription.ServiceId, Service(move(serviceDescription)))).first;
    changedServices_.insert(itInserted->first);

    auto & parent = itInserted->second.ServiceDesc.AffinitizedService;
    if (!parent.empty())
    {
        auto itChildServices = childServices_.find(parent);
        if (itChildServices == childServices_.end())
        {
            itChildServices = childServices_.insert(make_pair(parent, set<wstring>())).first;
        }
        itChildServices->second.insert(itInserted->second.ServiceDesc.Name);
    }

    bool affectsBalancing = IsServiceAffectingBalancing(itInserted->second);
    vector<ServiceMetric> const& metrics = itInserted->second.ServiceDesc.Metrics;

    for (auto itMetric = metrics.begin(); itMetric != metrics.end(); ++itMetric)
    {
        auto it = metricTable_.find(itMetric->Name);
        if (it == metricTable_.end())
        {
            ServiceDomainMetric domainMetric(wstring(itMetric->Name));
            domainMetric.IncreaseServiceCount(itMetric->Weight, affectsBalancing);
            metricTable_.insert(make_pair(itMetric->Name, move(domainMetric)));
        }
        else
        {
            it->second.IncreaseServiceCount(itMetric->Weight, affectsBalancing);
        }
    }

    //should we trace this during a domain split?...
    plb_.Trace.UpdateService(itInserted->second.ServiceDesc.Name, itInserted->second.ServiceDesc);
    scheduler_.OnServiceChanged(Stopwatch::Now());
    movePlan_.OnServiceChanged(itInserted->second);
}

void ServiceDomain::DeleteService(uint64 serviceId, wstring const& serviceName, uint64 & applicationId, std::vector<wstring> & deletedMetrics,
    bool & depended, wstring & affinitizedService, bool assertFailoverUnitEmpty)
{
    // returns the deleted metrics and whether the service is depended by any existing service
    // assume all failover units of this service are already deleted
    auto itService = serviceTable_.find(serviceId);

    ASSERT_IF(itService == serviceTable_.end(), "Service {0} doesn't exist", serviceName);

    changedServices_.erase(serviceId);

    deletedMetrics.clear();
    depended = false;
    affinitizedService = itService->second.ServiceDesc.AffinitizedService;
    applicationId = itService->second.ServiceDesc.ApplicationId;

    if (!affinitizedService.empty())
    {
        auto itChildServices = childServices_.find(affinitizedService);
        ASSERT_IF(itChildServices == childServices_.end(),
            "Deleting service {0}: the affinitized service {1} does not exist in childServices_",
            serviceName, affinitizedService);
        itChildServices->second.erase(serviceName);
        if (itChildServices->second.empty())
        {
            childServices_.erase(itChildServices);
        }
    }

    if (serviceTable_.size() > 1)
    {
        bool affectsBalancing = IsServiceAffectingBalancing(itService->second);

        vector<ServiceMetric> const& metrics = itService->second.ServiceDesc.Metrics;
        auto & blockList = itService->second.BlockList;
        for (auto itMetric = metrics.begin(); itMetric != metrics.end(); ++itMetric)
        {
            auto it = metricTable_.find(itMetric->Name);
            ASSERT_IF(it == metricTable_.end(), "Metric {0} doesn't exist", itMetric->Name);

            auto &domainMetric = it->second;

            if (affectsBalancing)
            {
                blockList.ForEach([&domainMetric](size_t nodeId) -> void
                {
                    domainMetric.UpdateBlockNodeServiceCount(nodeId, -1);
                });
            }

            if (domainMetric.ServiceCount > 1u)
            {
                domainMetric.DecreaseServiceCount(itMetric->Weight, affectsBalancing);
            }
            else if (domainMetric.ApplicationCount == 0u)
            {
                deletedMetrics.push_back(it->first);
                metricTable_.erase(it);
            }
        }

        serviceTable_.erase(itService);
        depended = HasDependentService(serviceName);
    }
    else
    {
        for (auto itMetric = metricTable_.begin(); itMetric != metricTable_.end(); ++itMetric)
        {
            deletedMetrics.push_back(itMetric->first);
        }
        metricTable_.clear();
        serviceTable_.clear();
        if(assertFailoverUnitEmpty)
        {
            ASSERT_IFNOT(failoverUnitTable_.empty(), "Failover unit table not empty when deleting service {0}", serviceName);
        }
    }

    if (assertFailoverUnitEmpty)
    {
        //should we trace this during a dummy-delete?
        plb_.Trace.UpdateServiceDeleted(serviceName);
    }

    scheduler_.OnServiceDeleted(Stopwatch::Now());
    movePlan_.OnServiceDeleted(serviceName);
}

bool ServiceDomain::HasDependentService(std::wstring const& dependedService) const
{
    for (auto it = serviceTable_.begin(); it != serviceTable_.end(); ++it)
    {
        if (it->second.ServiceDesc.AffinitizedService == dependedService)
        {
            return true;
        }
    }

    return false;
}

bool ServiceDomain::HasDependedService(std::wstring const& dependentService) const
{
    uint64 dependedServiceId(0);
    auto itServiceIdMap = plb_.serviceToIdMap_.find(dependentService);
    if (itServiceIdMap != plb_.serviceToIdMap_.end())
    {
        dependedServiceId = itServiceIdMap->second;
    }
    auto it = serviceTable_.find(dependedServiceId);

    ASSERT_IF(it == serviceTable_.end(), "Service {0} doesn't exist", dependentService);

    return !(it->second.ServiceDesc.AffinitizedService.empty());
}

void ServiceDomain::MergeDomain(ServiceDomain && other)
{
    // exclusive lock acquired at upper level

    // assumes there is no overlap between the two domains
    for (auto it = other.serviceTable_.begin(); it != other.serviceTable_.end(); ++it)
    {
        serviceTable_.insert(move(*it));
    }

    for (auto it = other.metricTable_.begin(); it != other.metricTable_.end(); ++it)
    {
        auto metricIt = metricTable_.find(it->first);
        if (metricIt == metricTable_.end())
        {
            metricTable_.insert(move(*it));
        }
        //if both domains contain an entry, we should choose whichever one is populated...
        else
        {
            //one of the domain-metrics must be populated,and the other will have only an applicationCount...
            if (metricIt->second.ServiceCount == 0u)
            {
                it->second.ApplicationCount  += metricIt->second.ApplicationCount;
                metricTable_.insert(move(*it));
            }
            else
            {
                metricIt->second.ApplicationCount += it->second.ApplicationCount;
            }
        }
    }

    for (auto it = other.failoverUnitTable_.begin(); it != other.failoverUnitTable_.end(); ++it)
    {
        failoverUnitTable_.insert(move(*it));
    }

    movePlan_.Merge(move(other.movePlan_));

    changedNodes_.insert(other.changedNodes_.begin(), other.changedNodes_.end());
    changedServiceTypes_.insert(other.changedServiceTypes_.begin(), other.changedServiceTypes_.end());
    changedServices_.insert(other.changedServices_.begin(), other.changedServices_.end());

    scheduler_.Merge(Stopwatch::Now(), move(other.scheduler_));
    lastNodeLoadsTrace_ = max(lastNodeLoadsTrace_, other.lastNodeLoadsTrace_);
    lastApplicationLoadsTrace_ = max(lastApplicationLoadsTrace_, other.lastApplicationLoadsTrace_);

    inTransitionPartitionCount_ += other.inTransitionPartitionCount_;
    movableReplicaCount_ += other.movableReplicaCount_;
    existingReplicaCount_ += other.existingReplicaCount_;
    newReplicaCount_ += other.newReplicaCount_;
    partitionsWithNewReplicas_.insert(other.partitionsWithNewReplicas_.begin(), other.partitionsWithNewReplicas_.end());
    partitionsWithExtraReplicas_.insert(other.partitionsWithExtraReplicas_.begin(), other.partitionsWithExtraReplicas_.end());
    partitionsWithInUpgradeReplicas_.insert(other.partitionsWithInUpgradeReplicas_.begin(), other.partitionsWithInUpgradeReplicas_.end());

    constraintCheckClosure_.insert(other.constraintCheckClosure_.begin(), other.constraintCheckClosure_.end());
    fullConstraintCheck_ = fullConstraintCheck_ || other.fullConstraintCheck_;
    for (auto it = other.partitionsPerNode_.begin(); it != other.partitionsPerNode_.end(); ++it)
    {
        auto itNode = partitionsPerNode_.find(it->first);
        if (itNode == partitionsPerNode_.end())
        {
            partitionsPerNode_.insert(make_pair(it->first, move(it->second)));
        }
        else
        {
            itNode->second.insert(it->second.begin(), it->second.end());
        }
    }

    for (auto it = other.inBuildCountPerNode_.begin(); it != other.inBuildCountPerNode_.end(); ++it)
    {
        auto itNode = inBuildCountPerNode_.find(it->first);
        if (itNode == inBuildCountPerNode_.end())
        {
            inBuildCountPerNode_.insert(make_pair(it->first, it->second));
        }
        else
        {
            itNode->second += it->second;
        }
    }

    for (auto it = other.childServices_.begin(); it != other.childServices_.end(); ++it)
    {
        auto itParent = childServices_.find(it->first);
        if (itParent == childServices_.end())
        {
            childServices_.insert(make_pair(it->first, move(it->second)));
        }
        else
        {
            itParent->second.insert(it->second.begin(), it->second.end());
        }
    }

    for (auto it = other.applicationPartitions_.begin(); it != other.applicationPartitions_.end(); ++it)
    {
        auto itApp = applicationPartitions_.find(it->first);
        if (itApp == applicationPartitions_.end())
        {
            applicationPartitions_.insert(make_pair(it->first, move(it->second)));
        }
        else
        {
            itApp->second.insert(it->second.begin(), it->second.end());
        }
    }

    // merge applicationLoads
    for (auto it = other.applicationLoadTable_.begin(); it != other.applicationLoadTable_.end(); ++it)
    {
        auto itAppLoad = applicationLoadTable_.find(it->first);
        if (itAppLoad == applicationLoadTable_.end())
        {
            // no application load for this application
            applicationLoadTable_.insert(make_pair(it->first, move(it->second)));
        }
        else
        {
            // merging two non-app group application (app doesn't have scaleout or same metrics)
            itAppLoad->second.Merge(it->second);
        }
    }

    // merge reservationLoadTable_
    for (auto it = other.reservationLoadTable_.begin(); it != other.reservationLoadTable_.end(); ++it)
    {
        auto itAppRes = reservationLoadTable_.find(it->first);
        if (itAppRes == reservationLoadTable_.end())
        {
            // no reservation load for this app
            itAppRes = reservationLoadTable_.insert(make_pair(it->first, move(it->second))).first;
        }
        else
        {
            itAppRes->second.MergeReservedLoad(it->second);
        }

    }

    //merge service package replica count per node
    for (auto itOther = other.servicePackageReplicaCountPerNode_.begin(); itOther != other.servicePackageReplicaCountPerNode_.end(); ++itOther)
    {
        auto itNode = servicePackageReplicaCountPerNode_.find(itOther->first);
        if (itNode != servicePackageReplicaCountPerNode_.end())
        {
            for (auto itOtherServicePackage = itOther->second.begin(); itOtherServicePackage != itOther->second.end(); ++itOtherServicePackage)
            {
                auto itServicePackage = itNode->second.find(itOtherServicePackage->first);
                if (itServicePackage != itNode->second.end())
                {
                    itServicePackage->second.MergeServicePackages(itOtherServicePackage->second);
                }
                else
                {
                    itNode->second[itOtherServicePackage->first] = move(itOtherServicePackage->second);
                }
            }
        }
        else
        {
            servicePackageReplicaCountPerNode_[itOther->first] = move(itOther->second);
        }
    }

    partitionsInAppUpgrade_ += other.partitionsInAppUpgrade_;

    autoScaler_.MergeAutoScaler(move(other.autoScaler_));
}

void ServiceDomain::AddFailoverUnit(FailoverUnit && failoverUnitToAdd, StopwatchTime timeStamp)
{
    Common::Guid fuId = failoverUnitToAdd.FuDescription.FUId;
    Service & service = GetService(failoverUnitToAdd.FuDescription.ServiceId);

    // Insert the partition to the application partition map
    if (ApplicationHasScaleoutOrCapacity(service.ServiceDesc.ApplicationId))
    {
        UpdateApplicationPartitions(service.ServiceDesc.ApplicationId, fuId);
    }

    auto itFU = failoverUnitTable_.insert(make_pair(fuId, move(failoverUnitToAdd))).first;

    // currentFU would be modified if necessary, so this should be called before AddNodeLoad
    AddToUpgradeStatistics(itFU->second.FuDescription);

    FailoverUnit const& failoverUnit = itFU->second;

    service.OnFailoverUnitAdded(failoverUnit, failoverUnitTable_, plb_.Settings);
    movePlan_.OnFailoverUnitAdded(failoverUnit);

    AddNodeLoad(service, failoverUnit, failoverUnit.FuDescription.Replicas);
    UpdateApplicationNodeCounts(service.ServiceDesc.ApplicationId, failoverUnit.FuDescription.Replicas, true);
    UpdateServicePackagePlacement(service, failoverUnit.FuDescription.Replicas, true);
    UpdateNodeToFailoverUnitMapping(failoverUnit, vector<ReplicaDescription>(), failoverUnit.FuDescription.Replicas, service.ServiceDesc.IsStateful);
    AddToPartitionStatistics(failoverUnit);
    AddToReplicaDifferenceStatistics(fuId, failoverUnit.ActualReplicaDifference);

    plb_.Trace.UpdateFailoverUnit(fuId, failoverUnit.FuDescription, failoverUnit.ActualReplicaDifference, true);
    scheduler_.OnFailoverUnitAdded(timeStamp, fuId);

    if (!fullConstraintCheck_)
    {
        constraintCheckClosure_.insert(fuId);
    }

    if (service.ServiceDesc.IsAutoScalingDefined)
    {
        if (service.ServiceDesc.AutoScalingPolicy.IsPartitionScaled())
        {
            autoScaler_.AddFailoverUnit(fuId, failoverUnit.NextScalingCheck);
        }
    }
}

bool ServiceDomain::UpdateFailoverUnit(FailoverUnitDescription && failoverUnitDescription, StopwatchTime timeStamp, bool traceDetail)
{
    bool ret = false;
    Common::Guid fuId = failoverUnitDescription.FUId;
    Service & service = GetService(failoverUnitDescription.ServiceId);
    bool isOnEveryNode = service.ServiceDesc.OnEveryNode;
    auto itFU = failoverUnitTable_.find(fuId);

    if (isOnEveryNode)
    {
        failoverUnitDescription.ReplicaDifference = INT_MAX;
    }

    //if no scaling policy, or if scaling is not per partition, target is always equal to the one from service description
    if (!service.ServiceDesc.IsAutoScalingDefined || !service.ServiceDesc.AutoScalingPolicy.IsPartitionScaled())
    {
        failoverUnitDescription.TargetReplicaSetSize = service.ServiceDesc.TargetReplicaSetSize;
    }

    if (failoverUnitDescription.IsDeleted)
    {
        // delete existing failover unit
        ASSERT_IF(itFU == failoverUnitTable_.end(), "FailoverUnit to be deleted {0} doesn't exist", fuId);

        FailoverUnit const& failoverUnit = itFU->second;

        DeleteNodeLoad(service, failoverUnit, failoverUnit.FuDescription.Replicas);
        UpdateApplicationNodeCounts(service.ServiceDesc.ApplicationId, failoverUnit.FuDescription.Replicas, false);
        UpdateServicePackagePlacement(service, failoverUnit.FuDescription.Replicas, false);
        UpdateNodeToFailoverUnitMapping(failoverUnit, failoverUnit.FuDescription.Replicas, failoverUnitDescription.Replicas, service.ServiceDesc.IsStateful);

        autoScaler_.RemoveFailoverUnit(fuId, failoverUnit.NextScalingCheck);
        service.OnFailoverUnitDeleted(failoverUnit, plb_.Settings);
        movePlan_.OnFailoverUnitDeleted(failoverUnit);

        RemoveFromPartitionStatistics(failoverUnit);
        RemoveFromReplicaDifferenceStatistics(fuId, failoverUnit.ActualReplicaDifference);
        RemoveFromUpgradeStatistics(failoverUnit);

        failoverUnitTable_.erase(itFU);

        // Erase the partition from the application partition map
        RemoveApplicationPartitions(service.ServiceDesc.ApplicationId, fuId);

        if (traceDetail)
        {
            plb_.Trace.UpdateFailoverUnitDeleted(fuId);
        }

        scheduler_.OnFailoverUnitDeleted(timeStamp, fuId);

        if (!fullConstraintCheck_)
        {
            constraintCheckClosure_.erase(fuId);
        }

        ret = true;
    }
    else if (itFU == failoverUnitTable_.end())
    {
        // insert new FailoverUnit
        vector<uint> primaryLoads = service.GetDefaultLoads(ReplicaRole::Primary);
        vector<uint> secondaryLoads = service.GetDefaultLoads(ReplicaRole::Secondary);
        uint primaryDefaultMoveCost = service.GetDefaultMoveCost(ReplicaRole::Primary);
        uint secondaryDefaultMoveCost = service.GetDefaultMoveCost(ReplicaRole::Secondary);

        StopwatchTime nextScalingTime = StopwatchTime::Zero;
        if (service.ServiceDesc.IsAutoScalingDefined)
        {
            if (service.ServiceDesc.AutoScalingPolicy.IsPartitionScaled())
            {
                nextScalingTime = timeStamp + service.ServiceDesc.AutoScalingPolicy.GetScalingInterval();
            }
        }

        AddFailoverUnit(FailoverUnit(move(failoverUnitDescription), move(primaryLoads), move(secondaryLoads),
            std::map<Federation::NodeId, std::vector<uint>>(), primaryDefaultMoveCost, secondaryDefaultMoveCost, isOnEveryNode, std::map<Federation::NodeId, FailoverUnit::ResourceLoad>(), nextScalingTime), timeStamp);
        ret = true;
    }
    else
    {
        // update existing failover unit
        FailoverUnit & currentFU = itFU->second;
        FailoverUnitDescription const& currentDescription = currentFU.FuDescription;
        int newReplicaDiff = failoverUnitDescription.ReplicaDifference;
        currentFU.IsServiceOnEveryNode = isOnEveryNode;

        //notify the auto scaler that it can remove the pending entry as FM has acked the target change
        if (currentDescription.TargetReplicaSetSize != failoverUnitDescription.TargetReplicaSetSize)
        {
            autoScaler_.ProcessFailoverUnitTargetUpdate(currentFU.FUId, failoverUnitDescription.TargetReplicaSetSize);
        }

        ASSERT_IFNOT(currentDescription.FUId == failoverUnitDescription.FUId && currentDescription.ServiceName ==
            failoverUnitDescription.ServiceName, "Comparison between two different FailoverUnit");
        ASSERT_IFNOT(currentDescription.Version <= failoverUnitDescription.Version,
            "Invalid failover unit version {0}", failoverUnitDescription);

        bool versionChanged = currentDescription.Version != failoverUnitDescription.Version;
        bool replicaDiffChanged = currentFU.ActualReplicaDifference != newReplicaDiff;

        bool shouldInterrupt = failoverUnitDescription.ReplicaDifference > 0
            || currentDescription.PrimaryReplicaIndex != failoverUnitDescription.PrimaryReplicaIndex
            || currentDescription.Replicas.size() != failoverUnitDescription.Replicas.size();

        if (versionChanged || replicaDiffChanged)
        {
            service.DeleteNodeLoad(currentFU, currentDescription.Replicas, plb_.Settings);
            DeleteNodeLoad(service, currentFU, currentDescription.Replicas);
            UpdateApplicationNodeCounts(service.ServiceDesc.ApplicationId, currentDescription.Replicas, false);
            UpdateServicePackagePlacement(service, currentDescription.Replicas, false);

            // If the upgrade is done, it will be removed here
            RemoveFromUpgradeStatistics(currentFU);

            // If upgrade is started for this partition, it will be added
            // currentFU would be modified if necessary, so this should be called before AddNodeLoad
            AddToUpgradeStatistics(failoverUnitDescription);

            itFU->second.CleanupResourceUsage(failoverUnitDescription);
            // Update the secondaryEntriesMap of the current FailoverUnit description with the new FailoverUnit description replicas
            itFU->second.UpdateSecondaryLoadMap(failoverUnitDescription, plb_.Settings);

            service.AddNodeLoad(currentFU, failoverUnitDescription.Replicas, plb_.Settings);
            AddNodeLoad(service, currentFU, failoverUnitDescription.Replicas);
            UpdateApplicationNodeCounts(service.ServiceDesc.ApplicationId, failoverUnitDescription.Replicas, true);
            UpdateServicePackagePlacement(service, failoverUnitDescription.Replicas, true);

            UpdateNodeToFailoverUnitMapping(currentFU, currentDescription.Replicas, failoverUnitDescription.Replicas, service.ServiceDesc.IsStateful);

            service.OnFailoverUnitChanged(currentFU, failoverUnitDescription);
            movePlan_.OnFailoverUnitChanged(currentFU, failoverUnitDescription);

            RemoveFromPartitionStatistics(currentFU);

            itFU->second.UpdateDescription(move(failoverUnitDescription));

            AddToPartitionStatistics(currentFU);
            if (replicaDiffChanged)
            {
                RemoveFromReplicaDifferenceStatistics(fuId, currentFU.ActualReplicaDifference);
                itFU->second.UpdateActualReplicaDifference(newReplicaDiff);
                AddToReplicaDifferenceStatistics(fuId, newReplicaDiff);
            }

            ret = PLBConfig::GetConfig().InterruptBalancingForAllFailoverUnitUpdates || shouldInterrupt;

            if (traceDetail)
            {
                plb_.Trace.UpdateFailoverUnit(
                    fuId,
                    currentFU.FuDescription,
                    currentFU.ActualReplicaDifference,
                    ret);
            }

            scheduler_.OnFailoverUnitChanged(timeStamp, fuId);

            if (!fullConstraintCheck_)
            {
                constraintCheckClosure_.insert(fuId);
            }
        }
    }

    return ret;
}

void ServiceDomain::UpdateFailoverUnitWithMoves(FailoverUnitMovement const& movement)
{
    auto itFailoverUnit = failoverUnitTable_.find(movement.FailoverUnitId);

    if (itFailoverUnit != failoverUnitTable_.end())
    {
        for (auto it = movement.Actions.begin(); it != movement.Actions.end(); ++it)
        {
            if ((it->Action == FailoverUnitMovementType::MoveSecondary ||
                it->Action == FailoverUnitMovementType::MoveInstance ||
                it->Action == FailoverUnitMovementType::SwapPrimarySecondary) &&
                itFailoverUnit->second.HasSecondaryOnNode(it->SourceNode))
            {
                itFailoverUnit->second.UpdateSecondaryMoves(it->SourceNode, it->TargetNode);
            }
            else if (it->Action == FailoverUnitMovementType::SwapPrimarySecondary &&
                itFailoverUnit->second.HasSecondaryOnNode(it->TargetNode))
            {
                itFailoverUnit->second.UpdateSecondaryMoves(it->TargetNode, it->SourceNode);
            }
        }
    }
}


void ServiceDomain::UpdateLoadOrMoveCost(LoadOrMoveCostDescription && loadOrMoveCost, StopwatchTime timeStamp)
{
    bool isReset = loadOrMoveCost.IsReset;

    size_t updatedMetricCount = 0;

    Common::Guid fuId = loadOrMoveCost.FailoverUnitId;
    auto itFailoverUnit = failoverUnitTable_.find(fuId);

    if (itFailoverUnit == failoverUnitTable_.end())
    {
        plb_.Trace.IgnoreLoadOrMoveCostFUNotExist(fuId);
        return;
    }

    itFailoverUnit->second.UpdateCount++; //This counter that tracks the number of load reports
    FailoverUnit const& failoverUnit = itFailoverUnit->second;

    Service & service = GetService(failoverUnit.FuDescription.ServiceId);
    bool isSingletonReplicaService = failoverUnit.TargetReplicaSetSize == 1;

    // If service is singleton replica, align primary and secondary default loads
    // This is needed to avoid issues during singleton replica movement,
    // as first new secondary is created, and then promoted to primary
    if (isSingletonReplicaService &&
        loadOrMoveCost.IsStateful &&
        loadOrMoveCost.SecondaryEntries.size() > 0)
    {
        loadOrMoveCost.AlignSingletonReplicaLoads();
    }

    DeleteNodeLoad(service, failoverUnit, failoverUnit.FuDescription.Replicas);
    service.DeleteNodeLoad(failoverUnit, failoverUnit.FuDescription.Replicas, plb_.Settings);

    UpdateServicePackagePlacement(service, failoverUnit.FuDescription.Replicas, false);

    if (loadOrMoveCost.IsStateful)
    {
        // Update the primary entries
        for (auto it = loadOrMoveCost.PrimaryEntries.begin(); it != loadOrMoveCost.PrimaryEntries.end(); ++it)
        {
            if (it->Name == *Constants::MoveCostMetricName)
            {
                if (itFailoverUnit->second.UpdateMoveCost(ReplicaRole::Primary, it->Value))
                {
                    plb_.Trace.UpdateMoveCost(fuId, ReplicaRole::ToString(ReplicaRole::Primary), it->Value);
                    ++updatedMetricCount;
                }
                else if (!itFailoverUnit->second.IsMoveCostValid(it->Value))
                {
                    plb_.Trace.IgnoreInvalidMoveCost(fuId, ReplicaRole::ToString(ReplicaRole::Primary), it->Value);
                }
            }
            else if (it->Name == *ServiceModel::Constants::SystemMetricNameCpuCores || it->Name == *ServiceModel::Constants::SystemMetricNameMemoryInMB)
            {
                // Primary replica might have crashed and doesn't exist
                if (failoverUnit.FuDescription.PrimaryReplica)
                {
                    if (itFailoverUnit->second.UpdateResourceLoad(it->Name, it->Value, (failoverUnit.FuDescription.PrimaryReplica)->NodeId))
                    {
                        plb_.Trace.UpdateLoadOnNode(fuId, ReplicaRole::ToString(ReplicaRole::Primary), it->Name, it->Value, (failoverUnit.FuDescription.PrimaryReplica)->NodeId);
                    }
                }
                else
                {
                    // Trace if primary is not found
                    plb_.Trace.PrimaryNodeNotFound(fuId);
                }
            }
            else
            {
                int index = service.GetCustomLoadIndex(it->Name);
                if (index < 0)
                {
                    plb_.Trace.IgnoreLoadInvalidMetric(fuId, ReplicaRole::ToString(ReplicaRole::Primary), it->Name, it->Value);
                }
                else if (itFailoverUnit->second.UpdateLoad(ReplicaRole::Primary, static_cast<size_t>(index), it->Value, plb_.Settings, false, Federation::NodeId(), isReset, isSingletonReplicaService))
                {
                    if  (failoverUnit.FuDescription.PrimaryReplica)
                    {
                        plb_.Trace.UpdateLoadOnNode(fuId, ReplicaRole::ToString(ReplicaRole::Primary), it->Name, it->Value, (failoverUnit.FuDescription.PrimaryReplica)->NodeId);
                    }
                    else
                    {
                        plb_.Trace.PrimaryNodeNotFound(fuId);
                        plb_.Trace.UpdateLoad(fuId, ReplicaRole::ToString(ReplicaRole::Primary), it->Name, it->Value);
                    }

                    ++updatedMetricCount;
                }
            }
        }
    }

    if (!plb_.Settings.UseSeparateSecondaryLoad)
    {
        // If use separate loads, the secondary entry shouldn't be updated
        // Replica should use default load if they didn't do reportload
        for (auto it = loadOrMoveCost.SecondaryEntries.begin(); it != loadOrMoveCost.SecondaryEntries.end(); ++it)
        {
            if (it->Name == *Constants::MoveCostMetricName)
            {
                if (itFailoverUnit->second.UpdateMoveCost(ReplicaRole::Secondary, it->Value))
                {
                    plb_.Trace.UpdateMoveCost(fuId, ReplicaRole::ToString(ReplicaRole::Secondary), it->Value);
                    ++updatedMetricCount;
                }
                else if (!itFailoverUnit->second.IsMoveCostValid(it->Value))
                {
                    plb_.Trace.IgnoreInvalidMoveCost(fuId, ReplicaRole::ToString(ReplicaRole::Secondary), it->Value);
                }
            }
            else
            {
                int index = service.GetCustomLoadIndex(it->Name);
                if (index < 0)
                {
                    plb_.Trace.IgnoreLoadInvalidMetric(fuId, ReplicaRole::ToString(ReplicaRole::Secondary), it->Name, it->Value);
                }
                else if (itFailoverUnit->second.UpdateLoad(ReplicaRole::Secondary, static_cast<size_t>(index), it->Value, plb_.Settings, false, Federation::NodeId(), isReset))
                {
                    plb_.Trace.UpdateLoad(fuId, ReplicaRole::ToString(ReplicaRole::Secondary), it->Name, it->Value);
                    ++updatedMetricCount;
                }
            }
        }
    }

    if (loadOrMoveCost.SecondaryEntriesMap.empty())
    {
        // Empty load update indicate it is a load reset
        for (auto it = loadOrMoveCost.SecondaryEntries.begin(); it != loadOrMoveCost.SecondaryEntries.end(); ++it)
        {
            int index = service.GetCustomLoadIndex(it->Name);
            if (index < 0)
            {
                plb_.Trace.IgnoreLoadInvalidMetric(fuId, ReplicaRole::ToString(ReplicaRole::Secondary), it->Name, it->Value);
            }
            else if (itFailoverUnit->second.UpdateLoad(ReplicaRole::Secondary, static_cast<size_t>(index), it->Value, plb_.Settings, false, Federation::NodeId(), isReset))
            {
                plb_.Trace.UpdateLoad(fuId, ReplicaRole::ToString(ReplicaRole::Secondary), it->Name, it->Value);
                ++updatedMetricCount;
            }
        }
    }
    else
    {
        for (auto mapIt = loadOrMoveCost.SecondaryEntriesMap.begin(); mapIt != loadOrMoveCost.SecondaryEntriesMap.end(); ++mapIt)
        {
            for (auto it = mapIt->second.begin(); it != mapIt->second.end(); ++it)
            {
                if (it->Name == *Constants::MoveCostMetricName)
                {
                    if (itFailoverUnit->second.UpdateMoveCost(ReplicaRole::Secondary, it->Value))
                    {
                        plb_.Trace.UpdateMoveCost(fuId, ReplicaRole::ToString(ReplicaRole::Secondary), it->Value);
                        ++updatedMetricCount;
                    }
                    else if (!itFailoverUnit->second.IsMoveCostValid(it->Value))
                    {
                        plb_.Trace.IgnoreInvalidMoveCost(fuId, ReplicaRole::ToString(ReplicaRole::Secondary), it->Value);
                    }
                }
                else if (it->Name == *ServiceModel::Constants::SystemMetricNameCpuCores || it->Name == *ServiceModel::Constants::SystemMetricNameMemoryInMB)
                {
                    if (itFailoverUnit->second.UpdateResourceLoad(it->Name, it->Value, mapIt->first))
                    {
                        plb_.Trace.UpdateLoadOnNode(fuId, ReplicaRole::ToString(ReplicaRole::Secondary), it->Name, it->Value, mapIt->first);
                    }
                }
                else
                {
                    int index = service.GetCustomLoadIndex(it->Name);
                    if (index < 0)
                    {
                        plb_.Trace.IgnoreLoadInvalidMetricOnNode(fuId, ReplicaRole::ToString(ReplicaRole::Secondary), it->Name, it->Value, mapIt->first);
                    }
                    else if (itFailoverUnit->second.UpdateLoad(ReplicaRole::Secondary, static_cast<size_t>(index),
                        it->Value, plb_.Settings, true, mapIt->first, isReset))
                    {
                        plb_.Trace.UpdateLoadOnNode(fuId, ReplicaRole::ToString(ReplicaRole::Secondary), it->Name, it->Value, mapIt->first);
                        ++updatedMetricCount;
                    }
                }
            }
        }
    }

    // when updating load and metric costs, number of replicas are not updated
    // so need to pass flag true -> that helps calculating reservation load for applications when adding new replicas on node
    AddNodeLoad(service, failoverUnit, failoverUnit.FuDescription.Replicas, true);
    service.AddNodeLoad(failoverUnit, failoverUnit.FuDescription.Replicas, plb_.Settings);
    UpdateServicePackagePlacement(service, failoverUnit.FuDescription.Replicas, true);

    if (updatedMetricCount > 0)
    {
        scheduler_.OnLoadChanged(timeStamp);
        movePlan_.OnLoadChanged();
        if (!fullConstraintCheck_)
        {
            constraintCheckClosure_.insert(fuId);
        }
    }
}

void ServiceDomain::SetMovementEnabled(bool constraintCheckEnabled, bool balancingEnabled, bool isDummyPLB, StopwatchTime timeStamp)
{
    scheduler_.SetConstraintCheckEnabled(constraintCheckEnabled, timeStamp);
    if (!isDummyPLB)
    {
        scheduler_.SetBalancingEnabled(balancingEnabled && !plb_.IsMaster, timeStamp);
    }

    if (!constraintCheckEnabled)
    {
        movePlan_.OnConstraintCheckDisabled();
    }

    if (!balancingEnabled && !isDummyPLB)
    {
        movePlan_.OnBalancingDisabled();
    }
}

bool ServiceDomain::CheckReservationViolation(
    Node const& node,
    FailoverUnit const& currentFUnit,
    wstring const& metricName,
    int64 loadChangeNode,
    int64 currentNodeLoad,
    uint currentNodeCapacity) const
{
    auto itReservationLoad = reservationLoadTable_.find(metricName);
    if (itReservationLoad != reservationLoadTable_.end())
    {
        // all applications reservations on that node, including the current one if exists
        int64  sumDifferentAppReservationOnNode = itReservationLoad->second.GetNodeReservedLoadUsed(node.NodeDescriptionObj.NodeId);

        // current application Id
        uint64 currentFUAppId = GetService(currentFUnit.FuDescription.ServiceId).ServiceDesc.ApplicationId;
        Application const * app = plb_.GetApplicationPtrCallerHoldsLock(currentFUAppId);

        if (currentFUAppId != 0 && nullptr != app && app->ApplicationDesc.HasCapacity())
        {
            // current application reservation on single node
            auto const& applicationCapacities = app->ApplicationDesc.AppCapacities;
            auto const& appCapacityIt = applicationCapacities.find(metricName);

            int64 reservationCapacity = 0;
            if (appCapacityIt != applicationCapacities.end())
            {
                reservationCapacity = appCapacityIt->second.ReservationCapacity;
            }

            // sum of all replicas load for that application on this node
            int64 currAppReplicasLoad = 0;
            ApplicationLoad const& appLoad = GetApplicationLoad(currentFUAppId);
            auto const& metricLoadIt = appLoad.MetricLoads.find(metricName);
            if (metricLoadIt != appLoad.MetricLoads.end())
            {
                currAppReplicasLoad = metricLoadIt->second.GetLoad(node.NodeDescriptionObj.NodeId);
            }

            // remaining reservation for current application on this node
            int64 remainingReservation = reservationCapacity - currAppReplicasLoad;
            if (remainingReservation < 0)
            {
                // if loaded more than it is supposed to be reserved, do not care about reservation
                remainingReservation = 0;
            }
            else
            {
                sumDifferentAppReservationOnNode -= remainingReservation;

                int64 diffLoad = loadChangeNode;
                if (remainingReservation >= loadChangeNode)
                {
                    remainingReservation -= loadChangeNode;

                }
                else
                {
                    diffLoad -= remainingReservation;
                    remainingReservation = 0;
                }
                if (currentNodeCapacity < currentNodeLoad + diffLoad + remainingReservation)
                {
                    return true;
                }
            }
        }

        if (currentNodeCapacity < currentNodeLoad + loadChangeNode + sumDifferentAppReservationOnNode)
        {
            return true;
        }
    }

    return false;
}

int ServiceDomain::CompareNodeForPromotion(Guid fuId, Node const& node1, Node const& node2) const
{
    FailoverUnit const& failoverUnit = GetFailoverUnit(fuId);
    Service const& service = GetService(failoverUnit.FuDescription.ServiceId);
    ASSERT_IF(!service.ServiceDesc.IsStateful, "Failover unit {0} is not stateful when promoting secondary.", fuId);

    if (   !service.ServiceDesc.AffinitizedService.empty()
        && plb_.ServiceIdMap.find(service.ServiceDesc.AffinitizedService) != plb_.ServiceIdMap.end())
    {
        auto itAffinitizedService = serviceTable_.find(plb_.ServiceIdMap.find(service.ServiceDesc.AffinitizedService)->second);
        // In case of aligned affinity, collocate primary with the primary of the parent if possible.
        if (itAffinitizedService != serviceTable_.end() && service.ServiceDesc.AlignedAffinity)
        {
            // find the root service, TODO: find the first placed root service
            while (!itAffinitizedService->second.ServiceDesc.AffinitizedService.empty())
            {
                Service const& affinitizedService = GetService(itAffinitizedService->second.ServiceDesc.AffinitizedService);
                auto it = serviceTable_.find(affinitizedService.ServiceDesc.ServiceId);
                if (it == serviceTable_.end())
                {
                    break;
                }
                itAffinitizedService = it;
            }

            if (itAffinitizedService->second.ActualPartitionCount > 0)
            {
                ASSERT_IFNOT(itAffinitizedService->second.ActualPartitionCount == 1, "Affinitized service should have only 1 partition");
                FailoverUnit const& parentFailoverUnit = GetFailoverUnit(itAffinitizedService->second.FailoverUnitId);

                ReplicaDescription const* parentPrimaryReplica = parentFailoverUnit.FuDescription.PrimaryReplica;

                if (parentPrimaryReplica != nullptr)
                {
                    if (node1.NodeDescriptionObj.NodeId == parentPrimaryReplica->NodeId)
                    {
                        return -1;
                    }
                    else if (node2.NodeDescriptionObj.NodeId == parentPrimaryReplica->NodeId)
                    {
                        return 1;
                    }
                }
            }
        }
    }

    // moving parent service will also consider dependent services
    vector<uint> childPrimaryLoadSum;

    auto itParent = childServices_.find(service.ServiceDesc.Name);

    if (itParent != childServices_.end())
    {
        for (auto itChild = itParent->second.begin(); itChild != itParent->second.end(); ++itChild)
        {
            Service const& childService = GetService(*itChild);

            auto childFailoverunitPtr = failoverUnitTable_.find(childService.FailoverUnitId);

            if (childFailoverunitPtr == failoverUnitTable_.end())
            {
                continue;
            }

            FailoverUnit const& childFailoverunit = childFailoverunitPtr->second;

            vector<uint> const& childPrimaryEntries = childFailoverunit.PrimaryEntries;
            bool includeChild = true;
            //include child partitions that are not in node1 or node2, TODO: consider replica on node1 or node2 for loadChange calculation
            for (auto replicaIter = childFailoverunit.FuDescription.Replicas.begin();
                replicaIter != childFailoverunit.FuDescription.Replicas.end(); ++replicaIter)
            {
                if (replicaIter->NodeId == node1.NodeDescriptionObj.NodeId || replicaIter->NodeId == node2.NodeDescriptionObj.NodeId)
                {
                    includeChild = false;
                    break;
                }
            }

            if (includeChild)
            {
                if (childPrimaryLoadSum.empty())
                {
                    childPrimaryLoadSum = childPrimaryEntries;
                }
                else
                {
                    for (uint i = 0; i < childPrimaryEntries.size(); ++i)
                    {
                        childPrimaryLoadSum[i] += childPrimaryEntries[i];
                    }
                }
            }
        }
    }

    bool node1ContainCapacity = !node1.NodeDescriptionObj.Capacities.empty();
    bool node2ContainCapacity = !node2.NodeDescriptionObj.Capacities.empty();

    // Check if application has capacity
    Application const * app = plb_.GetApplicationPtrCallerHoldsLock(service.ServiceDesc.ApplicationId);
    bool hasApplicationCapacity = false;

    if (nullptr != app)
    {
        if (app->ApplicationDesc.HasCapacity())
        {
            hasApplicationCapacity = true;
        }
    }

    bool node1ViolateCapacity = false, node2ViolateCapacity = false;
    double score1 = 0, score2 = 0;
    uint nodeCount = static_cast<uint>(plb_.nodes_.size());
    ASSERT_IFNOT(nodeCount > 0, "No existing node");
    for (size_t metricIndex = 0; metricIndex < service.ServiceDesc.Metrics.size(); ++metricIndex)
    {
        wstring const& metricName = service.ServiceDesc.Metrics[metricIndex].Name;
        auto itDomainMetric = metricTable_.find(metricName);
        ASSERT_IF(itDomainMetric == metricTable_.end(), "Metric {0} doesn't exist", metricName);

        int64 loadChangePrimary = static_cast<int64>(failoverUnit.PrimaryEntries[metricIndex]);
        //add possible load change because child will move to parent in the future
        if (childPrimaryLoadSum.size() > metricIndex)
        {
            loadChangePrimary += static_cast<int64>(childPrimaryLoadSum[metricIndex]);
        }

        int64 loadChangeNode1 = loadChangePrimary - static_cast<int64>(failoverUnit.GetSecondaryLoad(metricIndex,
            node1.NodeDescriptionObj.NodeId, plb_.Settings));
        int64 loadChangeNode2 = loadChangePrimary - static_cast<int64>(failoverUnit.GetSecondaryLoad(metricIndex,
            node2.NodeDescriptionObj.NodeId, plb_.Settings));

        // capacity check for node1 and node2
        // As long as one of them is not positive, the check should be skipped
        if (loadChangeNode1 > 0 && loadChangeNode2 > 0 && (node1ContainCapacity || node2ContainCapacity))
        {
            if (node1ContainCapacity && !node1ViolateCapacity)
            {
                auto itCapacity1 = node1.NodeDescriptionObj.Capacities.find(metricName);
                if (itCapacity1 != node1.NodeDescriptionObj.Capacities.end())
                {
                    if (itCapacity1->second < itDomainMetric->second.GetLoad(node1.NodeDescriptionObj.NodeId) + loadChangeNode1)
                    {
                        node1ViolateCapacity = true;
                    }

                    // check reservations for the applications on the nodes
                    if (!node1ViolateCapacity &&
                        CheckReservationViolation(
                        node1,
                        failoverUnit,
                        metricName,
                        loadChangeNode1,
                        itDomainMetric->second.GetLoad(node1.NodeDescriptionObj.NodeId),
                        itCapacity1->second))
                    {
                        node1ViolateCapacity = true;
                    }
                }

            }

            if (node2ContainCapacity && !node2ViolateCapacity)
            {
                auto itCapacity2 = node2.NodeDescriptionObj.Capacities.find(metricName);
                if (itCapacity2 != node2.NodeDescriptionObj.Capacities.end())
                {
                    if (itCapacity2->second < itDomainMetric->second.GetLoad(node2.NodeDescriptionObj.NodeId) + loadChangeNode2)
                    {
                        node2ViolateCapacity = true;
                    }

                    // check reservations for the applications on the nodes
                    if (!node2ViolateCapacity &&
                        CheckReservationViolation(
                        node2,
                        failoverUnit,
                        metricName,
                        loadChangeNode2,
                        itDomainMetric->second.GetLoad(node2.NodeDescriptionObj.NodeId),
                        itCapacity2->second))
                    {
                        node2ViolateCapacity = true;
                    }
                }

            }
        }

        if (app != nullptr && hasApplicationCapacity && (loadChangeNode1 != 0 || loadChangeNode2 != 0))
        {
            auto const& applicationCapacities = app->ApplicationDesc.AppCapacities;
            auto const& appCapacityIt = applicationCapacities.find(metricName);
            if (appCapacityIt != applicationCapacities.end() && ApplicationHasLoad(app->ApplicationDesc.ApplicationId))
            {
                auto applicationCapacity = appCapacityIt->second.MaxInstanceCapacity;
                auto const& metricLoads = GetApplicationLoad(service.ServiceDesc.ApplicationId).MetricLoads;
                auto const& metricLoadIt = metricLoads.find(metricName);

                if (metricLoadIt != metricLoads.end())
                {
                    if (loadChangeNode1 != 0
                        && loadChangeNode1 + metricLoadIt->second.GetLoad(node1.NodeDescriptionObj.NodeId) > applicationCapacity)
                    {
                        node1ViolateCapacity = true;
                    }

                    if (loadChangeNode2 != 0
                        && loadChangeNode2 + metricLoadIt->second.GetLoad(node2.NodeDescriptionObj.NodeId) > applicationCapacity)
                    {
                        node2ViolateCapacity = true;
                    }
                }
            }
        }

        if (node1ViolateCapacity && node2ViolateCapacity)
        {
            return 0;
        }

        if (loadChangeNode1 != 0 || loadChangeNode2 != 0)
        {
            double averageLoadOld = itDomainMetric->second.NodeLoadSum / static_cast<double>(nodeCount);
            double averageLoadNew1 = (itDomainMetric->second.NodeLoadSum + loadChangeNode1) / static_cast<double>(nodeCount);
            double averageLoadNew2 = (itDomainMetric->second.NodeLoadSum + loadChangeNode2) / static_cast<double>(nodeCount);

            int64 currentLoad1 = itDomainMetric->second.GetLoad(node1.NodeDescriptionObj.NodeId);
            int64 currentLoad2 = itDomainMetric->second.GetLoad(node2.NodeDescriptionObj.NodeId);

            score1 += abs(currentLoad1 + loadChangeNode1 - averageLoadNew1) - abs(currentLoad1 - averageLoadOld);
            score2 += abs(currentLoad2 + loadChangeNode2 - averageLoadNew2) - abs(currentLoad2 - averageLoadOld);
        }
    }

    if (node1ViolateCapacity)
    {
        return 1;
    }
    else if (node2ViolateCapacity)
    {
        return -1;
    }
    else if (score1 == score2)
    {
        return 0;
    }
    else if (score1 < score2)
    {
        return -1;
    }
    else
    {
        return 1;
    }
}

void ServiceDomain::OnNodeUp(uint64 node, StopwatchTime timeStamp)
{
    // exclusive lock acquired at upper level

    changedNodes_.insert(node);
    scheduler_.OnNodeUp(timeStamp);
    movePlan_.OnNodeUp();
    fullConstraintCheck_ = true;
}

void ServiceDomain::OnNodeDown(uint64 node, StopwatchTime timeStamp)
{
    // exclusive lock acquired at upper level

    changedNodes_.insert(node);
    scheduler_.OnNodeDown(timeStamp);
    movePlan_.OnNodeDown();
    fullConstraintCheck_ = true;
}

void ServiceDomain::OnNodeChanged(uint64 node, StopwatchTime timeStamp)
{
    // exclusive lock acquired at upper level

    changedNodes_.insert(node);
    scheduler_.OnNodeChanged(timeStamp);
    movePlan_.OnNodeChanged();
    fullConstraintCheck_ = true;
}

void ServiceDomain::OnServiceTypeChanged(std::wstring const& serviceTypeName)
{
    // exclusive lock acquired at upper level

    changedServiceTypes_.insert(serviceTypeName);
    scheduler_.OnServiceTypeChanged(Stopwatch::Now());
    movePlan_.OnServiceTypeChanged();
    fullConstraintCheck_ = true;
}

void ServiceDomain::UpdateFailoverUnitWithCreationMoves(vector<Common::Guid> & partitionsWithCreations)
{
    if (!partitionsWithCreations.empty() && (scheduler_.CurrentAction.IsCreation() || scheduler_.CurrentAction.IsCreationWithMove()))
    {
        for (auto it = partitionsWithCreations.begin(); it != partitionsWithCreations.end(); ++it)
        {
            auto itFt = failoverUnitTable_.find(*it);
            if (itFt != failoverUnitTable_.end() && itFt->second.ActualReplicaDifference > 0 && itFt->second.ActualReplicaDifference != INT_MAX)
            {
                RemoveFromReplicaDifferenceStatistics(*it, itFt->second.ActualReplicaDifference);
                itFt->second.UpdateActualReplicaDifference(0);
            }
        }

        partitionsWithCreations.clear();
    }
}

void ServiceDomain::OnMovementGenerated(StopwatchTime timeStamp, Common::Guid decisionGuid, double newAvgStdDev, FailoverUnitMovementTable && movementList)
{
    bool isCreation = (scheduler_.CurrentAction.Action == PLBSchedulerActionType::NewReplicaPlacement ||
        scheduler_.CurrentAction.Action == PLBSchedulerActionType::NewReplicaPlacementWithMove);

    for (auto it = movementList.begin(); it != movementList.end(); )
    {
        auto itFailoverUnit = failoverUnitTable_.find(it->first);
        FailoverUnitMovement const& movement = it->second;

        if (itFailoverUnit != failoverUnitTable_.end())
        {
            if (it->second.Version != itFailoverUnit->second.FuDescription.Version)
            {
                 //If the failover unit is updated, remove the movement
                 //FM is already going to drop the FT, so no point in keeping it here
                for (auto itPlbAction = it->second.Actions.begin(); itPlbAction != it->second.Actions.end(); ++itPlbAction)
                {
                    plb_.PublicTrace.OperationDiscarded(
                        it->second.FailoverUnitId,
                        decisionGuid,
                        scheduler_.CurrentAction,
                        StringUtility::ToWString(itPlbAction->Action),
                        it->second.ServiceName,
                        itPlbAction->SourceNode,
                        itPlbAction->TargetNode);
                }
                
                it = movementList.erase(it);
                continue;
            }

            if (isCreation && itFailoverUnit->second.ActualReplicaDifference > 0 && itFailoverUnit->second.ActualReplicaDifference != INT_MAX)
            {
                // assume constraint check and LB will not generate creations
                RemoveFromReplicaDifferenceStatistics(it->first, itFailoverUnit->second.ActualReplicaDifference);
                itFailoverUnit->second.UpdateActualReplicaDifference(0);
            }

            UpdateFailoverUnitWithMoves(movement);
        }

        ++it;
    }

    scheduler_.OnMovementGenerated(timeStamp, newAvgStdDev, movementList);
    movePlan_.OnMovementGenerated(move(movementList), timeStamp);
}

void ServiceDomain::OnDomainInterrupted(StopwatchTime timeStamp)
{
    // If we were doing balancing, notify the scheduler that balancing was interrupted
    if (scheduler_.CurrentAction.IsBalancing())
    {
        scheduler_.OnDomainInterrupted(timeStamp);
    }
}

FailoverUnitMovementTable ServiceDomain::GetMovements(StopwatchTime now)
{
    return movePlan_.GetMovements(now);
}

ServiceDomain::DomainData ServiceDomain::TakeSnapshot(PLBDiagnosticsSPtr const& plbDiagnosticsSPtr)
{
    // No partitions or replicas are neeeded for most of the queries.
    // If we need full information in snapshot it is enough to call IsConstraintSatisfied() to populate it.
    PartitionClosureType::Enum closureType = PartitionClosureType::None;

    SystemState systemState(*this, plb_.Trace, plbDiagnosticsSPtr);
    systemState.CreatePlacementAndChecker(closureType);

    return DomainData(DomainId(domainId_), scheduler_.CurrentAction, move(systemState), FailoverUnitMovementTable());
}

void ServiceDomain::TraceNodeLoads(StopwatchTime const& now)
{
    if (plb_.nodes_.size() > 0)
    {
        uint64 lastNodeLoadsTraceNodeId(0);
        uint iteration(0);

        do
        {
            iteration++;
            plb_.Trace.NodeLoads(
                domainId_,
                TraceLoadsVector<uint64, Node, ServiceDomainMetric, vector<Node>, map<wstring, ServiceDomainMetric>>(
                    plb_.nodes_,
                    metricTable_,
                    iteration,
                    &lastNodeLoadsTraceNodeId,
                    uint(PLBConfig::GetConfig().PLBNodeLoadTraceEntryCount / metricTable_.size()),
                    true)
                );
        } while (lastNodeLoadsTraceNodeId != 0 && iteration < plb_.nodes_.size());

        lastNodeLoadsTrace_ = now;
    }
}

ServiceDomain::DomainData ServiceDomain::RefreshStates(StopwatchTime now, PLBDiagnosticsSPtr const& plbDiagnosticsSPtr)
{
    // Reset cache of placement constraints and block lists. This happens on each refresh so in case of tow successive updates
    // in two different refreshes we will recalculate the block lists correctly.
    lastEvaluatedPlacementConstraint_ = L"";
    lastEvaluatedServiceType_ = L"";
    lastEvaluatedServiceBlockList_ = DynamicBitSet();
    lastEvaluatedPrimaryReplicaBlockList_ = DynamicBitSet();
    lastEvaluatedOverallBlocklist_ = DynamicBitSet();
    lastEvaluatedPartialPlacement_ = PLBConfig::GetConfig().PartiallyPlaceServices;
    lastEvaluatedFDDistributionPolicy_ = Service::Type::Enum::Packing;

    if (!changedNodes_.empty() || !changedServiceTypes_.empty())
    {
        for (auto itService = serviceTable_.begin(); itService != serviceTable_.end(); ++itService)
        {
            UpdateServiceBlockList(itService->second);
        }
    }
    else
    {
        for (auto it = changedServices_.begin(); it != changedServices_.end(); ++it)
        {
            auto itService = serviceTable_.find(*it);
            if (itService != serviceTable_.end())
            {
                UpdateServiceBlockList(itService->second);
            }
        }
    }

    changedNodes_.clear();
    changedServiceTypes_.clear();
    changedServices_.clear();

    if (now - lastNodeLoadsTrace_ >= PLBConfig::GetConfig().NodeLoadsTracingInterval)
    {
        TraceNodeLoads(now);
    }

    if (now - lastApplicationLoadsTrace_ >= PLBConfig::GetConfig().ApplicationLoadsTracingInterval)
    {
        if (plb_.applicationTable_.size() > 0)
        {
            uint totalMaxEntries = PLBConfig::GetConfig().PLBApplicationLoadTraceMaxSize;
            uint maxEntriesPerTrace = PLBConfig::GetConfig().PLBApplicationLoadTraceBatchSize;
            //this is in case we got stuck with an invalid application to start with between PLB runs...
            if (plb_.applicationTable_.find(lastTracedApplication_) == plb_.applicationTable_.end())
            {
                TESTASSERT_IF(totalMaxEntries > 0, "Unexpected Application Delete: {0}", lastTracedApplication_);
                lastTracedApplication_ = plb_.applicationTable_.begin()->first;
            }
            //if we loop around or reset, we come back to the zeroth trace...
            if (lastTracedApplication_ == plb_.applicationTable_.begin()->first)
            {
                applicationTraceIterationNumber_ = 0;
            }
            do
            {
                applicationTraceIterationNumber_++;
                plb_.Trace.ApplicationLoads(
                    domainId_,
                    TraceLoads<uint64, Application, ApplicationLoad, Uint64UnorderedMap<Application>, Uint64UnorderedMap<ApplicationLoad>>(
                        plb_.applicationTable_,
                        applicationLoadTable_,
                        applicationTraceIterationNumber_,
                        &lastTracedApplication_,
                        min(totalMaxEntries,maxEntriesPerTrace)
                    )
                );
                totalMaxEntries = totalMaxEntries > maxEntriesPerTrace ? totalMaxEntries - maxEntriesPerTrace : 0;
            }
            while (lastTracedApplication_ != plb_.applicationTable_.begin()->first &&
                applicationTraceIterationNumber_ < plb_.applicationTable_.size() && totalMaxEntries != 0);

            lastApplicationLoadsTrace_ = now;
        }
    }

    SystemState systemState(*this, plb_.Trace, plbDiagnosticsSPtr);

    movePlan_.Refresh(now);

    plb_.LoadBalancingCounters->NumberOfPLBPendingMovements.Value +=
        static_cast<Common::PerformanceCounterValue>(movePlan_.NumberOfPendingMovements);

    PLBSchedulerAction action;
    bool const movePlanExists = !movePlan_.IsEmpty;
    SchedulingDiagnostics::ServiceDomainSchedulerStageDiagnosticsSPtr stageDiagnosticsSPtr = make_shared<ServiceDomainSchedulerStageDiagnostics>();

    if (movePlanExists)
    {
        action.IsSkip = true;

        //Diagnostics
        {
            stageDiagnosticsSPtr->skipRefreshSchedule_ = true;
            stageDiagnosticsSPtr->stage_ = SchedulerStage::Stage::Skip;
        }
    }
    else
    {

        scheduler_.RefreshAction(systemState, now, stageDiagnosticsSPtr);
        systemState.CreatePlacementAndChecker(scheduler_.CurrentAction.Action);
    }

    //Diagnostics
    stageDiagnosticsSPtr->metricString_ = plbDiagnosticsSPtr->ServiceDomainMetricsString(domainId_, false);
    stageDiagnosticsSPtr->metricProperty_ = plbDiagnosticsSPtr->ServiceDomainMetricsString(domainId_, true);
    plbDiagnosticsSPtr->SchedulersDiagnostics->AddStageDiagnostics(domainId_, stageDiagnosticsSPtr);

    return DomainData(DomainId(domainId_), (movePlanExists ? action : scheduler_.CurrentAction), move(systemState), FailoverUnitMovementTable());
}

BalanceCheckerUPtr ServiceDomain::GetBalanceChecker(PartitionClosureUPtr const& partitionClosure) const
{
    if (plb_.Settings.IsTestMode)
    {
        VerifyNodeLoads(false);
        VerifyNodeLoads(true);
    }

    BalancingDiagnosticsDataSPtr balancingDiagnosticsDataSPtr = plb_.PlbDiagnosticsSPtr->SchedulersDiagnostics->AddOrGetSDBalancingDiagnostics(domainId_, true);
    balancingDiagnosticsDataSPtr->plbNodesPtr_ = &(plb_.PlbDiagnosticsSPtr->PLBNodes);
    balancingDiagnosticsDataSPtr->plbNodeToIndexMap_ = &(plb_.nodeToIndexMap_);
    BalanceCheckerCreator creator(*partitionClosure, plb_.nodes_, metricTable_, balancingDiagnosticsDataSPtr, plb_.upgradeCompletedUDs_, plb_.Settings);

    return creator.Create();
}

PlacementUPtr ServiceDomain::GetPlacement(PartitionClosureUPtr const& partitionClosure,
    BalanceCheckerUPtr && balanceChecker, set<Guid> && throttledPartitions) const
{
    PlacementCreator creator(plb_, *this, move(balanceChecker), move(throttledPartitions), partitionClosure);

    return creator.Create(plb_.randomSeed_);
}

void ServiceDomain::UpdateContraintCheckClosure(set<Guid> && partitions)
{
    constraintCheckClosure_ = move(partitions);
    fullConstraintCheck_ = false;
}

//------------------------------------------------------------
// private members
//------------------------------------------------------------

void ServiceDomain::UpdateReservationsServicesNoMetric(
    uint64 appId,
    std::set<std::wstring> &checkedMetrics,
    vector<ReplicaDescription> const& replicas,
    bool isAdd,
    bool decrementNodeCounts)
{
    auto const& appCapacities = plb_.GetApplication(appId).ApplicationDesc.AppCapacities;
    for (auto itCapacity = appCapacities.begin(); itCapacity != appCapacities.end(); itCapacity++)
    {
        // for metrics which aren't checked
        wstring metricName = itCapacity->first;
        if (checkedMetrics.find(metricName) == checkedMetrics.end())
        {
            int64 reservationCapacity = 0;
            auto const& appCapacityIt = appCapacities.find(metricName);
            if (appCapacityIt != appCapacities.end())
            {
                reservationCapacity = appCapacityIt->second.ReservationCapacity;
            }

            for (auto itReplica = replicas.begin(); itReplica != replicas.end(); ++itReplica)
            {
                if (reservationCapacity > 0 &&
                    itReplica->CouldCarryReservationOrScaleout())
                {
                    ApplicationLoad& appLoad = GetApplicationLoad(appId);
                    size_t numReplicasOnNode = 0;
                    auto itNumReplicas = appLoad.NodeCounts.find(itReplica->NodeId);
                    if (itNumReplicas != appLoad.NodeCounts.end())
                    {
                        numReplicasOnNode = itNumReplicas->second;
                    }

                    if (decrementNodeCounts)
                    {
                        TESTASSERT_IF(numReplicasOnNode == 0,
                            "Replica count on node {0} cannot be zero for application id {1} when decrementing node counts.",
                            itReplica->NodeId,
                            appId);
                        --numReplicasOnNode;
                    }

                    if (isAdd)
                    {
                        // if this is the first replica on this node
                        if (numReplicasOnNode == 0)
                        {
                            GetReservationLoad(metricName).AddNodeReservedLoadForAllApps(itReplica->NodeId, reservationCapacity);
                        }
                    }
                    else if (numReplicasOnNode == 1)
                    {
                        // remove reservation just for last replica without load
                        GetReservationLoad(metricName).DeleteNodeReservedLoadForAllApps(itReplica->NodeId, reservationCapacity);
                    }
                }
            }
        }
    }
}

inline void ServiceDomain::AddNodeLoad(Service const& service, FailoverUnit const& failoverUnit, vector<ReplicaDescription> const& replicas, bool isLoadOrMoveCostChange)
{
    vector<ServiceMetric> const& metrics = service.ServiceDesc.Metrics;
    ASSERT_IFNOT(metrics.size() == failoverUnit.PrimaryEntries.size() && metrics.size() ==
        failoverUnit.SecondaryEntries.size(), "Metric sizes don't match");

    uint64 appId = service.ServiceDesc.ApplicationId;
    std::set<std::wstring> checkedMetrics;
    std::map<uint64, std::vector<Federation::NodeId>> removeShouldDisappearServicePackageLoad;

    Application const* app = plb_.GetApplicationPtrCallerHoldsLock(appId);
    bool hasApplication = false;

    if (nullptr != app)
    {
        hasApplication = true;
    }
    for (size_t metricIndex = 0; metricIndex < metrics.size(); ++metricIndex)
    {
        auto & metric = metrics[metricIndex];
        wstring const& metricName = metric.Name;
        auto itServiceDomainMetric = metricTable_.find(metricName);
        ASSERT_IF(itServiceDomainMetric == metricTable_.end(),
            "Metric {0} doesn't exist in a domain", service.ServiceDesc.Metrics[metricIndex].Name);

        checkedMetrics.insert(metricName);

        int64 partitionLoad(0);
        int64 shouldDisappearDecrease(0);
        uint replicaLoad(0);
        for (auto itReplica = replicas.begin(); itReplica != replicas.end(); ++itReplica)
        {
            replicaLoad = 0;
            //needed to remove the should disappear load on a node when we add the first normal replica
            uint shouldDisappearLoadChange(0);
            if (itReplica->UsePrimaryLoad())
            {
                replicaLoad = failoverUnit.PrimaryEntries[metricIndex];
            }
            else if (itReplica->UseSecondaryLoad())
            {
                replicaLoad = failoverUnit.GetSecondaryLoad(metricIndex, itReplica->NodeId, plb_.Settings);
            }

            // If config for accounting None replicas that have RG is on
            // account the load only for exclusive hosts as SP is still on the node and LRM still sees that load
            // NOTE: Application reservation for cpu/mem cannot be supported with the existing code.
            // None replicas do not carry application reservation, but they carry load, so there can be load inconsistence.
            if (PLBConfig::GetConfig().IncludeResourceGovernanceNoneReplicaLoad &&
                itReplica->UseNoneLoad(metric.IsRGMetric) &&
                service.ServiceDesc.ServicePackageActivationMode == ServiceModel::ServicePackageActivationMode::ExclusiveProcess)
            {
                // the load is the same for primary and secondary
                replicaLoad = failoverUnit.PrimaryEntries[metricIndex];
            }
            // if it is single host replica it will have load set already
            // discard None and Dropped replicas as they do not contribute to the RG load
            else if (itReplica->HasLoad() &&
                metric.IsRGMetric &&
                service.ServiceDesc.ServicePackageActivationMode == ServiceModel::ServicePackageActivationMode::SharedProcess)
            {
                auto servicePackageReplicaCnt = GetSharedServicePackageNodeCount(service.ServiceDesc.ServicePackageId, itReplica->NodeId);
                //first replica of this service package on the node
                if (!itReplica->ShouldDisappear)
                {
                    if (servicePackageReplicaCnt.first == 0)
                    {
                        replicaLoad = GetServicePackageRgReservation(service.ServiceDesc.ServicePackageId, metricName);
                        //there is a should disappear replica, we need to remove its load as now we have active load
                        if (servicePackageReplicaCnt.second > 0)
                        {
                            shouldDisappearLoadChange = replicaLoad;
                        }
                    }
                }
                else
                {
                    //there is no normal replica and this is the first should disappear replica incoming
                    if (servicePackageReplicaCnt.first == 0 && servicePackageReplicaCnt.second == 0)
                    {
                        replicaLoad = GetServicePackageRgReservation(service.ServiceDesc.ServicePackageId, metricName);
                    }
                }
            }

            if (itReplica->VerifyReplicaLoad.size() <= metricIndex)
            {
                itReplica->VerifyReplicaLoad.push_back(replicaLoad);
            }
            else
            {
                itReplica->VerifyReplicaLoad[metricIndex] = replicaLoad;
            }

            if (replicaLoad > 0)
            {
                itServiceDomainMetric->second.AddLoad(
                    itReplica->NodeId,
                    replicaLoad,
                    itReplica->ShouldDisappear);
            }

            //remove should disappear load from the node
            if (shouldDisappearLoadChange > 0)
            {
                itServiceDomainMetric->second.DeleteLoad(
                    itReplica->NodeId,
                    shouldDisappearLoadChange,
                    true);
            }

            if (hasApplication)
            {
                ApplicationLoad& appLoad = GetApplicationLoad(appId);

                // node load before adding a replica
                int64 prevNodeLoad = 0;
                auto itAppMetricLoad = appLoad.MetricLoads.find(metricName);
                if (itAppMetricLoad != appLoad.MetricLoads.end())
                {
                    prevNodeLoad = itAppMetricLoad->second.GetLoad(itReplica->NodeId);
                }

               appLoad.AddNodeLoad(
                    metricName,
                    itReplica->NodeId,
                    replicaLoad,
                    itReplica->ShouldDisappear);

               if (shouldDisappearLoadChange > 0)
               {
                   appLoad.DeleteNodeLoad(
                       metricName,
                       itReplica->NodeId,
                       shouldDisappearLoadChange,
                       true);
               }
                // shouldDisappear, None and Dropped replicas do not have reservation
               if (app->ApplicationDesc.HasCapacity() &&
                    itReplica->CouldCarryReservationOrScaleout())
                {
                    // node load after adding a replica
                    int64 currNodeLoad = prevNodeLoad + replicaLoad;

                    // application reservation on single node
                    auto const& applicationCapacities = app->ApplicationDesc.AppCapacities;
                    auto const& appCapacityIt = applicationCapacities.find(metricName);

                    int64 reservationCapacity = 0;
                    if (appCapacityIt != applicationCapacities.end())
                    {
                        reservationCapacity = appCapacityIt->second.ReservationCapacity;
                    }

                    // if application has reservation
                    if (reservationCapacity > 0)
                    {
                        if (prevNodeLoad == 0)
                        {
                            // find number of replicas on that node
                            size_t numReplicasOnNode = 0;
                            auto itNumReplicas = appLoad.NodeCounts.find(itReplica->NodeId);
                            if (itNumReplicas != appLoad.NodeCounts.end())
                            {
                                numReplicasOnNode = itNumReplicas->second;
                            }

                            // When changing load or move cost, replica counts for application are not updated, so we need to update manually.
                            // When changing failover unit, replica counts are updated before calling this function, so no need to decrement.
                            if (isLoadOrMoveCostChange){
                                --numReplicasOnNode;
                            }

                            if (numReplicasOnNode > 0)
                            {
                                // (prevLoad = 0 && numReplicas > 0) => these replicas belong to service without metric or to service that reports zero load,
                                // but this service has an application with capacity reservation
                                if (currNodeLoad < reservationCapacity)
                                {
                                    GetReservationLoad(metricName).DeleteNodeReservedLoadForAllApps(itReplica->NodeId, replicaLoad);
                                }
                                else
                                {
                                    GetReservationLoad(metricName).DeleteNodeReservedLoadForAllApps(itReplica->NodeId, reservationCapacity);
                                }
                            }
                            else if (currNodeLoad < reservationCapacity)
                            {
                                // if first replica on node and it's less than reservationCapacity
                                // => update reservation load with reservationCapacity - replicaLoad
                                GetReservationLoad(metricName).AddNodeReservedLoadForAllApps(itReplica->NodeId, reservationCapacity - replicaLoad);
                            }
                        }
                        else
                        {
                            // application already has load on this node
                            if (currNodeLoad < reservationCapacity)
                            {
                                GetReservationLoad(metricName).DeleteNodeReservedLoadForAllApps(itReplica->NodeId, replicaLoad);
                            }
                            else
                            {
                                if (prevNodeLoad < reservationCapacity)
                                {
                                    GetReservationLoad(metricName).DeleteNodeReservedLoadForAllApps(itReplica->NodeId, reservationCapacity - prevNodeLoad);
                                }
                            }
                        }
                    }
                }
            }

            partitionLoad += replicaLoad;
            shouldDisappearDecrease += shouldDisappearLoadChange;
        }
        ApplicationUpdateLoad(service.ServiceDesc.ApplicationId, metricName, partitionLoad, true);
        if (shouldDisappearDecrease > 0)
        {
            ApplicationUpdateLoad(service.ServiceDesc.ApplicationId, metricName, shouldDisappearDecrease, false);
        }
    }

    // if service has no metrics or has zero load, but its application has metrics -> have to add reservation for replicas
    if (app != nullptr && app->ApplicationDesc.HasCapacity())
    {
        UpdateReservationsServicesNoMetric(appId, checkedMetrics, replicas, true, isLoadOrMoveCostChange);
    }
}

inline void ServiceDomain::DeleteNodeLoad(Service const& service, FailoverUnit const& failoverUnit,
    vector<ReplicaDescription> const& replicas)
{
    vector<ServiceMetric> const& metrics = service.ServiceDesc.Metrics;
    ASSERT_IFNOT(metrics.size() == failoverUnit.PrimaryEntries.size() && metrics.size() ==
        failoverUnit.SecondaryEntries.size(), "Metric sizes don't match");

    uint64 appId = service.ServiceDesc.ApplicationId;
    std::set<std::wstring> checkedMetrics;

    for (size_t metricIndex = 0; metricIndex < metrics.size(); ++metricIndex)
    {
        auto & metric = metrics[metricIndex];
        wstring const& metricName = metric.Name;
        auto itServiceDomainMetric = metricTable_.find(metricName);
        ASSERT_IF(itServiceDomainMetric == metricTable_.end(),
            "Metric {0} doesn't exist in a domain", service.ServiceDesc.Metrics[metricIndex].Name);

        checkedMetrics.insert(metricName);

        int64 partitionLoad(0);
        int64 shouldDisappearIncrease(0);

        for (auto itReplica = replicas.begin(); itReplica != replicas.end(); ++itReplica)
        {
            uint replicaLoad(0);
            //needed to activate the should disappear load on a node when we remove the last normal replica
            uint shouldDisappearLoadChange(0);
            if (itReplica->UsePrimaryLoad())
            {
                replicaLoad = failoverUnit.PrimaryEntries[metricIndex];
            }
            else if (itReplica->UseSecondaryLoad())
            {
                replicaLoad = failoverUnit.GetSecondaryLoad(metricIndex, itReplica->NodeId, plb_.Settings);
            }

            // If config for accounting None replicas that have RG is on
            // account the load only for exclusive hosts as SP is still on the node and LRM still sees that load
            // NOTE: Application reservation for cpu/mem cannot be supported with the existing code.
            // None replicas do not carry application reservation, but they carry load, so there can be load inconsistence
            if (PLBConfig::GetConfig().IncludeResourceGovernanceNoneReplicaLoad &&
                itReplica->UseNoneLoad(metric.IsRGMetric) &&
                service.ServiceDesc.ServicePackageActivationMode == ServiceModel::ServicePackageActivationMode::ExclusiveProcess)
            {
                // the load is the same for primary and secondary
                replicaLoad = failoverUnit.PrimaryEntries[metricIndex];
            }
            // if it is single host replica it will have load set already
            // discard None and Dropped replicas as they do not contribute to the RG load
            else if (itReplica->HasLoad() &&
                metric.IsRGMetric &&
                service.ServiceDesc.ServicePackageActivationMode == ServiceModel::ServicePackageActivationMode::SharedProcess)
            {
                auto servicePackageReplicaCnt = GetSharedServicePackageNodeCount(service.ServiceDesc.ServicePackageId, itReplica->NodeId);
                if (!itReplica->ShouldDisappear)
                {
                    //last replica of this service package on the node
                    if (servicePackageReplicaCnt.first == 1)
                    {
                        replicaLoad = GetServicePackageRgReservation(service.ServiceDesc.ServicePackageId, metricName);
                        //we now activate the should disappear load
                        if (servicePackageReplicaCnt.second > 0)
                        {
                            shouldDisappearLoadChange = replicaLoad;
                        }
                    }
                }
                else
                {
                    //last should disappear replica and we do not have normal replicas
                    //just remove should disappear load
                    if (servicePackageReplicaCnt.first == 0 && servicePackageReplicaCnt.second == 1)
                    {
                        replicaLoad = GetServicePackageRgReservation(service.ServiceDesc.ServicePackageId, metricName);
                    }
                }
            }

            //check only for non-rg metrics as the the order we remove the replicas from the node may vary
            //when the last replica is removed from the node we remove the load for the service package
            if (!metric.IsRGMetric && itReplica->VerifyReplicaLoad[metricIndex] != replicaLoad)
            {
                TESTASSERT_IF(itReplica->VerifyReplicaLoad[metricIndex] != replicaLoad, "");
                plb_.Trace.InconsistentReplicaLoad(itReplica->NodeId, failoverUnit.FUId, ReplicaRole::ToString(itReplica->CurrentRole),
                    ReplicaStates::ToString(itReplica->CurrentState), metricName, itReplica->VerifyReplicaLoad[metricIndex], replicaLoad);
            }

            if (replicaLoad > 0)
            {
                itServiceDomainMetric->second.DeleteLoad(
                    itReplica->NodeId,
                    replicaLoad,
                    itReplica->ShouldDisappear);
            }

            if (shouldDisappearLoadChange > 0)
            {
                itServiceDomainMetric->second.AddLoad(
                    itReplica->NodeId,
                    shouldDisappearLoadChange,
                    true);
            }

            Application const * app = plb_.GetApplicationPtrCallerHoldsLock(appId);
            if (!service.ServiceDesc.ApplicationName.empty() && nullptr != app)
            {
                ApplicationLoad& appLoad = GetApplicationLoad(appId);
                // Node load before removing replica
                int64 prevNodeLoad = 0;
                auto itAppMetricLoad = appLoad.MetricLoads.find(metricName);
                if (itAppMetricLoad != appLoad.MetricLoads.end())
                {
                    prevNodeLoad = itAppMetricLoad->second.GetLoad(itReplica->NodeId);
                }

                appLoad.DeleteNodeLoad(
                    metricName,
                    itReplica->NodeId,
                    replicaLoad,
                    itReplica->ShouldDisappear);

                if (shouldDisappearLoadChange > 0)
                {
                    appLoad.AddNodeLoad(
                        metricName,
                        itReplica->NodeId,
                        shouldDisappearLoadChange,
                        true);
                }

                // application reservation on single node
                if (app->ApplicationDesc.HasCapacity() && 
                    itReplica->CouldCarryReservationOrScaleout())
                {
                    // Node load after removing replica
                    int64 currNodeLoad = prevNodeLoad - replicaLoad;

                    auto const& applicationCapacities = app->ApplicationDesc.AppCapacities;
                    auto const& appCapacityIt = applicationCapacities.find(metricName);
                    int64 reservationCapacity = 0;
                    if (appCapacityIt != applicationCapacities.end())
                    {
                        reservationCapacity = appCapacityIt->second.ReservationCapacity;
                    }

                    size_t numReplicasOnNode = 0;
                    auto itNumReplicas = appLoad.NodeCounts.find(itReplica->NodeId);
                    if (itNumReplicas != appLoad.NodeCounts.end())
                    {
                        numReplicasOnNode = itNumReplicas->second;
                    }

                    // if application has reservation
                    if (reservationCapacity > 0)
                    {
                        // delete the last replica on node, remove reservation at all
                        if (numReplicasOnNode == 1)
                        {
                            if (replicaLoad < reservationCapacity)
                            {
                                GetReservationLoad(metricName).DeleteNodeReservedLoadForAllApps(itReplica->NodeId, reservationCapacity - replicaLoad);
                            }
                        }
                        else if (prevNodeLoad > reservationCapacity)
                        {
                            if (currNodeLoad < reservationCapacity)
                            {
                                GetReservationLoad(metricName).AddNodeReservedLoadForAllApps(itReplica->NodeId, reservationCapacity - currNodeLoad);
                            }
                        }
                        else
                        {
                            // update reservation load with replicaLoad;
                            GetReservationLoad(metricName).AddNodeReservedLoadForAllApps(itReplica->NodeId, replicaLoad);
                        }
                    }
                }
            }
            partitionLoad += replicaLoad;
            shouldDisappearIncrease += shouldDisappearLoadChange;
        }

        ApplicationUpdateLoad(service.ServiceDesc.ApplicationId, metricName, partitionLoad, false);
        if (shouldDisappearIncrease > 0)
        {
            ApplicationUpdateLoad(service.ServiceDesc.ApplicationId, metricName, shouldDisappearIncrease, true);
        }
    }

    // case when service has no metrics, but its application has metrics

    Application const * app = plb_.GetApplicationPtrCallerHoldsLock(appId);
    if (appId != 0 && nullptr != app && app->ApplicationDesc.HasCapacity())
    {
        UpdateReservationsServicesNoMetric(appId, checkedMetrics, replicas, false, false);
    }
}

void ServiceDomain::UpdateNodeToFailoverUnitMapping(FailoverUnit const& failoverUnit,
    std::vector<ReplicaDescription> const& oldReplicas,
    std::vector<ReplicaDescription> const& newReplicas,
    bool isStateful)
{
    Guid fuId = failoverUnit.FUId;
    for (auto itReplica = oldReplicas.begin(); itReplica != oldReplicas.end(); ++itReplica)
    {
        if (itReplica->HasLoad())
        {
            auto itNode = partitionsPerNode_.find(itReplica->NodeId);
            ASSERT_IF(itNode == partitionsPerNode_.end(), "Cannot find node {0} from partitionsPerNode_", itReplica->NodeId);

            itNode->second.erase(fuId);
            if (itNode->second.empty())
            {
                partitionsPerNode_.erase(itNode);
            }
        }
        if (itReplica->IsInBuild && isStateful && itReplica->CurrentRole != ReplicaRole::Primary)
        {
            auto itNodeIndex = plb_.nodeToIndexMap_.find(itReplica->NodeId);
            uint64 nodeIndex = itNodeIndex == plb_.nodeToIndexMap_.end() ? UINT64_MAX : itNodeIndex->second;
            ASSERT_IF(nodeIndex == UINT64_MAX, "Replica is on unknown node!");
            auto itNode = inBuildCountPerNode_.find(nodeIndex);
            ASSERT_IF(itNode == inBuildCountPerNode_.end(), "Cannot find node {0} in inBuildCountPerNode_", itReplica->NodeId);
            ASSERT_IF(itNode->second <= 0, "Cannot remove in build replica from node {0} in inBuildCountPerNode_", itReplica->NodeId);
            itNode->second = itNode->second - 1;
            if (itNode->second == 0)
            {
                inBuildCountPerNode_.erase(itNode);
            }
        }
    }

    for (auto itReplica = newReplicas.begin(); itReplica != newReplicas.end(); ++itReplica)
    {
        if (itReplica->HasLoad())
        {
            auto itNode = partitionsPerNode_.find(itReplica->NodeId);
            if (itNode == partitionsPerNode_.end())
            {
                itNode = partitionsPerNode_.insert(make_pair(itReplica->NodeId, map<Guid, bool>())).first;
            }

            itNode->second.insert(make_pair(fuId, itReplica->UsePrimaryLoad()));
        }
        if (itReplica->IsInBuild && isStateful && itReplica->CurrentRole != ReplicaRole::Primary)
        {
            auto itNodeIndex = plb_.nodeToIndexMap_.find(itReplica->NodeId);
            uint64 nodeIndex = itNodeIndex == plb_.nodeToIndexMap_.end() ? UINT64_MAX : itNodeIndex->second;
            ASSERT_IF(nodeIndex == UINT64_MAX, "Replica is on unknown node!");

            auto itNode = inBuildCountPerNode_.find(nodeIndex);
            if (itNode == inBuildCountPerNode_.end())
            {
                itNode = inBuildCountPerNode_.insert(make_pair(nodeIndex, 1)).first;
            }
            else
            {
                itNode->second = itNode->second + 1;
            }
            if (PLBConfig::GetConfig().PerNodeThrottlingCheck > 0)
            {
                // Test code: check if number is over limit.
                TESTASSERT_IF(
                    itNode->second > PLBConfig::GetConfig().PerNodeThrottlingCheck,
                    "Node {0} has {1} InBuildReplicas and limit is {2}.",
                    itReplica->NodeId,
                    itNode->second,
                    PLBConfig::GetConfig().PerNodeThrottlingCheck);
            }
        }
    }
}

void ServiceDomain::UpdateServiceDomainMetricBlockList(Service const& service, uint64 nodeIndex, int delta)
{
    auto & metrics = service.ServiceDesc.Metrics;
    for (auto metricIt = metrics.begin(); metricIt != metrics.end(); ++metricIt)
    {
        auto metricTableIt = metricTable_.find(metricIt->Name);
        TESTASSERT_IF(metricTableIt == metricTable_.end(), "Metric {0} doesn't exist", metricIt->Name);
        if (metricTableIt != metricTable_.end())
        {
            metricTableIt->second.UpdateBlockNodeServiceCount(nodeIndex, delta);
        }
    }
}

inline void ServiceDomain::ComputeServiceBlockList(Service & service) const
{
    DynamicBitSet blockList;
    DynamicBitSet primaryReplicaBlockList;

    wstring const& placementConstraints = service.ServiceDesc.PlacementConstraints;
    if (placementConstraints.empty())
    {
        service.UpdateServiceBlockList(move(blockList));
        service.UpdatePrimaryReplicaBlockList(move(primaryReplicaBlockList));
        return;
    }

    ExpressionSPtr compiledExpression = PLBConfig::GetConfig().ValidatePlacementConstraint && plb_.serviceNameToPlacementConstraintTable_.count(service.ServiceDesc.Name)
                                            ? plb_.serviceNameToPlacementConstraintTable_.at(service.ServiceDesc.Name)
                                            :  Expression::Build(placementConstraints);

    if (compiledExpression == nullptr)
    {
        plb_.Trace.PlacementConstraintParsingError(placementConstraints);
        service.UpdateServiceBlockList(move(blockList));
        service.UpdatePrimaryReplicaBlockList(move(primaryReplicaBlockList));

        return;
    }

    if (!compiledExpression->FDPolicy.empty())
    {
        service.SetFDDistributionPolicy(compiledExpression->FDPolicy);
    }

    if (!compiledExpression->PlacePolicy.empty())
    {
        service.SetPlaceDistributionPolicy(compiledExpression->PlacePolicy);
    }

    if (changedServices_.find(service.ServiceDesc.ServiceId) != changedServices_.end())
    {
        for (auto const& node : plb_.nodes_)
        {
            if (node.NodeDescriptionObj.IsUp)
            {
                if( IsServiceBlockNode(node.NodeDescriptionObj, *compiledExpression, placementConstraints, false, false) )
                {
                    blockList.Add(node.NodeDescriptionObj.NodeIndex);
                }

                if (compiledExpression->ForPrimary && IsPrimaryReplicaBlockNode(node.NodeDescriptionObj,
                    *compiledExpression, placementConstraints))
                {
                    primaryReplicaBlockList.Add(node.NodeDescriptionObj.NodeIndex);
                }
            }
        }
    }
    else
    {
        // start from what the service already has and only deal with the incremental
        blockList.Copy(service.ServiceBlockList);
        primaryReplicaBlockList.Copy(service.PrimaryReplicaBlockList);

        for (auto changedNodeIndex : changedNodes_)
        {
            ASSERT_IF(changedNodeIndex > plb_.nodes_.size(), "NodeId should be present in plb_.nodes_: {0}", changedNodeIndex);

            auto const& node = plb_.nodes_[changedNodeIndex];

            blockList.Delete(node.NodeDescriptionObj.NodeIndex);

            if (node.NodeDescriptionObj.IsUp)
            {
                if (IsServiceBlockNode(node.NodeDescriptionObj, *compiledExpression, placementConstraints, false, true))
                {
                    blockList.Add(node.NodeDescriptionObj.NodeIndex);
                }

                if (compiledExpression->ForPrimary && IsPrimaryReplicaBlockNode(node.NodeDescriptionObj,
                    *compiledExpression, placementConstraints))
                {
                    primaryReplicaBlockList.Add(node.NodeDescriptionObj.NodeIndex);
                }
            }
        }
    }

    service.UpdateServiceBlockList(move(blockList));

    if (primaryReplicaBlockList.Count == plb_.upNodeCount_)
    {
        // primaryReplicaBlockList include all activated nodes
        primaryReplicaBlockList.Clear();
    }
    service.UpdatePrimaryReplicaBlockList(move(primaryReplicaBlockList));
}

bool ServiceDomain::IsPrimaryReplicaBlockNode(NodeDescription const& nodeDescription, Expression & constraintExpression,
    wstring const& placementConstraints) const
{
    return IsServiceBlockNode(nodeDescription, constraintExpression, placementConstraints, true);
}

inline bool ServiceDomain::IsServiceBlockNode(NodeDescription const& nodeDescription, Expression & constraintExpression,
    wstring const& placementConstraints, bool forPrimary, bool traceDetail) const
{
    ASSERT_IFNOT(nodeDescription.IsUp, "Node {0} is down when computing service block list", nodeDescription);

    wstring error;
    bool result = constraintExpression.Evaluate(nodeDescription.NodeProperties, error, forPrimary);

    if (traceDetail && !error.empty())
    {
        plb_.Trace.PlacementConstraintEvaluationError(nodeDescription.NodeId, placementConstraints, L"evaluation error: " + error);
    }

    // if expression contains undefined properties result will be false
    return !result;
}

PartitionClosureUPtr ServiceDomain::GetPartitionClosure(PartitionClosureType::Enum closureType) const
{
    vector<Service const*> services;
    vector<FailoverUnit const*> partitions;
    vector<Application const*> applications;
    vector<ServicePackage const*> servicePackages;

    set<uint64> relatedServices;
    // used for NewReplicaPlacementWithMove
    set<Common::Guid> partialClosureFailoverUnits;
    set<Guid> relatedPartitions;
    set<uint64> relatedApplications;
    set<uint64> relatedApplicationsInSingletonReplicaUpgrade;
    // If move existing replica config is true, we need to include all replica for placement
    bool forCreation = (closureType == PartitionClosureType::NewReplicaPlacement);
    bool forCreationWithMove = (closureType == PartitionClosureType::NewReplicaPlacementWithMove);

    if (forCreation || (closureType == PartitionClosureType::ConstraintCheck && !fullConstraintCheck_))
    {
        if (forCreation)
        {
            // If we need to drop replicas, then we need to include child services into the closure
            // If there are in upgrade replicas and config is true, include all children services as well
            bool walkParentToChild = (0 != partitionsWithExtraReplicas_.size() ||
                (!partitionsWithInUpgradeReplicas_.empty() && PLBConfig::GetConfig().CheckAlignedAffinityForUpgrade)) ||
                (!partitionsWithNewReplicas_.empty() && PLBConfig::GetConfig().CheckAffinityForUpgradePlacement);

            set<Guid> partitionsForCreation = partitionsWithNewReplicas_;
            partitionsForCreation.insert(partitionsWithInUpgradeReplicas_.begin(), partitionsWithInUpgradeReplicas_.end());
            partitionsForCreation.insert(partitionsWithExtraReplicas_.begin(), partitionsWithExtraReplicas_.end());
            // Related partitions set will include all partitionsForCreation
            ComputeTransitiveClosure(partitionsForCreation, walkParentToChild, relatedServices, relatedPartitions, relatedApplications);

            // If there are applications with scaleout == 1 in upgrade,
            // and relaxed scaleout optimization should be performed,
            // then compute application closure for them
            if (partitionsWithNewReplicas_.size() > 0 &&                            // There are partitions with new replicas
                PLBConfig::GetConfig().RelaxScaleoutConstraintDuringUpgrade &&      // Is relax scaleout during upgrade enabled
                plb_.isSingletonReplicaMoveAllowedDuringUpgradeEntry_.GetValue())   // Move of singleton replica is allowed
            {
                relatedApplicationsInSingletonReplicaUpgrade = GetApplicationsInSingletonReplicaUpgrade(relatedApplications);
                if (relatedApplicationsInSingletonReplicaUpgrade.size() > 0)
                {
                    ComputeApplicationClosure(relatedPartitions, relatedServices, relatedApplicationsInSingletonReplicaUpgrade);
                }
            }
        }
        else
        {
            UpdateConstraintCheckClosure();
            ComputeTransitiveClosure(constraintCheckClosure_, true, relatedServices, relatedPartitions, relatedApplications);
            // Add application related partitions and services, needed for application related constraints.
            ComputeApplicationClosure(relatedPartitions, relatedServices, relatedApplications);
        }

        services.reserve(relatedServices.size());
        for (auto itServiceId = relatedServices.begin(); itServiceId != relatedServices.end(); ++itServiceId)
        {
            Service const& service = GetService(*itServiceId);;
            if (service.NonEmptyPartitionCount > 0)
            {
                services.push_back(&service);
            }
        }

        partitions.reserve(relatedPartitions.size());
        for (auto it = relatedPartitions.begin(); it != relatedPartitions.end(); ++it)
        {
            FailoverUnit const& ft = GetFailoverUnit(*it);
            if (!ft.IsEmpty)
            {
                partitions.push_back(&ft);
            }
        }

        for (auto itAppId = relatedApplications.begin(); itAppId != relatedApplications.end(); ++itAppId)
        {
            Application const * app = plb_.GetApplicationPtrCallerHoldsLock(*itAppId);
            if (app != nullptr)
            {
                applications.push_back(app);
            }
        }
    }
    else if (closureType != PartitionClosureType::None)
    {
        // NewReplicaPlacementWithMove will first try with partial closure, so we need to compute movable replicas
        // if that isn't successful, it will try with full closure
        if (forCreationWithMove)
        {
            set<Guid> partitionsForCreation = partitionsWithNewReplicas_;
            ComputeTransitiveClosure(partitionsForCreation, true, relatedServices, relatedPartitions, relatedApplications);
            ComputeApplicationClosure(relatedPartitions, relatedServices, relatedApplications);

            for (auto it = relatedPartitions.begin(); it != relatedPartitions.end(); ++it)
            {
                FailoverUnit const& ft = GetFailoverUnit(*it);
                if (!ft.IsEmpty)
                {
                    partialClosureFailoverUnits.insert(ft.FUId);
                }
            }
        }

        services.reserve(serviceTable_.size());
        for (auto it = serviceTable_.begin(); it != serviceTable_.end(); ++it)
        {
            if (it->second.NonEmptyPartitionCount > 0)
            {
                services.push_back(&(it->second));
            }
        }

        partitions.reserve(failoverUnitTable_.size());
        for (auto it = failoverUnitTable_.begin(); it != failoverUnitTable_.end(); ++it)
        {
            if (!it->second.IsEmpty)
            {
                partitions.push_back(&(it->second));
            }
        }

        // Add only applications that belong to this domain.
        applications.reserve(plb_.applicationTable_.size());
        for (auto it = plb_.applicationTable_.begin(); it != plb_.applicationTable_.end(); ++it)
        {
            auto appToDomainIt = plb_.applicationToDomainTable_.find(it->first);
            if (appToDomainIt != plb_.applicationToDomainTable_.end() && appToDomainIt->second->second.domainId_ == domainId_)
            {
                applications.push_back(&(it->second));
            }
        }
    }

    unordered_set<ServicePackage const*> addedSPs;
    // Add service packages
    for (auto service : services)
    {
        if (service->ServiceDesc.ServicePackageId != 0)
        {
            auto servicePackageIt = plb_.servicePackageTable_.find(service->ServiceDesc.ServicePackageId);
            if (servicePackageIt != plb_.servicePackageTable_.end() && 
                (servicePackageIt->second.HasRGMetrics || servicePackageIt->second.HasContainerImages))
            {
                addedSPs.insert(&(servicePackageIt->second));
            }
        }
    }

    servicePackages.reserve(addedSPs.size());
    for (auto itSP = addedSPs.begin(); itSP != addedSPs.end(); ++itSP)
    {
        servicePackages.push_back(*itSP);
    }

    return make_unique<PartitionClosure>(move(services),
        move(partitions),
        move(applications),
        move(partialClosureFailoverUnits),
        move(servicePackages),
        closureType);
}

std::set<uint64> ServiceDomain::GetApplicationsInSingletonReplicaUpgrade(std::set<uint64> & relatedApplications) const
{
    std::set<uint64> applicationInSingletonReplicaUpgrade;
    for (auto itAppId = relatedApplications.begin(); itAppId != relatedApplications.end(); ++itAppId)
    {
        Application const* application = plb_.GetApplicationPtrCallerHoldsLock(*itAppId);
        if (application != nullptr && application->ApplicationDesc.ScaleoutCount == 1)
        {
            auto itAppPartitions = applicationPartitions_.find(*itAppId);
            if (itAppPartitions != applicationPartitions_.end())
            {
                for (auto itPartition = itAppPartitions->second.begin(); itPartition != itAppPartitions->second.end(); ++itPartition)
                {
                    FailoverUnit const& partition = GetFailoverUnit(*itPartition);
                    if (partition.ActualReplicaDifference > 0 &&
                        partition.TargetReplicaSetSize == 1)
                    {
                        applicationInSingletonReplicaUpgrade.insert(*itAppId);
                        break;
                    }
                }
            }
        }
    }
    return applicationInSingletonReplicaUpgrade;
}

void ServiceDomain::UpdateConstraintCheckClosure() const
{
    // apart from constraintCheckClosure_, we need to identify partitions that are currently contributing to
    // capacity violations.

    // loop through each node and identify metrics that are violating capacity
    for (auto node : plb_.nodes_)
    {
        if (!node.NodeDescriptionObj.IsUp)
        {
            continue;
        }
        auto const& capacities = node.NodeDescriptionObj.Capacities;
        unordered_set<wstring> overCapacityMetrics;
        for (auto itMetric = metricTable_.begin(); itMetric != metricTable_.end(); ++itMetric)
        {
            auto itCapacity = capacities.find(itMetric->first);
            auto itNodeBufferPercentage = plb_.Settings.NodeBufferPercentage.find(itMetric->first);
            double nodeBufferPercentage = itNodeBufferPercentage != plb_.Settings.NodeBufferPercentage.end() ?
                itNodeBufferPercentage->second : 0;

            if (itCapacity != capacities.end())
            {
                auto load = itMetric->second.GetLoad(node.NodeDescriptionObj.NodeId);
                auto itMetricReservation = reservationLoadTable_.find(itMetric->first);
                int64 reservation = 0;
                if (itMetricReservation != reservationLoadTable_.end())
                {
                    reservation = itMetricReservation->second.GetNodeReservedLoadUsed(node.NodeDescriptionObj.NodeId);
                }
                if (load + reservation > (int64)(itCapacity->second * (1 - nodeBufferPercentage)))
                {
                    overCapacityMetrics.insert(itMetric->first);
                }
            }
        }

        if (overCapacityMetrics.empty())
        {
            continue;
        }

        unordered_set<uint64> overCapacityApplications;

        // here we also include partitions with replicas which have unmatched node instance (plb.NodeExistAndMatch >= 2)
        auto itNodeToPartitions = partitionsPerNode_.find(node.NodeDescriptionObj.NodeId);
        if (itNodeToPartitions != partitionsPerNode_.end())
        {
            auto &fuIds = itNodeToPartitions->second;
            for (auto itFuId = fuIds.begin(); itFuId != fuIds.end(); ++itFuId)
            {
                if (constraintCheckClosure_.find(itFuId->first) != constraintCheckClosure_.end())
                {
                    continue;
                }

                auto & failoverUnit = GetFailoverUnit(itFuId->first);
                auto & service = GetService(failoverUnit.FuDescription.ServiceId);
                auto &metrics = service.ServiceDesc.Metrics;
                bool addedToClosure = false;

                for (size_t metricIndex = 0; metricIndex < metrics.size(); metricIndex++)
                {
                    // Use secondary load for any secondary or standBy replica and primary load for primary replicas.
                    if (overCapacityMetrics.find(metrics[metricIndex].Name) != overCapacityMetrics.end())
                    {
                        uint load;
                        if (itFuId->second)
                        {
                            load = failoverUnit.PrimaryEntries[metricIndex];
                        }
                        else
                        {
                            load = failoverUnit.GetSecondaryLoad(metricIndex, node.NodeDescriptionObj.NodeId, plb_.Settings);
                        }

                        //we add all the partitions if we are dealing with RG metrics
                        if (load > 0 || metrics[metricIndex].IsRGMetric)
                        {
                            constraintCheckClosure_.insert(itFuId->first);
                            addedToClosure = true;
                            break;
                        }
                    }
                }
                if (!addedToClosure)
                {
                    auto appId = service.ServiceDesc.ApplicationId;
                    if (overCapacityApplications.find(appId) != overCapacityApplications.end())
                    {
                        constraintCheckClosure_.insert(itFuId->first);
                    }
                    else
                    {
                        auto application = plb_.GetApplicationPtrCallerHoldsLock(appId);
                        if (nullptr != application)
                        {
                            auto & appCapacities = application->ApplicationDesc.AppCapacities;
                            for (auto itAppMetric = appCapacities.begin(); itAppMetric != appCapacities.end(); ++itAppMetric)
                            {
                                if (itAppMetric->second.ReservationCapacity > 0 && overCapacityMetrics.find(itAppMetric->first) != overCapacityMetrics.end())
                                {
                                    constraintCheckClosure_.insert(itFuId->first);
                                    overCapacityApplications.insert(appId);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void ServiceDomain::ComputeTransitiveClosure(set<Guid> const& initialPartitions, bool walkParentToChild,
    set<uint64> & relatedServices, set<Guid> & relatedPartitions, std::set<uint64> & relatedApplications) const
{
    // the related set of services & partitions that are reachable from initialPartitions, and are connected by constraints
    // the code uses BFS thus is general enough to handle affinity chains
    deque<Guid> theQueue(initialPartitions.begin(), initialPartitions.end());
    relatedPartitions.insert(initialPartitions.begin(), initialPartitions.end());

    auto expandClosure = [&](wstring const& serviceName)
    {
        auto const& itServiceId = plb_.ServiceIdMap.find(serviceName);
        if (itServiceId == plb_.ServiceIdMap.end())
        {
            return;
        }
        auto itService = serviceTable_.find(itServiceId->second);
        if (itService != serviceTable_.end())
        {
            Service const& service = itService->second;
            if (service.ActualPartitionCount > 0u)
            {
                for (auto fuId : service.FailoverUnits)
                {
                    if (relatedPartitions.find(fuId) == relatedPartitions.end())
                    {
                        relatedPartitions.insert(fuId);
                        theQueue.push_back(fuId);
                    }
                }
            }
        }
    };

    while (!theQueue.empty())
    {
        Guid fuId = theQueue.front();
        theQueue.pop_front();

        FailoverUnit const& ft = GetFailoverUnit(fuId);
        wstring const& serviceName = ft.FuDescription.ServiceName;

        Service const& service = GetService(ft.FuDescription.ServiceId);
        relatedServices.insert(ft.FuDescription.ServiceId);

        Application const * app = plb_.GetApplicationPtrCallerHoldsLock(service.ServiceDesc.ApplicationId);
        if (nullptr != app)
        {
            relatedApplications.insert(service.ServiceDesc.ApplicationId);
        }

        // expand
        // child to parent
        if (!service.ServiceDesc.AffinitizedService.empty())
        {
            expandClosure(service.ServiceDesc.AffinitizedService);
        }

        if (walkParentToChild)
        {
            // parent to child
            auto itParent = childServices_.find(serviceName);
            if (itParent != childServices_.end())
            {
                for (auto itChild = itParent->second.begin(); itChild != itParent->second.end(); ++itChild)
                {
                    expandClosure(*itChild);
                }
            }
        }
    }
}

void ServiceDomain::ComputeApplicationClosure(
    set<Common::Guid> & relatedPartitions,
    set<uint64> & relatedServices,
    set<uint64> & relatedApplications) const
{
    for (auto itAppId = relatedApplications.begin(); itAppId != relatedApplications.end(); ++itAppId)
    {
        Application const* application = plb_.GetApplicationPtrCallerHoldsLock(*itAppId);
        if (application == nullptr)
        {
            continue;
        }
        if (application->ApplicationDesc.HasScaleoutOrCapacity())
        {
            auto itAppPartitions = applicationPartitions_.find(*itAppId);
            if (itAppPartitions != applicationPartitions_.end())
            {
                for (auto itPartition = itAppPartitions->second.begin(); itPartition != itAppPartitions->second.end(); ++itPartition)
                {
                    //if it was not present before this returns true
                    if (relatedPartitions.insert(*itPartition).second)
                    {
                        auto & ft = GetFailoverUnit(*itPartition);
                        relatedServices.insert(ft.FuDescription.ServiceId);
                    }
                }
            }
        }
    }
}

void ServiceDomain::UpdateServiceBlockList(Service & service)
{
    wstring const& serviceTypeName = service.ServiceDesc.ServiceTypeName;

    bool recomputeServiceBlockList = changedServices_.find(service.ServiceDesc.ServiceId) != changedServices_.end() || !changedNodes_.empty();
    if (recomputeServiceBlockList || changedServiceTypes_.find(serviceTypeName) != changedServiceTypes_.end())
    {
        DynamicBitSet oldOverallBlockList(service.BlockList);

        if (recomputeServiceBlockList)
        {
            if (lastEvaluatedPlacementConstraint_ == service.ServiceDesc.PlacementConstraints)
            {
                // These properties do not depend on service type, so no need to check
                service.UpdatePrimaryReplicaBlockList(DynamicBitSet(lastEvaluatedPrimaryReplicaBlockList_));
                service.UpdateServiceBlockList(DynamicBitSet(lastEvaluatedServiceBlockList_));
                service.PartiallyPlace = lastEvaluatedPartialPlacement_;
                service.FDDistribution = lastEvaluatedFDDistributionPolicy_;
            }
            else
            {
                ComputeServiceBlockList(service);
            }
        }

        if (   lastEvaluatedPlacementConstraint_ == service.ServiceDesc.PlacementConstraints
            && lastEvaluatedServiceType_ == service.ServiceDesc.ServiceTypeName)
        {
            // Reusing last evaluated overall blocklist only if last evaluated service and service type are the same!
            service.UpdateOverallBlockList(DynamicBitSet(lastEvaluatedOverallBlocklist_));
        }
        else
        {
            DynamicBitSet blockList(service.ServiceBlockList);
            ServiceType const& serviceType = plb_.GetServiceType(service.ServiceDesc);
            auto & typeBlockList = serviceType.ServiceTypeDesc.BlockList;
            for (auto itBlockNode = typeBlockList.begin(); itBlockNode != typeBlockList.end(); ++itBlockNode)
            {
                auto itNodeIndex = plb_.nodeToIndexMap_.find(*itBlockNode);
                uint64 nodeIndex = itNodeIndex == plb_.nodeToIndexMap_.end() ? UINT64_MAX : itNodeIndex->second;

                if (nodeIndex != UINT64_MAX && plb_.nodes_[nodeIndex].NodeDescriptionObj.IsUp)
                {
                    blockList.Add(nodeIndex);
                }
            }

            service.UpdateOverallBlockList(move(blockList));
        }

        if (IsServiceAffectingBalancing(service))
        {
            auto & newOverallBlockList = service.BlockList;
            oldOverallBlockList.ForEach([&](size_t nodeId) -> void
            {
                if (!newOverallBlockList.Check(nodeId))
                {
                    // Node was blocked but not blocked now
                    UpdateServiceDomainMetricBlockList(service, nodeId, -1);
                }
            });

            newOverallBlockList.ForEach([&](size_t nodeId) -> void
            {
                if (!oldOverallBlockList.Check(nodeId))
                {
                    // Was not blocked before, but blocked now
                    UpdateServiceDomainMetricBlockList(service, nodeId, 1);
                }
            });
        }

        lastEvaluatedServiceBlockList_.Copy(service.ServiceBlockList);
        lastEvaluatedPrimaryReplicaBlockList_.Copy(service.PrimaryReplicaBlockList);
        lastEvaluatedPlacementConstraint_ = service.ServiceDesc.PlacementConstraints;
        lastEvaluatedOverallBlocklist_.Copy(service.BlockList);
        lastEvaluatedServiceType_ = service.ServiceDesc.ServiceTypeName;
        lastEvaluatedPartialPlacement_ = service.PartiallyPlace;
        lastEvaluatedFDDistributionPolicy_ = service.FDDistribution;
    }
}

void ServiceDomain::AddToPartitionStatistics(FailoverUnit const& failoverUnit)
{
    inTransitionPartitionCount_ += failoverUnit.FuDescription.IsInTransition ? 1 : 0;
    movableReplicaCount_ += static_cast<int64>(failoverUnit.FuDescription.MovableReplicaCount);
    existingReplicaCount_ += static_cast<int64>(failoverUnit.FuDescription.Replicas.size());
}

void ServiceDomain::RemoveFromPartitionStatistics(FailoverUnit const& failoverUnit)
{
    inTransitionPartitionCount_ -= failoverUnit.FuDescription.IsInTransition ? 1 : 0;
    movableReplicaCount_ -= static_cast<int64>(failoverUnit.FuDescription.MovableReplicaCount);
    existingReplicaCount_ -= static_cast<int64>(failoverUnit.FuDescription.Replicas.size());
    ASSERT_IF(inTransitionPartitionCount_ < 0 || movableReplicaCount_ < 0 || existingReplicaCount_ < 0, "Replica count error");
}

void ServiceDomain::AddToReplicaDifferenceStatistics(Guid fuId, int replicaDiff)
{
    if (replicaDiff > 0)
    {
        newReplicaCount_ += replicaDiff;
        partitionsWithNewReplicas_.insert(fuId);
    }
    else if (replicaDiff < 0)
    {
        partitionsWithExtraReplicas_.insert(fuId);
    }

    if (replicaDiff == INT_MAX)
    {
        partitionsWithInstOnEveryNode_.insert(fuId);
    }
}

void ServiceDomain::RemoveFromReplicaDifferenceStatistics(Guid fuId, int replicaDiff)
{
    if (replicaDiff > 0)
    {
        newReplicaCount_ -= replicaDiff;
        ASSERT_IF(newReplicaCount_ < 0, "Replica count error");
        auto it = partitionsWithNewReplicas_.find(fuId);
        ASSERT_IF(it == partitionsWithNewReplicas_.end(), "Partitions with new replicas error");
        partitionsWithNewReplicas_.erase(it);
    }
    else if (replicaDiff < 0)
    {
        auto it = partitionsWithExtraReplicas_.find(fuId);
        ASSERT_IF(it == partitionsWithExtraReplicas_.end(), "Partition with extra replicas not found");
        partitionsWithExtraReplicas_.erase(it);
    }

    if (replicaDiff == INT_MAX)
    {
        partitionsWithInstOnEveryNode_.erase(fuId);
    }
}

void ServiceDomain::AddToUpgradeStatistics(FailoverUnitDescription const & newDescription)
{
    if (newDescription.HasInUpgradeReplica)
    {
        // Either swap of place movement would be generated
        partitionsWithInUpgradeReplicas_.insert(newDescription.FUId);
    }
    if (newDescription.IsInUpgrade)
    {
        ++partitionsInAppUpgrade_;
    }
}

void ServiceDomain::RemoveFromUpgradeStatistics(FailoverUnit const& failoverUnit)
{
    auto it = partitionsWithInUpgradeReplicas_.find(failoverUnit.FuDescription.FUId);
    if (it != partitionsWithInUpgradeReplicas_.end())
    {
        partitionsWithInUpgradeReplicas_.erase(failoverUnit.FuDescription.FUId);
    }
    if (failoverUnit.FuDescription.IsInUpgrade)
    {
        TESTASSERT_IF(partitionsInAppUpgrade_ == 0,
            "Partition in upgrade count is 0 when removing {0} from upgrade count", failoverUnit.FuDescription.FUId);
        --partitionsInAppUpgrade_;
    }
}

Service const& ServiceDomain::GetService(std::wstring const& name) const
{
    auto const& itServiceId = plb_.ServiceIdMap.find(name);
    ASSERT_IF(itServiceId == plb_.ServiceIdMap.end(), "Service {0} doesn't exist", name);

    auto it = serviceTable_.find(itServiceId->second);
    ASSERT_IF(it == serviceTable_.end(), "Service {0} doesn't exist", name);

    return it->second;
}

Service & ServiceDomain::GetService(std::wstring const& name)
{
    auto const& itServiceId = plb_.ServiceIdMap.find(name);
    ASSERT_IF(itServiceId == plb_.ServiceIdMap.end(), "Service {0} doesn't exist", name);

    auto it = serviceTable_.find(itServiceId->second);

    ASSERT_IF(it == serviceTable_.end(), "Service {0} doesn't exist", name);

    return it->second;
}

Service const& ServiceDomain::GetService(uint64 serviceId) const
{
    auto it = serviceTable_.find(serviceId);
    ASSERT_IF(it == serviceTable_.end(), "Service {0} doesn't exist", serviceId);

    return it->second;
}

Service & ServiceDomain::GetService(uint64 serviceId)
{
    auto it = serviceTable_.find(serviceId);

    ASSERT_IF(it == serviceTable_.end(), "Service {0} doesn't exist", serviceId);

    return it->second;
}

FailoverUnit const& ServiceDomain::GetFailoverUnit(Guid const& id) const
{
    auto it = failoverUnitTable_.find(id);
    ASSERT_IF(it == failoverUnitTable_.end(), "FailoverUnit {0} doesn't exist", id);

    return it->second;
}

FailoverUnit & ServiceDomain::GetFailoverUnit(Guid const& id)
{
    auto it = failoverUnitTable_.find(id);
    ASSERT_IF(it == failoverUnitTable_.end(), "FailoverUnit {0} doesn't exist", id);

    return it->second;
}

// verify dynamically maintained load info is consistent, only runs under test mode
void ServiceDomain::VerifyNodeLoads(bool includeDisappearingReplicas) const
{
    map<wstring, map<wstring, ServiceDomainLocalMetric>> serviceMetrics;
    set<wstring> rgMetrics;

    for (auto it = serviceTable_.begin(); it != serviceTable_.end(); ++it)
    {
        map<wstring, ServiceDomainLocalMetric> domainMetrics;
        auto &metrics = it->second.ServiceDesc.Metrics;
        for (auto itMetric = metrics.begin(); itMetric != metrics.end(); ++itMetric)
        {
            domainMetrics.insert(make_pair(itMetric->Name, ServiceDomainLocalMetric()));
        }
        serviceMetrics.insert(make_pair(it->second.ServiceDesc.Name, move(domainMetrics)));
    }

    map<wstring, ServiceDomainLocalMetric> globalMetrics;

    for (auto itFU = failoverUnitTable_.begin(); itFU != failoverUnitTable_.end(); ++itFU)
    {
        FailoverUnitDescription const& fuDesc = itFU->second.FuDescription;

        auto const& itServiceId = plb_.ServiceIdMap.find(fuDesc.ServiceName);
        ASSERT_IF(itServiceId == plb_.ServiceIdMap.end(), "Service {0} doesn't exist", fuDesc.ServiceName);

        auto itService = serviceTable_.find(itServiceId->second);
        ASSERT_IF(itService == serviceTable_.end(),
            "FailoverUnit's service {0} doesn't exist in serviceTable_, includeDisappearingReplicas {1}",
            fuDesc.ServiceName, includeDisappearingReplicas);
        auto &metrics = itService->second.ServiceDesc.Metrics;
        auto itServiceMetric = serviceMetrics.find(fuDesc.ServiceName);
        ASSERT_IF(itServiceMetric == serviceMetrics.end(),
            "Service metrics not found for service {0} FailoverUnit",
            fuDesc.ServiceName);
        LoadEntry primaryEntry(vector<int64>(itFU->second.PrimaryEntries.begin(), itFU->second.PrimaryEntries.end()));

        fuDesc.ForEachReplica([&](ReplicaDescription const & replica)
        {
            if ((replica.UsePrimaryLoad() || replica.UseSecondaryLoad()) && replica.ShouldDisappear == includeDisappearingReplicas)
            {
                int metricIndex = 0;

                for (auto itMetric = metrics.begin(); itMetric != metrics.end(); ++itMetric)
                {
                    if (itMetric->IsRGMetric)
                    {
                        rgMetrics.insert(itMetric->Name);
                        ++metricIndex;
                        continue;
                    }

                    int64 value = 0;
                    if (replica.UsePrimaryLoad())
                    {
                        value = primaryEntry.Values[metricIndex];
                    }
                    else if (replica.UseSecondaryLoad())
                    {
                        value = itFU->second.GetSecondaryLoad(metricIndex, replica.NodeId, plb_.Settings);
                    }

                    itServiceMetric->second[itMetric->Name].AddLoad(replica.NodeId, static_cast<uint>(value));

                    // add to global metric
                    auto itGlobalMetric = globalMetrics.find(itMetric->Name);
                    if (itGlobalMetric == globalMetrics.end())
                    {
                        itGlobalMetric = globalMetrics.insert(make_pair(itMetric->Name, ServiceDomainLocalMetric())).first;
                    }
                    itGlobalMetric->second.AddLoad(replica.NodeId, static_cast<uint>(value));
                    ++metricIndex;
                }
            }
        });
    }

    for (auto it = serviceTable_.begin(); it != serviceTable_.end(); ++it)
    {
        if (it->second.IsSingleton)
        {
            continue;
        }
        it->second.VerifyMetricLoads(serviceMetrics[it->second.ServiceDesc.Name], includeDisappearingReplicas);
    }

    // verify global loads
    for (auto it = globalMetrics.begin(); it != globalMetrics.end(); ++it)
    {
        auto itMetric = metricTable_.find(it->first);
        ASSERT_IF(itMetric == metricTable_.end(), "Global metric {0} cannot be found in metricTable_, includeDisappearingReplicas {1}",
            it->first, includeDisappearingReplicas);
        itMetric->second.VerifyLoads(it->second, includeDisappearingReplicas);
    }

    for (auto it = metricTable_.begin(); it != metricTable_.end(); ++it)
    {
        if (!rgMetrics.empty() && rgMetrics.find(it->first) == rgMetrics.end())
        {
            auto itMetric = globalMetrics.find(it->first);
            if (itMetric == globalMetrics.end())
            {
                it->second.VerifyAreZero(includeDisappearingReplicas);
            }
        }
    }
}

int64 ServiceDomain::GetTotalRemainingResource(wstring const& metricName, double nodeBufferPercentage) const
{
    // Get the ServiceDomainMetric, which includes the node load
    auto itDomainMetric = metricTable_.find(metricName);
    if (itDomainMetric == metricTable_.end())
    {
        return INT64_MAX;
    }

    int64 totalClusterCapacity = plb_.GetTotalClusterCapacity(metricName);
    if (totalClusterCapacity == INT64_MAX)
    {
        return INT64_MAX;
    }

    int64 totalClusterCapacityWithoutBuffer = (int64)((1 - nodeBufferPercentage) * totalClusterCapacity);
    int64 metricTotalResource = totalClusterCapacityWithoutBuffer - itDomainMetric->second.NodeLoadSum;

    // We should not return negativity since service with 0 required load should still be allowed
    return metricTotalResource > 0 ? metricTotalResource : 0;
}

int64 ServiceDomain::GetReservationNodeLoad(std::wstring const& metricName, Federation::NodeId nodeId)
{
    auto it = reservationLoadTable_.find(metricName);
    if (it != reservationLoadTable_.end())
    {
        return it->second.GetNodeReservedLoadUsed(nodeId);
    }

    return 0;
}

ReservationLoad & ServiceDomain::GetReservationLoad(std::wstring const& metricName)
{
    auto it = reservationLoadTable_.find(metricName);
    if (it == reservationLoadTable_.end())
    {
        it = reservationLoadTable_.insert(make_pair(metricName, ReservationLoad())).first;
    }

    return it->second;
}

int64 ServiceDomain::GetTotalReservedLoadUsed(wstring const& metricName) const
{
    auto it = reservationLoadTable_.find(metricName);
    if (it != reservationLoadTable_.end())
    {
        return it->second.TotalReservedLoadUsed;
    }
    return 0;
}

void ServiceDomain::UpdateTotalReservedLoadUsed(wstring const& metricName, int64 loadChange)
{
    auto it = reservationLoadTable_.find(metricName);
    if (it != reservationLoadTable_.end())
    {
        it->second.UpdateTotalReservedLoadUsed(loadChange);
        ASSERT_IF(it->second.TotalReservedLoadUsed < 0, "Existing reserved load is less than the deleted load {0} when updating total load for metric {1}", loadChange, metricName);
    }
    else
    {
        ASSERT_IF(loadChange < 0, "There shouldn't be negative reserved load {0} when adding to total load for metric {1}", loadChange, metricName);
        ReservationLoad resLoad = ReservationLoad();
        resLoad.UpdateTotalReservedLoadUsed(loadChange);
        reservationLoadTable_.insert(make_pair(metricName, move(resLoad)));
    }
}

bool ServiceDomain::ApplicationHasScaleoutOrCapacity(uint64 applicationId) const
{
    if (applicationId == 0)
    {
        return false;
    }

    Application const * app = plb_.GetApplicationPtrCallerHoldsLock(applicationId);

    if (nullptr == app)
    {
        return false;
    }

    if (app->ApplicationDesc.HasScaleoutOrCapacity())
    {
        return true;
    }

    return false;
}

void ServiceDomain::CheckAndUpdateAppPartitions(uint64 applicationId)
{
    if (!ApplicationHasScaleoutOrCapacity(applicationId))
    {
        return;
    }

    for (auto itFU = failoverUnitTable_.begin(); itFU != failoverUnitTable_.end(); ++itFU)
    {
        FailoverUnitDescription const& fuDesc = itFU->second.FuDescription;
        Service & service = GetService(fuDesc.ServiceId);
        if (applicationId == service.ServiceDesc.ApplicationId)
        {
            UpdateApplicationPartitions(applicationId, fuDesc.FUId);
        }
    }
}

void ServiceDomain::AddAppPartitionsToPartialClosure(uint64 applicationId)
{
    if (!ApplicationHasScaleoutOrCapacity(applicationId) || fullConstraintCheck_)
    {
        return;
    }

    auto itAppPartitions = applicationPartitions_.find(applicationId);
    if (itAppPartitions != applicationPartitions_.end())
    {
        for (auto itFU = itAppPartitions->second.begin(); itFU != itAppPartitions->second.end(); ++itFU)
        {
            constraintCheckClosure_.insert(*itFU);
        }
    }
}

void ServiceDomain::UpdateApplicationPartitions(uint64 applicationId, Common::Guid partitionId)
{
    auto itApp = applicationPartitions_.find(applicationId);
    if (itApp == applicationPartitions_.end())
    {
        itApp = applicationPartitions_.insert(make_pair(applicationId, set<Common::Guid>())).first;
    }

    itApp->second.insert(partitionId);
}

void ServiceDomain::RemoveApplicationPartitions(uint64 applicationId, Common::Guid partitionId)
{
    if (applicationId == 0)
    {
        // can't check scaleout here since as long as the app in the partition map, it should be removed
        return;
    }

    auto itApp = applicationPartitions_.find(applicationId);
    if (itApp == applicationPartitions_.end())
    {
        if (ApplicationHasScaleoutOrCapacity(applicationId))
        {
            // if app has scaleout count or capacity, it should be in this map; Not fatal error, output warning trace
            plb_.Trace.NotFoundInAppPartitionsMap(L"Delete partition", wformatString("{0}", applicationId));
        }

        return;
    }

    itApp->second.erase(partitionId);
    if (itApp->second.empty())
    {
        applicationPartitions_.erase(itApp);
    }

}

void ServiceDomain::RemoveApplicationFromApplicationPartitions(uint64 applicationId)
{
    auto itApp = applicationPartitions_.find(applicationId);
    if (itApp == applicationPartitions_.end())
    {
        return;
    }
    else
    {
        applicationPartitions_.erase(itApp);
    }
}

ApplicationLoad & ServiceDomain::GetApplicationLoad(uint64 applicationId)
{
    // Assert used for testing only, should not happen in production
    TESTASSERT_IF(applicationId == 0, "ApplicationId cannot be zero!");
    auto it = applicationLoadTable_.find(applicationId);
    if (it == applicationLoadTable_.end())
    {
        it = applicationLoadTable_.insert(make_pair(applicationId, ApplicationLoad(applicationId))).first;
    }

    return it->second;
}

ApplicationLoad const& ServiceDomain::GetApplicationLoad(uint64 applicationId) const
{
    // Assert used for testing only, should not happen in production
    TESTASSERT_IF(applicationId == 0, "ApplicationId cannot be zero!");
    auto const& it = applicationLoadTable_.find(applicationId);
    ASSERT_IF(it == applicationLoadTable_.end(), "Application {0} doesn't exist", applicationId);
    return it->second;
}

void ServiceDomain::ApplicationUpdateLoad(uint64 applicationId, wstring const& metricName, int64 load, bool isAdd)
{
    if (applicationId == 0)
    {
        return;
    }

    Application const * app = plb_.GetApplicationPtrCallerHoldsLock(applicationId);
    if (nullptr == app)
    {
        return;
    }
    ApplicationLoad & appLoad = GetApplicationLoad(applicationId);

    auto appReservedCapacity = app->GetReservedCapacity(metricName);
    auto oldReservedLoad = appLoad.GetReservedLoadUsed(metricName, appReservedCapacity);

    if (isAdd)
    {
        appLoad.AddLoad(metricName, load);
    }
    else
    {
        appLoad.DeleteLoad(metricName, load);
    }

    // calculate the reserved load change
    auto newReservedLoad = appLoad.GetReservedLoadUsed(metricName, appReservedCapacity);
    UpdateTotalReservedLoadUsed(metricName, newReservedLoad - oldReservedLoad);
}

void ServiceDomain::UpdateApplicationReservedLoad(ApplicationDescription const& applicationDescription, bool isAdd)
{
    if (applicationLoadTable_.find(applicationDescription.ApplicationId) == applicationLoadTable_.end())
    {
        // Skip the update if application has no load
        return;
    }

    for (auto applicationCapacity : applicationDescription.AppCapacities)
    {
        wstring const& metricName = applicationCapacity.first;
        if (reservationLoadTable_.find(metricName) == reservationLoadTable_.end())
        {
            // If there is no reserved load skip the update (there should be 0 for each metric)!
            continue;
        }

        ApplicationLoad const& appLoad = GetApplicationLoad(applicationDescription.ApplicationId);

        int64 capacity = applicationCapacity.second.ReservationCapacity * applicationDescription.MinimumNodes;
        int64 reservedLoadUsed = appLoad.GetReservedLoadUsed(applicationCapacity.first, capacity);
        if (isAdd)
        {
            // Assumption: there was no reservation, so we are adding from 0 to reservedLoadUsed
            UpdateTotalReservedLoadUsed(metricName, reservedLoadUsed);
        }
        else
        {
            // Assumption: there is a reservation, so we want to remove because we are removing reserved capacity
            // Note: when updating an application, we will first remove old reservation and then add the new one
            UpdateTotalReservedLoadUsed(metricName, -reservedLoadUsed);
        }
    }
}

void ServiceDomain::UpdateReservationForNewApplication(ApplicationDescription const& appDescription)
{
    uint64 appId = appDescription.ApplicationId;

    if (!ApplicationHasLoad(appId))
    {
        // There is no load, hence no reservation.
        return;
    }

    ApplicationLoad const& appLoad = GetApplicationLoad(appId);

    // This function is called only when we add new application to this domain.
    // As a consequence, we will just add reserved load used to the table.

    for (auto const& appMetric : appDescription.AppCapacities)
    {
        int64 reservationCapacity = appMetric.second.ReservationCapacity;
        if (reservationCapacity == 0)
        {
            continue;
        }

        auto const& metricLoadIt = appLoad.MetricLoads.find(appMetric.first);
        if (metricLoadIt != appLoad.MetricLoads.end())
        {
            for (auto const& nodeLoad : metricLoadIt->second.NodeLoads)
            {
                if (nodeLoad.second < reservationCapacity)
                {
                    // Add reservation to this node.
                    GetReservationLoad(appMetric.first).AddNodeReservedLoadForAllApps(nodeLoad.first, reservationCapacity - nodeLoad.second);
                }
            }
        }
    }
}

void ServiceDomain::UpdateApplicationFailoverUnitsReservation(ApplicationDescription const& applicationDescription, bool isAdd)
{
    uint64 appId = applicationDescription.ApplicationId;

    auto itApp = applicationPartitions_.find(appId);

    if (itApp != applicationPartitions_.end())
    {
        std::set<Common::Guid> failoverUnits = itApp->second;
        for (auto itFailoverUnits = failoverUnits.begin(); itFailoverUnits != failoverUnits.end(); ++itFailoverUnits)
        {
            auto itFUnit = failoverUnitTable_.find(*itFailoverUnits);
            if (itFUnit != failoverUnitTable_.end()){
                FailoverUnit const& failoverUnit = move(itFUnit->second);
                Service const& service = GetService(failoverUnit.FuDescription.ServiceId);
                vector<ReplicaDescription> const& replicas = failoverUnit.FuDescription.Replicas;

                if (isAdd)
                {
                    AddNodeLoad(service, failoverUnit, replicas);
                    UpdateApplicationNodeCounts(service.ServiceDesc.ApplicationId, replicas, true);
                    UpdateServicePackagePlacement(service, replicas, true);
                }
                else
                {
                    DeleteNodeLoad(service, failoverUnit, replicas);
                    UpdateApplicationNodeCounts(service.ServiceDesc.ApplicationId, replicas, false);
                    UpdateServicePackagePlacement(service, replicas, false);
                }
            }
        }
    }

}

inline void ServiceDomain::UpdateApplicationNodeCounts(
    uint64 applicationId,
    std::vector<ReplicaDescription> const& replicas,
    bool isAdd)
{
    if (applicationId == 0)
    {
        return;
    }

    ApplicationLoad & appLoad = GetApplicationLoad(applicationId);
    for (auto r : replicas)
    {
        if (r.CouldCarryReservationOrScaleout())
        {
            if (isAdd)
            {
                appLoad.AddReplicaToNode(r.NodeId);
            }
            else
            {
                appLoad.DeleteReplicaFromNode(r.NodeId);
            }
        }
    }
}

void ServiceDomain::UpdateServicePackagePlacement(
    Service const & service,
    std::vector<ReplicaDescription> const& replicas,
    bool isAdd)
{
    auto servicePackageId = service.ServiceDesc.ServicePackageId;

    bool isExclusive = service.ServiceDesc.ServicePackageActivationMode == ServiceModel::ServicePackageActivationMode::ExclusiveProcess;

    if (servicePackageId == 0)
    {
        return;
    }

    for (auto & replica : replicas)
    {
        // do not consider None and Dropped replicas
        if (!replica.HasLoad())
        {
            continue;
        }

        auto & servicePackage = servicePackageReplicaCountPerNode_[replica.NodeId][servicePackageId];
        if (isAdd)
        {
            if (replica.ShouldDisappear)
            {
                if (isExclusive)
                {
                    ++servicePackage.ExclusiveServiceTotalReplicaCount;
                }
                else
                {
                    ++servicePackage.SharedServiceDisappearReplicaCount;
                }
            }
            else
            {
                if (isExclusive)
                {
                    ++servicePackage.ExclusiveServiceTotalReplicaCount;
                }
                else
                {
                    ++servicePackage.SharedServiceRegularReplicaCount;
                }
            }
        }
        else
        {
            if (replica.ShouldDisappear)
            {
                if (isExclusive)
                {
                    --servicePackage.ExclusiveServiceTotalReplicaCount;
                    ASSERT_IF(servicePackage.ExclusiveServiceTotalReplicaCount < 0, "Service package {0} replica count underflow with replica {1}", servicePackageId, replica);
                }
                else
                {
                    --servicePackage.SharedServiceDisappearReplicaCount;
                    ASSERT_IF(servicePackage.SharedServiceDisappearReplicaCount < 0, "Service package {0} replica count underflow with replica {1}", servicePackageId, replica);
                }
            }
            else
            {
                if (isExclusive)
                {
                    --servicePackage.ExclusiveServiceTotalReplicaCount;
                    ASSERT_IF(servicePackage.ExclusiveServiceTotalReplicaCount < 0, "Service package {0} replica count underflow with replica {1}", servicePackageId, replica);
                }
                else
                {
                    --servicePackage.SharedServiceRegularReplicaCount;
                    ASSERT_IF(servicePackage.SharedServiceRegularReplicaCount < 0, "Service package {0} replica count underflow with replica {1}", servicePackageId, replica);
                }
            }
        }
    }
}

pair<int, int> ServiceDomain::GetSharedServicePackageNodeCount(uint64 servicePackageId, Federation::NodeId nodeId) const
{
    auto itServicePackageNode = servicePackageReplicaCountPerNode_.find(nodeId);
    if (itServicePackageNode != servicePackageReplicaCountPerNode_.end())
    {
        auto itServicePackage = itServicePackageNode->second.find(servicePackageId);
        if (itServicePackage != itServicePackageNode->second.end())
        {
            int regularReplicasCnt = itServicePackage->second.SharedServiceRegularReplicaCount;
            int shouldDisappearReplicasCnt = itServicePackage->second.SharedServiceDisappearReplicaCount;
            return pair<int, int>(regularReplicasCnt, shouldDisappearReplicasCnt);
        }
    }
    return pair<int, int> (0, 0);
}

bool ServiceDomain::IsSafeToUpgradeSP(uint64 servicePackageId, ServicePackageDescription const & servicePackage)
{
    // It is safe to upgrade SP if we there are enough resources on each node (i.e. node is not in violation on any of RG metrics).
    // We will check only SPs that are actually increasing resource usage on the nodes.
    for (auto & node : plb_.nodes_)
    {
        if (node.NodeDescriptionObj.IsUp)
        {
            Federation::NodeId nodeId = node.NodeDescriptionObj.NodeId;
            auto itServicePackageNode = servicePackageReplicaCountPerNode_.find(nodeId);
            if (itServicePackageNode != servicePackageReplicaCountPerNode_.end())
            {
                auto itServicePackage = itServicePackageNode->second.find(servicePackageId);
                if (itServicePackage != itServicePackageNode->second.end())
                {
                    auto const& capacities = node.NodeDescriptionObj.Capacities;
                    //in case there are replicas on this node (either shared or exclusive, either regular ones or should disappear ones)
                    if (itServicePackage->second.HasAnyReplica())
                    {
                        for (auto & spResource : servicePackage.CorrectedRequiredResources)
                        {
                            auto itServiceDomainMetric = metricTable_.find(spResource.first);
                            if (itServiceDomainMetric != metricTable_.end())
                            {
                                int64 capacity = INT64_MAX;
                                auto const& capacityIt = capacities.find(spResource.first);
                                if (capacityIt != capacities.end())
                                {
                                    capacity = capacityIt->second;
                                }

                                //here we need to account for ALL load for these metrics on the node
                                auto load = itServiceDomainMetric->second.GetLoad(nodeId, false);
                                auto shouldDisappearLoad = itServiceDomainMetric->second.GetLoad(nodeId, true);
                                if (load + shouldDisappearLoad > capacity)
                                {
                                    plb_.Trace.ServicePackageInsufficientResourceUpgrade(nodeId, spResource.first, load, shouldDisappearLoad, capacity, servicePackage, itServicePackage->second);
                                    return false;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return true;
}

void ServiceDomain::UpdateServicePackageLoad(uint64 serviceId, bool isAdd)
{
    auto itService = Services.find(serviceId);
    if (itService != Services.end())
    {
        if (itService->second.ServiceDesc.ServicePackageActivationMode == ServiceModel::ServicePackageActivationMode::SharedProcess)
        {
            for (auto ftId : itService->second.FailoverUnits)
            {
                auto itFT = failoverUnitTable_.find(ftId);
                if (itFT != failoverUnitTable_.end())
                {
                    auto & replicas = itFT->second.FuDescription.Replicas;
                    //because we are removing load for all metrics we need to adjust app node counts due to reservation
                    if (isAdd)
                    {
                        itService->second.AddNodeLoad(itFT->second, replicas, plb_.Settings);
                        AddNodeLoad(itService->second, itFT->second, replicas);
                        UpdateApplicationNodeCounts(itService->second.ServiceDesc.ApplicationId, replicas, true);
                        UpdateServicePackagePlacement(itService->second, replicas, true);
                    }
                    else
                    {
                        itService->second.DeleteNodeLoad(itFT->second, replicas, plb_.Settings);
                        DeleteNodeLoad(itService->second, itFT->second, replicas);
                        UpdateApplicationNodeCounts(itService->second.ServiceDesc.ApplicationId, replicas, false);
                        UpdateServicePackagePlacement(itService->second, replicas, false);
                    }
                }
            }
        }
    }
}

std::wstring ServiceDomain::GetDomainIdPrefix(ServiceDomain::DomainId const domainId)
{
    std::wstring::size_type pivot = domainId.find_first_of(L"_");
    ASSERT_IF(pivot == std::wstring::npos, "Couldn't find the pivot charactor '_' in domain id {0}", domainId);

    return domainId.substr(0, pivot);
}

std::wstring ServiceDomain::GetDomainIdSuffix(ServiceDomain::DomainId const domainId)
{
    std::wstring::size_type pivot = domainId.find_first_of(L"_");
    ASSERT_IF(pivot == std::wstring::npos, "Couldn't find the pivot charactor '_' in domain id {0}", domainId);

    return domainId.substr(pivot+1);
}

size_t ServiceDomain::BalancingMovementCount() const
{
    return plb_.GlobalBalancingMovementCounter.GetCount();
}

size_t ServiceDomain::PlacementMovementCount() const
{
    return plb_.GlobalPlacementMovementCounter.GetCount();
}

uint ServiceDomain::GetServicePackageRgReservation(uint64 servicePackageId, wstring const & metricName) const
{
    auto itServicePackage = plb_.servicePackageTable_.find(servicePackageId);
    if (itServicePackage != plb_.servicePackageTable_.end())
    {
        auto & servicePackage = itServicePackage->second;
        return servicePackage.Description.GetMetricRgReservation(metricName);
    }
    return 0;
}

void ServiceDomain::RemoveServicePackageInfo(uint64 servicePackageId)
{
    for (auto itServicePackageNode = servicePackageReplicaCountPerNode_.begin(); itServicePackageNode != servicePackageReplicaCountPerNode_.end(); ++itServicePackageNode)
    {
        itServicePackageNode->second.erase(servicePackageId);
    }
}

// For testing purpose
// Gets the number of replicas for a specified service package and given node
void ServiceDomain::Test_GetServicePackageReplicaCountForGivenNode(
    uint64 servicePackageId,
    int& exclusiveRegularReplicaCount,
    int& sharedRegularReplicaCount,
    int& sharedDisappearReplicaCount,
    Federation::NodeId nodeId)
{

    auto servicePackageOnNode = servicePackageReplicaCountPerNode_.find(nodeId);
    if (servicePackageOnNode != servicePackageReplicaCountPerNode_.end())
    {
        auto itServicePackageReplicas = servicePackageOnNode->second.find(servicePackageId);
        if (itServicePackageReplicas != servicePackageOnNode->second.end())
        {
            auto & servicePackage = servicePackageReplicaCountPerNode_[nodeId][servicePackageId];
            exclusiveRegularReplicaCount = servicePackage.ExclusiveServiceTotalReplicaCount;
            sharedRegularReplicaCount = servicePackage.SharedServiceRegularReplicaCount;
            sharedDisappearReplicaCount = servicePackage.SharedServiceDisappearReplicaCount;
        }
        else
        {
            TESTASSERT_IF(itServicePackageReplicas == servicePackageOnNode->second.end(),
                "Invalid situation - there are no replicas of this service package: {0}.", servicePackageId);
        }
    }
    else 
    {
        exclusiveRegularReplicaCount = 0;
        sharedRegularReplicaCount = 0;
        sharedDisappearReplicaCount = 0;
    }
}
