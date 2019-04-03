// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <algorithm>    // std::sort
#include <iostream>
#include "FM.h"
#include "TestSession.h"
#include "DataParser.h"
#include "LBSimulatorConfig.h"
#include "Utility.h"
#include "Reliability/LoadBalancing/PlacementAndLoadBalancing.h"
#include "Client/ClientPointers.h"
#include "Reliability/Failover/common/FailoverConfig.h"
#include "Reliability/LoadBalancing/DiagnosticsDataStructures.h"


using namespace std;
using namespace Common;
using namespace LBSimulator;
using namespace Reliability;

FMSPtr FM::Create(LBSimulatorConfig const& config)
{
    FMSPtr ret(new FM(config));
    ret->CreatePLB();

    return move(ret);
}

void FM::GetNodeProperties(wstring nodeType, map<wstring, wstring> & nodePropertyVector)
{
    for each (auto nodeProperty in nodeToNodePropertyMap_[nodeType])
    {
        nodePropertyVector.insert(nodeProperty);
    }
}

bool FM::AddNodeProperty(wstring nodeType, map<wstring, wstring> nodeProperties)
{
    if (nodeToNodePropertyMap_.count(nodeType) != 0)
    {
        for each (auto nodeProperty in nodeProperties)
        {
            nodeToNodePropertyMap_[nodeType].insert(nodeProperty);
        }
        return true;
    }
    nodeToNodePropertyMap_.insert(make_pair(nodeType, nodeProperties));
    return false;
}

/// <summary>
/// Insert the node into FM's nodes list and Update PLB regarding the same.
/// </summary>
/// <param name="node">The node object (passed by reference).</param>
void FM::CreateNode(LBSimulator::Node && node)
{
    AcquireWriteLock grab(lock_);

    int nodeIndex = node.Index;

    auto it = nodes_.find(nodeIndex);
    ASSERT_IFNOT(it == nodes_.end() || !it->second.IsUp, "Node {0} is already up", nodeIndex);

    int oldInstance = -1;
    if (it != nodes_.end())
    {
        oldInstance = it->second.Instance;
        nodes_.erase(it);
    }

    it = nodes_.insert(make_pair(nodeIndex, move(node))).first;
    it->second.Instance = oldInstance + 1;

    plb_->UpdateNode(it->second.GetPLBNodeDescription());

    GetPLBObject()->ProcessPendingUpdatesPeriodicTask();

    for (auto itService = services_.begin(); itService != services_.end(); itService++)
    {
        plb_->UpdateServiceType(itService->second.GetPLBServiceTypeDescription(nodes_));
    }
}

/// <summary>
/// Bring a node Down.
/// </summary>
/// <param name="nodeIndex">Index of the node to be brought down.</param>
void FM::DeleteNode(int nodeIndex)
{
    AcquireWriteLock grab(lock_);

    Node & node = GetNode(nodeIndex);
    ASSERT_IFNOT(node.IsUp, "Node {0} is already down", nodeIndex);

    node.IsUp = false;

    plb_->UpdateNode(node.GetPLBNodeDescription());

    for (auto itService = services_.begin(); itService != services_.end(); itService++)
    {
        plb_->UpdateServiceType(itService->second.GetPLBServiceTypeDescription(nodes_));
    }

    ProcessFailoverUnits();
}

/// <summary>
/// Creates a service in FM.
/// </summary>
/// <param name="service">The service object (passed by reference).</param>
/// <param name="createFailoverUnit">if set to <c>true</c> [Create Empty FailoverUnit for the service as well].  (Default: true)</param>
void FM::CreateService(LBSimulator::Service && service, bool createFailoverUnit)
{
    AcquireWriteLock grab(lock_);

    int serviceIndex = service.Index;

    auto it = services_.find(serviceIndex);
    ASSERT_IFNOT(it == services_.end(), "Service {0} already exists", serviceIndex);

    it = services_.insert(make_pair(serviceIndex, move(service))).first;

    LBSimulator::Service & insertedService = it->second;
    insertedService.SetStartFailoverUnitIndex(nextFailoverUnitIndex_);
    nextFailoverUnitIndex_ += insertedService.PartitionCount;

    plb_->UpdateServiceType(insertedService.GetPLBServiceTypeDescription(nodes_));
    if (!plb_->UpdateService(insertedService.GetPLBServiceDescription()).IsSuccess())
    {
        services_.erase(it);
        TestSession::WriteInfo(Utility::TraceSource, "Unable to create service with index {0}", insertedService.get_Index());
        return;
    }

    if (createFailoverUnit)
    {
        int partitionCount = insertedService.PartitionCount;
        for (int i = 0; i < partitionCount; i++)
        {
            int currentFUIndex = insertedService.StartFailoverUnitIndex + i;
            map<wstring, uint> primaryLoad;
            map<wstring, uint> secondaryLoad;
            for (auto itMetric = insertedService.Metrics.begin(); itMetric < insertedService.Metrics.end(); itMetric++)
            {
                primaryLoad.insert(make_pair(itMetric->Name, itMetric->PrimaryDefaultLoad));
                secondaryLoad.insert(make_pair(itMetric->Name, itMetric->SecondaryDefaultLoad));
            }

            Common::Guid failoverUnitId = Common::Guid(currentFUIndex, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
            auto itFailoverUnit = failoverUnits_.insert(make_pair(failoverUnitId, FailoverUnit(
                failoverUnitId,
                insertedService.Index,
                insertedService.ServiceName,
                insertedService.TargetReplicaSetSize,
                insertedService.IsStateful,
                map<wstring, uint>(primaryLoad),
                map<wstring, uint>(secondaryLoad)))).first;

            plb_->UpdateFailoverUnit(itFailoverUnit->second.GetPLBFailoverUnitDescription());
        }
    }
}

/// <summary>
/// Adds the Specified FailoverUnit to the FM and updates PLB regarding the same.
/// </summary>
/// <param name="failoverUnit">The FailoverUnit object (passed by reference).</param>
void FM::AddFailoverUnit(FailoverUnit && failoverUnit)
{
    AcquireWriteLock grab(lock_);
    Common::Guid failoverUnitId = failoverUnit.FailoverUnitId;
    auto itFailoverUnit = failoverUnits_.find(failoverUnitId);
    ASSERT_IFNOT(itFailoverUnit == failoverUnits_.end(), "FailoverUnit {0} already exist", failoverUnitId);

    itFailoverUnit = failoverUnits_.insert(make_pair(failoverUnitId, move(failoverUnit))).first;
    plb_->UpdateFailoverUnit(itFailoverUnit->second.GetPLBFailoverUnitDescription());
    plb_->UpdateLoadOrMoveCost(itFailoverUnit->second.GetPLBLoadOrMoveCostDescription());
}

void FM::AddReplica(Common::Guid failoverUnitGuid, FailoverUnit::Replica && replica)
{
    AcquireWriteLock grab(lock_);

    FailoverUnit & failoverUnit = GetFailoverUnit(failoverUnitGuid);
    Node & node = GetNode(replica.NodeIndex); //make sure the node exists
    node;
    failoverUnit.AddReplica(move(replica));
    plb_->UpdateFailoverUnit(failoverUnit.GetPLBFailoverUnitDescription());
}

void FM::DeleteService(int serviceIndex)
{
    AcquireWriteLock grab(lock_);

    services_.erase(serviceIndex);

    ProcessFailoverUnits();

    plb_->DeleteService(StringUtility::ToWString(serviceIndex));
    plb_->DeleteServiceType(StringUtility::ToWString(serviceIndex) + L"_type");
}

void FM::ReportLoad(Common::Guid failoverUnitGuid, int role, wstring const& metric, uint load)
{
    AcquireWriteLock grab(lock_);

    ASSERT_IF(Service::IsBuiltInMetric(metric), "Built-in metric {0} doesn't need to be reported", metric);

    FailoverUnit & failoverUnit = GetFailoverUnit(failoverUnitGuid);

    failoverUnit.UpdateLoad(role, metric, load);

    plb_->UpdateLoadOrMoveCost(failoverUnit.GetPLBLoadOrMoveCostDescription());
}

void FM::ReportBatchLoad(int serviceIndex, std::wstring const& metric, LoadDistribution const& loadDistribution)
{
    AcquireWriteLock grab(lock_);

    ASSERT_IF(Service::IsBuiltInMetric(metric), "Built-in metric {0} doesn't need to be reported", metric);

    Service & service = GetService(serviceIndex);
    vector<Guid> failoverUnits;
    GetFailoverUnits(service.ServiceName, failoverUnits);
    for (Guid const & guid : failoverUnits)
    {
        FailoverUnit & failoverUnit = GetFailoverUnit(guid);
        for (Service::Metric const& serviceMetric : service.Metrics)
        {
            if (service.IsStateful)
            {
                failoverUnit.UpdateLoad(0, serviceMetric.Name, loadDistribution.GetRandom(random_));
            }
            failoverUnit.UpdateLoad(1, serviceMetric.Name, loadDistribution.GetRandom(random_));
        }

        plb_->UpdateLoadOrMoveCost(failoverUnit.GetPLBLoadOrMoveCostDescription());
    }
}

void FM::ReportBatchMoveCost(uint moveCost)
{
    AcquireWriteLock grab(lock_);

    for (auto itService = services_.begin(); itService != services_.end(); itService++)
    {
        Service & service = itService->second;
        vector<Common::Guid> failoverUnits;
        GetFailoverUnits(service.ServiceName, failoverUnits);
        for each(auto guid in failoverUnits)
        {
            FailoverUnit & failoverUnit = GetFailoverUnit(guid);
            if (service.IsStateful)
            {
                failoverUnit.UpdateLoad(0, L"_MoveCost_", moveCost);
            }
            failoverUnit.UpdateLoad(1, L"_MoveCost_", moveCost);

            plb_->UpdateLoadOrMoveCost(failoverUnit.GetPLBLoadOrMoveCostDescription());
        }
    }
}

void FM::DeleteReplica(Common::Guid failoverUnitGuid, int nodeIndex)
{
    AcquireWriteLock grab(lock_);

    FailoverUnit & failoverUnit = GetFailoverUnit(failoverUnitGuid);
    //make sure the node exists
    GetNode(nodeIndex);

    failoverUnit.ReplicaDown(nodeIndex);
    plb_->UpdateFailoverUnit(failoverUnit.GetPLBFailoverUnitDescription());
}

void FM::PromoteReplica(Common::Guid failoverUnitGuid, int nodeIndex)
{
    AcquireWriteLock grab(lock_);

    FailoverUnit & failoverUnit = GetFailoverUnit(failoverUnitGuid);
    //make sure the node exists
    GetNode(nodeIndex);

    failoverUnit.PromoteReplica(nodeIndex);
    plb_->UpdateFailoverUnit(failoverUnit.GetPLBFailoverUnitDescription());
}

void FM::MoveReplica(Common::Guid failoverUnitGuid, int sourceNodeIndex, int targetNodeIndex)
{
    AcquireWriteLock grab(lock_);

    FailoverUnit & failoverUnit = GetFailoverUnit(failoverUnitGuid);
    //make sure the node exists
    GetNode(sourceNodeIndex);
    GetNode(targetNodeIndex);

    failoverUnit.MoveReplica(sourceNodeIndex, targetNodeIndex, nodes_);
    plb_->UpdateFailoverUnit(failoverUnit.GetPLBFailoverUnitDescription());
}

//TODO:: deprecate this...
void FM::ExecutePlacementAndLoadBalancing()
{
    currentTicks_++;

    GetPLBObject()->Refresh(StopwatchTime(currentTicks_));

    Reliability::LoadBalancingComponent::PLBConfig &config = Reliability::LoadBalancingComponent::PLBConfig::GetConfig();
    currentTicks_ += min(min(config.MinPlacementInterval.Ticks, config.MinConstraintCheckInterval.Ticks),
        config.MinLoadBalancingInterval.Ticks);
}

/// <summary>
/// Prints the state of all nodes in the cluster, classified by Upgrade Domain.
/// </summary>
void FM::PrintNodeState(bool printToConsole)
{
    wstringstream output;
    set<wstring> metrics = GetUniqueMetrics();
    if (metrics.size() == 0) { return; }
    for (auto metric : metrics)
    {
        if (!IsLogged(metric)) { continue; }

        output << metric << ":" << "\n\r";
        map<wstring, vector< pair<int64,wstring>>> nodeLoadsPerUd = map<wstring, vector< pair<int64,wstring>>>();
        for (auto const& node : nodes_)
        {
            ServiceModel::NodeLoadInformationQueryResult nodeLoadInfo;
            GetPLBObject()->GetNodeLoadInformationQueryResult(Node::CreateNodeId(node.second.Index), nodeLoadInfo);
            int64 nodeLoad = 0;
            for (auto const& nodeLoadMetricInfo : nodeLoadInfo.NodeLoadMetricInformation)
            {
                if (nodeLoadMetricInfo.Name.compare(metric) == 0)
                {
                    nodeLoad = nodeLoadMetricInfo.NodeLoad;
                    break;
                }
            }
            auto foundNodeUD = nodeLoadsPerUd.find(node.second.UpgradeDomainId);
            if (foundNodeUD == nodeLoadsPerUd.end())
            {
                vector<pair<int64, wstring>> vectorLoad = vector<pair<int64, wstring>>();
                vectorLoad.push_back(make_pair(nodeLoad,node.second.NodeName));
                nodeLoadsPerUd.insert(make_pair(node.second.UpgradeDomainId, vectorLoad));
            }
            else
            {
                foundNodeUD->second.push_back(make_pair(nodeLoad,node.second.NodeName) );
            }
        }
        for (auto it = nodeLoadsPerUd.begin(); it != nodeLoadsPerUd.end(); it++)
        {
            std::sort(it->second.begin(), it->second.end(), std::greater<std::pair<int64,std::wstring>>());
        }
        for (auto const& ud : nodeLoadsPerUd)
        {
            wstring nodeLoads;
            wstring nodeNames;
            int64 udTotalLoad = 0;
            for (auto nodeEntry : ud.second)
            {
                nodeLoads += wformatString(nodeEntry.first) + L"(" + nodeEntry.second + L")" + L"\t";
                udTotalLoad += nodeEntry.first;
            }
            output << ud.first << " \tTotal:\t" << udTotalLoad <<  " \tNodes:\t" << nodeLoads << "\n\r";
        }
    }
    if (printToConsole)
    {
        TestSession::WriteInfo(Utility::TraceSource, "{0}", output.str());
    }
    else
    {
        TestSession::WriteNoise(Utility::TraceSource, "{0}", output.str());
    }
}
/// <summary>
/// Forces the PLB to update.
/// </summary>
void FM::ForcePLBUpdate()
{
    GetPLBObject()->ProcessPendingUpdatesPeriodicTask();
}

/// <summary>
/// Forces the PLB state change.
/// </summary>
void FM::ForcePLBStateChange()
{
    //nodes_.begin()->second
    //GetPLBObject()->UpdateNode(nodes_.begin()->second.GetPLBNodeDescription());
    //failoverUnits_.begin()->second.ChangeVersion(1);
    //GetPLBObject()->UpdateFailoverUnit(failoverUnits_.begin()->second.GetPLBFailoverUnitDescription());
    std::map<Common::Guid, FailoverUnit>::iterator it = failoverUnits_.begin();
    for (;it != failoverUnits_.end();it++)
    {
        it->second.ChangeVersion(1);
        GetPLBObject()->UpdateFailoverUnit(it->second.GetPLBFailoverUnitDescription());
    }
}

/// <summary>
/// Forces the execution PLB. By Default, disallows Creation and Constraint Check phases.
/// </summary>
/// <param name="noOfRuns">Number of PLB runs to execute.</param>
/// <param name="needPlacement">if set to <c>true</c> [Allow Placement phases in PLB run]. Default: False</param>
/// <param name="needPlacement">if set to <c>true</c> [Allow ConstraintCheck phase in PLB run]. Default: False</param>
/// <returns></returns>
int FM::ForceExecutePLB(int noOfRuns, bool needPlacement, bool needConstraintCheck)
{
    Reliability::LoadBalancingComponent::PlacementAndLoadBalancing *plb = GetPLBObject();
    Reliability::LoadBalancingComponent::PLBConfig &config = Reliability::LoadBalancingComponent::PLBConfig::GetConfig();
    int oldChainViolations = 0;

    TimeSpan oldPlacementInterval = config.MinPlacementInterval;
    config.MinPlacementInterval = needPlacement ? Common::TimeSpan::FromSeconds(1) : Common::TimeSpan::FromSeconds(10000000);

    TimeSpan oldMinConstraintCheckInterval = config.MinConstraintCheckInterval;
    config.MinConstraintCheckInterval = needConstraintCheck ? Common::TimeSpan::FromSeconds(1) : Common::TimeSpan::FromSeconds(10000000);

    TimeSpan oldLoadBalancingInterval = config.MinLoadBalancingInterval;
    config.MinLoadBalancingInterval = Common::TimeSpan::FromSeconds(0);

    Common::StopwatchTime time = Stopwatch::Now();
    for (int i = 0; i < noOfRuns; i++)
    {
        PLBRun results;
        Reliability::LoadBalancingComponent::LoadBalancingCountersSPtr oldLBCounters = plb->LoadBalancingCounters;
        INT64 creations = oldLBCounters->NumberOfPLBSchedulerCreationActions.Value;
        INT64 creationsWithMove = oldLBCounters->NumberOfPLBSchedulerCreationWithMoveActions.Value;
        INT64 movementsFast = oldLBCounters->NumberOfPLBSchedulerFastBalancingActions.Value;
        INT64 movementsSlow = oldLBCounters->NumberOfPLBSchedulerSlowBalancingActions.Value;
        INT64 constraintCheck = oldLBCounters->NumberOfPLBSchedulerConstraintCheckActions.Value;
        oldChainViolations = totalChainViolations_;

        //removing expired blockList entries....
        if (NoOfPLBRuns > nodeBlockDuration_ && nodeBlockDuration_ > 0)
        {
            LBSimulator::PLBRun expiredRun = GetPLBRunDetails(static_cast<int>(NoOfPLBRuns)-(nodeBlockDuration_ + 1));
            for (auto blockListEntry : blockList_)
            {
                if (expiredRun.blockListAfter.count(blockListEntry))
                {
                    RemoveFromBlockList(blockListEntry.first, blockListEntry.second);
                }
            }
            for (auto expiredBlockListEntry : expiredRun.blockListAfter)
            {
                blockList_.erase(expiredBlockListEntry);
            }
        }
        results.blockListBefore = blockList_;

        //using artificial time instead of Now() is forced by behavior of PLB with regards to Slow balancing, as well as the fact that
        //all movements happen instantly in the simulator. IF we use Now(), there are cases in which Slow Balancing never happens...
        TestSession::WriteNoise(Utility::TraceSource, "[DEBUG]PLB Size: {0}", sizeof(*plb));

        plb->Refresh(time);
        ForcePLBStateChange();

        set<wstring> metrics = GetUniqueMetrics();
        for (auto metric : metrics)
        {
            double average = 0;
            double deviation = 0;
            if (GetMetricAggregates(metric, average, deviation) != 0)
            {
                results.metrics.push_back(metric);
                results.averages.push_back(average);
                results.stDeviations.push_back(deviation);
            }
        }
        Reliability::LoadBalancingComponent::LoadBalancingCountersSPtr newLBCounters = plb->LoadBalancingCounters;
        results.creations = newLBCounters->NumberOfPLBSchedulerCreationActions.Value - creations;
        results.creationsWithMove = newLBCounters->NumberOfPLBSchedulerCreationWithMoveActions.Value - creationsWithMove;
        if (newLBCounters->NumberOfPLBSchedulerFastBalancingActions.Value != movementsFast)
        {
            results.movements = newLBCounters->NumberOfPLBSchedulerFastBalancingActions.Value - movementsFast;
        }
        else
        {
            results.movements = newLBCounters->NumberOfPLBSchedulerSlowBalancingActions.Value - movementsSlow;
        }
        results.action = L"";
        if (newLBCounters->NumberOfPLBSchedulerFastBalancingActions.Value != movementsFast)
        {
            results.action += L" FastBalancing/" + StringUtility::ToWString(
                newLBCounters->NumberOfPLBSchedulerFastBalancingActions.Value - movementsFast) + L" ";
        }
        if (newLBCounters->NumberOfPLBSchedulerSlowBalancingActions.Value != movementsSlow)
        {
            results.action += L"SlowBalancing/" + StringUtility::ToWString(
                newLBCounters->NumberOfPLBSchedulerSlowBalancingActions.Value - movementsSlow) + L" ";
        }
        if (newLBCounters->NumberOfPLBSchedulerCreationActions.Value != creations)
        {
            results.action += L"Creation/" + StringUtility::ToWString(
                newLBCounters->NumberOfPLBSchedulerCreationActions.Value - creations) + L" ";
        }
        if (newLBCounters->NumberOfPLBSchedulerCreationWithMoveActions.Value != creationsWithMove)
        {
            results.action += L"CreationWithMove/" + StringUtility::ToWString(
                newLBCounters->NumberOfPLBSchedulerCreationWithMoveActions.Value - creationsWithMove) + L" ";
        }
        if (newLBCounters->NumberOfPLBSchedulerConstraintCheckActions.Value != constraintCheck)
        {
            results.action += L"ConstraintCheck/" + StringUtility::ToWString(newLBCounters->NumberOfPLBSchedulerConstraintCheckActions.Value - constraintCheck) + L" ";
        }
        if (results.action == L"")
        {
            results.action += L"NoAction";
        }
        results.noOfChainviolations = totalChainViolations_ - oldChainViolations;
        results.blockListAfter = blockList_;

        TestSession::WriteNoise(Utility::TraceSource,
            "Refresh No : {0} \n Creations + CreationsWithMove:{1} / Movements: {2}/{3} / Action:{4} \n Metrics: {5} \n Averages:{6} \n StDeviations{7}/ ChainViolations:{8}",
            NoOfPLBRuns, results.creations + results.creationsWithMove, results.movements, noOfMovementsApplied_, results.action,
            results.metrics, results.averages, results.stDeviations, results.noOfChainviolations);
        history_.push_back(results);
        time += Common::TimeSpan::FromSeconds(5);
    }

    config.MinPlacementInterval = oldPlacementInterval;
    config.MinConstraintCheckInterval = oldMinConstraintCheckInterval;
    config.MinLoadBalancingInterval = oldLoadBalancingInterval;

    return 0;
}

/// <summary>
/// Gets the PLB run details.
/// </summary>
/// <param name="iterationNumber">The iteration number (First index is 0).</param>
/// <returns></returns>
PLBRun FM::GetPLBRunDetails(int iterationNumber)
{
    //more than one full circle out of range would be an assert
    ASSERT_IF((iterationNumber > NoOfPLBRuns) || (iterationNumber < -1.0 * static_cast<int>(NoOfPLBRuns)), "Error");
    if (iterationNumber < 0)
    {
        //doing this the weird way because unsigned integers behave weirdly with negative numbers
        return history_[NoOfPLBRuns - (-1 * iterationNumber)];
    }
    else
    {
        return history_[iterationNumber];
    }
}

/// <summary>
/// Resets the FM.
/// </summary>
/// <param name="resetPLB">if set to <c>true</c> [Reset PLB as well].</param>
void FM::Reset(bool resetPLB)
{
    if (resetPLB)
    {
        plb_->Dispose();
    }
    AcquireWriteLock grab(lock_);
    nodeToNodePropertyMap_.clear();
    nodes_.clear();
    services_.clear();
    nextFailoverUnitIndex_ = 0;
    failoverUnits_.clear();
    archive_.insert(archive_.end(), history_.begin(), history_.end());
    history_.clear();
    blockList_.clear();
    moveActions_.clear();
    if (resetPLB)
    {
        CreatePLB();
    }
}

void FM::Reset(vector<Reliability::LoadBalancingComponent::NodeDescription> && nodes,
    vector<Reliability::LoadBalancingComponent::ApplicationDescription> && applications,
    vector<Reliability::LoadBalancingComponent::ServiceTypeDescription> && serviceTypes,
    vector<Reliability::LoadBalancingComponent::ServiceDescription> && services,
    vector<Reliability::LoadBalancingComponent::FailoverUnitDescription> && failoverUnits,
    vector<Reliability::LoadBalancingComponent::LoadOrMoveCostDescription> && loadsOrMoveCosts)
{
    
    plb_->Dispose();

    AcquireWriteLock grab(lock_);
    nodeToNodePropertyMap_.clear();
    nodes_.clear();
    services_.clear();
    nextFailoverUnitIndex_ = 0;
    failoverUnits_.clear();
    archive_.insert(archive_.end(), history_.begin(), history_.end());
    history_.clear();
    blockList_.clear();

    vector<Reliability::LoadBalancingComponent::NodeDescription> nodesInternal;
    vector<Reliability::LoadBalancingComponent::FailoverUnitDescription> failoverUnitsInternal;

    map<Federation::NodeId, Node> nodeIdToNodeMap;
    int cnt = 0;
    for (auto nodeDescription = nodes.begin(); nodeDescription != nodes.end(); ++nodeDescription)
    {
        int index = 0;
        auto nodeFound = nodeIdToNodeMap.find(nodeDescription->NodeId);
        if (nodeFound != nodeIdToNodeMap.end())
        {
            nodesInternal.erase(nodesInternal.begin() + nodeFound->second.Index);
            index = nodeFound->second.Index;
            nodeIdToNodeMap.erase(nodeFound);
        }
        else
        {
            index = cnt++;
        }

        wstring faultDomainStr = L"fd:/";
        for (auto fd = nodeDescription->FaultDomainId.begin(); fd != nodeDescription->FaultDomainId.end(); ++fd)
        {
            if (fd != nodeDescription->FaultDomainId.begin())
            {
                faultDomainStr += L",";
            }

            faultDomainStr += *fd;
        }

        Uri faultDomainUri;
        Uri::TryParse(faultDomainStr, faultDomainUri);

        Node node(
            index,
            move(map<std::wstring, uint>(nodeDescription->Capacities)),
            move(Uri(faultDomainUri)),
            move(wstring(nodeDescription->UpgradeDomainId)),
            move(std::map<std::wstring, std::wstring>(nodeDescription->NodeProperties)),
            nodeDescription->IsUp);
        node.Instance = 0;

        nodeIdToNodeMap.insert(make_pair(nodeDescription->NodeId, move(node)));

        nodesInternal.insert(nodesInternal.begin() + index,
            Reliability::LoadBalancingComponent::NodeDescription(
                Federation::NodeInstance(Federation::NodeId(LargeInteger(0, index)), 0),
                nodeDescription->IsUp,
                nodeDescription->DeactivationIntent,
                nodeDescription->DeactivationStatus,
                move(std::map<std::wstring, std::wstring>(nodeDescription->NodeProperties)),
                move(Reliability::LoadBalancingComponent::NodeDescription::DomainId(nodeDescription->FaultDomainId)),
                move(wstring(nodeDescription->UpgradeDomainId)),
                move(std::map<std::wstring, uint>(nodeDescription->CapacityRatios)),
                move(std::map<std::wstring, uint>(nodeDescription->Capacities))));
    }

    map<wstring, Service> serviceNameToServiceMap;
    cnt = 0;
    for (auto serviceDescription = services.begin(); serviceDescription != services.end(); ++serviceDescription)
    {
        wstring serviceTypeName = serviceDescription->ServiceTypeName;
        std::set<int> blockList;

        for (auto sti = serviceTypes.begin(); sti != serviceTypes.end(); ++sti)
        {
            if (sti->Name == serviceTypeName)
            {
                for (auto nodeId = sti->BlockList.begin(); nodeId != sti->BlockList.end(); ++nodeId)
                {
                    auto nodeFound = nodeIdToNodeMap.find(*nodeId);
                    blockList.insert(nodeFound->second.Index);
                }
            }
        }

        wstring affinitizedService = serviceDescription->AffinitizedService;

        vector<Service::Metric> metrics;
        for (auto metric = serviceDescription->Metrics.begin(); metric != serviceDescription->Metrics.end(); ++metric)
        {
            wstring metricName = metric->Name;
            metrics.push_back(move(Service::Metric(move(metricName), metric->Weight, metric->PrimaryDefaultLoad, metric->SecondaryDefaultLoad)));
        }

        Service service(cnt++,
            serviceDescription->Name,
            serviceDescription->IsStateful,
            serviceDescription->PartitionCount,
            serviceDescription->TargetReplicaSetSize,
            affinitizedService,
            move(blockList),
            move(metrics),
            serviceDescription->DefaultMoveCost,
            serviceDescription->PlacementConstraints);

        auto serviceFound = serviceNameToServiceMap.find(service.ServiceName);
        if (serviceFound != serviceNameToServiceMap.end())
        {
            serviceNameToServiceMap.erase(serviceFound);
        }
        serviceNameToServiceMap.insert(make_pair(service.ServiceName, move(service)));
    }

    for (auto failoverUnitDescription = failoverUnits.begin(); failoverUnitDescription != failoverUnits.end(); ++failoverUnitDescription)
    {
        auto serviceFound = serviceNameToServiceMap.find(failoverUnitDescription->ServiceName);
        if (serviceFound == serviceNameToServiceMap.end())
        {
            Assert::CodingError("Service {0} not found while reseting FM and PLB state", failoverUnitDescription->ServiceName);
        }

        int serviceIndex = serviceFound->second.Index;
        bool isStateful = serviceFound->second.IsStateful;

        std::map<std::wstring, uint> primaryLoad;
        std::map<std::wstring, uint> secondaryLoad;
        for (auto metric = serviceFound->second.Metrics.begin(); metric != serviceFound->second.Metrics.end(); ++metric)
        {
            primaryLoad.insert(make_pair(metric->Name, metric->PrimaryDefaultLoad));
            secondaryLoad.insert(make_pair(metric->Name, metric->SecondaryDefaultLoad));
        }

        FailoverUnit failoverUnit(
            failoverUnitDescription->FUId,
            serviceIndex,
            failoverUnitDescription->ServiceName,
            failoverUnitDescription->TargetReplicaSetSize,
            isStateful,
            move(primaryLoad),
            move(secondaryLoad));

        failoverUnit.ChangeVersion((int)failoverUnitDescription->Version);
        vector<Reliability::LoadBalancingComponent::ReplicaDescription> replicasInternal;

        for (auto replicaDescription = failoverUnitDescription->Replicas.begin();
            replicaDescription != failoverUnitDescription->Replicas.end();
            ++replicaDescription)
        {
            auto foundNode = nodeIdToNodeMap.find(replicaDescription->NodeId);
            if (foundNode == nodeIdToNodeMap.end())
            {
                Assert::CodingError("Node {0} not found while reseting FM and PLB state", replicaDescription->NodeId);
            }

            int nodeIndex = foundNode->second.Index;
            int nodeInstance = foundNode->second.Instance;
            int role = 0;
            if (replicaDescription->CurrentRole == Reliability::LoadBalancingComponent::ReplicaRole::Enum::Primary)
            {
                role = 0;
            }
            else if (replicaDescription->CurrentRole == Reliability::LoadBalancingComponent::ReplicaRole::Enum::Secondary)
            {
                role = 1;
            }
            else if (replicaDescription->CurrentRole == Reliability::LoadBalancingComponent::ReplicaRole::Enum::StandBy)
            {
                role = 2;
            }
            else if (replicaDescription->CurrentRole == Reliability::LoadBalancingComponent::ReplicaRole::Enum::Dropped)
            {
                role = 3;
            }
            else if (replicaDescription->CurrentRole == Reliability::LoadBalancingComponent::ReplicaRole::Enum::None)
            {
                role = 4;
            }
            else
            {
                role = 5;
            }
            FailoverUnit::Replica replica(nodeIndex, nodeInstance, role);
            failoverUnit.AddReplica(move(replica));

            ReplicaFlags::Enum flags = ReplicaFlags::None;

            
            if (replicaDescription->IsToBeDroppedByFM)
            {
                flags = ReplicaFlags::Enum(flags + ReplicaFlags::Enum::ToBeDroppedByFM);
            }
            if (replicaDescription->IsToBeDroppedByPLB)
            {
                flags = ReplicaFlags::Enum(flags + ReplicaFlags::Enum::ToBeDroppedByPLB);
            }
            if (replicaDescription->IsToBePromoted)
            {
                flags = ReplicaFlags::Enum(flags + ReplicaFlags::Enum::ToBePromoted);
            }
            if (replicaDescription->IsPendingRemove)
            {
                flags = ReplicaFlags::Enum(flags + ReplicaFlags::Enum::PendingRemove);
            }
            if (replicaDescription->IsDeleted)
            {
                flags = ReplicaFlags::Enum(flags + ReplicaFlags::Enum::Deleted);
            }
            if (replicaDescription->IsPreferredPrimaryLocation)
            {
                flags = ReplicaFlags::Enum(flags + ReplicaFlags::Enum::PreferredPrimaryLocation);
            }
            if (replicaDescription->IsPreferredReplicaLocation)
            {
                flags = ReplicaFlags::Enum(flags + ReplicaFlags::Enum::PreferredReplicaLocation);
            }
            if (replicaDescription->IsPrimaryToBeSwappedOut)
            {
                flags = ReplicaFlags::Enum(flags + ReplicaFlags::Enum::PrimaryToBeSwappedOut);
            }
            if (replicaDescription->IsPrimaryToBePlaced)
            {
                flags = ReplicaFlags::Enum(flags + ReplicaFlags::Enum::PrimaryToBePlaced);
            }
            if (replicaDescription->IsReplicaToBePlaced)
            {
                flags = ReplicaFlags::Enum(flags + ReplicaFlags::Enum::ReplicaToBePlaced);
            }
            if (replicaDescription->IsEndpointAvailable)
            {
                flags = ReplicaFlags::Enum(flags + ReplicaFlags::Enum::EndpointAvailable);
            }
            if (replicaDescription->IsMoveInProgress)
            {
                flags = ReplicaFlags::Enum(flags + ReplicaFlags::Enum::MoveInProgress);
            }
            if (replicaDescription->IsToBeDroppedForNodeDeactivation)
            {
                flags = ReplicaFlags::Enum(flags + ReplicaFlags::Enum::ToBeDroppedForNodeDeactivation);
            }

            replicasInternal.push_back(Reliability::LoadBalancingComponent::ReplicaDescription(
                Federation::NodeInstance(Federation::NodeId(LargeInteger(0, nodeIndex)), nodeInstance),
                replicaDescription->CurrentRole,
                replicaDescription->CurrentState,
                replicaDescription->IsUp,
                flags
                ));
        }

        auto fuFound = failoverUnits_.find(failoverUnit.FailoverUnitId);
        if (fuFound != failoverUnits_.end())
        {
            failoverUnits_.erase(fuFound);
        }
        failoverUnits_.insert(make_pair(failoverUnit.FailoverUnitId, move(failoverUnit)));

        Reliability::FailoverUnitFlags::Flags flags = Reliability::FailoverUnitFlags::Flags(
            false,
            false,
            false,
            failoverUnitDescription->IsDeleted,
            failoverUnitDescription->SwapPrimaryReplicaCount == 1,
            false,
            failoverUnitDescription->IsInUpgrade);

        failoverUnitsInternal.push_back(Reliability::LoadBalancingComponent::FailoverUnitDescription(
            failoverUnitDescription->FUId,
            wstring(failoverUnitDescription->ServiceName),
            failoverUnitDescription->Version,
            move(replicasInternal),
            failoverUnitDescription->ReplicaDifference,
            flags,
            false,
            failoverUnitDescription->IsInQuorumLost,
            failoverUnitDescription->TargetReplicaSetSize));
    }

    for (auto node = nodeIdToNodeMap.begin(); node != nodeIdToNodeMap.end(); ++node)
    {
        nodes_.insert(make_pair(node->second.Index, move(node->second)));
    }

    for (auto service = serviceNameToServiceMap.begin(); service != serviceNameToServiceMap.end(); ++service)
    {
        services_.insert(make_pair(service->second.Index, move(service->second)));
    }

    vector<Reliability::LoadBalancingComponent::ServiceTypeDescription> serviceTypesInternal;
    for (auto service = services_.begin(); service != services_.end(); ++service)
    {
        set<Federation::NodeId> blockList;
        for (auto nodeIt = nodeIdToNodeMap.begin(); nodeIt != nodeIdToNodeMap.end(); ++nodeIt)
        {
            auto nodeFound = service->second.BlockList.find(nodeIt->second.Index);
            if (nodeFound != service->second.BlockList.end())
            {
                blockList.insert(Federation::NodeId(LargeInteger(0, nodeIt->second.Index)));
            }
        }

        serviceTypesInternal.push_back(move(
            Reliability::LoadBalancingComponent::ServiceTypeDescription(
            StringUtility::ToWString(service->second.Index) + L"_type", move(blockList))));
    }
    // Try move this up
    vector<Reliability::LoadBalancingComponent::ServiceDescription> servicesInternal;
    for (auto serviceDescription = services.begin(); serviceDescription != services.end(); ++serviceDescription)
    {
        auto serviceFound = serviceNameToServiceMap.find(serviceDescription->Name);
        if (serviceFound != serviceNameToServiceMap.end())
        {
            std::vector<Reliability::LoadBalancingComponent::ServiceMetric> metrics = serviceDescription->Metrics;
            Reliability::LoadBalancingComponent::ServiceDescription description(
                wstring(serviceDescription->Name),
                wstring(StringUtility::ToWString(serviceFound->second.Index) + L"_type"),
                wstring(serviceDescription->ApplicationName),
                serviceDescription->IsStateful,
                wstring(serviceDescription->PlacementConstraints),
                wstring(serviceDescription->AffinitizedService),
                serviceDescription->AlignedAffinity,
                std::vector<Reliability::LoadBalancingComponent::ServiceMetric>(serviceDescription->Metrics),
                serviceDescription->DefaultMoveCost,
                serviceDescription->OnEveryNode,
                serviceDescription->PartitionCount,
                serviceDescription->TargetReplicaSetSize,
                serviceDescription->HasPersistedState
                );

            servicesInternal.push_back(move(description));
        }
        else
        {
            Assert::CodingError("There is no service: {0}", serviceDescription->Name);
        }
       
    }

    CreatePLB(
        move(nodesInternal),
        move(applications),
        move(serviceTypesInternal),
        move(servicesInternal),
        move(failoverUnitsInternal),
        move(loadsOrMoveCosts));

    //Add empty failover unit update in order to force processing of pending updates
    GetPLBObject()->UpdateServiceType(Reliability::LoadBalancingComponent::ServiceTypeDescription(wstring(L"ForcePLBUpdateServiceType"), set<Federation::NodeId>()));
    GetPLBObject()->UpdateService(Reliability::LoadBalancingComponent::ServiceDescription(
        wstring(L"ForcePLBUpdateService"),
        wstring(L"ForcePLBUpdateServiceType"),
        wstring(L""),
        false,
        wstring(L""),
        wstring(L""),
        false,
        vector<Reliability::LoadBalancingComponent::ServiceMetric>(),
        0,
        false));
    GetPLBObject()->UpdateFailoverUnit(LoadBalancingComponent::FailoverUnitDescription(
        Guid::Empty(),
        wstring(L"ForcePLBUpdateService"),
        0,
        vector<LoadBalancingComponent::ReplicaDescription>(),
        0,
        Reliability::FailoverUnitFlags::Flags(),
        false,
        false));
    GetPLBObject()->ProcessPendingUpdatesPeriodicTask();
}

/// <summary>
/// Gets the metric aggregates. Return value is how many nodes have that metric defined. Nodes with Infinite Capacity are also considered for averaging
/// </summary>
/// <param name="metric">The metric.</param>
/// <param name="avg">Average metric load across relevant nodes.</param>
/// <param name="dev">StDeviation of metric across relevant nodes.</param>
/// <returns>Number of nodes that have this metric defined</returns>
int FM::GetMetricAggregates(const std::wstring metric, double &avg, double &dev)
{
    int64 totalLoad = 0;
    int64 totalSquaredLoad = 0;
    int totalNoOfNodes = 0;
    for (auto const& node : nodes_)
    {
        ServiceModel::NodeLoadInformationQueryResult nodeLoadInfo;
        GetPLBObject()->GetNodeLoadInformationQueryResult(Node::CreateNodeId(node.second.Index), nodeLoadInfo);
        if (nodeLoadInfo.NodeLoadMetricInformation.size() == 0)
        {
            continue;
        }
        for (auto nodeload : nodeLoadInfo.NodeLoadMetricInformation)
        {
            if (StringUtility::AreEqualCaseInsensitive(nodeload.Name, metric))
            {
                totalNoOfNodes++;
                totalLoad += nodeload.NodeLoad;
                totalSquaredLoad += nodeload.NodeLoad*nodeload.NodeLoad;
            }
        }
    }
    double average = ((double)(totalLoad) / (totalNoOfNodes));
    double stdeviation = ((double)(totalSquaredLoad) / (totalNoOfNodes))-(average*average);

    avg = average;
    dev = sqrt(stdeviation);
    return totalNoOfNodes;
}

/// <summary>
/// Gets the node loads.
/// </summary>
/// <param name="nodeLoads">The structure to hold node loads. Passed by reference</param>
void FM::GetNodeLoads(std::map<int, std::vector< std::pair<std::wstring, int64>>> &nodeLoads)
{
    nodeLoads.clear();
    for (auto const& node : nodes_)
    {
        ServiceModel::NodeLoadInformationQueryResult nodeLoadInfo;
        GetPLBObject()->GetNodeLoadInformationQueryResult(Node::CreateNodeId(node.second.Index), nodeLoadInfo);
        if (nodeLoadInfo.NodeLoadMetricInformation.size() == 0)
        {
            continue;
        }

        for (auto nodeload : nodeLoadInfo.NodeLoadMetricInformation)
        {
            if (nodeLoads.count(node.second.Index) == 0)
            {
                nodeLoads[node.second.Index] = std::vector< std::pair<std::wstring, int64>>();
            }
            std::wstring metric = nodeload.Name;
            if (IsLogged(metric))
            {
                nodeLoads[node.second.Index].push_back(std::make_pair(nodeload.Name, nodeload.NodeLoad));
            }
        }
    }
}

///<summary>
///Adds the argument metric to the list of metrics to log while getting aggregates and node details.
///</summary>
///<param name="metric">name of metric to be logged. "all" resets the list to default.</param>
void FM::LogMetric(wstring metric)
{
    if (metric == L"all")
    {
        loggedMetrics_.clear();
    }
    else
    {
        StringUtility::ToLower(metric);
        if (find(loggedMetrics_.begin(), loggedMetrics_.end(), metric) == loggedMetrics_.end())
        {
            loggedMetrics_.push_back(metric);
        }
    }
}

/// <summary>
/// Determines whether the specified metric is logged. Case Insensitive.
/// </summary>
/// <param name="metric">The metric name.</param>
/// <returns>True if logged, or if no logging specified. False if not logged.</returns>
bool FM::IsLogged(wstring metric)
{
    if (loggedMetrics_.size() == 0)
    {
        return true;
    }
    StringUtility::ToLower(metric);
    if (find(loggedMetrics_.begin(), loggedMetrics_.end(), metric) != loggedMetrics_.end())
    {
        return true;
    }
    else
    {
        return false;
    }
}

/// <summary>
/// Gets the cluster aggregates.
/// </summary>
/// <param name="metricAggregates">The structure that should contain metric aggregates. Passed by reference</param>
void FM::GetClusterAggregates(map<wstring, pair<double, double>> &metricAggregates)
{
    //map<std::wstring, std::pair<double, double>> metricAggregates = map<std::wstring, std::pair<double, double>>();
    metricAggregates.clear();
    if (loggedMetrics_.size() != 0)
    {
        for (auto metric : loggedMetrics_)
        {
            double average;
            double deviation;
            if (GetMetricAggregates(metric, average, deviation) != 0)
            {
                metricAggregates[metric] = std::make_pair(average, deviation);
            }
        }
    }
    else
    {
        set<wstring> metrics = GetUniqueMetrics();
        for (auto metric : metrics)
        {
            double average;
            double deviation;
            if (GetMetricAggregates(metric, average, deviation) != 0)
            {
                metricAggregates[metric] = std::make_pair(average, deviation);
            }
        }
    }
}

/// <summary>
/// Creates a new service from a list of metrics and primary Loads. Uses default Metric weight(0.5) and Same Load for secondary as primary
/// </summary>
/// <param name="metricNames">List of metric names.</param>
/// <param name="metricPrimarySize">Corresponding Default Metric Loads of the primary replicas.</param>
void FM::CreateNewService(vector<wstring> metricNames, vector<uint> metricPrimarySize)
{
    vector<Service::Metric> metrics = vector<Service::Metric>();

    for (int i = 0; i < metricNames.size(); i++ )
    {
        wstring metricName = metricNames[i];
        //Default Metric weight is 0.5.
        double serviceMetricWeight = 0.5;
        uint serviceMetricSize = metricPrimarySize[i];
        //Secondary has same load as primary.
        uint serviceMetricSecondarySize = serviceMetricSize;
        metrics.push_back(Service::Metric(move(metricName), serviceMetricWeight, serviceMetricSize, serviceMetricSecondarySize));
    }
    CreateNewService(metrics);
}

/// <summary>
/// Creates a new service from a list of metric objects, and refreshes PLB until the service has a stable number of replicas.
/// </summary>
/// <param name="metrics">The metrics.</param>
void FM::CreateNewService(vector<Service::Metric> &metrics)
{
    int fuIndex = nextFailoverUnitIndex_;
    CreateService(Service(static_cast<int>(services_.size()), L"Service_"+StringUtility::ToWString(services_.size()),
        true, 1, 3, L"", move(set<int>()), move(metrics), config_.DefaultMoveCost, L""), true);
    WaitFuStable(Common::Guid(fuIndex,0,0,0,0,0,0,0,0,0,0));
}

/// <summary>
/// Creates a new service from a list of metric objects, and refreshes PLB until the service has a stable number of replicas.
/// </summary>
/// <param name="metrics">The metrics.</param>
void FM::CreateNewService(Service && service)
{
    int fuIndex = nextFailoverUnitIndex_;
    CreateService(move(service), true);
    WaitFuStable(Common::Guid(fuIndex, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0));
}

/// <summary>
/// Refreshes PLV until the parameter FailoverUnit has a stable number of replicas.
/// </summary>
/// <param name="failoverUnitId">Id of the failover unit.</param>
void FM::WaitFuStable(Common::Guid failoverUnitId)
{
    if (failoverUnits_.find(failoverUnitId) == failoverUnits_.end())
    {
        TestSession::WriteWarning(Utility::TraceSource, "Cannot find the failover unit");
        return;
    }
    FailoverUnit const& targetFu = failoverUnits_.at(failoverUnitId);
    int replicaSize = static_cast<int>(targetFu.Replicas.size());
    Common::StopwatchTime start = Common::Stopwatch::Now();
    TestSession::WriteNoise(Utility::TraceSource, "Waiting for replicas of failover unit:{0} to be stable ", targetFu);

    int diffMove = 0;
    Common::TimeSpan diff;

    while (replicaSize != targetFu.TargetReplicaCount)
    {
        Common::StopwatchTime nowPLBRun = Common::Stopwatch::Now();
        Sleep(1000);
        diffMove += ForceExecutePLB(1, true);
        int newReplicaSize = static_cast<int>(targetFu.Replicas.size());
        if (newReplicaSize > replicaSize)
        {
            diff = Common::Stopwatch::Now() - nowPLBRun;

            replicaSize = newReplicaSize;

            TestSession::WriteNoise(Utility::TraceSource, "Replica Set Size at {0} after: {1} seconds", replicaSize, diff.ToString());
        }
    }

    TestSession::WriteNoise(Utility::TraceSource, "Stable replica size: {0}", replicaSize);
    diff = Common::Stopwatch::Now() - start;
}

bool FM::IsBlocked(int nodeIndex, Common::Guid failoverUnitId)
{
    auto it = failoverUnits_.find(failoverUnitId);
    if (it == failoverUnits_.end()) { return false; }
    FailoverUnit & failoverUnit = it->second;
    auto targetService = services_.find(failoverUnit.ServiceIndex);

    return (targetService->second.BlockList.count(nodeIndex) != 0);
}

void FM::RemoveFromBlockList(int nodeIndex, Common::Guid failoverUnitId)
{
    auto it = failoverUnits_.find(failoverUnitId);
    if (it == failoverUnits_.end()) { return; }
    FailoverUnit & failoverUnit = it->second;
    auto targetService = services_.find(failoverUnit.ServiceIndex);

    targetService->second.RemoveFromBlockList(nodeIndex);
}

void FM::BreakChains(std::map<Common::Guid, Reliability::LoadBalancingComponent::FailoverUnitMovement> & moveActions)
{
    //AcquireWriteLock grab(lock_);
    std::map<std::pair<std::wstring, Federation::NodeId>, double> NodePercentageOccupancies;
    std::map<std::pair<std::wstring, Federation::NodeId>, int> NodeMoveInCounts;
    bool clear = false;
    for each (auto const& moveAction in moveActions)
    {
        clear = false;
        for each (Reliability::LoadBalancingComponent::FailoverUnitMovement::PLBAction action in moveAction.second.Actions)
        {
            Common::Guid failoverUnitId = moveAction.first;

            auto it = failoverUnits_.find(failoverUnitId);
            if (it == failoverUnits_.end())
            {
                continue;
            }
            FailoverUnit & failoverUnit = it->second;

            switch (action.Action)
            {

            case Reliability::LoadBalancingComponent::FailoverUnitMovementType::SwapPrimarySecondary:
                break;
            default:
                int nodeIndex = static_cast<int>(action.TargetNode.IdValue.Low);
                map<int, LBSimulator::Node>::const_iterator itNode = nodes_.find(nodeIndex);
                if (itNode == nodes_.end())
                {
                    break;
                }
                Federation::NodeId nodeid = Node::CreateNodeId(itNode->second.Index);
                ServiceModel::NodeLoadInformationQueryResult nodeLoadInfo;
                Common::ErrorCode result = GetPLBObject()->GetNodeLoadInformationQueryResult(nodeid, nodeLoadInfo);
                ASSERT_IF(!result.IsSuccess(), "Error while trying to get NodeLoadInfo: {0}", result.Message);
                for (auto metric : nodeLoadInfo.NodeLoadMetricInformation)
                {
                    if (metric.NodeCapacity > 0)
                    {
                        auto key = std::make_pair(metric.Name, nodeid);
                        double occupancy = 0;
                        if (NodePercentageOccupancies.count(key))
                        {
                            occupancy = NodePercentageOccupancies[key];
                        }
                        else
                        {
                            NodeMoveInCounts[key] = 0;
                            occupancy = ((metric.NodeLoad*1.0) / metric.NodeCapacity);
                        }
                        unsigned int load = 0;
                        if (failoverUnit.IsStateful)
                        {
                            if (action.Action == Reliability::LoadBalancingComponent::FailoverUnitMovementType::MovePrimary
                                || action.Action == Reliability::LoadBalancingComponent::FailoverUnitMovementType::AddPrimary)
                            {
                                load = failoverUnit.PrimaryLoad.find(metric.Name)->second;
                            }
                            else
                            {
                                load = failoverUnit.SecondaryLoad.find(metric.Name)->second;
                            }
                        }
                        else
                            load = failoverUnit.InstanceLoad.find(metric.Name)->second;

                        occupancy = (occupancy*metric.NodeCapacity + load) / metric.NodeCapacity;
                        NodePercentageOccupancies[key] = occupancy;
                        NodeMoveInCounts[key]++;
                    }

                }

                break;

            }
        }
    }
    clear = false;
    for each (auto const& moveAction in moveActions)
    {
        clear = false;
        for each (Reliability::LoadBalancingComponent::FailoverUnitMovement::PLBAction action in moveAction.second.Actions)
        {
            Common::Guid failoverUnitId = moveAction.first;

            auto it = failoverUnits_.find(failoverUnitId);
            if (it == failoverUnits_.end())
            {
                continue;
            }
            FailoverUnit & failoverUnit = it->second;

            switch (action.Action)
            {

            case Reliability::LoadBalancingComponent::FailoverUnitMovementType::SwapPrimarySecondary:
                break;
            default:
                int nodeIndex = static_cast<int>(action.TargetNode.IdValue.Low);
                map<int, LBSimulator::Node>::const_iterator itNode = nodes_.find(nodeIndex);
                if (itNode == nodes_.end())
                {
                    break;
                }
                Federation::NodeId nodeid = Node::CreateNodeId(itNode->second.Index);
                ServiceModel::NodeLoadInformationQueryResult nodeLoadInfo;
                Common::ErrorCode result = GetPLBObject()->GetNodeLoadInformationQueryResult(nodeid, nodeLoadInfo);
                ASSERT_IF(!result.IsSuccess(), "Error while trying to get NodeLoadInfo: {0}", result.Message);
                for (auto metric : nodeLoadInfo.NodeLoadMetricInformation)
                {
                    if (metric.NodeCapacity > 0)
                    {
                        auto key = std::make_pair(metric.Name, nodeid);
                        double occupancy = 0;
                        if (NodePercentageOccupancies.count(key))
                        {
                            occupancy = NodePercentageOccupancies[key];
                        }
                        unsigned int load = 0;
                        if (failoverUnit.IsStateful)
                        {
                            if (action.Action == Reliability::LoadBalancingComponent::FailoverUnitMovementType::MovePrimary
                                || action.Action == Reliability::LoadBalancingComponent::FailoverUnitMovementType::AddPrimary)
                            {
                                load = failoverUnit.PrimaryLoad.find(metric.Name)->second;
                            }
                            else
                            {
                                load = failoverUnit.SecondaryLoad.find(metric.Name)->second;
                            }
                        }
                        else
                            load = failoverUnit.InstanceLoad.find(metric.Name)->second;
                        if (occupancy > 1)
                        {
                            auto targetService = services_.find(failoverUnit.ServiceIndex);
                            //targetService->second.AddToBlockList(nodeIndex);
                            //blockList_.insert(make_pair(nodeIndex, failoverUnit.ServiceIndex));
                            TestSession::WriteNoise(Utility::TraceSource,
                                "[PROCESSMOVEMENT]:Breaking Chain Violation at:{0}, Current NodeLoad: {1}/{2}, ReplicaLoad: {3}",
                                moveAction.second, metric.Name, metric.NodeLoad, load);
                            clear = true;
                        }

                    }

                }

                break;

            }
        }
        if (clear)
        {
            TestSession::WriteNoise(Utility::TraceSource, "clear:{0} actions:{1}", clear, moveAction.second);
            //moveActions.erase(moveAction.first);
        }
    }
}

/// <summary>
/// Counts the number of Capacity-Violations due to MoveIn-MoveOut Chains on nodes. Also updates the global variable for total number of chain violations.
/// </summary>
/// <param name="moveActions">Movements Generated by PLB.</param>
/// <returns>Total Number of Chain Violations</returns>
int FM::CountChainsOnFullNodes(std::map<Common::Guid, Reliability::LoadBalancingComponent::FailoverUnitMovement> & moveActions)
{
    std::map<std::pair<std::wstring, Federation::NodeId>, double> NodePercentageOccupancies;
    std::map<std::pair<std::wstring, Federation::NodeId>, int> NodeMoveInCounts;
    int chainViolations = 0;

    for each (auto const& moveAction in moveActions)
    {
        for each (Reliability::LoadBalancingComponent::FailoverUnitMovement::PLBAction action in moveAction.second.Actions)
        {
            Common::Guid failoverUnitId = moveAction.first;

            auto it = failoverUnits_.find(failoverUnitId);
            if (it == failoverUnits_.end())
            {
                continue;
            }
            FailoverUnit & failoverUnit = it->second;
            switch (action.Action)
            {
                case Reliability::LoadBalancingComponent::FailoverUnitMovementType::SwapPrimarySecondary:
                    break;
                default:
                    int nodeIndex = static_cast<int>(action.TargetNode.IdValue.Low);
                    map<int, LBSimulator::Node>::const_iterator itNode = nodes_.find(nodeIndex);
                    if (itNode == nodes_.end())
                    {
                        break;
                    }
                    Federation::NodeId nodeid = Node::CreateNodeId(itNode->second.Index);
                    ServiceModel::NodeLoadInformationQueryResult nodeLoadInfo;
                    GetPLBObject()->GetNodeLoadInformationQueryResult(nodeid, nodeLoadInfo);
                    for (auto metric : nodeLoadInfo.NodeLoadMetricInformation)
                    {
                        if (metric.NodeCapacity > 0)
                        {
                            auto key = std::make_pair(metric.Name, nodeid);
                            double occupancy = 0;
                            if (NodePercentageOccupancies.count(key))
                            {
                                occupancy = NodePercentageOccupancies[key];
                            }
                            else
                            {
                                NodeMoveInCounts[key] = 0;
                                occupancy = ((metric.NodeLoad*1.0) / metric.NodeCapacity);
                            }
                            unsigned int load = 0;
                            if (failoverUnit.IsStateful)
                            {
                                if (action.Action == Reliability::LoadBalancingComponent::FailoverUnitMovementType::MovePrimary
                                    || action.Action == Reliability::LoadBalancingComponent::FailoverUnitMovementType::AddPrimary)
                                {
                                    load = failoverUnit.PrimaryLoad.find(metric.Name)->second;
                                }
                                else
                                {
                                    load = failoverUnit.SecondaryLoad.find(metric.Name)->second;
                                }
                            }
                            else
                            {
                                load = failoverUnit.InstanceLoad.find(metric.Name)->second;
                            }
                            occupancy = (occupancy*metric.NodeCapacity + load)/metric.NodeCapacity;
                            if (occupancy > 1)
                            {
                                TestSession::WriteNoise(Utility::TraceSource,
                                    "[PROCESSMOVEMENT]:Chain Violation at:{0}, Current NodeLoad: {1}", moveAction.second, metric.NodeLoad);
                                totalChainViolations_++;
                                chainViolations++;
                            }

                            NodePercentageOccupancies[key] = occupancy;
                            NodeMoveInCounts[key]++;
                        }
                    }
                    break;
            }
        }
    }
    return chainViolations;
}

void FM::ProcessFailoverUnitMovements(std::map<Common::Guid, Reliability::LoadBalancingComponent::FailoverUnitMovement> && moveActions, Reliability::LoadBalancingComponent::DecisionToken && dToken)
{
    UNREFERENCED_PARAMETER(dToken);
    AcquireWriteLock grab(lock_);

    //if (nodeBlockDuration_ > 0) { BreakChains(moveActions); }
    //CountChainsOnFullNodes(moveActions);
    for each (auto const& moveAction in moveActions)
    {
        TestSession::WriteNoise(Utility::TraceSource, "Executing movements:{0}", moveAction.second);
        Common::Guid failoverUnitId = moveAction.first;
        auto it = failoverUnits_.find(failoverUnitId);
        if (it == failoverUnits_.end())
        {
            continue;
        }

        FailoverUnit & failoverUnit = it->second;

        if (failoverUnit.Version == moveAction.second.Version)
        {
            set<int> ignoredActions;
            for each (Reliability::LoadBalancingComponent::FailoverUnitMovement::PLBAction action in moveAction.second.Actions)
            {
                int nodeIndex = static_cast<int>(action.TargetNode.IdValue.Low);
                if (action.Action == Reliability::LoadBalancingComponent::FailoverUnitMovementType::SwapPrimarySecondary)
                {
                    noOfSwapsApplied_++;
                }
                else
                {
                    noOfMovementsApplied_++;
                }

                ASSERT_IFNOT(!IsBlocked(nodeIndex, failoverUnitId),
                    "[PROCESSMOVEMENTS]:Following movements are blocked: {0}", nodeIndex);
            }
            failoverUnit.ProcessMovement(moveAction.second, nodes_, ignoredActions);
            plb_->UpdateFailoverUnit(failoverUnit.GetPLBFailoverUnitDescription());
        }

        std::vector<Reliability::LoadBalancingComponent::FailoverUnitMovement::PLBAction> actions;
        
        for (auto it2 = moveAction.second.Actions.begin(); it2 != moveAction.second.Actions.end(); ++it2)
        {
            actions.push_back(*it2);
        }

        Reliability::LoadBalancingComponent::FailoverUnitMovement movement(
            moveAction.second.FailoverUnitId,
            move(wstring(moveAction.second.ServiceName)),
            moveAction.second.IsStateful,
            moveAction.second.Version,
            moveAction.second.IsFuInTransition,
            move(actions));
        moveActions_.insert(make_pair(moveAction.first, move(movement)));
    }
}
void FM::UpdateAppUpgradePLBSafetyCheckStatus(ServiceModel::ApplicationIdentifier const& appId)
{
    UNREFERENCED_PARAMETER(appId);
}

void FM::UpdateFailoverUnitTargetReplicaCount(Common::Guid const & partitionId, int targetCount)
{
    UNREFERENCED_PARAMETER(partitionId);
    UNREFERENCED_PARAMETER(targetCount);
}

bool FM::VerifyAll()
{
    AcquireReadLock grab(lock_);

    bool ret = true;

    if (ret) { ret = VerifyNodes(); }

    if (ret) { ret = VerifyServices(); }

    if (ret) { ret = VerifyStable(); }

    if (ret) { ret = FailoverUnitsStable(); }

    return ret;
}

wstring FM::NodesToString()
{
    AcquireReadLock grab(lock_);

    std::wstring result;
    Common::StringWriter writer(result);
    writer.WriteLine("{0} nodes:", nodes_.size());
    writer.WriteLine("index nodeName instance isUp capacityRatios capacities faultDomainId upgradeDomainId");
    for (auto it = nodes_.begin(); it != nodes_.end(); it++)
    {
        writer.WriteLine("{0}", it->second);
    }

    return result;
}

wstring FM::ServicesToString()
{
    AcquireReadLock grab(lock_);

    std::wstring result;
    Common::StringWriter writer(result);
    writer.WriteLine("{0} services:", services_.size());
    writer.WriteLine("index isStateful partitionCount replicaCountPerPartition blockList startFailoverUnitIndex metrics");

    for (auto it = services_.begin(); it != services_.end(); it++)
    {
        writer.WriteLine("{0}", it->second);
    }

    return result;
}

wstring FM::FailoverUnitsToString(size_t start, size_t end)
{
    ASSERT_IFNOT(start <= end, "Error");
    AcquireReadLock grab(lock_);

    std::wstring result;
    Common::StringWriter writer(result);
    writer.WriteLine("{0} failover units:", failoverUnits_.size());
    writer.WriteLine("index serviceIndex requiredReplicaCount isStateful primaryLoad secondaryLoad replicas(nodeIndex: nodeInstance/role)");

    for (auto it = failoverUnits_.begin(); it != failoverUnits_.end(); it++)
    {
        size_t failoverUnitIndex = static_cast<size_t>(it->second.FailoverUnitId.AsGUID().Data1);
        if (failoverUnitIndex < start || failoverUnitIndex > end)
        {
            continue;
        }

        writer.WriteLine("{0}", it->second);
        if (result.size() > 8000)
        {
            result.append(L"...<truncated>");
            break;
        }
    }

    return result;
}

wstring FM::PlacementToString()
{
    AcquireReadLock grab(lock_);

    return wstring(L"Feature disabled");
}

// private members

FM::FM(LBSimulatorConfig const& config)
    : config_(config), random_(config.RandomSeed), nextFailoverUnitIndex_(0)
{
    noOfMovementsApplied_ = 0;
    noOfSwapsApplied_ = 0;
    totalChainViolations_ = 0;
    loggedMetrics_ = std::vector<std::wstring>();
    history_ = vector<PLBRun>();
    blockList_ = set<pair<int, Common::Guid>>();
    nodeBlockDuration_ = 1;
}

void FM::CreatePLB()
{
    // Create a blank PLB
    plb_ = Reliability::LoadBalancingComponent::IPlacementAndLoadBalancing::Create(
        *this, *this, false, true,
        vector<Reliability::LoadBalancingComponent::NodeDescription>(),
        vector<Reliability::LoadBalancingComponent::ApplicationDescription>(),
        vector<Reliability::LoadBalancingComponent::ServiceTypeDescription>(),
        vector<Reliability::LoadBalancingComponent::ServiceDescription>(),
        vector<Reliability::LoadBalancingComponent::FailoverUnitDescription>(),
        vector<Reliability::LoadBalancingComponent::LoadOrMoveCostDescription>(),
        Client::HealthReportingComponentSPtr(),
        Api::IServiceManagementClientPtr(),
        Reliability::FailoverConfig::GetConfig().IsSingletonReplicaMoveAllowedDuringUpgradeEntry);

    currentTicks_ = Stopwatch::Now().Ticks;
}

void FM::CreatePLB(
    std::vector<Reliability::LoadBalancingComponent::NodeDescription> && nodes,
    std::vector<Reliability::LoadBalancingComponent::ApplicationDescription> && applications,
    std::vector<Reliability::LoadBalancingComponent::ServiceTypeDescription> && serviceTypes,
    std::vector<Reliability::LoadBalancingComponent::ServiceDescription> && services,
    std::vector<Reliability::LoadBalancingComponent::FailoverUnitDescription> && failoverUnits,
    std::vector<Reliability::LoadBalancingComponent::LoadOrMoveCostDescription> && loadsOrMoveCosts)
{
    // Create a blank PLB
    plb_ = Reliability::LoadBalancingComponent::IPlacementAndLoadBalancing::Create(
        *this, *this, false, true,
        move(nodes),
        move(applications),
        move(serviceTypes),
        move(services),
        move(failoverUnits),
        move(loadsOrMoveCosts),
        Client::HealthReportingComponentSPtr(),
        Api::IServiceManagementClientPtr(),
        Reliability::FailoverConfig::GetConfig().IsSingletonReplicaMoveAllowedDuringUpgradeEntry);

    currentTicks_ = Stopwatch::Now().Ticks;
}

Reliability::LoadBalancingComponent::PlacementAndLoadBalancing * FM::GetPLBObject()
{
    return dynamic_cast<Reliability::LoadBalancingComponent::PlacementAndLoadBalancing *>(&(*plb_));
}

void FM::ProcessFailoverUnits()
{
    for (auto it = failoverUnits_.begin(); it != failoverUnits_.end();)
    {
        FailoverUnit & failoverUnit = it->second;

        // check deleted services
        auto serviceIt = services_.find(failoverUnit.ServiceIndex);
        if (serviceIt == services_.end())
        {
            failoverUnit.FailoverUnitToBeDeleted();
            plb_->UpdateFailoverUnit(failoverUnit.GetPLBFailoverUnitDescription());
            plb_->DeleteFailoverUnit(StringUtility::ToWString(failoverUnit.ServiceIndex), failoverUnit.FailoverUnitId);
            it = failoverUnits_.erase(it);
        }
        else
        {
            // check down nodes
            set<int> removedReplicas;
            for each (FailoverUnit::Replica const& replica in failoverUnit.Replicas)
            {
                Node const& node = GetNode(replica.NodeIndex);
                if (!node.IsUp)
                {
                    removedReplicas.insert(replica.NodeIndex);
                }
            }

            if (!removedReplicas.empty())
            {
                for each (int nodeIndex in removedReplicas)
                {
                    failoverUnit.ReplicaDown(nodeIndex);
                }
                plb_->UpdateFailoverUnit(failoverUnit.GetPLBFailoverUnitDescription());
            }

            it++;
        }
    }
}

/// <summary>
/// Returns a set of all metric name applicable in the cluster
/// </summary>
/// <returns>A set of metric names</returns>
set<wstring> FM::GetUniqueMetrics()
{
    // this method update the unique metric list
    set<wstring> metrics;
    for (auto it = services_.begin(); it != services_.end(); ++it)
    {
        for (auto itMetric = it->second.Metrics.begin(); itMetric != it->second.Metrics.end(); ++itMetric)
        {
            metrics.insert(itMetric->Name);
        }
    }

    return metrics;
}

void FM::InternalReportLoad(int failoverUnitIndex, int role, wstring const& metric, uint load)
{
    FailoverUnit & failoverUnit = GetFailoverUnit(failoverUnitIndex);

    failoverUnit.UpdateLoad(role, metric, load);
}

void FM::UpdateLoad(int failoverUnitIndex)
{
    FailoverUnit & failoverUnit = GetFailoverUnit(failoverUnitIndex);
    plb_->UpdateLoadOrMoveCost(failoverUnit.GetPLBLoadOrMoveCostDescription());
}

LBSimulator::Node & FM::GetNode(int nodeIndex)
{
    auto it = nodes_.find(nodeIndex);
    ASSERT_IF(it == nodes_.end(), "Node {0} doesn't exist", nodeIndex);
    return it->second;
}

LBSimulator::Node const & FM::GetNode(int nodeIndex) const
{
    auto it = nodes_.find(nodeIndex);
    ASSERT_IF(it == nodes_.end(), "Node {0} doesn't exist", nodeIndex);
    return it->second;
}

LBSimulator::Service & FM::GetService(int serviceIndex)
{
    auto it = services_.find(serviceIndex);
    ASSERT_IF(it == services_.end(), "Service {0} doesn't exist", serviceIndex);
    return it->second;
}
bool FM::SetServiceStartFailoverUnitIndex(int serviceIndex, int startFailoverUnitIndex)
{
    (services_.find(serviceIndex))->second.SetStartFailoverUnitIndex(startFailoverUnitIndex);
    return true;
}

LBSimulator::Service const & FM::GetService(int serviceIndex) const
{
    auto it = services_.find(serviceIndex);
    ASSERT_IF(it == services_.end(), "Service {0} doesn't exist", serviceIndex);
    return it->second;
}

FailoverUnit & FM::GetFailoverUnit(int failoverUnitIndex)
{
    auto it = failoverUnits_.find(Common::Guid(failoverUnitIndex,0,0,0,0,0,0,0,0,0,0));
    ASSERT_IF(it == failoverUnits_.end(), "FailoverUnit {0} doesn't exist", failoverUnitIndex);
    return it->second;
}

void FM::GetFailoverUnits(wstring serviceName, vector<Common::Guid> failoverUnits)
{
    for each (auto const& iter in failoverUnits_)
    {
        std::wstring parentServiceName = iter.second.ServiceName;
        if (serviceName == parentServiceName)
        {
            Common::Guid fuGuid(iter.second.FailoverUnitId);
            failoverUnits.push_back(fuGuid);
        }
    }
    ASSERT_IF(failoverUnits.size() == 0, "No Failover units exist with service {0} ", serviceName);
}

FailoverUnit & FM::GetFailoverUnit(Common::Guid guid)
{
    auto it = failoverUnits_.find(guid);
    ASSERT_IF(it == failoverUnits_.end(), "FailoverUnit {0} doesn't exist", guid);
    return it->second;
}

FailoverUnit const & FM::GetFailoverUnit(Common::Guid guid) const
{
    auto it = failoverUnits_.find(guid);
    ASSERT_IF(it == failoverUnits_.end(), "FailoverUnit {0} doesn't exist", guid);
    return it->second;
}

FailoverUnit const & FM::GetFailoverUnit(int failoverUnitIndex) const
{
    auto it = failoverUnits_.find(Common::Guid(failoverUnitIndex,0,0,0,0,0,0,0,0,0,0));
    ASSERT_IF(it == failoverUnits_.end(), "FailoverUnit {0} doesn't exist", failoverUnitIndex);
    return it->second;
}

bool FM::VerifyNodes()
{
    // check each domain If they have domain id defined for all nodes
    bool verificationResult = true;

    if (nodes_.empty())
    {
        return true;
    }

    for (auto itNode = nodes_.begin(); itNode != nodes_.end(); itNode ++)
    {
        Node const& node = itNode->second;
        if (!node.IsUp)
        {
            TestSession::WriteInfo(Utility::TraceSource, "Node {0} is not UP", node.get_Index());
            verificationResult = false;
            continue;
        }
    }

    return verificationResult;
}

bool FM::VerifyServices()
{
    // verify # of services and # of partitions for each service
    map<int, int> services;
    for each (auto const& pair in failoverUnits_)
    {
        FailoverUnit const& failoverUnit = pair.second;

        auto it = services.find(failoverUnit.ServiceIndex);
        if (it == services.end())
        {
            it = services.insert(make_pair(failoverUnit.ServiceIndex, 0)).first;
        }
        it->second++;
    }

    if (services.size() != services_.size())
    {
        TestSession::WriteInfo(Utility::TraceSource,
            "Number of services doesn't match, current {0}, expected {1}", services.size(), services_.size());
        return false;
    }

    for each (auto const& pair in services_)
    {
        Service const& service = pair.second;

        auto itService = services.find(service.Index);

        if (service.IsEmpty)
        {
            TestSession::WriteInfo(Utility::TraceSource, "Service {0} is empty", service.Index);
            return false;
        }

        ASSERT_IFNOT(service.StartFailoverUnitIndex >= 0
        && service.EndFailoverUnitIndex - service.StartFailoverUnitIndex + 1 == service.PartitionCount,
        "Service partition index wrong {0} {1}", service.StartFailoverUnitIndex, service.EndFailoverUnitIndex);

        if (itService == services.end())
        {
            TestSession::WriteInfo(Utility::TraceSource, "Service {0} doesn't exist", service.Index);
            return false;
        }

        if (itService->second != service.PartitionCount)
        {
            TestSession::WriteInfo(Utility::TraceSource, "Service partition count doesn't match: actual:{0}, expected:{1}",
            itService->second, service.PartitionCount);
            return false;
        }
    }

    return true;
}

bool FM::VerifyStable()
{
    auto domains = GetPLBObject()->GetServiceDomains();
    wstring result;
    for (size_t i = 0; i < domains.size(); ++i)
    {
        if (domains[i].schedulerAction_.Action != Reliability::LoadBalancingComponent::PLBSchedulerActionType::NoActionNeeded)
        {
            if (!result.empty())
            {
                result.append(L" ");
            }

            result.append(wformatString("{0}:{1}",
                domains[i].domainId_,
                domains[i].schedulerAction_));
        }
    }

    if (result.empty())
    {
        return true;
    }
    else
    {
        TestSession::WriteInfo(Utility::TraceSource, "Domain states: {0}", result);
        return false;
    }
}

bool FM::FailoverUnitsStable()
{
    bool ret = true;
    for each (auto const& pair in failoverUnits_)
    {
        FailoverUnit const& failoverUnit = pair.second;
        int replicaSize = static_cast<int>(failoverUnit.Replicas.size());

        if (failoverUnit.TargetReplicaCount != replicaSize)
        {
            ret = false;
            TestSession::WriteInfo(Utility::TraceSource,
                "Error: Failover unit {0} of service {1} does not have enough replicas",
                failoverUnit.get_FailoverUnitId(),
                failoverUnit.get_ServiceIndex(),
                failoverUnit.TargetReplicaCount,
                replicaSize);
            break;
        }
    }
    return ret;
}

int FM::GetTotalMetricCount()
{
    int ret = 0;
    for (auto it = services_.begin(); it != services_.end(); it++)
    {
        ret += static_cast<int>(it->second.Metrics.size());
    }

    return ret;
}

void FM::InitiateAutoScaleServiceRepartitioning(std::wstring const& name, int change)
{
    UNREFERENCED_PARAMETER(name);
    UNREFERENCED_PARAMETER(change);
}
