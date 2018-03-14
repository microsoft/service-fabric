// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

using namespace Management::ClusterManager;

StoreDataVerifiedFabricUpgradeDomains::StoreDataVerifiedFabricUpgradeDomains()
    : StoreData()
    , upgradeDomains_()
{
}

StoreDataVerifiedFabricUpgradeDomains::StoreDataVerifiedFabricUpgradeDomains(
    vector<wstring> && verifiedDomains)
    : StoreData()
    , upgradeDomains_(move(verifiedDomains))
{
}

wstring const & StoreDataVerifiedFabricUpgradeDomains::get_Type() const
{
    return Constants::StoreType_VerifiedFabricUpgradeDomains;
}

wstring StoreDataVerifiedFabricUpgradeDomains::ConstructKey() const
{
    return Constants::StoreKey_VerifiedFabricUpgradeDomains;
}

void StoreDataVerifiedFabricUpgradeDomains::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("VerifiedFabricUpgradeDomains: {0}", upgradeDomains_);
}
