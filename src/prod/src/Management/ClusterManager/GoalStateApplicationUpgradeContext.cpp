// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace ServiceModel;
using namespace Management::ClusterManager;

StringLiteral const TraceComponent("GoalStateApplicationUpgradeContext");

GoalStateApplicationUpgradeContext::GoalStateApplicationUpgradeContext(
    ComponentRoot const & root,
    ClientRequestSPtr const & clientRequest,
    ApplicationUpgradeDescription const & description,
    ServiceModelVersion const & targetVersion,
    std::wstring const & targetApplicationManifestId,
    uint64 upgradeInstance)
    : ApplicationUpgradeContext(
        root,
        clientRequest,
        description,
        targetVersion,
        targetApplicationManifestId,
        upgradeInstance)
{
}

GoalStateApplicationUpgradeContext::GoalStateApplicationUpgradeContext(NamingUri const & applicationName)
    : ApplicationUpgradeContext(applicationName)
{
}

std::wstring const & GoalStateApplicationUpgradeContext::get_Type() const
{
    return Constants::StoreType_GoalStateApplicationUpgradeContext;
}

StringLiteral const & GoalStateApplicationUpgradeContext::GetTraceComponent() const
{
    return TraceComponent;
}
