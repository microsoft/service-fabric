// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Reliability;
using namespace Reliability::FailoverManagerComponent;
using namespace std;

UpgradeDomains::UpgradeDomains(UpgradeDomainSortPolicy::Enum sortPolicy)
    : upgradeDomains_()
    , sortPolicy_(sortPolicy)
    , isStale_(false)
{
}

UpgradeDomains::UpgradeDomains(UpgradeDomains const& other, UpgradeDomainSortPolicy::Enum sortPolicy)
    : upgradeDomains_(other.upgradeDomains_)
    , sortPolicy_(sortPolicy)
    , isStale_(false)
{
    NodeCache::SortUpgradeDomains(sortPolicy_, upgradeDomains_);
}

UpgradeDomains::UpgradeDomains(UpgradeDomains const& other)
    : upgradeDomains_(other.upgradeDomains_)
    , sortPolicy_(other.sortPolicy_)
    , isStale_(false)
{
}

bool UpgradeDomains::Exists(std::wstring const& upgradeDomain) const
{
    return (find(upgradeDomains_.begin(), upgradeDomains_.end(), upgradeDomain) != upgradeDomains_.end());
}

void UpgradeDomains::AddUpgradeDomain(wstring const& upgradeDomain)
{
    if (!Exists(upgradeDomain))
    {
        upgradeDomains_.push_back(upgradeDomain);
        NodeCache::SortUpgradeDomains(sortPolicy_, upgradeDomains_);
    }
}

void UpgradeDomains::RemoveUpgradeDomain(wstring const& upgradeDomain)
{
    if (Exists(upgradeDomain))
    {
        auto it = remove(upgradeDomains_.begin(), upgradeDomains_.end(), upgradeDomain);
        upgradeDomains_.erase(it, upgradeDomains_.end());
        NodeCache::SortUpgradeDomains(sortPolicy_, upgradeDomains_);
    }
}

size_t UpgradeDomains::GetUpgradeDomainIndex(wstring const& upgradeDomain) const
{
    auto it = lower_bound(
        upgradeDomains_.begin(),
        upgradeDomains_.end(),
        upgradeDomain,
        [this](wstring const& upgradeDomain1, wstring const& upgradeDomain2)
        {
            return NodeCache::CompareUpgradeDomains(sortPolicy_, upgradeDomain1, upgradeDomain2);
        });

    size_t upgradeDomainIndex = it - upgradeDomains_.begin();
    return upgradeDomainIndex;
}

wstring UpgradeDomains::GetUpgradeDomain(size_t upgradeDomainIndex) const
{
    if (upgradeDomainIndex >= upgradeDomains_.size())
    {
        return wstring();
    }

    return upgradeDomains_[upgradeDomainIndex];
}

bool UpgradeDomains::IsUpgradeComplete(std::wstring const& currentUpgradeDomain) const
{
    size_t currentUpgradeDomainIndex = GetUpgradeDomainIndex(currentUpgradeDomain);
    return (currentUpgradeDomainIndex >= upgradeDomains_.size());
}


bool UpgradeDomains::IsUpgradeComplete(size_t upgradeDomainIndex) const
{
    return (upgradeDomainIndex >= upgradeDomains_.size());
}

set<wstring> UpgradeDomains::GetCompletedUpgradeDomains(size_t currentUpgradeDomainIndex) const
{
    set<wstring> completedUpgradeDomains;
    for (size_t i = 0; i < currentUpgradeDomainIndex; i++)
    {
        completedUpgradeDomains.insert(upgradeDomains_[i]);
    }

    return completedUpgradeDomains;
}

void UpgradeDomains::GetProgress(
    size_t upgradeDomainIndex,
    _Out_ vector<std::wstring> & completedUDs,
    _Out_ vector<std::wstring> & pendingUDs) const
{
    for (size_t i = 0; i < upgradeDomainIndex; i++)
    {
        completedUDs.push_back(upgradeDomains_[i]);
    }

    for (size_t i = upgradeDomainIndex; i < upgradeDomains_.size(); i++)
    {
        pendingUDs.push_back(upgradeDomains_[i]);
    }
}

void UpgradeDomains::WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const
{
    w.Write(upgradeDomains_);
}
