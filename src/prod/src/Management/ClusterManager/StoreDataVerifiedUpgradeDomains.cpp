// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

using namespace Management::ClusterManager;

StoreDataVerifiedUpgradeDomains::StoreDataVerifiedUpgradeDomains()
    : StoreData()
    , applicationId_()
    , upgradeDomains_()
{
}

StoreDataVerifiedUpgradeDomains::StoreDataVerifiedUpgradeDomains(
    ServiceModelApplicationId const & appId)
    : StoreData()
    , applicationId_(appId)
    , upgradeDomains_()
{
}

StoreDataVerifiedUpgradeDomains::StoreDataVerifiedUpgradeDomains(
    ServiceModelApplicationId const & appId,
    vector<wstring> && verifiedDomains)
    : StoreData()
    , applicationId_(appId)
    , upgradeDomains_(move(verifiedDomains))
{
}

wstring const & StoreDataVerifiedUpgradeDomains::get_Type() const
{
    return Constants::StoreType_VerifiedUpgradeDomains;
}

wstring StoreDataVerifiedUpgradeDomains::ConstructKey() const
{
    wstring temp;
    StringWriter(temp).Write("{0}", applicationId_);
    return temp;
}

void StoreDataVerifiedUpgradeDomains::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("VerifiedUpgradeDomains({0}): {1}", applicationId_, upgradeDomains_);
}
