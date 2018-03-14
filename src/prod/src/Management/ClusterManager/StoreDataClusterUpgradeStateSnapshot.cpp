// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

using namespace Management::HealthManager;
using namespace Management::ClusterManager;

StoreDataClusterUpgradeStateSnapshot::StoreDataClusterUpgradeStateSnapshot()
    : StoreData()
    , stateSnapshot_()
{
}

StoreDataClusterUpgradeStateSnapshot::StoreDataClusterUpgradeStateSnapshot(
    StoreDataClusterUpgradeStateSnapshot && other)
    : StoreData(move(other))
    , stateSnapshot_(move(other.stateSnapshot_))
{
}

StoreDataClusterUpgradeStateSnapshot::StoreDataClusterUpgradeStateSnapshot(
    ClusterUpgradeStateSnapshot && snapshot)
    : StoreData()
    , stateSnapshot_(move(snapshot))
{
}

wstring const & StoreDataClusterUpgradeStateSnapshot::get_Type() const
{
    return Constants::StoreType_FabricUpgradeStateSnapshot;
}

wstring StoreDataClusterUpgradeStateSnapshot::ConstructKey() const
{
    return Constants::StoreKey_FabricUpgradeStateSnapshot;
}

void StoreDataClusterUpgradeStateSnapshot::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("ClusterUpgradeStateSnapshot[{0}]", stateSnapshot_);
}
