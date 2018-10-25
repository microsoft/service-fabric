// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestUtility.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;
using namespace PlacementAndLoadBalancingUnitTest;


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

wstring const ItemDelimiter = L",";
wstring const PairDelimiter = L"/";

StringLiteral const PLBTestSource("PLBTest");

Federation::NodeId PlacementAndLoadBalancingUnitTest::CreateNodeId(int id)
{
    return Federation::NodeId(Common::LargeInteger(0, id));
}

Federation::NodeInstance PlacementAndLoadBalancingUnitTest::CreateNodeInstance(int id, uint64 instance)
{
    return Federation::NodeInstance(CreateNodeId(id), instance);
}

Guid PlacementAndLoadBalancingUnitTest::CreateGuid(int i)
{
    return Guid(i, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int PlacementAndLoadBalancingUnitTest::GetNodeId(Federation::NodeId nodeId)
{
    return static_cast<int>(nodeId.IdValue.Low);
}

int PlacementAndLoadBalancingUnitTest::GetFUId(Guid fuId)
{
    return static_cast<int>(fuId.AsGUID().Data1);
}

vector<wstring> PlacementAndLoadBalancingUnitTest::GetActionListString(map<Guid, FailoverUnitMovement> const& actionList)
{
    vector<wstring> ret;

    for (auto const& actionPair : actionList)
    {
        vector<FailoverUnitMovement::PLBAction> const& allActions = actionPair.second.Actions;
        auto itMovePrimary = find_if(allActions.begin(), allActions.end(), [](FailoverUnitMovement::PLBAction action)
        {
            return action.Action == FailoverUnitMovementType::MovePrimary;
        });

        for (FailoverUnitMovement::PLBAction currentAction : allActions)
        {
            wstring result;
            StringWriter w(result);

            w.Write("{0} ", GetFUId(actionPair.first));

            switch (currentAction.Action)
            {
            case FailoverUnitMovementType::MoveInstance:

                w.Write("move instance {0}=>{1}", GetNodeId(currentAction.SourceNode), GetNodeId(currentAction.TargetNode));

                break;

            case FailoverUnitMovementType::MoveSecondary:
                if (itMovePrimary != allActions.end() && 
                    currentAction.SourceNode == itMovePrimary->TargetNode && 
                    currentAction.TargetNode == itMovePrimary->SourceNode)
                {
                    w.Write("swap primary {0}<=>{1}", GetNodeId(itMovePrimary->SourceNode), GetNodeId(itMovePrimary->TargetNode));
                    itMovePrimary = allActions.end();
                }
                else
                {
                    w.Write("move secondary {0}=>{1}", GetNodeId(currentAction.SourceNode), GetNodeId(currentAction.TargetNode));
                }

                break;

            case FailoverUnitMovementType::MovePrimary:
                result.clear();
                break;

            case FailoverUnitMovementType::SwapPrimarySecondary:
                w.Write("swap primary {0}<=>{1}", GetNodeId(currentAction.SourceNode), GetNodeId(currentAction.TargetNode));
                break;

            case FailoverUnitMovementType::AddPrimary:
                w.Write("add primary {0}", GetNodeId(currentAction.TargetNode));
                break;

            case FailoverUnitMovementType::AddSecondary:
                w.Write("add secondary {0}", GetNodeId(currentAction.TargetNode));
                break;

            case FailoverUnitMovementType::AddInstance:
                w.Write("add instance {0}", GetNodeId(currentAction.TargetNode));
                break;

            case FailoverUnitMovementType::RequestedPlacementNotPossible:
                w.Write("void movement on {0}", GetNodeId(currentAction.SourceNode));
                break;

            case FailoverUnitMovementType::DropPrimary:
                w.Write("drop primary {0}", GetNodeId(currentAction.SourceNode));
                break;

            case FailoverUnitMovementType::DropSecondary:
                w.Write("drop secondary {0}", GetNodeId(currentAction.SourceNode));
                break;

            case FailoverUnitMovementType::DropInstance:
                w.Write("drop instance {0}", GetNodeId(currentAction.SourceNode));
                break;

            case FailoverUnitMovementType::PromoteSecondary:
                w.Write("promote secondary {0}", GetNodeId(currentAction.TargetNode));
                break;

            default:
                Assert::CodingError("Unknown Replica Move Type");
                break;
            }

            if (!result.empty())
            {
                ret.push_back(result);
            }
        }

        if (itMovePrimary != allActions.end())
        {
            wstring result;
            StringWriter w(result);

            w.Write("{0} ", GetFUId(actionPair.first));

            w.Write("move primary");
            w.Write(" {0}=>{1}", GetNodeId(itMovePrimary->SourceNode), GetNodeId(itMovePrimary->TargetNode));

            ret.push_back(result);
        }
    }

    Trace.WriteInfo(PLBTestSource, "{0}", ret);

    return ret;
}

size_t PlacementAndLoadBalancingUnitTest::GetActionRawCount(map<Guid, FailoverUnitMovement> const& actionList)
{
    size_t sum = 0;
    for (auto it = actionList.begin(); it != actionList.end(); ++it)
    {
        sum += it->second.Actions.size();
    }
    return sum;
}

bool PlacementAndLoadBalancingUnitTest::ActionMatch(wstring const& expected, wstring const& actual)
{
    vector<wstring> expectedList;
    vector<wstring> actualList;

    StringUtility::Split<wstring>(expected, expectedList, wstring(L" =<>"));
    StringUtility::Split<wstring>(actual, actualList, wstring(L" =<>"));

    if (expectedList.size() != actualList.size())
    {
        return false;
    }

    for (size_t i = 0; i < expectedList.size(); ++i)
    {
        if (!OneOfMatch(expectedList[i], actualList[i]) &&
            expectedList[i] != L"*")
        {
            return false;
        }
    }

    return true;
}

bool PlacementAndLoadBalancingUnitTest::OneOfMatch(std::wstring const& expected, std::wstring const& actual)
{
    vector<wstring> expectedCandidates;
    StringUtility::Split<wstring>(expected, expectedCandidates, wstring(L"|"));

    bool match = false;
    for (size_t i = 0; i < expectedCandidates.size(); ++i)
    {
        if (expectedCandidates[i] == actual)
        {
            match = true;
            break;
        }
    }

    return match;
}

vector<ReplicaDescription> PlacementAndLoadBalancingUnitTest::CreateReplicas(wstring replicas, std::vector<bool>& isReplicaUp)
{
    StringCollection replicaSet;
    vector<ReplicaDescription> replicaDescription;
    StringUtility::Split<wstring>(replicas, replicaSet, ItemDelimiter);

    if (replicaSet.size() != isReplicaUp.size())
    {
        return replicaDescription;
    }

    for (int i = 0; i < replicaSet.size(); i++)
    {
        replicaDescription.push_back(CreateReplicas(replicaSet[i], isReplicaUp[i])[0]);
    }

    return replicaDescription;
}

vector<ReplicaDescription> PlacementAndLoadBalancingUnitTest::CreateReplicas(wstring replicas, bool isUp)
{
    vector<ReplicaDescription> replicaVector;

    // format [replica1], [replica2], ...
    // each replica: [replicaRole]/[nodeIndex]/[IsToBePromoted][IsMoveInProgress][IsToBeDropped][IsInBuild]
    // replicaRole: P or S or SB
    // the last three descriptions optional inputs: PDB; treated as false by default
    StringCollection placement;
    StringUtility::Split<wstring>(replicas, placement, ItemDelimiter);

    for (wstring const& replica : placement)
    {
        StringCollection replicaFields;
        StringUtility::Split<wstring>(replica, replicaFields, PairDelimiter);

        ReplicaRole::Enum role = ReplicaRole::None;
        Reliability::ReplicaStates::Enum state = Reliability::ReplicaStates::Ready;
        StringUtility::TrimSpaces(replicaFields[0]);

        if (replicaFields[0] == L"P")
        {
            role = ReplicaRole::Primary;
        }
        else if (replicaFields[0] == L"S" || replicaFields[0] == L"I")
        {
            role = ReplicaRole::Secondary;
        }
        else if (replicaFields[0] == L"N")
        {
            role = ReplicaRole::None;
        }
        else if (replicaFields[0] == L"SB")
        {
            role = ReplicaRole::StandBy;
            state = Reliability::ReplicaStates::StandBy;
        }
        else if (replicaFields[0] == L"D")
        {
            role = ReplicaRole::Dropped;
            state = Reliability::ReplicaStates::Dropped;
        }

        int nodeIndex = _wtoi(replicaFields[1].c_str());
        Reliability::ReplicaFlags::Enum flags = Reliability::ReplicaFlags::None;

        if (replicaFields.size() >= 3)
        {
            wstring properties = replica.substr(replicaFields[0].length() + replicaFields[1].length() + 2);

            if (StringUtility::Contains<wstring>(properties, L"B"))
            {
                state = Reliability::ReplicaStates::InBuild;
            }

            if (StringUtility::Contains<wstring>(properties, L"I"))
            {
                flags = static_cast<Reliability::ReplicaFlags::Enum>(flags ^ Reliability::ReplicaFlags::PrimaryToBeSwappedOut);
            }

            if (StringUtility::Contains<wstring>(properties, L"J"))
            {
                flags = static_cast<Reliability::ReplicaFlags::Enum>(flags ^ Reliability::ReplicaFlags::PrimaryToBePlaced);
            }

            if (StringUtility::Contains<wstring>(properties, L"K"))
            {
                flags = static_cast<Reliability::ReplicaFlags::Enum>(flags ^ Reliability::ReplicaFlags::ReplicaToBePlaced);
            }

            if (StringUtility::Contains<wstring>(properties, L"V"))
            {
                flags = static_cast<Reliability::ReplicaFlags::Enum>(flags ^ Reliability::ReplicaFlags::MoveInProgress);
            }

            if (StringUtility::Contains<wstring>(properties, L"D"))
            {
                flags = static_cast<Reliability::ReplicaFlags::Enum>(flags ^ Reliability::ReplicaFlags::ToBeDroppedByFM);
            }

            if (StringUtility::Contains<wstring>(properties, L"R"))
            {
                flags = static_cast<Reliability::ReplicaFlags::Enum>(flags ^ Reliability::ReplicaFlags::ToBeDroppedByPLB);
            }

            if (StringUtility::Contains<wstring>(properties, L"T"))
            {
                flags = static_cast<Reliability::ReplicaFlags::Enum>(flags ^ Reliability::ReplicaFlags::ToBeDroppedForNodeDeactivation);
            }

            if (StringUtility::Contains<wstring>(properties, L"P"))
            {
                flags = static_cast<Reliability::ReplicaFlags::Enum>(flags ^ Reliability::ReplicaFlags::ToBePromoted);
            }

            if (StringUtility::Contains<wstring>(properties, L"N"))
            {
                flags = static_cast<Reliability::ReplicaFlags::Enum>(flags ^ Reliability::ReplicaFlags::PendingRemove);
            }

            if (StringUtility::Contains<wstring>(properties, L"Z"))
            {
                flags = static_cast<Reliability::ReplicaFlags::Enum>(flags ^ Reliability::ReplicaFlags::Deleted);
            }

            if (StringUtility::Contains<wstring>(properties, L"L"))
            {
                flags = static_cast<Reliability::ReplicaFlags::Enum>(flags ^ Reliability::ReplicaFlags::PreferredPrimaryLocation);
            }

            if (StringUtility::Contains<wstring>(properties, L"E"))
            {
                flags = static_cast<Reliability::ReplicaFlags::Enum>(flags ^ Reliability::ReplicaFlags::EndpointAvailable);
            }

            if (StringUtility::Contains<wstring>(properties, L"M"))
            {
                flags = static_cast<Reliability::ReplicaFlags::Enum>(flags ^ Reliability::ReplicaFlags::PreferredReplicaLocation);
            }
        }

        replicaVector.push_back(ReplicaDescription(
            CreateNodeInstance(nodeIndex, 0),
            role,
            state,
            isUp,   // isUp
            flags));
    }

    return replicaVector;
}

vector<ServiceMetric> PlacementAndLoadBalancingUnitTest::CreateMetrics(wstring metrics)
{
    StringCollection metricCollection;
    StringUtility::Split<wstring>(metrics, metricCollection, ItemDelimiter);

    vector<ServiceMetric> metricList;
    for (wstring const& metricStr : metricCollection)
    {
        StringCollection metricProperties;
        StringUtility::Split<wstring>(metricStr, metricProperties, PairDelimiter);

        ASSERT_IFNOT(metricProperties.size() == 4, "Metric error");

        metricList.push_back(ServiceMetric(move(metricProperties[0]), Common::Double_Parse(metricProperties[1]), _wtoi(metricProperties[2].c_str()), _wtoi(metricProperties[3].c_str())));
    }

    return metricList;
}

map<wstring, uint> PlacementAndLoadBalancingUnitTest::CreateCapacities(std::wstring capacityStr)
{
    map<wstring, uint> capacities;
    if (!capacityStr.empty())
    {
        StringCollection capacityCollection;
        StringUtility::Split<wstring>(capacityStr, capacityCollection, ItemDelimiter);
        for (int i = 0; i < capacityCollection.size(); ++i)
        {
            StringCollection capacityPair;
            StringUtility::Split<wstring>(capacityCollection[i], capacityPair, PairDelimiter);

            ASSERT_IFNOT(capacityPair.size() == 2, "Capacity error");
            capacities.insert(make_pair(capacityPair[0], _wtoi(capacityPair[1].c_str())));
        }
    }

    return capacities;
}

map<wstring, ApplicationCapacitiesDescription> PlacementAndLoadBalancingUnitTest::CreateApplicationCapacities(wstring appCapacities)
{
    map<std::wstring, ApplicationCapacitiesDescription> capacityDesc;

    StringCollection capacityCollection;
    StringUtility::Split<wstring>(appCapacities, capacityCollection, ItemDelimiter);
    for (wstring const& metricStr : capacityCollection)
    {
        StringCollection metricProperties;
        StringUtility::Split<wstring>(metricStr, metricProperties, PairDelimiter);

        ASSERT_IFNOT(metricProperties.size() == 4, "CreateApplicationCapacities metric error");

        wstring metricName = metricProperties[0];
        capacityDesc.insert(make_pair(metricName,
            ApplicationCapacitiesDescription(move(metricProperties[0]), _wtoi(metricProperties[1].c_str()), _wtoi(metricProperties[2].c_str()), _wtoi(metricProperties[3].c_str()))));
    }

    return capacityDesc;
}

LoadOrMoveCostDescription PlacementAndLoadBalancingUnitTest::CreateLoadOrMoveCost(
    int failoverUnitId, 
    wstring serviceName, 
    wstring metricName, 
    uint primaryLoad, 
    uint secondaryLoad)
{
    vector<LoadMetric> primaryEntries, secondaryEntries;
    primaryEntries.push_back(LoadMetric(wstring(metricName), primaryLoad));
    secondaryEntries.push_back(LoadMetric(move(metricName), secondaryLoad));
    LoadOrMoveCostDescription load(CreateGuid(failoverUnitId), move(serviceName), true);
    StopwatchTime now = Stopwatch::Now();
    load.MergeLoads(ReplicaRole::Primary, move(primaryEntries), now);
    load.MergeLoads(ReplicaRole::Secondary, move(secondaryEntries), now, false);
    return load;
}

LoadOrMoveCostDescription PlacementAndLoadBalancingUnitTest::CreateLoadOrMoveCost(
    int failoverUnitId, 
    wstring serviceName, 
    wstring metricName, 
    uint instanceLoad)
{
    vector<LoadMetric> entries;
    entries.push_back(LoadMetric(move(metricName), instanceLoad));
    LoadOrMoveCostDescription load(CreateGuid(failoverUnitId), move(serviceName), false);
    StopwatchTime now = Stopwatch::Now();
    load.MergeLoads(ReplicaRole::Secondary, move(entries), now);
    return load;
}

LoadOrMoveCostDescription PlacementAndLoadBalancingUnitTest::CreateLoadOrMoveCost(
    int failoverUnitId,
    std::wstring serviceName,
    std::wstring metricName,
    uint primaryLoad,
    std::map<Federation::NodeId, uint> const& secondaryLoads,
    bool isStateful)
{
    vector<LoadMetric> primaryEntries;
    primaryEntries.push_back(LoadMetric(wstring(metricName), primaryLoad));
    LoadOrMoveCostDescription load(CreateGuid(failoverUnitId), move(serviceName), isStateful);
    StopwatchTime now = Stopwatch::Now();
    if (isStateful)
    {
        load.MergeLoads(ReplicaRole::Primary, move(primaryEntries), now);
    }

    for (auto sIt = secondaryLoads.begin(); sIt != secondaryLoads.end(); sIt++)
    {
        vector<LoadMetric> secondaryEntries;
        secondaryEntries.push_back(LoadMetric(wstring(metricName), sIt->second));
        load.MergeLoads(ReplicaRole::Secondary, move(secondaryEntries), now, true, sIt->first);
    }

    return load;

}

NodeDescription PlacementAndLoadBalancingUnitTest::CreateNodeDescription(
    int nodeId,
    wstring fdPath,
    wstring ud,
    map<wstring, wstring> && nodeProperties,
    std::wstring capacityStr,
    bool isUp)
{
    NodeDescription::DomainId fdId = fdPath.empty() ? 
        NodeDescription::DomainId() : Uri(*Reliability::LoadBalancingComponent::Constants::FaultDomainIdScheme, L"", fdPath).Segments;

    //add fdPath and ud as implicit properties
    map<wstring, wstring> nodePropertiesWithImplicitProperties(nodeProperties);

    if (!fdPath.empty())
    {
        Uri fdUri = Uri(*Reliability::LoadBalancingComponent::Constants::FaultDomainIdScheme, L"", fdPath);
        nodePropertiesWithImplicitProperties.insert(make_pair(*Reliability::LoadBalancingComponent::Constants::ImplicitFaultDomainId, fdUri.ToString()));
    }

    if (!ud.empty())
    {
        nodePropertiesWithImplicitProperties.insert(make_pair(*Reliability::LoadBalancingComponent::Constants::ImplicitUpgradeDomainId, ud));
    }

    return NodeDescription(
        CreateNodeInstance(nodeId, 0),
        isUp,
        Reliability::NodeDeactivationIntent::Enum::None,
        Reliability::NodeDeactivationStatus::Enum::None,
        move(nodePropertiesWithImplicitProperties),
        move(fdId),
        move(ud),
        map<wstring, uint>(),
        CreateCapacities(capacityStr));
}

NodeDescription PlacementAndLoadBalancingUnitTest::CreateNodeDescriptionWithCapacity(
    int nodeId, 
    wstring capacityStr,
    bool isUp)
{
    return NodeDescription(CreateNodeInstance(nodeId, 0), isUp, Reliability::NodeDeactivationIntent::Enum::None, Reliability::NodeDeactivationStatus::Enum::None,
        map<wstring, wstring>(), NodeDescription::DomainId(), wstring(), map<wstring, uint>(), CreateCapacities(capacityStr));
}

Reliability::LoadBalancingComponent::NodeDescription PlacementAndLoadBalancingUnitTest::CreateNodeDescriptionWithResources(int nodeId,
    uint cpuCores,
    uint memoryInMegaBytes,
    std::map<std::wstring, std::wstring> && nodeProperties,
    std::wstring capacityStr,
    std::wstring ud,
    std::wstring fdPath,
    bool isUp)
{
    NodeDescription::DomainId fdId = fdPath.empty() ?
        NodeDescription::DomainId() : Uri(*Reliability::LoadBalancingComponent::Constants::FaultDomainIdScheme, L"", fdPath).Segments;

    std::map<wstring, uint> capacities = CreateCapacities(capacityStr);
    capacities.insert(make_pair(ServiceModel::Constants::SystemMetricNameCpuCores, cpuCores));
    capacities.insert(make_pair(ServiceModel::Constants::SystemMetricNameMemoryInMB, memoryInMegaBytes));

    return NodeDescription(CreateNodeInstance(nodeId, 0),
        isUp,
        Reliability::NodeDeactivationIntent::Enum::None,
        Reliability::NodeDeactivationStatus::Enum::None,
        move(nodeProperties),
        move(fdId),
        wstring(ud),
        map<wstring, uint>(),
        move(capacities));
}

NodeDescription PlacementAndLoadBalancingUnitTest::CreateNodeDescriptionWithPlacementConstraintAndCapacity(
    int nodeId,
    wstring capacityStr,
    map<wstring, wstring> && nodeProperties)
{
    map<wstring, wstring> nodePropertiesWithImplicitProperties(nodeProperties);

    return NodeDescription(CreateNodeInstance(nodeId, 0), true, Reliability::NodeDeactivationIntent::Enum::None, Reliability::NodeDeactivationStatus::Enum::None,
        move(nodePropertiesWithImplicitProperties), NodeDescription::DomainId(), wstring(), map<wstring, uint>(), CreateCapacities(capacityStr));
}

NodeDescription PlacementAndLoadBalancingUnitTest::CreateNodeDescriptionWithFaultDomainAndCapacity(
    int nodeId, 
    wstring fdPath, 
    wstring capacityStr,
    Reliability::NodeDeactivationIntent::Enum deactivationIntent,
    Reliability::NodeDeactivationStatus::Enum deactivationStatus
    )
{
    NodeDescription::DomainId fdId = fdPath.empty() ? 
        NodeDescription::DomainId() : Uri(*Reliability::LoadBalancingComponent::Constants::FaultDomainIdScheme, L"", fdPath).Segments;

    //add fdPath and ud as implicit properties
    map<wstring, wstring> nodePropertiesWithImplicitProperties;

    if (!fdPath.empty())
    {
        Uri fdUri = Uri(*Reliability::LoadBalancingComponent::Constants::FaultDomainIdScheme, L"", fdPath);
        nodePropertiesWithImplicitProperties.insert(make_pair(*Reliability::LoadBalancingComponent::Constants::ImplicitFaultDomainId, fdUri.ToString()));
    }

    return NodeDescription(CreateNodeInstance(nodeId, 0), true, deactivationIntent, deactivationStatus,
        move(nodePropertiesWithImplicitProperties), move(fdId), wstring(), map<wstring, uint>(), CreateCapacities(capacityStr));
}

NodeDescription PlacementAndLoadBalancingUnitTest::CreateNodeDescriptionWithDomainsAndCapacity(
    int nodeId,
    std::wstring fdPath,
    std::wstring ud,
    std::wstring capacityStr,
    Reliability::NodeDeactivationIntent::Enum deactivationIntent,
    Reliability::NodeDeactivationStatus::Enum deactivationStatus
    )
{
    NodeDescription::DomainId fdId = fdPath.empty() ?
        NodeDescription::DomainId() : Uri(*Reliability::LoadBalancingComponent::Constants::FaultDomainIdScheme, L"", fdPath).Segments;

    map<wstring, uint> capacities = CreateCapacities(capacityStr);

    //add fdPath and ud as implicit properties
    map<wstring, wstring> nodePropertiesWithImplicitProperties;

    if (!fdPath.empty())
    {
        Uri fdUri = Uri(*Reliability::LoadBalancingComponent::Constants::FaultDomainIdScheme, L"", fdPath);
        nodePropertiesWithImplicitProperties.insert(make_pair(*Reliability::LoadBalancingComponent::Constants::ImplicitFaultDomainId, fdUri.ToString()));
    }

    return NodeDescription(CreateNodeInstance(nodeId, 0), true, deactivationIntent, deactivationStatus,
        move(nodePropertiesWithImplicitProperties), move(fdId), move(ud), map<wstring, uint>(), move(capacities));
}

ServicePackageDescription PlacementAndLoadBalancingUnitTest::CreateServicePackageDescription(
    ServiceModel::ServicePackageIdentifier servicePackageIdentifier,
    double cpuCores,
    uint memoryInBytes,
    std::vector<std::wstring>&& codePackages)
{
    map<std::wstring, double> requiredResources;

    if (cpuCores != 0 || memoryInBytes != 0)
    {
        requiredResources.insert(make_pair(ServiceModel::Constants::SystemMetricNameCpuCores, cpuCores));
        requiredResources.insert(make_pair(ServiceModel::Constants::SystemMetricNameMemoryInMB, memoryInBytes));
    }

    return ServicePackageDescription(ServiceModel::ServicePackageIdentifier(servicePackageIdentifier), move(requiredResources), move(codePackages));
}

ServicePackageDescription PlacementAndLoadBalancingUnitTest::CreateServicePackageDescription(
    std::wstring servicePackageName,
    std::wstring appTypeName,
    std::wstring applicationName,
    double cpuCores,
    uint memoryInBytes,
    ServiceModel::ServicePackageIdentifier & spId,
    std::vector<std::wstring>&& codePackages)
{
    spId = ServiceModel::ServicePackageIdentifier(ServiceModel::ApplicationIdentifier(appTypeName, 0), servicePackageName);

    return CreateServicePackageDescription(ServiceModel::ServicePackageIdentifier(spId), cpuCores, memoryInBytes, move(codePackages));
}

ServiceDescription PlacementAndLoadBalancingUnitTest::CreateServiceDescriptionWithServicePackage(
    wstring serviceName,
    wstring serviceTypeName,
    bool isStateful,
    ServiceModel::ServicePackageIdentifier & spId,
    std::vector<Reliability::LoadBalancingComponent::ServiceMetric> && metrics,
    bool isSharedServicePackage,
    wstring appName,
    wstring placementConstraint)
{
    auto servicePackageKind = ServiceModel::ServicePackageActivationMode::SharedProcess;
    if (!isSharedServicePackage)
    {
        servicePackageKind = ServiceModel::ServicePackageActivationMode::ExclusiveProcess;
    }
    if (appName == L"" && PLBConfig::GetConfig().UseAppGroupsInBoost)
    {
        return ServiceDescription(
            move(wstring(serviceName)),
            move(wstring(serviceTypeName)),
            wstring(L"fabric:/NastyApplication314"),
            isStateful,
            wstring(placementConstraint),
            wstring(L""),
            true,
            move(metrics),
            FABRIC_MOVE_COST_LOW,
            false,
            0,
            4,
            true,
            ServiceModel::ServicePackageIdentifier(spId),
            servicePackageKind);
    }
    else
    {
        return ServiceDescription(
            move(wstring(serviceName)),
            move(wstring(serviceTypeName)),
            wstring(appName),
            isStateful,
            wstring(placementConstraint),
            wstring(L""),
            true,
            move(metrics),
            FABRIC_MOVE_COST_LOW,
            false,
            0,
            4,
            true,
            ServiceModel::ServicePackageIdentifier(spId),
            servicePackageKind);
    }
}

ServiceDescription PlacementAndLoadBalancingUnitTest::CreateServiceDescription(
    wstring serviceName,
    wstring serviceTypeName,
    bool isStateful,
    vector<ServiceMetric> && metrics,
    uint defaultMoveCost,
    bool onEveryNode,
    int targetReplicaSetSize,
    bool hasPersistedState,
    ServiceModel::ServicePackageIdentifier spId,
    bool isShared)
{
    auto servicePackageKind = ServiceModel::ServicePackageActivationMode::SharedProcess;
    if (!isShared)
    {
        servicePackageKind = ServiceModel::ServicePackageActivationMode::ExclusiveProcess;
    }
    if (PLBConfig::GetConfig().UseAppGroupsInBoost)
    {
        return ServiceDescription(
            move(serviceName),
            move(serviceTypeName),
            wstring(L"fabric:/NastyApplication314"),
            isStateful,
            wstring(L""),
            wstring(L""),
            true,
            move(metrics),
            defaultMoveCost,
            onEveryNode,
            0,
            targetReplicaSetSize,
            hasPersistedState,
            ServiceModel::ServicePackageIdentifier(spId),
            servicePackageKind);
    }
    else
    {
        return ServiceDescription(
            move(serviceName),
            move(serviceTypeName),
            wstring(L""),
            isStateful,
            wstring(L""),
            wstring(L""),
            true,
            move(metrics),
            defaultMoveCost,
            onEveryNode,
            0,
            targetReplicaSetSize,
            hasPersistedState,
            ServiceModel::ServicePackageIdentifier(spId),
            servicePackageKind);
    }
}

ServiceDescription PlacementAndLoadBalancingUnitTest::CreateServiceDescriptionWithPartitionAndReplicaCount(wstring serviceName, wstring serviceTypeName, bool isStateful, int partitionCount, int replicaCount, vector<ServiceMetric> && metrics)
{
    if (PLBConfig::GetConfig().UseAppGroupsInBoost)
    {
        return ServiceDescription(move(serviceName), move(serviceTypeName), wstring(L"fabric:/NastyApplication314"), isStateful, wstring(L""), wstring(L""), true, move(metrics), FABRIC_MOVE_COST_LOW, false, partitionCount, replicaCount);
    }
    else
    {
        return ServiceDescription(move(serviceName), move(serviceTypeName), wstring(L""), isStateful, wstring(L""), wstring(L""), true, move(metrics), FABRIC_MOVE_COST_LOW, false, partitionCount, replicaCount);
    }
}

ServiceDescription PlacementAndLoadBalancingUnitTest::CreateServiceDescriptionWithAffinity(
    wstring serviceName,
    wstring serviceTypeName,
    bool isStateful,
    wstring affinitizedService,
    vector<ServiceMetric> && metrics,
    bool onEveryNode,
    int targetReplicaSetSize,
    bool hasPersistedState,
    bool allignedAffinity,
    ServiceModel::ServicePackageIdentifier spId,
    bool isShared)
{
    auto servicePackageKind = ServiceModel::ServicePackageActivationMode::SharedProcess;
    if (!isShared)
    {
        servicePackageKind = ServiceModel::ServicePackageActivationMode::ExclusiveProcess;
    }
    if (PLBConfig::GetConfig().UseAppGroupsInBoost)
    {
        return ServiceDescription(
            move(serviceName),
            move(serviceTypeName),
            wstring(L"fabric:/NastyApplication314"),
            isStateful,
            wstring(L""),
            move(affinitizedService),
            allignedAffinity,
            move(metrics),
            FABRIC_MOVE_COST_LOW,
            onEveryNode,
            0,
            targetReplicaSetSize,
            hasPersistedState,
            ServiceModel::ServicePackageIdentifier(spId),
            servicePackageKind);
    }
    else
    {
        return ServiceDescription(
            move(serviceName),
            move(serviceTypeName),
            wstring(L""),
            isStateful,
            wstring(L""),
            move(affinitizedService),
            allignedAffinity,
            move(metrics),
            FABRIC_MOVE_COST_LOW,
            onEveryNode,
            0,
            targetReplicaSetSize,
            hasPersistedState,
            ServiceModel::ServicePackageIdentifier(spId),
            servicePackageKind);
    }
}

ServiceDescription PlacementAndLoadBalancingUnitTest::CreateServiceDescriptionWithAffinityWithEmptyApplication(wstring serviceName, wstring serviceTypeName, bool isStateful, wstring affinitizedService, vector<ServiceMetric> && metrics)
{
    return ServiceDescription(move(serviceName), move(serviceTypeName), wstring(L""), isStateful, wstring(L""), move(affinitizedService), true, move(metrics), FABRIC_MOVE_COST_LOW, false);
}
ServiceDescription PlacementAndLoadBalancingUnitTest::CreateServiceDescriptionWithAffinityAndApplication(wstring serviceName, wstring serviceTypeName, bool isStateful,wstring applicationName, wstring affinitizedService, vector<ServiceMetric> && metrics)
{
    return ServiceDescription(move(serviceName), move(serviceTypeName), move(applicationName), isStateful, wstring(L""), move(affinitizedService), true, move(metrics), FABRIC_MOVE_COST_LOW, false);
}

ServiceDescription PlacementAndLoadBalancingUnitTest::CreateServiceDescriptionWithNonAlignedAffinity(
    wstring serviceName,
    wstring serviceTypeName,
    bool isStateful,
    wstring affinitizedService,
    vector<ServiceMetric> && metrics)
{
    if (PLBConfig::GetConfig().UseAppGroupsInBoost)
    {
        return ServiceDescription(move(serviceName), move(serviceTypeName), wstring(L"fabric:/NastyApplication314"), isStateful, wstring(L""), move(affinitizedService), false, move(metrics), FABRIC_MOVE_COST_LOW, false);
    }
    else
    {
        return ServiceDescription(move(serviceName), move(serviceTypeName), wstring(L""), isStateful, wstring(L""), move(affinitizedService), false, move(metrics), FABRIC_MOVE_COST_LOW, false);
    }
}

size_t PlacementAndLoadBalancingUnitTest::NumberOfDomains(vector<set<wstring>> const& metricNameSetVector)
{
    if (metricNameSetVector.size() < 2)
    {
        return metricNameSetVector.size();
    }
    set<wstring> metrics;
    set<wstring> applicationMetrics;
    map<wstring, int> metricDomainMap;
    map<int, set<wstring>> domainMetricMap;
    int currentNewDomain = 0;
    for (auto metricSet : metricNameSetVector)
    {
        set<int> domainIdsToBeMerged;
        //add metrics in that domain
        set<wstring> allMetricsToMerge(metricSet.begin(), metricSet.end());

        for (auto metricName : metricSet)
        {
            metrics.insert(metricName);
            if (metricDomainMap.find(metricName) != metricDomainMap.end())
            {
                domainIdsToBeMerged.insert(metricDomainMap[metricName]);
            }
        }

        // add all metrics to related domain if any
        for (int domainId : domainIdsToBeMerged)
        {
            auto domainMetricSet = domainMetricMap[domainId];
            allMetricsToMerge.insert(domainMetricSet.begin(), domainMetricSet.end());
            domainMetricMap.erase(domainMetricMap.find(domainId));
        }

        // update all metrics to a new domain
        for (auto metricName : allMetricsToMerge)
        {
            metricDomainMap[metricName] = currentNewDomain;
        }

        if (!allMetricsToMerge.empty())
        {
            domainMetricMap[currentNewDomain] = move(allMetricsToMerge);
        }
        currentNewDomain++;
    }

    return domainMetricMap.size();
}

ServiceDescription PlacementAndLoadBalancingUnitTest::CreateServiceDescriptionWithConstraint(
    wstring serviceName,
    wstring serviceTypeName,
    bool isStateful,
    wstring placementConstraint,
    vector<ServiceMetric> && metrics,
    bool isOnEveryNode,
    int targetReplicaSetSize)
{
    if (PLBConfig::GetConfig().UseAppGroupsInBoost)
    {
        return ServiceDescription(
            move(serviceName),
            move(serviceTypeName),
            wstring(L"fabric:/NastyApplication314"),
            isStateful,
            move(placementConstraint),
            wstring(L""),
            true,
            move(metrics),
            FABRIC_MOVE_COST_LOW,
            isOnEveryNode,
            0,
            targetReplicaSetSize,
            true);
    }
    else
    {
        return ServiceDescription(
            move(serviceName),
            move(serviceTypeName),
            wstring(L""),
            isStateful,
            move(placementConstraint),
            wstring(L""),
            true,
            move(metrics),
            FABRIC_MOVE_COST_LOW,
            isOnEveryNode,
            0,
            targetReplicaSetSize,
            true);
    }
}

ServiceDescription PlacementAndLoadBalancingUnitTest::CreateServiceDescriptionWithApplication(
    wstring serviceName,
    wstring serviceTypeName,
    wstring applicationName,
    bool isStateful,
    vector<ServiceMetric> && metrics,
    ServiceModel::ServicePackageIdentifier servicePackageIdentifier,
    bool isShared,
    int partitionCount)
{
    auto servicePackageKind = ServiceModel::ServicePackageActivationMode::SharedProcess;
    if (!isShared)
    {
        servicePackageKind = ServiceModel::ServicePackageActivationMode::ExclusiveProcess;
    }
    return ServiceDescription(move(serviceName), move(serviceTypeName), move(applicationName), isStateful, wstring(L""), wstring(L""),
        true, move(metrics), FABRIC_MOVE_COST_LOW, false, partitionCount, 0, true,
        ServiceModel::ServicePackageIdentifier(servicePackageIdentifier), servicePackageKind);
}

ServiceDescription PlacementAndLoadBalancingUnitTest::CreateServiceDescriptionWithEmptyApplication(
wstring serviceName,
wstring serviceTypeName,
bool isStateful,
vector<ServiceMetric> && metrics)
{
    return ServiceDescription(move(serviceName), move(serviceTypeName), wstring(L""), isStateful, wstring(L""), wstring(L""), true, move(metrics), FABRIC_MOVE_COST_LOW, false);
}

vector<Reliability::ServiceScalingPolicyDescription> PlacementAndLoadBalancingUnitTest::CreateAutoScalingPolicyForPartition(
    wstring metricName,
    double lowerLoadThreshold,
    double upperLoadThreshold,
    uint scaleIntervalInSeconds,
    long minInstanceCount,
    long maxInstanceCount,
    long scaleIncrement)
{
    vector<Reliability::ServiceScalingPolicyDescription> result;
    Reliability::ScalingMechanismSPtr mechanism = make_shared<Reliability::InstanceCountScalingMechanism>(minInstanceCount, maxInstanceCount, scaleIncrement);
    Reliability::ScalingTriggerSPtr trigger = make_shared<Reliability::AveragePartitionLoadScalingTrigger>(metricName, lowerLoadThreshold, upperLoadThreshold, scaleIntervalInSeconds);
    result.push_back(Reliability::ServiceScalingPolicyDescription(trigger, mechanism));
    return result;
}

vector<Reliability::ServiceScalingPolicyDescription> PlacementAndLoadBalancingUnitTest::CreateAutoScalingPolicyForService(
    wstring metricName,
    double lowerLoadThreshold,
    double upperLoadThreshold,
    uint scaleIntervalInSeconds,
    long minPartitionCount,
    long maxPartitionCount,
    long scaleIncrement,
    bool useOnlyPrimaryLoad)
{
    vector<Reliability::ServiceScalingPolicyDescription> result;
    Reliability::ScalingMechanismSPtr mechanism = make_shared<Reliability::AddRemoveIncrementalNamedPartitionScalingMechanism>(minPartitionCount, maxPartitionCount, scaleIncrement);
    Reliability::ScalingTriggerSPtr trigger = make_shared<Reliability::AverageServiceLoadScalingTrigger>(metricName, lowerLoadThreshold, upperLoadThreshold, scaleIntervalInSeconds, useOnlyPrimaryLoad);
    result.push_back(Reliability::ServiceScalingPolicyDescription(trigger, mechanism));
    return result;
}

ServiceDescription PlacementAndLoadBalancingUnitTest::CreateServiceDescriptionWithAutoscalingPolicy(
    wstring serviceName,
    wstring serviceTypeName,
    bool isStateful,
    int targetReplicaSetSize,
    vector<Reliability::ServiceScalingPolicyDescription> && scalingPolicies,
    vector<ServiceMetric> && metrics,
    int partitionCount)
{
    if (PLBConfig::GetConfig().UseAppGroupsInBoost)
    {
        return ServiceDescription(
            move(serviceName),
            move(serviceTypeName),
            wstring(L"fabric:/NastyApplication314"),
            isStateful,
            wstring(L""),
            wstring(L""),
            true,
            move(metrics),
            FABRIC_MOVE_COST_LOW,
            false,
            partitionCount,
            targetReplicaSetSize,
            true,
            ServiceModel::ServicePackageIdentifier(),
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess,
            0,
            move(scalingPolicies));
    }
    else
    {
        return ServiceDescription(
            move(serviceName),
            move(serviceTypeName),
            wstring(L""),
            isStateful,
            wstring(L""),
            wstring(L""),
            true,
            move(metrics),
            FABRIC_MOVE_COST_LOW,
            false,
            partitionCount,
            targetReplicaSetSize,
            true,
            ServiceModel::ServicePackageIdentifier(),
            ServiceModel::ServicePackageActivationMode::ExclusiveProcess,
            0,
            move(scalingPolicies));
    }
}

ApplicationDescription PlacementAndLoadBalancingUnitTest::CreateApplicationDescriptionWithCapacities(
    wstring applicationName,
    int scaleoutCount,
    map<std::wstring, ApplicationCapacitiesDescription> && capacities,
    std::vector<ServicePackageDescription> servicePackages,
    std::wstring appTypeName)
{
    map<ServiceModel::ServicePackageIdentifier, ServicePackageDescription> packageMap;

    ServiceModel::ApplicationIdentifier appId(appTypeName, 0);
    for (auto const& sp : servicePackages)
    {
        ServiceModel::ServicePackageIdentifier spId(appId, sp.Name);
        packageMap.insert(make_pair(spId, sp));
    }

    return ApplicationDescription(move(applicationName), move(capacities), 0, scaleoutCount, move(packageMap));
}

ApplicationDescription PlacementAndLoadBalancingUnitTest::CreateApplicationDescriptionWithCapacities(
    wstring applicationName,
    int minimalCount,
    int scaleoutCount,
    map<std::wstring, ApplicationCapacitiesDescription> && capacities,
    bool isUpgradeInProgress,
    std::set<std::wstring> && completedUDs)
{
    return ApplicationDescription(
        move(applicationName),
        move(capacities),
        minimalCount,
        scaleoutCount,
        move(map<ServiceModel::ServicePackageIdentifier, ServicePackageDescription>()),
        move(map<ServiceModel::ServicePackageIdentifier, ServicePackageDescription>()),
        isUpgradeInProgress,
        move(completedUDs));
}

ApplicationDescription PlacementAndLoadBalancingUnitTest::CreateApplicationWithServicePackages(
    std::wstring appTypeName,
    std::wstring appName,
    std::vector<ServicePackageDescription> servicePackages,
    std::vector<ServicePackageDescription> servicePackagesUpgrading)
{
    map<ServiceModel::ServicePackageIdentifier, ServicePackageDescription> packageMap1;

    ServiceModel::ApplicationIdentifier appId(appTypeName, 0);
    for (auto const& sp : servicePackages)
    {
        ServiceModel::ServicePackageIdentifier spId(appId, sp.Name);
        packageMap1.insert(make_pair(spId, sp));
    }

    map<ServiceModel::ServicePackageIdentifier, ServicePackageDescription> packageMap2;
    for (auto const& sp : servicePackagesUpgrading)
    {
        ServiceModel::ServicePackageIdentifier spId(appId, sp.Name);
        packageMap2.insert(make_pair(spId, sp));
    }

    bool isUpgrading = false;
    if (packageMap2.size() > 0)
    {
        isUpgrading = true;
    }
    return ApplicationDescription(move(appName), map<std::wstring, ApplicationCapacitiesDescription>(), 0, 0, move(packageMap1), move(packageMap2), isUpgrading, std::set<wstring> (), appId);
}

ApplicationDescription PlacementAndLoadBalancingUnitTest::CreateApplicationWithServicePackagesAndCapacity(
    std::wstring appTypeName,
    std::wstring appName,
    int minimumCount,
    int scaleoutCount,
    map<std::wstring, ApplicationCapacitiesDescription> && capacities,
    std::vector<ServicePackageDescription> servicePackages,
    std::vector<ServicePackageDescription> servicePackagesUpgrading)
{
    map<ServiceModel::ServicePackageIdentifier, ServicePackageDescription> packageMap1;

    ServiceModel::ApplicationIdentifier appId(appTypeName, 0);
    for (auto const& sp : servicePackages)
    {
        ServiceModel::ServicePackageIdentifier spId(appId, sp.Name);
        packageMap1.insert(make_pair(spId, sp));
    }

    map<ServiceModel::ServicePackageIdentifier, ServicePackageDescription> packageMap2;
    for (auto const& sp : servicePackagesUpgrading)
    {
        ServiceModel::ServicePackageIdentifier spId(appId, sp.Name);
        packageMap2.insert(make_pair(spId, sp));
    }

    bool isUpgrading = false;
    if (packageMap2.size() > 0)
    {
        isUpgrading = true;
    }
    return ApplicationDescription(move(appName), move(capacities), minimumCount, scaleoutCount, move(packageMap1), move(packageMap2), isUpgrading, std::set<wstring>(), appId);
}

void PlacementAndLoadBalancingUnitTest::VerifyPLBAction(Reliability::LoadBalancingComponent::PlacementAndLoadBalancing & plb, std::wstring const& action, std::wstring const & metricName)
{
    ServiceModel::ClusterLoadInformationQueryResult loadInformationResult;
    ErrorCode retValue = plb.GetClusterLoadInformationQueryResult(loadInformationResult);
    VERIFY_ARE_EQUAL(ErrorCodeValue::Success, retValue.ReadValue());
    for (auto loadMetricInfo : loadInformationResult.LoadMetric)
    {
        if (loadMetricInfo.Name == metricName || metricName == L"")
        {
            if (loadMetricInfo.Action == action) return;
        }
    }
    VERIFY_FAIL(wformatString("No Match for action {0}", action).c_str());
}

void PlacementAndLoadBalancingUnitTest::GetClusterLoadMetricInformation(
    Reliability::LoadBalancingComponent::PlacementAndLoadBalancing & plb,
    ServiceModel::LoadMetricInformation& loadMetricInfo,
    std::wstring metricName)
{
    ServiceModel::ClusterLoadInformationQueryResult queryResult;
    ErrorCode result = plb.GetClusterLoadInformationQueryResult(queryResult);
    VERIFY_ARE_EQUAL(ErrorCodeValue::Success, result.ReadValue());

    for (auto itMetric = queryResult.LoadMetric.begin(); itMetric != queryResult.LoadMetric.end(); ++itMetric)
    {
        if (itMetric->Name == metricName)
        {
            loadMetricInfo = *itMetric;
            return;
        }
    }

    VERIFY_FAIL(L"Metric is not present in the result, fail the test");
}

void PlacementAndLoadBalancingUnitTest::GetNodeLoadMetricInformation(
    Reliability::LoadBalancingComponent::PlacementAndLoadBalancing & plb,
    ServiceModel::NodeLoadMetricInformation& nodeLoadMetricInfo,
    int nodeIndex,
    std::wstring const& metricName)
{
    ServiceModel::NodeLoadInformationQueryResult queryResult;
    ErrorCode retValue = plb.GetNodeLoadInformationQueryResult(CreateNodeId(nodeIndex), queryResult);
    VERIFY_ARE_EQUAL(ErrorCodeValue::Success, retValue.ReadValue());

    for (auto itMetric = queryResult.NodeLoadMetricInformation.begin(); itMetric != queryResult.NodeLoadMetricInformation.end(); ++itMetric)
    {
        if (itMetric->Name == metricName)
        {
            nodeLoadMetricInfo =  *itMetric;
            return;
        }
    }

    VERIFY_FAIL(L"Metric is not present in the result, fail the test");
}

void PlacementAndLoadBalancingUnitTest::VerifyCapacity(
    ServiceModel::ClusterLoadInformationQueryResult & queryResult,
    std::wstring metricName,
    uint expectedValue)
{
    for (auto itMetric = queryResult.LoadMetric.begin(); itMetric != queryResult.LoadMetric.end(); ++itMetric)
    {
        if (itMetric->Name == metricName)
        {
            VERIFY_ARE_EQUAL(itMetric->ClusterCapacity, expectedValue);
            return;
        }
    }

    VERIFY_FAIL(L"Metric is not present in the result, fail the test");
}

void PlacementAndLoadBalancingUnitTest::VerifyLoad(
    ServiceModel::ClusterLoadInformationQueryResult & queryResult,
    std::wstring metricName,
    uint expectedValue)
{
    for (auto itMetric = queryResult.LoadMetric.begin(); itMetric != queryResult.LoadMetric.end(); ++itMetric)
    {
        if (itMetric->Name == metricName)
        {
            VERIFY_ARE_EQUAL(itMetric->ClusterLoad, expectedValue);
            return;
        }
    }

    VERIFY_FAIL(L"Metric is not present in the result, fail the test");
}

void PlacementAndLoadBalancingUnitTest::VerifyLoad(
    ServiceModel::ClusterLoadInformationQueryResult & queryResult,
    std::wstring metricName,
    uint expectedValue,
    bool expectedIsCapacityViolation)
{
    for (auto itMetric : queryResult.LoadMetric)
    {
        if (itMetric.Name == metricName)
        {
            VERIFY_ARE_EQUAL(itMetric.ClusterLoad, expectedValue);
            VERIFY_ARE_EQUAL(itMetric.IsClusterCapacityViolation, expectedIsCapacityViolation);
            return;
        }
    }

    VERIFY_FAIL(L"Metric is not present in the result, fail the test");
}

void PlacementAndLoadBalancingUnitTest::VerifyClusterLoadDouble(
    ServiceModel::ClusterLoadInformationQueryResult & queryResult,
    std::wstring metricName,
    uint expectedValue,
    double expectedValueD,
    uint expectedCapacity,
    uint expectedRemaining,
    double expectedRemainingD,
    bool expectedIsCapacityViolation)
{
    for (auto itMetric : queryResult.LoadMetric)
    {
        if (itMetric.Name == metricName)
        {
            VERIFY_ARE_EQUAL(itMetric.ClusterLoad, expectedValue);
            VERIFY_ARE_EQUAL(itMetric.RemainingUnbufferedCapacity, expectedRemaining);
            VERIFY_ARE_EQUAL(itMetric.ClusterCapacity, expectedCapacity);
            VERIFY_ARE_EQUAL(itMetric.IsClusterCapacityViolation, expectedIsCapacityViolation);
            double diff = abs(expectedValueD - itMetric.CurrentClusterLoad);
            VERIFY_IS_TRUE(diff < 0.000001);
            diff = abs(expectedRemainingD - itMetric.ClusterCapacityRemaining);
            VERIFY_IS_TRUE(diff < 0.000001);
            return;
        }
    }

    VERIFY_FAIL(L"Metric is not present in the result, fail the test");
}

void PlacementAndLoadBalancingUnitTest::VerifyNodeLoadQuery(
    Reliability::LoadBalancingComponent::PlacementAndLoadBalancing & plb,
    int nodeIndex,
    std::wstring const& metricName,
    std::vector<uint> const & expectedValues,
    bool metricMustBePresent)
{
    ServiceModel::NodeLoadInformationQueryResult queryResult;
    ErrorCode retValue = plb.GetNodeLoadInformationQueryResult(CreateNodeId(nodeIndex), queryResult);
    VERIFY_ARE_EQUAL(ErrorCodeValue::Success, retValue.ReadValue());

    for (auto itMetric = queryResult.NodeLoadMetricInformation.begin(); itMetric != queryResult.NodeLoadMetricInformation.end(); ++itMetric)
    {
        if (itMetric->Name == metricName)
        {
            for (uint expected : expectedValues)
            {
                if (itMetric->NodeLoad == expected)
                {
                    return;
                }
            }
            VERIFY_FAIL(wformatString("Metric load for metric {0} ({1}) on node {2} does not match any of the expected values ({3})",
                metricName,
                itMetric->NodeLoad,
                nodeIndex,
                expectedValues
                ).c_str());
        }
    }
    if (metricMustBePresent)
    {
        VERIFY_FAIL(wformatString("Metric {0} is not present in the result for node {1}",
            metricName,
            nodeIndex).c_str());
    }
}

void PlacementAndLoadBalancingUnitTest::VerifyNodeLoadQuery(
    Reliability::LoadBalancingComponent::PlacementAndLoadBalancing & plb,
    int nodeIndex,
    std::wstring const& metricName,
    uint expectedValue,
    bool metricMustBePresent)
{
    std::vector<uint> expectedValues;
    expectedValues.push_back(expectedValue);
    VerifyNodeLoadQuery(plb, nodeIndex, metricName, expectedValues, metricMustBePresent);
}

void PlacementAndLoadBalancingUnitTest::VerifyNodeLoadQueryWithDoubleValues(
    Reliability::LoadBalancingComponent::PlacementAndLoadBalancing & plb,
    int nodeIndex,
    std::wstring const& metricName,
    uint expectedValue,
    double expectedValueD,
    uint remainingCapacity,
    double remainingCapacityD)
{
    ServiceModel::NodeLoadInformationQueryResult queryResult;
    ErrorCode retValue = plb.GetNodeLoadInformationQueryResult(CreateNodeId(nodeIndex), queryResult);
    VERIFY_ARE_EQUAL(ErrorCodeValue::Success, retValue.ReadValue());

    for (auto itMetric = queryResult.NodeLoadMetricInformation.begin(); itMetric != queryResult.NodeLoadMetricInformation.end(); ++itMetric)
    {
        if (itMetric->Name == metricName)
        {
            VERIFY_ARE_EQUAL(itMetric->NodeLoad, expectedValue);
            VERIFY_ARE_EQUAL(itMetric->NodeRemainingCapacity, remainingCapacity);
            double diff = abs(itMetric->CurrentNodeLoad - expectedValueD);
            VERIFY_IS_TRUE(diff < 0.000001);
            diff = abs(itMetric->NodeCapacityRemaining - remainingCapacityD);
            VERIFY_IS_TRUE(diff < 0.000001);
            return;
        }
    }
    VERIFY_FAIL(wformatString("Metric {0} is not present in the result for node {1}",
        metricName,
        nodeIndex).c_str());
}

void PlacementAndLoadBalancingUnitTest::VerifyNodeCapacity(
    Reliability::LoadBalancingComponent::PlacementAndLoadBalancing & plb,
    int nodeIndex,
    std::wstring const& metricName,
    uint expectedValue)
{
    ServiceModel::NodeLoadInformationQueryResult queryResult;
    ErrorCode retValue = plb.GetNodeLoadInformationQueryResult(CreateNodeId(nodeIndex), queryResult);
    VERIFY_ARE_EQUAL(ErrorCodeValue::Success, retValue.ReadValue());

    for (auto itMetric = queryResult.NodeLoadMetricInformation.begin(); itMetric != queryResult.NodeLoadMetricInformation.end(); ++itMetric)
    {
        if (itMetric->Name == metricName)
        {
            if (itMetric->NodeCapacity == expectedValue)
            {
                return;
            }

            VERIFY_FAIL(wformatString("Metric capacity for metric {0} ({1}) on node {2} does not match the expected value ({3})",
                metricName,
                itMetric->NodeLoad,
                nodeIndex,
                expectedValue
            ).c_str());
        }
    }

    VERIFY_FAIL(wformatString("Metric {0} is not present in the result for node {1}",
        metricName,
        nodeIndex).c_str());

}

void PlacementAndLoadBalancingUnitTest::VerifyApplicationLoadQuery(
    ServiceModel::ApplicationLoadInformationQueryResult queryResult,
    std::wstring metricName,
    uint expectedValue)
{
    for (auto metricResult : queryResult.ApplicationLoadMetricInformation)
    {
        if (metricResult.Name == metricName)
        {
            VERIFY_ARE_EQUAL(metricResult.ApplicationLoad, expectedValue);
            return;
        }
    }
    VERIFY_FAIL(L"Metric is not present in the result, fail the test");
}
