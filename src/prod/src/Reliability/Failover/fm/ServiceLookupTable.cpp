// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Reliability;

StringLiteral const TraceComponent("ServiceLookupTable");

ServiceLookupTable::ServiceLookupTable(Common::ComponentRoot const & root)
    : Common::RootedObject(root), vEntries_(), idEntries_(), lockObject_()
{
}

bool ServiceLookupTable::TryUpdateEntryCallerHoldingLock(ServiceTableEntrySPtr && newEntry)
{
    if (!newEntry) { return false; }

    auto const & consistencyUnitId = newEntry->ConsistencyUnitId;
    auto const & newVersion = newEntry->Version;

    auto i = idEntries_.insert(make_pair(consistencyUnitId, newEntry));
    if (!i.second)
    {
        auto currentVersion = i.first->second->Version;
        if (currentVersion >= newVersion)
        {
            // The incoming entry is known or stale, ignore it.
            return false;
        }

        i.first->second = newEntry;

        // Remove the existing entries
        vEntries_.erase(currentVersion);
    }

    vEntries_.insert(make_pair(newVersion, consistencyUnitId));

    if (Assert::IsTestAssertEnabled())
    {
        if (idEntries_.size() != vEntries_.size())
        {
            auto msg = wformatString("Item count doesn't match: idEntries: {0}, vEntries_: {1}", idEntries_.size(), vEntries_.size());

            Trace.WriteError(TraceComponent, "{0}", msg);

            for (auto const & id : idEntries_)
            {
                Trace.WriteError(TraceComponent, "id: {0}->{1}", id.first, *id.second);
            }

            for (auto const & v : vEntries_)
            {
                Trace.WriteError(TraceComponent, "v: {0}->{1}", v.first, v.second);
            }

            Assert::TestAssert("{0}", msg);
        }
    }

    return true;
}

bool ServiceLookupTable::TryGetEntry(ConsistencyUnitId const & cuid, __out ServiceTableEntry & entry) const
{
    AcquireReadLock lock(lockObject_);
    return TryGetEntryCallerHoldsReadLock(cuid, entry);
}

bool ServiceLookupTable::TryGetEntryCallerHoldsReadLock(ConsistencyUnitId const& cuid, _Out_ ServiceTableEntry & entry) const
{
    auto entryIter = idEntries_.find(cuid);
    if (entryIter == idEntries_.end())
    {
        return false;
    }
    else
    {
        entry = *(entryIter->second);
        return true;
    }
}

__int64 ServiceLookupTable::GetVersionForEntryCallerHoldingLock(ConsistencyUnitId const & cuid) const
{
    auto entryIter = idEntries_.find(cuid);
    if (entryIter == idEntries_.end())
    {
        return -1;
    }
    else
    {
        return entryIter->second->Version;
    }
}

int64 ServiceLookupTable::GetUpdatedEntriesCallerHoldingLock(
    size_t pageSizeLimit,
    vector<ServiceTableEntry> & entries,
    VersionRangeCollection const& rangeToExclude) const
{
    vector<VersionRange> const & excludeRanges = rangeToExclude.VersionRanges;

    for (size_t i = 0; i <= excludeRanges.size(); i++)
    {
        int64 low = (i == 0 ? 0 : excludeRanges[i - 1].EndVersion);
        int64 high = (i == excludeRanges.size() ? numeric_limits<int>::max() : excludeRanges[i].StartVersion);

        for (auto it = vEntries_.lower_bound(low); it != vEntries_.end() && it->first < high; ++it)
        {
            auto const & entry = idEntries_.at(it->second);

            size_t currentEntrySize = entry->EstimateSize();
            if (pageSizeLimit < currentEntrySize)
            {
                return it->first;
            }

            entries.push_back(*entry);
            pageSizeLimit -= currentEntrySize;

        }
    }

    return 0;
}

bool ServiceLookupTable::TryRemoveEntryCallerHoldingLock(ConsistencyUnitId const& consistencyUnitId, int64 versionToRemove)
{
    bool isRemoved = false;

    auto i = idEntries_.find(consistencyUnitId);
    if (i != idEntries_.end())
    {
        auto const & currentEntry = i->second;
        auto const & currentVersion = currentEntry->Version;

        if (currentVersion < versionToRemove)
        {
            vEntries_.erase(currentVersion);
            idEntries_.erase(consistencyUnitId);

            isRemoved = true;
            if (idEntries_.size() != vEntries_.size())
            {
                Assert::TestAssert("Item count doesn't match: idEntries: {0}, vEntries_: {1}", idEntries_.size(), vEntries_.size());
            }
        }
    }

    return isRemoved;
}

void ServiceLookupTable::ClearCallerHoldingLock()
{
    idEntries_.clear();
    vEntries_.clear();
}
