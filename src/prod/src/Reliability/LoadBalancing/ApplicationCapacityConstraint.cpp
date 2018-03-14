// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Checker.h"
#include "ApplicationCapacityConstraint.h"
#include "NodeCapacityConstraint.h"
#include "NodeBlockListConstraint.h"
#include "TempSolution.h"
#include "NodeEntry.h"
#include "NodeSet.h"
#include "IViolation.h"
#include "PlacementReplicaSet.h"
#include "Node.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

void ApplicationCapacitySubspace::GetTargetNodes(
    TempSolution const& tempSolution,
    PlacementReplica const* replica,
    NodeSet & candidateNodes,
    bool useNodeBufferCapacity,
    NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr /* = nullptr */) const
{
    std::vector<PlacementReplica const*> replicas;
    replicas.push_back(replica);
    GetTargetNodesForReplicas(tempSolution, replicas, candidateNodes, useNodeBufferCapacity, nodeToConstraintDiagnosticsDataMapSPtr);
}

void ApplicationCapacitySubspace::GetTargetNodesForReplicas(
    TempSolution const& tempSolution,
    std::vector<PlacementReplica const*> const& replicas,
    NodeSet & candidateNodes,
    bool useNodeBufferCapacity,
    NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr /* = nullptr */) const
{
    UNREFERENCED_PARAMETER(useNodeBufferCapacity);
    UNREFERENCED_PARAMETER(nodeToConstraintDiagnosticsDataMapSPtr);

    ASSERT_IF(replicas.size() == 0, "Replica empty.");

    std::map<ApplicationEntry const*, std::vector<PlacementReplica const*>> applicationToReplicasMap;

    for (auto replica : replicas)
    {
        ApplicationEntry const* application = replica->Partition->Service->Application;

        // Only include applications that have app or per-node capacity
        if (application && (application->HasAppCapacity || application->HasPerNodeCapacity))
        {
            applicationToReplicasMap[application].push_back(replica);
        }
    }

    for (auto applicationsReplicas : applicationToReplicasMap)
    {
        ApplicationEntry const* application = applicationsReplicas.first;
        NodeEntry const* currentNode = applicationsReplicas.second.size() == 1 ? tempSolution.GetReplicaLocation(applicationsReplicas.second[0]) : nullptr;

        size_t globalMetricCount = tempSolution.OriginalPlacement->GlobalMetricCount;
        size_t totalMetricCount = tempSolution.OriginalPlacement->TotalMetricCount;
        size_t globalMetricStartIndex = totalMetricCount - globalMetricCount;
        LoadEntry newReplicaLoad(globalMetricCount);
        bool hasNewReplica = false;

        for (auto replica : applicationsReplicas.second)
        {
            if (!replica->IsNew)
            {
                continue;
            }
            hasNewReplica = true;
            ServiceEntry const* replicaService = replica->Partition->Service;

            for (size_t metricIndex = 0; metricIndex < replicaService->MetricCount; ++metricIndex)
            {
                ASSERT_IFNOT(metricIndex < replica->ReplicaEntry->Length,
                    "Metric index should be < replica entry size: {0} {1}",
                    metricIndex,
                    replica->ReplicaEntry->Length);

                size_t globalMetricIndex = replicaService->GlobalMetricIndices[metricIndex];

                int64 loadValue(0);
                loadValue = replica->ReplicaEntry->Values[metricIndex];
                newReplicaLoad.AddLoad(globalMetricIndex - globalMetricStartIndex, loadValue);
            }
        }

        if (hasNewReplica && application->HasAppCapacity && !application->TotalLoads.Values.empty())
        {
            LoadEntry appTotalLoad = tempSolution.ApplicationTotalLoads[application];

            // Check if application total load is over capacity
            for (size_t metricIndex = 0; metricIndex < globalMetricCount; ++metricIndex)
            {
                int64 loadValue = newReplicaLoad.Values[metricIndex];
                int64 capacity = application->AppCapacities.Values[metricIndex];

                if (loadValue > 0 && capacity >= 0 && appTotalLoad.Values[metricIndex] + loadValue > capacity)
                {
                    // If we have a new replica and there is a standby replica,
                    // which load (for the current metric) is enough to promote it, without violating application capacity,
                    // then nodes containing such a standby replica should be a part of candidate nodes list.
                    if (applicationsReplicas.second.size() == 1 &&
                        applicationsReplicas.second[0]->Partition->StandByLocations.size() > 0)
                    {
                        PartitionEntry const* curPartition = applicationsReplicas.second[0]->Partition;
                        NodeSet candidateNodesSnapshot(candidateNodes);
                        candidateNodes.Clear();
                        for(auto nodeEntry : curPartition->StandByLocations)
                        {
                            if (candidateNodesSnapshot.Check(nodeEntry))
                            {
                                LoadEntry const& localReplicaLoad = curPartition->GetReplicaEntry(ReplicaRole::Enum::StandBy, true /* useNodeId */, nodeEntry->NodeId);
                                int64 standByReplicaLoad = PlacementReplica::GetReplicaLoadValue(curPartition, localReplicaLoad, metricIndex, globalMetricStartIndex);
                                if ((appTotalLoad.Values[metricIndex] + loadValue - standByReplicaLoad <= capacity))
                                {
                                    candidateNodes.Add(nodeEntry->NodeIndex);
                                }
                            }
                        }
                    }
                    // After singleton replica upgrade, stateful persisted service replica
                    // may need to be returned to the original node, but there are now StandBy replicas.
                    // In order not to go over application capacity (even above doubled app capacity),
                    // loads from this replicas should be subtracted from total application load,
                    // as all standby replicas will be substituted by new secondary replicas
                    else if (applicationsReplicas.second.size() > 1 &&
                            application->HasPartitionsInSingletonReplicaUpgrade &&
                            tempSolution.OriginalPlacement->Settings.RelaxScaleoutConstraintDuringUpgrade)
                    {
                        int64 totalStandByReplicaLoad = 0;
                        NodeEntry const* curStandByLocation = nullptr;
                        NodeEntry const* commonStandByLocation = nullptr;
                        NodeSet candidateNodesSnapshot(candidateNodes);
                        candidateNodes.Clear();

                        for (auto replica : applicationsReplicas.second)
                        {
                            if (replica->Partition->Service->IsStateful &&
                                replica->Partition->StandByLocations.size() == 1 &&
                                replica->Partition->StandByLocations[0] != nullptr)
                            {
                                curStandByLocation = replica->Partition->StandByLocations[0];

                                // If there are two nodes with standby replicas, then application is in violation
                                // and there is no candidate node to places all required replicas
                                if (commonStandByLocation != nullptr && commonStandByLocation != curStandByLocation)
                                {
                                    commonStandByLocation = nullptr;
                                    break;
                                }
                                else if (!commonStandByLocation)
                                {
                                    commonStandByLocation = curStandByLocation;
                                }

                                LoadEntry const& standByReplicaLoad = replica->Partition->GetReplicaEntry(
                                    ReplicaRole::Enum::StandBy,
                                    true,
                                    commonStandByLocation->NodeId);
                                totalStandByReplicaLoad += PlacementReplica::GetReplicaLoadValue(replica->Partition, standByReplicaLoad, metricIndex, globalMetricStartIndex);
                            }
                        }
                        if (commonStandByLocation &&
                            (appTotalLoad.Values[metricIndex] + loadValue - totalStandByReplicaLoad <= capacity) &&
                            candidateNodesSnapshot.Check(commonStandByLocation))
                        {
                            candidateNodes.Add(commonStandByLocation->NodeIndex);
                        }
                    }
                    else
                    {
                        candidateNodes.Clear();
                        return;
                    }
                }
            }
        }

        // Check for per node capacity
        candidateNodes.Filter([&](NodeEntry const *node) -> bool
        {
            // Has no node capacity and application per node capacity
            if (!application || (!application->HasAppCapacity && !application->HasPerNodeCapacity))
            {
                return true;
            }
            else if (node == currentNode)
            {
                auto itNode = nodeExtraLoads_.find(currentNode);
                if (itNode != nodeExtraLoads_.end())
                {
                    auto itReplica = itNode->second.find(applicationsReplicas.second[0]);
                    if (itReplica != itNode->second.end())
                    {
                        // if the replica is one of invalid replicas on this node
                        return false;
                    }
                }

                return true;
            }
            else
            {
                return FilterCandidateNodesForCapacityConstraints(node, tempSolution, applicationsReplicas.second, false, true);
            }
        });
    }
}

bool ApplicationCapacitySubspace::FilterByNodeCapacity(
    NodeEntry const* node,
    TempSolution const& tempSolution,
    std::vector<PlacementReplica const*> const& replicas,
    int64 replicaLoadValue,
    bool useNodeBufferCapacity,
    size_t capacityIndex,
    size_t globalMetricIndex,
    IConstraintDiagnosticsDataSPtr const constraintDiagnosticsDataSPtr /* = nullptr */) const
{
    UNREFERENCED_PARAMETER(useNodeBufferCapacity);
    UNREFERENCED_PARAMETER(constraintDiagnosticsDataSPtr);
    UNREFERENCED_PARAMETER(globalMetricIndex);

    // All replicas in the array will belong to the same application
    if (replicas.size() == 0)
    {
        return true;
    }
    PlacementReplica const* replica = replicas[0];

    ApplicationEntry const* application = replica->Partition->Service->Application;
    if (!application)
    {
        return true;
    }

    int64 baseMetricLoadOnNode = ((NodeMetrics const&)tempSolution.BaseSolution.ApplicationNodeLoad[application])[node].Values[capacityIndex]; 
    int64 applicationMetricLoadOnNode = ((NodeMetrics const&)tempSolution.ApplicationNodeLoad[application])[node].Values[capacityIndex];

    int64 capacity = application->PerNodeCapacities.Values[capacityIndex];
    if (capacity >= 0)
    {
        if (relaxed_)
        {
            capacity = max(capacity, baseMetricLoadOnNode);
        }

        if (replicaLoadValue > 0 &&
            application->GetDisappearingMetricNodeLoad(node, capacityIndex) +
            applicationMetricLoadOnNode +
            replicaLoadValue > capacity)
        {
            return false;
        }
    }

    return true;
}

void ApplicationCapacitySubspace::PrimarySwapback(TempSolution const& tempSolution,
    PartitionEntry const* partition, NodeSet & candidateNodes) const
{
    PromoteSecondary(tempSolution, partition, candidateNodes);
}

bool ApplicationCapacitySubspace::PromoteSecondary(
    TempSolution const& tempSolution,
    PartitionEntry const* partition,
    NodeSet & candidateNodes) const
{
    if (partition == nullptr)
    {
        return false;
    }

    std::vector<PartitionEntry const*> partitions;
    partitions.push_back(partition);

    PromoteSecondaryForPartitions(tempSolution, partitions, candidateNodes);

    return false;
}

void ApplicationCapacitySubspace::PromoteSecondaryForPartitions(TempSolution const& tempSolution,
    std::vector<PartitionEntry const*> const& partitions, NodeSet & candidateNodes) const
{
    size_t globalMetricCount = tempSolution.OriginalPlacement->GlobalMetricCount;
    size_t globalMetricStartIndex = tempSolution.OriginalPlacement->TotalMetricCount - globalMetricCount;

    candidateNodes.Filter([&](NodeEntry const *node) -> bool
    {
        if (!node->IsUp)
        {
            return false;
        }
        unordered_map<ApplicationEntry const*, LoadEntry> applicationLoadDiffs;

        for (auto pIt = partitions.begin(); pIt != partitions.end(); ++pIt)
        {
            auto partition = *pIt;
            auto application = partition->Service->Application;


            if (application && application->HasPerNodeCapacity)
            {
                LoadEntry const& primaryEntry = partition->GetReplicaEntry(ReplicaRole::Primary);
                LoadEntry const& secondaryEntry = partition->GetReplicaEntry(ReplicaRole::Secondary, true, node->NodeId);
                LoadEntry swapLoadDiffEntry(globalMetricCount);

                for (size_t metricIndex = 0; metricIndex < partition->Service->MetricCount; metricIndex++)
                {
                    size_t globalMetricIndex = partition->Service->GlobalMetricIndices[metricIndex];
                    size_t capacityIndex = globalMetricIndex - globalMetricStartIndex;

                    int64 swapLoadDiffValue = primaryEntry.Values[metricIndex] - secondaryEntry.Values[metricIndex];

                    swapLoadDiffEntry.AddLoad(capacityIndex, swapLoadDiffValue);
                }

                auto itApplication = applicationLoadDiffs.find(application);

                if (itApplication != applicationLoadDiffs.end())
                {
                    itApplication->second += swapLoadDiffEntry;
                }
                else
                {
                    applicationLoadDiffs.insert(pair<ApplicationEntry const*, LoadEntry>(application, move(swapLoadDiffEntry)));
                }
            }
        }

        for (auto itApplication = applicationLoadDiffs.begin(); itApplication != applicationLoadDiffs.end(); ++itApplication)
        {
            auto application = itApplication->first;
            auto loadDiff = itApplication->second;
            auto applicationNodeCapacities = application->PerNodeCapacities;

            for (size_t metricIndex = 0; metricIndex < globalMetricCount; metricIndex++)
            {
                int64 capacity = applicationNodeCapacities.Values[metricIndex];

                if (capacity > 0)
                {
                    int64 disappearingMetricLoad = application->GetDisappearingMetricNodeLoad(node, metricIndex);
                    
                    int64 applicationMetricLoadOnNode = ((NodeMetrics const&)tempSolution.ApplicationNodeLoad[application])[node].Values[metricIndex];

                    int64 totalLoad = applicationMetricLoadOnNode + disappearingMetricLoad;
                    int64 swapDiff = loadDiff.Values[metricIndex];

                    if (totalLoad + swapDiff > capacity)
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    });
}

ApplicationCapacityConstraint::ApplicationCapacityConstraint(int priority)
    : IDynamicConstraint(priority)
{
}

void ApplicationCapacityConstraint::CorrectViolations(TempSolution & solution, std::vector<ISubspaceUPtr> const& subspaces, Common::Random & random) const
{
    UNREFERENCED_PARAMETER(subspaces);

    ConstraintCheckResult result = CheckSolutionAndGenerateSubspace(solution, false, false, false, random);

    if (!result.IsValid)
    {
        ApplicationCapacitySubspace const& subspace = dynamic_cast<ApplicationCapacitySubspace const&>(*(result.Subspace));
        NodeSet candidateNodes(solution.OriginalPlacement, false);
        for (auto itNode = subspace.NodeExtraLoads.begin(); itNode != subspace.NodeExtraLoads.end(); ++itNode)
        {
            PlacementReplicaSet const& replicas = itNode->second;
            for (auto itReplica = replicas.begin(); itReplica != replicas.end(); ++itReplica)
            {
                candidateNodes.SelectEligible(*itReplica, solution.IsSwapPreferred);
                candidateNodes.Delete(itNode->first);

                NodeBlockListConstraint::ExcludeServiceBlockList(candidateNodes, *itReplica);

                NodeEntry const* targetNode = candidateNodes.SelectRandom(random);
                if (targetNode != nullptr)
                {
                    bool shouldTryMove = true;
                    if (solution.IsSwapPreferred)
                    {
                        shouldTryMove = !solution.TrySwapMovement(*itReplica, targetNode, random, false);
                    }
                    if (shouldTryMove)
                    {
                        solution.MoveReplica(*itReplica, targetNode, random);
                    }
                }
            }
        }
    }
}

IViolationUPtr ApplicationCapacityConstraint::GetViolations(
    TempSolution const& tempSolution,
    bool changedOnly,
    bool relaxed,
    bool useNodeBufferCapacity,
    Random& random) const
{
    UNREFERENCED_PARAMETER(changedOnly);
    UNREFERENCED_PARAMETER(useNodeBufferCapacity);
    UNREFERENCED_PARAMETER(random);
    size_t globalMetricCount = tempSolution.OriginalPlacement->GlobalMetricCount;
    map<NodeEntry const*, LoadEntry> nodeLoadOverCapacity;
    int64 totalLoadOverCapacity = 0;

    tempSolution.ForEachApplication(false, [&](ApplicationEntry const* application) -> bool
    {
        if (!application->HasPerNodeCapacity)
        {
            return true;
        }

        auto const& applicationNodes = tempSolution.ApplicationNodeCounts[application];
        for (auto nIt = applicationNodes.begin(); nIt != applicationNodes.end(); ++nIt)
        {
            // iterate through application nodes
            NodeEntry const* node = nIt->first;
            //skip down nodes
            if (!node->IsUp)
            {
                return true;
            }

            LoadEntry loadOverCapacity(globalMetricCount);

            int64 totalNodeViolation = CalculateNodeLoadDiff(tempSolution, application, node, relaxed, loadOverCapacity);
            if (totalNodeViolation > 0)
            {
                auto nodeLoadIt = nodeLoadOverCapacity.find(node);
                if (nodeLoadIt == nodeLoadOverCapacity.end())
                {
                    nodeLoadOverCapacity.insert(make_pair(node, loadOverCapacity));
                }
                else
                {
                    nodeLoadIt->second += loadOverCapacity;
                }

                totalLoadOverCapacity += totalNodeViolation;
                loadOverCapacity.clear();
            }
        }

        return true;
    });

    return make_unique<NodeLoadViolation>(
        totalLoadOverCapacity, 
        move(nodeLoadOverCapacity),
        &tempSolution.OriginalPlacement->BalanceCheckerObj->LBDomains.back());
}

ConstraintCheckResult ApplicationCapacityConstraint::CheckSolutionAndGenerateSubspace(
    TempSolution const& tempSolution,
    bool changedOnly,
    bool relaxed,
    bool useNodeBufferCapacity,
    Random & random,
    std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr /* = nullptr */) const
{
    UNREFERENCED_PARAMETER(useNodeBufferCapacity);

    PlacementReplicaSet invalidReplicas;
    size_t globalMetricCount = tempSolution.OriginalPlacement->GlobalMetricCount;

    map<NodeEntry const*, PlacementReplicaSet> nodeChanges;
    PlacementReplicaSet nodeInvalidReplicas;
    // Not used, need to pass it as parameter.
    std::set<ApplicationEntry const*> nodeInvalidApplications;
    std::set<ServicePackageEntry const*> nodeInvalidServicePackages;

    tempSolution.ForEachApplication(changedOnly, [&](ApplicationEntry const* application) -> bool
    {
        if (!application->HasPerNodeCapacity)
        {
            return true;
        }

        // Itreate throught the nodes of this application
        tempSolution.ForEachNode(application, changedOnly, [&](NodeEntry const* node) -> bool
        {
            //skip down nodes
            if (!node->IsUp)
            {
                return true;
            }
            LoadEntry loadOverCapacity(globalMetricCount);

            int64 totalNodeViolation = CalculateNodeLoadDiff(tempSolution, application, node, relaxed, loadOverCapacity, diagnosticsDataPtr);

            if (0 == totalNodeViolation)
            {
                return true;
            }

            auto const& replicasOnNode = (tempSolution.ApplicationPlacements[application]).operator[] (node);

            NodeCapacityConstraint::FindAllInvalidReplicas(
                tempSolution,
                replicasOnNode.Data,
                tempSolution.OriginalPlacement->LBDomains.back().MetricStartIndex,
                random,
                loadOverCapacity,
                invalidReplicas,
                nodeInvalidReplicas,
                nodeInvalidApplications,
                nodeInvalidServicePackages,
                node,
                true);

            auto nodeLoadIt = nodeChanges.find(node);
            if (nodeLoadIt == nodeChanges.end())
            {
                nodeChanges.insert(make_pair(node, move(nodeInvalidReplicas)));
            }
            else
            {
                nodeLoadIt->second.insert(nodeInvalidReplicas.begin(), nodeInvalidReplicas.end());
                nodeInvalidReplicas.clear();
            }

            return true;
        });

        return true;
    });

    return ConstraintCheckResult(make_unique<ApplicationCapacitySubspace>(relaxed, move(nodeChanges)), move(invalidReplicas));
}

int64 ApplicationCapacityConstraint::CalculateNodeLoadDiff(
    TempSolution const& tempSolution,
    ApplicationEntry const* application,
    NodeEntry const* node,
    bool relaxed,
    LoadEntry & loadOverCapacity,
    std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr  /* = nullptr */) const
{
    size_t globalMetricCount = tempSolution.OriginalPlacement->GlobalMetricCount;
    size_t totalMetricCount = tempSolution.OriginalPlacement->TotalMetricCount;
    ASSERT_IFNOT(totalMetricCount >= globalMetricCount, "Invalid metric count");

    int64 totalLoadOverCapacity(0);
    for (size_t i = 0; i < globalMetricCount; ++i)
    {
        int64 baseMetricLoadOnNode = ((NodeMetrics const&)tempSolution.BaseSolution.ApplicationNodeLoad[application])[node].Values[i];
        int64 applicationMetricLoadOnNode = ((NodeMetrics const&)tempSolution.ApplicationNodeLoad[application])[node].Values[i];

        int64 capacity = application->PerNodeCapacities.Values[i];
        if (capacity >= 0)
        {
            if (relaxed)
            {
                capacity = max(capacity, baseMetricLoadOnNode);
            }

            if (applicationMetricLoadOnNode > capacity)
            {
                loadOverCapacity.Set(i, applicationMetricLoadOnNode - capacity);
                totalLoadOverCapacity += applicationMetricLoadOnNode - capacity;

                //Diagnostics
                if (diagnosticsDataPtr != nullptr)
                {
                    diagnosticsDataPtr->changed_ = true;
                    auto appCap = std::static_pointer_cast<ApplicationCapacityConstraintDiagnosticsData>(diagnosticsDataPtr);
                    LoadBalancingDomainEntry const& globalDomain = tempSolution.OriginalPlacement->BalanceCheckerObj->LBDomains.back();

                    MetricCapacityDiagnosticInfo info(
                        globalDomain.Metrics[i].Name,
                        node->NodeId,
                        (diagnosticsDataPtr->plbNodesPtr_ != nullptr) ? (*(diagnosticsDataPtr->plbNodesPtr_)).at(node->NodeIndex).NodeDescriptionObj.NodeName : L"",
                        applicationMetricLoadOnNode,
                        capacity,
                        application->Name);

                    appCap->AddMetricDiagnosticInfo(move(info));
                }

            }
        }
    }

    return totalLoadOverCapacity;
}
