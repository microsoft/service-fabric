// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ThrottlingConstraint.h"
#include "IViolation.h"
#include "TempSolution.h"

using namespace Reliability::LoadBalancingComponent;

ThrottlingSubspace::ThrottlingSubspace(bool relaxed)
    : relaxed_(relaxed)
{
}

void ThrottlingSubspace::GetTargetNodes(TempSolution const & tempSolution,
    PlacementReplica const * replica,
    NodeSet & candidateNodes,
    bool useNodeBufferCapacity,
    NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr) const
{
    UNREFERENCED_PARAMETER(useNodeBufferCapacity);
    UNREFERENCED_PARAMETER(nodeToConstraintDiagnosticsDataMapSPtr);

    if (!replica->CanBeThrottled)
    {
        return;
    }

    candidateNodes.Filter([&](NodeEntry const *node) -> bool
    {
        if (!node->IsThrottled)
        {
            return true;
        }

        if (node == replica->Node)
        {
            // Replica can always return to its original position
            return true;
        }

        uint64_t maxConcurrentBuilds = node->MaxConcurrentBuilds;
        uint64_t currentBuilds = tempSolution.InBuildCountsPerNode[node];

        if (currentBuilds < maxConcurrentBuilds)
        {
            return true;
        }

        return false;
    });

}

ThrottlingConstraint::ThrottlingConstraint(int priority)
    : IDynamicConstraint(priority)
{
}

IViolationUPtr ThrottlingConstraint::GetViolations(TempSolution const & solution,
    bool changedOnly,
    bool relaxed,
    bool useNodeBufferCapacity,
    Common::Random & random) const
{
    UNREFERENCED_PARAMETER(useNodeBufferCapacity);
    PlacementReplicaSet invalidReplicas = GetInvalidReplicas(solution, changedOnly, relaxed, random);
    return make_unique<ReplicaSetViolation> (move(invalidReplicas));
}

ConstraintCheckResult ThrottlingConstraint::CheckSolutionAndGenerateSubspace(TempSolution const & tempSolution,
    bool changedOnly,
    bool relaxed,
    bool useNodeBufferCapacity,
    Common::Random & random,
    std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr) const
{
    UNREFERENCED_PARAMETER(useNodeBufferCapacity);

    auto invalidReplicas = GetInvalidReplicas(tempSolution, changedOnly, relaxed, random);
    return ConstraintCheckResult(make_unique<ThrottlingSubspace>(relaxed), move(invalidReplicas));
}

int ThrottlingConstraint::GetPriority(PLBSchedulerActionType::Enum action)
{
    // This function is called only if there are nodes with limits.
    // There are configurations that decide if each of phases is throttled or not.
    // If a phase is not throttled, then constraint priority is -1 (not used)
    // Otherwise, constraint priority is defined by ThrottlingConstraintPriority setting.
    switch (action)
    {
    case PLBSchedulerActionType::LoadBalancing:
    case PLBSchedulerActionType::QuickLoadBalancing:
        if (PLBConfig::GetConfig().ThrottleBalancingPhase)
        {
            return PLBConfig::GetConfig().ThrottlingConstraintPriority;
        }
        break;
    case PLBSchedulerActionType::ConstraintCheck:
        if (PLBConfig::GetConfig().ThrottleConstraintCheckPhase)
        {
            return PLBConfig::GetConfig().ThrottlingConstraintPriority;
        }
        break;
    case PLBSchedulerActionType::NewReplicaPlacement:
    case PLBSchedulerActionType::NewReplicaPlacementWithMove:
        if (PLBConfig::GetConfig().ThrottlePlacementPhase)
        {
            return PLBConfig::GetConfig().ThrottlingConstraintPriority;
        }
        break;
    default:
        // Throttling is not needed in this case.
        return -1;
    }
    return -1;
}

PlacementReplicaSet ThrottlingConstraint::GetInvalidReplicas(TempSolution const & solution,
    bool changedOnly,
    bool relaxed,
    Common::Random & random) const
{
    PlacementReplicaSet invalidReplicas;
    if (!changedOnly || !relaxed)
    {
        // Throttling is not a "true constraint", meaning that original placement can't have any violations.
        // It does not make sense to check violations anywhere outside of Checker::MoveSolution.
        return invalidReplicas;
    }

    // Check if per-node throttling is satisfied.
    solution.ForEachNode(changedOnly, [&](NodeEntry const* node) -> bool
    {
        if (!node->IsUp || !node->IsThrottled)
        {
            return true;
        }

        uint64_t maxConcurrentBuilds = node->MaxConcurrentBuilds;
        uint64_t currentBuilds = solution.InBuildCountsPerNode[node];

        if (currentBuilds <= maxConcurrentBuilds)
        {
            return true;
        }

        uint64_t overCount = currentBuilds - maxConcurrentBuilds;
        uint64_t baseBuilds = solution.BaseSolution.InBuildCountsPerNode[node];

        if (baseBuilds > maxConcurrentBuilds)
        {
            // This can happen only if original placement had more IB replicas than maximum.
            // In that case, remove only what was added in temp solution.
            TESTASSERT_IF(currentBuilds < baseBuilds,
                "Current builds {0} smaller than builds in base {1}",
                currentBuilds,
                baseBuilds);
            if (currentBuilds >= baseBuilds)
            {
                overCount = currentBuilds - baseBuilds;
            }
        }

        if (overCount == 0)
        {
            // In case when all "over" replicas existed also in CandidateSolution --> nothing to do here.
            return true;
        }

        auto const& tempPlacements = solution.NodePlacements[node];

        std::vector<PlacementReplica const*> candidateReplicas;
        candidateReplicas.reserve(overCount);

        // Pick replicas that were moved into the node only in TempSolution or in CandidateSolution.
        // Moving other replicas can't fix the violation (won't decrease IB count).
        for (auto replica : tempPlacements.Data)
        {
            if (replica->Partition != nullptr
                && replica->Partition->Service != nullptr
                && !replica->Partition->Service->IsStateful)
            {
                continue;
            }
            if (replica->Partition->HasReplicaOnNode(node))
            {
                continue;
            }
            candidateReplicas.push_back(replica);
        }

        auto myFunc = [&random](size_t n) -> int
        {
            return random.Next(static_cast<int>(n));
        };
        random_shuffle(candidateReplicas.begin(), candidateReplicas.end(), myFunc);

        TESTASSERT_IF(candidateReplicas.size() < overCount, "");
        uint64_t insertedCount = 0;
        for (auto replica : candidateReplicas)
        {
            invalidReplicas.insert(replica);
            if (++insertedCount >= overCount)
            {
                break;
            }
        }

        return true;
    },
        false,
        true);  // Iterate only over nodes that have throttling

    return invalidReplicas;
}
