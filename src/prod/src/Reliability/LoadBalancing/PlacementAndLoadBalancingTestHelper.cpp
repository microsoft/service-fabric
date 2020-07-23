// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "PlacementAndLoadBalancingTestHelper.h"
#include "PlacementAndLoadBalancing.h"
#include "IFailoverManager.h"
#include "TestUtility.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

PlacementAndLoadBalancingTestHelper::PlacementAndLoadBalancingTestHelper(IPlacementAndLoadBalancing & plb)
    : plb_(dynamic_cast<PlacementAndLoadBalancing &>(plb))
{
    ResetTiming();
}

void PlacementAndLoadBalancingTestHelper::SetMovementEnabled(bool constraintCheckEnabled, bool balancingEnabled)
{
    plb_.SetMovementEnabled(constraintCheckEnabled, balancingEnabled);
}

void PlacementAndLoadBalancingTestHelper::UpdateNode(NodeDescription && nodeDescription)
{
    plb_.UpdateNode(std::move(nodeDescription));
}

void PlacementAndLoadBalancingTestHelper::UpdateServiceType(ServiceTypeDescription && serviceTypeDescription)
{
    plb_.UpdateServiceType(std::move(serviceTypeDescription));
}

void PlacementAndLoadBalancingTestHelper::DeleteServiceType(std::wstring const& serviceTypeName)
{
    plb_.DeleteServiceType(serviceTypeName);
}

void PlacementAndLoadBalancingTestHelper::UpdateClusterUpgrade(bool isUpgradeInProgress, std::set<std::wstring> && completedUDs)
{
    plb_.UpdateClusterUpgrade(isUpgradeInProgress, move(completedUDs));
}

Common::ErrorCode PlacementAndLoadBalancingTestHelper::UpdateService(ServiceDescription && serviceDescription, bool forceUpdate)
{
    return plb_.UpdateService(std::move(serviceDescription), forceUpdate);
}

void PlacementAndLoadBalancingTestHelper::DeleteService(std::wstring const& serviceName)
{
    plb_.DeleteService(serviceName);
}

void PlacementAndLoadBalancingTestHelper::UpdateFailoverUnit(FailoverUnitDescription && failoverUnitDescription)
{
    plb_.UpdateFailoverUnit(std::move(failoverUnitDescription));
}

void PlacementAndLoadBalancingTestHelper::DeleteFailoverUnit(std::wstring && serviceName, Common::Guid fuId)
{
    plb_.DeleteFailoverUnit(std::move(serviceName), fuId);
}

void PlacementAndLoadBalancingTestHelper::UpdateLoadOrMoveCost(LoadOrMoveCostDescription && loadOrMoveCost)
{
    plb_.UpdateLoadOrMoveCost(std::move(loadOrMoveCost));
}

Common::ErrorCode PlacementAndLoadBalancingTestHelper::ResetPartitionLoad(Reliability::FailoverUnitId const & failoverUnitId, std::wstring const & serviceName, bool isStateful)
{
    return plb_.ResetPartitionLoad(failoverUnitId, serviceName, isStateful);
}

Common::ErrorCode PlacementAndLoadBalancingTestHelper::UpdateApplication(ApplicationDescription && applicationDescription, bool forceUpdate)
{
    return plb_.UpdateApplication(std::move(applicationDescription), forceUpdate);
}

void PlacementAndLoadBalancingTestHelper::DeleteApplication(std::wstring const& applicationName, bool forceUpdate)
{
    plb_.DeleteApplication(applicationName, forceUpdate);
}

Common::ErrorCode PlacementAndLoadBalancingTestHelper::ToggleVerboseServicePlacementHealthReporting(bool enabled)
{
    return plb_.ToggleVerboseServicePlacementHealthReporting(enabled);
}

Common::ErrorCode PlacementAndLoadBalancingTestHelper::GetClusterLoadInformationQueryResult(ServiceModel::ClusterLoadInformationQueryResult & queryResult)
{
    return plb_.GetClusterLoadInformationQueryResult(queryResult);
}

Common::ErrorCode PlacementAndLoadBalancingTestHelper::GetNodeLoadInformationQueryResult(Federation::NodeId nodeId, ServiceModel::NodeLoadInformationQueryResult & queryResult, bool onlyViolations) const
{
    return plb_.GetNodeLoadInformationQueryResult(nodeId, queryResult, onlyViolations);
}

ServiceModel::UnplacedReplicaInformationQueryResult PlacementAndLoadBalancingTestHelper::GetUnplacedReplicaInformationQueryResult(std::wstring const& serviceName, Common::Guid const& partitionId, bool onlyQueryPrimaries)
{
    return plb_.GetUnplacedReplicaInformationQueryResult(serviceName, partitionId, onlyQueryPrimaries);
}

Common::ErrorCode PlacementAndLoadBalancingTestHelper::GetApplicationLoadInformationResult(std::wstring const & applicationName, ServiceModel::ApplicationLoadInformationQueryResult & result)
{
    return plb_.GetApplicationLoadInformationResult(applicationName, result);
}

int PlacementAndLoadBalancingTestHelper::CompareNodeForPromotion(std::wstring const& serviceName, Common::Guid fuId, Federation::NodeId node1, Federation::NodeId node2)
{
    return plb_.CompareNodeForPromotion(serviceName, fuId, node1, node2);
}

void PlacementAndLoadBalancingTestHelper::Dispose()
{
    plb_.Dispose();

    plb_.Test_WaitForTracingThreadToFinish();
}

Common::ErrorCode PlacementAndLoadBalancingTestHelper::TriggerPromoteToPrimary(std::wstring const& serviceName, Common::Guid const& fuId, Federation::NodeId newPrimary)
{
    return plb_.TriggerPromoteToPrimary(serviceName, fuId, newPrimary);
}

Common::ErrorCode PlacementAndLoadBalancingTestHelper::TriggerSwapPrimary(std::wstring const& serviceName, Common::Guid const& fuId, Federation::NodeId currentPrimary, Federation::NodeId & newPrimary, bool force, bool chooseRandom)
{
    return plb_.TriggerSwapPrimary(serviceName, fuId, currentPrimary, newPrimary, force, chooseRandom);
}

Common::ErrorCode PlacementAndLoadBalancingTestHelper::TriggerMoveSecondary(std::wstring const& serviceName, Common::Guid const& fuId, Federation::NodeId currentSecondary, Federation::NodeId & newSecondary, bool force, bool chooseRandom)
{
    return plb_.TriggerMoveSecondary(serviceName, fuId, currentSecondary, newSecondary, force, chooseRandom);
}

void PlacementAndLoadBalancingTestHelper::OnDroppedPLBMovement(Common::Guid const& failoverUnitId, Reliability::PlbMovementIgnoredReasons::Enum reason, Common::Guid const& decisionId)
{
    plb_.OnDroppedPLBMovement(failoverUnitId, reason, decisionId);
}

void PlacementAndLoadBalancingTestHelper::OnFMBusy()
{
    plb_.OnFMBusy();
}

void PlacementAndLoadBalancingTestHelper::OnDroppedPLBMovements(std::map<Common::Guid, FailoverUnitMovement> && droppedMovements, Reliability::PlbMovementIgnoredReasons::Enum reason, Common::Guid const& decisionId)
{
    plb_.OnDroppedPLBMovements(std::move(droppedMovements), reason, decisionId);
}

void PlacementAndLoadBalancingTestHelper::OnSafetyCheckAcknowledged(ServiceModel::ApplicationIdentifier const & appId)
{
    plb_.OnSafetyCheckAcknowledged(appId);
}

void PlacementAndLoadBalancingTestHelper::OnExecutePLBMovement(Common::Guid const & partitionId)
{
    plb_.OnExecutePLBMovement(partitionId);
}

void PlacementAndLoadBalancingTestHelper::ProcessPendingUpdatesPeriodicTask()
{
    plb_.ProcessPendingUpdatesPeriodicTask();
}

void PlacementAndLoadBalancingTestHelper::RefreshServicePackageForRG(
    bool & hasServicePackage,
    bool & hasApp)
{
    map<wstring, ServiceModel::ServicePackageIdentifier> serviceTypes;
    vector<ServiceDescription> serviceDescrptions;

    PLBConfig const& config = PLBConfig::GetConfig();
    if (!config.DefragmentationMetrics.empty() ||
        !config.MetricActivityThresholds.empty() ||
        !config.MetricBalancingThresholds.empty())
    {
        hasServicePackage = true;
    }

    for (auto itDomain = plb_.serviceDomainTable_.begin();
        itDomain != plb_.serviceDomainTable_.end() && !hasServicePackage && !hasApp;
        ++itDomain)
    {
        Uint64UnorderedMap<Service> const& services = itDomain->second.Services;

        for (auto itService = services.begin(); itService != services.end(); ++itService)
        {
            auto serviceDesc = itService->second.ServiceDesc;
            if (!serviceDesc.ServicePackageIdentifier.IsEmpty())
            {
                hasServicePackage = true;
                break;
            }

            if (!serviceDesc.ApplicationName.empty())
            {
                hasApp = true; 
                break;
            }

            if (serviceTypes.find(serviceDesc.ServiceTypeName) == serviceTypes.end())
            {
                ServiceModel::ServicePackageIdentifier sp;
                serviceTypes.insert(make_pair(serviceDesc.ServiceTypeName, sp));
            }

            serviceDescrptions.push_back(serviceDesc);
        }
    }

    if (!hasServicePackage && !hasApp)
    {
        // All services don't have service package and application defined
        wstring generalTestRGApp = L"generalRGtest";
        wstring appTypeName = wformatString("{0}_AppType", generalTestRGApp);
        wstring appName = wformatString("{0}_Application", generalTestRGApp);

        vector<ServicePackageDescription> packages;

        for (auto it = serviceTypes.begin(); it != serviceTypes.end(); ++it)
        {
            wstring servicePackageName = wformatString("{0}_ServicePackage", it->first);
            ServiceModel::ServicePackageIdentifier & sp = it->second;
            ServiceModel::ApplicationIdentifier appId(appTypeName, 0);
            sp = ServiceModel::ServicePackageIdentifier(ServiceModel::ApplicationIdentifier(appId), servicePackageName);

            map<std::wstring, double> requiredResources;
            requiredResources.insert(make_pair(ServiceModel::Constants::SystemMetricNameCpuCores, 1));
            requiredResources.insert(make_pair(ServiceModel::Constants::SystemMetricNameMemoryInMB, 1048));

            packages.push_back(ServicePackageDescription(
                ServiceModel::ServicePackageIdentifier(sp),
                move(requiredResources),
                std::vector<std::wstring>()));

            map<ServiceModel::ServicePackageIdentifier, ServicePackageDescription> packageMap1;

            for (auto const& spi : packages)
            {
                ServiceModel::ServicePackageIdentifier spId(appId, spi.Name);
                packageMap1.insert(make_pair(spId, spi));
            }

            plb_.UpdateApplication(ApplicationDescription(
                wstring(appName),
                map<std::wstring, ApplicationCapacitiesDescription>(),
                0,
                0,
                move(packageMap1),
                map<ServiceModel::ServicePackageIdentifier, ServicePackageDescription>(),
                false, // IsUpgrading
                std::set<wstring>(),
                appId));
                
        }

        for (auto it = serviceDescrptions.begin(); it != serviceDescrptions.end(); ++it)
        {
            ServiceDescription & description = *it;

            if (serviceDescrptions.size() == 1)
            {
                // Add a random service to avoid the service domain being deleted
                plb_.UpdateService(ServiceDescription(
                    L"rgPlainService",
                    wstring(description.ServiceTypeName),           // Service type
                    wstring(appName),                   // Application name
                    true,                               // Stateful?
                    L"",                       // Placement constraints
                    L"",                       // Parent
                    true,                               // Aligned affinity
                    vector<ServiceMetric>(description.Metrics),           // Default metrics
                    description.DefaultMoveCost,               // Move cost
                    false                              // On every node
                ));
            }

            ServiceModel::ServicePackageIdentifier & spId = serviceTypes.find(description.ServiceTypeName)->second;

            plb_.UpdateService(ServiceDescription(
                wstring(description.Name),
                wstring(description.ServiceTypeName),           // Service type
                wstring(appName),                   // Application name
                description.IsStateful,                               // Stateful?
                wstring(description.PlacementConstraints),                       // Placement constraints
                wstring(description.AffinitizedService),                       // Parent
                description.AlignedAffinity,                               // Aligned affinity
                vector<ServiceMetric>(description.Metrics),           // Default metrics
                description.DefaultMoveCost,               // Move cost
                description.OnEveryNode,                              // On every node
                description.PartitionCount,                                  // Partition count
                description.TargetReplicaSetSize,                                  // Target replica set size
                description.HasPersistedState,                               // Persisted?
                ServiceModel::ServicePackageIdentifier(spId),
                ServiceModel::ServicePackageActivationMode::SharedProcess,
                0,
                vector<ServiceScalingPolicyDescription>(description.ScalingPolicies)
            ));  // Service package
        }

    }

}

void PlacementAndLoadBalancingTestHelper::Refresh(Common::StopwatchTime now)
{
    bool hasServicePackage = false;
    bool hasApp = false;

    if (PLBConfig::GetConfig().UseRGInBoost)
    {
        RefreshServicePackageForRG(hasServicePackage, hasApp);
    }

    // 'now' passed-in may be a ajusted value
    Common::StopwatchTime newNow = Stopwatch::Now();
    if (newNow < now)
    {
        newNow = now + Common::TimeSpan::FromSeconds(1);
    }

    if (!hasServicePackage && !hasApp)
    {
        PLBConfig::KeyIntegerValueMap activityThresholds = PLBConfig::GetConfig().MetricActivityThresholds;
        activityThresholds.insert(make_pair(ServiceModel::Constants::SystemMetricNameCpuCores, INT_MAX));
        activityThresholds.insert(make_pair(ServiceModel::Constants::SystemMetricNameMemoryInMB, INT_MAX));
        PLBConfigScopeChange(MetricActivityThresholds, PLBConfig::KeyIntegerValueMap, activityThresholds);

        PLBConfig::KeyDoubleValueMap balancingThresholds = PLBConfig::GetConfig().MetricBalancingThresholds;
        balancingThresholds.insert(make_pair(ServiceModel::Constants::SystemMetricNameCpuCores, 0));
        balancingThresholds.insert(make_pair(ServiceModel::Constants::SystemMetricNameMemoryInMB, 0));
        PLBConfigScopeChange(MetricBalancingThresholds, PLBConfig::KeyDoubleValueMap, balancingThresholds);

        PLBConfig::KeyDoubleValueMap metricWeight = PLBConfig::GetConfig().GlobalMetricWeights;
        metricWeight.insert(make_pair(ServiceModel::Constants::SystemMetricNameCpuCores, 0));
        metricWeight.insert(make_pair(ServiceModel::Constants::SystemMetricNameMemoryInMB, 0));
        PLBConfigScopeChange(GlobalMetricWeights, PLBConfig::KeyDoubleValueMap, metricWeight);

        plb_.Refresh(newNow);
    }
    else
    {
        plb_.Refresh(newNow);
    }

}

bool PlacementAndLoadBalancingTestHelper::CheckLoadReport(std::wstring const& serviceName, Common::Guid fuId, int numberOfReports)
{
    {
        AcquireReadLock grab(plb_.lock_);
        auto const& itId = plb_.serviceToIdMap_.find(serviceName);
        TESTASSERT_IF(itId == plb_.serviceToIdMap_.end(), "Service {0} does not exist", serviceName);
        auto itServiceToDomain = plb_.serviceToDomainTable_.find(itId->second);
        TESTASSERT_IF(itServiceToDomain == plb_.serviceToDomainTable_.end(), "Service {0} does not exist in the serviceToDomainTable_", serviceName);
        auto itServiceDomain = plb_.serviceDomainTable_.find((itServiceToDomain->second)->first);
        TESTASSERT_IF(itServiceDomain == plb_.serviceDomainTable_.end(), "Service domain does not exist in the serviceDomainTable_ for service {0}", serviceName);
        auto itFt = (itServiceDomain->second).get_FailoverUnits().find(fuId);

        if (itFt != (itServiceDomain->second).get_FailoverUnits().end())
        {
            if (itFt->second.UpdateCount == numberOfReports)
            {
                return true;
            }
        }
        else if (numberOfReports == 0)
        {
            return true;
        }
    }
    return false;
}

bool PlacementAndLoadBalancingTestHelper::CheckLoadValue(
    std::wstring const& serviceName,
    Common::Guid fuId,
    wstring const& metricName,
    ReplicaRole::Enum role,
    uint loadValue,
    Federation::NodeId const& nodeId,
    bool callGetSecondaryLoad)
{
    {
        AcquireReadLock grab(plb_.lock_);
        auto const& itId = plb_.serviceToIdMap_.find(serviceName);
        TESTASSERT_IF(itId == plb_.serviceToIdMap_.end(), "Service {0} does not exist", serviceName);
        auto itServiceToDomain = plb_.serviceToDomainTable_.find(itId->second);
        TESTASSERT_IF(itServiceToDomain == plb_.serviceToDomainTable_.end(), "Service {0} does not exist in the serviceToDomainTable_", serviceName);
        auto itServiceDomain = plb_.serviceDomainTable_.find((itServiceToDomain->second)->first);
        TESTASSERT_IF(itServiceDomain == plb_.serviceDomainTable_.end(), "Service domain does not exist in the serviceDomainTable_ for service {0}", serviceName);
        auto ftEntry = (itServiceDomain->second).get_FailoverUnits().find(fuId);

        if (ftEntry != (itServiceDomain->second).get_FailoverUnits().end())
        {

            int index = itServiceDomain->second.get_Services().find(plb_.serviceToIdMap_.find(serviceName)->second)->second.GetCustomLoadIndex(metricName);
            if (index < 0)
            {
                return false;
            }

            if (role == ReplicaRole::Primary)
            {
                return ftEntry->second.PrimaryEntries[index] == loadValue;
            }
            else if (callGetSecondaryLoad)
            {
                return ftEntry->second.GetSecondaryLoad(index, nodeId, plb_.Settings) == loadValue;
            }
            else
            {
                auto it = ftEntry->second.SecondaryEntriesMap.find(nodeId);
                if (it != ftEntry->second.SecondaryEntriesMap.end())
                {
                    return it->second[index] == loadValue;
                }

                return ftEntry->second.SecondaryEntries[index] == loadValue;
            }
        }
    }

    return false;
}

bool PlacementAndLoadBalancingTestHelper::CheckMoveCostValue(std::wstring const& serviceName, Common::Guid fuId, ReplicaRole::Enum role, uint moveCostValue)
{
    {
        AcquireReadLock grab(plb_.lock_);

        auto const& itId = plb_.serviceToIdMap_.find(serviceName);
        if (itId == plb_.serviceToIdMap_.end())
        {
            return false;
        }

        auto sdtEntry = plb_.serviceToDomainTable_.find(itId->second);
        if (sdtEntry == plb_.serviceToDomainTable_.end())
        {
            return false;
        }

        auto sdEntry = plb_.serviceDomainTable_.find((sdtEntry->second)->first);
        if (sdEntry == plb_.serviceDomainTable_.end())
        {
            return false;
        }

        auto fuEntry = (sdEntry->second).get_FailoverUnits().find(fuId);

        if (fuEntry != (sdEntry->second).get_FailoverUnits().end())
        {
            uint recordedMoveCost = (role == ReplicaRole::Primary) ? fuEntry->second.PrimaryMoveCost : fuEntry->second.SecondaryMoveCost;

            return moveCostValue == recordedMoveCost;
        }
    }
    return false;
}

void PlacementAndLoadBalancingTestHelper::GetReservedLoad(wstring const& metricName, int64 & reservedCapacity, int64 & reservedLoadUsed) const
{
    auto sdIterator = plb_.metricToDomainTable_.find(metricName);

    if (sdIterator == plb_.metricToDomainTable_.end())
    {
        // Test should expect this in case when domains are empty!
        reservedLoadUsed = -1;
    }
    else
    {
        reservedLoadUsed = sdIterator->second->second.GetTotalReservedLoadUsed(metricName);
    }

    auto capacityIt = plb_.totalReservedCapacity_.find(metricName);

    if (capacityIt == plb_.totalReservedCapacity_.end())
    {
        // No capacity, return 0
        reservedCapacity = 0;
    }
    else
    {
        reservedCapacity = capacityIt->second;
    }
}

void PlacementAndLoadBalancingTestHelper::Test_GetServicePackageReplicaCountForGivenNode(
    ServiceModel::ServicePackageIdentifier const& servicePackageIdentifier,
    int& exclusiveRegularReplicaCount,
    int& sharedRegularReplicaCount,
    int& sharedDisappearReplicaCount,
    Federation::NodeId nodeId) const
{
    //get service package id
    auto spIdIterator = plb_.servicePackageToIdMap_.find(servicePackageIdentifier);
    if (spIdIterator != plb_.servicePackageToIdMap_.end())
    {
        // from service package id => get servicePackage
        auto spIterator = plb_.servicePackageTable_.find(spIdIterator->second);
        if (spIterator != plb_.servicePackageTable_.end())
        {
            // get all services from this service package
            auto allServices = spIterator->second.Services;

            // take the first service => all of them should belong to one domain
            if (allServices.size() > 0)
            {
                auto itServiceDomain = plb_.serviceToDomainTable_.find(*allServices.begin());
                if (itServiceDomain != plb_.serviceToDomainTable_.end())
                {
                    itServiceDomain->second->second.Test_GetServicePackageReplicaCountForGivenNode(
                        spIdIterator->second,
                        exclusiveRegularReplicaCount,
                        sharedRegularReplicaCount,
                        sharedDisappearReplicaCount,
                        nodeId);
                }
                else
                {
                    TESTASSERT_IF(itServiceDomain == plb_.serviceToDomainTable_.end(),
                        "Merging error - cannot find domain for the specified service {0}.", *allServices.begin());
                }
                // check if all services are in the same domain as they should
                for (auto itServiceID : allServices)
                {
                    auto itCurrentServiceDomain = plb_.serviceToDomainTable_.find(itServiceID);
                    TESTASSERT_IF(itCurrentServiceDomain->second->second.Id != itServiceDomain->second->second.Id,
                        "All of the services for the service package: {0} should be in one domain.", itServiceID);
                }
            }
            else
            {
                TESTASSERT_IF(allServices.size() == 0,
                    "Invalid situation - there are no services for this service package identifier: {0}.", servicePackageIdentifier);
            }
        }
        else 
        {
            TESTASSERT_IF(spIterator == plb_.servicePackageTable_.end(),
                "Cannot find specified service package id: {0} in the map.", spIdIterator->second);
        }
    }
    else
    {
        TESTASSERT_IF(spIdIterator == plb_.servicePackageToIdMap_.end(),
            "Cannot find specified service package identifier: {0} in the map.", servicePackageIdentifier);
    }
}

void PlacementAndLoadBalancingTestHelper::GetReservedLoadNode(wstring const& metricName, int64 & reservedLoadUsed, Federation::NodeId nodeId) const
{
    auto sdIterator = plb_.metricToDomainTable_.find(metricName);

    if (sdIterator == plb_.metricToDomainTable_.end())
    {
        // Test should expect this in case when domains are empty!
        reservedLoadUsed = -1;
    }
    else
    {
        reservedLoadUsed = sdIterator->second->second.GetReservationNodeLoad(metricName, nodeId);
    }
}

void PlacementAndLoadBalancingTestHelper::GetApplicationSumLoadAndCapacityHelper(
    ServiceDomain const& serviceDomain,
    uint64 appId, 
    wstring const& metricName,
    int64 & appSumLoad, 
    int64& size) const
{
    size = static_cast<int64> (serviceDomain.GetApplicationLoadTableSize());
    ApplicationLoad const* applicationLoad = &serviceDomain.GetApplicationLoad(appId);
    auto itMetricLoads = applicationLoad->MetricLoads.find(metricName);
    if (itMetricLoads != applicationLoad->MetricLoads.end())
    {
        appSumLoad = itMetricLoads->second.NodeLoadSum;
    }
    else
    {
        appSumLoad = -1;
    }
}

// This function checks loads for both appGroup applications and nonAppGroup applications(without capacity or scaleout)
// applicationToDomainTable_ will add to domain only appGroup applications (which has scaleout or reservation/capacity/ instance capacity for some metic)
// for nonAppGroup applications applicationToDomainTable_ will be empty
void PlacementAndLoadBalancingTestHelper::GetApplicationSumLoadAndCapacity(
    wstring const& appName, 
    wstring const& metricName, 
    int64 & appSumLoad, 
    int64& size) const
{
    auto appIdIter = plb_.applicationToIdMap_.find(appName);
    uint64 appId = (appIdIter == plb_.applicationToIdMap_.end()) ? 0 : appIdIter->second;

    auto const& appTableIt = plb_.applicationTable_.find(appId);
    if (appTableIt == plb_.applicationTable_.end()) 
    {
        appSumLoad = -1;
        size = -1;
    }

    // it is appGroup
    if (appTableIt->second.ApplicationDesc.HasScaleoutOrCapacity())
    {
        // for appGroup applications
        auto serviceDomainIterator = plb_.applicationToDomainTable_.find(appId);

        if (serviceDomainIterator != plb_.applicationToDomainTable_.end())
        {
            ServiceDomain const& serviceDomain = serviceDomainIterator->second->second;
            GetApplicationSumLoadAndCapacityHelper(serviceDomain, appId, metricName, appSumLoad, size);
        }
        else
        {
            appSumLoad = -1;
            size = -1;
        }
    }
    else
    {
        //for nonAppGroup applications(without scaleout or capacities/reservation)
        auto serviceDomainIterator = plb_.metricToDomainTable_.find(metricName);
        if (serviceDomainIterator != plb_.metricToDomainTable_.end())
        {
            ServiceDomain const& serviceDomain = serviceDomainIterator->second->second;
            GetApplicationSumLoadAndCapacityHelper(serviceDomain, appId, metricName, appSumLoad, size);
        }
        else
        {
            appSumLoad = -1;
            size = -1;
        }
    }
}

bool PlacementAndLoadBalancingTestHelper::CheckAutoScalingStatistics(std::wstring const & metricName, int partitionCount) const
{
    auto const& autoScaleStats = plb_.plbStatistics_.StatisticsAutoScale;

    if (metricName == *ServiceModel::Constants::SystemMetricNameCpuCores)
    {
        auto & cpuStats = autoScaleStats.CpuCoresStatistics;
        if (cpuStats.PartitionCount != partitionCount)
        {
            Trace.WriteError("PlacementAndLoadBalancingTestHelper",
                "CheckAutoScalingStatistics: Number of partitions does not match, metric={0} expected={1} actual={2}",
                metricName,
                partitionCount,
                cpuStats.PartitionCount);
            return false;
        }
    }
    else if (metricName == *ServiceModel::Constants::SystemMetricNameMemoryInMB)
    {
        auto & memoryStats = autoScaleStats.MemoryInMBStatistics;
        if (memoryStats.PartitionCount != partitionCount)
        {
            Trace.WriteError("PlacementAndLoadBalancingTestHelper",
                "CheckAutoScalingStatistics: Number of partitions does not match, metric={0} expected={1} actual={2}",
                metricName,
                partitionCount,
                memoryStats.PartitionCount);
            return false;
        }
    }
    else
    {
        auto & customStats = autoScaleStats.CustomMetricStatistics;
        if (customStats.PartitionCount != partitionCount)
        {
            Trace.WriteError("PlacementAndLoadBalancingTestHelper",
                "CheckAutoScalingStatistics: Number of partitions does not match, metric={0} expected={1} actual={2}",
                metricName,
                partitionCount,
                customStats.PartitionCount);
            return false;
        }
    }

    return true;
}

bool PlacementAndLoadBalancingTestHelper::CheckRGSPStatistics(uint64_t numSPs, uint64_t numGovernedSPs) const
{
    auto const& rgStats = plb_.plbStatistics_.StatisticsRG;
    if (numSPs != rgStats.ServicePackageCount)
    {
        Trace.WriteError("PlacementAndLoadBalancingTestHelper",
            "CheckRGSPStatistics: Number of service packages not correct, expected={0} actual={1}",
            numSPs,
            rgStats.ServicePackageCount);
        return false;
    }
    if (numGovernedSPs != rgStats.GovernedServicePackageCount)
    {
        Trace.WriteError("PlacementAndLoadBalancingTestHelper",
            "CheckRGSPStatistics: Number of governed service packages not correct, expected={0} actual={1}",
            numGovernedSPs,
            rgStats.GovernedServicePackageCount);
        return false;
    }
    return true;
}

bool PlacementAndLoadBalancingTestHelper::CheckRGServiceStatistics(uint64_t numShared, uint64_t numExclusive) const
{
    auto const& rgStats = plb_.plbStatistics_.StatisticsRG;
    if (numShared != rgStats.SharedServicesCount)
    {
        Trace.WriteError("PlacementAndLoadBalancingTestHelper",
            "CheckRGServiceStatistics: Number of shared services not correct, expected={0} actual={1}",
            numShared,
            rgStats.SharedServicesCount);
        return false;
    }
    if (numExclusive != rgStats.ExclusiveServicesCount)
    {
        Trace.WriteError("PlacementAndLoadBalancingTestHelper",
            "CheckRGServiceStatistics: Number of exclusive services not correct, expected={0} actual={1}",
            numExclusive,
            rgStats.ExclusiveServicesCount);
        return false;
    }
    return true;
}

bool PlacementAndLoadBalancingTestHelper::CheckRGMetricStatistics(
    std::wstring const & name,
    double usage,
    double minUsage,
    double maxUsage,
    int64 capacity) const
{
    auto const& rgStats = plb_.plbStatistics_.StatisticsRG;
    auto const& metricStats = name == *ServiceModel::Constants::SystemMetricNameCpuCores ? rgStats.CpuCoresStatistics : rgStats.MemoryInMBStatistics;
    if (usage > 0.0 && fabs(usage - metricStats.ResourceUsed) > 0.00001)
    {
        Trace.WriteError("PlacementAndLoadBalancingTestHelper",
            "CheckRGMetricStatistics: Total usage not correct for metric {0}, expected={1} actual={2}",
            name,
            usage,
            metricStats.ResourceUsed);
        return false;
    }
    if (minUsage > 0.0 && fabs(minUsage - metricStats.MinRequest) > 0.00001)
    {
        Trace.WriteError("PlacementAndLoadBalancingTestHelper",
            "CheckRGMetricStatistics: Minimum usage not correct for metric {0}, expected={1} actual={2}",
            name,
            minUsage,
            metricStats.MinRequest);
        return false;
    }
    if (maxUsage > 0.0 && fabs(maxUsage - metricStats.MaxRequest) > 0.00001)
    {
        Trace.WriteError("PlacementAndLoadBalancingTestHelper",
            "CheckRGMetricStatistics: Maximum usage not correct for metric {0}, expected={1} actual={2}",
            name,
            maxUsage,
            metricStats.MaxRequest);
        return false;
    }
    if (capacity > 0 && static_cast<uint64_t>(capacity) != metricStats.ResourceTotal)
    {
        Trace.WriteError("PlacementAndLoadBalancingTestHelper",
            "CheckRGMetricStatistics: Capacity not correct for metric {0}, expected={1} actual={2}",
            name,
            capacity,
            metricStats.ResourceTotal);
        return false;
    }
    return true;
}

bool PlacementAndLoadBalancingTestHelper::CheckDefragStatistics(
    uint64 numberOfBalancingMetrics,
    uint64 numberOfBalancingReservationMetrics,
    uint64 numberOfReservationMetrics,
    uint64 numberOfPackReservationMetrics,
    uint64 numberOfDefragMetrics) const
{
    auto const & defragStats = plb_.plbStatistics_.StatisticsDefrag;
    if (numberOfBalancingMetrics != defragStats.BalancingMetricsCount || numberOfBalancingReservationMetrics != defragStats.BalancingReservationMetricsCount ||
        numberOfReservationMetrics != defragStats.ReservationMetricsCount || numberOfPackReservationMetrics != defragStats.PackReservationMetricsCount ||
        numberOfDefragMetrics != defragStats.DefragMetricsCount)
    {
        return false;
    }
    return true;
}

int PlacementAndLoadBalancingTestHelper::GetPartitionCountChangeForService(std::wstring const & serviceName) const
{
    int count = 0;
    int result = INT_MAX;
    for (auto repartition : plb_.PendingAutoScalingRepartitions)
    {
        if (repartition.ServiceName == serviceName)
        {
            ++count;
            result = repartition.Change;
        }
    }
    if (count > 1)
    {
        return INT_MAX;
    }
    return result;
}

void PlacementAndLoadBalancingTestHelper::ClearPendingRepartitions()
{
    plb_.PendingAutoScalingRepartitions.clear();
}

void PlacementAndLoadBalancingTestHelper::InduceRepartitioningFailure(std::wstring const & serviceName, Common::ErrorCode error)
{
    auto serviceId = plb_.GetServiceId(serviceName);
    ASSERT_IF(serviceId == 0, "Service not found in PlacementAndLoadBalancingTestHelper::InduceRepartitioningFailure");
    AcquireExclusiveLock grab(plb_.autoScalingFailedOperationsVectorsLock_);
    plb_.pendingAutoScalingFailedRepartitions_.push_back(make_pair(serviceId, error));
}

int64 PlacementAndLoadBalancingTestHelper::GetInBuildCountPerNode(int node, std::wstring metricName)
{
    if (plb_.serviceDomainTable_.size() == 0 || (metricName == L"" && plb_.serviceDomainTable_.size() != 1))
    {
        // Any negative number means that:
        //  - Either there are no service domains.
        //  - Or metric does not belong to any domain.
        return -314;
    }

    auto itDomain = plb_.serviceDomainTable_.begin();

    if (metricName != L"")
    {
        auto itMetricToDomain = plb_.metricToDomainTable_.find(metricName);
        if (itMetricToDomain == plb_.metricToDomainTable_.end())
        {
            return -314;
        }
        itDomain = itMetricToDomain->second;
    }

    Federation::NodeId nodeId = Federation::NodeId(Common::LargeInteger(0, node));

    auto itIndex = plb_.nodeToIndexMap_.find(nodeId);
    if (itIndex == plb_.nodeToIndexMap_.end())
    {
        return -314;
    }

    uint64_t nodeIndex = itIndex->second;

    auto itNode = itDomain->second.InBuildCountPerNode.find(nodeIndex);

    if (itNode == itDomain->second.InBuildCountPerNode.end())
    {
        // No IB replicas on this node.
        return 0;
    }

    return static_cast<int64>(itNode->second);
}

void PlacementAndLoadBalancingTestHelper::ResetTiming()
{
    RefreshTime = 0;
}

void PlacementAndLoadBalancingTestHelper::UpdateAvailableImagesPerNode(std::wstring const& nodeId, std::vector<std::wstring> const& images)
{
    plb_.UpdateAvailableImagesPerNode(nodeId, images);
}
