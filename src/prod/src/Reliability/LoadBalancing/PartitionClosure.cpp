// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "PartitionClosure.h"
#include "Service.h"
#include "Application.h"
#include "ServicePackage.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

PartitionClosureType::Enum PartitionClosureType::FromPLBSchedulerAction(PLBSchedulerActionType::Enum action)
{
    switch (action)
    {
    case PLBSchedulerActionType::NewReplicaPlacement:
        return PartitionClosureType::NewReplicaPlacement;

    case PLBSchedulerActionType::ConstraintCheck:
        return PartitionClosureType::ConstraintCheck;

    case PLBSchedulerActionType::QuickLoadBalancing:
    case PLBSchedulerActionType::LoadBalancing:
        return PartitionClosureType::Full;

    case PLBSchedulerActionType::NewReplicaPlacementWithMove:
        return PartitionClosureType::NewReplicaPlacementWithMove;

    default:
        // Currently we don't expect that phase can be "Upgrade"
        TESTASSERT_IFNOT(action == PLBSchedulerActionType::None || action == PLBSchedulerActionType::NoActionNeeded);
        return PartitionClosureType::None;
    }
}

PartitionClosure::PartitionClosure(
    vector<Service const*> && services,
    vector<FailoverUnit const*> && partitions,
    vector<Application const*> && applications,
    set<Common::Guid> && partialClosureFailoverUnits,
    vector<ServicePackage const*> && servicePackages,
    PartitionClosureType::Enum closureType)
    : services_(move(services)),
    partitions_(move(partitions)),
    applications_(move(applications)),
    partialClosureFailoverUnits_(move(partialClosureFailoverUnits)),
    servicePackages_(move(servicePackages)),
    type_(closureType)
{
}

PartitionClosure::PartitionClosure(PartitionClosure && other)
    : services_(move(other.services_)),
    partitions_(move(other.partitions_)),
    applications_(move(other.applications_)),
    partialClosureFailoverUnits_(move(other.partialClosureFailoverUnits_)),
    servicePackages_(move(other.servicePackages_)),
    type_(move(type_))
{
}
