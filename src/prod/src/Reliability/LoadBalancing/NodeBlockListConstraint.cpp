// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "PlacementReplica.h"
#include "PartitionEntry.h"
#include "ServiceEntry.h"
#include "NodeBlockListConstraint.h"
#include "TempSolution.h"
#include "NodeSet.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

void NodeBlockListConstraint::GetTargetNodes(
    TempSolution const& tempSolution,
    PlacementReplica const* replica,
    NodeSet & candidateNodes,
    NodeToConstraintDiagnosticsDataMapSPtr const nodeToConstraintDiagnosticsDataMapSPtr /* = nullptr */) const
{
    UNREFERENCED_PARAMETER(tempSolution);
    UNREFERENCED_PARAMETER(nodeToConstraintDiagnosticsDataMapSPtr);

    ExcludeServiceBlockList(candidateNodes, replica);
}

void NodeBlockListConstraint::ExcludeServiceBlockList(NodeSet & candidateNodes, PlacementReplica const * replica)
{
    // Remove block list for efficiency
    ServiceEntry const* service = replica->Partition->Service;
    candidateNodes.DeleteNodes(service->BlockList);
    if (replica->IsPrimary)
    {
        candidateNodes.DeleteNodes(service->PrimaryReplicaBlockList);
    }
}

bool NodeBlockListConstraint::CheckReplica(TempSolution const& tempSolution, PlacementReplica const* replica, NodeEntry const* target) const
{
    UNREFERENCED_PARAMETER(tempSolution);

    ASSERT_IF(replica == nullptr || target == nullptr, "Replica or target node are empty {0} {1}", replica, target);

    ServiceEntry const* service = replica->Partition->Service;

    return (!(service->IsNodeInBlockList(target) || (replica->IsPrimary && service->IsNodeInPrimaryReplicaBlockList(target))));
}


