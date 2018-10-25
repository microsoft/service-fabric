// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "IFailoverManager.h"
#include "IPlacementAndLoadBalancing.h"
#include "PlacementAndLoadBalancing.h"


using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

IPlacementAndLoadBalancingUPtr IPlacementAndLoadBalancing::Create(
    IFailoverManager & failoverManager,
    ComponentRoot const& root,
    bool isMaster,
    bool movementEnabled,
    vector<NodeDescription> && nodes,
    vector<ApplicationDescription> && applications,
    vector<ServiceTypeDescription> && serviceTypes,
    vector<ServiceDescription> && services,
    vector<FailoverUnitDescription> && failoverUnits,
    vector<LoadOrMoveCostDescription> && loadAndMoveCosts,
    Client::HealthReportingComponentSPtr const & healthClient,
    Api::IServiceManagementClientPtr const& serviceManagementClient,
    Common::ConfigEntry<bool> const& isSingletonReplicaMoveAllowedDuringUpgradeEntry)
{
    return make_unique<PlacementAndLoadBalancing>(
        failoverManager,
        root,
        isMaster,
        movementEnabled,
        move(nodes),
        move(applications),
        move(serviceTypes),
        move(services),
        move(failoverUnits),
        move(loadAndMoveCosts),
        healthClient,
        serviceManagementClient,
        isSingletonReplicaMoveAllowedDuringUpgradeEntry);
}
