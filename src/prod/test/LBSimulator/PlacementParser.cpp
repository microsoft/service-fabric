// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "FM.h"
#include "PlacementParser.h"
#include "Utility.h"

#include "Reliability/Failover/common/FailoverConfig.h"
#include "Reliability/LoadBalancing/Placement.h"
#include "Reliability/LoadBalancing/DiagnosticsDataStructures.h"
#include "Reliability/LoadBalancing/Metric.h"
#include "Reliability/LoadBalancing/SearcherSettings.h"
#include "Reliability/LoadBalancing/ServicePackagePlacement.h"


using namespace std;
using namespace Common;
using namespace LBSimulator;
using namespace Reliability::LoadBalancingComponent;

wstring const PlacementParser::SectionLBDomains = L"[LBDomains]";
wstring const PlacementParser::SectionNodes = L"[Nodes]";
wstring const PlacementParser::SectionServices = L"[Services]";
wstring const PlacementParser::SectionPartitions = L"[Partitions]";
int const PlacementParser::MaxReplicaCount = 1024 * 1024;

PlacementParser::PlacementParser(SearcherSettings const & settings)
    : replicaIndex_(0),
    settings_(settings),
    quorumBasedServicesCount_(0),
    quorumBasedPartitionsCount_(0)
{
}

void PlacementParser::Parse(wstring const& fileName)
{
    wstring fileTextW;
    Utility::LoadFile(fileName, fileTextW);

    replicaIndex_ = 0;

    allReplicas_.reserve(MaxReplicaCount);
    standByReplicas_.reserve(MaxReplicaCount);
    toBeDroppedReplicas_.reserve(MaxReplicaCount);

    size_t metricStartIndex = 0;
    int nodeIndex = 0;
    int serviceIndex = 0;

    int phase = -1;
    size_t pos = 0;
    while (pos != std::wstring::npos)
    {
        wstring lineBuf = Utility::ReadLine(fileTextW, pos);

        StringUtility::TrimWhitespaces(lineBuf);
        if (lineBuf.empty())
        {
            continue;
        }

        if (StringUtility::StartsWith(lineBuf, Utility::CommentInit))
        {
            continue;
        }

        if (StringUtility::StartsWithCaseInsensitive(lineBuf, PlacementParser::SectionLBDomains))
        {
            phase = 0;
        }
        else if (StringUtility::StartsWithCaseInsensitive(lineBuf, PlacementParser::SectionNodes))
        {
            CreateServiceToLBDomainTable();
            phase = 1;
        }
        else if (StringUtility::StartsWithCaseInsensitive(lineBuf, PlacementParser::SectionServices))
        {
            CreateNodeDict();
            phase = 2;
        }
        else if (StringUtility::StartsWithCaseInsensitive(lineBuf, PlacementParser::SectionPartitions))
        {
            CreateServiceDict();
            phase = 3;
        }
        else
        {
            switch (phase)
            {
            case 0:
                ParseOneLBDomain(lineBuf, metricStartIndex);
                break;
            case 1:
                ParseOneNode(lineBuf, nodeIndex++);
                break;
            case 2:
                ParseOneService(lineBuf, serviceIndex++);
                break;
            case 3:
                ParseOnePartition(lineBuf);
                break;
            default:
                break;
            }
        }
    }
}

void PlacementParser::TrimParenthese(std::wstring & text)
{
    if (text.size() >= 2 && text[0] == L'(' && text[text.size() - 1] == L')')
    {
        text = text.substr(1, text.size() - 2);
    }
}

void PlacementParser::ParseOneLBDomain(wstring const& text, size_t & metricStartIndex)
{
    StringCollection lbDomainFields = Utility::SplitWithParentheses(text, Utility::FieldDelimiter, Utility::EmptyCharacter);

    ASSERT_IFNOT(lbDomainFields.size() == 4, "Invalid lbDomain {0}", text);

    int serviceIndex = _wtoi(lbDomainFields[0].c_str());
    wstring metricStr = lbDomainFields[1];

    TrimParenthese(metricStr);

    vector<Metric> metrics;
    double weightSum = 0.0;

    StringCollection metricCollection = Utility::SplitWithParentheses(metricStr, Utility::FieldDelimiter, Utility::EmptyCharacter);

    for (size_t metricIndex = 0; metricIndex < metricCollection.size(); ++metricIndex)
    {

        StringCollection metricFields = Utility::SplitWithParentheses(metricCollection[metricIndex], Utility::PairDelimiter, Utility::EmptyCharacter);

        ASSERT_IFNOT(metricFields.size() == 5, "Invalid metric {0}", metricCollection[metricIndex]);

        wstring metricName = metricFields[0];
        double weight = _wtof(metricFields[1].c_str());
        weightSum += weight;
        double balancingThreshold = _wtof(metricFields[2].c_str());

        wstring blockListStr = metricFields[4];
        TrimParenthese(blockListStr);

        StringCollection blockListFields = Utility::Split(blockListStr, Utility::FieldDelimiter);
        set<Federation::NodeId> blockList;
        for (size_t i = 0; i < blockListFields.size(); ++i)
        {
            LargeInteger li;
            bool ret = LargeInteger::TryParse(blockListFields[i], li);
            ASSERT_IFNOT(ret, "Invalid node id {0}", blockListFields[i]);

            blockList.insert(Federation::NodeId(li));
        }

        int activityThreshold = _wtoi(metricFields[5].c_str());
        int clusterTotalCapacity = _wtoi(metricFields[6].c_str());
        int clusterBufferedCapacity = _wtoi(metricFields[7].c_str());
        int clusterLoad = _wtoi(metricFields[8].c_str());
        bool isDefrag = Utility::Bool_Parse(metricFields[9].c_str());

        Metric::PlacementStrategy placementStrategy = Metric::PlacementStrategy::Reservation;

        // TODO: Add support for defrag free UD/FD threshold in LB simulator.
        int defragNodeCount = 0;
        size_t defragEmptyNodeLoadThreashold = 0;
        int64 reservationLoad = 0;
        Metric::DefragDistributionType defragDistribution = Metric::DefragDistributionType::SpreadAcrossFDs_UDs;
        double incomingLoadFactor = 0.0;
        double emptySpacePercentage = 0.0;
        bool balanceByPercentage = false;

        existDefragMetric_ |= isDefrag;

        DynamicBitSet dynamicBlockList;

        for (auto const& nodeId : blockList)
        {
            if (nodeDict_.find(nodeId) != nodeDict_.end())
            {
                dynamicBlockList.Add(nodeDict_.find(nodeId)->second->NodeIndex);
            }
        }

        metrics.push_back(Metric(
            move(metricName),
            weight,
            balancingThreshold,
            move(dynamicBlockList),
            activityThreshold,
            clusterTotalCapacity,
            clusterBufferedCapacity,
            clusterLoad,
            isDefrag,
            defragNodeCount,
            defragEmptyNodeLoadThreashold,
            reservationLoad,
            defragDistribution,
            incomingLoadFactor,
            emptySpacePercentage,
            false,
            placementStrategy,
            0,
            0,
            balanceByPercentage));
    }

    lbDomainEntries_.push_back(LoadBalancingDomainEntry(move(metrics), weightSum, metricStartIndex, serviceIndex));
    metricStartIndex += metricCollection.size();
}

void PlacementParser::AddDomainId(TreeNodeIndex const& domainIndex, BalanceChecker::DomainTree & domainStructure)
{
    BalanceChecker::DomainTree::Node * currentNode = &(domainStructure.RootRef);
    ++currentNode->DataRef.NodeCount;

    for (size_t i = 0; i < domainIndex.Length; ++i)
    {
        size_t currentSegment = domainIndex.GetSegment(i);
        while (currentNode->Children.size() < currentSegment + 1 )
        {
            currentNode->ChildrenRef.push_back(BalanceChecker::DomainTree::Node(BalanceChecker::DomainData(0)));
        }

        currentNode = &(currentNode->ChildrenRef[currentSegment]);
        ++currentNode->DataRef.NodeCount;
    }
}

void PlacementParser::ParseOneNode(wstring const& text, int index)
{
    StringCollection nodeFields = Utility::SplitWithParentheses(text, Utility::FieldDelimiter, Utility::EmptyCharacter);
    ASSERT_IFNOT(nodeFields.size() == 6, "Invalid node {0}", text);

    Federation::NodeId nodeId;
    bool result = Federation::NodeId::TryParse(nodeFields[0], nodeId);

    ASSERT_IFNOT(result, "Invalid nodeId {0}", nodeFields[0]);

    TreeNodeIndex fdIndex;
    TrimParenthese(nodeFields[1]);

    StringCollection fdIndexFields = Utility::Split(nodeFields[1], Utility::FieldDelimiter);

    for (size_t j = 0; j < fdIndexFields.size(); ++j)
    {
        fdIndex.Append(static_cast<size_t>(_wtoi(fdIndexFields[j].c_str())));
    }

    TreeNodeIndex udIndex;
    TrimParenthese(nodeFields[2]);

    if (!nodeFields[2].empty())
    {
        udIndex.Append(static_cast<size_t>(_wtoi(nodeFields[2].c_str())));
    }

    AddDomainId(fdIndex, fdStructure_);
    AddDomainId(udIndex, udStructure_);

    vector<int64> loads;
    vector<int64> toBeDroppedLoads;
    TrimParenthese(nodeFields[3]);
    StringCollection loadFields = Utility::Split(nodeFields[3], Utility::FieldDelimiter);
    for (size_t i = 0; i < loadFields.size(); ++i)
    {
        loads.push_back(_wtoi(loadFields[i].c_str()));
        toBeDroppedLoads.push_back(0);
    }

    vector<int64> capacityRatios;
    TrimParenthese(nodeFields[4]);
    StringCollection capacityRatioFields = Utility::Split(nodeFields[4], Utility::FieldDelimiter);
    for (size_t i = 0; i < capacityRatioFields.size(); ++i)
    {
        capacityRatios.push_back(_wtoi(capacityRatioFields[i].c_str()));
    }

    vector<int64> totalCapacities;
    vector<int64> bufferedCapacities;
    TrimParenthese(nodeFields[5]);
    StringCollection capacityFields = Utility::Split(nodeFields[5], Utility::FieldDelimiter);
    for (size_t i = 0; i < capacityFields.size(); ++i)
    {
        totalCapacities.push_back(_wtoi(capacityFields[i].c_str()));
        bufferedCapacities.push_back(_wtoi(capacityFields[i].c_str()));
    }

    nodeEntries_.push_back(NodeEntry(
        index,
        nodeId,
        LoadEntry(move(loads)),
        LoadEntry(move(toBeDroppedLoads)),
        LoadEntry(move(capacityRatios)),
        LoadEntry(move(bufferedCapacities)),
        LoadEntry(move(totalCapacities)),
        move(fdIndex),
        move(udIndex),
        false,   // isDeactivated
        true));
}

void PlacementParser::ParseOneService(wstring const& text, int index)
{
    StringCollection serviceFields = Utility::SplitWithParentheses(text, Utility::FieldDelimiter, Utility::EmptyCharacter);
    ASSERT_IFNOT(serviceFields.size() == 9, "Invalid service {0}", text);

    vector<size_t> globalMetricIndices;
    TrimParenthese(serviceFields[4]);
    StringCollection globalMetricIndexFields = Utility::Split(serviceFields[4], Utility::FieldDelimiter);
    for (size_t i = 0; i < globalMetricIndexFields.size(); ++i)
    {
        globalMetricIndices.push_back(static_cast<size_t>(_wtoi(globalMetricIndexFields[i].c_str())));
    }

    Reliability::LoadBalancingComponent::Service::Type::Enum fdDistribution = Reliability::LoadBalancingComponent::Service::Type::Packing;
    bool partiallyPlace = true;

    if (serviceFields[9] == L"Packing")
    {
        fdDistribution = Reliability::LoadBalancingComponent::Service::Type::Packing;
    }
    else if (serviceFields[9] == L"Ignore")
    {
        fdDistribution = Reliability::LoadBalancingComponent::Service::Type::Ignore;
    }
    else if (serviceFields[9] == L"Nonpacking")
    {
        fdDistribution = Reliability::LoadBalancingComponent::Service::Type::Nonpacking;
    }
    else if (serviceFields[9] == L"Nonpartially")
    {
        partiallyPlace = false;
    }
    else if (serviceFields[9] == L"Partially")
    {
        partiallyPlace = true;
    }
    else
    {
        Assert::CodingError("Invalid FDDistribution type");
    }

    bool onEveryNode = Utility::Bool_Parse(serviceFields[8]);

    set<Federation::NodeId> nodeBlockList;
    TrimParenthese(serviceFields[6]);
    StringCollection blockListFields = Utility::Split(serviceFields[6], Utility::FieldDelimiter);
    for (size_t i = 0; i < blockListFields.size(); ++i)
    {

        Federation::NodeId nodeId;
        bool result = Federation::NodeId::TryParse(blockListFields[i], nodeId);

        ASSERT_IFNOT(result, "Invalid nodeId {0}", blockListFields[i]);
        nodeBlockList.insert(nodeId);
    }

    set<NodeEntry const*> blockList = ConstructBlockList(nodeBlockList, nodeDict_);

    set<Federation::NodeId> nodePrimaryBlockList;
    TrimParenthese(serviceFields[7]);
    StringCollection primaryBlockListFields = Utility::Split(serviceFields[7], Utility::FieldDelimiter);
    for (size_t i = 0; i < primaryBlockListFields.size(); ++i)
    {

        Federation::NodeId nodeId;
        bool result = Federation::NodeId::TryParse(primaryBlockListFields[i], nodeId);

        ASSERT_IFNOT(result, "Invalid nodeId {0}", primaryBlockListFields[i]);
        nodePrimaryBlockList.insert(nodeId);
    }

    set<NodeEntry const*> primaryBlockList = ConstructBlockList(nodePrimaryBlockList, nodeDict_);

    BalanceChecker::DomainTree faultDomainStructure =
        fdStructure_.IsEmpty || blockList.empty() || onEveryNode ||
        fdDistribution == Reliability::LoadBalancingComponent::Service::Type::Ignore ?
        BalanceChecker::DomainTree() :
        GetFilteredDomainTree(fdStructure_, blockList, true);

    BalanceChecker::DomainTree upgradeDomainStructure =
        udStructure_.IsEmpty || blockList.empty() || onEveryNode ?
        BalanceChecker::DomainTree() :
        GetFilteredDomainTree(udStructure_, blockList, false);
    
    bool autoSwitchToQuorumBasedLogic = Utility::Bool_Parse(serviceFields[14]);

    if (autoSwitchToQuorumBasedLogic ||
        settings_.QuorumBasedReplicaDistributionPerFaultDomains ||
        settings_.QuorumBasedReplicaDistributionPerUpgradeDomains)
    {
        quorumBasedServicesCount_++;
    }

    ServiceEntry serviceEntry(
        move(serviceFields[0]),
        nullptr,
        Utility::Bool_Parse(serviceFields[1]),
        Utility::Bool_Parse(serviceFields[2]),
        Utility::Bool_Parse(serviceFields[3]),
        CreateBitSetFromNodes(blockList),
        CreateBitSetFromNodes(primaryBlockList),
        0,
        move(globalMetricIndices),
        onEveryNode,
        fdDistribution,
        move(faultDomainStructure),
        move(upgradeDomainStructure),
        move(ServiceEntryDiagnosticsData(wstring())),
        partiallyPlace,
        nullptr,
        true,
        autoSwitchToQuorumBasedLogic);

    auto itLBDomain = serviceToLBDomainTable_.find(index);
    auto itGlobalDomain = serviceToLBDomainTable_.find(-1);
    ASSERT_IF(itGlobalDomain == serviceToLBDomainTable_.end(), "Global domain doesn't exist");

    if (itLBDomain != serviceToLBDomainTable_.end())
    {
        serviceEntry.SetLBDomain(itLBDomain->second, itGlobalDomain->second);
    }
    else
    {
        serviceEntry.SetLBDomain(nullptr, itGlobalDomain->second);
    }

    serviceEntries_.push_back(move(serviceEntry));
}

ReplicaRole::Enum PlacementParser::ReplicaRoleFromString(wstring const& roleStr)
{
    if (roleStr == L"Primary")
    {
        return ReplicaRole::Primary;
    }
    else if (roleStr == L"Secondary")
    {
        return ReplicaRole::Secondary;
    }
    else if (roleStr == L"None")
    {
        return ReplicaRole::None;
    }
    else
    {
        Assert::CodingError("Invalid replica role {0}", roleStr);
    }
}

void PlacementParser::ParseOnePartition(wstring const& text)
{
    StringCollection partitionFields = Utility::SplitWithParentheses(text, Utility::FieldDelimiter, Utility::EmptyCharacter);
    ASSERT_IFNOT(partitionFields.size() == 9, "Invalid partition {0}", text);

    int fieldIndex = 0;
    Guid partitionId(partitionFields[fieldIndex]);

    fieldIndex += 2;
    int version = _wtoi(partitionFields[fieldIndex++].c_str());
    bool isInTransition = Utility::Bool_Parse(partitionFields[fieldIndex++]);
    bool isMovable = Utility::Bool_Parse(partitionFields[fieldIndex++]);

    vector<int64> primaryLoad;
    TrimParenthese(partitionFields[fieldIndex]);
    StringCollection primaryLoadFields = Utility::Split(partitionFields[fieldIndex++], Utility::FieldDelimiter);
    for (size_t i = 0; i < primaryLoadFields.size(); ++i)
    {
        primaryLoad.push_back(static_cast<int64>(_wtoi(primaryLoadFields[i].c_str())));
    }

    vector<int64> secondaryLoad;
    TrimParenthese(partitionFields[fieldIndex]);
    StringCollection secondaryLoadFields = Utility::Split(partitionFields[fieldIndex++], Utility::FieldDelimiter);
    for (size_t i = 0; i < secondaryLoadFields.size(); ++i)
    {
        secondaryLoad.push_back(static_cast<int64>(_wtoi(secondaryLoadFields[i].c_str())));
    }

    vector<PlacementReplica*> replicas;

    TrimParenthese(partitionFields[fieldIndex]);
    StringCollection existingReplicaFields = Utility::Split(partitionFields[fieldIndex++], Utility::FieldDelimiter);
    for (size_t i = 0; i < existingReplicaFields.size(); ++i)
    {
        StringCollection oneReplicaFields = Utility::Split(existingReplicaFields[i], Utility::PairDelimiter);
        ASSERT_IFNOT(oneReplicaFields.size() == 3, "Invalid existing replica {0}", existingReplicaFields[i]);

        Federation::NodeId nodeId;
        bool ret = Federation::NodeId::TryParse(oneReplicaFields[0], nodeId);
        ASSERT_IFNOT(ret, "Invalid node id {0}", oneReplicaFields[0]);
        auto itNode = nodeDict_.find(nodeId);
        ASSERT_IF(itNode == nodeDict_.end(), "Node {0} doesn't exist", nodeId);

        ReplicaRole::Enum role = ReplicaRoleFromString(oneReplicaFields[1]);
        bool isReplicaMovable = Utility::Bool_Parse(oneReplicaFields[2]);

        allReplicas_.push_back(make_unique<PlacementReplica>(replicaIndex_++, role, isReplicaMovable, false, false, itNode->second, false, false, false, true, false));
        replicas.push_back(allReplicas_.back().get());
    }

    TrimParenthese(partitionFields[fieldIndex]);
    StringCollection newReplicaFields = Utility::Split(partitionFields[fieldIndex++], Utility::FieldDelimiter);
    for (size_t i = 0; i < newReplicaFields.size(); ++i)
    {

        ReplicaRole::Enum role = ReplicaRoleFromString(newReplicaFields[i]);

        allReplicas_.push_back(make_unique<PlacementReplica>(replicaIndex_++, role, false));
        replicas.push_back(allReplicas_.back().get());
    }

    ASSERT_IF(replicas.empty(), "No replica for this partition");

    // get standByLocations
    vector<NodeEntry const*> standByLocations;
    TrimParenthese(partitionFields[fieldIndex]);
    StringCollection standByLocationFields = Utility::Split(partitionFields[fieldIndex++], Utility::FieldDelimiter);
    for (size_t i = 0; i < standByLocationFields.size(); ++i)
    {
        Federation::NodeId nodeId;
        bool ret = Federation::NodeId::TryParse(standByLocationFields[i], nodeId);
        ASSERT_IFNOT(ret, "Invalid node id {0}", standByLocationFields[i]);
        auto itNode = nodeDict_.find(nodeId);
        ASSERT_IF(itNode == nodeDict_.end(), "Node {0} doesn't exist", nodeId);

        standByLocations.push_back(itNode->second);
        standByReplicas_.push_back(make_unique<PlacementReplica>(replicaIndex_++, ReplicaRole::Enum::StandBy, false, false, false, itNode->second, false, false, false, true, false));
    }

    fieldIndex = 1;
    // get service entry and set the affinity order
    auto itService = serviceDict_.find(partitionFields[fieldIndex]);
    ASSERT_IF(itService == serviceDict_.end(), "No such service entry for a partition entry");
    ServiceEntry const* serviceEntry = itService->second;
    int order = 0;

    if (serviceEntry->AutoSwitchToQuorumBasedLogic ||
        settings_.QuorumBasedReplicaDistributionPerFaultDomains ||
        settings_.QuorumBasedReplicaDistributionPerUpgradeDomains)
    {
        quorumBasedPartitionsCount_++;
    }

    ServiceEntry const* dependedServiceEntry = serviceEntry->DependedService;
    while (dependedServiceEntry != nullptr)
    {
        ++order;
        dependedServiceEntry = dependedServiceEntry->DependedService;
    }

    // Todo add in upgrade scenario simulation
    bool inUpgrade = false;
    bool fuInUpgrade = false;
    bool partitionInSingletonReplicaUpgrade = false;
    PartitionEntry::SingletonReplicaUpgradeOptimizationType upgradeOptimization = PartitionEntry::SingletonReplicaUpgradeOptimizationType::None;

    // TODO: Add scenario with extra replicas to be dropped
    int extraReplicaCount = 0;

    NodeEntry const* primarySwapOutLocation = nullptr;
    NodeEntry const* primaryUpgradeLocation = nullptr;
    vector<NodeEntry const*> secondaryUpgradeLocation;

    map<Federation::NodeId, vector<uint>> secondaryLoads;
    partitionEntries_.push_back(PartitionEntry(
        partitionId,
        version,
        isInTransition,
        isMovable,
        serviceEntry,
        move(replicas),
        LoadEntry(move(primaryLoad)),
        LoadEntry(move(secondaryLoad)),
        0,
        0,
        move(standByLocations),
        primarySwapOutLocation,
        primaryUpgradeLocation,
        move(secondaryUpgradeLocation),
        inUpgrade,
        fuInUpgrade,
        partitionInSingletonReplicaUpgrade,
        upgradeOptimization,
        order,
        extraReplicaCount,
        secondaryLoads,
        vector<PlacementReplica *> (),
        settings_,
        serviceEntry->TargetReplicaSetSize));
}

bool PlacementParser::Validate()
{
    return true;
}

PlacementUPtr PlacementParser::CreatePlacement()
{
    if (!Validate())
    {
        return nullptr;
    }
    else
    {
        BalancingDiagnosticsDataSPtr balancingDiagnosticsDataSPtr = std::make_shared<BalancingDiagnosticsData>();

        BalanceCheckerUPtr balanceChecker = make_unique<BalanceChecker>(
            move(fdStructure_),
            move(udStructure_),
            move(lbDomainEntries_),
            move(nodeEntries_),
            vector<int>(), // deactivatedNodes
            vector<int>(), // deactivatingNodesAllowPlacement
            vector<int>(), // deactivatingNodesAllowServiceOnEveryNode
            vector<int>(), // down nodes
            existDefragMetric_,
            false,
            balancingDiagnosticsDataSPtr,
            set<Common::TreeNodeIndex>(), // fabric upgraded UDs
            std::map<uint64, std::set<Common::TreeNodeIndex>>(),  // application upgraded UDs
            std::map<Common::TreeNodeIndex, std::vector<wstring>>(),
            std::map<Common::TreeNodeIndex, wstring>(),
            settings_,
            PartitionClosureType::Enum::Full,
            set<wstring>(), // Empty, but the ServiceEntry metricHasCapacity is always true
            map<TreeNodeIndex, NodeSet>(),
            map<TreeNodeIndex, NodeSet>()
            );

        PLBSchedulerAction action;
        bool hasUpgradePartitions = false;
        return make_unique<Placement>(
            move(balanceChecker),
            move(serviceEntries_),
            move(partitionEntries_),
            vector<ApplicationEntry>(),
            vector<ServicePackageEntry>(),
            move(allReplicas_),
            move(standByReplicas_),
            ApplicationReservedLoad(static_cast<size_t>(0)),
            InBuildCountPerNode(0),
            hasUpgradePartitions,
            false,// IsSingletonReplicaMoveAllowedDuringUpgrade
            0,
            action,
            PartitionClosureType::Enum::Full,
            set<Common::Guid>(),
            ServicePackagePlacement(),
            quorumBasedServicesCount_,
            quorumBasedPartitionsCount_,
            move(quorumBasedServicesTempCache_));
    }
}

// private functions

void PlacementParser::CreateServiceToLBDomainTable()
{
    for (auto it = lbDomainEntries_.begin(); it != lbDomainEntries_.end(); ++it)
    {
        serviceToLBDomainTable_.insert(make_pair(it->ServiceIndex, &*it));
    }
    fdStructure_.SetRoot(make_unique<BalanceChecker::DomainTree::Node>(BalanceChecker::DomainData(0)));
    udStructure_.SetRoot(make_unique<BalanceChecker::DomainTree::Node>(BalanceChecker::DomainData(0)));
}

void PlacementParser::CreateNodeDict()
{
    for (auto it = nodeEntries_.begin(); it != nodeEntries_.end(); ++it)
    {
        nodeDict_.insert(make_pair(it->NodeId, &*it));
    }
}

void PlacementParser::CreateServiceDict()
{
    for (auto it = serviceEntries_.begin(); it != serviceEntries_.end(); ++it)
    {
        serviceDict_.insert(make_pair(it->Name, &*it));
    }
}

set<NodeEntry const*> PlacementParser::ConstructBlockList(set<Federation::NodeId> const& blockList, map<Federation::NodeId, NodeEntry const*> const& nodeEntryDict)
{
    set<NodeEntry const*> list;
    for each (Federation::NodeId const& nodeId in blockList)
    {
        auto it = nodeEntryDict.find(nodeId);
        if (it != nodeEntryDict.end())
        {
            list.insert(it->second);
        }
    }
    return list;
}

BalanceChecker::DomainTree PlacementParser::GetFilteredDomainTree(BalanceChecker::DomainTree const& baseTree, std::set<NodeEntry const*> const& blockList, bool isFaultDomain)
{
    ASSERT_IF(baseTree.IsEmpty, "Base tree is empty");

    BalanceChecker::DomainTree tree(baseTree);

    for (auto it = blockList.begin(); it != blockList.end(); ++it)
    {
        TreeNodeIndex const& nodeIndex = isFaultDomain ? (*it)->FaultDomainIndex : (*it)->UpgradeDomainIndex;
        tree.ForEachNodeInPath(nodeIndex, [](BalanceChecker::DomainTree::Node & n)
        {
            ASSERT_IFNOT(n.Data.NodeCount > 0, "Node count should be greater than 0");
            --n.DataRef.NodeCount;
        });
    }

    return tree;
}

DynamicBitSet PlacementParser::CreateBitSetFromNodes(std::set<NodeEntry const*> nodeSet)
{
    DynamicBitSet bitSet;
    for (auto node : nodeSet)
    {
        bitSet.Add(node->NodeIndex);
    }
    return bitSet;
}
