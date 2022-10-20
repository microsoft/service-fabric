// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "PlacementAndLoadBalancing.h"
#include "PLBConfig.h"
#include "PLBDiagnostics.h"
#include "Placement.h"
#include "Searcher.h"
#include "Checker.h"
#include "IFailoverManager.h"
#include "LoadOrMoveCostDescription.h"
#include "SystemState.h"
#include "FailoverUnitMovement.h"
#include "TraceHelpers.h"
#include "LoadMetric.h"
#include "TempSolution.h"

#include "client/ClientPointers.h"
#include "client/INotificationClientSettings.h"
#include "client/FabricClientInternalSettingsHolder.h"
#include "client/HealthReportingComponent.h"
#include "client/HealthReportingTransport.h"
#include "client/client.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

class PlacementAndLoadBalancing::ServiceDomainStats
{
    DENY_COPY(ServiceDomainStats);

public:
    ServiceDomainStats()
        : nodeCount_(0),
        serviceCount_(0),
        partitionCount_(0),
        existingReplicaCount_(0),
        toBeCreatedReplicaCount_(0),
        movableReplicaCount_(0),
        inTransitionPartitionCount_(0),
        partitionsWithNewReplicaCount_(0),
        partitionsWithInstOnEveryNodeCount_(0)
    {
    }

    void Update(SystemState const& state)
    {
        serviceCount_ += state.ServiceCount;
        partitionCount_ += state.FailoverUnitCount;
        existingReplicaCount_ += state.ExistingReplicaCount();
        toBeCreatedReplicaCount_ += state.NewReplicaCount;
        movableReplicaCount_ += state.MovableReplicaCount;
        inTransitionPartitionCount_ += state.InTransitionPartitionCount;
        partitionsWithNewReplicaCount_ += state.PartitionsWithNewReplicaCount;
        partitionsWithInstOnEveryNodeCount_ += state.PartitionsWithInstOnEveryNodeCount;
    }

    int64 nodeCount_;
    int64 serviceCount_;
    int64 partitionCount_;
    int64 existingReplicaCount_;
    int64 toBeCreatedReplicaCount_;
    int64 movableReplicaCount_;
    int64 inTransitionPartitionCount_;
    int64 partitionsWithNewReplicaCount_;
    int64 partitionsWithInstOnEveryNodeCount_;
};

Global<PLBEventSource> const PlacementAndLoadBalancing::PLBTrace = make_global<PLBEventSource>(TraceTaskCodes::PLB);
Global<PLBEventSource> const PlacementAndLoadBalancing::PLBMTrace = make_global<PLBEventSource>(TraceTaskCodes::PLBM);
Global<PLBEventSource> const PlacementAndLoadBalancing::CRMTrace = make_global<PLBEventSource>(TraceTaskCodes::CRM);
Global<PLBEventSource> const PlacementAndLoadBalancing::MCRMTrace = make_global<PLBEventSource>(TraceTaskCodes::MCRM);

PlacementAndLoadBalancing::PlacementAndLoadBalancing(
    IFailoverManager & failoverManager,
    ComponentRoot const& root,
    bool isMaster,
    bool movementEnabled,
    vector<NodeDescription> && nodes,
    vector<ApplicationDescription> && applications,
    vector<ServiceTypeDescription> && serviceTypes,
    vector<ServiceDescription> && services,
    vector<FailoverUnitDescription> && failoverUnits,
    vector<LoadOrMoveCostDescription> && loadOrMoveCosts,
    Client::HealthReportingComponentSPtr const & healthClient,
    Api::IServiceManagementClientPtr const& serviceManagementClient,
    Common::ConfigEntry<bool> const & isSingletonReplicaMoveAllowedDuringUpgradeEntry)
    : RootedObject(root),
    failoverManager_(failoverManager),
    isMaster_(isMaster),
    searcher_(nullptr),
    stopSearching_(false),
    disposed_(false),
    constraintCheckEnabled_(movementEnabled),
    balancingEnabled_(movementEnabled),
    isSingletonReplicaMoveAllowedDuringUpgradeEntry_(isSingletonReplicaMoveAllowedDuringUpgradeEntry),
    lastPeriodicTrace_(StopwatchTime::Zero),
    nextServiceToBeTraced_(0),
    nextNodeToBeTraced_(UINT64_MAX),
    nextApplicationToBeTraced_(0),
    healthClient_(healthClient),
    serviceManagementClient_(serviceManagementClient),
    droppedMovementQueue_(),
    nodePropertyConstraintKeyParams_(),
    expressionCache_(nullptr),
    serviceNameToPlacementConstraintTable_(),
    verbosePlacementHealthReportingEnabled_(true),
    plbDiagnosticsSPtr_(make_shared<PLBDiagnostics>(healthClient_, verbosePlacementHealthReportingEnabled_, nodes_, serviceDomainTable_, Trace)),
    bufferUpdateLock_(false),
    dummyServiceType_(ServiceTypeDescription(wstring(L""), set<Federation::NodeId>())),
    globalBalancingMovementCounter_(PLBConfig::GetConfig().GlobalMovementThrottleCountingInterval),
    globalPlacementMovementCounter_(PLBConfig::GetConfig().GlobalMovementThrottleCountingInterval),
    nextServiceId_(1),
    nextApplicationId_(1),
    nextServicePackageIndex_(1),
    upNodeCount_(0),
    interruptSearcherRunThread_(nullptr),
    interruptSearcherRunFinished_(false),
    domainToSplit_(L""),
    settings_(),
    rgDomainId_(),
    defaultDomainId_(Guid::NewGuid().ToString()),
    tracingMovementsJobQueueUPtr_(nullptr),
    tracingJobQueueFinished_(false),
    testTracingLock_(),
    testTracingBarrier_(testTracingLock_),
    lastStatisticsTrace_(StopwatchTime::Zero),
    plbStatistics_(),
    WriteWarning(isMaster ? Common::TraceTaskCodes::PLBM : Common::TraceTaskCodes::PLB, LogLevel::Warning)
{
    Trace.PLBConstruct(static_cast<int64>(nodes.size()), static_cast<int64>(serviceTypes.size()), static_cast<int64>(services.size()), static_cast<int64>(failoverUnits.size()), static_cast<int64>(loadOrMoveCosts.size()));

    for (auto itNode = nodes.begin(); itNode != nodes.end(); ++itNode)
    {
        itNode->CorrectCpuCapacity();
        Federation::NodeId nodeId = itNode->NodeId;
        Trace.UpdateNode(nodeId, *itNode);
        UpdateTotalClusterCapacity(*itNode, *itNode, true);
        nodeToIndexMap_.insert(make_pair(nodeId, nodes_.size()));
        itNode->NodeIndex = nodes_.size();
        if (itNode->IsUp)
        {
            upNodeCount_++;
        }
        plbStatistics_.AddNode(*itNode);
        nodes_.push_back(Node(move(*itNode)));
        if (nodes_.size() == 1) { nextNodeToBeTraced_ = 0; }
    }

    UpdateNodePropertyConstraintKeyParams();

    metricConnections_.SetPLB(this);

    for (auto applicationDescription : applications)
    {
        UpdateApplication(move(applicationDescription));
    }

    for (auto itServiceType = serviceTypes.begin(); itServiceType != serviceTypes.end(); ++itServiceType)
    {
        wstring serviceTypeName(itServiceType->Name);
        Trace.UpdateServiceType(serviceTypeName, *itServiceType);
        serviceTypeTable_.insert(make_pair(move(serviceTypeName), ServiceType(move(*itServiceType))));
    }

    for (auto itService = services.begin(); itService != services.end(); ++itService)
    {
        // Forced update: don't check for capacity, report health error in case of affinity chain or placement constraint.
        InternalUpdateService(move(*itService), true);
    }

    for (auto itFailoverUnit = failoverUnits.begin(); itFailoverUnit != failoverUnits.end(); ++itFailoverUnit)
    {
        InternalUpdateFailoverUnit(move(*itFailoverUnit));
    }

    for (auto itLoad = loadOrMoveCosts.begin(); itLoad != loadOrMoveCosts.end(); ++itLoad)
    {
        InternalUpdateLoadOrMoveCost(move(*itLoad));
    }

    tracingMovementsJobQueueUPtr_ = make_unique<TracingMovementsJobQueue>(
        isMaster ? L"PLBM" : L"PLB",
        *this,
        false,
        1,
        tracingJobQueueFinished_,
        testTracingLock_,
        testTracingBarrier_);

    auto componentRoot = Root.CreateComponentRoot();
    timer_ = Common::Timer::Create(
        "PLB.Refresh",
        [this, componentRoot](Common::TimerSPtr const&) mutable { this->Refresh(Stopwatch::Now()); }, true);
    processPendingUpdatesTimer_ = Common::Timer::Create(
        "PLB.ProcessPendingUpdatesPeriodicTask",
        [this, componentRoot](Common::TimerSPtr const&) mutable { this->ProcessPendingUpdatesPeriodicTask(); }, true);

    PLBConfig const& config = PLBConfig::GetConfig();

    randomSeed_ = config.InitialRandomSeed != -1 ? config.InitialRandomSeed : static_cast<int>(DateTime::Now().Ticks);

    timer_->Change(config.PLBRefreshInterval);
    processPendingUpdatesTimer_->Change(config.ProcessPendingUpdatesInterval);

    std::wstring guidString = StringUtility::ToWString(Guid::NewGuid());
    std::wstring performanceCounterInstanceName = isMaster ? guidString + L"-PLBM:" + StringUtility::ToWString(DateTime::Now().Ticks) : guidString + L"-PLB:" + StringUtility::ToWString(DateTime::Now().Ticks);
    plbCounterSPtr_ = LoadBalancingCounters::CreateInstance(performanceCounterInstanceName);
}

PlacementAndLoadBalancing::~PlacementAndLoadBalancing()
{
}

void PlacementAndLoadBalancing::SetMovementEnabled(bool constraintCheckEnabled, bool balancingEnabled)
{
    bool isDummyPLB = settings_.DummyPLBEnabled;

    if (IsDisposed())
    {
        return;
    }

    if (constraintCheckEnabled_.load() == constraintCheckEnabled && balancingEnabled_.load() == balancingEnabled)
    {
        return;
    }

    AcquireWriteLock grab(lock_);
    StopwatchTime now = Stopwatch::Now();

    constraintCheckEnabled_.store(constraintCheckEnabled);
    if (!isDummyPLB)
    {
        balancingEnabled_.store(balancingEnabled);
    }

    ForEachDomain([=](ServiceDomain & d)
    {
        d.SetMovementEnabled(constraintCheckEnabled, balancingEnabled, isDummyPLB, now);
    });

    Trace.UpdateMovementEnabled(constraintCheckEnabled, balancingEnabled);
    StopSearcher();
}

void PlacementAndLoadBalancing::UpdateNode(NodeDescription && nodeDescription)
{
    if (IsDisposed())
    {
        return;
    }

    LockAndSetBooleanLock grab(bufferUpdateLock_);

    nodeDescription.CorrectCpuCapacity();
    pendingNodeUpdates_.push_back(make_pair(move(nodeDescription), Stopwatch::Now()));
}

void PlacementAndLoadBalancing::ProcessUpdateNode(NodeDescription && nodeDescription, StopwatchTime timeStamp)
{
    Federation::NodeId nodeId = nodeDescription.NodeId;
    auto itNodeId = nodeToIndexMap_.find(nodeId);
    uint64 nodeIndex = UINT64_MAX;
    if (itNodeId != nodeToIndexMap_.end())
    {
        // Existing node, just reuse the index.
        nodeIndex = itNodeId->second;
        nodeDescription.NodeIndex = nodeIndex;
    }

    if (nodeDescription.NodeIndex == UINT64_MAX)
    {
        // New node added to PLB
        plbStatistics_.AddNode(nodeDescription);
        UpdateTotalClusterCapacity(nodeDescription, nodeDescription, true);

        if (nodeDescription.IsUp)
        {
            upNodeCount_++;
        }

        nodeIndex = nodes_.size();
        nodeDescription.NodeIndex = nodeIndex;
        nodeToIndexMap_.insert(make_pair(nodeDescription.NodeId, nodeIndex));
        nodes_.push_back(Node(move(nodeDescription)));
        if (nodes_.size() == 1) { nextNodeToBeTraced_ = 0; }

        //Efficiently Update Cached NodeProperties in nodePropertyConstraintKeyParams_
        auto & nodeProp = nodes_[nodeIndex].NodeDescriptionObj.NodeProperties;
        for (auto it = nodeProp.begin(); it != nodeProp.end(); ++it)
        {
            nodePropertyConstraintKeyParams_.insert(std::make_pair(it->first, L"0"));
        }

        if (nodes_[nodeIndex].NodeDescriptionObj.IsUp)
        {
            ForEachDomain([=](ServiceDomain & d) {d.OnNodeUp(nodeIndex, timeStamp); });
        }
    }
    else
    {
        ASSERT_IF((nodeDescription.NodeInstance < nodes_[nodeIndex].NodeDescriptionObj.NodeInstance) && (!settings_.DummyPLBEnabled), "Invalid node instance {0}", nodeDescription);

        plbStatistics_.UpdateNode(nodes_[nodeIndex].NodeDescriptionObj, nodeDescription);
        UpdateTotalClusterCapacity(nodes_[nodeIndex].NodeDescriptionObj, nodeDescription, false);

        if (nodeDescription.IsUp && !nodes_[nodeIndex].NodeDescriptionObj.IsUp)
        {
            // Down --> Up
            upNodeCount_++;
        }
        else if (!nodeDescription.IsUp && nodes_[nodeIndex].NodeDescriptionObj.IsUp)
        {
            // Up --> Down
            upNodeCount_--;
        }

        if (nodeDescription.NodeInstance == nodes_[nodeIndex].NodeDescriptionObj.NodeInstance)
        {
            if (!nodeDescription.IsUp)
            {
                ForEachDomain([=](ServiceDomain & d) {d.OnNodeDown(nodeIndex, timeStamp); });
            }
            else if (nodeDescription.IsDeactivated != nodes_[nodeIndex].NodeDescriptionObj.IsDeactivated)
            {
                ForEachDomain([=](ServiceDomain & d) {d.OnNodeChanged(nodeIndex, timeStamp); });
            }

        }
        else
        {
            if (nodes_[nodeIndex].NodeDescriptionObj.IsUp)
            {
                ForEachDomain([=](ServiceDomain & d) {d.OnNodeDown(nodeIndex, timeStamp); });
            }

            ForEachDomain([=](ServiceDomain & d) {d.OnNodeUp(nodeIndex, timeStamp); });
            if (!nodeDescription.IsUp)
            {
                ForEachDomain([=](ServiceDomain & d) {d.OnNodeDown(nodeIndex, timeStamp); });
            }
        }

        nodes_[nodeIndex].UpdateDescription(move(nodeDescription));

        //Update Cached NodeProperties
        UpdateNodePropertyConstraintKeyParams();
    }

    // TODO: do we need to keep the capacityratio and capacity data If a node is down?
    Trace.UpdateNode(nodeDescription.NodeId, nodes_[nodeIndex].NodeDescriptionObj);
    StopSearcher();
}

void PlacementAndLoadBalancing::ProcessUpdateNodeImages(Federation::NodeId const& nodeId, vector<wstring>&& nodeImages)
{
    auto itNodeId = nodeToIndexMap_.find(nodeId);
    if (itNodeId != nodeToIndexMap_.end())
    {
        // node should always exist in PLB
        auto nodeIndex = itNodeId->second;
        nodes_[nodeIndex].UpdateNodeImages(move(nodeImages));
        Trace.UpdateNodeImages(wformatString(
            "Updating node {0} with images {1}", nodeId, nodes_[nodeIndex].NodeImages));
    }
}

Common::ErrorCode PlacementAndLoadBalancing::InternalUpdateServicePackageCallerHoldsLock(ServicePackageDescription && description, bool & changed)
{
    auto itServicePackageId = servicePackageToIdMap_.find(description.ServicePackageIdentifier);
    std::map<uint64, ServicePackage>::iterator itServicePackage = servicePackageTable_.end();

    if (itServicePackageId == servicePackageToIdMap_.end())
    {
        // Bookkeeping for statistics
        plbStatistics_.AddServicePackage(description);
        auto servicePackageId = nextServicePackageIndex_;
        servicePackageToIdMap_.insert(make_pair(description.ServicePackageIdentifier, nextServicePackageIndex_++));
        itServicePackage = servicePackageTable_.insert(make_pair(servicePackageId, ServicePackage(move(description)))).first;
        changed = true;
    }
    else
    {
        itServicePackage = servicePackageTable_.find(itServicePackageId->second);
        if (itServicePackage != servicePackageTable_.end())
        {
            changed = itServicePackage->second.Description != description;

            if (changed)
            {
                //make a copy here because update service will do inserts in this set
                auto allServices = itServicePackage->second.Services;
                bool isNewSPGoverned = description.RequiredResources.size() > 0;
                auto resources = description.CorrectedRequiredResources;

                plbStatistics_.UpdateServicePackage(itServicePackage->second.Description, description);

                auto UpdateServices = [&]()
                {
                    for (auto serviceId : allServices)
                    {
                        auto itServiceDomain = serviceToDomainTable_.find(serviceId);
                        if (itServiceDomain != serviceToDomainTable_.end())
                        {
                            auto itService = itServiceDomain->second->second.Services.find(serviceId);
                            if (itService != itServiceDomain->second->second.Services.end())
                            {
                                ServiceDescription sd(itService->second.ServiceDesc);
                                sd.UpdateRGMetrics(resources);
                                InternalUpdateService(move(sd), true, false, true, true);
                            }
                        }
                    }
                };

                auto UpdateLoads = [&](bool isAdd)
                {
                    for (auto serviceId : allServices)
                    {
                        auto itServiceDomain = serviceToDomainTable_.find(serviceId);
                        if (itServiceDomain != serviceToDomainTable_.end())
                        {
                            itServiceDomain->second->second.UpdateServicePackageLoad(serviceId, isAdd);
                        }
                    }
                };

                //there are 3 cases here
                //FIRST case when we didn't have RG and now we add it, we need to first add the new metrics and do merge/split
                //after that we adjust the load for shared services (as for exclusive update service will do it)
                //SECOND case we just changed some RG values, but we keep ALL metrics so we can do this in any order
                //as update service will do it for exclusive services regardless of service package description update
                //THIRD case we no longer have RG and we used to have it, we first need to remove the load and then
                //update the metrics as updating will remove the RG metrics so we would not be able to remove shared services load
                //when we remove just a single metric but keep the other we still have both metrics in the vector just the removed one has load 0
                if (isNewSPGoverned)
                {
                    UpdateServices();
                    UpdateLoads(false);
                    itServicePackage->second.UpdateDescription(move(description));
                    UpdateLoads(true);
                }
                else
                {
                    UpdateLoads(false);
                    itServicePackage->second.UpdateDescription(move(description));
                    UpdateLoads(true);
                    UpdateServices();
                }

                StopSearcher();
            }
        }
    }

    if (changed)
    {
        Trace.UpdateServicePackage(itServicePackage->second.Description.Name, itServicePackage->second.Description);
    }

    return ErrorCodeValue::Success;
}

void PlacementAndLoadBalancing::InternalDeleteServicePackageCallerHoldsLock(ServiceModel::ServicePackageIdentifier const & servicePackageIdentifier)
{
    auto itServicePackageId = servicePackageToIdMap_.find(servicePackageIdentifier);
    if (itServicePackageId != servicePackageToIdMap_.end())
    {
        if (rgDomainId_ != L"")
        {
            wstring prefix = ServiceDomain::GetDomainIdPrefix(rgDomainId_);
            auto sdIterator = serviceDomainTable_.find(prefix);
            if (sdIterator != serviceDomainTable_.end())
            {
                sdIterator->second.RemoveServicePackageInfo(itServicePackageId->second);
            }
        }
        auto itServicePackage = servicePackageTable_.find(itServicePackageId->second);
        if (itServicePackage != servicePackageTable_.end())
        {
            plbStatistics_.RemoveServicePackage(itServicePackage->second.Description);
            servicePackageTable_.erase(itServicePackage);
        }
        servicePackageToIdMap_.erase(itServicePackageId);
        Trace.DeleteServicePackage(servicePackageIdentifier.ServicePackageName);
    }
}

void PlacementAndLoadBalancing::UpdateServiceType(ServiceTypeDescription && serviceTypeDescription)
{
    if (IsDisposed())
    {
        return;
    }

    AcquireWriteLock grab(lock_);

    wstring serviceTypeName = serviceTypeDescription.Name;
    bool changed = false;
    bool isBlockListEmpty = serviceTypeDescription.BlockList.empty();

    auto itServiceType = serviceTypeTable_.find(serviceTypeDescription.Name);
    if (itServiceType == serviceTypeTable_.end())
    {
        itServiceType = serviceTypeTable_.insert(make_pair(serviceTypeName, ServiceType(move(serviceTypeDescription)))).first;

        changed = true;
    }
    else
    {
        changed = itServiceType->second.UpdateDescription(move(serviceTypeDescription));
    }

    if (changed)
    {
        Trace.UpdateServiceType(itServiceType->first, itServiceType->second.ServiceTypeDesc);

        if (!isBlockListEmpty)
        {
            StopSearcher();
        }

        for (auto it = serviceDomainTable_.begin(); it != serviceDomainTable_.end(); ++it)
        {
            it->second.OnServiceTypeChanged(serviceTypeName);
        }
    }
}

void PlacementAndLoadBalancing::UpdateClusterUpgrade(bool isUpgradeInProgress, std::set<std::wstring> && completedUDs)
{
    if (IsDisposed())
    {
        return;
    }

    Trace.UpdateUpgradeCompletedUDs(L"cluster", isUpgradeInProgress, UDsToString(completedUDs));

    AcquireWriteLock grab(lock_);

    clusterUpgradeInProgress_.store(isUpgradeInProgress);

    // UDs are used as hint and searcher shouldn't be stopped, so it is not needed to check if changed
    if (false == clusterUpgradeInProgress_.load())
    {
        upgradeCompletedUDs_.clear();
    }
    else
    {
        upgradeCompletedUDs_ = move(completedUDs);
    }
}

std::wstring PlacementAndLoadBalancing::UDsToString(std::set<std::wstring> UDs)
{
    wstring udStr = L"";
    for (auto it = UDs.begin(); it != UDs.end(); ++it)
    {
        udStr.append(*it);
        udStr.append(L" ");
    }

    return udStr;
}


Common::ErrorCode PlacementAndLoadBalancing::UpdateApplication(ApplicationDescription && applicationDescription, bool forceUpdate)
{
    if (IsDisposed())
    {
        return ErrorCodeValue::PLBNotReady;
    }

    wstring applicationName = applicationDescription.Name;

    AcquireWriteLock grab(lock_);

    if (applicationDescription.ApplicationId == 0)
    {
        auto appIter = applicationToIdMap_.find(applicationName);
        if (appIter == applicationToIdMap_.end())
        {
            // This is new application, so we need to assign new ID to it.
            appIter = applicationToIdMap_.insert(make_pair(applicationName, nextApplicationId_++)).first;
        }
        applicationDescription.ApplicationId = appIter->second;
    }

    uint64 applicationId = applicationDescription.ApplicationId;

    bool changed = false;
    bool appGroupsAdded = false;
    bool appGroupsRemoved = false;
    bool scaleoutChanged = false;
    bool maxInstanceCapChanged = false;

    vector<wstring> applicationMetrics;
    vector<wstring> oldApplicationMetrics;

    if (!forceUpdate && !CheckClusterResourceForReservation(applicationDescription))
    {
        return ErrorCodeValue::InsufficientClusterCapacity;
    }

    auto itApplication = applicationTable_.find(applicationDescription.ApplicationId);
    if (itApplication == applicationTable_.end())
    {
        // This is a new application.
        // Add to total reserved capacity
        UpdateAppReservedCapacity(applicationDescription, true);

        // Add to application table
        itApplication = applicationTable_.insert(make_pair(applicationDescription.ApplicationId, Application(move(applicationDescription)))).first;
        if (applicationTable_.size() == 1)
        {
            nextApplicationToBeTraced_ = applicationTable_.begin()->first;
            for (auto it = serviceDomainTable_.begin(); it != serviceDomainTable_.end(); it++)
            {
                it->second.LastApplicationTraced = applicationTable_.begin()->first;
            }
        }

        changed = true;

        if (itApplication->second.ApplicationDesc.HasScaleoutOrCapacity())
        {
            appGroupsAdded = true;
            map<wstring, ApplicationCapacitiesDescription> const& appCapacities = itApplication->second.ApplicationDesc.AppCapacities;
            for (auto itMetric = appCapacities.begin(); itMetric != appCapacities.end(); ++itMetric)
            {
                applicationMetrics.push_back(itMetric->second.MetricName);
            }
        }

        bool anyPackageChanged = false;
        for (auto & spIt : itApplication->second.ApplicationDesc.ServicePackages)
        {
            bool packagedChanged = false;
            InternalUpdateServicePackageCallerHoldsLock(ServicePackageDescription(spIt.second), packagedChanged);
            anyPackageChanged = anyPackageChanged || packagedChanged;
        }
        //this is a new application but it might be already in upgrade so we need to check for resources in that case
        if (itApplication->second.ApplicationDesc.UpgradeInProgess && anyPackageChanged && itApplication->second.ApplicationDesc.IsPLBSafetyCheckRequiredForUpgrade)
        {
            Trace.ApplicationInRGChange(applicationDescription);
            applicationsInUpgradeCheckRg_.insert(applicationDescription.ApplicationId);
        }
    }
    else
    {
        // Check which SPs need to be changed.
        set<ServiceModel::ServicePackageIdentifier> updatedSPs;
        set<ServiceModel::ServicePackageIdentifier> deletedSPs;
        bool anyPackageChanged = false;
        itApplication->second.GetChangedServicePackages(applicationDescription, deletedSPs, updatedSPs);
        for (auto deletedSPId : deletedSPs)
        {
            InternalDeleteServicePackageCallerHoldsLock(deletedSPId);
        }
        for (auto updatedSPId : updatedSPs)
        {
            auto updatedSpDesc = applicationDescription.ServicePackages.find(updatedSPId);
            ASSERT_IF(updatedSpDesc == applicationDescription.ServicePackages.end(),
                "Service Package {0} not found in applications SPs. Application description: {1}",
                updatedSPId,
                applicationDescription);
            bool packageChanged = false;
            InternalUpdateServicePackageCallerHoldsLock(ServicePackageDescription(updatedSpDesc->second), packageChanged);
            anyPackageChanged = anyPackageChanged || packageChanged;
        }
        //if there is an ongoing upgrade and we have some packages that have changed we need to queue a safety check
        //this will happen only the first time we receive an update for an application that is changing RG so we will not queue it more than once
        //because we will save the new resource governance policy with the first update
        if (applicationDescription.UpgradeInProgess && anyPackageChanged && applicationDescription.IsPLBSafetyCheckRequiredForUpgrade)
        {
            Trace.ApplicationInRGChange(applicationDescription);
            applicationsInUpgradeCheckRg_.insert(applicationDescription.ApplicationId);
        }

        //this app is no longer in upgrade, so if it was canceled before we finished the safety check no need to keep checking for it
        if (!applicationDescription.UpgradeInProgess)
        {
            applicationsInUpgradeCheckRg_.erase(applicationDescription.ApplicationId);
        }

        // Application already exists
        // Delete the old app reserved load
        UpdateAppReservedCapacity(itApplication->second.ApplicationDesc, false);
        // Add reserved load in the new app description
        UpdateAppReservedCapacity(applicationDescription, true);

        std::map<wstring, uint64> applicationReservationMetrics;
        std::map<wstring, uint64> oldApplicationReservationMetrics;
        std::set<wstring> allMetricsWithReservation;
        std::map<wstring, uint64> applicationMaxInstanceCapMetrics;
        std::map<wstring, uint64> oldApplicationMaxInstanceCapMetrics;
        std::set<wstring> allMetricsWithMaxInstanceCapMetrics;

        //capacities were merely modified...
        if (itApplication->second.ApplicationDesc.HasScaleoutOrCapacity())
        {
            if (!applicationDescription.HasScaleoutOrCapacity())
            {
                appGroupsRemoved = true;
            }
            map<wstring, ApplicationCapacitiesDescription> const& oldAppCapacities = itApplication->second.ApplicationDesc.AppCapacities;
            for (auto itMetric = oldAppCapacities.begin(); itMetric != oldAppCapacities.end(); ++itMetric)
            {
                oldApplicationMetrics.push_back(itMetric->second.MetricName);

                // add old reservation for this metric
                if (itMetric->second.ReservationCapacity > 0)
                {
                    oldApplicationReservationMetrics.insert(make_pair(itMetric->second.MetricName, itMetric->second.ReservationCapacity));
                    allMetricsWithReservation.insert(itMetric->second.MetricName);
                }

                // add old max instance capacity for this metric
                if (itMetric->second.MaxInstanceCapacity > 0)
                {
                    oldApplicationMaxInstanceCapMetrics.insert(make_pair(itMetric->second.MetricName, itMetric->second.MaxInstanceCapacity));
                    allMetricsWithMaxInstanceCapMetrics.insert(itMetric->second.MetricName);
                }
            }
        }
        if (applicationDescription.HasScaleoutOrCapacity())
        {
            // If the application description has changes of scaleout or capacity
            // Placement creator can do the update
            if (!itApplication->second.ApplicationDesc.HasScaleoutOrCapacity())
            {
                appGroupsAdded = true;
            }
            map<wstring, ApplicationCapacitiesDescription> const& appCapacities = applicationDescription.AppCapacities;
            for (auto itMetric = appCapacities.begin(); itMetric != appCapacities.end(); ++itMetric)
            {
                applicationMetrics.push_back(itMetric->second.MetricName);

                // add new reservation for this metric
                if (itMetric->second.ReservationCapacity > 0)
                {
                    applicationReservationMetrics.insert(make_pair(itMetric->second.MetricName, itMetric->second.ReservationCapacity));
                    allMetricsWithReservation.insert(itMetric->second.MetricName);
                }

                // add max instance capacity for this metric
                if (itMetric->second.MaxInstanceCapacity > 0)
                {
                    applicationMaxInstanceCapMetrics.insert(make_pair(itMetric->second.MetricName, itMetric->second.MaxInstanceCapacity));
                    allMetricsWithMaxInstanceCapMetrics.insert(itMetric->second.MetricName);
                }
            }
        }

        // for each metric check if reservation has been changed
        // if reservation was changed remove FailoverUnits, change reservation and than add them back
        bool changedReservationMetric = false;
        for (auto itMetric = allMetricsWithReservation.begin(); itMetric != allMetricsWithReservation.end(); ++itMetric)
        {
            wstring currMetricName = *itMetric;

            bool existsOldReservationMetric = false;
            auto itOldReservationMetric = oldApplicationReservationMetrics.find(currMetricName);
            if (itOldReservationMetric != oldApplicationReservationMetrics.end())
            {
                existsOldReservationMetric = true;
            }

            bool existsReservationMetric = false;
            auto itReservationMetric = applicationReservationMetrics.find(currMetricName);
            if (itReservationMetric != applicationReservationMetrics.end())
            {
                existsReservationMetric = true;
            }

            // this metric exists in both old and new application decriptions
            // check if the reservation changed
            if (existsOldReservationMetric && existsReservationMetric)
            {
                if (itReservationMetric->second != itOldReservationMetric->second)
                {
                    changedReservationMetric = true;
                    break;
                }
            }
            else if (existsOldReservationMetric)
            {
                // old application contains this metric, but new one doesn't
                changedReservationMetric = true;
                break;
            }
            else if (existsReservationMetric)
            {
                // new application contains this metric, but old one doesn't
                changedReservationMetric = true;
                break;
            }
        }

        // For each metric check if capacity per node has been changed
        for (auto itMetric = allMetricsWithMaxInstanceCapMetrics.begin(); itMetric != allMetricsWithMaxInstanceCapMetrics.end(); ++itMetric)
        {
            wstring currMetricName = *itMetric;

            bool existsOldCapacityMetric = false;
            auto itOldMaxInstanceCapMetric = oldApplicationMaxInstanceCapMetrics.find(currMetricName);
            if (itOldMaxInstanceCapMetric != oldApplicationMaxInstanceCapMetrics.end())
            {
                existsOldCapacityMetric = true;
            }

            bool existsCapacityMetric = false;
            auto itMaxInstanceCapMetric = applicationMaxInstanceCapMetrics.find(currMetricName);
            if (itMaxInstanceCapMetric != applicationMaxInstanceCapMetrics.end())
            {
                existsCapacityMetric = true;
            }

            // this metric exists in both old and new application decriptions
            // check if the max instance capacity changed
            if (existsOldCapacityMetric && existsCapacityMetric)
            {
                if (itMaxInstanceCapMetric->second != itOldMaxInstanceCapMetric->second)
                {
                    maxInstanceCapChanged = true;
                    break;
                }
            }
            else if (existsOldCapacityMetric)
            {
                // old application contains this metric, but new one doesn't
                maxInstanceCapChanged = true;
                break;
            }
            else if (existsCapacityMetric)
            {
                // new application contains this metric, but old one doesn't
                maxInstanceCapChanged = true;
                break;
            }
        }

        // check if number of minimum nodes has changed
        bool isMinNodesChanged = itApplication->second.ApplicationDesc.MinimumNodes != applicationDescription.MinimumNodes;
        scaleoutChanged = itApplication->second.ApplicationDesc.ScaleoutCount != applicationDescription.ScaleoutCount;

        auto sdIterator = applicationToDomainTable_.find(applicationId);
        if (sdIterator != applicationToDomainTable_.end())
        {
            // remove old reservation load when minNodes or application reservation is changed
            if (changedReservationMetric)
            {
                // Delete all failoverUnits reservation for old application
                // No need to update UpdateApplicationReservedLoad - already done that here
                sdIterator->second->second.UpdateApplicationFailoverUnitsReservation(itApplication->second.ApplicationDesc, false);
            }
            else if (isMinNodesChanged)
            {
                // Remove the existing reservation
                sdIterator->second->second.UpdateApplicationReservedLoad(itApplication->second.ApplicationDesc, false);
            }
        }
        // change application description
        ApplicationDescription newDescription = applicationDescription;
        changed = itApplication->second.UpdateDescription(move(applicationDescription));

        if (sdIterator != applicationToDomainTable_.end())
        {
            // add new reservation load when minNodes or application reservation is changed
            if (changedReservationMetric)
            {
                // Add all failoverUnits reservation for new application
                // No need to update UpdateApplicationReservedLoad - already done that here
                sdIterator->second->second.UpdateApplicationFailoverUnitsReservation(newDescription, true);

            }
            else if (isMinNodesChanged)
            {
                // Add the new reservation
                sdIterator->second->second.UpdateApplicationReservedLoad(newDescription, true);
            }
        }
    }

    if (changed)
    {
        Trace.UpdateApplication(itApplication->second.ApplicationDesc.Name, itApplication->second.ApplicationDesc);

        if (appGroupsAdded || appGroupsRemoved || applicationMetrics != oldApplicationMetrics)
        {
            // Update the metric Connections
            // Metric connections are as follows: each service is connected to the one alphabetically before it, and the first service is connected to the application's capacity metrics...
            uint64 relatedServiceId(0);
            bool toExecuteDomainChange = false;
            if (applicationMetrics != oldApplicationMetrics)
            {
                toExecuteDomainChange |= metricConnections_.AddOrRemoveMetricConnection(applicationMetrics, true);
                //general policy is all removes after all adds, but this is kept here, since it shouldn't split any domains that are instantly merged
                toExecuteDomainChange |= metricConnections_.AddOrRemoveMetricConnection(oldApplicationMetrics, false);
            }

            auto const& services = itApplication->second.Services;
            Uint64UnorderedMap<Service>::const_iterator itAdjacentService;
            for (auto itServiceName = services.begin(); itServiceName != services.end(); itServiceName++)
            {
                auto const& serviceId = GetServiceId(*itServiceName);
                if (serviceId != 0)
                {
                    auto itServiceDomain = serviceToDomainTable_.find(serviceId);
                    if (itServiceDomain != serviceToDomainTable_.end())
                    {
                        auto itService = itServiceDomain->second->second.Services.find(serviceId);
                        if (itServiceName == services.begin())
                        {
                            auto const& serviceDesc = itService->second.ServiceDesc;
                            if (applicationMetrics != oldApplicationMetrics)
                            {
                                // The first service is connected to the application's capacity metrics...
                                toExecuteDomainChange |= metricConnections_.AddOrRemoveMetricAffinity(serviceDesc.Metrics, applicationMetrics, true);
                                toExecuteDomainChange |= metricConnections_.AddOrRemoveMetricAffinity(serviceDesc.Metrics, oldApplicationMetrics, false);
                            }
                            itAdjacentService = itService;
                        }
                        else
                        {
                            auto const& serviceDesc = itService->second.ServiceDesc;
                            auto const& adjacentServiceDesc = itAdjacentService->second.ServiceDesc;
                            // Each service is connected to the one alphabetically before it
                            if (appGroupsAdded)
                            {
                                toExecuteDomainChange |= metricConnections_.AddOrRemoveMetricAffinity(serviceDesc.Metrics, adjacentServiceDesc.Metrics, true);
                            }
                            else if (appGroupsRemoved)
                            {
                                toExecuteDomainChange |= metricConnections_.AddOrRemoveMetricAffinity(serviceDesc.Metrics, adjacentServiceDesc.Metrics, false);
                            }
                            else
                            {
                                break;
                            }
                            itAdjacentService = itService;
                        }
                    }
                    relatedServiceId = serviceId;
                }
            }

            if (toExecuteDomainChange)
            {
                ExecuteDomainChange();
            }

            if (relatedServiceId != 0)
            {
                auto iter = serviceToDomainTable_.find(relatedServiceId);
                if (iter != serviceToDomainTable_.end())
                {
                    bool newDomain = AddApplicationToDomain(itApplication->second.ApplicationDesc, iter->second, oldApplicationMetrics, true);
                    if (newDomain)
                    {
                        // Handle the case when applicaiton is added to domain after its services
                        auto const& sdIterator = applicationToDomainTable_.find(applicationId);

                        if (sdIterator != applicationToDomainTable_.end() && itApplication->second.ApplicationDesc.AppCapacities.size() > 0)
                        {
                            sdIterator->second->second.UpdateReservationForNewApplication(itApplication->second.ApplicationDesc);
                        }
                    }
                }
            }
        }


        // Remove the application group partitions from the service domain table
        if (appGroupsRemoved)
        {
            auto sdIterator = applicationToDomainTable_.find(applicationId);
            if (sdIterator != applicationToDomainTable_.end())
            {
                sdIterator->second->second.RemoveApplicationFromApplicationPartitions(applicationId);
            }
        }

        // Update constraint check closure, with partitions from the application group
        if ((scaleoutChanged || maxInstanceCapChanged) && !appGroupsRemoved)
        {
            auto sdIterator = applicationToDomainTable_.find(applicationId);
            if (sdIterator != applicationToDomainTable_.end())
            {
                sdIterator->second->second.AddAppPartitionsToPartialClosure(applicationId);
            }
        }

        if (applicationDescription.HasScaleoutOrCapacity())
        {
            StopSearcher();
        }
    }

    return ErrorCodeValue::Success;
}

void PlacementAndLoadBalancing::DeleteApplication(std::wstring const& applicationName, bool forceUpdate)
{
    // Unreferenced for now, added for consistency with UpdateApplication API.
    // This parameter is set to true when FM needs to force deletion of application when creation needs to be revered.
    // In that case, DeleteApplication must not fail because application will not be present in FM/CM.
    UNREFERENCED_PARAMETER(forceUpdate);

    if (IsDisposed())
    {
        return;
    }

    AcquireWriteLock grab(lock_);

    if (applicationToIdMap_.find(applicationName) == applicationToIdMap_.end())
    {
        // Application already deleted, just return
        return;
    }

    uint64 applicationId = applicationToIdMap_.find(applicationName)->second;
    // If application was updated not to have scaleout, then it is already deleted from PLB
    auto itApp = applicationTable_.find(applicationId);
    if (itApp != applicationTable_.end())
    {
        for (auto servicePackage : itApp->second.ApplicationDesc.ServicePackages)
        {
            InternalDeleteServicePackageCallerHoldsLock(servicePackage.first);
        }
        Trace.DeleteApplication(applicationName, itApp->second.ApplicationDesc);
        bool toExecuteDomainChange = false;
        // Since scaleout and Capacity are removed, we should try to split domains...
        // Metric connections to be removed are as follows: each service is connected to the one alphabetically before it, and the first service is connected to the application's capacity metrics...
        if (itApp->second.ApplicationDesc.HasScaleoutOrCapacity())
        {
            DBGASSERT_IF(applicationToDomainTable_.find(applicationId) != applicationToDomainTable_.end(), "Application {0} deleted without removing all it's contituent services");

            vector<wstring> applicationMetrics;
            map<wstring, ApplicationCapacitiesDescription> const& appCapacities = itApp->second.ApplicationDesc.AppCapacities;
            for (auto itMetric = appCapacities.begin(); itMetric != appCapacities.end(); ++itMetric)
            {
                applicationMetrics.push_back(itMetric->second.MetricName);
            }

            //remove the application metric connections...
            toExecuteDomainChange |= metricConnections_.AddOrRemoveMetricConnection(applicationMetrics, false);
            if (toExecuteDomainChange)
            {
                ExecuteDomainChange();
            }
        }

        UpdateAppReservedCapacity(itApp->second.ApplicationDesc, false);

        if (applicationTable_.size() > 1)
        {
            //next Application in the list...
            auto nextApp = next(itApp);
            if (nextApp == applicationTable_.end())
            {
                nextApp = applicationTable_.begin();
            }

            if (nextApplicationToBeTraced_ == applicationId)
            {
                nextApplicationToBeTraced_ = nextApp->first;
            }
            for (auto it = serviceDomainTable_.begin(); it != serviceDomainTable_.end(); it++)
            {
                if (it->second.LastApplicationTraced == applicationId)
                {
                    it->second.LastApplicationTraced = nextApp->first;
                }
            }
        }
        else if (applicationTable_.size() == 1)
        {
            nextApplicationToBeTraced_ = 0;
            auto itAppToDomain = applicationToDomainTable_.find(applicationId);
            //we might have not added it if there was no related service for this app
            if (itAppToDomain != applicationToDomainTable_.end())
            {
                (*(itAppToDomain->second)).second.LastApplicationTraced = 0;
            }
        }

        applicationsInUpgradeCheckRg_.erase(applicationId);
        applicationTable_.erase(applicationId);
        applicationToDomainTable_.erase(applicationId);
    }
}

/// <summary>
/// Updates the service metric connections for an application.
/// This function assumes that service has already been added to the applications 'services' list,
/// and makes appropriate modifications to metric connections if the application has scaleout or capacity.
/// Metric connections are as follows: each service is connected to the one alphabetically before it, and the first service is connected to the application's capacity metrics...
/// </summary>
/// <param name="applicationId">The application identifier.</param>
/// <param name="serviceName">Name of the service.</param>
/// <param name="metrics">The metrics.</param>
/// <param name="originalMetrics">The original metrics, whose connections need to be removed. Will be empty if it is a new service.</param>
void PlacementAndLoadBalancing::ChangeServiceMetricsForApplication(uint64 applicationId, wstring serviceName, vector<ServiceMetric> const& metrics, vector<ServiceMetric> const& originalMetrics)
{
    vector<wstring> applicationMetrics;
    auto itApp = applicationTable_.find(applicationId);
    if (itApp == applicationTable_.end()) return;
    if (!itApp->second.ApplicationDesc.HasScaleoutOrCapacity()) return;

    map<wstring, ApplicationCapacitiesDescription> const& appCapacities = itApp->second.ApplicationDesc.AppCapacities;
    for (auto itMetric = appCapacities.begin(); itMetric != appCapacities.end(); ++itMetric)
    {
        applicationMetrics.push_back(itMetric->second.MetricName);
    }

    auto const& serviceNames = itApp->second.Services;
    bool isFirstService = false;
    if (serviceNames.size() > 1)
    {
        //RelatedServices will contain the services before and after this service in the application.
        //Service should be connected to them, so as to keep all services in the application part of the same domain...
        vector<wstring> relatedServices;
        auto serviceIter = serviceNames.find(serviceName);
        if (serviceIter == serviceNames.end())
        {
            TESTASSERT_IF(true, "Service {0} does not exist", serviceName);
            return;
        }
        auto nextService = next(serviceIter);
        //it is not the last service...
        if (nextService != serviceNames.end()) { relatedServices.push_back(*nextService); }
        //it is the first service...
        if (serviceIter == serviceNames.begin())
        {
            isFirstService = true;
            //changing relatedness to application metrics...
            metricConnections_.AddOrRemoveMetricAffinity(metrics, applicationMetrics, true);
            metricConnections_.AddOrRemoveMetricAffinity(originalMetrics, applicationMetrics, false);
        }
        else
        {
            relatedServices.push_back(*(prev(serviceIter)));
        }
        Uint64UnorderedMap<Service>::const_iterator itAdjacentService;
        bool isAdjacentServiceFound = false;
        for (auto it = relatedServices.begin(); it != relatedServices.end(); it++)
        {
            auto serviceId = GetServiceId(*it);
            TESTASSERT_IF(serviceId == 0, "Service {0} does not exist", *it);
            auto itServiceDomain = serviceToDomainTable_.find(serviceId);
            TESTASSERT_IF(itServiceDomain == serviceToDomainTable_.end(), "Service {0} does not exist in the serviceToDomainTable_", *it);
            if (itServiceDomain == serviceToDomainTable_.end())
            {
                continue;
            }
            auto const& services = itServiceDomain->second->second.Services;
            auto itService = services.find(serviceId);
            TESTASSERT_IF(itService == services.end(), "Service {0} not found in it's domain", *it, itServiceDomain->second->second.Id);
            if (itService != services.end() && itService->second.ServiceDesc.Name != serviceName)
            {
                //relating to surrounding service...
                metricConnections_.AddOrRemoveMetricAffinity(metrics, itService->second.ServiceDesc.Metrics, true);
                metricConnections_.AddOrRemoveMetricAffinity(originalMetrics, itService->second.ServiceDesc.Metrics, false);
                //freshly added, or about to be removed...
                if (originalMetrics.empty() || metrics.empty())
                {
                    //if the first service is added/removed, then the previous first must be disconnected/connected from the application metrics...
                    if (isFirstService)
                    {
                        metricConnections_.AddOrRemoveMetricAffinity(itService->second.ServiceDesc.Metrics, applicationMetrics, metrics.empty());
                    }
                    else
                        //otherwise we disconnect/connect the surounding services...
                    {
                        if (it == relatedServices.begin())
                        {
                            itAdjacentService = itService;
                            isAdjacentServiceFound = true;
                        }
                        else
                        {
                            if (isAdjacentServiceFound)
                            {
                                metricConnections_.AddOrRemoveMetricAffinity(
                                    itService->second.ServiceDesc.Metrics,
                                    itAdjacentService->second.ServiceDesc.Metrics,
                                    metrics.empty());
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        //only service in application, relating to application...
        metricConnections_.AddOrRemoveMetricAffinity(metrics, applicationMetrics, true);
        metricConnections_.AddOrRemoveMetricAffinity(originalMetrics, applicationMetrics, false);
    }
}

void PlacementAndLoadBalancing::DeleteServiceType(wstring const& serviceTypeName)
{
    if (IsDisposed())
    {
        return;
    }

    // assume all failover units of the service have already been deleted
    AcquireWriteLock grab(lock_);

    auto itServiceType = serviceTypeTable_.find(serviceTypeName);
    if (itServiceType != serviceTypeTable_.end()) // to deal with the case where a deleted service be deleted again
    {
        serviceTypeTable_.erase(itServiceType);
        Trace.UpdateServiceTypeDeleted(serviceTypeName);
    }
}

ErrorCode PlacementAndLoadBalancing::UpdateService(ServiceDescription && serviceDescription, bool forceUpdate)
{
    if (IsDisposed())
    {
        return ErrorCodeValue::ObjectClosed;
    }

    AcquireWriteLock grab(lock_);

    wstring serviceName = serviceDescription.Name;

    ErrorCode ret = InternalUpdateService(move(serviceDescription), forceUpdate).first;
    ASSERT_IF(forceUpdate && !ret.IsSuccess(), "Unable to perform forced update of PLB service {0} (error={1}).", serviceName, ret);
    return ret;
}

void PlacementAndLoadBalancing::DeleteService(std::wstring const& serviceName)
{
    if (IsDisposed())
    {
        return;
    }

    // assume all failover units of the service have already been deleted
    AcquireWriteLock grab(lock_);

    // consume all pending updates so we won't have dangling FailoverUnits
    ProcessPendingUpdatesCallerHoldsLock(Stopwatch::Now());

    InternalDeleteService(serviceName, true);
}

void PlacementAndLoadBalancing::UpdateFailoverUnit(FailoverUnitDescription && failoverUnitDescription)
{
    if (IsDisposed())
    {
        return;
    }

    LockAndSetBooleanLock grab(bufferUpdateLock_);

    InternalUpdateFailoverUnit(move(failoverUnitDescription));
}

void PlacementAndLoadBalancing::DeleteFailoverUnit(wstring && serviceName, Common::Guid fuId)
{
    if (IsDisposed())
    {
        return;
    }

    LockAndSetBooleanLock grab(bufferUpdateLock_);

    InternalDeleteFailoverUnit(move(serviceName), fuId);
}

void PlacementAndLoadBalancing::UpdateLoadOrMoveCost(LoadOrMoveCostDescription && loadOrMoveCost)
{
    if (IsDisposed())
    {
        return;
    }

    // don't stop existing load balancing If only load is changed

    LockAndSetBooleanLock grab(bufferUpdateLock_);

    InternalUpdateLoadOrMoveCost(move(loadOrMoveCost));
}

Common::ErrorCode PlacementAndLoadBalancing::ResetPartitionLoad(Reliability::FailoverUnitId const & failoverUnitId, wstring const & serviceName, bool isStateful)
{
    Trace.ResetPartitionLoadStart(failoverUnitId.Guid);

    vector<LoadMetric> primaryLoadList;
    vector<LoadMetric> secondaryLoadList;
    ErrorCode error = ErrorCodeValue::Success;

    LoadOrMoveCostDescription loadDescription = LoadOrMoveCostDescription(failoverUnitId.Guid, wstring(serviceName), isStateful, true);
    {
        AcquireReadLock grab(lock_);
        auto const& itServiceId = serviceToIdMap_.find(serviceName);
        if (itServiceId == serviceToIdMap_.end())
        {
            error = ErrorCodeValue::ServiceNotFound;
            Trace.ResetPartitionLoadEnd(failoverUnitId.Guid, error);

            return error;
        }

        auto itServiceToDomain = serviceToDomainTable_.find(itServiceId->second);

        if (itServiceToDomain == serviceToDomainTable_.end())
        {
            error = ErrorCodeValue::ServiceNotFound;
            Trace.ResetPartitionLoadEnd(failoverUnitId.Guid, error);

            return error;
        }

        auto itService = itServiceToDomain->second->second.Services.find(itServiceId->second);
        if (itService == itServiceToDomain->second->second.Services.end())
        {
            error = ErrorCodeValue::ServiceNotFound;
            Trace.ResetPartitionLoadEnd(failoverUnitId.Guid, error);

            return error;
        }

        // get default load from service description
        ServiceDescription const & serviceDesc = itService->second.ServiceDesc;
        vector<ServiceMetric> const & defaultMetrics = serviceDesc.Metrics;

        for (auto iter = defaultMetrics.begin(); iter != defaultMetrics.end(); ++iter)
        {
            if (isStateful)
            {
                primaryLoadList.push_back(LoadMetric(wstring(iter->Name), iter->PrimaryDefaultLoad));
                secondaryLoadList.push_back(LoadMetric(wstring(iter->Name), iter->SecondaryDefaultLoad));
            }
            else {
                secondaryLoadList.push_back(LoadMetric(wstring(iter->Name), iter->PrimaryDefaultLoad));
            }
        }

    }

    if (isStateful)
    {
        loadDescription.MergeLoads(ReplicaRole::Primary, move(primaryLoadList), Stopwatch::Now());
        loadDescription.MergeLoads(ReplicaRole::Secondary, move(secondaryLoadList), Stopwatch::Now(), false);
    }
    else
    {
        loadDescription.MergeLoads(ReplicaRole::Secondary, move(secondaryLoadList), Stopwatch::Now(), false);
    }

    // Update load with default loads
    UpdateLoadOrMoveCost(move(loadDescription));

    Trace.ResetPartitionLoadEnd(failoverUnitId.Guid, error);

    return error;
}


int PlacementAndLoadBalancing::CompareNodeForPromotion(wstring const& serviceName, Common::Guid fuId, Federation::NodeId node1, Federation::NodeId node2)
{
    if (IsDisposed())
    {
        return 0;
    }

    if (!settings_.DummyPLBEnabled) //DummyPLB Handled node promotion differently
    {
        if (node1 == node2)
        {
            return 0; // Nothing to compare here
        }

        CheckAndProcessPendingUpdates();

        AcquireReadLock grab(lock_);

        uint64 node1Index = GetNodeIndex(node1);
        uint64 node2Index = GetNodeIndex(node2);

        bool nodeExist1 = node1Index != UINT64_MAX && nodes_[node1Index].NodeDescriptionObj.IsUpAndActivated;
        bool nodeExist2 = node2Index != UINT64_MAX && nodes_[node2Index].NodeDescriptionObj.IsUpAndActivated;

        if (!nodeExist1 && !nodeExist2)
        {
            return 0;
        }
        else if (nodeExist1 && !nodeExist2)
        {
            return -1;
        }
        else if (!nodeExist1 && nodeExist2)
        {
            return 1;
        }
        else
        {
            auto const& itServiceIdMap = serviceToIdMap_.find(serviceName);

            uint64 serviceId = 0;
            if (itServiceIdMap != serviceToIdMap_.end())
            {
                serviceId = itServiceIdMap->second;
            }

            auto domainIt = serviceToDomainTable_.find(serviceId);
            if (domainIt == serviceToDomainTable_.end())
            {
                Common::Assert::TestAssert("Service domain for service {0} does not exist when comparing nodes for promotion", serviceName);
                // We do not have this service. Both nodes are equal to us.
                return 0;
            }
            ServiceDomain const& domain = domainIt->second->second;

            return domain.CompareNodeForPromotion(fuId, nodes_[node1Index], nodes_[node2Index]);
        }
    }
    else
    {
        UNREFERENCED_PARAMETER(serviceName);
        UNREFERENCED_PARAMETER(fuId);

        if (node1 < node2)
        {
            return 1;
        }
        else if (node2 < node1)
        {
            return -1;
        }
        else
        {
            return 0;
        }
    }


}

void PlacementAndLoadBalancing::Dispose()
{
    if (IsDisposed())
    {
        return;
    }

    {
        AcquireWriteLock grab(lock_);

        // don't clear services/nodes/failover units here
        // because other threads may still need them

        if (timer_ != nullptr)
        {
            timer_->Cancel();
        }
        if (processPendingUpdatesTimer_ != nullptr)
        {
            processPendingUpdatesTimer_->Cancel();
        }
        if (interruptSearcherRunThread_ != nullptr)
        {
            interruptSearcherRunThread_->Cancel();
        }
        StopSearcher();

        disposed_.store(true);
    }

    if (tracingMovementsJobQueueUPtr_)
    {
        tracingMovementsJobQueueUPtr_->Close();
    }

    Trace.PLBDispose();
}

void PlacementAndLoadBalancing::Refresh(StopwatchTime now)
{
    if (IsDisposed())
    {
        Trace.PLBSkipObjectDisposed();
        return;
    }

    plbRefreshTimers_.Reset();

    nextActionPeriod_ = Common::TimeSpan::MaxValue;

    Stopwatch refreshStopwatch;
    Stopwatch durationStopwatch;

    Stopwatch pendingUpdatesStopwatch;
    Stopwatch beginRefreshStopwatch;
    Stopwatch snapshotStopwatch;
    Stopwatch engineStopwatch;
    Stopwatch endRefreshStopwatch;
    Stopwatch autoScalingStopwatch;

    refreshStopwatch.Start();
    durationStopwatch.Start();

    //we need to capture UseSeparateSecondaryLoad while holding the lock
    settings_.RefreshSettings();

    StopwatchTime startTime = Stopwatch::Now();
    vector<ServiceDomain::DomainData> searcherDataList;
    ServiceDomainStats stats;
    unique_ptr<Snapshot> snapshotBeforeRefresh, snapshotAfterRefresh;
    Trace.PLBPrepareStart();

    bool oldUseSeparateSecondaryLoad = settings_.UseSeparateSecondaryLoad;
    bool newUseSeparateSecondaryLoad = PLBConfig::GetConfig().UseSeparateSecondaryLoad;

    if (bufferUpdateLock_.IsSet() || oldUseSeparateSecondaryLoad != newUseSeparateSecondaryLoad)
    {
        AcquireWriteLock grab(lock_);

        if (oldUseSeparateSecondaryLoad != newUseSeparateSecondaryLoad)
        {
            UpdateUseSeparateSecondaryLoadConfig(newUseSeparateSecondaryLoad);
        }

        pendingUpdatesStopwatch.Start();
        ProcessPendingUpdatesCallerHoldsLock(now);
        pendingUpdatesStopwatch.Stop();
    }

    autoScalingStopwatch.Start();
    {
        AcquireReadLock grab(autoScalingFailedOperationsVectorsLock_);
        if (!pendingAutoScalingFailedRepartitionsForRefresh_.empty())
        {
            TRACE_WARNING_AND_TESTASSERT("AutoScaling", "pendingAutoScalingFailedRepartitionsForRefresh_ is not empty in Refresh().");
            pendingAutoScalingFailedRepartitionsForRefresh_.clear();
        }
        pendingAutoScalingFailedRepartitionsForRefresh_.swap(pendingAutoScalingFailedRepartitions_);
    }
    autoScalingStopwatch.Stop();

    {
        AcquireReadLock grab(lock_);

        autoScalingStopwatch.Start();
        ProcessFailedAutoScalingRepartitionsCallerHoldsLock();
        autoScalingStopwatch.Stop();

        beginRefreshStopwatch.Start();
        BeginRefresh(searcherDataList, stats, now);
        beginRefreshStopwatch.Stop();

        snapshotStopwatch.Start();
        snapshotBeforeRefresh.reset(new Snapshot(TakeSnapShot()));
        snapshotStopwatch.Stop();

        TracePeriodical(stats, now);
    }

    autoScalingStopwatch.Start();
    ProcessAutoScalingRepartitioning();
    autoScalingStopwatch.Stop();

    engineStopwatch.Start();
    StopwatchTime engineStartTime = Stopwatch::Now();
    uint64 msTimeCountForPrepare = (engineStartTime - startTime).TotalMilliseconds();
    Trace.PLBPrepareEnd(msTimeCountForPrepare);
    Trace.PLBStart(
        stats.nodeCount_,
        stats.serviceCount_,
        stats.partitionCount_,
        stats.inTransitionPartitionCount_,
        stats.existingReplicaCount_,
        stats.movableReplicaCount_,
        stats.partitionsWithNewReplicaCount_,
        stats.partitionsWithInstOnEveryNodeCount_,
        searcher_ != nullptr ? searcher_->RandomSeed : -1);

    size_t currentBalancingMovementCount = 0;
    size_t currentPlacementMovementCount = 0;
    size_t totalOperationCount = 0;

    if (settings_.IsTestMode && interruptSearcherRunThread_ != nullptr)
    {
        //test thread started
        interruptSearcherRunThread_->Change(TimeSpan::Zero, TimeSpan::MaxValue);
        //wait until thread finishes the update and interrupt the search
        while (interruptSearcherRunFinished_.load() == false)
        {
            ;
        }
        interruptSearcherRunThread_->Cancel();
        interruptSearcherRunThread_ = nullptr;
    }

    if (stats.nodeCount_ == 0 || stats.serviceCount_ == 0 || stats.partitionCount_ == 0)
    {
        Trace.PLBSkipEmpty();
    }
    else
    {
        globalBalancingMovementCounter_.Refresh(now);
        globalPlacementMovementCounter_.Refresh(now);

        vector<size_t> scrambler;
        size_t noOfServiceDomains = searcherDataList.size();
        for (size_t i = 0; i < noOfServiceDomains; i++)
        {
            scrambler.push_back(i);
        }

        Common::Random random(randomSeed_);
        auto myFunc = [&random](size_t n) -> int
        {
            return random.Next(static_cast<int>(n));
        };
        random_shuffle(scrambler.begin(), scrambler.end(), myFunc);

        currentBalancingMovementCount = 0;
        currentPlacementMovementCount = 0;
        totalOperationCount = 0;

        for (size_t i = 0; i < noOfServiceDomains; i++)
        {
            auto itData = &(searcherDataList[scrambler[i]]);

            if (itData->action_.IsSkip)
            {
                // The placement is not generated
                continue;
            }

            PlacementUPtr const& pl = itData->state_.PlacementObj;
            PLBConfig const& config = PLBConfig::GetConfig();

            size_t numBatch = 1;
            if (config.UseBatchPlacement && pl != nullptr &&
                pl->NewReplicaCount > config.PlacementReplicaCountPerBatch &&
                (itData->action_.Action == PLBSchedulerActionType::NewReplicaPlacement ||
                    itData->action_.Action == PLBSchedulerActionType::NewReplicaPlacementWithMove))
            {
                // The batch index vector size doesn't include 0 as the starting index, so it is 1 less than the number of batches
                // For example, if new replica count is less than the config, index vec is empty and batch count is 1.

                numBatch = pl->PartitionBatchIndexVec.size() + 1;

                Trace.Searcher(wformatString("RunSearcher: {0} batches are needed for domain {1}", numBatch, itData->domainId_));
            }

            if (searcher_)
            {
                searcher_->BatchIndex = 0;
            }

            size_t placementBatchIndex = 0;
            while (placementBatchIndex < numBatch)
            {
                RunSearcher(itData, stats, totalOperationCount, currentPlacementMovementCount, currentBalancingMovementCount);

                placementBatchIndex++;

                if (placementBatchIndex == numBatch)
                {
                    AcquireWriteLock grab(lock_);

                    endRefreshStopwatch.Start();
                    EndRefresh(itData, now);
                    endRefreshStopwatch.Stop();
                }

                if (searcher_)
                {
                    if (numBatch > 1)
                    {
                        searcher_->BatchIndex = placementBatchIndex;
                        Trace.Searcher(wformatString("RunSearcher: Domain {0} passed movements to FM at batch {1}", itData->domainId_, placementBatchIndex - 1));
                    }
                    else
                    {
                        Trace.Searcher(wformatString("RunSearcher: Domain {0} passed movements to FM", itData->domainId_));
                    }
                }

                UpdatePartitionsWithCreation(itData);
                PassMovementsToFM(itData);
            }
        }

        uint64 moveCount = static_cast<uint64>(totalOperationCount);
        durationStopwatch.Stop();
        engineStopwatch.Stop();
        plbRefreshTimers_.msTimeCountForEngine = engineStopwatch.ElapsedMilliseconds;

        Trace.PLBEnd(static_cast<int>(totalOperationCount), durationStopwatch.ElapsedMilliseconds, plbRefreshTimers_.msTimeCountForEngine);

        //Health And Diagnostic Bookkeeping
        LoadBalancingCounters->UpdateAggregatePerformanceCounters(moveCount, durationStopwatch.ElapsedMilliseconds);

        globalBalancingMovementCounter_.Record(currentBalancingMovementCount, now);
        globalPlacementMovementCounter_.Record(currentPlacementMovementCount, now);
        plbDiagnosticsSPtr_->CleanupSearchForPlacementDiagnostics();
        plbDiagnosticsSPtr_->SchedulersDiagnostics->CleanupStageDiagnostics();
        plbDiagnosticsSPtr_->ReportConstraintHealth();
        plbDiagnosticsSPtr_->ReportSearchForPlacementHealth();
        plbDiagnosticsSPtr_->ReportBalancingHealth();
    }

    searcher_ = nullptr;

    {
        AcquireReadLock grab(lock_);
        snapshotStopwatch.Start();
        snapshotAfterRefresh.reset(new Snapshot(TakeSnapShot()));
        snapshotStopwatch.Stop();
    }

    if (!isMaster_ && lastStatisticsTrace_ + PLBConfig::GetConfig().StatisticsTracingInterval <= now)
    {
        lastStatisticsTrace_ = now;
        // Populate the loads and capacities.
        plbStatistics_.Update(*snapshotAfterRefresh);
        plbStatistics_.TraceStatistics(PublicTrace);
    }

    {
        AcquireExclusiveLock grab(snapshotLock_);
        snapshotsOnEachRefresh_ =
            shared_ptr<std::pair<Snapshot, Snapshot>>(new pair<Snapshot, Snapshot>(move(*snapshotBeforeRefresh), move(*snapshotAfterRefresh)));
    }

    for (auto & appToUpgrade : applicationsSafeToUpgrade_)
    {
        failoverManager_.UpdateAppUpgradePLBSafetyCheckStatus(appToUpgrade);
    }

    // Traces and Updates PLB Perf counters for PLB movements dropped by the FM
    this->TrackDroppedPLBMovements();
    plbDiagnosticsSPtr_->ReportDroppedPLBMovementsHealth();

    if (!IsDisposed())
    {
        // the object may already be disposed when algorithm was running
        if (Common::TimeSpan::MaxValue == nextActionPeriod_ || nextActionPeriod_ < PLBConfig::GetConfig().PLBRefreshGap)
        {
            // If there are no domain, we want to run again after the gap
            nextActionPeriod_ = PLBConfig::GetConfig().PLBRefreshGap;
        }
        timer_->Change(nextActionPeriod_);
    }

    refreshStopwatch.Stop();

    plbRefreshTimers_.msPendingUpdatesTime = pendingUpdatesStopwatch.ElapsedMilliseconds;
    plbRefreshTimers_.msBeginRefreshTime = beginRefreshStopwatch.ElapsedMilliseconds;
    plbRefreshTimers_.msSnapshotTime = snapshotStopwatch.ElapsedMilliseconds;
    plbRefreshTimers_.msEndRefreshTime = endRefreshStopwatch.ElapsedMilliseconds;
    plbRefreshTimers_.msRefreshTime = refreshStopwatch.ElapsedMilliseconds;
    plbRefreshTimers_.msAutoScalingTime = autoScalingStopwatch.ElapsedMilliseconds;

    plbRefreshTimers_.CalculateRemainderTime();

    Trace.PLBRefreshTiming(
        plbRefreshTimers_.msRefreshTime,
        plbRefreshTimers_.msPendingUpdatesTime,
        plbRefreshTimers_.msBeginRefreshTime,
        plbRefreshTimers_.msEndRefreshTime,
        plbRefreshTimers_.msSnapshotTime,
        plbRefreshTimers_.msTimeCountForEngine,
        plbRefreshTimers_.msAutoScalingTime,
        plbRefreshTimers_.msRemainderTime);
}

void PlacementAndLoadBalancing::UpdatePartitionsWithCreation(ServiceDomain::DomainData * searcherDomainData)
{
    // Save the partition ID before passing the movements to FM
    if (searcherDomainData->movementTable_.empty())
    {
        return;
    }

    for (auto itMovement = searcherDomainData->movementTable_.begin(); itMovement != searcherDomainData->movementTable_.end(); ++itMovement)
    {
        if (itMovement->second.ContainCreation())
        {
            searcherDomainData->partitionsWithCreations_.push_back(itMovement->first);
        }
    }
}

void PlacementAndLoadBalancing::PassMovementsToFM(ServiceDomain::DomainData * searcherDomainData)
{
    ASSERT_IF(tracingMovementsJobQueueUPtr_ == nullptr, "Tracing movements job queue is null, master:{0}", isMaster_);
    TracingMovementsJob tracingMovementsJob(searcherDomainData);
    tracingMovementsJobQueueUPtr_->Enqueue(move(tracingMovementsJob));

    if (!searcherDomainData->movementTable_.empty())
    {
        failoverManager_.ProcessFailoverUnitMovements(move(searcherDomainData->movementTable_), move(searcherDomainData->dToken_));
        searcherDomainData->movementTable_.clear();
    }
}
vector<PlacementAndLoadBalancing::ServiceDomainData> PlacementAndLoadBalancing::GetServiceDomains()
{
    vector<ServiceDomainData> ret;

    if (IsDisposed())
    {
        return ret;
    }

    AcquireReadLock grab(lock_);

    for (auto it = serviceDomainTable_.begin(); it != serviceDomainTable_.end(); ++it)
    {
        set<wstring> services;
        for (auto itService = it->second.Services.begin(); itService != it->second.Services.end(); ++itService)
        {
            services.insert(itService->second.ServiceDesc.Name);
        }

        ret.push_back(ServiceDomainData(wstring(it->first), it->second.Scheduler.CurrentAction, move(services)));
    }

    return ret;
}

void PlacementAndLoadBalancing::UpdateNextActionPeriod(Common::TimeSpan nextActionPeriod)
{
    if (nextActionPeriod < nextActionPeriod_)
    {
        nextActionPeriod_ = nextActionPeriod;
    }
}

void PlacementAndLoadBalancing::PLBRefreshTimers::CalculateRemainderTime()
{
    uint64 msNonRefreshTime =
        msPendingUpdatesTime +
        msBeginRefreshTime +
        msEndRefreshTime +
        msSnapshotTime +
        msTimeCountForEngine +
        msAutoScalingTime;

    // Due to an imprecise nature of timers, we need to guard against a possible uint underflow here
    msRemainderTime = msRefreshTime > msNonRefreshTime ? msRefreshTime - msNonRefreshTime : 0;
}

void PlacementAndLoadBalancing::PLBRefreshTimers::Reset()
{
    msPendingUpdatesTime = 0;
    msBeginRefreshTime = 0;
    msEndRefreshTime = 0;
    msSnapshotTime = 0;
    msAutoScalingTime = 0;
    msTimeCountForEngine = 0;
    msRemainderTime = 0;
    msRefreshTime = 0;
}

//------------------------------------------------------------
// private members
//------------------------------------------------------------

PlacementAndLoadBalancing::TracingMovementsJob::TracingMovementsJob()
    : tracingMovementsStopwatch_(),
    domainFTMovements_()
{
}

PlacementAndLoadBalancing::TracingMovementsJob::TracingMovementsJob(ServiceDomain::DomainData const* searcherDomainData)
    : tracingMovementsStopwatch_(),
    domainFTMovements_()
{
    tracingMovementsStopwatch_.Start();

    // Copy all the movements from the movement table in order to pass it to the tracing thread
    for (auto itFTMovement = searcherDomainData->movementTable_.begin();
        itFTMovement != searcherDomainData->movementTable_.end();
        ++itFTMovement)
    {
        domainFTMovements_.push_back(make_pair(searcherDomainData->dToken_.DecisionId, itFTMovement->second));
    }

}

PlacementAndLoadBalancing::TracingMovementsJob::TracingMovementsJob(TracingMovementsJob && other)
    : tracingMovementsStopwatch_(move(other.tracingMovementsStopwatch_)),
    domainFTMovements_(move(other.domainFTMovements_))
{
}

PlacementAndLoadBalancing::TracingMovementsJob & PlacementAndLoadBalancing::TracingMovementsJob::operator=(TracingMovementsJob && other)
{
    if (this != &other)
    {
        tracingMovementsStopwatch_ = move(other.tracingMovementsStopwatch_);
        domainFTMovements_ = move(other.domainFTMovements_);
    }

    return *this;
}

bool PlacementAndLoadBalancing::TracingMovementsJob::ProcessJob(PlacementAndLoadBalancing & plb)
{
    for (auto itMovementTable = domainFTMovements_.begin(); itMovementTable != domainFTMovements_.end(); ++itMovementTable)
    {
        for (auto itAction = itMovementTable->second.Actions.begin();
            itAction != itMovementTable->second.Actions.end();
            ++itAction)
        {
            plb.PublicTrace.Operation(
                itMovementTable->second.FailoverUnitId,
                itMovementTable->first,
                itAction->SchedulerActionType,
                itAction->Action,
                itMovementTable->second.ServiceName,
                itAction->SourceNode,
                itAction->TargetNode);
        }
    }

    tracingMovementsStopwatch_.Stop();
    plb.Trace.TrackingProposedMovementsEnd(tracingMovementsStopwatch_.ElapsedMilliseconds);

    return true;
}

PlacementAndLoadBalancing::TracingMovementsJobQueue::TracingMovementsJobQueue(
    std::wstring const & name,
    PlacementAndLoadBalancing & root,
    bool forceEnqueue,
    int maxThreads,
    bool& tracingJobQueueFinished,
#if !defined(PLATFORM_UNIX)
    Common::ExclusiveLock& testTracingLock,
#else
    Common::Mutex& testTracingLock,
#endif
    Common::ConditionVariable& testTracingBarrier)
    : JobQueue(name, root, forceEnqueue, maxThreads),
    tracingJobQueueFinished_(tracingJobQueueFinished),
    testTracingLock_(testTracingLock),
    testTracingBarrier_(testTracingBarrier)
{
}

void PlacementAndLoadBalancing::TracingMovementsJobQueue::OnFinishItems()
{
#if !defined(PLATFORM_UNIX)
    AcquireExclusiveLock lock(testTracingLock_);
#else
    AcquireMutexLock lock(&testTracingLock_);
#endif
    tracingJobQueueFinished_ = true;
    testTracingBarrier_.Wake();
}

/// <summary>
/// Internally Update Service Details.
/// </summary>
/// <param name="serviceDescription">The service Description.</param>
/// <param name="forceUpdate">if set to <c>true</c> [force update].</param>
/// <param name="traceDetail">if set to <c>true</c> [trace all details].</param>
/// <param name="updateMetricConnections">if set to <c>true</c> we update the [Metric Connections Graph]. This is only false when we're using updateservice as a result of a domain split.</param>
/// <returns>(errorCode, skipUpdate)</returns>
pair<ErrorCode, bool> PlacementAndLoadBalancing::InternalUpdateService(ServiceDescription && serviceDescription, bool forceUpdate, bool traceDetail, bool updateMetricConnections, bool skipServicePackageUpdate)
{
    auto itServiceIdMap = serviceToIdMap_.find(serviceDescription.Name);
    if (serviceDescription.ServiceId == 0 && itServiceIdMap == serviceToIdMap_.end())
    {
        // This is a new service, we need to assign next service ID to it.
        serviceDescription.ServiceId = nextServiceId_++;
        serviceToIdMap_.insert(make_pair(serviceDescription.Name, serviceDescription.ServiceId));
        plbStatistics_.AddService(serviceDescription);
    }
    else
    {
        if (itServiceIdMap == serviceToIdMap_.end())
        {
            return make_pair(ErrorCodeValue::ServiceNotFound, false);
        }

        // Service already exists, reuse the existing service ID.
        serviceDescription.ServiceId = itServiceIdMap->second;
    }

    Uint64UnorderedMap<Application>::iterator appIt = applicationTable_.end();
    bool appHasScaleoutOrCapacity = false;
    if (serviceDescription.ApplicationName != L"")
    {
        auto appIter = applicationToIdMap_.find(serviceDescription.ApplicationName);
        if (appIter == applicationToIdMap_.end())
        {
            // In case when service gets created before the application (possible for application created before v4.5)
            // we need to keep track of this application as well, so we assign new AppID to it.
            appIter = applicationToIdMap_.insert(make_pair(serviceDescription.ApplicationName, nextApplicationId_++)).first;
            ApplicationDescription appDescription(move(wstring(serviceDescription.ApplicationName)));
            appDescription.ApplicationId = appIter->second;
            appIt = applicationTable_.insert(make_pair(appDescription.ApplicationId, Application(move(appDescription)))).first;
            if (applicationTable_.size() == 1)
            {
                nextApplicationToBeTraced_ = applicationTable_.begin()->first;
                for (auto it = serviceDomainTable_.begin(); it != serviceDomainTable_.end(); it++)
                {
                    it->second.LastApplicationTraced = applicationTable_.begin()->first;
                }
            }
        }
        else
        {
            // If application already exists, we just need to find it.
            appIt = applicationTable_.find(appIter->second);

            // As we never remove applicaiton from applicationToIdMap_ there is a possibility that we already deleted the application
            // In this case we should not allow service creation
            if (appIt == applicationTable_.end())
            {
                Trace.ApplicationNotFound(serviceDescription.ApplicationName, serviceDescription.ApplicationId, serviceDescription.Name);
                return make_pair(ErrorCodeValue::ApplicationInstanceDeleted, false);
            }

            appHasScaleoutOrCapacity = appIt->second.ApplicationDesc.HasScaleoutOrCapacity();
        }
        serviceDescription.ApplicationId = appIter->second;

    }
    auto itServicePackage = servicePackageTable_.end();
    auto itServicePackageId = servicePackageToIdMap_.find(serviceDescription.ServicePackageIdentifier);
    if (itServicePackageId != servicePackageToIdMap_.end())
    {
        itServicePackage = servicePackageTable_.find(itServicePackageId->second);
        serviceDescription.ServicePackageId = itServicePackageId->second;
        //in case we skip this it means that the service package descritpion was already adjusted with appropriate rg metrics
        if (!skipServicePackageUpdate)
        {
            if (itServicePackage != servicePackageTable_.end())
            {
                PLBConfig const& config = PLBConfig::GetConfig();
                bool isSingleReplicaPerHost = serviceDescription.ServicePackageActivationMode == ServiceModel::ServicePackageActivationMode::Enum::ExclusiveProcess;
                // Always add default metrics.
                serviceDescription.AddDefaultMetrics();
                // Resources should be added as metrics in case that SP is governed.
                for (auto & rgResource : itServicePackage->second.Description.CorrectedRequiredResources)
                {
                    double rgMetricWeight = (rgResource.first == ServiceModel::Constants::SystemMetricNameCpuCores) ?
                        config.CpuCoresMetricWeight :
                        config.MemoryInMBMetricWeight;

                    if (isSingleReplicaPerHost)
                    {
                        serviceDescription.AddRGMetric(rgResource.first, rgMetricWeight, rgResource.second, rgResource.second);
                    }
                    else
                    {
                        serviceDescription.AddRGMetric(rgResource.first, rgMetricWeight, 0, 0);
                    }
                }
            }
        }
    }
    //this is a service package created before 5.6 and we do not have info about it so we will create a new one so that we can keep track of all the services it has
    //so we can do proper safety check when this service package starts using rg
    else
    {
        if (appIt != applicationTable_.end())
        {
            ServicePackageDescription description(
                ServiceModel::ServicePackageIdentifier(serviceDescription.ServicePackageIdentifier),
                std::map<std::wstring, double>(),
                std::vector<std::wstring>());

            auto servicePackageId = nextServicePackageIndex_;
            serviceDescription.ServicePackageId = servicePackageId;
            servicePackageToIdMap_.insert(make_pair(description.ServicePackageIdentifier, nextServicePackageIndex_++));
            itServicePackage = servicePackageTable_.insert(make_pair(servicePackageId, ServicePackage(move(description)))).first;
            appIt->second.AddServicePackage(itServicePackage->second.Description);
            plbStatistics_.AddServicePackage(itServicePackage->second.Description);
            Trace.UpdateServicePackage(itServicePackage->second.Description.Name, itServicePackage->second.Description);
        }
    }

    // This will add default metrics in case that service does not report any other metric.
    // In case that service uses RG, default metrics were added above.
    serviceDescription.AddDefaultMetrics();

    // If service is stateful singleton replica, align primary and secondary default loads
    // This is needed to avoid issues during singleton replica movement,
    // as first new secondary is created, and then promoted to primary
    if (serviceDescription.IsStateful && serviceDescription.TargetReplicaSetSize == 1)
    {
        serviceDescription.AlignSinletonReplicaDefLoads();
    }

    //is this a new service create, or an update for an existing service...
    bool isNewService = serviceToDomainTable_.find(serviceDescription.ServiceId) == serviceToDomainTable_.end();

    //Affinty relationship is not allowed to have chains...
    if (IsAffinityChain(serviceDescription))
    {
        Trace.AffinityChainDetected(serviceDescription.Name, serviceDescription.AffinitizedService);
        if (forceUpdate)
        {
            // Report health error on this service
            plbDiagnosticsSPtr_->ReportServiceCorrelationError(serviceDescription);
            // Remove affinity from service description and proceed
            serviceDescription.ClearAffinitizedService();
        }
        else
        {
            return make_pair(ErrorCodeValue::ServiceAffinityChainNotSupported, false);
        }
    }

    if (!forceUpdate)
    {
        if (!CheckClusterResource(serviceDescription))
        {
            return make_pair(ErrorCodeValue::InsufficientClusterCapacity, false);
        }

        if (PLBConfig::GetConfig().ValidatePlacementConstraint && !IsMaster && !serviceDescription.PlacementConstraints.empty())
        {
            if (!CheckConstraintKeyDefined(serviceDescription))
            {
                Trace.ConstraintKeyUndefined(serviceDescription.Name, serviceDescription.PlacementConstraints);
                return make_pair(ErrorCodeValue::ConstraintKeyUndefined, false);
            }
            else
            {
                if (serviceNameToPlacementConstraintTable_.size() < PLBConfig::GetConfig().PlacementConstraintValidationCacheSize)
                {
                    //Adds this to the table only if the CreateServiceCall it will be successful
                    serviceNameToPlacementConstraintTable_.insert(std::make_pair(serviceDescription.Name, expressionCache_));
                }
                expressionCache_ = nullptr;
            }
        }
    }


    PLBConfig & config = PLBConfig::GetConfig();
    // we take a copy of the metrics instead of reference since
    // we can use it later for reattaching FT's in case the service is being redefined.
    vector<ServiceMetric> metrics = serviceDescription.Metrics;
    ASSERT_IF(metrics.empty(), "Service metrics should not be empty");
    //store a copy of existing failoverUnits and original metrics
    vector<FailoverUnit> relatedFailoverUnits;
    vector<ServiceMetric> originalMetrics;
    //domains to be merged: we only use this when we aren't supposed to update metric connections...(i.e during domain split)
    set<ServiceDomain::DomainId> domains;
    bool areRelatedFailoverUnitsStateful = false;
    //freshly affinitized as child, but parent hasn't been created yet.
    bool isParentAbsent = false;

    // This is used for scaling based on number of partitions
    Common::StopwatchTime serviceScalingExpiry = Common::StopwatchTime::Zero;
    int partitionCountChange = 0;

    auto isAutoScalingPolicyValid = [](ServiceDescription const& serviceDescription)
    {
        int minCount = 0;
        auto autoScalingPolicy = serviceDescription.AutoScalingPolicy;
        if (autoScalingPolicy.Mechanism->Kind == ScalingMechanismKind::PartitionInstanceCount)
        {
            InstanceCountScalingMechanismSPtr mechanism = static_pointer_cast<InstanceCountScalingMechanism>(serviceDescription.AutoScalingPolicy.Mechanism);
            minCount = mechanism->MinimumInstanceCount;
        }
        if (autoScalingPolicy.Mechanism->Kind == ScalingMechanismKind::AddRemoveIncrementalNamedPartition)
        {
            AddRemoveIncrementalNamedPartitionScalingMechanismSPtr mechanism = static_pointer_cast<AddRemoveIncrementalNamedPartitionScalingMechanism>(serviceDescription.AutoScalingPolicy.Mechanism);
            minCount = mechanism->MinimumPartitionCount;
        }
        if (minCount == 0)
        {
            return false;
        }
        else
        {
            return true;
        }
    };

    if (isNewService)
    {
        if (!forceUpdate && serviceDescription.IsAutoScalingDefined)
        {
            //New auto scaling policy should be created with new service, so checking is mincount > 0
            if (!isAutoScalingPolicyValid(serviceDescription))
            {
                Trace.InvalidAutoScaleMinCount(serviceDescription.Name);
                return make_pair(ErrorCodeValue::InvalidServiceScalingPolicy, false);
            }
        }
    }

    bool isScalingPolicyChanged = false;

    //In case of redefinition, we will execute a deleteService() to clear all info. This deleteservice won't split any domains though.
    if (!isNewService)
    {
        ASSERT_IF(!updateMetricConnections, "Metric Connections should always be updated for a non-new service");
        //Get all failover unit associated with that service
        auto itServiceToDomain = serviceToDomainTable_.find(serviceDescription.ServiceId);
        // No need to check itServiceToDomain, because isNewService is true if element is not found
        auto itServiceDomain = serviceDomainTable_.find((itServiceToDomain->second)->first);
        if (itServiceDomain != serviceDomainTable_.end())
        {
            // store original metrics related in original service description
            auto itService = itServiceDomain->second.Services.find(serviceDescription.ServiceId);
            if (itService != itServiceDomain->second.Services.end())
            {
                serviceScalingExpiry = itService->second.NextAutoScaleCheck;
                ServiceDescription const& originalServiceDescription = itService->second.ServiceDesc;
                partitionCountChange = serviceDescription.PartitionCount - originalServiceDescription.PartitionCount;
                originalMetrics = originalServiceDescription.Metrics;

                plbStatistics_.UpdateService(originalServiceDescription, serviceDescription);

                // Check if FTs will have to be checked and updated for AS
                if (serviceDescription.IsAutoScalingDefined != originalServiceDescription.IsAutoScalingDefined)
                {
                    isScalingPolicyChanged = true;
                }
                else if (serviceDescription.IsAutoScalingDefined)
                {
                    isScalingPolicyChanged = serviceDescription.AutoScalingPolicy != originalServiceDescription.AutoScalingPolicy;
                }

                if (!forceUpdate && serviceDescription.IsAutoScalingDefined && isScalingPolicyChanged)
                {
                    // Autoscaling policy is changed so check is it still valid
                    if (!isAutoScalingPolicyValid(serviceDescription))
                    {
                        Trace.InvalidAutoScaleMinCount(serviceDescription.Name);
                        return make_pair(ErrorCodeValue::InvalidServiceScalingPolicy, false);
                    }
                }
                //inquires whether the affinity relationship was changed

                areRelatedFailoverUnitsStateful = originalServiceDescription.IsStateful;
            }

            GuidUnorderedMap<FailoverUnit> const& failoverUnits = (itServiceDomain->second).FailoverUnits;

            for (auto iter = failoverUnits.begin(); iter != failoverUnits.end(); ++iter)
            {
                if (iter->second.FuDescription.ServiceName == serviceDescription.Name)
                {
                    relatedFailoverUnits.push_back(iter->second);
                }
            }
            for (auto iter = relatedFailoverUnits.begin(); iter != relatedFailoverUnits.end(); ++iter)
            {
                wstring serviceName = serviceDescription.Name;
                // Delete failover unit by id and name
                FailoverUnitDescription fuDescription(iter->FUId, move(serviceName), serviceDescription.ServiceId);
                itServiceDomain->second.UpdateFailoverUnit(move(fuDescription), Common::Stopwatch::Now(), false);
            }
        }

        if (!forceUpdate)
        {
            // Error is reported when affinity chain is detected and service update is required to remove affinity chain.
            // Clear the health error only if this is user-initiated service update (!force and !new)
            plbDiagnosticsSPtr_->ReportServiceDescriptionOK(serviceDescription);
        }
        bool deleted = InternalDeleteService(serviceDescription.Name, false, false);
        ASSERT_IFNOT(deleted, "Failed to delete a service for service update.");
    }

    //in case its new service, just make sure its not being re-created due to a splitdomain.
    //if it is being created due to a splitdomain, we will take care of metric merges later.
    if (updateMetricConnections)
    {
        metricConnections_.AddOrRemoveMetricConnectionsForService(metrics, true);
    }

    //add this service to the service package
    if (itServicePackage != servicePackageTable_.end())
    {
        itServicePackage->second.Services.insert(serviceDescription.ServiceId);
    }
    //in case service is part of an application with capacity
    if (!serviceDescription.ApplicationName.empty())
    {
        appIt->second.AddService(serviceDescription.Name);
        if (appHasScaleoutOrCapacity)
        {
            if (updateMetricConnections)
            {
                ChangeServiceMetricsForApplication(
                    serviceDescription.ApplicationId, serviceDescription.Name, metrics);
            }
            else
            {
                //in case the application already has a Domain...
                auto appDomainIt = applicationToDomainTable_.find(serviceDescription.ApplicationId);
                if (appDomainIt != applicationToDomainTable_.end())
                {
                    domains.insert(appDomainIt->second->second.Id);
                }
                else
                    //in case application metrics can connect Domains...
                {
                    auto itAppById = applicationTable_.find(serviceDescription.ApplicationId);
                    DBGASSERT_IF(itAppById == applicationTable_.end(), "Undefined Application {0}", serviceDescription.ApplicationName);
                    if (itAppById != applicationTable_.end() && itAppById->second.ApplicationDesc.HasScaleoutOrCapacity())
                    {
                        auto capacities = itAppById->second.ApplicationDesc.AppCapacities;
                        for (auto capacityIt = capacities.begin(); capacityIt != capacities.end(); capacityIt++)
                        {
                            auto metricDomainIt = metricToDomainTable_.find(capacityIt->second.MetricName);
                            if (metricDomainIt != metricToDomainTable_.end())
                            {
                                domains.insert(metricDomainIt->second->second.Id);
                            }
                        }
                    }
                }
            }
        }
    }
    //in case service was affinitized as child...
    if (!serviceDescription.AffinitizedService.empty())
    {
        auto const& itId = serviceToIdMap_.find(serviceDescription.AffinitizedService);
        if (itId != serviceToIdMap_.end())
        {
            auto itDomain = serviceToDomainTable_.find(itId->second);
            if (itDomain != serviceToDomainTable_.end())
            {
                auto itService = itDomain->second->second.Services.find(itId->second);
                if (itService != itDomain->second->second.Services.end())
                {
                    if (isNewService && itService->second.ServiceDesc.PartitionCount > 1)
                    {
                        Trace.ParentServiceMultipleParitions(itService->second.ServiceDesc);
                    }
                }

                //make sure its not being re-created due to a splitdomain
                if (updateMetricConnections)
                {
                    auto itSecondService = itDomain->second->second.Services.find(itId->second);
                    if (itSecondService != itDomain->second->second.Services.end())
                    {
                        metricConnections_.AddOrRemoveMetricAffinity(metrics, itSecondService->second.ServiceDesc.Metrics, true);
                    }
                }
                else
                {
                    domains.insert(itDomain->second->second.Id);
                }
            }
            else
            {
                isParentAbsent = true;
            }
        }
        else
        {
            isParentAbsent = true;
        }
    }
    //in case it's affinitized as parent
    else
    {
        bool hasChildren = false;
        auto it = dependedServiceToDomainTable_.find(serviceDescription.Name);
        //parent created after child...
        if (it != dependedServiceToDomainTable_.end())
        {
            DBGASSERT_IF(it->second.empty(), "dependedServiceToDomainTable entry shouldn't contain 0 domains for service {0}", serviceDescription.Name);
            vector<wstring> dependentDomains;
            dependentDomains.reserve(it->second.size());
            for (auto domainIt = it->second.begin(); domainIt != it->second.end(); ++domainIt)
            {
                dependentDomains.push_back((*domainIt)->second.Id);
                auto childServices = (*domainIt)->second.ChildServices.find(serviceDescription.Name);
                if (childServices != (*domainIt)->second.ChildServices.end())
                {
                    for (auto itChild = childServices->second.begin(); itChild != childServices->second.end(); itChild++)
                    {
                        uint64 childServiceId = GetServiceId(*itChild);

                        if (childServiceId != 0)
                        {
                            auto itService = (*domainIt)->second.Services.find(childServiceId);
                            hasChildren = true;

                            if (updateMetricConnections)
                            {
                                auto itSecondService = (*domainIt)->second.Services.find(childServiceId);
                                if (itSecondService != (*domainIt)->second.Services.end())
                                {
                                    metricConnections_.AddOrRemoveMetricAffinity(metrics, itSecondService->second.ServiceDesc.Metrics, true);
                                }
                            }
                            else
                            {
                                auto itDomain = serviceToDomainTable_.find(childServiceId);
                                if (itDomain != serviceToDomainTable_.end())
                                {
                                    domains.insert(itDomain->second->second.Id);
                                }
                            }
                        }
                    }
                }
            }
            dependedServiceToDomainTable_.erase(it);
        }

        if (isNewService && hasChildren && serviceDescription.PartitionCount > 1)
        {
            Trace.ParentServiceMultipleParitions(serviceDescription);
        }
    }
    if (updateMetricConnections)
    {
        //If it is a new service, we should not bother with trying to Split Domains...
        ExecuteDomainChange(true);
    }

    for (auto itDomain = serviceDomainTable_.begin(); itDomain != serviceDomainTable_.end(); itDomain++)
    {
        DBGASSERT_IF(!metricConnections_.AreMetricsConnected(itDomain->second.Metrics) && isNewService && PLBConfig::GetConfig().SplitDomainEnabled, "Metrics aren't connected before adding service: {0}", serviceDescription.Name);
    }

    ServiceDomainTable::iterator itNewDomain = serviceDomainTable_.end();
    for (auto itMetric = metrics.begin(); itMetric != metrics.end(); ++itMetric)
    {
        auto itMetricToDomain = metricToDomainTable_.find(itMetric->Name);
        if (itMetricToDomain != metricToDomainTable_.end())
        {
            if (updateMetricConnections)
            {
                DBGASSERT_IF(itNewDomain != itMetricToDomain->second && itNewDomain != serviceDomainTable_.end(), "Domains we not properly merged for {0}", metrics);
                //we decided the domain from its metrics.
                itNewDomain = itMetricToDomain->second;
            }
            else
            {
                domains.insert(itMetricToDomain->second->second.Id);
            }
        }

        if (!itMetric->IsBuiltIn && !itMetric->IsMetricOfDefaultServices() &&
            (config.MetricBalancingThresholds.find(itMetric->Name) == config.MetricBalancingThresholds.end() ||
                config.MetricActivityThresholds.find(itMetric->Name) == config.MetricActivityThresholds.end()))
        {
            // Trace warning that metric does not have activity or balancing threshold
            if (traceDetail)
            {
                Trace.MetricDoesNotHaveThreshold(itMetric->Name, serviceDescription.Name);
            }
        }
    }
    //if we're not performing metric connection operations, we get the domain from it's dependencies.
    if (!domains.empty() && !updateMetricConnections)
    {
        itNewDomain = MergeDomains(domains);
    }
    //if we couldn't find any domain, we will create a new one.
    if (itNewDomain == serviceDomainTable_.end())
    {
        ServiceDomain::DomainId newDomainId = GenerateUniqueDomainId(serviceDescription);
        ServiceDomain newDomain(ServiceDomain::DomainId(newDomainId), *this);

        std::wstring prefix = ServiceDomain::GetDomainIdPrefix(newDomainId);
        itNewDomain = serviceDomainTable_.insert(make_pair(move(prefix), move(newDomain))).first;

        if (traceDetail)
        {
            Trace.AddServiceDomain(itNewDomain->second.Id);
        }
    }
    //Various domain addition operations(app+dependedservice+service)...
    if (appHasScaleoutOrCapacity)
    {
        // No need to check if application exists because it is added above and we're under lock
        AddApplicationToDomain(applicationTable_.find(serviceDescription.ApplicationId)->second.ApplicationDesc, itNewDomain, vector<wstring>());
    }
    //if paren't hasnt been created yet, we add depended service to itNewDomain
    if (isParentAbsent)
    {
        AddDependedServiceToDomain(serviceDescription.AffinitizedService, itNewDomain);
    }
    auto serviceId = serviceDescription.ServiceId;

    if (isScalingPolicyChanged || isNewService)
    {
        if (serviceDescription.IsAutoScalingDefined)
        {
            if (serviceDescription.AutoScalingPolicy.IsServiceScaled())
            {
                serviceScalingExpiry = Stopwatch::Now() + serviceDescription.AutoScalingPolicy.GetScalingInterval();
            }
            else
            {
                serviceScalingExpiry = Common::StopwatchTime::Zero;
            }
        }
        else
        {
            serviceScalingExpiry = Common::StopwatchTime::Zero;
        }
    }

    AddServiceToDomain(move(serviceDescription), itNewDomain);

    AutoScaler & autoScaler = itNewDomain->second.AutoScalerComponent;
    autoScaler.AddService(serviceId, serviceScalingExpiry);
    autoScaler.UpdateServicePartitionCount(serviceId, partitionCountChange);

    auto serviceIt = itNewDomain->second.Services.find(serviceId);
    // Keep this updated
    if (serviceIt != itNewDomain->second.Services.end())
    {
        serviceIt->second.NextAutoScaleCheck = serviceScalingExpiry;
    }

    DBGASSERT_IF(itNewDomain->second.CheckMetrics() && PLBConfig::GetConfig().SplitDomainEnabled, "Found metrics with No services and No Applications in {0}", itNewDomain->second.Id);

    //reattach the failover units...
    if (!isNewService)
    {
        auto itServiceToDomain = serviceToDomainTable_.find(serviceId);
        if (itServiceToDomain != serviceToDomainTable_.end())
        {
            auto & service = itServiceToDomain->second->second.GetService(serviceId);
            // add back failover units when update service
            for (auto iter = relatedFailoverUnits.begin(); iter != relatedFailoverUnits.end(); ++iter)
            {
                FailoverUnit ft = *iter;

                ASSERT_IFNOT(originalMetrics.size() == ft.PrimaryEntries.size() &&
                    originalMetrics.size() == ft.SecondaryEntries.size(),
                    "original metrics size does not match with load entry size in failover unit. MetricsSize:{0}, PrimaryEntry:{1}, SecondaryEntry:{2}",
                    originalMetrics.size(), ft.PrimaryEntries.size(), ft.SecondaryEntries.size());

                vector<uint> primaryEntries;
                vector<uint> secondaryEntries;
                map<Federation::NodeId, std::vector<uint>> newSecondaryEntriesMap;

                for (int j = 0; j < metrics.size(); ++j)
                {
                    primaryEntries.push_back(service.GetDefaultMetricLoad(j, ReplicaRole::Primary));
                    secondaryEntries.push_back(service.GetDefaultMetricLoad(j, ReplicaRole::Secondary));
                }
                for (auto it = ft.SecondaryEntriesMap.begin(); it != ft.SecondaryEntriesMap.end(); ++it)
                {
                    newSecondaryEntriesMap.insert(std::make_pair(it->first, secondaryEntries));
                }

                StopwatchTime scalingExpiry = ft.NextScalingCheck;

                //if the scaling policy has changed we need to recompute the next interval to look at it
                //if we have removed the policy now we should just set the interval to zero
                //else we calculate it from this point + scale interval
                if (isScalingPolicyChanged)
                {
                    if (service.ServiceDesc.IsAutoScalingDefined)
                    {
                        if (service.ServiceDesc.AutoScalingPolicy.IsPartitionScaled())
                        {
                            scalingExpiry = Stopwatch::Now() + service.ServiceDesc.AutoScalingPolicy.GetScalingInterval();
                        }
                        else
                        {
                            scalingExpiry = StopwatchTime::Zero;
                        }
                    }
                    else
                    {
                        scalingExpiry = StopwatchTime::Zero;
                    }
                }

                FailoverUnitDescription newFTDescription(ft.FuDescription);

                //if no scaling policy, or if scaling is not per partition, target is always equal to the one from service description
                if (!service.ServiceDesc.IsAutoScalingDefined || !service.ServiceDesc.AutoScalingPolicy.IsPartitionScaled())
                {
                    newFTDescription.TargetReplicaSetSize = service.ServiceDesc.TargetReplicaSetSize;
                }

                //We create FailoverUnit with all default values for Move and Load costs
                FailoverUnit newFt(move(newFTDescription), move(primaryEntries), move(secondaryEntries),
                    move(newSecondaryEntriesMap), service.GetDefaultMoveCost(ReplicaRole::Primary), service.GetDefaultMoveCost(ReplicaRole::Secondary),
                    serviceDescription.OnEveryNode, move(ft.ResourceLoadMap), scalingExpiry);

                //Recostructing FailoverUnit by updating with original primary and secondary load costs
                for (int j = 0; j < metrics.size(); ++j)
                {
                    wstring const & metricName = metrics[j].Name;
                    for (size_t k = 0; k < originalMetrics.size(); ++k)
                    {
                        // If the metric also defined before and it is not default value (updated by service report load),
                        // we should keep the reported load
                        // otherwise we will still use the new default load.
                        if (originalMetrics[k].Name == metricName)
                        {
                            if (areRelatedFailoverUnitsStateful && ft.IsPrimaryLoadReported[k])
                            {
                                newFt.UpdateLoad(ReplicaRole::Primary, static_cast<size_t>(j), ft.PrimaryEntries[k], this->Settings);
                            }
                            if (!this->Settings.UseSeparateSecondaryLoad)
                            {
                                if (ft.IsSecondaryLoadReported[k])
                                {
                                    newFt.UpdateLoad(ReplicaRole::Secondary, static_cast<size_t>(j), ft.SecondaryEntries[k], this->Settings);
                                }
                            }
                            else
                            {
                                for (auto it = ft.IsSecondaryLoadMapReported.begin(); it != ft.IsSecondaryLoadMapReported.end(); ++it)
                                {
                                    if (it->second[k])
                                    {
                                        auto itMap = ft.SecondaryEntriesMap.find(it->first);
                                        if (itMap != ft.SecondaryEntriesMap.end())
                                        {
                                            newFt.UpdateLoad(ReplicaRole::Secondary, static_cast<size_t>(j), itMap->second[k], this->Settings, true, it->first);
                                        }
                                    }
                                }
                            }
                            break;
                        }
                    }
                }
                // Recostructing FailoverUnit by updating with original primary and secondary move costs
                if (ft.IsMoveCostReported(ReplicaRole::Primary))
                {
                    newFt.UpdateMoveCost(ReplicaRole::Primary, ft.PrimaryMoveCost);
                }

                if (ft.IsMoveCostReported(ReplicaRole::Secondary))
                {
                    newFt.UpdateMoveCost(ReplicaRole::Secondary, ft.SecondaryMoveCost);
                }

                auto itServiceToDomain2 = serviceToDomainTable_.find(newFt.FuDescription.ServiceId);
                // Failover units that were deleted for update are reattached to service here. If service is missing in serviceToDomainTable_
                // assert is neccessary, because PLB would lose track of service failover units completely.
                ASSERT_IF(itServiceToDomain2 == serviceToDomainTable_.end(), "Service {0} doesn't exist in service to domain id table", newFt.FuDescription.ServiceId);
                itServiceToDomain2->second->second.AddFailoverUnit(move(newFt), Stopwatch::Now());
            }
        }
    }
    //Split at the end(we check !isNewService, because new services cannot cause a domain split.)...
    if (updateMetricConnections && !isNewService)
    {
        ExecuteDomainChange();
    }

    for (auto itDomain = serviceDomainTable_.begin(); itDomain != serviceDomainTable_.end(); itDomain++)
    {
        DBGASSERT_IF(!metricConnections_.AreMetricsConnected(itDomain->second.Metrics) && PLBConfig::GetConfig().SplitDomainEnabled, "Metrics aren't connected for: {0}", itDomain->second.Id);
    }
    return make_pair(ErrorCodeValue::Success, false);
}

bool PlacementAndLoadBalancing::IsAffinityChain(ServiceDescription & serviceDescription) const
{
    // eliminate affinity self-loop
    if (serviceDescription.AffinitizedService == serviceDescription.Name)
    {
        serviceDescription.ClearAffinitizedService();
    }

    auto &affinitizedService = serviceDescription.AffinitizedService;
    if (affinitizedService.empty())
    {
        return false;
    }

    if (dependedServiceToDomainTable_.find(serviceDescription.Name) != dependedServiceToDomainTable_.end())
    {
        return true;
    }

    auto const& itId = serviceToIdMap_.find(affinitizedService);
    if (itId == serviceToIdMap_.end())
    {
        return false;
    }

    auto it = serviceToDomainTable_.find(itId->second);
    if (it != serviceToDomainTable_.end())
    {
        return it->second->second.HasDependedService(affinitizedService);
    }
    else
    {
        return false;
    }
}

bool PlacementAndLoadBalancing::CheckClusterResource(ServiceDescription & serviceDescription) const
{
    bool hasResource = true;
    int primaryCount = serviceDescription.IsStateful ? serviceDescription.PartitionCount : 0;
    int secondaryCount = serviceDescription.PartitionCount * serviceDescription.TargetReplicaSetSize - primaryCount;

    for (auto itMetric = serviceDescription.Metrics.begin(); itMetric != serviceDescription.Metrics.end(); ++itMetric)
    {
        int64 metricRequiredLoad = primaryCount * static_cast<int64>(itMetric->PrimaryDefaultLoad)
            + secondaryCount * static_cast<int64>(itMetric->SecondaryDefaultLoad);

        PLBConfig const& config = PLBConfig::GetConfig();
        auto itNodeBufferPercentage = config.NodeBufferPercentage.find(itMetric->Name);
        double nodeBufferPercentage = itNodeBufferPercentage != config.NodeBufferPercentage.end() ? itNodeBufferPercentage->second : 0;

        int64 reservedAppCapacity = GetTotalApplicationReservedCapacity(itMetric->Name, serviceDescription.ApplicationId);

        // First, find out If there is an existing service domain
        auto itMetricToDomain = metricToDomainTable_.find(itMetric->Name);
        if (itMetricToDomain == metricToDomainTable_.end())
        {
            if (metricRequiredLoad > GetTotalClusterCapacity(itMetric->Name) - reservedAppCapacity)
            {

                hasResource = false;
                Trace.InsufficientResourcesDetected(
                    serviceDescription.Name,
                    itMetric->Name,
                    metricRequiredLoad,
                    GetTotalClusterCapacity(itMetric->Name),
                    0, // reserved load
                    nodeBufferPercentage,
                    GetTotalClusterCapacity(itMetric->Name));
            }
        }
        else
        {
            ServiceDomainTable::iterator itDomain = itMetricToDomain->second;
            int64 metricTotalRemainingResource = itDomain->second.GetTotalRemainingResource(itMetric->Name, nodeBufferPercentage);

            int64 reservedLoadUsed = itDomain->second.GetTotalReservedLoadUsed(itMetric->Name);

            // PLB object maintain all applications reserved capacity = MinNode * nodeReservation
            // ServiceDomain maintain load used for each application and it is guaranteed to be less than the application reserved capacity
            int64 metricTotalReservedLoad = 0;
            if (reservedAppCapacity > reservedLoadUsed)
            {
                // Capacity is adjusted by removing application for this application, so it may be less than the load
                metricTotalReservedLoad = reservedAppCapacity - reservedLoadUsed;
            }

            auto service = itDomain->second.Services.find(serviceDescription.ServiceId);

            // For existing service, we should subtract the existing load to calculate cluster resource
            if (service != itDomain->second.Services.end())
            {
                service->second.ServiceDesc.Metrics;
                for (auto existingMetricIter = service->second.ServiceDesc.Metrics.begin(); existingMetricIter != service->second.ServiceDesc.Metrics.end(); ++existingMetricIter)
                {
                    if (existingMetricIter->Name == itMetric->Name)
                    {
                        metricRequiredLoad -= (primaryCount * existingMetricIter->PrimaryDefaultLoad + secondaryCount * existingMetricIter->SecondaryDefaultLoad);
                        break;
                    }
                }
            }

            if (metricRequiredLoad > metricTotalRemainingResource - metricTotalReservedLoad)
            {
                hasResource = false;
                Trace.InsufficientResourcesDetected(
                    serviceDescription.Name,
                    itMetric->Name,
                    metricRequiredLoad,
                    metricTotalRemainingResource,
                    metricTotalReservedLoad,
                    nodeBufferPercentage,
                    GetTotalClusterCapacity(itMetric->Name));
            }
        }

        if (!hasResource)
        {
            break;
        }
    }

    return hasResource;
}

bool PlacementAndLoadBalancing::CheckClusterResourceForReservation(ApplicationDescription const& appDescription) const
{
    auto const& appIdIter = applicationToIdMap_.find(appDescription.Name);
    bool isNew = false;
    uint64 applicationId = 0;
    Uint64UnorderedMap<Application>::const_iterator appIter;
    if (appIdIter != applicationToIdMap_.end())
    {
        applicationId = appIdIter->second;
        appIter = applicationTable_.find(applicationId);
        // If there is an ID, but we do not have the application then it is new.
        isNew = (appIter == applicationTable_.end());
    }
    bool hasCapacity = true;

    if (appDescription.MinimumNodes == 0 || appDescription.AppCapacities.size() == 0)
    {
        // No reservation
        return true;
    }

    for (auto const& capacityDescription : appDescription.AppCapacities)
    {
        PLBConfig const& config = PLBConfig::GetConfig();
        wstring const& metricName = capacityDescription.first;
        auto itNodeBufferPercentage = config.NodeBufferPercentage.find(metricName);
        double nodeBufferPercentage = itNodeBufferPercentage != config.NodeBufferPercentage.end() ? itNodeBufferPercentage->second : 0.0;

        // The amount of reservation that we need for this update
        int64 newReservation = appDescription.MinimumNodes * capacityDescription.second.ReservationCapacity;
        // The amount of reservation that we had with existing application description
        int64 oldReservation(0);
        // How much capacity is left for this metric in the cluster
        int64 remainingCapacity(0);
        // How much load are services of this application using
        int64 applicationLoadUsed(0);

        if (!isNew)
        {
            auto const& oldCapacityIter = appIter->second.ApplicationDesc.AppCapacities.find(metricName);
            if (oldCapacityIter != appIter->second.ApplicationDesc.AppCapacities.end())
            {
                // There was an old reservation
                oldReservation = appIter->second.ApplicationDesc.MinimumNodes * oldCapacityIter->second.ReservationCapacity;
            }
        }

        if (oldReservation > newReservation)
        {
            // We are decreasing the reservation for this metric, this is OK regardless of remaining capacity
            continue;
        }

        auto const& itApplicationToDomain = applicationToDomainTable_.find(applicationId);
        if (itApplicationToDomain != applicationToDomainTable_.end())
        {
            ServiceDomain const& serviceDomain = itApplicationToDomain->second->second;
            if (serviceDomain.ApplicationHasLoad(applicationId))
            {
                ApplicationLoad const& appLoad = serviceDomain.GetApplicationLoad(applicationId);

                if (appLoad.MetricLoads.find(metricName) != appLoad.MetricLoads.end())
                {
                    applicationLoadUsed = appLoad.MetricLoads.find(metricName)->second.NodeLoadSum;
                }
            }
        }

        if (applicationLoadUsed > newReservation)
        {
            // We are already using more than reservation, so just accept the update
            continue;
        }

        // Difference between reservation and load that is already used (>=0 because of previous check)
        int64 loadReservationDiff = newReservation - applicationLoadUsed;

        // Calculate for how much we need to increase the reservation
        int64 requiredReservationIncrease = newReservation - oldReservation;
        if (loadReservationDiff < requiredReservationIncrease)
        {
            // In case when we are using more than old reservation then we just need this diff
            requiredReservationIncrease = loadReservationDiff;
        }

        // Get the remaining capacity and reservation load that is used
        int64 reservationLoadUsed(0);
        auto const& itMetricToDomain = metricToDomainTable_.find(metricName);
        if (itMetricToDomain != metricToDomainTable_.end())
        {
            remainingCapacity = itMetricToDomain->second->second.GetTotalRemainingResource(metricName, nodeBufferPercentage);
            // ServiceDomain guarantees that this value will be smaller than reservation in case that more is used
            reservationLoadUsed = itMetricToDomain->second->second.GetTotalReservedLoadUsed(metricName);
        }
        else
        {
            remainingCapacity = GetTotalClusterCapacity(metricName);
            if (remainingCapacity != INT64_MAX)
            {
                // In case when capacity is INT_MAX and buffer is 0.0 this may overflow
                remainingCapacity = static_cast<int64>(remainingCapacity * (1.0 - nodeBufferPercentage));
            }
        }

        // How much load we still have to reserve for all applications that use this metric (sum - already used reservation).
        int64 remainingReservation = GetTotalApplicationReservedCapacity(metricName, 0);
        remainingReservation = remainingReservation - reservationLoadUsed;
        if (remainingReservation < 0)
        {
            wstring errorMessage = wformatString("Remaining reservation load ({0}) is smaller than zero for metric {1} when updating application {2}",
                remainingReservation,
                metricName,
                appDescription.Name);
            Trace.InternalError(errorMessage);
            Common::Assert::TestAssert("{0}", errorMessage);
            // This should not happen because there is a guarantee that reserved load used in service domain is smaller than
            // reserved capacity that is kept in PLB Object! In production, we will just skip the update.
            hasCapacity = false;
            break;
        }

        if (requiredReservationIncrease > remainingCapacity - remainingReservation)
        {
            hasCapacity = false;
            Trace.InsufficientResourceForApplicationUpdate(appDescription.Name, // id
                appDescription,             // Description of the app
                metricName,
                newReservation,             // How much we are requesting
                requiredReservationIncrease,// What is the increase
                remainingCapacity,          // How much capacity is left
                remainingReservation);      // How much are we still reserving for applications
            break;
        }
    }

    return hasCapacity;
}

void PlacementAndLoadBalancing::UpdateNodePropertyConstraintKeyParams()
{
    std::wstring zero = L"0";
    std::wstring FD = L"FaultDomain";
    std::wstring FDPath = Uri(*Reliability::LoadBalancingComponent::Constants::FaultDomainIdScheme, L"", L"").ToString();

    nodePropertyConstraintKeyParams_.clear();

    for (auto const& node : nodes_)
    {
        auto const& nodeProp = node.NodeDescriptionObj.NodeProperties;

        for (auto it = nodeProp.begin(); it != nodeProp.end(); ++it)
        {
            nodePropertyConstraintKeyParams_.insert(std::make_pair(it->first, zero));
        }
    }

    //Inserting a Test Fault Domain so that If the placement constraint contains a Fault domain restriction, the (FD ^ fd:/path expression) properly evaluates to a boolean instead of an undefined literal
    nodePropertyConstraintKeyParams_.insert(std::make_pair(FD, FDPath));
}

bool PlacementAndLoadBalancing::CheckConstraintKeyDefined(ServiceDescription & serviceDescription)
{
    if (!serviceDescription.PlacementConstraints.empty())
    {
        //This method is already called within lock_
        expressionCache_ = Expression::Build(serviceDescription.PlacementConstraints);
        std::wstring error = L"";

        //If the expression is able to be evaluated to true, then this returns true, otherwise it returns whether the err
        return (expressionCache_ != nullptr) ? ((!expressionCache_->Evaluate(nodePropertyConstraintKeyParams_, error, false)) ? error.empty() : true) : false;
    }

    //If no placement constraints are defined, then this check should return true.
    return true;
}

Common::ErrorCode PlacementAndLoadBalancing::ToggleVerboseServicePlacementHealthReporting(bool enabled)
{
    ErrorCode success = ErrorCodeValue::UnspecifiedError;

    {
        AcquireExclusiveLock grab(lock_);

        if (!isMaster_)
        {
            if (!enabled && !verbosePlacementHealthReportingEnabled_)
            {
                //This acts as a failsafe in case there's a logical pathway such that the DiagnosticsTable starts accumulating junk data and isn't cleaning itself out
                plbDiagnosticsSPtr_->ClearDiagnosticsTables();
            }

            verbosePlacementHealthReportingEnabled_ = enabled;
            success = ErrorCodeValue::Success;
            Trace.ToggleVerboseServicePlacementHealthReporting(enabled);
        }
        else
        {
            verbosePlacementHealthReportingEnabled_ = true;
            success = ErrorCodeValue::VerboseFMPlacementHealthReportingRequired;
            Trace.ToggleVerboseServicePlacementHealthReporting(true);
        }
    }

    return success;
}

int64 PlacementAndLoadBalancing::GetTotalClusterCapacity(std::wstring const& metricName) const
{
    if (nodes_.size() == 0)
    {
        return 0;
    }
    else if (clusterMetricCapacity_.find(metricName) == clusterMetricCapacity_.end())
    {
        return INT64_MAX;
    }

    return clusterMetricCapacity_.at(metricName);
}

int64 PlacementAndLoadBalancing::GetTotalApplicationReservedCapacity(std::wstring const& metricName, uint64 appId) const
{
    // Exclude the application for the service creation being checked
    int serviceAppReservedLoad(0);
    Application const* app = GetApplicationPtrCallerHoldsLock(appId);
    if (appId != 0 && nullptr != app)
    {
        auto itService = app->ApplicationDesc.AppCapacities.find(metricName);
        if (itService != app->ApplicationDesc.AppCapacities.end())
        {
            serviceAppReservedLoad = app->ApplicationDesc.MinimumNodes * itService->second.ReservationCapacity;
        }
    }

    auto it = totalReservedCapacity_.find(metricName);
    if (it != totalReservedCapacity_.end())
    {
        return it->second > serviceAppReservedLoad ? it->second - serviceAppReservedLoad : 0;
    }

    return 0;
}

void PlacementAndLoadBalancing::UpdateAppReservedCapacity(ApplicationDescription const& desc, bool isAdd)
{
    if (desc.AppCapacities.empty())
    {
        return;
    }

    map<wstring, ApplicationCapacitiesDescription> const& appCapacity = desc.AppCapacities;
    for (auto it = appCapacity.begin(); it != appCapacity.end(); ++it)
    {
        int capacity = it->second.ReservationCapacity * desc.MinimumNodes;
        if (isAdd)
        {
            AddAppReservedMetricCapacity(it->first, capacity);
        }
        else
        {
            DeleteAppReservedMetricCapacity(it->first, capacity);
        }
    }

}

void PlacementAndLoadBalancing::AddAppReservedMetricCapacity(wstring const& metricName, uint capacity)
{
    auto it = totalReservedCapacity_.find(metricName);
    if (it != totalReservedCapacity_.end())
    {
        it->second += capacity;
    }
    else
    {
        totalReservedCapacity_.insert(make_pair(metricName, capacity));
    }
}

void PlacementAndLoadBalancing::DeleteAppReservedMetricCapacity(wstring const& metricName, uint capacity)
{
    auto it = totalReservedCapacity_.find(metricName);
    ASSERT_IF(it == totalReservedCapacity_.end(), "Metric {0} should be in reserved capacity table {1}", metricName);
    ASSERT_IF(it->second < capacity, "Capacity {0} in table is less than the deleted value", it->second, capacity);

    it->second -= capacity;
}

int64 PlacementAndLoadBalancing::RecomputeTotalClusterCapacity(std::wstring metricName, Federation::NodeId excludedNode) const
{
    int64 clusterCapacity = 0;

    for (auto const& node : nodes_)
    {
        if (!node.NodeDescriptionObj.IsUpAndActivated || excludedNode == node.NodeDescriptionObj.NodeId)
        {
            continue;
        }

        auto const& capacities = node.NodeDescriptionObj.Capacities;

        auto itCapacity = capacities.find(metricName);

        if (itCapacity == capacities.end())
        {
            return INT64_MAX;
        }
        else
        {
            clusterCapacity += static_cast<int64>(itCapacity->second);
        }
    }

    return clusterCapacity;
}

void PlacementAndLoadBalancing::UpdateTotalClusterCapacity(const NodeDescription &oldDescription, const NodeDescription &newDescription, bool isAdded)
{
    // If some of the metrics does not exist on the node, then capacity of the cluster is infinite
    for (auto itMetricCapacity = clusterMetricCapacity_.begin(); itMetricCapacity != clusterMetricCapacity_.end(); itMetricCapacity++)
    {
        if (newDescription.Capacities.find(itMetricCapacity->first) == newDescription.Capacities.end())
        {
            clusterMetricCapacity_[itMetricCapacity->first] = INT64_MAX;
        }
    }
    // For each metric that is defined on this node, we will update the capacity If needed
    for (auto itNodeMetricCapacity = newDescription.Capacities.begin(); itNodeMetricCapacity != newDescription.Capacities.end(); itNodeMetricCapacity++)
    {
        uint64 newClusterCapacity = INT64_MAX;
        uint64 oldNodeCapacity;
        uint64 newNodeCapacity = itNodeMetricCapacity->second;
        if (oldDescription.Capacities.find(itNodeMetricCapacity->first) == oldDescription.Capacities.end())
        {
            oldNodeCapacity = INT64_MAX;
        }
        else
        {
            oldNodeCapacity = oldDescription.Capacities.at(itNodeMetricCapacity->first);
        }
        if (isAdded || (!oldDescription.IsUpAndActivated && newDescription.IsUpAndActivated))
        {
            // Node is added or came back online, increase capacity
            if (clusterMetricCapacity_.find(itNodeMetricCapacity->first) == clusterMetricCapacity_.end())
            {
                if (nodes_.size() == 0)
                {
                    // This is the first node added (if it's not, then capacity is infinite since previous nodes did not have this metric)
                    newClusterCapacity = newNodeCapacity;
                }
            }
            else if (clusterMetricCapacity_[itNodeMetricCapacity->first] != INT64_MAX)
            {
                // If other nodes had this metric, then simply add capacity
                newClusterCapacity = clusterMetricCapacity_[itNodeMetricCapacity->first] + newNodeCapacity;
            }
        }
        else if (oldDescription.IsUpAndActivated && !newDescription.IsUpAndActivated)
        {
            // Node went offline, or became unavailable
            if (oldNodeCapacity == INT64_MAX)
            {
                // Need to recompute entire capacity, in case that this was the only node with inifinte capacity
                newClusterCapacity = RecomputeTotalClusterCapacity(itNodeMetricCapacity->first, newDescription.NodeId);
            }
            else if (clusterMetricCapacity_[itNodeMetricCapacity->first] != INT64_MAX)
            {
                newClusterCapacity = clusterMetricCapacity_[itNodeMetricCapacity->first] - oldNodeCapacity;
            }
        }
        else if (oldDescription.IsUpAndActivated && newDescription.IsUpAndActivated)
        {
            // Node is online, but has changed so check for capacity changes
            if (oldNodeCapacity == INT64_MAX)
            {
                // Capacity wasn't defined before... recompute entire capacity and add this one
                newClusterCapacity = RecomputeTotalClusterCapacity(itNodeMetricCapacity->first, newDescription.NodeId);
                if (newClusterCapacity != INT64_MAX)
                {
                    newClusterCapacity = newClusterCapacity + newNodeCapacity;
                }
            }
            else
            {
                // Only if both new and old node capacity are defined, we update cluster capacity
                if (clusterMetricCapacity_[itNodeMetricCapacity->first] != INT64_MAX)
                {
                    newClusterCapacity = clusterMetricCapacity_[itNodeMetricCapacity->first] + newNodeCapacity - oldNodeCapacity;
                }
            }
        }
        clusterMetricCapacity_[itNodeMetricCapacity->first] = newClusterCapacity;
    }
}

ServiceDomain::DomainId PlacementAndLoadBalancing::GenerateUniqueDomainId(ServiceDescription const& service)
{
    bool hasNonBuiltInMetric = false;
    std::wstring suffix;

    for (auto itMetric = service.Metrics.begin(); itMetric != service.Metrics.end(); ++itMetric)
    {
        if (!itMetric->IsBuiltIn)
        {
            suffix = itMetric->Name;
            hasNonBuiltInMetric = true;
            break;
        }
    }

    if (!hasNonBuiltInMetric)
    {
        suffix = service.Metrics.begin()->Name;
    }

    return GenerateUniqueDomainId(suffix);
}

ServiceDomain::DomainId PlacementAndLoadBalancing::GenerateDefaultDomainId(std::wstring const& metricName)
{
    std::wstring prefix;

    if (deletedDomainId_ == L"")
    {
        prefix = defaultDomainId_;
    }
    else
    {
        prefix = ServiceDomain::GetDomainIdPrefix(deletedDomainId_);
        deletedDomainId_ = L"";
    }

    ServiceDomain::DomainId newDomainId(prefix + L"_" + metricName);

    return newDomainId;
}

ServiceDomain::DomainId PlacementAndLoadBalancing::GenerateUniqueDomainId(std::wstring const& metricName)
{
    std::wstring prefix;

    if (deletedDomainId_ == L"")
    {
        prefix = Guid::NewGuid().ToString();
    }
    else
    {
        prefix = ServiceDomain::GetDomainIdPrefix(deletedDomainId_);
        deletedDomainId_ = L"";
    }

    ServiceDomain::DomainId newDomainId(prefix + L"_" + metricName);

    return newDomainId;
}

void PlacementAndLoadBalancing::UpdateServiceDomainId(ServiceDomainTable::iterator itDomain)
{
    std::wstring oldMetric = ServiceDomain::GetDomainIdSuffix(itDomain->second.Id);
    bool isDomainIdValid = false;
    auto itMetricToDomain = metricToDomainTable_.find(oldMetric);

    if (itMetricToDomain != metricToDomainTable_.end() && itMetricToDomain->second->first == itDomain->first)
    {
        isDomainIdValid = true;
    }

    if (!isDomainIdValid)
    {
        std::wstring prefix = ServiceDomain::GetDomainIdPrefix(itDomain->second.Id);
        ServiceDomain::DomainId newDomainId(prefix + L"_" + itDomain->second.Metrics.begin()->second.Name);
        itDomain->second.UpdateDomainId(newDomainId);
    }
}

void PlacementAndLoadBalancing::AddServiceToDomain(ServiceDescription && serviceDescription, ServiceDomainTable::iterator itDomain)
{
    // add the service to domain id mappings
    bool inserted = serviceToDomainTable_.insert(make_pair(serviceDescription.ServiceId, itDomain)).second;
    ASSERT_IFNOT(inserted, "Insert service to domain id mapping failed");

    // add the metrics to domain id mappings...
    vector<ServiceMetric> const& metrics = serviceDescription.Metrics;
    for (auto itMetric = metrics.begin(); itMetric != metrics.end(); ++itMetric)
    {
        auto itMetricToDomain = metricToDomainTable_.find(itMetric->Name);

        if (itMetricToDomain == metricToDomainTable_.end())
        {
            AddMetricToDomain(itMetric->Name, itDomain);
        }
        else if (itMetricToDomain->second != itDomain)
        {
            ASSERT_IFNOT(itMetricToDomain->second == itDomain, "Metric {0} should be in domain {1}", itMetric->Name, itDomain->second.Id);
        }
    }

    if (serviceDescription.ServicePackageId != 0 && serviceDescription.HasRGMetric())
    {
        rgDomainId_ = itDomain->second.Id;
    }

    // add the service to domain
    Trace.AddServiceToServiceDomain(itDomain->second.Id, serviceDescription.Name);
    itDomain->second.AddService(move(serviceDescription));
}

/// <summary>
/// Adds an application to a ServiceDomain.
/// </summary>
/// <param name="applicationDescription">The application description.</param>
/// <param name="itDomain">Domain Iterator.</param>
bool PlacementAndLoadBalancing::AddApplicationToDomain(
    ApplicationDescription const& applicationDescription,
    ServiceDomainTable::iterator itDomain,
    vector<wstring> const& oldMetrics,
    bool changedAppMetrics)
{
    bool domainChanged = false;
    if (itDomain == serviceDomainTable_.end())
    {
        return false;
    }

    wstring const& applicationName = applicationDescription.Name;
    uint64 applicationId = applicationDescription.ApplicationId;
    // add the service to domain id mappings
    auto appIt = applicationToDomainTable_.find(applicationId);
    ServiceDomainTable::iterator oldDomain = serviceDomainTable_.end();
    if (appIt == applicationToDomainTable_.end())
    {
        domainChanged = true;
        applicationToDomainTable_.insert(make_pair(applicationId, itDomain)).second;
        Trace.AddApplicationToServiceDomain(applicationName, itDomain->second.Id);
    }
    else if (appIt->second != itDomain)
    {
        domainChanged = true;
        Trace.ChangeApplicationServiceDomain(applicationName, appIt->second->second.Id, itDomain->second.Id);
        oldDomain = appIt->second;
        appIt->second = itDomain;
    }
    //If the application has scaleout, we may need to add extra metrics to the metricToDomainTable_
    if (applicationDescription.HasScaleoutOrCapacity())
    {
        for (auto it = applicationDescription.AppCapacities.begin(); it != applicationDescription.AppCapacities.end(); ++it)
        {
            auto itMetric = metricToDomainTable_.find(it->second.MetricName);
            if (itMetric != metricToDomainTable_.end())
            {
                DBGASSERT_IF(itDomain->second.Id != itMetric->second->second.Id, "Metric {0} is in two domains...", it->second.MetricName);
            }

            bool isNewMetric = false;
            if (changedAppMetrics)
            {
                isNewMetric = std::find(oldMetrics.begin(), oldMetrics.end(), itMetric->first) == oldMetrics.end();
            }
            // it is not enough to check only if domain changed
            // it can happen that application added new metric and did not change domain
            // so we have to increase number of applications for that metric
            bool incrementAppCount = domainChanged || isNewMetric;
            AddMetricToDomain(it->second.MetricName, itDomain, incrementAppCount);
            //in case application was MOVED from one domain to another without a merge...
            if (oldDomain != serviceDomainTable_.end())
            {
                oldDomain->second.RemoveMetric(wstring(it->second.MetricName), domainChanged);
            }
        }
    }

    itDomain->second.CheckAndUpdateAppPartitions(applicationId);

    if (domainChanged)
    {
        // Add reservation if needed
        itDomain->second.UpdateApplicationReservedLoad(applicationDescription, true);
    }

    return domainChanged;
}

/// <summary>
/// Merges the ServiceDomains.
/// </summary>
/// <param name="domainIds">The domain ids.</param>
/// <returns></returns>
PlacementAndLoadBalancing::ServiceDomainTable::iterator PlacementAndLoadBalancing::MergeDomains(set<ServiceDomain::DomainId> const& domainIds)
{
    ASSERT_IF(domainIds.empty(), "Service domains empty");

    set<ServiceDomain::DomainId> domainPrefixSet;
    for (auto it = domainIds.begin(); it != domainIds.end(); it++)
    {
        auto prefix = ServiceDomain::GetDomainIdPrefix(*it);
        if (serviceDomainTable_.find(prefix) != serviceDomainTable_.end())
        {
            domainPrefixSet.insert(prefix);
        }
    }
    //If domain Ids specified do not exist, or its only a single domain, some other domain operation has happened, and we don't need to merge.
    if (domainPrefixSet.size() == 1) return serviceDomainTable_.find(*domainPrefixSet.begin());

    auto itFirst = domainPrefixSet.begin();
    ServiceDomainTable::iterator itFirstDomain = serviceDomainTable_.find(*itFirst);
    ASSERT_IF(itFirstDomain == serviceDomainTable_.end(), "First domain doesn't exist");

    // update dependedService->domain mapping
    for (auto itDepended = dependedServiceToDomainTable_.begin(); itDepended != dependedServiceToDomainTable_.end(); ++itDepended)
    {
        auto &v = itDepended->second;
        auto itNewEnd = remove_if(v.begin(), v.end(), [&](ServiceDomainTable::iterator toFind) -> bool { return domainPrefixSet.find(toFind->first) != domainPrefixSet.end(); });
        if (itNewEnd != v.end())
        {
            v.erase(itNewEnd, v.end());
            v.push_back(itFirstDomain);
        }
    }

    for (auto it = ++domainPrefixSet.begin(); it != domainPrefixSet.end(); ++it)
    {
        ServiceDomainTable::iterator itCurrentDomain = serviceDomainTable_.find(*it);
        ASSERT_IF(itCurrentDomain == serviceDomainTable_.end(), "Current domain doesn't exist");

        // modify service mapping
        Uint64UnorderedMap<Service> const& services = itCurrentDomain->second.Services;
        set<uint64> applicationIds;
        for (auto itService = services.begin(); itService != services.end(); ++itService)
        {
            auto itServiceToDomain = serviceToDomainTable_.find(itService->second.ServiceDesc.ServiceId);
            ASSERT_IF(itServiceToDomain == serviceToDomainTable_.end(), "Service {0} doesn't exist in service to domain id table", itService->first);
            itServiceToDomain->second = itFirstDomain;

            applicationIds.insert(itService->second.ServiceDesc.ApplicationId);
        }

        for (auto appId = applicationIds.begin(); appId != applicationIds.end(); appId++)
        {
            auto appToDomainIter = applicationToDomainTable_.find(*appId);
            if (appToDomainIter != applicationToDomainTable_.end())
            {
                appToDomainIter->second = itFirstDomain;
            }
        }

        // modify metric mapping
        map<wstring, ServiceDomainMetric> const& metrics = itCurrentDomain->second.Metrics;
        for (auto itMetric = metrics.begin(); itMetric != metrics.end(); ++itMetric)
        {
            auto itMetricToDomain = metricToDomainTable_.find(itMetric->first);
            ASSERT_IF(itMetricToDomain == metricToDomainTable_.end(), "Metric {0} doesn't exist in metric to domain id table", itMetric->first);
            itMetricToDomain->second = itFirstDomain;
        }

        Trace.MergeServiceDomain(itFirstDomain->first, itCurrentDomain->first);
        itFirstDomain->second.MergeDomain(move(itCurrentDomain->second));

        serviceDomainTable_.erase(*it);
    }

    return itFirstDomain;
}


/// <summary>
/// Attempts a ServiceDomain Split. Executed by removing every service in the domain; this is followed by re-adding them, wherein they may end up in different domains. Very expensive operation, and is done under lock.
/// </summary>
/// <param name="domainIter">The ServiceDomain iterator.</param>
void PlacementAndLoadBalancing::SplitDomain(ServiceDomainTable::iterator domainIter)
{
    wstring domainId = domainIter->first;

    Trace.SplitServiceDomainStart(domainId, serviceDomainTable_.size());

    Uint64UnorderedMap<Service> const& remainingServices = domainIter->second.Services;
    GuidUnorderedMap<FailoverUnit> const& remainingFailoverUnits = domainIter->second.FailoverUnits;

    //We check if the metrics are truly disconnected in the metric connections graph
    if (metricConnections_.AreMetricsConnected(domainIter->second.Metrics))
    {
        return;
    }
    //This codesection lists everything that needs to be emptied
    vector<ServiceDescription> serviceDescriptions;
    vector<FailoverUnit> failoverUnits;
    for (auto it = remainingServices.begin(); it != remainingServices.end(); ++it)
    {
        ServiceDescription serviceDescription = it->second.ServiceDesc;
        serviceDescriptions.push_back(serviceDescription);
    }

    for (auto it = remainingFailoverUnits.begin(); it != remainingFailoverUnits.end(); ++it)
    {
        FailoverUnit failoverUnit = it->second;
        failoverUnits.push_back(failoverUnit);
    }

    //Start by removing every metric in the domain from metricToDomainTable_...
    for (auto it = domainIter->second.Metrics.begin(); it != domainIter->second.Metrics.end(); it++)
    {
        auto itMetricToDomain = metricToDomainTable_.find(it->second.Name);
        if (itMetricToDomain != metricToDomainTable_.end())
        {
            metricToDomainTable_.erase(itMetricToDomain);
        }
    }

    //Remove every service...
    for (auto it = serviceDescriptions.begin(); it != serviceDescriptions.end(); ++it)
    {
        ServiceDescription serviceDescription = *it;
        vector<wstring> deletedMetrics;
        bool depended;
        uint64 applicationId;
        wstring affinitizedService;

        domainIter->second.DeleteService(serviceDescription.ServiceId, serviceDescription.Name, applicationId, deletedMetrics, depended, affinitizedService, false);

        if (!affinitizedService.empty())
        {
            auto itAffinitized = dependedServiceToDomainTable_.find(affinitizedService);
            if (itAffinitized != dependedServiceToDomainTable_.end() &&
                !domainIter->second.HasDependentService(affinitizedService))
            {
                DeleteDependedServiceToDomain(itAffinitized, domainIter);
            }
        }

        auto itServiceToDomain = serviceToDomainTable_.find(serviceDescription.ServiceId);
        if (itServiceToDomain != serviceToDomainTable_.end())
        {
            serviceToDomainTable_.erase(itServiceToDomain);
        }

        auto itApplicationToDomain = applicationToDomainTable_.find(applicationId);
        if (itApplicationToDomain != applicationToDomainTable_.end())
        {
            applicationToDomainTable_.erase(itApplicationToDomain);
        }
    }
    //check for leftover applications...
    vector<uint64> applicationsToDisconnect;
    for (auto it = applicationToDomainTable_.begin(); it != applicationToDomainTable_.end(); it++)
    {
        if (it->second->second.Id == domainIter->second.Id)
            applicationsToDisconnect.push_back(it->first);
    }
    for (auto it = applicationsToDisconnect.begin(); it != applicationsToDisconnect.end(); it++)
    {
        applicationToDomainTable_.erase(*it);
    }
    //delete the servicedomain
    deletedDomainId_ = domainIter->second.Id;
    serviceDomainTable_.erase(domainIter);
    //re-add every service...
    for (auto it = serviceDescriptions.begin(); it != serviceDescriptions.end(); ++it)
    {
        ServiceDescription serviceDescription(*it);
        InternalUpdateService(move(ServiceDescription(*it)), false, false, false);
    }
    //re-add every FT...
    for (auto iter = failoverUnits.begin(); iter != failoverUnits.end(); ++iter)
    {
        FailoverUnit failoverUnit(*iter);
        auto itServiceToDomain = serviceToDomainTable_.find(failoverUnit.FuDescription.ServiceId);
        // Failover units are reattached to services after domain split here. If service is missing in serviceToDomainTable_
        // assert is neccessary, because PLB would lose track of service failover units completely.
        ASSERT_IF(itServiceToDomain == serviceToDomainTable_.end(), "Service {0} doesn't exist in service to domain id table", failoverUnit.FuDescription.ServiceId);
        itServiceToDomain->second->second.AddFailoverUnit(move(failoverUnit), Stopwatch::Now());
    }


    Trace.SplitServiceDomainEnd(domainId, serviceDomainTable_.size());
}

/// <summary>
/// Internally deletes the service.
/// </summary>
/// <param name="serviceName">Name of the service.</param>
/// <param name="permanentDelete">set to <c>true</c> when an external [DeleteService] call was made. set to <c>false</c> when we are resetting the state due to an [UpdateService] call</param>
/// <param name="assertFailoverUnitEmpty">if set to <c>true</c> we assert  if the [FailoverUnit] is empty.</param>
/// <returns>
/// Returns whether the [DeleteService] was successful.
/// </returns>
bool PlacementAndLoadBalancing::InternalDeleteService(wstring const& serviceName, bool permanentDelete, bool assertFailoverUnitEmpty)
{
    //this is what we will return at the end of the function.
    bool deleted = false;
    //Already under lock_ so this is safe
    serviceNameToPlacementConstraintTable_.erase(serviceName);

    uint64 serviceId = GetServiceId(serviceName);
    //service was never really added?
    if (serviceId == 0)
    {
        return deleted;
    }

    auto itServiceToDomain = serviceToDomainTable_.find(serviceId);
    if (itServiceToDomain != serviceToDomainTable_.end()) // to deal with the case where a deleted service be deleted again
    {
        auto itCurrentDomain = itServiceToDomain->second;

        //metrics which must be disconnected because of this service delete...
        vector<wstring> metricsToDisconnect;
        uint64 applicationId(0);

        auto itService = itCurrentDomain->second.Services.find(serviceId);
        DBGASSERT_IF(itService == itCurrentDomain->second.Services.end(), "Service {0} not found in its domain", serviceName);

        if (itService == itCurrentDomain->second.Services.end())
        {
            return deleted;
        }

        auto serviceDesc = itService->second.ServiceDesc;

        if (permanentDelete)
        {
            plbStatistics_.DeleteService(serviceDesc);
        }

        auto nextAutoScalingCheck = itService->second.NextAutoScaleCheck;
        itCurrentDomain->second.AutoScalerComponent.RemoveService(serviceId, nextAutoScalingCheck);

        //just removing metric connections for now. the domainchange will be executed later..
        {
            metricConnections_.AddOrRemoveMetricConnectionsForService(serviceDesc.Metrics, false);
            //removing metric connections with the application...
            ChangeServiceMetricsForApplication(serviceDesc.ApplicationId, serviceName, vector<ServiceMetric>(), serviceDesc.Metrics);
            //removing metric connections due to affinity as child...
            if (!serviceDesc.AffinitizedService.empty())
            {
                auto const& itAffinitizedId = GetServiceId(serviceDesc.AffinitizedService);
                if (itAffinitizedId != 0)
                {
                    auto itAffinitizedService = itCurrentDomain->second.Services.find(itAffinitizedId);
                    if (itAffinitizedService != itCurrentDomain->second.Services.end())
                    {
                        metricConnections_.AddOrRemoveMetricAffinity(serviceDesc.Metrics, itAffinitizedService->second.ServiceDesc.Metrics, false);
                    }
                }
            }
            //removing metric connections due to affinity as parent...
            else
            {
                auto childServices = itCurrentDomain->second.ChildServices.find(serviceName);
                if (childServices != itCurrentDomain->second.ChildServices.end())
                {
                    for (auto itChild = childServices->second.begin(); itChild != childServices->second.end(); itChild++)
                    {
                        auto childServiceId = GetServiceId(*itChild);
                        if (childServiceId != 0)
                        {
                            auto itChildService = itCurrentDomain->second.Services.find(childServiceId);
                            if (itChildService != itCurrentDomain->second.Services.end())
                            {
                                metricConnections_.AddOrRemoveMetricAffinity(serviceDesc.Metrics, itChildService->second.ServiceDesc.Metrics, false);
                            }
                        }
                    }
                }
            }
        }

        //application operations
        //these happen regardless of permanentDelete, since the service could be switching applications...?
        auto appIt = applicationTable_.find(serviceDesc.ApplicationId);
        if (appIt != applicationTable_.end())
        {
            //handle metrics for the case when the last service of an application is deleted...
            if (appIt->second.Services.find(serviceName) != appIt->second.Services.end() && appIt->second.Services.size() == 1)
            {
                //remove applicationToDomainTable_ entry...
                auto appToDomainIt = applicationToDomainTable_.find(appIt->second.ApplicationDesc.ApplicationId);
                if (appToDomainIt != applicationToDomainTable_.end()
                    && appToDomainIt->second->first == itCurrentDomain->first)
                {
                    applicationToDomainTable_.erase(appIt->second.ApplicationDesc.ApplicationId);
                }
                //remove application's capacity metrics from the domain...
                if (appIt->second.ApplicationDesc.HasScaleoutOrCapacity())
                {
                    auto appCapacities = appIt->second.ApplicationDesc.AppCapacities;
                    for (auto metricIt = appCapacities.begin(); metricIt != appCapacities.end(); metricIt++)
                    {
                        if (itCurrentDomain->second.RemoveMetric(metricIt->second.MetricName, true))
                        {
                            auto itMetricToDomain = metricToDomainTable_.find(metricIt->second.MetricName);
                            ASSERT_IF(itMetricToDomain == metricToDomainTable_.end(), "Metric {0} doesn't exist in metric to domain id table", metricIt->second.MetricName);
                            metricToDomainTable_.erase(itMetricToDomain);
                        }
                    }
                }
            }
            // Remove service from application table
            if (appIt != applicationTable_.end())
            {
                appIt->second.RemoveService(serviceName);
            }
        }

        //metrics that disappear as a result of the service delete
        vector<wstring> deletedMetrics;
        //will be set to true if the delete results in a broken affinity relationship
        bool depended;
        wstring affinitizedService;
        itCurrentDomain->second.DeleteService(serviceId, serviceName, applicationId, deletedMetrics, depended, affinitizedService, assertFailoverUnitEmpty);

        if (!affinitizedService.empty())
        {
            auto it = dependedServiceToDomainTable_.find(affinitizedService);
            if (it != dependedServiceToDomainTable_.end() &&
                !itCurrentDomain->second.HasDependentService(affinitizedService))
            {
                DeleteDependedServiceToDomain(it, itCurrentDomain);
            }
        }

        for (auto itMetric = deletedMetrics.begin(); itMetric != deletedMetrics.end(); ++itMetric)
        {
            auto itMetricToDomain = metricToDomainTable_.find(*itMetric);
            ASSERT_IF(itMetricToDomain == metricToDomainTable_.end(), "Metric {0} doesn't exist in metric to domain id table", *itMetric);
            metricToDomainTable_.erase(itMetricToDomain);
        }

        if (itCurrentDomain->second.Services.empty())
        {
            ASSERT_IF(depended, "Domain empty, the service should not be depended by any service");
            Trace.DeleteServiceDomain(itCurrentDomain->first);
            serviceDomainTable_.erase(itCurrentDomain);
        }
        else
        {
            UpdateServiceDomainId(itCurrentDomain);
        }

        serviceToDomainTable_.erase(itServiceToDomain);

        if (depended)
        {
            AddDependedServiceToDomain(serviceName, itCurrentDomain);
        }

        if (permanentDelete)
        {
            // Delete the ID only if we are really deleting the service
            serviceToIdMap_.erase(serviceName);
            //same for trying a splitDomain, only if we are really deleting the service
            ExecuteDomainChange();
            //we check if it worked fine
            for (auto itDomain = serviceDomainTable_.begin(); itDomain != serviceDomainTable_.end(); itDomain++)
            {
                DBGASSERT_IF(!metricConnections_.AreMetricsConnected(itDomain->second.Metrics) && PLBConfig::GetConfig().SplitDomainEnabled, "Metrics aren't connected after deleting service: {0}", serviceName);
            }

            // If service used quorum based logic, remove it from cache set.
            quorumBasedServicesCache_.erase(serviceId);
        }

        // Removes the Service from the diagnostic table's tracking
        plbDiagnosticsSPtr_->RemoveService(wstring(serviceName));
        deleted = true;

        //remove the service from the service package
        auto itServicePackage = servicePackageTable_.find(serviceDesc.ServicePackageId);
        if (itServicePackage != servicePackageTable_.end())
        {
            itServicePackage->second.Services.erase(serviceDesc.ServiceId);
        }
    }

    return deleted;
}

void PlacementAndLoadBalancing::AddDependedServiceToDomain(std::wstring const& service, ServiceDomainTable::iterator itDomain)
{
    auto it = dependedServiceToDomainTable_.find(service);

    if (it == dependedServiceToDomainTable_.end())
    {
        it = dependedServiceToDomainTable_.insert(make_pair(wstring(service), vector<ServiceDomainTable::iterator>())).first;
    }

    auto& domains = it->second;
    if (find(domains.begin(), domains.end(), itDomain) == domains.end())
    {
        domains.push_back(itDomain);
    }
}

void PlacementAndLoadBalancing::DeleteDependedServiceToDomain(DependedServiceToDomainType::iterator itDependedService, ServiceDomainTable::iterator itDomain)
{
    auto &v = itDependedService->second;
    v.erase(remove(v.begin(), v.end(), itDomain), v.end());
    if (v.empty())
    {
        dependedServiceToDomainTable_.erase(itDependedService);
    }
}

void PlacementAndLoadBalancing::InternalUpdateFailoverUnit(FailoverUnitDescription && failoverUnitDescription)
{
    pendingFailoverUnitUpdates_.push_back(make_pair(move(failoverUnitDescription), Stopwatch::Now()));
}

void PlacementAndLoadBalancing::InternalDeleteFailoverUnit(wstring && serviceName, Guid fuId)
{
    pendingFailoverUnitUpdates_.push_back(make_pair(FailoverUnitDescription(fuId, move(serviceName)), Stopwatch::Now()));
}

bool PlacementAndLoadBalancing::ProcessUpdateFailoverUnit(FailoverUnitDescription && failoverUnitDescription, StopwatchTime timeStamp)
{
    uint64 serviceId = 0;
    auto serviceIdIterator = serviceToIdMap_.find(failoverUnitDescription.ServiceName);
    if (serviceIdIterator != serviceToIdMap_.end())
    {
        serviceId = serviceIdIterator->second;
    }
    failoverUnitDescription.ServiceId = serviceId;

    auto itServiceToDomain = serviceToDomainTable_.find(failoverUnitDescription.ServiceId);
    if (failoverUnitDescription.ServiceId == 0 || itServiceToDomain == serviceToDomainTable_.end())
    {
        TESTASSERT_IF(itServiceToDomain == serviceToDomainTable_.end(), "Service {0} doesn't exist", failoverUnitDescription.ServiceName);
        Trace.ServiceDomainNotFound(failoverUnitDescription.ServiceName);
        return false;
    }

    if (!failoverUnitDescription.IsDeleted)
    {
        //If it's not supposed to be deleted no need to update the PLBDiagnostics module here, proceed as usual
        return itServiceToDomain->second->second.UpdateFailoverUnit(move(failoverUnitDescription), timeStamp);
    }
    else
    {
        auto serviceName = failoverUnitDescription.ServiceName;
        auto fUId = failoverUnitDescription.FUId;
        //If it's supposed to be deleted, then update the PLBDiagnostics module if the delete succeeds and return true
        if (itServiceToDomain->second->second.UpdateFailoverUnit(move(failoverUnitDescription), timeStamp))
        {

            plbDiagnosticsSPtr_->RemoveFailoverUnit(move(serviceName), fUId);

            return true;
        }
        else
        {
            return false;
        }
    }
}

void PlacementAndLoadBalancing::InternalUpdateLoadOrMoveCost(LoadOrMoveCostDescription && loadOrMoveCost)
{
    // TODO: should we use Now as timestamp?
    pendingLoadsOrMoveCosts_.push_back(make_pair(move(loadOrMoveCost), Stopwatch::Now()));
}

void PlacementAndLoadBalancing::ProcessUpdateLoadOrMoveCost(LoadOrMoveCostDescription && loadOrMoveCost, StopwatchTime timeStamp)
{
    auto const& itId = serviceToIdMap_.find(loadOrMoveCost.ServiceName);

    if (itId == serviceToIdMap_.end())
    {
        return;
    }

    auto itServiceToDomain = serviceToDomainTable_.find(itId->second);

    if (itServiceToDomain != serviceToDomainTable_.end())
    {
        itServiceToDomain->second->second.UpdateLoadOrMoveCost(move(loadOrMoveCost), timeStamp);
    }
}

void PlacementAndLoadBalancing::TriggerUpdateTargetReplicaCount(Common::Guid const& failoverUnitId, int targetReplicaSetSize) const
{
    failoverManager_.UpdateFailoverUnitTargetReplicaCount(failoverUnitId, targetReplicaSetSize);
}

size_t PlacementAndLoadBalancing::GetMovementThreshold(size_t existingReplicaCount, size_t absoluteThreshold, double percentageThreshold)
{
    size_t maxReplicasToMove = SIZE_T_MAX;

    if (absoluteThreshold != 0)
    {
        maxReplicasToMove = absoluteThreshold;
    }

    if (percentageThreshold > 0.0)
    {
        size_t maxMovesBasedOnPercentage = static_cast<size_t>(ceil(percentageThreshold * existingReplicaCount));
        if (maxMovesBasedOnPercentage < maxReplicasToMove)
        {
            // always pick the more conservative value.
            maxReplicasToMove = maxMovesBasedOnPercentage;
        }
    }

    if (maxReplicasToMove == SIZE_T_MAX)
    {
        // 0 means do not throttle
        maxReplicasToMove = 0;
    }
    return maxReplicasToMove;
}

//Logic for calculating number of allowed movements...
size_t PlacementAndLoadBalancing::GetAllowedMovements(
    PLBSchedulerAction action,
    size_t existingReplicaCount,
    size_t currentPlacementMovementCount,
    size_t currentBalancingMovementCount)
{
    //this value isnt used anyway for phases other than creationwithmove and balancing.....
    if (!action.IsCreationWithMove() && !action.IsBalancing())
    {
        return SIZE_T_MAX;
    }

    size_t allowedMovements = SIZE_T_MAX;
    size_t threshold = GetMovementThreshold(existingReplicaCount,
        PLBConfig::GetConfig().GlobalMovementThrottleThreshold,
        PLBConfig::GetConfig().GlobalMovementThrottleThresholdPercentage);
    size_t placementThreshold = GetMovementThreshold(existingReplicaCount,
        PLBConfig::GetConfig().GlobalMovementThrottleThresholdForPlacement,
        PLBConfig::GetConfig().GlobalMovementThrottleThresholdPercentageForPlacement);
    size_t balancingThreshold = GetMovementThreshold(existingReplicaCount,
        PLBConfig::GetConfig().GlobalMovementThrottleThresholdForBalancing,
        PLBConfig::GetConfig().GlobalMovementThrottleThresholdPercentageForBalancing);
    size_t globalPlacementMoveCount = globalPlacementMovementCounter_.GetCount();
    size_t globalBalancingMoveCount = globalBalancingMovementCounter_.GetCount();

    //using threshold for total number of movements...
    if (threshold != 0)
    {
        if (threshold < (globalBalancingMoveCount + globalPlacementMoveCount))
        {
            allowedMovements = 0;
        }
        else
        {
            allowedMovements = threshold - (globalBalancingMoveCount + globalPlacementMoveCount + currentPlacementMovementCount + currentBalancingMovementCount);
        }
    }

    if (action.IsCreationWithMove())
        //using threshold for placement movements only...
    {
        if (placementThreshold > (globalPlacementMoveCount + currentPlacementMovementCount) && placementThreshold != 0)
        {
            allowedMovements = min(
                allowedMovements,
                placementThreshold - (globalPlacementMoveCount + currentPlacementMovementCount)
            );
        }
    }
    else if (action.IsBalancing())
        //using threshold for balancing movements only...
    {
        if (balancingThreshold  >(globalBalancingMoveCount + currentBalancingMovementCount) && balancingThreshold != 0)
        {
            allowedMovements = min(
                allowedMovements,
                balancingThreshold - (globalBalancingMoveCount + currentBalancingMovementCount)
            );
        }
    }

    return allowedMovements;
}

void PlacementAndLoadBalancing::BeginRefresh(vector<ServiceDomain::DomainData> & searcherDataList, ServiceDomainStats & stats, StopwatchTime refreshTime)
{
    PLBConfig const& config = PLBConfig::GetConfig();

    for (auto const& node : nodes_)
    {
        if (node.NodeDescriptionObj.IsUpAndActivated)
        {
            ++stats.nodeCount_;
        }
    }

    if (stats.nodeCount_ == 0 || serviceDomainTable_.empty())
    {
        return;
    }

    auto PopulateNodeLoadInfo =
        [&](Federation::NodeId nodeId, ServiceModel::NodeLoadInformationQueryResult & nodeViolationData)
        -> Common::ErrorCode
    {
        return (!IsMaster) ?
            GetNodeLoadInformationQueryResult(nodeId, nodeViolationData, true)
            : ErrorCodeValue::PLBNotReady;
    };

    plbDiagnosticsSPtr_->TrackNodeCapacityViolation(PopulateNodeLoadInfo);

    //Determine the Next PLB Action
    bool createSearcher = false;
    size_t totalMovementThrottledFailoverUnits = 0;

    for (auto it = serviceDomainTable_.begin(); it != serviceDomainTable_.end(); ++it)
    {
        ServiceDomain & sd = it->second;
        ServiceDomain::DomainData domainData = sd.RefreshStates(refreshTime, plbDiagnosticsSPtr_);
        sd.AutoScalerComponent.Refresh(refreshTime, sd, upNodeCount_);
        UpdateNextActionPeriod(sd.GetNextActionInterval(refreshTime));
        stats.Update(domainData.state_);
        totalMovementThrottledFailoverUnits += sd.Scheduler.GetMovementThrottledFailoverUnits().size();

        if (!domainData.action_.IsSkip && domainData.action_.Action == PLBSchedulerActionType::None)
        {
            // This is not a valid situation and it is most likely that we have a bug in our code.
            // It is dangerous to assert in production because it may repeat on PLB failover.
            // In this case, we should just skip the service domain and emit an error trace.
            wstring errorMessage = wformatString("Invalid action for domain {0}: Action = {1}",
                domainData.domainId_,
                domainData.action_);
            Trace.InternalError(errorMessage);
            Common::Assert::TestAssert("{0}", errorMessage);
            continue;
        }

        if (domainData.action_.Action != PLBSchedulerActionType::NoActionNeeded && !domainData.action_.IsSkip)
        {
            createSearcher = true;
            if (domainData.state_.PlacementObj == nullptr)
            {
                // This is not a valid situation and it is most likely that we have a bug in our code.
                // It is dangerous to assert in production because it may repeat on PLB failover.
                // In this case, we should just skip the service domain and emit an error trace.
                Common::Assert::TestAssert("Placement should not be null (service domain: {0})", it->second.Id);
                Trace.InternalError(L"Placement object is nullptr in BeginRefresh()");
                continue;
            }
            Trace.PlacementDump(sd.Id, *(domainData.state_.PlacementObj));
        }

        searcherDataList.push_back(move(domainData));
    }

    //Update Throttled FailoverUnit PerfCounters and PendingMovements Base PerfCounters (only the base, since its numerator is updated in RefreshStates)
    LoadBalancingCounters->NumberOfPLBMovementThrottledFailoverUnits.Value += static_cast<Common::PerformanceCounterValue>(totalMovementThrottledFailoverUnits);
    LoadBalancingCounters->NumberOfPLBMovementThrottledFailoverUnitsBase.Increment();
    LoadBalancingCounters->NumberOfPLBPendingMovementsBase.Increment();

    if (createSearcher)
    {
        stopSearching_.store(false);
        searcher_ = make_unique<Searcher>(
            Trace,
            stopSearching_,
            balancingEnabled_,
            plbDiagnosticsSPtr_,
            static_cast<size_t>(config.YieldDurationPer10ms),
            randomSeed_++
            );
    }

    applicationsSafeToUpgrade_.clear();

    if (rgDomainId_ != L"" && applicationsInUpgradeCheckRg_.size() > 0)
    {
        wstring prefix = ServiceDomain::GetDomainIdPrefix(rgDomainId_);
        auto sdIterator = serviceDomainTable_.find(prefix);
        if (sdIterator != serviceDomainTable_.end())
        {
            for (auto & appId : applicationsInUpgradeCheckRg_)
            {
                auto app = GetApplicationPtrCallerHoldsLock(appId);
                if (app != nullptr)
                {
                    bool isOkToUpgrade = true;
                    for (auto servicePackageId : app->ApplicationDesc.ServicePackageIDsToCheck)
                    {
                        auto itServicePackage = app->ApplicationDesc.ServicePackages.find(servicePackageId);
                        if (itServicePackage != app->ApplicationDesc.ServicePackages.end())
                        {
                            auto itServicePackageIndex = servicePackageToIdMap_.find(itServicePackage->second.ServicePackageIdentifier);
                            if (itServicePackageIndex != servicePackageToIdMap_.end())
                            {
                                if (itServicePackage != app->ApplicationDesc.ServicePackages.end())
                                {
                                    isOkToUpgrade = isOkToUpgrade && sdIterator->second.IsSafeToUpgradeSP(itServicePackageIndex->second, itServicePackage->second);
                                }
                            }
                        }
                    }
                    if (isOkToUpgrade)
                    {
                        applicationsSafeToUpgrade_.insert(app->ApplicationDesc.ApplicationIdentifier);
                    }
                }
            }
        }
    }
}

void PlacementAndLoadBalancing::RunSearcher(ServiceDomain::DomainData * searcherDomainData,
    ServiceDomainStats const& stats,
    size_t& totalOperationCount,
    size_t& currentPlacementMovementCount,
    size_t& currentBalancingMovementCount)
{
    this->LoadBalancingCounters->ResetCategoricalCounterCheckStates();

    size_t batchIndex(0);
    if (searcher_)
    {
        batchIndex = searcher_->BatchIndex;
    }
    Placement const& pl = *(searcherDomainData->state_.PlacementObj);

    if (batchIndex > 0 && pl.NewReplicaCount < batchIndex * PLBConfig::GetConfig().PlacementReplicaCountPerBatch)
    {
        return;
    }

    auto trace1 = plbDiagnosticsSPtr_->SchedulersDiagnostics->GetLatestStageDiagnostics(searcherDomainData->domainId_);

    searcherDomainData->dToken_.SetToken(trace1->DecisionGuid, trace1->stage_, trace1->constraintsDiagnosticsData_);

    if (trace1 != nullptr)
    {
        Trace.SchedulerAction(searcherDomainData->domainId_, trace1->DecisionGuid, trace1->metricString_, *trace1);
        Trace.DecisionToken(searcherDomainData->dToken_);

        if (trace1->IsTracingNeeded())
        {
            PublicTrace.Decision(trace1->DecisionGuid, trace1->metricString_, wformatString(*trace1));
        }
    }

    if (searcherDomainData->action_.IsSkip)
    {
        // TODO: tracing already in the PLBScheduler::RefreshAction, move the tracing to here
        return;
    }

    Score originalScore(
        pl.TotalMetricCount,
        pl.LBDomains,
        pl.TotalReplicaCount,
        pl.BalanceCheckerObj->FaultDomainLoads,
        pl.BalanceCheckerObj->UpgradeDomainLoads,
        pl.BalanceCheckerObj->ExistDefragMetric,
        pl.BalanceCheckerObj->ExistScopedDefragMetric,
        settings_,
        &pl.BalanceCheckerObj->DynamicNodeLoads);

    if (searcherDomainData->action_.Action == PLBSchedulerActionType::NoActionNeeded)
    {
        Trace.PLBDomainSkip(
            searcherDomainData->domainId_,
            wformatString("current action is NoActionNeeded ConstraintCheckEnabled:{0} BalancingEnabled:{1} ConfigLoadBalancingEnabled:{2} HasMovableReplica:{3}",
                constraintCheckEnabled_.load(),
                balancingEnabled_.load(),
                PLBConfig::GetConfig().LoadBalancingEnabled,
                searcherDomainData->state_.HasMovableReplica()));
    }
    else
    {
        bool traceMetricInfo = PLBConfig::GetConfig().TraceMetricInfoForBalancingRun;

        Trace.PLBDomainStart(
            searcherDomainData->domainId_,
            searcherDomainData->action_,
            pl.NodeCount,
            pl.Services.size(),
            pl.GetImbalancedServices(),
            pl.PartitionCount,
            pl.ExistingReplicaCount,
            pl.NewReplicaCount,
            pl.PartitionsInUpgradeCount,
            originalScore.AvgStdDev,
            pl.QuorumBasedServicesCount,
            pl.QuorumBasedPartitionsCount);

        if (searcherDomainData->action_.IsBalancing() && traceMetricInfo)
        {
            pl.BalanceCheckerObj->CalculateMetricStatisticsForTracing(true, originalScore, NodeMetrics(pl.BalanceCheckerObj->TotalMetricCount, 0, true));
        }

        StopwatchTime domainStartTime = Stopwatch::Now();

        size_t allowedMovements = GetAllowedMovements(searcherDomainData->action_,
            stats.existingReplicaCount_,
            currentPlacementMovementCount,
            currentBalancingMovementCount);
        CandidateSolution solution = searcher_->SearchForSolution(
            searcherDomainData->action_,
            pl,
            *(searcherDomainData->state_.CheckerObj),
            searcherDomainData->domainId_,
            pl.BalanceCheckerObj->ExistDefragMetric,
            allowedMovements);
        TimeSpan domainDelta = Stopwatch::Now() - domainStartTime;

        searcherDomainData->isInterrupted_ = searcher_->IsInterrupted();
        searcherDomainData->newAvgStdDev_ = solution.AvgStdDev;
        if (searcherDomainData->isInterrupted_)
        {
            searcherDomainData->interruptTime_ = Stopwatch::Now();
            Trace.PLBDomainInterrupted(searcherDomainData->domainId_, searcherDomainData->action_, domainDelta.TotalMilliseconds());
        }

        if (!searcherDomainData->isInterrupted_ || !searcherDomainData->action_.IsBalancing())
        {
            size_t movementCount = 0;
            uint64 noneMoves = 0, swapMoves = 0, moveMoves = 0, addMoves = 0, promoteMoves = 0, addAndPromoteMoves = 0, voidMoves = 0, dropMoves = 0;
            for (size_t moveIndex = 0; moveIndex < solution.MaxNumberOfCreationAndMigration; ++moveIndex)
            {
                Movement const& m = solution.GetMovement(moveIndex);
                if (m.IsValid)
                {
                    movementCount++;
                    switch (m.MoveType) //Distinguish between different moveTypes in PerfCounters
                    {
                    case Movement::Type::None:
                        noneMoves++;
                        break;
                    case Movement::Type::Swap:
                        swapMoves++;
                        break;
                    case Movement::Type::Move:
                        moveMoves++;
                        break;
                    case Movement::Type::Add:
                        addMoves++;
                        break;
                    case Movement::Type::Promote:
                        promoteMoves++;
                        break;
                    case Movement::Type::AddAndPromote:
                        addAndPromoteMoves++;
                        break;
                    case Movement::Type::Void:
                        voidMoves++;
                        break;
                    case Movement::Type::Drop:
                        dropMoves++;
                        break;
                    }

                    // This condition checks for the case where the DummyPLB is enabled after fast balancing was already started and
                    // fast balancing finishes after DummyPLB is set to true -- we should not generate movements in this case
                    if (!(settings_.DummyPLBEnabled && searcherDomainData->action_.IsBalancing()))
                    {
                        PLBSchedulerActionType::Enum schedulerAction = searcherDomainData->action_.Action;

                        if (m.Partition->IsInUpgrade && m.IsSwap &&
                            searcherDomainData->action_.Action == PLBSchedulerActionType::NewReplicaPlacement)
                        {
                            schedulerAction = PLBSchedulerActionType::Upgrade;
                        }

                        AddFailoverUnitMovement(m, schedulerAction, searcherDomainData->movementTable_);
                    }
                }
            }

            //update global interval counters for movements...
            if (searcherDomainData->action_.IsCreationWithMove())
            {
                currentPlacementMovementCount += moveMoves + swapMoves;
            }
            else if (searcherDomainData->action_.IsBalancing())
            {
                currentBalancingMovementCount += moveMoves + swapMoves;
            }
            //record total number of movements for aggregate counters...
            totalOperationCount += movementCount;

            if (totalOperationCount > 0 && searcherDomainData->action_.IsBalancing() && traceMetricInfo)
            {
                solution.OriginalPlacement->BalanceCheckerObj->CalculateMetricStatisticsForTracing(false, solution.SolutionScore, solution.NodeChanges);
            }
            auto movementTypeTuple = make_tuple(noneMoves, swapMoves, moveMoves, addMoves, promoteMoves, addAndPromoteMoves, voidMoves, dropMoves);

            this->LoadBalancingCounters->UpdateCategoricalPerformanceCounters(
                static_cast<uint64>(movementCount),
                static_cast<uint64>(domainDelta.TotalMilliseconds()),
                searcherDomainData->action_, movementTypeTuple);

            Trace.PLBDomainEnd(
                searcherDomainData->domainId_,
                static_cast<int>(movementCount),
                searcherDomainData->action_,
                originalScore.AvgStdDev,
                solution.AvgStdDev,
                domainDelta.TotalMilliseconds());
        }
    }

    this->LoadBalancingCounters->IncrementCategoricalPerformanceCounterBases();
}

void PlacementAndLoadBalancing::EndRefresh(ServiceDomain::DomainData* searcherDomainData, StopwatchTime refreshTime)
{
    UpdateQuorumBasedServicesCache(searcherDomainData);

    auto itServiceDomain = serviceDomainTable_.find(ServiceDomain::GetDomainIdPrefix(searcherDomainData->domainId_));
    if (itServiceDomain != serviceDomainTable_.end())
    {
        if (searcherDomainData->isInterrupted_)
        {
            itServiceDomain->second.OnDomainInterrupted(searcherDomainData->interruptTime_);
        }

        if (false == constraintCheckEnabled_.load() && searcherDomainData->action_.IsConstraintCheck() ||
            false == balancingEnabled_.load() && searcherDomainData->action_.IsBalancing())
        {
            // If constraintCheck/balancing is disabled by FM (fabric upgrade ...), skip the movements
            searcherDomainData->movementTable_.clear();
            return;
        }

        if (!searcherDomainData->action_.IsSkip && searcherDomainData->action_.Action != PLBSchedulerActionType::NoActionNeeded)
        {
            if (!searcherDomainData->isInterrupted_ || !searcherDomainData->action_.IsBalancing())
            {
                itServiceDomain->second.OnMovementGenerated(
                    refreshTime,
                    searcherDomainData->dToken_.DecisionId,
                    searcherDomainData->newAvgStdDev_,
                    move(searcherDomainData->movementTable_));
            }
            itServiceDomain->second.UpdateFailoverUnitWithCreationMoves(searcherDomainData->partitionsWithCreations_);
        }

        searcherDomainData->movementTable_ = itServiceDomain->second.GetMovements(refreshTime);
    }
}

void PlacementAndLoadBalancing::TracePeriodical(ServiceDomainStats& stats, Common::StopwatchTime refreshTime)
{
    int64 nodeCount = stats.nodeCount_;
    for (auto const& node : nodes_)
    {
        if (node.NodeDescriptionObj.IsUpAndActivated)
        {
            ++nodeCount;
        }
    }

    if (nodeCount > 0 && !serviceDomainTable_.empty() &&
        refreshTime - lastPeriodicTrace_ >= PLBConfig::GetConfig().PLBPeriodicalTraceInterval)
    {
        // "1" indicates quorum based logic in replicas domain distribution.
        // "0" indicates "+1" logic in replicas domain distribution.
        vector<int> serviceReplicaDistributionLogic;
        size_t numberOfServices = serviceToIdMap_.size();
        serviceReplicaDistributionLogic.reserve(numberOfServices);

        bool quorumBasedLogicAllServices =
            settings_.QuorumBasedReplicaDistributionPerFaultDomains ||
            settings_.QuorumBasedReplicaDistributionPerUpgradeDomains;

        map<int, const Service*> serviceTable;
        for (auto it = serviceDomainTable_.begin(); it != serviceDomainTable_.end(); ++it)
        {
            for (auto itS = it->second.Services.begin(); itS != it->second.Services.end(); ++itS)
            {
                serviceTable.insert(pair<int, const Service*>(static_cast<int>(serviceTable.size()), &(itS->second)));

                if (quorumBasedLogicAllServices)
                {
                    serviceReplicaDistributionLogic.push_back(1);
                }
                else
                {
                    serviceReplicaDistributionLogic.push_back(
                        quorumBasedServicesCache_.find(itS->second.ServiceDesc.ServiceId) != 
                        quorumBasedServicesCache_.end() ? 1 : 0);
                }
            }
        }

        wstring nextServiceTypeToBeTraced = serviceTypeTable_.begin()->first;
        Trace.PLBPeriodicalTrace(
            TraceVectorDescriptions<Node, NodeDescription>(
                nodes_,
                &nextNodeToBeTraced_,
                PLBConfig::GetConfig().PLBPeriodicalTraceNodeCount
                ),
            TraceDescriptions<uint64, Application, ApplicationDescription, Uint64UnorderedMap<Application>>(
                applicationTable_,
                &nextApplicationToBeTraced_,
                PLBConfig::GetConfig().PLBPeriodicalTraceApplicationCount
                ),
            TraceDescriptions<wstring, ServiceType, ServiceTypeDescription>(
                serviceTypeTable_,
                &nextServiceTypeToBeTraced,
                UINT_MAX),
            TraceDescriptions<int, const Service*, ServiceDescription>(
                serviceTable, &nextServiceToBeTraced_,
                PLBConfig::GetConfig().PLBPeriodicalTraceServiceCount,
                serviceReplicaDistributionLogic
                )
        );

        lastPeriodicTrace_ = refreshTime;
    }
}

void PlacementAndLoadBalancing::CheckAndProcessPendingUpdates()
{
    if (bufferUpdateLock_.IsSet())
    {
        AcquireWriteLock grab(lock_);
        if (ProcessPendingUpdatesCallerHoldsLock(Stopwatch::Now()))
        {
            StopSearcher();
        }
    }
}

bool PlacementAndLoadBalancing::ProcessPendingUpdatesCallerHoldsLock(StopwatchTime now)
{
    Stopwatch timer;
    timer.Start();
    vector<pair<FailoverUnitDescription, StopwatchTime>> fuUpdates;
    vector<pair<LoadOrMoveCostDescription, StopwatchTime>> loadOrMoveCostUpdates;
    vector<pair<NodeDescription, StopwatchTime>> nodeUpdates;
    map<Federation::NodeId, vector<wstring>> nodeImages;
    {
        // Lock and set value to false!
        LockAndSetBooleanLock grab(bufferUpdateLock_, false);

        fuUpdates.swap(pendingFailoverUnitUpdates_);
        loadOrMoveCostUpdates.swap(pendingLoadsOrMoveCosts_);
        nodeUpdates.swap(pendingNodeUpdates_);
        nodeImages.swap(pendingNodeImagesUpdate_);
    }

    size_t numUpdates = fuUpdates.size() + loadOrMoveCostUpdates.size() + nodeUpdates.size() + nodeImages.size();

    for (auto it = nodeUpdates.begin(); it != nodeUpdates.end(); ++it)
    {
        ProcessUpdateNode(move(it->first), now);
    }

    for (auto it = nodeImages.begin(); it != nodeImages.end(); ++it)
    {
        ProcessUpdateNodeImages(it->first, move(it->second));
    }

    bool fuChangeShouldInterrupt = false;
    for (auto it = fuUpdates.begin(); it != fuUpdates.end(); ++it)
    {
        fuChangeShouldInterrupt |= ProcessUpdateFailoverUnit(move(it->first), now);
    }

    for (auto it = loadOrMoveCostUpdates.begin(); it != loadOrMoveCostUpdates.end(); ++it)
    {
        ProcessUpdateLoadOrMoveCost(move(it->first), now);
    }

    timer.Stop();
    Trace.PLBProcessPendingUpdates(numUpdates, timer.ElapsedMilliseconds);
    return fuChangeShouldInterrupt;
}

void PlacementAndLoadBalancing::ProcessPendingUpdatesPeriodicTask()
{
    if (IsDisposed())
    {
        Trace.PLBSkipObjectDisposed();
        return;
    }

    CheckAndProcessPendingUpdates();

    processPendingUpdatesTimer_->Change(PLBConfig::GetConfig().ProcessPendingUpdatesInterval);
}

ServiceType const& PlacementAndLoadBalancing::GetServiceType(ServiceDescription const& desc) const
{
    auto it = serviceTypeTable_.find(desc.ServiceTypeName);
    if (it == serviceTypeTable_.end())
    {
        TESTASSERT_IF(it == serviceTypeTable_.end(), "Service type {0} doesn't exist", desc.ServiceTypeName);
        Trace.ServiceTypeNotFound(desc.Name, desc.ServiceTypeName);
        return dummyServiceType_;
    }

    return  it->second;
}

Application const& PlacementAndLoadBalancing::GetApplication(uint64 applicationId) const
{
    TESTASSERT_IF(applicationId == 0, "Attempting to GetApplication(0)");
    return applicationTable_.find(applicationId)->second;
}

//returns null in case the application is not present!
Application const* PlacementAndLoadBalancing::GetApplicationPtrCallerHoldsLock(uint64 applicationId) const
{
    auto it = applicationTable_.find(applicationId);
    if (it != applicationTable_.end())
    {
        return &(it->second);
    }
    else
    {
        return nullptr;
    }
}

void PlacementAndLoadBalancing::ForEachDomain(function<void(ServiceDomain &)> processor)
{
    for (auto it = serviceDomainTable_.begin(); it != serviceDomainTable_.end(); ++it)
    {
        processor(it->second);
    }
}

void PlacementAndLoadBalancing::ForEachDomain(function<void(ServiceDomain const&)> processor) const
{
    for (auto it = serviceDomainTable_.begin(); it != serviceDomainTable_.end(); ++it)
    {
        processor(it->second);
    }
}

bool PlacementAndLoadBalancing::IsDisposed() const
{
    return disposed_.load();
}

void PlacementAndLoadBalancing::StopSearcher()
{
    stopSearching_.store(true);
}

void PlacementAndLoadBalancing::AddFailoverUnitMovement(
    Movement const& movement,
    PLBSchedulerActionType::Enum schedulerAction,
    FailoverUnitMovementTable  & movementList) const
{
    Guid fuId = movement.Partition->PartitionId;
    auto it = movementList.find(movement.Partition->PartitionId);
    if (it == movementList.end())
    {
        it = movementList.insert(make_pair(fuId, FailoverUnitMovement(
            fuId,
            wstring(movement.Service->Name),
            movement.Service->IsStateful,
            movement.Partition->Version,
            movement.Partition->IsInTransition,
            vector<FailoverUnitMovement::PLBAction>()))).first;
    }

    vector<FailoverUnitMovement::PLBAction> & actions = it->second.Actions;
    Federation::NodeId sourceNode;
    Federation::NodeId targetNode;
    FailoverUnitMovementType::Enum moveType;

    if (movement.IsSwap)
    {
        moveType = FailoverUnitMovementType::SwapPrimarySecondary;
        if (movement.SourceRole == ReplicaRole::Primary)
        {
            sourceNode = movement.SourceNode->NodeId;
            targetNode = movement.TargetNode->NodeId;
        }
        else
        {
            ASSERT_IFNOT(movement.SourceRole == ReplicaRole::Secondary, "Invalid movement role {0}", movement.SourceRole);
            targetNode = movement.SourceNode->NodeId;
            sourceNode = movement.TargetNode->NodeId;
        }
    }
    else if (movement.IsMove)
    {
        if (movement.SourceRole == ReplicaRole::Primary)
        {
            moveType = FailoverUnitMovementType::MovePrimary;
        }
        else if (movement.Service->IsStateful)
        {
            moveType = FailoverUnitMovementType::MoveSecondary;
        }
        else
        {
            moveType = FailoverUnitMovementType::MoveInstance;
        }

        ASSERT_IFNOT(movement.SourceNode != nullptr && movement.TargetNode != nullptr, "Source node or target node should not be null");
        sourceNode = movement.SourceNode->NodeId;
        targetNode = movement.TargetNode->NodeId;
    }
    else if (movement.IsAdd)
    {
        if (movement.TargetToBeAddedReplica->IsPrimary)
        {
            moveType = FailoverUnitMovementType::AddPrimary;
        }
        else if (movement.Service->IsStateful)
        {
            moveType = FailoverUnitMovementType::AddSecondary;
        }
        else
        {
            moveType = FailoverUnitMovementType::AddInstance;
        }

        ASSERT_IFNOT(movement.SourceNode == nullptr && movement.TargetNode != nullptr, "Source node should be null and target node should not be null");
        targetNode = movement.TargetNode->NodeId;
    }
    else if (movement.IsPromote)
    {
        moveType = FailoverUnitMovementType::PromoteSecondary;
        ASSERT_IFNOT(movement.SourceNode == nullptr && movement.TargetNode != nullptr, "Source node should be null and target node should not be null");
        targetNode = movement.TargetNode->NodeId;
    }
    else if (movement.IsVoid)
    {
        moveType = FailoverUnitMovementType::RequestedPlacementNotPossible;
        ASSERT_IFNOT(movement.SourceNode != nullptr && movement.TargetNode == nullptr, "Target node should be null for void move type");
        sourceNode = movement.SourceNode->NodeId;
    }
    else if (movement.IsDrop)
    {
        if (movement.Service->IsStateful)
        {
            moveType = movement.SourceRole == ReplicaRole::Primary ?
                FailoverUnitMovementType::DropPrimary :
                FailoverUnitMovementType::DropSecondary;
        }
        else
        {
            moveType = FailoverUnitMovementType::DropInstance;
        }

        ASSERT_IFNOT(movement.SourceNode != nullptr && movement.TargetNode == nullptr, "Target node should be null for drop move type");
        sourceNode = movement.SourceNode->NodeId;
    }
    else
    {
        Assert::CodingError("Invalid movement type {0}", movement.MoveType);
    }

    actions.push_back(FailoverUnitMovement::PLBAction(sourceNode, targetNode, moveType, schedulerAction));
}

bool PlacementAndLoadBalancing::DoAllReplicasMatch(vector<ReplicaDescription> const& replicas) const
{
    for (ReplicaDescription const& replica : replicas)
    {
        uint64 nodeIndex = GetNodeIndex(replica.NodeId);

        if (nodeIndex == UINT64_MAX)
        {
            TESTASSERT_IF(nodeIndex == UINT64_MAX, "Replica's node doesn't exist");
            // Treat this FT as unstable if we can't find the node. We will fail the move or swap that user initiated.
            return false;
        }
        if (!replica.IsInTransition &&
            (nodes_[nodeIndex].NodeDescriptionObj.NodeInstance != replica.NodeInstance ||
                !nodes_[nodeIndex].NodeDescriptionObj.IsUp))
        {
            return false;
        }
    }

    return true;
}

void PlacementAndLoadBalancing::Test_WaitForTracingThreadToFinish()
{
#if !defined(PLATFORM_UNIX)
    AcquireExclusiveLock lock(testTracingLock_);
#else
    AcquireMutexLock lock(&testTracingLock_);
#endif

    while (!tracingJobQueueFinished_)
    {
        testTracingBarrier_.Sleep(TimeSpan::MaxValue);
    }
}

bool PlacementAndLoadBalancing::Test_IsPLBStable()
{
    AcquireExclusiveLock grab(lock_);
    {

        for (auto it = serviceDomainTable_.begin(); it != serviceDomainTable_.end(); it++)
        {
            for (auto it2 = it->second.FailoverUnits.begin(); it2 != it->second.FailoverUnits.end(); it2++)
            {
                if (it2->second.FuDescription.IsInTransition)
                {
                    return false;
                }
            }
        }

    }
    return true;
}

bool PlacementAndLoadBalancing::IsPLBStable(Guid const & fuId)
{
    AcquireExclusiveLock grab(lock_);
    {
        for (auto it = serviceDomainTable_.begin(); it != serviceDomainTable_.end(); it++)
        {
            auto it2 = find_if(
                it->second.FailoverUnits.begin(),
                it->second.FailoverUnits.end(),
                [&](pair<Guid, FailoverUnit> const & failoverUnit) -> bool { return failoverUnit.second.FUId == fuId; });

            if (it2 != it->second.FailoverUnits.end())
            {
                return !it2->second.FuDescription.IsInTransition;
            }
        }
    }

    return false;
}

Common::ErrorCode PlacementAndLoadBalancing::TriggerPromoteToPrimary(std::wstring const& serviceName, Guid const& fuId, Federation::NodeId newPrimary)
{
    bool force = true;

    if (!settings_.DummyPLBEnabled && !this->Test_IsPLBStable())
    {
        Trace.PLBNotReady();
        return ErrorCodeValue::PLBNotReady;
    }

    //Called to make sure that the FM's update version is in sync with the PLB's -- timing issues can cause inconsistent results otherwise
    CheckAndProcessPendingUpdates();
    map<Guid, FailoverUnitMovement> movementList;
    std::unique_ptr<FailoverUnit> failoverUnitPtr;
    bool isStateful = false;
    ServiceDomainTable::iterator itServiceDomain;

    {
        AcquireReadLock grab(lock_);

        auto const& itServiceId = serviceToIdMap_.find(serviceName);
        if (itServiceId == serviceToIdMap_.end())
        {
            Trace.MoveReplicaOrInstance(L"ServiceNotFound");
            return ErrorCodeValue::ServiceNotFound;
        }

        auto itServiceToDomain = serviceToDomainTable_.find(itServiceId->second);
        if (itServiceToDomain == serviceToDomainTable_.end())
        {
            Trace.MoveReplicaOrInstance(L"ServiceNotFound");
            return ErrorCodeValue::ServiceNotFound;
        }

        itServiceDomain = serviceDomainTable_.find((itServiceToDomain->second)->first);
        if (itServiceDomain == serviceDomainTable_.end())
        {
            Trace.MoveReplicaOrInstance(L"ServiceNotFound");
            return ErrorCodeValue::ServiceNotFound;
        }

        auto itService = itServiceDomain->second.Services.find(itServiceId->second);
        if (itService == itServiceDomain->second.Services.end())
        {
            Trace.MoveReplicaOrInstance(L"ServiceNotFound");
            return ErrorCodeValue::ServiceNotFound;
        }

        isStateful = itService->second.ServiceDesc.IsStateful;
        auto iterator = (itServiceDomain->second).get_FailoverUnits().find(fuId);

        if (iterator == (itServiceDomain->second).get_FailoverUnits().end())
        {
            Trace.FailoverUnitNotFound(fuId);
            return ErrorCodeValue::FMFailoverUnitNotFound;
        }

        failoverUnitPtr = make_unique<FailoverUnit>(iterator->second);
    }

    auto & failoverUnit = *failoverUnitPtr;

    if (failoverUnit.FuDescription.HasPlacement ||
        failoverUnit.FuDescription.IsInTransition ||
        !DoAllReplicasMatch(failoverUnit.FuDescription.Replicas))
    {
        if (failoverUnit.FuDescription.IsInTransition)
        {
            for (ReplicaDescription const& transitionReplica : failoverUnit.FuDescription.Replicas)
            {
                if (transitionReplica.NodeId == newPrimary && transitionReplica.CurrentRole == ReplicaRole::Primary)
                {
                    return ErrorCode().Success();
                }
            }
        }

        Trace.FailoverUnitNotStable(failoverUnit.FUId);
        return ErrorCodeValue::PLBNotReady;
    }

    vector<FailoverUnitMovement::PLBAction> actions;
    Common::ErrorCode candidateErrorRetVal = TriggerPromoteToPrimaryHelper(
        serviceName,
        failoverUnit,
        newPrimary,
        actions,
        force,
        PLBSchedulerActionType::Enum::ClientApiPromoteToPrimary);

    if (!candidateErrorRetVal.IsSuccess())
    {
        return candidateErrorRetVal;
    }

    auto guid = Common::Guid::NewGuid();

    for (FailoverUnitMovement::PLBAction const& actionIt : actions)
    {
        EmitCRMOperationTrace(
            failoverUnit.FUId,
            guid,
            actionIt.SchedulerActionType,
            actionIt.Action,
            failoverUnit.FuDescription.ServiceName,
            actionIt.SourceNode,
            actionIt.TargetNode);
    }

    movementList.insert(
        make_pair(failoverUnit.FUId, FailoverUnitMovement(
            failoverUnit.FUId,
            wstring(failoverUnit.FuDescription.ServiceName),
            isStateful,
            failoverUnit.FuDescription.Version,
            failoverUnit.FuDescription.IsInTransition,
            move(actions))));

    auto moveIt = movementList.find(failoverUnit.FUId);
    if (moveIt != movementList.end())
    {
        AcquireWriteLock grab(lock_);
        itServiceDomain->second.UpdateFailoverUnitWithMoves(moveIt->second);
    }

    StopSearcher();

    failoverManager_.ProcessFailoverUnitMovements(move(movementList), DecisionToken(guid, 1));

    return ErrorCodeValue::Success;
}

Common::ErrorCode PlacementAndLoadBalancing::TriggerPromoteToPrimaryHelper(
    std::wstring const& serviceName,
    FailoverUnit const & failoverUnit,
    Federation::NodeId newPrimary,
    std::vector<FailoverUnitMovement::PLBAction>& actions,
    bool force,
    PLBSchedulerActionType::Enum schedulerAction)
{
    vector<FailoverUnitMovement::PLBAction> tempActions;

    bool newPrimaryExists = false;
    vector<ReplicaDescription>::const_iterator newPrimaryReplica;

    bool primaryExists = false;
    vector<ReplicaDescription>::const_iterator primaryReplica;

    for (auto it = failoverUnit.FuDescription.Replicas.begin(); it != failoverUnit.FuDescription.Replicas.end(); ++it)
    {
        if (it->CurrentRole == ReplicaRole::Primary)
        {
            primaryExists = true;
            primaryReplica = it;
        }

        if (it->NodeId == newPrimary)
        {
            newPrimaryExists = true;
            newPrimaryReplica = it;
        }
    }

    if (newPrimaryExists)
    {
        if (primaryExists)
        {
            if (newPrimaryReplica == primaryReplica)
            {
                Trace.MoveReplicaOrInstance(L"AlreadyPrimaryReplica");
                return ErrorCodeValue::AlreadyPrimaryReplica;
            }
            else
            {
                if (newPrimaryReplica->CurrentRole == ReplicaRole::Secondary)
                {
                    tempActions.push_back(FailoverUnitMovement::PLBAction(primaryReplica->NodeId, newPrimary, FailoverUnitMovementType::SwapPrimarySecondary, schedulerAction));
                }
                else
                {
                    tempActions.push_back(FailoverUnitMovement::PLBAction(primaryReplica->NodeId, newPrimary, FailoverUnitMovementType::MovePrimary, schedulerAction));
                }
            }
        }
        else
        {
            if (newPrimaryReplica->CurrentRole == ReplicaRole::Secondary)
            {
                tempActions.push_back(FailoverUnitMovement::PLBAction(newPrimary, newPrimary, FailoverUnitMovementType::PromoteSecondary, schedulerAction));
            }
            else
            {
                tempActions.push_back(FailoverUnitMovement::PLBAction(newPrimary, newPrimary, FailoverUnitMovementType::AddPrimary, schedulerAction));
            }
        }
    }
    else
    {
        Trace.ReplicaNotFound(failoverUnit.FUId, serviceName, newPrimary);
        return ErrorCodeValue::REReplicaDoesNotExist;
    }

    auto retVal = force ? ErrorCode().Success() : SnapshotConstraintChecker(serviceName, failoverUnit.FUId, newPrimary, newPrimary, ReplicaRole::Primary);

    if (retVal.IsSuccess())
    {
        std::swap(tempActions, actions);
    }

    return retVal;
}

Common::ErrorCode PlacementAndLoadBalancing::TriggerSwapPrimary(
    std::wstring const& serviceName,
    Guid const& fuId,
    Federation::NodeId currentPrimary,
    Federation::NodeId & newPrimary,
    bool force /* = false */,
    bool chooseRandom /* = false */)
{
    if (!settings_.DummyPLBEnabled && (!force && !this->IsPLBStable(fuId)))
    {
        Trace.PLBNotReady();
        return ErrorCodeValue::PLBNotReady;
    }

    CheckAndProcessPendingUpdates();
    map<Guid, FailoverUnitMovement> movementList;
    std::unique_ptr<FailoverUnit> failoverUnitPtr;
    bool isStateful = false;
    ServiceDomainTable::iterator itServiceDomain;

    {
        AcquireReadLock grab(lock_);

        auto const& itServiceId = serviceToIdMap_.find(serviceName);
        if (itServiceId == serviceToIdMap_.end())
        {
            Trace.MoveReplicaOrInstance(L"ServiceNotFound");
            return ErrorCodeValue::ServiceNotFound;
        }

        auto itServiceToDomain = serviceToDomainTable_.find(itServiceId->second);
        if (itServiceToDomain == serviceToDomainTable_.end())
        {
            Trace.MoveReplicaOrInstance(L"ServiceNotFound");
            return ErrorCodeValue::ServiceNotFound;
        }

        itServiceDomain = serviceDomainTable_.find((itServiceToDomain->second)->first);
        if (itServiceDomain == serviceDomainTable_.end())
        {
            Trace.MoveReplicaOrInstance(L"ServiceNotFound");
            return ErrorCodeValue::ServiceNotFound;
        }

        auto itService = itServiceDomain->second.Services.find(itServiceId->second);
        if (itService == itServiceDomain->second.Services.end())
        {
            Trace.MoveReplicaOrInstance(L"ServiceNotFound");
            return ErrorCodeValue::ServiceNotFound;
        }

        isStateful = itService->second.ServiceDesc.IsStateful;
        auto iterator = (itServiceDomain->second).get_FailoverUnits().find(fuId);
        if (iterator == (itServiceDomain->second).get_FailoverUnits().end())
        {
            Trace.FailoverUnitNotFound(fuId);
            return ErrorCodeValue::FMFailoverUnitNotFound;
        }

        failoverUnitPtr = make_unique<FailoverUnit>(iterator->second);
    }

    auto & failoverUnit = *failoverUnitPtr;

    if ((failoverUnit.FuDescription.HasPlacement && !force)
        || failoverUnit.FuDescription.IsInTransition
        || !DoAllReplicasMatch(failoverUnit.FuDescription.Replicas))
    {
        if (failoverUnit.FuDescription.IsInTransition)
        {
            for (ReplicaDescription const& transitionReplica : failoverUnit.FuDescription.Replicas)
            {
                if (transitionReplica.NodeId == newPrimary && transitionReplica.CurrentRole == ReplicaRole::Primary)
                {
                    return ErrorCode().Success();
                }
            }
        }

        Trace.FailoverUnitNotStable(failoverUnit.FUId);
        return ErrorCodeValue::PLBNotReady;
    }

    vector<FailoverUnitMovement::PLBAction> actions;

    Common::ErrorCode candidateErrorRetVal = chooseRandom ?
        FindRandomReplicaHelper(serviceName,
            failoverUnit,
            currentPrimary,
            newPrimary,
            actions,
            ReplicaRole::Primary,
            force,
            PLBSchedulerActionType::Enum::ClientApiMovePrimary,
            isStateful)
        : TriggerSwapPrimaryHelper(
            serviceName,
            failoverUnit,
            currentPrimary,
            newPrimary,
            actions,
            force,
            PLBSchedulerActionType::Enum::ClientApiMovePrimary);

    if (!candidateErrorRetVal.IsSuccess())
    {
        return candidateErrorRetVal;
    }

    auto guid = Common::Guid::NewGuid();

    for (FailoverUnitMovement::PLBAction const& actionIt : actions)
    {
        EmitCRMOperationTrace(
            failoverUnit.FUId,
            guid,
            actionIt.SchedulerActionType,
            actionIt.Action,
            failoverUnit.FuDescription.ServiceName,
            actionIt.SourceNode,
            actionIt.TargetNode);
    }

    movementList.insert(
        make_pair(failoverUnit.FUId, FailoverUnitMovement(
            failoverUnit.FUId,
            wstring(failoverUnit.FuDescription.ServiceName),
            isStateful,
            failoverUnit.FuDescription.Version,
            failoverUnit.FuDescription.IsInTransition,
            move(actions))));

    auto moveIt = movementList.find(failoverUnit.FUId);
    if (moveIt != movementList.end())
    {
        AcquireWriteLock grab(lock_);
        itServiceDomain->second.UpdateFailoverUnitWithMoves(moveIt->second);
    }

    StopSearcher();

    failoverManager_.ProcessFailoverUnitMovements(move(movementList), DecisionToken(guid, 1));
    return ErrorCode().Success();
}

Common::ErrorCode PlacementAndLoadBalancing::TriggerSwapPrimaryHelper(
    std::wstring const& serviceName,
    FailoverUnit const & failoverUnit,
    Federation::NodeId currentPrimary,
    Federation::NodeId newPrimary,
    vector<FailoverUnitMovement::PLBAction>& actions,
    bool force,
    PLBSchedulerActionType::Enum schedulerAction)
{
    vector<FailoverUnitMovement::PLBAction> tempActions;
    for (ReplicaDescription const& replica : failoverUnit.FuDescription.Replicas)
    {
        if (replica.CurrentRole == ReplicaRole::Primary)
        {
            if (currentPrimary != replica.NodeId)
            {
                Trace.CurrentPrimaryNotMatch(replica.NodeId, currentPrimary);
                return ErrorCodeValue::REReplicaDoesNotExist;
            }
        }

        if (replica.NodeId == newPrimary)
        {
            if (replica.CurrentRole == ReplicaRole::None || replica.CurrentRole == ReplicaRole::StandBy || replica.CurrentRole == ReplicaRole::Dropped)
            {
                continue;
            }

            if (replica.CurrentRole == ReplicaRole::Primary)
            {
                Trace.MoveReplicaOrInstance(L"AlreadyPrimaryReplica");
                return ErrorCodeValue::AlreadyPrimaryReplica;
            }

            if (replica.CurrentRole != ReplicaRole::Secondary)
            {
                Trace.ReplicaNotFound(failoverUnit.FUId, serviceName, newPrimary);
                return ErrorCodeValue::REReplicaDoesNotExist;
            }

            tempActions.push_back(
                FailoverUnitMovement::PLBAction(
                    currentPrimary,
                    newPrimary,
                    FailoverUnitMovementType::SwapPrimarySecondary,
                    schedulerAction));
        }
    }

    if (tempActions.empty())
    {
        tempActions.push_back(FailoverUnitMovement::PLBAction(
            currentPrimary,
            newPrimary,
            FailoverUnitMovementType::MovePrimary,
            schedulerAction));
    }

    auto retVal = force ? ErrorCode().Success() : SnapshotConstraintChecker(serviceName, failoverUnit.FUId, currentPrimary, newPrimary, ReplicaRole::Primary);

    if (retVal.IsSuccess())
    {
        std::swap(tempActions, actions);
    }

    return retVal;
}

Common::ErrorCode PlacementAndLoadBalancing::SnapshotConstraintChecker(
    std::wstring const& serviceName,
    Common::Guid const& fuId,
    Federation::NodeId currentNode,
    Federation::NodeId newNode,
    ReplicaRole::Enum role)
{
    ServiceDomain::DomainId domainId;
    {
        AcquireReadLock grab(lock_);

        auto const& itServiceId = serviceToIdMap_.find(serviceName);
        if (itServiceId == serviceToIdMap_.end())
        {
            return ErrorCode(ErrorCodeValue::ServiceNotFound, L"ServiceNotFound");
        }

        auto itServiceToDomain = serviceToDomainTable_.find(itServiceId->second);
        if (itServiceToDomain == serviceToDomainTable_.end())
        {
            return ErrorCode(ErrorCodeValue::ServiceNotFound, L"ServiceNotFound");
        }

        domainId = (itServiceToDomain->second)->first;
    }

    PlacementReplica const * replicaPtr = nullptr;
    std::wstring notReadyErrMsg = L"The Resource Balancer is not yet ready to verify that the move satisfies constraints.  Please retry the action, or use the IgnoreConstraints option to to skip the verification.";

    bool snapshotsExist = false;
    {
        AcquireReadLock grab(snapshotLock_);
        snapshotsExist = (snapshotsOnEachRefresh_ != nullptr);
    }

    if (snapshotsExist)
    {
        AcquireReadLock grab(snapshotLock_);
        Trace.MoveReplicaOrInstance(L"Snapshot Pointer Valid");

        auto & newSnap = (snapshotsOnEachRefresh_->second.CreatedTimeUtc > snapshotsOnEachRefresh_->first.CreatedTimeUtc) ?
            snapshotsOnEachRefresh_->second.ServiceDomainSnapshot : snapshotsOnEachRefresh_->first.ServiceDomainSnapshot;

        auto & snapState = newSnap.at(domainId).state_;

        {
            AcquireExclusiveLock grabBig(lock_);
            snapState.CreatePlacementAndChecker(PartitionClosureType::Full);
        }

        PlacementUPtr const& pl = snapState.PlacementObj;
        CandidateSolution solution(&(*pl), vector<Movement>(), vector<Movement>(), PLBSchedulerActionType::ConstraintCheck);
        TempSolution tempSolution(solution);
        NodeEntry *currentEntry = nullptr, *futureEntry = nullptr;
        auto & tracePtr = Trace;

        tempSolution.ForEachNode(false, [currentNode, newNode, &currentEntry, &futureEntry, &tracePtr](NodeEntry const *n) -> bool
        {
            if (currentNode == n->NodeId)
            {
                currentEntry = const_cast<NodeEntry*>(n);
                tracePtr.MoveReplicaOrInstance(L"Current Node Found");
            }

            if (newNode == n->NodeId)
            {
                futureEntry = const_cast<NodeEntry*>(n);
                tracePtr.MoveReplicaOrInstance(L"Future Node Found");
            }

            return !(currentEntry && futureEntry);
        },
            false,
            false);


        tempSolution.ForEachReplica(false, [fuId, currentNode, newNode, futureEntry, &replicaPtr, &tempSolution, role, &tracePtr](PlacementReplica const * r) -> bool
        {
            if (r->Partition->PartitionId == fuId && r->Role == role && r->Node->NodeId == currentNode)
            {
                replicaPtr = r;
                tracePtr.MoveReplicaOrInstance(L"Replica Found");
                return false;
            }

            return true;
        });

        if (currentEntry && futureEntry && replicaPtr)
        {
            //Prevents the assert from being hit in the case that this is true
            if (!replicaPtr->IsMovable || !replicaPtr->Partition->IsMovable)
            {
                if (!replicaPtr->IsMovable)
                {
                    Trace.MoveReplicaOrInstance(L"Replica Is not Movable");
                }

                if (!replicaPtr->Partition->IsMovable)
                {
                    Trace.MoveReplicaOrInstance(L"Partition Is not Movable");
                }

                return ErrorCode(ErrorCodeValue::PLBNotReady, move(notReadyErrMsg));
            }

            vector<Movement> migrates(1, role == ReplicaRole::Primary ? Movement::CreateMigratePrimaryMovement(replicaPtr->Partition, futureEntry, settings_.IsTestMode) :
                Movement::Create(replicaPtr, futureEntry));

            CandidateSolution newSolution(&(*pl), vector<Movement>(), move(migrates), PLBSchedulerActionType::ConstraintCheck);
            TempSolution newTempSolution(newSolution);
            std::wstring errMsg = L"";
            bool isValid = snapState.CheckerObj->CheckSolutionForTriggerMigrateReplicaValidity(newTempSolution, replicaPtr, errMsg);

            if (!isValid)
            {
                Trace.MoveReplicaOrInstance(L"ConstraintNotSatisfied");
                return ErrorCode(ErrorCodeValue::ConstraintNotSatisfied, move(errMsg));
            }
        }
        else
        {
            if (replicaPtr)
            {
                Trace.MoveReplicaOrInstance(L"NodeNotFound");
            }
            else
            {
                Trace.MoveReplicaOrInstance(L"REReplicaDoesNotExist");
            }
            return replicaPtr ? ErrorCodeValue::NodeNotFound : ErrorCodeValue::REReplicaDoesNotExist;
        }
    }
    else
    {
        Trace.MoveReplicaOrInstance(L"Snapshot does not exist");
        return ErrorCode(ErrorCodeValue::PLBNotReady, move(notReadyErrMsg));
    }

    return ErrorCode().Success();
}

Common::ErrorCode PlacementAndLoadBalancing::FindRandomReplicaHelper(
    std::wstring const& serviceName,
    FailoverUnit const & failoverUnit,
    Federation::NodeId currentNode,
    Federation::NodeId & newNode,
    std::vector<FailoverUnitMovement::PLBAction>& actions,
    ReplicaRole::Enum role,
    bool force,
    PLBSchedulerActionType::Enum schedulerAction,
    bool isStatefull)
{
    Common::Random randomEng(static_cast<unsigned long>(time(0)));
    auto randomResult = FindRandomNodes(serviceName, failoverUnit, currentNode, role, randomEng, force);
    auto & nodes = randomResult.second;
    Trace.MoveReplicaOrInstance(wformatString("{0} nodes available", nodes.size()));

    if (!randomResult.first.IsSuccess())
    {
        Trace.MoveReplicaOrInstance(StringUtility::ToWString(randomResult.first));
        return randomResult.first;
    }

    std::vector<FailoverUnitMovement::PLBAction> actionCache;

    while (nodes.size() != 0)
    {
        size_t indexToRemove = 0;
        auto nodeId = [&nodes, &randomEng, &indexToRemove]() -> Federation::NodeId
        {
            return nodes[indexToRemove = randomEng.Next(static_cast<int>(nodes.size()))];
        }();

        auto errVal = (role == ReplicaRole::Primary) ?
            TriggerSwapPrimaryHelper(serviceName, failoverUnit, currentNode, nodeId, actionCache, true, schedulerAction) :
            TriggerMoveSecondaryHelper(serviceName, failoverUnit, currentNode, nodeId, actionCache, true, schedulerAction, isStatefull);

        if (errVal.IsSuccess())
        {
            newNode = nodeId;
            std::swap(actions, actionCache);
            return ErrorCode().Success();
        }
        else
        {
            Trace.MoveReplicaOrInstance(wformatString("NodeId: {0} eliminated due to Error {1}", StringUtility::ToWString(nodeId), errVal));
        }

        nodes.erase(nodes.begin() + indexToRemove);
    }

    std::wstring errMsg = force ? L"No suitable node was found" : L"No nodes satisfied all the constraints";
    return ErrorCode(ErrorCodeValue::ConstraintNotSatisfied, move(errMsg));
}

std::pair<Common::ErrorCode, std::vector<Federation::NodeId>> PlacementAndLoadBalancing::FindRandomNodes(
    std::wstring const& serviceName,
    FailoverUnit const & failoverUnit,
    Federation::NodeId currentNodeId,
    ReplicaRole::Enum replicaRole,
    Common::Random randomEng,
    bool force)
{
    ServiceDomain::DomainId domainId;
    std::vector<Federation::NodeId> returnSet;
    {
        AcquireReadLock grab(lock_);

        auto const& itServiceId = serviceToIdMap_.find(serviceName);
        if (itServiceId == serviceToIdMap_.end())
        {
            return make_pair(ErrorCode(ErrorCodeValue::ServiceNotFound, L"ServiceNotFound"), returnSet);
        }

        auto itServiceToDomain = serviceToDomainTable_.find(itServiceId->second);
        if (itServiceToDomain == serviceToDomainTable_.end())
        {
            return make_pair(ErrorCode(ErrorCodeValue::ServiceNotFound, L"ServiceNotFound"), returnSet);
        }

        domainId = (itServiceToDomain->second)->first;
    }

    PlacementReplica const * replicaPtr = nullptr;

    bool snapshotsExist = false;
    {
        AcquireReadLock grab(snapshotLock_);
        snapshotsExist = (snapshotsOnEachRefresh_ != nullptr);
    }

    if (snapshotsExist)
    {
        AcquireReadLock grab(snapshotLock_);
        Trace.MoveReplicaOrInstance(L"Snapshot Pointer Valid");

        auto& newSnap = (snapshotsOnEachRefresh_->second.CreatedTimeUtc > snapshotsOnEachRefresh_->first.CreatedTimeUtc) ?
            snapshotsOnEachRefresh_->second.ServiceDomainSnapshot : snapshotsOnEachRefresh_->first.ServiceDomainSnapshot;

        auto& snapState = newSnap.at(domainId).state_;

        {
            AcquireExclusiveLock bigGrab(lock_);
            //Run this to generate the checkerObj
            snapState.CreatePlacementAndChecker(PartitionClosureType::Full);
        }

        PlacementUPtr const& pl = snapState.PlacementObj;
        CandidateSolution solution(&(*pl), vector<Movement>(), vector<Movement>(), PLBSchedulerActionType::ConstraintCheck);
        TempSolution tempSolution(solution);

        for (size_t i = 0; i < pl->PartitionCount; ++i)
        {
            PartitionEntry const& partition = pl->SelectPartition(i);

            if (partition.PartitionId == failoverUnit.FUId)
            {
                Trace.MoveReplicaOrInstance(L"Partition Found");
                partition.ForEachExistingReplica([&](PlacementReplica const* r)
                {
                    Trace.MoveReplicaOrInstance(wformatString("LambdaRole {0}, Parameter Role {1}, LambdaNode {2}, Parameter Node {3} ", r->Role, replicaRole, r->Node->NodeId, currentNodeId));
                    if (r->Role == replicaRole && r->Node->NodeId == currentNodeId)
                    {
                        replicaPtr = r;
                        Trace.MoveReplicaOrInstance(wformatString("MATCHED! LambdaRole {0}, Parameter Role {1}, LambdaNode {2}, Parameter Node {3} ", r->Role, replicaRole, r->Node->NodeId, currentNodeId));
                    }
                }, false, true);

                break;
            }
        }

        if (replicaPtr)
        {
            Trace.MoveReplicaOrInstance(L"Replica Pointer Valid");
            return make_pair(ErrorCode().Success(), snapState.CheckerObj->NodesAvailableForTriggerRandomMove(tempSolution, replicaPtr, randomEng, force));
        }
        else
        {
            if (snapState.serviceDomain_.FailoverUnits.count(failoverUnit.FUId))
            {
                FailoverUnit const& cachedFUState = snapState.serviceDomain_.FailoverUnits.at(failoverUnit.FUId);

                if (cachedFUState.FuDescription.Version != failoverUnit.FuDescription.Version)
                {
                    Trace.MoveReplicaOrInstance(L"Replica was not found because of a Version Mismatch between the FailoverUnits in the Resource Balancer snapshot and current state.");
                    return make_pair(ErrorCode(ErrorCodeValue::PLBNotReady, L"Replica was not found because of a Version Mismatch between the FailoverUnits in the Resource Balancer snapshot and current state."), returnSet);
                }
            }
            else
            {
                Trace.MoveReplicaOrInstance(L"FailoverUnit was Not Found in the Resource Balancer's Snapshot");
                return make_pair(ErrorCode(ErrorCodeValue::PLBNotReady, L"FailoverUnit was Not Found in the Snapshot."), returnSet);
            }

            Trace.MoveReplicaOrInstance(L"The replica pointer was not found despite state consistency between the FM and Resource Balancer.");
            return make_pair(ErrorCode(ErrorCodeValue::PLBNotReady, L"The replica pointer was not found despite state consistency between the FM and PLB."), returnSet);
        }
    }
    else
    {
        Trace.MoveReplicaOrInstance(L"Returning Empty NodeSet by Default");
        return make_pair(ErrorCode(ErrorCodeValue::PLBNotReady, L"The Resource Balancer has not yet bootstrapped a snapshot of the cluster state."), returnSet);
    }
}


Common::ErrorCode PlacementAndLoadBalancing::TriggerMoveSecondaryHelper(
    std::wstring const& serviceName,
    FailoverUnit const & failoverUnit,
    Federation::NodeId currentSecondary,
    Federation::NodeId newSecondary,
    vector<FailoverUnitMovement::PLBAction>& actions,
    bool force,
    PLBSchedulerActionType::Enum schedulerAction,
    bool isStatefull)
{
    vector<FailoverUnitMovement::PLBAction> tempActions;
    for (ReplicaDescription const& replica : failoverUnit.FuDescription.Replicas)
    {
        if (replica.NodeId == newSecondary)
        {
            if (replica.CurrentRole == ReplicaRole::Secondary)
            {
                if (!tempActions.empty())
                {
                    tempActions.clear();
                }
                Trace.MoveReplicaOrInstance(L"AlreadySecondaryReplica");
                return ErrorCodeValue::AlreadySecondaryReplica;
            }

            if (replica.CurrentRole == ReplicaRole::Primary)
            {
                if (!tempActions.empty())
                {
                    tempActions.clear();
                }
                Trace.MoveReplicaOrInstance(L"AlreadyPrimaryReplica");
                return ErrorCodeValue::AlreadyPrimaryReplica;
            }
        }

        if (replica.NodeId == currentSecondary)
        {
            if (replica.CurrentRole != ReplicaRole::Secondary)
            {
                Trace.ReplicaInInvalidState(failoverUnit.FUId);
                return ErrorCodeValue::InvalidReplicaStateForReplicaOperation;
            }

            tempActions.push_back(FailoverUnitMovement::PLBAction(
                currentSecondary,
                newSecondary,
                isStatefull ? FailoverUnitMovementType::MoveSecondary : FailoverUnitMovementType::MoveInstance,
                schedulerAction));
        }
    }

    if (tempActions.empty())
    {
        Trace.ReplicaNotFound(failoverUnit.FUId, serviceName, currentSecondary);
        return ErrorCodeValue::REReplicaDoesNotExist;
    }

    auto retVal = force ? ErrorCode().Success() : SnapshotConstraintChecker(serviceName, failoverUnit.FUId, currentSecondary, newSecondary, ReplicaRole::Secondary);

    if (retVal.IsSuccess())
    {
        std::swap(tempActions, actions);
    }

    return retVal;
}

Common::ErrorCode PlacementAndLoadBalancing::TriggerMoveSecondary(
    std::wstring const& serviceName,
    Guid const& fuId, Federation::NodeId currentSecondary,
    Federation::NodeId & newSecondary,
    bool force /* = false */,
    bool chooseRandom /*= false*/)
{
    if (!settings_.DummyPLBEnabled && (!force && !this->IsPLBStable(fuId)))
    {
        Trace.PLBNotReady();
        return ErrorCodeValue::PLBNotReady;
    }

    // Called to make sure that the FM's update version is in sync with the PLB's -- timing issues can cause inconsistent results otherwise
    CheckAndProcessPendingUpdates();
    map<Guid, FailoverUnitMovement> movementList;
    std::unique_ptr<FailoverUnit> failoverUnitPtr;
    ServiceDomainTable::iterator itServiceDomain;
    bool isStateful = false;

    {
        AcquireReadLock grab(lock_);

        auto const& itServiceId = serviceToIdMap_.find(serviceName);
        if (itServiceId == serviceToIdMap_.end())
        {
            Trace.MoveReplicaOrInstance(L"ServiceNotFound");
            return ErrorCodeValue::ServiceNotFound;
        }

        auto itServiceToDomain = serviceToDomainTable_.find(itServiceId->second);
        if (itServiceToDomain == serviceToDomainTable_.end())
        {
            Trace.MoveReplicaOrInstance(L"ServiceNotFound");
            return ErrorCodeValue::ServiceNotFound;
        }

        itServiceDomain = serviceDomainTable_.find((itServiceToDomain->second)->first);
        if (itServiceDomain == serviceDomainTable_.end())
        {
            Trace.MoveReplicaOrInstance(L"ServiceNotFound");
            return ErrorCodeValue::ServiceNotFound;
        }

        auto itService = itServiceDomain->second.Services.find(itServiceId->second);
        if (itService == itServiceDomain->second.Services.end())
        {
            Trace.MoveReplicaOrInstance(L"ServiceNotFound");
            return ErrorCodeValue::ServiceNotFound;
        }

        isStateful = itService->second.ServiceDesc.IsStateful;
        auto iterator = (itServiceDomain->second).get_FailoverUnits().find(fuId);
        if (iterator == (itServiceDomain->second).get_FailoverUnits().end())
        {
            Trace.FailoverUnitNotFound(fuId);
            return ErrorCodeValue::FMFailoverUnitNotFound;
        }

        failoverUnitPtr = make_unique<FailoverUnit>(iterator->second);
    }

    auto & failoverUnit = *failoverUnitPtr;

    if ((failoverUnit.FuDescription.HasPlacement && !force)
        || failoverUnit.FuDescription.IsInTransition
        || !DoAllReplicasMatch(failoverUnit.FuDescription.Replicas))
    {
        if (failoverUnit.FuDescription.IsInTransition)
        {
            for (ReplicaDescription const& transitionReplica : failoverUnit.FuDescription.Replicas)
            {
                if (transitionReplica.NodeId == newSecondary)
                {
                    return ErrorCode().Success();
                }
            }
        }

        Trace.FailoverUnitNotStable(failoverUnit.FUId);
        return ErrorCodeValue::PLBNotReady;
    }

    vector<FailoverUnitMovement::PLBAction> actions;

    Common::ErrorCode candidateErrorRetVal = chooseRandom ?
        FindRandomReplicaHelper(
            serviceName,
            failoverUnit,
            currentSecondary,
            newSecondary,
            actions,
            ReplicaRole::Secondary,
            force,
            PLBSchedulerActionType::Enum::ClientApiMoveSecondary,
            isStateful) :
        TriggerMoveSecondaryHelper(
            serviceName,
            failoverUnit,
            currentSecondary,
            newSecondary,
            actions,
            force,
            PLBSchedulerActionType::Enum::ClientApiMoveSecondary,
            isStateful);

    if (!candidateErrorRetVal.IsSuccess())
    {
        return candidateErrorRetVal;
    }

    auto guid = Common::Guid::NewGuid();

    for (FailoverUnitMovement::PLBAction const& actionIt : actions)
    {
        EmitCRMOperationTrace(
            failoverUnit.FUId,
            guid,
            actionIt.SchedulerActionType,
            actionIt.Action,
            failoverUnit.FuDescription.ServiceName,
            actionIt.SourceNode,
            actionIt.TargetNode);
    }

    movementList.insert(
        make_pair(failoverUnit.FUId, FailoverUnitMovement(
            failoverUnit.FUId,
            wstring(failoverUnit.FuDescription.ServiceName),
            isStateful,
            failoverUnit.FuDescription.Version,
            failoverUnit.FuDescription.IsInTransition,
            move(actions))));

    auto moveIt = movementList.find(failoverUnit.FUId);
    if (moveIt != movementList.end())
    {
        AcquireWriteLock grab(lock_);
        itServiceDomain->second.UpdateFailoverUnitWithMoves(moveIt->second);
    }

    StopSearcher();

    failoverManager_.ProcessFailoverUnitMovements(move(movementList), DecisionToken(guid, 1));
    return ErrorCode().Success();
}

Snapshot PlacementAndLoadBalancing::TakeSnapShot()
{
    Stopwatch timer;
    timer.Start();
    Trace.SnapshotStart();

    map<wstring, ServiceDomain::DomainData> serviceDomainMap;

    for (auto iter = serviceDomainTable_.begin(); iter != serviceDomainTable_.end(); ++iter)
    {
        ServiceDomain::DomainData domainData = iter->second.TakeSnapshot(plbDiagnosticsSPtr_);
        serviceDomainMap.insert(pair<wstring, ServiceDomain::DomainData>(iter->first, move(domainData)));
    }

    // We want to snapshot metrics that do have capacity but no services.
    wstring prefix = L"";
    unique_ptr<ServiceDomain> tempDomainUPtr;
    for (auto metricCapacity : clusterMetricCapacity_)
    {
        if (metricToDomainTable_.find(metricCapacity.first) == metricToDomainTable_.end())
        {
            // If a metric does have capacity, and it is not in any domain add it to snaphot
            if (prefix == L"")
            {
                auto domainId = GenerateDefaultDomainId(metricCapacity.first);
                prefix = ServiceDomain::GetDomainIdPrefix(domainId);
                tempDomainUPtr = make_unique<ServiceDomain>(move(domainId), *this);
            }
            tempDomainUPtr->AddMetric(metricCapacity.first, false);
        }
    }

    if (prefix != L"")
    {
        ServiceDomain::DomainData domainData = tempDomainUPtr->TakeSnapshot(plbDiagnosticsSPtr_);
        serviceDomainMap.insert(pair<wstring, ServiceDomain::DomainData>(prefix, move(domainData)));
    }

    Snapshot snapshot = move(Snapshot(move(serviceDomainMap)));

    wstring rgDomainId = L"";
    auto const& itCpuDomain = metricToDomainTable_.find(*ServiceModel::Constants::SystemMetricNameCpuCores);
    if (itCpuDomain != metricToDomainTable_.end())
    {
        rgDomainId = itCpuDomain->second->first;
    }
    else {
        auto const& itMetricCapacity = clusterMetricCapacity_.find(*ServiceModel::Constants::SystemMetricNameCpuCores);
        if (itMetricCapacity != clusterMetricCapacity_.end())
        {
            rgDomainId = prefix;
        }
    }
    snapshot.SetRGDomain(rgDomainId);

    timer.Stop();

    Trace.SnapshotEnd(timer.ElapsedMilliseconds);

    return snapshot;
}

void PlacementAndLoadBalancing::TrackDroppedPLBMovements()
{
    //This method is used to trace when the FM drops PLB movements and update the PLB Perf Counters to track the moving average of dropped movements.
    {
        AcquireExclusiveLock grab(droppedMovementLock_);
        if (!droppedMovementQueue_.empty())
        {
            //Perf Counters should be updated only If there are movements in the queue, or else the moving average denominator gets skewed
            LoadBalancingCounters->NumberOfPLBMovementsDroppedByFM.Value += static_cast<Common::PerformanceCounterValue>(droppedMovementQueue_.size());
            LoadBalancingCounters->NumberOfPLBMovementsDroppedByFMBase.Increment();

            while (!droppedMovementQueue_.empty())
            {
                auto droppedMovementTuple = move(droppedMovementQueue_.front());
                EmitCRMOperationIgnoredTrace(get<0>(droppedMovementTuple), get<1>(droppedMovementTuple), get<2>(droppedMovementTuple));
                plbDiagnosticsSPtr_->TrackDroppedPLBMovement(get<0>(droppedMovementTuple));
                droppedMovementQueue_.pop();
            }

            while (!executedMovementQueue_.empty())
            {
                plbDiagnosticsSPtr_->TrackExecutedPLBMovement(executedMovementQueue_.front());
                executedMovementQueue_.pop();
            }
        }
    }
}

void PlacementAndLoadBalancing::UpdateQuorumBasedServicesCache(ServiceDomain::DomainData* searcherDomainData)
{
    if (searcherDomainData == nullptr)
    {
        return;
    }

    PlacementUPtr const& pl = searcherDomainData->state_.PlacementObj;

    if (pl == nullptr)
    {
        return;
    }

    if (settings_.QuorumBasedLogicAutoSwitch &&
        !settings_.QuorumBasedReplicaDistributionPerFaultDomains &&
        !settings_.QuorumBasedReplicaDistributionPerUpgradeDomains)
    {
        std::vector<ServiceEntry> const& services = pl->Services;

        for (auto it = services.begin(); it != services.end(); ++it)
        {
            auto currentService = serviceToIdMap_.find(it->Name);
            if (currentService == serviceToIdMap_.end())
            {
                continue;
            }

            if (pl->QuorumBasedServicesTempCache.find(currentService->second) != pl->QuorumBasedServicesTempCache.end())
            {
                quorumBasedServicesCache_.insert(currentService->second);
            }
            else
            {
                quorumBasedServicesCache_.erase(currentService->second);
            }
        }
    }
    else
    {
        quorumBasedServicesCache_.clear();
    }
}

bool PlacementAndLoadBalancing::IsClusterUpgradeInProgress()
{
    return clusterUpgradeInProgress_.load();
}

void PlacementAndLoadBalancing::OnDroppedPLBMovement(Common::Guid const& failoverUnitId, Reliability::PlbMovementIgnoredReasons::Enum reason, Common::Guid const& decisionId)
{
    {
        AcquireExclusiveLock grab(droppedMovementLock_);
        droppedMovementQueue_.push(make_tuple(failoverUnitId, reason, decisionId));
    }
}

void PlacementAndLoadBalancing::OnDroppedPLBMovements(std::map<Common::Guid, FailoverUnitMovement> && droppedMovements, Reliability::PlbMovementIgnoredReasons::Enum reason, Common::Guid const& decisionId)
{
    {
        AcquireExclusiveLock grab(droppedMovementLock_);

        for (auto it = droppedMovements.begin(); it != droppedMovements.end(); ++it)
        {
            droppedMovementQueue_.push(make_tuple(it->second.FailoverUnitId, reason, decisionId));
        }
    }
}

void PlacementAndLoadBalancing::OnExecutePLBMovement(Common::Guid const & partitionId)
{
    AcquireExclusiveLock grab(droppedMovementLock_);
    executedMovementQueue_.push(partitionId);
}

void PlacementAndLoadBalancing::OnFMBusy()
{
    //TODO: add code to slowdown requests to FM
    //RDBUG 7429685
}

Common::ErrorCode PlacementAndLoadBalancing::GetClusterLoadInformationQueryResult(ServiceModel::ClusterLoadInformationQueryResult & queryResult)
{
    std::vector<ServiceModel::LoadMetricInformation> loadMetricInformationVec;
    map<wstring, ServiceModel::LoadMetricInformation> metricsMap;
    std::shared_ptr<std::pair<Snapshot, Snapshot>> snapshots;

    //this section is under snapshotLock_
    {
        AcquireReadLock grab(snapshotLock_);
        if (!snapshotsOnEachRefresh_)
        {
            return ErrorCodeValue::PLBNotReady;
        }

        //get the snapshots taken at last refresh interval
        snapshots = snapshotsOnEachRefresh_;
    }

    Common::DateTime startTimeUtc = snapshots->first.CreatedTimeUtc;
    Common::DateTime endTimeUtc = snapshots->second.CreatedTimeUtc;

    //go through the snapshot and create the data structure for the query

    for (auto iter = snapshots->first.ServiceDomainSnapshot.begin(); iter != snapshots->first.ServiceDomainSnapshot.end(); iter++)
    {
        TESTASSERT_IFNOT(iter->second.state_.PlacementObj, "Placement object before refresh is not available for service domain: {0}", iter->second.domainId_);
        if (!iter->second.state_.PlacementObj)
        {
            Trace.InternalError(L"GetClusterLoadInformationQueryResult(): Placement object is nullptr in shapshot.");
            return ErrorCodeValue::PLBNotReady;
        }
        Placement const& pl = *(iter->second.state_.PlacementObj);

        Score originalScore(
            pl.TotalMetricCount,
            pl.LBDomains,
            pl.TotalReplicaCount,
            pl.BalanceCheckerObj->FaultDomainLoads,
            pl.BalanceCheckerObj->UpgradeDomainLoads,
            pl.BalanceCheckerObj->ExistDefragMetric,
            pl.BalanceCheckerObj->ExistScopedDefragMetric,
            settings_,
            &pl.BalanceCheckerObj->DynamicNodeLoads);

        for (auto i = pl.LBDomains.begin(); i != pl.LBDomains.end(); ++i)
        {
            vector<Metric> const & metrics = i->Metrics;
            for (auto j = metrics.begin(); j != metrics.end(); ++j)
            {
                ServiceModel::LoadMetricInformation loadMetricInformation;
                loadMetricInformation.put_Name(j->Name);
                loadMetricInformation.IsBalancedBefore = j->IsBalanced;
                loadMetricInformation.DeviationBefore = originalScore.CalculateAvgStdDevForMetric(j->Name);
                PLBSchedulerAction action = iter->second.action_;
                loadMetricInformation.Action = action.ToQueryString();
                loadMetricInformation.MaxNodeLoadValue = -1;
                loadMetricInformation.MinNodeLoadValue = -1;
                loadMetricInformation.MaximumNodeLoad = -1;
                loadMetricInformation.MinimumNodeLoad = -1;
                metricsMap[j->Name] = loadMetricInformation;
            }
        }
    }

    for (auto iter = snapshots->second.ServiceDomainSnapshot.begin(); iter != snapshots->second.ServiceDomainSnapshot.end(); iter++)
    {
        TESTASSERT_IFNOT(iter->second.state_.PlacementObj, "Placement object after refresh is not available for service domain: {0}", iter->second.domainId_);
        if (!iter->second.state_.PlacementObj)
        {
            Trace.InternalError(L"GetClusterLoadInformationQueryResult(): Placement object is nullptr in shapshot.");
            return ErrorCodeValue::PLBNotReady;
        }

        Placement const& pl = *(iter->second.state_.PlacementObj);

        Score originalScore(
            pl.TotalMetricCount,
            pl.LBDomains,
            pl.TotalReplicaCount,
            pl.BalanceCheckerObj->FaultDomainLoads,
            pl.BalanceCheckerObj->UpgradeDomainLoads,
            pl.BalanceCheckerObj->ExistDefragMetric,
            pl.BalanceCheckerObj->ExistScopedDefragMetric,
            settings_,
            &pl.BalanceCheckerObj->DynamicNodeLoads);

        if (pl.LBDomains.size() > 0)
        {
            // Calculate cluster reserved load per metric
            LoadEntry metricReservationLoad(pl.GlobalMetricCount);
            pl.ApplicationReservedLoads.ForEach([&](std::pair<NodeEntry const*, LoadEntry> const& item) -> bool
            {
                metricReservationLoad += item.second;
                return true;
            });

            // Use global load balancing domain,
            // as it contains info for all metrics in a system
            auto& globalLBDomain = pl.LBDomains.back();
            vector<Metric> const & metrics = globalLBDomain.Metrics;
            size_t metricIdx = 0;
            for (auto j = metrics.begin(); j != metrics.end(); ++j, ++metricIdx)
            {
                ServiceModel::LoadMetricInformation & loadMetricInformation = metricsMap[j->Name];
                loadMetricInformation.IsBalancedAfter = j->IsBalanced;
                loadMetricInformation.DeviationAfter = originalScore.CalculateAvgStdDevForMetric(j->Name);
                loadMetricInformation.BalancingThreshold = j->BalancingThreshold;
                loadMetricInformation.ActivityThreshold = j->ActivityThreshold;
                loadMetricInformation.ClusterCapacity = j->ClusterTotalCapacity;
                int64 clusterLoad = j->ClusterLoad + metricReservationLoad.Values[metricIdx];
                loadMetricInformation.ClusterLoad = clusterLoad;

                loadMetricInformation.RemainingUnbufferedCapacity = 0;
                if (clusterLoad < j->ClusterTotalCapacity)
                {
                    loadMetricInformation.RemainingUnbufferedCapacity = j->ClusterTotalCapacity - clusterLoad;
                }

                PLBConfig const& config = PLBConfig::GetConfig();
                auto itNodeBufferPercentage = config.NodeBufferPercentage.find(j->Name);
                loadMetricInformation.NodeBufferPercentage = itNodeBufferPercentage != config.NodeBufferPercentage.end() ?
                    itNodeBufferPercentage->second : 0;

                loadMetricInformation.BufferedCapacity = j->ClusterBufferedCapacity;

                loadMetricInformation.RemainingBufferedCapacity = 0;
                loadMetricInformation.IsClusterCapacityViolation = false;

                if (j->ClusterBufferedCapacity > 0)
                {
                    if (clusterLoad > j->ClusterBufferedCapacity)
                    {
                        loadMetricInformation.IsClusterCapacityViolation = true;
                    }
                    else
                    {
                        loadMetricInformation.RemainingBufferedCapacity = j->ClusterBufferedCapacity - clusterLoad;
                    }
                }

                if (loadMetricInformation.Name == ServiceModel::Constants::SystemMetricNameCpuCores)
                {
                    // We need to correct the values for this metric
                    double doubleTotalLoad = static_cast<double>(loadMetricInformation.ClusterLoad) / ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor;
                    double doubleTotalCapacity = static_cast<double>(loadMetricInformation.ClusterCapacity) / ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor;
                    double doubleBufferedCapacity = static_cast<double>(loadMetricInformation.BufferedCapacity) / ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor;

                    double doubleRemainingCapacity = doubleTotalCapacity - doubleTotalLoad;
                    if (doubleRemainingCapacity < 0)
                    {
                        doubleRemainingCapacity = 0;
                    }
                    double doubleRemainingBufferedCapacity = doubleBufferedCapacity - doubleTotalLoad;
                    if (doubleRemainingBufferedCapacity < 0)
                    {
                        doubleRemainingBufferedCapacity = 0;
                    }

                    loadMetricInformation.ClusterCapacity = static_cast<int64>(ceil(doubleTotalCapacity));
                    loadMetricInformation.BufferedCapacity = static_cast<int64>(ceil(doubleBufferedCapacity));
                    loadMetricInformation.CurrentClusterLoad = doubleTotalLoad;
                    loadMetricInformation.BufferedClusterCapacityRemaining = doubleRemainingBufferedCapacity;
                    loadMetricInformation.ClusterCapacityRemaining = doubleRemainingCapacity;
                    loadMetricInformation.ClusterLoad = static_cast<int64>(ceil(doubleTotalLoad));
                    loadMetricInformation.RemainingBufferedCapacity = static_cast<int64>(floor(doubleRemainingBufferedCapacity));
                    loadMetricInformation.RemainingUnbufferedCapacity = static_cast<int64>(floor(doubleRemainingCapacity));
                }
                else
                {
                    // Double values;
                    loadMetricInformation.CurrentClusterLoad = static_cast<double>(loadMetricInformation.ClusterLoad);
                    loadMetricInformation.BufferedClusterCapacityRemaining = static_cast<double>(loadMetricInformation.RemainingBufferedCapacity);
                    loadMetricInformation.ClusterCapacityRemaining = static_cast<double>(loadMetricInformation.RemainingUnbufferedCapacity);
                }
            }
        }

        for (size_t i = 0; i < pl.NodeCount; ++i)
        {
            NodeEntry const & node = pl.SelectNode(i);
            size_t globalMetricCount = pl.GlobalMetricCount;
            size_t totalMetricCount = pl.TotalMetricCount;
            size_t globalMetricStartIndex = totalMetricCount - globalMetricCount;

            for (size_t j = 0; j < globalMetricCount; ++j)
            {
                // Application reserved load has only global metric load info
                int64 nodeReservedLoad = pl.ApplicationReservedLoads.HasKey(&node) ?
                    pl.ApplicationReservedLoads.operator[](&node).Values[j] :
                    0;
                size_t totalIndex = globalMetricStartIndex + j;
                int64 nodeLoad = node.Loads.Values[totalIndex] + nodeReservedLoad;
                wstring metricName = pl.LBDomains.back().Metrics[j].Name;
                ServiceModel::LoadMetricInformation & loadMetricInformation = metricsMap[metricName];

                double loadValue;
                if (loadMetricInformation.Name == ServiceModel::Constants::SystemMetricNameCpuCores)
                {
                    loadValue = static_cast<double>(nodeLoad) / ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor;
                    nodeLoad = static_cast<int64>(ceil(loadValue));
                }
                else
                {
                    loadValue = static_cast<double>(nodeLoad);
                }

                if (loadMetricInformation.MinimumNodeLoad < 0 || loadValue < loadMetricInformation.MinimumNodeLoad)
                {
                    loadMetricInformation.MinNodeLoadValue = nodeLoad;
                    loadMetricInformation.MinimumNodeLoad = loadValue;
                    loadMetricInformation.MinNodeLoadNodeId = node.NodeId;
                }

                if (loadMetricInformation.MaximumNodeLoad < 0 || loadValue > loadMetricInformation.MaximumNodeLoad)
                {
                    loadMetricInformation.MaximumNodeLoad = loadValue;
                    loadMetricInformation.MaxNodeLoadValue = nodeLoad;
                    loadMetricInformation.MaxNodeLoadNodeId = node.NodeId;
                }
            }
        }
    }

    for (auto i = metricsMap.begin(); i != metricsMap.end(); ++i)
    {
        loadMetricInformationVec.push_back(i->second);
    }

    queryResult = move(ServiceModel::ClusterLoadInformationQueryResult(startTimeUtc, endTimeUtc, move(loadMetricInformationVec)));
    return ErrorCode::Success();
}

Common::ErrorCode PlacementAndLoadBalancing::GetNodeLoadInformationQueryResult(Federation::NodeId nodeId, ServiceModel::NodeLoadInformationQueryResult & queryResult, bool onlyViolations /* = false */) const
{
    std::vector<ServiceModel::NodeLoadMetricInformation> nodeLoadMetricInformationList;
    std::shared_ptr<std::pair<Snapshot, Snapshot>> snapshots;

    //this section is under snapshotLock_
    {
        AcquireReadLock grab(snapshotLock_);
        if (!snapshotsOnEachRefresh_)
        {
            return ErrorCodeValue::PLBNotReady;
        }
        //get the snapshots taken at last refresh interval
        snapshots = snapshotsOnEachRefresh_;
    }

    for (auto iter = snapshots->second.ServiceDomainSnapshot.begin(); iter != snapshots->second.ServiceDomainSnapshot.end(); ++iter)
    {
        if (iter->second.state_.PlacementObj)
        {
            Placement const& pl = *(iter->second.state_.PlacementObj);
            for (size_t j = 0; j< pl.NodeCount; ++j)
            {
                NodeEntry const & node = pl.SelectNode(j);

                if (node.NodeId != nodeId)
                {
                    continue;
                }

                size_t globalMetricCount = pl.GlobalMetricCount;
                size_t totalMetricCount = pl.TotalMetricCount;
                size_t globalMetricStartIndex = totalMetricCount - globalMetricCount;

                for (size_t i = 0; i < globalMetricCount; ++i)
                {
                    int64 nodeReservedLoad = pl.ApplicationReservedLoads.HasKey(&node) ?
                        pl.ApplicationReservedLoads.operator[](&node).Values[i] :
                        0;
                    size_t totalIndex = globalMetricStartIndex + i;
                    int64 nodeLoad = node.Loads.Values[totalIndex] + nodeReservedLoad;
                    wstring metricName = pl.LBDomains.back().Metrics[i].Name;

                    int64 nodeTotalCapacity = node.TotalCapacities.Values[i];
                    int64 nodeReaminingCapacity = 0;
                    if (nodeTotalCapacity > 0)
                    {
                        if (nodeLoad < nodeTotalCapacity)
                        {
                            nodeReaminingCapacity = nodeTotalCapacity - nodeLoad;
                        }
                    }

                    int64 nodeBufferedCapacity = node.BufferedCapacities.Values[i];
                    int64 nodeReaminingBufferedCapacity = 0;
                    bool isCapacityViolation = false;
                    if (nodeBufferedCapacity > 0)
                    {
                        if (nodeLoad > nodeBufferedCapacity)
                        {
                            isCapacityViolation = true;
                        }
                        else
                        {
                            nodeReaminingBufferedCapacity = nodeBufferedCapacity - nodeLoad;
                        }
                    }

                    ServiceModel::NodeLoadMetricInformation nodeLoadMetricInformation;
                    nodeLoadMetricInformation.Name = move(metricName);
                    nodeLoadMetricInformation.IsCapacityViolation = isCapacityViolation;
                    if (nodeLoadMetricInformation.Name == ServiceModel::Constants::SystemMetricNameCpuCores)
                    {
                        double doubleNodeLoad = static_cast<double>(nodeLoad) / ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor;
                        double doubleNodeCapacity = static_cast<double>(nodeTotalCapacity) / ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor;
                        double doubleNodeBufferedCapacity = static_cast<double>(nodeBufferedCapacity) / ServiceModel::Constants::ResourceGovernanceCpuCorrectionFactor;

                        double doubleNodeRemainingCapacity = doubleNodeCapacity - doubleNodeLoad;
                        double doubleNodeRemainingBufferedCapacity = doubleNodeBufferedCapacity - doubleNodeLoad;

                        nodeLoadMetricInformation.NodeLoad = static_cast<int64>(ceil(doubleNodeLoad));
                        nodeLoadMetricInformation.NodeCapacity = static_cast<int64>(ceil(doubleNodeCapacity));
                        nodeLoadMetricInformation.NodeRemainingCapacity = static_cast<int64>(floor(doubleNodeRemainingCapacity));
                        nodeLoadMetricInformation.NodeBufferedCapacity = static_cast<int64>(ceil(doubleNodeBufferedCapacity));
                        nodeLoadMetricInformation.NodeRemainingBufferedCapacity = static_cast<int64>(floor(doubleNodeRemainingBufferedCapacity));
                        // Double values in the query
                        nodeLoadMetricInformation.CurrentNodeLoad = doubleNodeLoad;
                        nodeLoadMetricInformation.NodeCapacityRemaining = doubleNodeRemainingCapacity;
                        nodeLoadMetricInformation.BufferedNodeCapacityRemaining = doubleNodeRemainingBufferedCapacity;
                    }
                    else
                    {
                        nodeLoadMetricInformation.NodeLoad = nodeLoad;
                        nodeLoadMetricInformation.NodeCapacity = nodeTotalCapacity;
                        nodeLoadMetricInformation.NodeRemainingCapacity = nodeReaminingCapacity;
                        nodeLoadMetricInformation.NodeBufferedCapacity = nodeBufferedCapacity;
                        nodeLoadMetricInformation.NodeRemainingBufferedCapacity = nodeReaminingBufferedCapacity;
                        // Double values in the query
                        nodeLoadMetricInformation.CurrentNodeLoad = static_cast<double>(nodeLoad);
                        nodeLoadMetricInformation.NodeCapacityRemaining = static_cast<double>(nodeReaminingCapacity);
                        nodeLoadMetricInformation.BufferedNodeCapacityRemaining = static_cast<double>(nodeReaminingBufferedCapacity);
                    }

                    if (!onlyViolations || (onlyViolations && isCapacityViolation))
                    {
                        nodeLoadMetricInformationList.push_back(nodeLoadMetricInformation);
                    }
                }
            }
        }
    }

    queryResult = ServiceModel::NodeLoadInformationQueryResult();
    queryResult.put_NodeLoadMetricInformation(move(nodeLoadMetricInformationList));
    return ErrorCode::Success();
}

ServiceModel::UnplacedReplicaInformationQueryResult PlacementAndLoadBalancing::GetUnplacedReplicaInformationQueryResult(std::wstring const& serviceName, Common::Guid const& partitionId, bool onlyQueryPrimaries /* = false */)
{
    return ServiceModel::UnplacedReplicaInformationQueryResult(serviceName, partitionId, move(plbDiagnosticsSPtr_->GetUnplacedReplicaInformation(serviceName, partitionId, onlyQueryPrimaries)));
}

Common::ErrorCode PlacementAndLoadBalancing::GetApplicationLoadInformationResult(std::wstring const & applicationName, ServiceModel::ApplicationLoadInformationQueryResult & result)
{
    result.ApplicationName = applicationName;
    result.NodeCount = 0;
    std::map<std::wstring, ServiceModel::ApplicationLoadMetricInformation> applicationLoads;
    {
        AcquireReadLock grab_(lock_);

        auto appIdIter = applicationToIdMap_.find(applicationName);
        uint64 applicationId = (appIdIter == applicationToIdMap_.end()) ? 0 : appIdIter->second;

        Application const * app = GetApplicationPtrCallerHoldsLock(applicationId);
        if (nullptr == app)
        {
            // This application does not exist, hence there are no capacities.
            // Just leave the default values and exit.
            result.MinimumNodes = 0;
            result.MaximumNodes = 0;
            result.ApplicationLoadMetricInformation = move(std::vector<ServiceModel::ApplicationLoadMetricInformation>());
            return ErrorCodeValue::Success;
        }

        // Basic scaleout information
        result.MinimumNodes = app->ApplicationDesc.MinimumNodes;
        result.MaximumNodes = app->ApplicationDesc.ScaleoutCount;

        auto serviceDomainIterator = applicationToDomainTable_.find(app->ApplicationDesc.ApplicationId);
        bool hasLoad = serviceDomainIterator != applicationToDomainTable_.end()
            && serviceDomainIterator->second->second.ApplicationHasLoad(applicationId);
        ApplicationLoad const* applicationLoad = nullptr;

        if (hasLoad)
        {
            ServiceDomain const& serviceDomain = serviceDomainIterator->second->second;
            applicationLoad = &serviceDomain.GetApplicationLoad(applicationId);
            result.NodeCount = static_cast<uint> (applicationLoad->NodeCounts.size());
        }

        // Create empty structures to fill from placement info
        for (auto metric : app->ApplicationDesc.AppCapacities)
        {
            ServiceModel::ApplicationLoadMetricInformation metricLoad;
            wstring metricName = metric.first;
            metricLoad.Name = move(metricName);
            metricLoad.ApplicationCapacity = metric.second.TotalCapacity;
            metricLoad.ReservationCapacity = app->ApplicationDesc.MinimumNodes * metric.second.ReservationCapacity;
            // No load for now
            metricLoad.ApplicationLoad = 0;
            if (hasLoad)
            {
                auto const& metricLoadIt = applicationLoad->MetricLoads.find(metric.first);
                if (metricLoadIt != applicationLoad->MetricLoads.end())
                {
                    metricLoad.ApplicationLoad = metricLoadIt->second.NodeLoadSum;
                }
            }
            applicationLoads.insert(make_pair(metric.first, metricLoad));
        }
    }

    result.ApplicationLoadMetricInformation = std::vector<ServiceModel::ApplicationLoadMetricInformation>();
    for (auto metricResult : applicationLoads)
    {
        result.ApplicationLoadMetricInformation.push_back(move(metricResult.second));
    }

    return ErrorCodeValue::Success;
}

/// <summary>
/// Gets the service identifier.
/// </summary>
/// <param name="serviceName">Name of the service.</param>
/// <returns>the service identifier</returns>
uint64 PlacementAndLoadBalancing::GetServiceId(wstring const& serviceName) const
{
    auto const& itId = serviceToIdMap_.find(serviceName);
    if (itId != serviceToIdMap_.end())
    {
        return itId->second;
    }
    else return 0;
}

void PlacementAndLoadBalancing::AddMetricToDomain(wstring const& metricName, ServiceDomainTable::iterator const& itDomain, bool incrementAppCount)
{
    metricToDomainTable_.insert(make_pair(metricName, itDomain));
    itDomain->second.AddMetric(metricName, incrementAppCount);
}

/// <summary>
/// Queues the domain change.
/// </summary>
/// <param name="domainId">The domain identifier.</param>
/// <param name="merge">if set to <c>true</c> [merge]. if set to <c>true</c> [split].</param>
void PlacementAndLoadBalancing::QueueDomainChange(ServiceDomain::DomainId const& domainId, bool merge)
{
    if (merge)
    {
        domainsToMerge_.insert(domainId);
    }
    else
    {
        domainToSplit_ = domainId;
    }
}

/// <summary>
/// This function deals with all the queued domain changes. We dont execute domain splits and merges right away, because most structures we use are passed by const&, and may become invalid once we execute domain changes.
/// </summary>
/// <param name="mergeOnly">if set to <c>true</c> [merge only].</param>
void PlacementAndLoadBalancing::ExecuteDomainChange(bool mergeOnly)
{
    if (domainsToMerge_.empty() && domainToSplit_ == L"")
    {
        return;
    }
    //first we do a merge.
    if (!domainsToMerge_.empty())
    {
        auto it = MergeDomains(domainsToMerge_);
        //if a domain split is queued on one of these, we'll try a split on the merged domain later...
        if (domainsToMerge_.find(domainToSplit_) != domainsToMerge_.end())
        {
            domainToSplit_ = it->second.Id;
        }
        domainsToMerge_.clear();
    }
    //then we do a split.
    if (domainToSplit_ != L"" && !mergeOnly)
    {
        auto domainIter = serviceDomainTable_.find(ServiceDomain::GetDomainIdPrefix(domainToSplit_));
        if (domainIter != serviceDomainTable_.end())
        {
            SplitDomain(domainIter);
        }
        domainToSplit_ = L"";
    }
}

void PlacementAndLoadBalancing::UpdateUseSeparateSecondaryLoadConfig(bool newUseSeparateSecondaryLoad)
{
    //if there was a change we remove all FTs and readd them with new config
    for (auto itDomains = serviceDomainTable_.begin(); itDomains != serviceDomainTable_.end(); ++itDomains)
    {
        auto& serviceDomain = itDomains->second;
        for (auto itFailoverUnits = serviceDomain.FailoverUnits.begin(); itFailoverUnits != serviceDomain.FailoverUnits.end(); ++itFailoverUnits)
        {
            auto& failoverUnit = itFailoverUnits->second;
            auto& service = serviceDomain.GetService(failoverUnit.FuDescription.ServiceId);
            serviceDomain.DeleteNodeLoad(service, failoverUnit, failoverUnit.FuDescription.Replicas);
            service.DeleteNodeLoad(failoverUnit, failoverUnit.FuDescription.Replicas, settings_);
            serviceDomain.UpdateApplicationNodeCounts(service.ServiceDesc.ApplicationId, failoverUnit.FuDescription.Replicas, false);
            serviceDomain.UpdateServicePackagePlacement(service, failoverUnit.FuDescription.Replicas, false);
            //we should reset the move map and secondary entries map
            (const_cast<FailoverUnit&>(failoverUnit)).ResetMovementMaps();
        }
    }
    settings_.UseSeparateSecondaryLoad = newUseSeparateSecondaryLoad;
    for (auto itDomains = serviceDomainTable_.begin(); itDomains != serviceDomainTable_.end(); ++itDomains)
    {
        auto& serviceDomain = itDomains->second;
        for (auto itFailoverUnits = serviceDomain.FailoverUnits.begin(); itFailoverUnits != serviceDomain.FailoverUnits.end(); ++itFailoverUnits)
        {
            auto& failoverUnit = itFailoverUnits->second;
            auto& service = serviceDomain.GetService(failoverUnit.FuDescription.ServiceId);
            serviceDomain.AddNodeLoad(service, failoverUnit, failoverUnit.FuDescription.Replicas);
            service.AddNodeLoad(failoverUnit, failoverUnit.FuDescription.Replicas, settings_);
            serviceDomain.UpdateApplicationNodeCounts(service.ServiceDesc.ApplicationId, failoverUnit.FuDescription.Replicas, true);
            serviceDomain.UpdateServicePackagePlacement(service, failoverUnit.FuDescription.Replicas, true);
        }
    }
}

void PlacementAndLoadBalancing::ProcessAutoScalingRepartitioning()
{
    // This method is executed during refresh outside of the lock.
    // pendingAutoScalingRepartitions_ is filled during BeginRefresh.
    if (pendingAutoScalingRepartitions_.empty())
    {
        return;
    }
    if (serviceManagementClient_.get() == nullptr)
    {
        return;
    }

    auto timeout = PLBConfig::GetConfig().ServiceRepartitionForAutoScalingTimeout;

    for (auto repartitionInfo : pendingAutoScalingRepartitions_)
    {
        // Calculate the names to add or to remove.
        vector<wstring> namesToAdd;
        vector<wstring> namesToRemove;
        auto startIndex = repartitionInfo.CurrentPartitionCount;
        if (repartitionInfo.Change > 0)
        {
            for (auto index = 0; index < repartitionInfo.Change; ++index)
            {
                namesToAdd.push_back(wformatString("{0}", startIndex + index));
            }
        }
        else
        {
            auto toRemoveCount = -repartitionInfo.Change;
            for (auto index = 0; index < toRemoveCount; ++index)
            {
                namesToRemove.push_back(wformatString("{0}", startIndex - index - 1));
            }
        }

        Common::NamingUri serviceNameUri;

        bool success = NamingUri::TryParse(repartitionInfo.ServiceName, serviceNameUri);
        if (!success)
        {
            continue;
        }

        Trace.AutoScaler(wformatString(
            "Repartitioning service {0} (id={1}) namesToAdd={2} namesToRemove={3}",
            serviceNameUri,
            repartitionInfo.ServiceId,
            namesToAdd,
            namesToRemove));

        Naming::ServiceUpdateDescription updateDescription(repartitionInfo.IsStateful, move(namesToAdd), move(namesToRemove));

        // Perform the actual call.
        serviceManagementClient_->BeginUpdateService(
            serviceNameUri,
            updateDescription,
            timeout,
            [this, repartitionInfo](AsyncOperationSPtr const& operation)
            {
                this->OnAutoScalingRepartitioningComplete(operation, repartitionInfo.ServiceId);
            },
            AsyncOperationRoot<ComponentRootSPtr>::Create(this->Root.CreateComponentRoot()));
    }

    pendingAutoScalingRepartitions_.clear();
}

void PlacementAndLoadBalancing::OnAutoScalingRepartitioningComplete(Common::AsyncOperationSPtr const & operation, uint64_t serviceId)
{
    auto error = serviceManagementClient_->EndUpdateService(operation);
    if (!error.IsSuccess())
    {
        // Success is handled with UpdateService call from FM.
        Trace.AutoScalingRepartitionFailed(serviceId, error);
        AcquireExclusiveLock grab(autoScalingFailedOperationsVectorsLock_);
        pendingAutoScalingFailedRepartitions_.push_back(make_pair(serviceId, error));
    }
}

void PlacementAndLoadBalancing::ProcessFailedAutoScalingRepartitionsCallerHoldsLock()
{
    for (auto failedScaling : pendingAutoScalingFailedRepartitionsForRefresh_)
    {
        auto sdIterator = serviceToDomainTable_.find(failedScaling.first);
        if (sdIterator != serviceToDomainTable_.end())
        {
            sdIterator->second->second.AutoScalerComponent.UpdateServiceAutoScalingOperationFailed(failedScaling.first);
        }
        else
        {
            TRACE_WARNING_AND_TESTASSERT(
                "AutoScaling",
                "Service with id {0} does not belong to any domain when processing failed repartitions.",
                failedScaling.first);
        }
    }
    pendingAutoScalingFailedRepartitionsForRefresh_.clear();
}

void PlacementAndLoadBalancing::OnSafetyCheckAcknowledged(ServiceModel::ApplicationIdentifier const & appId)
{
    AcquireWriteLock grab(lock_);

    for (auto & app : applicationTable_)
    {
        if (app.second.ApplicationDesc.ApplicationIdentifier == appId)
        {
            applicationsInUpgradeCheckRg_.erase(app.second.ApplicationDesc.ApplicationId);
            break;
        }
    }
}

void PlacementAndLoadBalancing::EmitCRMOperationTrace(
    Common::Guid const & failoverUnitId,
    Common::Guid const & decisionId,
    PLBSchedulerActionType::Enum schedulerActionType,
    FailoverUnitMovementType::Enum failoverUnitMovementType,
    std::wstring const & serviceName,
    Federation::NodeId const & sourceNode,
    Federation::NodeId const & destinationNode) const
{
    PublicTrace.Operation(
        failoverUnitId,
        decisionId,
        schedulerActionType,
        failoverUnitMovementType,
        serviceName,
        sourceNode,
        destinationNode);

    // PublicTrace.Operation and PublicTrace.OperationQuery are duplicates.
    // RDBug 11870228 tracks the task of writing a test to keep them in sync
    // until we deprecate PublicTrace.Operation
    PublicTrace.OperationQuery(
        failoverUnitId,
        decisionId,
        schedulerActionType,
        failoverUnitMovementType,
        serviceName,
        sourceNode.ToString(),
        destinationNode.ToString());
}

void PlacementAndLoadBalancing::EmitCRMOperationIgnoredTrace(
    Common::Guid const & failoverUnitId,
    Reliability::PlbMovementIgnoredReasons::Enum movementIgnoredReason,
    Common::Guid const & decisionId) const
{
    PublicTrace.OperationIgnored(failoverUnitId, movementIgnoredReason, decisionId);

    // PublicTrace.OperationIgnored and PublicTrace.OperationIgnoredQuery are duplicates.
    // RDBug 11870228 tracks the task of writing a test to keep them in sync
    // until we deprecate PublicTrace.OperationIgnored
    PublicTrace.OperationIgnoredQuery(failoverUnitId, movementIgnoredReason, decisionId);
}

void PlacementAndLoadBalancing::UpdateAvailableImagesPerNode(std::wstring const& nodeId, std::vector<std::wstring> const& images)
{
    LockAndSetBooleanLock grab(bufferUpdateLock_);

    LargeInteger nodeLargeInteger;
    if (!LargeInteger::TryParse(nodeId, nodeLargeInteger))
    {
        Trace.InternalError(wformatString("Error while parsing nodeId"));
        return;
    }

    Trace.AvailableImagesFromNode(wformatString("UpdateAvailableImagesPerNode on node {0} - images {1}", nodeId, images));

    Federation::NodeId fNodeId(nodeLargeInteger);

    auto itNode = pendingNodeImagesUpdate_.find(fNodeId);
    if (itNode != pendingNodeImagesUpdate_.end())
    {
        // For this node we already had images, so we need to overwrite stale state
        pendingNodeImagesUpdate_.erase(fNodeId);
    }

    pendingNodeImagesUpdate_.insert(make_pair(fNodeId, images));
}
