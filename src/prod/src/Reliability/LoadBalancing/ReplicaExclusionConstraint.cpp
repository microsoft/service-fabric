// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Checker.h"
#include "PlacementReplica.h"
#include "PartitionEntry.h"
#include "TempSolution.h"
#include "ReplicaExclusionConstraint.h"
#include "IViolation.h"
#include "Node.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

void ReplicaExclusionStaticConstraint::GetTargetNodes(
    TempSolution const& tempSolution,
    PlacementReplica const* replica,
    NodeSet & candidateNodes,
    NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr /* = nullptr */) const
{
    UNREFERENCED_PARAMETER(tempSolution);
    UNREFERENCED_PARAMETER(nodeToConstraintDiagnosticsDataMapSPtr);

    replica->Partition->ForEachExistingReplica([&](PlacementReplica const* r)
    {
        if (r != replica && r->Role == replica->Role)
        {
            candidateNodes.Delete(r->Node);
        }
    }, true, true);
}

bool ReplicaExclusionStaticConstraint::CheckReplica(TempSolution const& tempSolution, PlacementReplica const* replica, NodeEntry const* target) const
{
    UNREFERENCED_PARAMETER(tempSolution);

    ASSERT_IF(replica == nullptr || target == nullptr, "Replica or target node are empty {0} {1}", replica, target);

    if (replica->Node == target)
    {
        return true;
    }
    else
    {
        PlacementReplica const* targetReplica = replica->Partition->GetReplica(target);

        return (targetReplica == nullptr || replica->Role != targetReplica->Role);
    }
}

ReplicaExclusionDynamicSubspace::ReplicaExclusionDynamicSubspace()
{
}

void ReplicaExclusionDynamicSubspace::GetTargetNodes(
    TempSolution const& tempSolution,
    PlacementReplica const* replica,
    NodeSet & candidateNodes,
    bool useNodeBufferCapacity,
	NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr /* = nullptr */) const
{
    UNREFERENCED_PARAMETER(useNodeBufferCapacity);
	UNREFERENCED_PARAMETER(nodeToConstraintDiagnosticsDataMapSPtr);

    auto const& replicaPlacement = tempSolution.PartitionPlacements[replica->Partition];

    for (auto it = replicaPlacement.Data.begin(); it != replicaPlacement.Data.end(); ++it)
    {
        if (!(it->second.size() == 1 && *(it->second.begin()) == replica))
        {
            candidateNodes.Delete(it->first);
        }
    }
}

IViolationUPtr ReplicaExclusionDynamicConstraint::GetViolations(
    TempSolution const& tempSolution,
    bool changedOnly,
    bool relaxed,
    bool useNodeBufferCapacity,
    Random& random) const
{
    UNREFERENCED_PARAMETER(relaxed); // this is hard constraint and will never be relaxed
    UNREFERENCED_PARAMETER(useNodeBufferCapacity);
    UNREFERENCED_PARAMETER(random);
    set<PartitionEntry const*> invalidPartitions;

    tempSolution.ForEachPartition(changedOnly, [&](PartitionEntry const* partition) -> bool
    {
        ReplicaPlacement const& replicaPlacement = tempSolution.PartitionPlacements[partition];
        ASSERT_IFNOT(replicaPlacement.Data.size() <= replicaPlacement.ReplicaCount, "Replica placement size {0} should be <= placed replica count {1}", replicaPlacement.Data.size(), replicaPlacement.ReplicaCount);

        if (replicaPlacement.Data.size() < replicaPlacement.ReplicaCount)
        {
            invalidPartitions.insert(partition);
        }

        return true;
    });

    return make_unique<PartitionSetViolation>(move(invalidPartitions));
}

ConstraintCheckResult ReplicaExclusionDynamicConstraint::CheckSolutionAndGenerateSubspace(
    TempSolution const& tempSolution,
    bool changedOnly,
    bool relaxed,
    bool useNodeBufferCapacity,
    Random & random,
	std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr /* = nullptr */) const
{
    UNREFERENCED_PARAMETER(relaxed); // this is hard constraint and will never be relaxed
    UNREFERENCED_PARAMETER(useNodeBufferCapacity);

    PlacementReplicaSet invalidReplicas;

    tempSolution.ForEachPartition(changedOnly, [&](PartitionEntry const* partition) -> bool
    {
        ReplicaPlacement const& replicaPlacement = tempSolution.PartitionPlacements[partition];
        ASSERT_IFNOT(replicaPlacement.Data.size() <= replicaPlacement.ReplicaCount, "Replica placement size {0} should be <= placed replica count {1}", replicaPlacement.Data.size(), replicaPlacement.ReplicaCount);

        if (replicaPlacement.Data.size() < replicaPlacement.ReplicaCount)
        {
            for (auto itNode = replicaPlacement.Data.begin(); itNode != replicaPlacement.Data.end(); ++itNode)
            {
                size_t replicaCount = itNode->second.size();
                if (replicaCount > 1)
                {
                    int index = random.Next(static_cast<int>(replicaCount));

                    auto itReplica = itNode->second.begin();

                    for (int i = 0; i < static_cast<int>(replicaCount); ++i)
                    {
                        if (i != index)
                        {
                            invalidReplicas.insert(*itReplica);

                            //Diagnostics
                            if (diagnosticsDataPtr != nullptr)
                            {
                                diagnosticsDataPtr->changed_ = true;

                                BasicDiagnosticInfo info;
                                auto rxD = std::static_pointer_cast<ReplicaExclusionDynamicConstraintDiagnosticsData>(diagnosticsDataPtr);

                                info.serviceName_ = (*itReplica)->Partition->Service->Name;
                                info.AddPartition((*itReplica)->Partition->PartitionId);
                                info.AddNode(itNode->first->NodeId,
                                    (diagnosticsDataPtr->plbNodesPtr_ != nullptr)
                                    ? (*(diagnosticsDataPtr->plbNodesPtr_)).at(itNode->first->NodeIndex).NodeDescriptionObj.NodeName
                                    : L"");

                                rxD->AddBasicDiagnosticsInfo(move(info));
                            }
                        }
                        ++itReplica;
                    }
                }
            }
        }

        return true;
    });

    return ConstraintCheckResult(make_unique<ReplicaExclusionDynamicSubspace>(), move(invalidReplicas));
}
