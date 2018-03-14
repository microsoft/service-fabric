// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Failover.stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace Transport;
using namespace ServiceModel;

StringLiteral const TraceComponent("NotificationCacheIndex");

// NameIndexEntry - Each CachedServiceTableEntry wraps a single ServiceTableEntry,
//                  which is a single partition. This class groups together all
//                  CachedServiceTableEntry objects belonging to the same service
//                  since indexing for notifications is based on service name
//                  rather than partition IDs.
//

class NotificationCacheIndex::NameIndexEntry 
    : public ICachedServiceTableEntryOwner
    , public enable_shared_from_this<ICachedServiceTableEntryOwner>
{
    DENY_COPY(NameIndexEntry)

private:
    typedef std::map<ConsistencyUnitId, CachedServiceTableEntrySPtr> CachedEntryMap;

public:
    NameIndexEntry(NamingUri const & name)
        : uri_(name)
        , cachedEntriesByCuid_()
    {
    }

    __declspec(property(get=get_IsEmpty)) bool IsEmpty;
    bool get_IsEmpty() const { return cachedEntriesByCuid_.empty(); }

    __declspec(property(get=get_CachedEntries)) CachedEntryMap const & CachedEntries;
    CachedEntryMap get_CachedEntries() const { return cachedEntriesByCuid_; }

    void Clear() { cachedEntriesByCuid_.clear(); }

public:

    //
    // ICachedServiceTableEntryOwner
    //

    virtual NamingUri const & GetUri() const { return uri_; }

public:

    CachedServiceTableEntrySPtr AddOrUpdateEntry(
        ActivityId const & activityId,
        ServiceTableEntrySPtr const & entry,
        __out int64 & previousVersion)
    {
        CachedServiceTableEntrySPtr cachedEntry;
        previousVersion = -1;

        auto findIt = cachedEntriesByCuid_.find(entry->ConsistencyUnitId);
        if (findIt == cachedEntriesByCuid_.end())
        {
            cachedEntry = make_shared<CachedServiceTableEntry>(
                activityId,
                entry,
                shared_from_this());

            cachedEntriesByCuid_.insert(make_pair(
                entry->ConsistencyUnitId,
                cachedEntry));
        }
        else
        {
            cachedEntry = findIt->second;

            previousVersion = cachedEntry->Version;

            cachedEntry->UpdateServiceTableEntry(activityId, entry);
        }

        return cachedEntry;
    }

    vector<CachedServiceTableEntrySPtr> TryGetCachedServiceTableEntries(
        ActivityId const & activityId,
        VersionRangeCollectionSPtr const & clientVersions,
        bool primaryChangeOnly)
    {
        vector<CachedServiceTableEntrySPtr> results;

        for (auto const & it : cachedEntriesByCuid_)
        {
            ServiceTableEntrySPtr unusedEntry;
            if (it.second->TryGetServiceTableEntry(
                activityId,
                clientVersions,
                primaryChangeOnly,
                unusedEntry))
            {
                results.push_back(it.second);
            }
        }

        return results;
    }

    void DeleteEntry(CachedServiceTableEntrySPtr const & cachedEntry)
    {
        auto cuid = cachedEntry->Partition->ConsistencyUnitId;

        cachedEntriesByCuid_.erase(cuid);
    }

private:

    NamingUri uri_;
    CachedEntryMap cachedEntriesByCuid_;
};

//
// NotificationCacheIndex
//

NotificationCacheIndex::NotificationCacheIndex(wstring const & traceId)
    : traceId_(traceId)
    , indexEntriesByName_()
    , emptyPartitionsByVersion_()
    , allPartitionsByCuid_()
    , lastDeletedEmptyPartitionVersion_(0)
{
}

NotificationCacheIndex::~NotificationCacheIndex()
{
}

void NotificationCacheIndex::Clear()
{
    //
    // Release reverse reference on ICachedServiceTableEntryOwner
    //
    for (auto const & entry : indexEntriesByName_)
    {
        entry.second->Clear();
    }

    indexEntriesByName_.clear();
    emptyPartitionsByVersion_.clear();
    allPartitionsByCuid_.clear();
    lastDeletedEmptyPartitionVersion_ = 0;
}

vector<CachedServiceTableEntrySPtr> NotificationCacheIndex::AddOrUpdateEntries(
    ActivityId const & activityId,
    vector<ServiceTableEntrySPtr> const & entries)
{
    vector<CachedServiceTableEntrySPtr> results;

    for (auto const & entry : entries)
    {
        auto result = this->AddOrUpdateEntry(activityId, entry);
        if (result)
        {
            results.push_back(result);
        }
    }

    return results;
}

CachedServiceTableEntrySPtr NotificationCacheIndex::AddOrUpdateEntry(
    ActivityId const & activityId,
    ServiceTableEntrySPtr const & entry)
{
    CachedServiceTableEntrySPtr result;

    // Do not index system service entries
    //
    NamingUri uri;
    if (NamingUri::TryParse(entry->ServiceName, uri))
    {
        NameIndexEntrySPtr indexEntry;
        int64 previousVersion = 0;

        auto findIt = indexEntriesByName_.find(uri);
        if (findIt == indexEntriesByName_.end())
        {
            indexEntry = make_shared<NameIndexEntry>(uri);

            indexEntriesByName_.insert(make_pair(
                    uri, 
                    indexEntry));
        }
        else
        {
            indexEntry = findIt->second;
        }

        result = indexEntry->AddOrUpdateEntry(activityId, entry, previousVersion);

        this->UpdateVersionMapAndTrimEmptyPartitions(activityId, previousVersion, result);
    }

    return result;
}

MatchedServiceTableEntryMap NotificationCacheIndex::GetMatches(
    ActivityId const & activityId,
    IClientRegistration const & clientRegistration,
    vector<ServiceNotificationFilterSPtr> const & filters,
    VersionRangeCollectionSPtr const & clientVersions)
{
    MatchedServiceTableEntryMap results;

    WriteInfo(
        TraceComponent, 
        this->TraceId,
        "{0}: matching {1} filters: vers={2}", 
        activityId,
        filters.size(), 
        (clientVersions ? *clientVersions : VersionRangeCollection()));

    for (auto const & filter : filters)
    {
        WriteInfo(
            TraceComponent, 
            this->TraceId,
            "{0}: matching filter id={1}",
            activityId,
            filter->FilterId);

        if (filter->IsPrefix)
        {
            this->GetPrefixMatches(activityId, clientRegistration, filter, clientVersions, results);
        }
        else
        {
            this->GetExactMatch(activityId, filter, clientVersions, results);
        }
    }

    return results;
}

void NotificationCacheIndex::UpdateVersionMapAndTrimEmptyPartitions(
    ActivityId const & activityId,
    int64 previousVersion,
    CachedServiceTableEntrySPtr const & updatedEntry)
{
    emptyPartitionsByVersion_.erase(previousVersion);

    auto findIt = allPartitionsByCuid_.find(updatedEntry->Partition->ConsistencyUnitId);
    if (findIt == allPartitionsByCuid_.end())
    {
        allPartitionsByCuid_.insert(updatedEntry->Partition->ConsistencyUnitId);
    }

    if (!updatedEntry->Partition->ServiceReplicaSet.IsEmpty())
    {
        // No-op: Only trim empty partitions if we are updating empty partitions
        //
        return;
    }

    size_t emptyPartitionLimit = ServiceModelConfig::GetConfig().MaxIndexedEmptyPartitions;

    emptyPartitionsByVersion_.insert(make_pair(
        updatedEntry->Version,
        updatedEntry));

    vector<CachedServiceTableEntrySPtr> deletedEntries;

    while (emptyPartitionsByVersion_.size() > emptyPartitionLimit)
    {
        auto it = emptyPartitionsByVersion_.begin();
        if (it != emptyPartitionsByVersion_.end())
        {
            deletedEntries.push_back(it->second);

            emptyPartitionsByVersion_.erase(it);
        }
    }
    
    if (deletedEntries.empty())
    {
        return;
    }

    lastDeletedEmptyPartitionVersion_ = deletedEntries.back()->Version;

    auto const & remaining = emptyPartitionsByVersion_;

    WriteInfo(
        TraceComponent, 
        this->TraceId,
        "{0}: deleting empty partition versions {1}: limit={2} remaining={3}",
        activityId,
        deletedEntries.size() == 1 
            ? wformatString("{0}", lastDeletedEmptyPartitionVersion_)
            : wformatString("[{0}, {1}]", deletedEntries.front()->Version, deletedEntries.back()->Version),
        emptyPartitionLimit,
        remaining.empty() 
            ? L"[]"
            : wformatString("[{0}, {1}]", remaining.begin()->first, remaining.rbegin()->first));

    for (auto const & it : deletedEntries)
    {
        auto findByNameIt = indexEntriesByName_.find(it->Uri);
        if (findByNameIt != indexEntriesByName_.end())
        {
            auto const & indexEntry = findByNameIt->second;

            indexEntry->DeleteEntry(it);

            if (indexEntry->IsEmpty)
            {
                indexEntriesByName_.erase(findByNameIt);
            }
        }

        allPartitionsByCuid_.erase(it->Partition->ConsistencyUnitId);
    }
}

int64 NotificationCacheIndex::GetLastDeletedEmptyPartitionVersion()
{
    return lastDeletedEmptyPartitionVersion_;
}

size_t NotificationCacheIndex::GetEmptyPartitionsCount()
{
    return emptyPartitionsByVersion_.size();
}

size_t NotificationCacheIndex::GetAllPartitionsCount()
{
    return allPartitionsByCuid_.size();
}


vector<int64> NotificationCacheIndex::GetDeletedVersions(
    vector<VersionedConsistencyUnitId> const & partitionsTocheck)
{
    vector<int64> deletedVersions;

    for (auto partition : partitionsTocheck)
    {
        if (allPartitionsByCuid_.find(partition.Cuid) == allPartitionsByCuid_.end())
        {
            deletedVersions.push_back(partition.Version);
        }
    }

    return deletedVersions;
}

void NotificationCacheIndex::GetExactMatch(
    ActivityId const & activityId,
    ServiceNotificationFilterSPtr const & filter,
    VersionRangeCollectionSPtr const & clientVersions,
    __inout MatchedServiceTableEntryMap & results)
{
    auto it = indexEntriesByName_.find(filter->Name);
    if (it != indexEntriesByName_.end())
    {
        auto const & indexEntry = it->second;

        auto cachedEntries = indexEntry->TryGetCachedServiceTableEntries(
            activityId,
            clientVersions,
            filter->IsPrimaryOnly);

        for (auto const & cachedEntry : cachedEntries)
        {
            this->AddMatchedEntryResult(
                cachedEntry, 
                filter->IsPrimaryOnly,
                results);
        }
    }
    else
    {
        WriteNoise(
            TraceComponent, 
            this->TraceId,
            "{0}: {1} [none]",
            activityId,
            filter->Name); 
    }
}

void NotificationCacheIndex::GetPrefixMatches(
    ActivityId const & activityId,
    IClientRegistration const & clientRegistration,
    ServiceNotificationFilterSPtr const & filter,
    VersionRangeCollectionSPtr const & clientVersions,
    __inout MatchedServiceTableEntryMap & results)
{
    auto const & prefixName = filter->Name;

    // Seek to position preceeding first possible prefix match
    // before iterating through remainder of index.
    //
    // Note that there is another possible heuristic here.
    // We can also diff the client and cache version ranges to
    // determine which versions are potentially interesting to
    // the client and lookup each entry using the version.
    // This alternative is better if re-connecting clients
    // tend to be mostly up-to-date and use very broad
    // prefix filters.
    //
    // It is also possible to combine the two approaches.
    // For example, search for the upper and lower bounds
    // and compare the total number of potential matches by
    // name to the number of potential matches by version.
    // The approach with the smaller number of potential
    // matches can then be used. There is some upfront
    // cost to both finding the bounds and computing the
    // version range diff, so the benefits will depend
    // largely on workload patterns.
    //
    auto it = std::lower_bound(
        indexEntriesByName_.begin(), 
        indexEntriesByName_.end(), 
        make_pair(prefixName, NameIndexEntrySPtr()),
        [](NameEntryPair const & left, NameEntryPair const & right) -> bool
        {
            return (left.first < right.first);
        });

    auto prefixNameString = prefixName.ToString();

    while (it != indexEntriesByName_.end())
    {
        if (clientRegistration.IsSenderStopped())
        {
            WriteInfo(
                TraceComponent, 
                this->TraceId,
                "{0}: aborting notification filter processing",
                activityId);

            results.clear();

            break;
        }

        auto const & entryUri = it->first;
        auto const & indexEntry = it->second;

        if (prefixName.IsPrefixOf(it->first) || prefixName == it->first)
        {
            auto cachedEntries = indexEntry->TryGetCachedServiceTableEntries(
                activityId,
                clientVersions,
                filter->IsPrimaryOnly);

            for (auto const & cachedEntry : cachedEntries)
            {
                this->AddMatchedEntryResult(
                    cachedEntry, 
                    filter->IsPrimaryOnly,
                    results);
            }

            ++it;
        }
        else if (StringUtility::StartsWith(entryUri.ToString(), prefixNameString))
        {
            WriteNoise(
                TraceComponent, 
                this->TraceId,
                "{0}: {1} -> {2} [skip]",
                activityId,
                prefixName, 
                entryUri);

            ++it;
        }
        else
        {
            WriteNoise(
                TraceComponent, 
                this->TraceId,
                "{0}: {1} -> {2} [done]",
                activityId,
                prefixName, 
                entryUri);

            break;
        }
    } // while not end of cache entries
}

void NotificationCacheIndex::AddMatchedEntryResult(
    CachedServiceTableEntrySPtr const & cachedEntry,
    bool matchedPrimaryOnly,
    __in MatchedServiceTableEntryMap & results)
{
    auto findIt = results.find(cachedEntry->Version);
    if (findIt == results.end())
    {
        auto matchedEntry = MatchedServiceTableEntry::CreateOnConnectionMatch(
            cachedEntry, 
            matchedPrimaryOnly);
                
        results.insert(make_pair(
            matchedEntry->Version,
            matchedEntry));
    }
    else
    {
        findIt->second->UpdateMatchedPrimaryOnly(matchedPrimaryOnly);
    }
}
