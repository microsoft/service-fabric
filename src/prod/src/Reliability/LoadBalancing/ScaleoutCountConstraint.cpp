// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Checker.h"
#include "ScaleoutCountConstraint.h"
#include "PlacementReplica.h"
#include "PartitionEntry.h"
#include "ApplicationEntry.h"
#include "TempSolution.h"
#include "PLBConfig.h"
#include "Node.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

ScaleoutCountSubspace::ScaleoutCountSubspace(map<ApplicationEntry const*, set<NodeEntry const*> > && applicationNodes, bool relaxed)
: applicationNodes_(std::move(applicationNodes)), relaxed_(relaxed)
{
}

void ScaleoutCountSubspace::GetTargetNodes(
    TempSolution const& tempSolution,
    PlacementReplica const* replica,
    NodeSet & candidateNodes,
    bool useNodeBufferCapacity,
    NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr /* = nullptr */) const
{
    UNREFERENCED_PARAMETER(useNodeBufferCapacity);
    UNREFERENCED_PARAMETER(nodeToConstraintDiagnosticsDataMapSPtr);

    ApplicationEntry const* application = replica->Partition->Service->Application;
    if (!application || application->ScaleoutCount == 0)
    {
        return;
    }

    auto const& applicationNodeCounts = tempSolution.ApplicationNodeCounts[application];

    auto it = applicationNodes_.find(application);
    if (it == applicationNodes_.end())
    {
        size_t scaleoutCount = application->ScaleoutCount;
        size_t currentCount = applicationNodeCounts.size();

        if (currentCount < scaleoutCount)
        {
            return;
        }
        else
        {
            if (candidateNodes.Count <= currentCount)
            {
                candidateNodes.Filter([&](NodeEntry const *node) -> bool
                {
                    return applicationNodeCounts.find(node) != applicationNodeCounts.end();
                });
            }
            else
            {
                candidateNodes.Intersect([&](function<void(NodeEntry const *)> add)
                {
                    for (auto it = applicationNodeCounts.begin(); it != applicationNodeCounts.end(); ++it)
                    {
                        add(it->first);
                    }
                }, applicationNodeCounts.size());
            }

            return;
        }
    }
    else
    {
        set<NodeEntry const *> const & invalidNodes = it->second;
        set<NodeEntry const *> validNodes;
        size_t baseSolutionScaleout = tempSolution.BaseSolution.ApplicationNodeCounts[application].size();
        bool isRelaxed = relaxed_ && baseSolutionScaleout >= applicationNodeCounts.size();

        for (auto nodeIt = applicationNodeCounts.begin(); nodeIt != applicationNodeCounts.end(); ++nodeIt)
        {
            if (isRelaxed || invalidNodes.find(nodeIt->first) == invalidNodes.end())
            {
                validNodes.insert(nodeIt->first);
            }
        }

        candidateNodes.Filter([&](NodeEntry const *node) -> bool
        {
            return validNodes.find(node) != validNodes.end();
        });

        return;
    }
}

IViolationUPtr ScaleoutCountConstraint::GetViolations(
    TempSolution const& tempSolution,
    bool changedOnly,
    bool relaxed,
    bool useNodeBufferCapacity,
    Random& random) const
{
    UNREFERENCED_PARAMETER(useNodeBufferCapacity);
    map<ApplicationEntry const*, set<NodeEntry const *>> appNodes;
    PlacementReplicaSet invalidReplicas  = GetInvalidReplicas(tempSolution, changedOnly, relaxed, random, appNodes);

    if (invalidReplicas.size() == 0 && appNodes.size() > 0)
    {
        //for placement this can happen because we do not add all the replicas into the closure
        ASSERT_IF(tempSolution.OriginalPlacement->BalanceCheckerObj->ClosureType != PartitionClosureType::Enum::NewReplicaPlacement &&
            tempSolution.OriginalPlacement->Settings.IsTestMode, "Violation without invalid replicas should not exist.");
        appNodes.clear();
    }
    return make_unique<ScaleoutCountViolation>(move(appNodes), move(invalidReplicas));
}

ScaleoutCountConstraint::ScaleoutCountConstraint(int priority)
    : IDynamicConstraint(priority)
{
}

ConstraintCheckResult ScaleoutCountConstraint::CheckSolutionAndGenerateSubspace(
    TempSolution const& tempSolution,
    bool changedOnly,
    bool relaxed,
    bool useNodeBufferCapacity,
    Random & random,
    std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr /* = nullptr */) const
{
    UNREFERENCED_PARAMETER(useNodeBufferCapacity);

    std::map<ApplicationEntry const*, std::set<NodeEntry const *>> appNodes;
    PlacementReplicaSet invalidReplicas = GetInvalidReplicas(tempSolution, changedOnly, relaxed, random, appNodes, diagnosticsDataPtr);

    return ConstraintCheckResult(make_unique<ScaleoutCountSubspace>(move(appNodes), relaxed), move(invalidReplicas));
}

PlacementReplicaSet ScaleoutCountConstraint::GetInvalidReplicas(
    TempSolution const& tempSolution,
    bool changedOnly,
    bool relaxed,
    Random & random,
    std::map<ApplicationEntry const*, std::set<NodeEntry const *>> & appNodes,
    std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr) const
{
    PlacementReplicaSet invalidReplicas;

    tempSolution.ForEachApplication(changedOnly, [&](ApplicationEntry const* application) -> bool
    {
        auto const& applicationNodeCounts = tempSolution.ApplicationNodeCounts[application];
        size_t scaleoutCount = application->ScaleoutCount;

        if (scaleoutCount == 0)
        {
            return true;
        }

        size_t currentNodeCount = applicationNodeCounts.size();
        if (currentNodeCount > scaleoutCount)
        {
            auto const& replicasOnNode = tempSolution.ApplicationPlacements[application];
            set<NodeEntry const *> nodes;
            size_t invalidNodeCount = currentNodeCount - scaleoutCount;
            if (relaxed)
            {
                // When relaxed, don't always choose nodes with most replicas and this is for balancing
                auto myFunc = [&random](size_t n) -> int
                {
                    return random.Next(static_cast<int>(n));
                };

                vector<NodeEntry const *> nodesVec;
                nodesVec.reserve(currentNodeCount);
                for (auto it = applicationNodeCounts.begin(); it != applicationNodeCounts.end(); ++it)
                {
                    //only add nodes that have all the replicas in the closure
                    if (replicasOnNode[it->first].Data.size() == it->second)
                    {
                        nodesVec.push_back(it->first);
                    }
                }

                random_shuffle(nodesVec.begin(), nodesVec.end(), myFunc);
                //we might not have all the invalid nodes here during placement
                //if we made a violation even worse than we will have that node here
                invalidNodeCount = min(invalidNodeCount, nodesVec.size());
                nodes.insert(nodesVec.begin(), nodesVec.begin() + invalidNodeCount);
            }
            else
            {
                // mapping of <replicaCountOnNode, node>
                multimap<size_t, NodeEntry const *> replicaCount;
                for (auto it = applicationNodeCounts.begin(); it != applicationNodeCounts.end(); ++it)
                {
                    //only add nodes that have all the replicas in the closure
                    if (replicasOnNode[it->first].Data.size() == it->second)
                    {
                        replicaCount.insert(make_pair(it->second, it->first));
                    }
                }

                for (auto nodeIt = replicaCount.begin(); nodeIt != replicaCount.end(); nodeIt++)
                {
                    nodes.insert(nodeIt->second);
                    if (nodes.size() == invalidNodeCount)
                    {
                        break;
                    }
                }
            }

            for (auto itNode = nodes.begin(); itNode != nodes.end(); ++itNode)
            {
                auto & nodeInvalidReplicas = replicasOnNode[*itNode].Data;
                invalidReplicas.insert(nodeInvalidReplicas.begin(), nodeInvalidReplicas.end());

                //Diagnostics
                if (diagnosticsDataPtr != nullptr)
                {
                    diagnosticsDataPtr->changed_ = true;
                    auto sca = std::static_pointer_cast<ScaleoutCountConstraintDiagnosticsData>(diagnosticsDataPtr);

                    for (auto const & r : nodeInvalidReplicas)
                    {
                        BasicDiagnosticInfo info;

                        info.serviceName_ = r->Partition->Service->Name;
                        info.miscellanious_ = application->Name;
                        info.AddPartition(r->Partition->PartitionId);
                        info.AddNode(r->Node->NodeId,
                            (diagnosticsDataPtr->plbNodesPtr_ != nullptr)
                            ? (*(diagnosticsDataPtr->plbNodesPtr_)).at(r->Node->NodeIndex).NodeDescriptionObj.NodeName
                            : L"");

                        sca->AddBasicDiagnosticsInfo(move(info));
                    }
                }
            }

            appNodes.insert(std::make_pair(application, std::move(nodes)));
        }

        return true;
    });

    return invalidReplicas;
}
