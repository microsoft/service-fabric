// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "PreferredLocationConstraint.h"
#include "PlacementReplica.h"
#include "PartitionEntry.h"
#include "NodeSet.h"
#include "ServiceEntry.h"
#include "TempSolution.h"
#include "Node.h"
#include "FaultDomainConstraint.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

std::set<Common::TreeNodeIndex> const& PreferredLocationConstraint::EmptyUpgradedUDs = std::set<Common::TreeNodeIndex>();

PreferredLocationSubspace::PreferredLocationSubspace(PreferredLocationConstraint const* constraint)
    : StaticSubspace(constraint, false)
{
}

void PreferredLocationSubspace::GetNodesForReplicaDrop(
    TempSolution const& tempSolution,
    PartitionEntry const& partition,
    NodeSet & candidateNodes) const
{
    // Number of elements in these vectors will be at most equal to number of replicas.
    // In general case, that is orders of magnitude smaller than number of nodes in the cluster.
    // Using std::vector, NodeSet::Intersect will have smaller complexity than if we used NodeSet here.
    std::vector<NodeEntry const*> blocklistedAndMoveInProgressNodes;
    std::vector<NodeEntry const*> moveInProgressReplicasNodes;
    std::vector<NodeEntry const*> blocklistedNodesWithReplicas;
    std::vector<NodeEntry const*> fdNodesWithMoreThanQuorumReplicas;
    std::vector<NodeEntry const*> udNodesWithMoreThanQuorumReplicas;

    ServiceEntry const* service = partition.Service;

    // Active replicas marked with MoveInProgress should be prioritized for dropping
    partition.ForEachExistingReplica([&](PlacementReplica const* r)
    {
        NodeEntry const* node = tempSolution.GetReplicaLocation(r);

        if (nullptr == node)
        {
            return;
        }

        if (r->IsMoveInProgress && service->IsNodeInBlockList(node))
        {
            blocklistedAndMoveInProgressNodes.push_back(node);
        }
        else if (r->IsMoveInProgress)
        {
            moveInProgressReplicasNodes.push_back(node);
        }
        else if (service->IsNodeInBlockList(node))
        {
            blocklistedNodesWithReplicas.push_back(node);
        }
        else if (!service->OnEveryNode)
        {
            auto& settings = tempSolution.OriginalPlacement->Settings;

            if (settings.FaultDomainEnabled &&
                settings.FaultDomainConstraintPriority != -1 &&
                settings.QuorumBasedReplicaDistributionPerFaultDomains &&
                service->FDDistribution != Service::Type::Ignore &&
                !tempSolution.OriginalPlacement->BalanceCheckerObj->FaultDomainStructure.IsEmpty)
            {
                if (IsNodeInOverloadedDomain(tempSolution, r->Partition, node, true))
                {
                    fdNodesWithMoreThanQuorumReplicas.push_back(node);
                }
            }

            if (settings.UpgradeDomainEnabled &&
                settings.UpgradeDomainConstraintPriority != -1 &&
                settings.QuorumBasedReplicaDistributionPerUpgradeDomains &&
                !tempSolution.OriginalPlacement->BalanceCheckerObj->UpgradeDomainStructure.IsEmpty)
            {
                if (IsNodeInOverloadedDomain(tempSolution, r->Partition, node, false))
                {
                    udNodesWithMoreThanQuorumReplicas.push_back(node);
                }
            }
        }
    }, false, true);

    if (!blocklistedAndMoveInProgressNodes.empty())
    {
        candidateNodes.Intersect([&](function<void(NodeEntry const *)> f)
        {
            for (auto itNode = blocklistedAndMoveInProgressNodes.begin(); itNode != blocklistedAndMoveInProgressNodes.end(); ++itNode)
            {
                f(*itNode);
            }
        });
    }
    else if (!moveInProgressReplicasNodes.empty())
    {
        candidateNodes.Intersect([&](function<void(NodeEntry const *)> f)
        {
            for (auto itNode = moveInProgressReplicasNodes.begin(); itNode != moveInProgressReplicasNodes.end(); ++itNode)
            {
                f(*itNode);
            }
        });
    }
    else if (!blocklistedNodesWithReplicas.empty())
    {
        candidateNodes.Intersect([&](function<void(NodeEntry const *)> f)
        {
            for (auto itNode = blocklistedNodesWithReplicas.begin(); itNode != blocklistedNodesWithReplicas.end(); ++itNode)
            {
                f(*itNode);
            }
        });
    }
    else if (!udNodesWithMoreThanQuorumReplicas.empty())
    {
        candidateNodes.Intersect([&](function<void(NodeEntry const *)> f)
        {
            for (auto itNode = udNodesWithMoreThanQuorumReplicas.begin(); itNode != udNodesWithMoreThanQuorumReplicas.end(); ++itNode)
            {
                f(*itNode);
            }
        });
    }
    else if (!fdNodesWithMoreThanQuorumReplicas.empty())
    {
        candidateNodes.Intersect([&](function<void(NodeEntry const *)> f)
        {
            for (auto itNode = fdNodesWithMoreThanQuorumReplicas.begin(); itNode != fdNodesWithMoreThanQuorumReplicas.end(); ++itNode)
            {
                f(*itNode);
            }
        });
    }
}

bool PreferredLocationSubspace::IsNodeInOverloadedDomain(
    TempSolution const& tempSolution,
    PartitionEntry const* partition,
    NodeEntry const* node,
    bool isFaultDomain) const
{
    PartitionDomainTree const* replicaTreeTemp = nullptr;
    PartitionDomainTree const* replicaTreeBase = nullptr;
    BalanceChecker::DomainTree const* nodeTree = nullptr;

    if (FaultDomainConstraint::GetTempAndBaseTrees(tempSolution, partition, isFaultDomain, false, replicaTreeTemp, replicaTreeBase, nodeTree))
    {
        size_t level = 0;
        size_t targetSegment = 0;
        size_t childDomainsCount = 0;
        size_t targetDomainReplicasTemp = 0;
        size_t targetDomainReplicasBase = 0;
        auto& candidateNodeIndex = isFaultDomain ? node->FaultDomainIndex : node->UpgradeDomainIndex;

        if (FaultDomainConstraint::GetFirstChildSubDomain(
            replicaTreeBase,
            replicaTreeTemp,
            nodeTree,
            candidateNodeIndex,
            level,
            targetSegment,
            childDomainsCount,
            targetDomainReplicasTemp,
            targetDomainReplicasBase))
        {
            size_t replicaLimit = partition->MaxReplicasPerDomain;

            if (childDomainsCount * replicaLimit < partition->TargetReplicaSetSize)
            {
                ++replicaLimit;
            }

            if (targetDomainReplicasTemp > replicaLimit)
            {
                return true;
            }
        }
    }

    return false;
}

bool PreferredLocationSubspace::PromoteSecondary(TempSolution const& tempSolution, PartitionEntry const* partition, NodeSet & candidateNodes) const
{
    if (partition == nullptr || partition->Service->OnEveryNode)
    {
        return false;
    }

    set<Common::TreeNodeIndex> const& upgradedUDs = PreferredLocationConstraint::GetUpgradedUDs(tempSolution, partition);
    if (!upgradedUDs.empty())
    {
        NodeSet nodesUpgradedUDs(candidateNodes);
        nodesUpgradedUDs.Filter([&](NodeEntry const *node) -> bool
        {
            return PreferredLocationConstraint::CheckUpgradedUDs(upgradedUDs, node);
        });

        if (!nodesUpgradedUDs.IsEmpty)
        {
            // Use the intersection of preferred location and upgradedUDs;
            // If empty, do nothing since candidateNodes already include the preferred location
            candidateNodes.Copy(nodesUpgradedUDs);
        }
    }

    return false;
}

ConstraintCheckResult PreferredLocationConstraint::CheckSolutionAndGenerateSubspace(
    TempSolution const& tempSolution,
    bool changedOnly,
    bool relaxed,
    bool useNodeBufferCapacity,
    Common::Random & random,
    std::shared_ptr<IConstraintDiagnosticsData> const diagnosticsDataPtr) const
{
    UNREFERENCED_PARAMETER(random);
    UNREFERENCED_PARAMETER(useNodeBufferCapacity);

    auto invalidReplicas = GetInvalidReplicas(tempSolution, changedOnly, relaxed);

    //Diagnostics
    if (diagnosticsDataPtr != nullptr)
    {
        diagnosticsDataPtr->changed_ = !invalidReplicas.empty();
        auto pref = std::static_pointer_cast<PreferredLocationConstraintDiagnosticsData>(diagnosticsDataPtr);

        for (auto const & r : invalidReplicas)
        {
            BasicDiagnosticInfo info;

            info.serviceName_ = r->Partition->Service->Name;
            info.AddPartition(r->Partition->PartitionId);
            info.AddNode(r->Node->NodeId,
                (diagnosticsDataPtr->plbNodesPtr_ != nullptr)
                ? (*(diagnosticsDataPtr->plbNodesPtr_)).at(r->Node->NodeIndex).NodeDescriptionObj.NodeName
                : L"");

            pref->AddBasicDiagnosticsInfo(move(info));
        }
    }

    return ConstraintCheckResult(make_unique<PreferredLocationSubspace>(this), move(invalidReplicas));
}

void PreferredLocationConstraint::GetTargetNodes(
    TempSolution const& tempSolution,
    PlacementReplica const* replica,
    NodeSet & candidateNodes,
    NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr) const
{
    UNREFERENCED_PARAMETER(nodeToConstraintDiagnosticsDataMapSPtr);

    PartitionEntry const* partition = replica->Partition;
    if (partition->Service->OnEveryNode)
    {
        return;
    }

    bool hasPreferredLocations = partition->ExistsUpgradeLocation || !partition->StandByLocations.empty();

    set<Common::TreeNodeIndex> const& upgradedUDs = PreferredLocationConstraint::GetUpgradedUDs(tempSolution, partition);
    bool hasUpgradeCompletedUDs = !upgradedUDs.empty();

    // preferred container placement is enabled if config for it is enabled and replica has some required images
    bool hasPreferredContainerPlacement =
        tempSolution.OriginalPlacement->Settings.PreferNodesForContainerPlacement &&
        replica->Partition->Service->ServicePackage != nullptr &&
        !replica->Partition->Service->ServicePackage->ContainerImages.empty();

    if (replica->IsNew && !hasPreferredLocations && !hasUpgradeCompletedUDs && !hasPreferredContainerPlacement)
    {
        return;
    }

    // Use prefer locations if exist, otherwise, use completed UDs
    NodeSet nodesUpgradedUDs(candidateNodes);
    if (!hasPreferredLocations && hasUpgradeCompletedUDs)
    {
        // Only prefer upgraded nodes if SB/Upgrade location is empty
        nodesUpgradedUDs.Filter([&](NodeEntry const *node) -> bool
        {
            return CheckUpgradedUDs(upgradedUDs, node);
        });
    }

    NodeSet nodesPreferredContainerPlacement(candidateNodes);
    if (hasPreferredContainerPlacement)
    {
        // for container replicas use nodes with images
        FilterPreferredNodesWithImages(nodesPreferredContainerPlacement, replica);
    }

    bool preferExistingReplicaLocations = tempSolution.OriginalPlacement->Settings.PreferExistingReplicaLocations;
    candidateNodes.Intersect([=](function<void(NodeEntry const *)> f)
    {
        // During singleton replica upgrade, there could be replicas which are not in upgrade,
        // but are correlated to the ones that are in upgrade (affinity or scaleout), 
        // and need to be moved together with replicas which are in upgrade
        if ((partition->IsInUpgrade || partition->IsInSingletonReplicaUpgradeOptimization()) && partition->ExistsUpgradeLocation)
        {
            if (partition->PrimaryUpgradeLocation)
            {
                f(partition->PrimaryUpgradeLocation);
            }
            else
            {
                for (auto it = partition->SecondaryUpgradeLocations.begin(); it != partition->SecondaryUpgradeLocations.end(); ++it)
                {
                    f(*it);
                }
            }
        }

        for (auto itNode = partition->StandByLocations.begin(); itNode != partition->StandByLocations.end(); ++itNode)
        {
            f(*itNode);
        }

        if (preferExistingReplicaLocations)
        {
            partition->ForEachExistingReplica([&f](PlacementReplica const* r)
            {
                f(r->Node);
            }, true, false);
        }
    });

    if (hasPreferredContainerPlacement)
    {
        // for container replicas use nodes with images
        candidateNodes.Union(nodesPreferredContainerPlacement);
    }

    if (!hasPreferredLocations && hasUpgradeCompletedUDs)
    {
        candidateNodes.Union(nodesUpgradedUDs);
    }
}

void PreferredLocationConstraint::FilterPreferredNodesWithImages(
    NodeSet & nodesPreferredContainerPlacement,
    PlacementReplica const* replica) const
{
    auto requiredImages = replica->Partition->Service->ServicePackage->ContainerImages;
    int minRequiredImages = requiredImages.size() == 1 ? 1 : static_cast<int>(floor((requiredImages.size() / 2)));
    // Filter out the nodes which have at least half of the number of the required images
    nodesPreferredContainerPlacement.Filter([&requiredImages, &minRequiredImages](NodeEntry const *node) -> bool
    {
        int numberOfRequiredImagesOnNode = 0;
        for (auto& image : requiredImages)
        {
            if (std::find(node->NodeImages.begin(), node->NodeImages.end(), image) != node->NodeImages.end())
            {
                numberOfRequiredImagesOnNode++;
            }
        }
        // Node is candidate if has at least half of the required images
        return numberOfRequiredImagesOnNode >= minRequiredImages;
    });
}

bool PreferredLocationConstraint::CheckReplica(TempSolution const& tempSolution, PlacementReplica const* replica, NodeEntry const* target) const
{
    ASSERT_IF(replica == nullptr || target == nullptr, "Replica or target node are empty {0} {1}", replica, target);

    PartitionEntry const* partition = replica->Partition;
    if (partition->Service->OnEveryNode)
    {
        return true;
    }

    bool hasPreferredLocations = partition->ExistsUpgradeLocation || !partition->StandByLocations.empty();
    set<Common::TreeNodeIndex> const& upgradedUDs = PreferredLocationConstraint::GetUpgradedUDs(tempSolution, partition);
    bool hasUpgradCompletedUDs = !upgradedUDs.empty();

    if (replica->IsNew && !hasPreferredLocations && !hasUpgradCompletedUDs)
    {
        return true;
    }

    // During singleton replica upgrade, there could be replicas which are not in upgrade,
    // but are correlated to the ones that are in upgrade (affinity or scaleout), 
    // and need to be moved together with replicas which are in upgrade
    if ((partition->IsInUpgrade || partition->IsInSingletonReplicaUpgradeOptimization()) && partition->ExistsUpgradeLocation)
    {
        if (partition->PrimaryUpgradeLocation && partition->PrimaryUpgradeLocation == target)
        {
            return true;
        }
        else
        {
            auto it = find(partition->SecondaryUpgradeLocations.begin(), partition->SecondaryUpgradeLocations.end(), target);
            if (it != partition->SecondaryUpgradeLocations.end())
            {
                return true;
            }
        }
    }

    vector<NodeEntry const*> const& locations = replica->Partition->StandByLocations;
    auto it = find(locations.begin(), locations.end(), target);
    if (it != locations.end())
    {
        return true;
    }

    // Check upgraded UDs for both cluster and application upgrade
    if (CheckUpgradedUDs(upgradedUDs, target))
    {
        return true;
    }


    // If no standby locations or replica is not in the locations, check if there is already replica in the target location;
    // This will help prefer swap over move
    bool preferExistingReplicaLocations = tempSolution.OriginalPlacement->Settings.PreferExistingReplicaLocations;
    if (preferExistingReplicaLocations)
    {
        PlacementReplica const* r = partition->GetReplica(target);
        if (r == nullptr)
        {
            return false;
        }
    }

    return true;
}

set<Common::TreeNodeIndex> const& PreferredLocationConstraint::GetUpgradedUDs(TempSolution const& tempSolution, PartitionEntry const* partition)
{

    if (!tempSolution.OriginalPlacement->Settings.PreferUpgradedUDs)
    {
        return PreferredLocationConstraint::EmptyUpgradedUDs;
    }

    set<Common::TreeNodeIndex> const& fabricUpgradedUDs = tempSolution.BaseSolution.OriginalPlacement->BalanceCheckerObj->UpgradeCompletedUDs;

    ApplicationEntry const* application = partition->Service->Application;
    if (!application)
    {
        return fabricUpgradedUDs;
    }

    set<Common::TreeNodeIndex> const& applicationUpgradedUDs = application->UpgradeCompletedUDs;

    if (!fabricUpgradedUDs.empty() && !applicationUpgradedUDs.empty())
    {
        // Has both application and fabric upgrade
        PlacementReplica const* primary = partition->PrimaryReplica;
        if (primary && primary->Node && primary->Node->UpgradeDomainIndex.Length != 0)
        {
            // If primary replica is in one of the upgraded UDs, the current upgrade is for the other type
            // If primary is NOT in any of them or in both of them, which means primary is not for any specific upgrade type
            // use the set that has more complete UDs
            TreeNodeIndex const& primaryUDindex = primary->Node->UpgradeDomainIndex;

            bool primaryInFabricUDs = (fabricUpgradedUDs.find(primaryUDindex) != fabricUpgradedUDs.end());
            bool primaryInAppUDs = (applicationUpgradedUDs.find(primaryUDindex) != applicationUpgradedUDs.end());

            if (primaryInFabricUDs && !primaryInAppUDs)
            {
                return applicationUpgradedUDs;
            }
            else if (primaryInAppUDs && !primaryInFabricUDs)
            {
                return fabricUpgradedUDs;
            }
        }

        set<Common::TreeNodeIndex> const& upgradedUDsToUse =
            (fabricUpgradedUDs.size() > applicationUpgradedUDs.size()) ? fabricUpgradedUDs : applicationUpgradedUDs;

        return upgradedUDsToUse;
    }
    else if (!applicationUpgradedUDs.empty())
    {
        // If fabric upgraded UDs is empty, but application upgrade is not
        return applicationUpgradedUDs;
    }

    return fabricUpgradedUDs;

}

bool PreferredLocationConstraint::CheckUpgradedUDs(set<Common::TreeNodeIndex> const& upgradedUDsToUse, NodeEntry const* target)
{
    TreeNodeIndex const& targetUDindex = target->UpgradeDomainIndex;
    if (0 == targetUDindex.Length || upgradedUDsToUse.empty())
    {
        return false;
    }

    return (upgradedUDsToUse.find(targetUDindex) != upgradedUDsToUse.end());

}
