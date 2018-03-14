// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Failover.stdafx.h"

namespace Reliability
{
    using namespace std;
    using namespace Common;

    ServiceResolverCache::ServiceResolverCache(ComponentRoot const & root)
      : ServiceLookupTable(root)
      , generationNumber_()
      , knownVersions_()
      , cacheIndex_(make_unique<NotificationCacheIndex>(root.TraceId))
      , handler_()
      , nextNotificationId_()
    {
    }

    ServiceResolverCache::~ServiceResolverCache()
    {
        if (cacheIndex_)
        {
            cacheIndex_->Clear();
        }
    }

    GenerationNumber ServiceResolverCache::get_GenerationNumber() const
    { 
        AcquireReadLock lock(this->ServiceLookupTableLock);
        return generationNumber_; 
    }

    bool ServiceResolverCache::IsUpdateNeeded_CallerHoldsLock(ServiceTableEntry const & entry, GenerationNumber const & generationNumber) const
    {
        if (generationNumber_ != generationNumber)
        {
            return (generationNumber_ < generationNumber);
        }

        return (GetVersionForEntryCallerHoldingLock(entry.ConsistencyUnitId) < entry.Version);
    }

    bool ServiceResolverCache::StoreUpdate_CallerHoldsLock(
        vector<ServiceTableEntry> const & newEntries,
        GenerationNumber const & generationNumber,
        VersionRangeCollection const & coveredRanges,
        int64 endVersion,
        __inout ServiceCuidMap & updatedCuids,
        __inout ServiceCuidMap & removedCuids)
    {
        updatedCuids.clear();
        removedCuids.clear();

        vector<ServiceTableEntrySPtr> indexedCacheEntries;
        indexedCacheEntries.reserve(newEntries.size());

        if (this->generationNumber_ > generationNumber)
        {
            WriteInfo(
                Constants::ServiceResolverSource,
                Root.TraceId,
                "Incoming generation {0} is lower than local generation {1}",
                generationNumber,
                this->generationNumber_);
            return true;
        }

        bool acceptAll = false;
        if (this->generationNumber_ < generationNumber)
        {
            WriteInfo(
                Constants::ServiceResolverSource,
                Root.TraceId,
                "Updating generation: {0} -> {1}",
                generationNumber_,
                generationNumber);

            acceptAll = true;

            generationNumber_ = generationNumber;
            knownVersions_.Clear();
            ClearCallerHoldingLock();
        }

        for (auto const & iter : newEntries)
        {
            bool isUpdated = false;

            auto tableEntry = make_shared<ServiceTableEntry>(iter);
            auto tableEntryPtr = tableEntry.get();

            if (acceptAll || this->GetVersionForEntryCallerHoldingLock(iter.ConsistencyUnitId) < iter.Version)
            {
                WriteNoise(
                    Constants::ServiceResolverSource, 
                    Root.TraceId, 
                    "Broadcast updated for name={0} cuid={1} vers={2}",
                    tableEntry->ServiceName,
                    iter.ConsistencyUnitId,
                    iter.Version);

                updatedCuids[tableEntry->ServiceName].push_back(tableEntry->ConsistencyUnitId);
            }
            else
            {
                WriteNoise(
                    Constants::ServiceResolverSource, 
                    Root.TraceId, 
                    "Broadcast stale for name={0} cuid={1} vers={2}",
                    tableEntry->ServiceName,
                    iter.ConsistencyUnitId,
                    iter.Version);
            }

            // Do not filter indexed updates since the updates in
            // the notification must match the update versions
            // from the FM in the original broadcast.
            //
            // This means that duplicate FM broadcasts (by design)
            // will also be forwarded to the client. If this
            // becomes a scalability issue, then we will need to 
            // come up with an optimization that does not compromise the
            // correctness of the version ranges.
            //
            indexedCacheEntries.push_back(tableEntry);

            if (!tableEntry->IsFound)
            {
                if (coveredRanges.IsEmpty)
                {
                    WriteNoise(
                        Constants::ServiceResolverSource,
                        Root.TraceId,
                        "Empty range for CUID {0}",
                        tableEntry->ConsistencyUnitId);

                    // ignore this entry
                }
                else 
                {
                    // EndVersion is exclusive, which means that the next update
                    // could be equal to EndVersion
                    //
                    int64 highRange = coveredRanges.VersionRanges.back().EndVersion - 1;
                    if (TryRemoveEntryCallerHoldingLock(tableEntry->ConsistencyUnitId, highRange))
                    {
                        WriteInfo(
                            Constants::ServiceResolverSource,
                            Root.TraceId,
                            "Removed cache entry for CUID {0} ({1}): partition not found",
                            tableEntry->ConsistencyUnitId,
                            highRange);
                        isUpdated = true;

                        // Notification indexing will treat empty entries as deletes
                        //
                        tableEntry->EnsureEmpty(highRange);

                        removedCuids[tableEntry->ServiceName].push_back(tableEntry->ConsistencyUnitId);
                    }
                }
            }
            else if (tableEntry->ServiceReplicaSet.IsEmpty())
            {
                if (TryRemoveEntryCallerHoldingLock(tableEntry->ConsistencyUnitId, tableEntry->Version))
                {
                    WriteInfo(
                        Constants::ServiceResolverSource,
                        Root.TraceId,
                        "Removed cache entry for CUID {0} ({1}): no replicas",
                        tableEntry->ConsistencyUnitId,
                        tableEntry->Version);
                    isUpdated = true;

                    removedCuids[tableEntry->ServiceName].push_back(tableEntry->ConsistencyUnitId);
                }
            }
            else
            {
                if (TryUpdateEntryCallerHoldingLock(move(tableEntry)))
                {
                    WriteNoise(
                        Constants::ServiceResolverSource,
                        Root.TraceId,
                        "Updated cache entry for CUID {0}: {1}",
                        tableEntryPtr->ConsistencyUnitId,
                        tableEntryPtr->ServiceReplicaSet);
                    isUpdated = true;
                }
            }

            if (!isUpdated)
            {
                WriteNoise(
                    Constants::ServiceResolverSource, 
                    Root.TraceId, 
                    "Ignored update for CUID {0} : {1}",
                    tableEntryPtr->ConsistencyUnitId,
                    tableEntryPtr->ServiceReplicaSet);
            }
        } // for newEntries

        knownVersions_.Merge(coveredRanges);
         
        bool isUpToDate = ((knownVersions_.VersionRanges.size() == 1) && 
                           (knownVersions_.EndVersion >= endVersion) &&
                           (knownVersions_.StartVersion <= 1));

        nextNotificationId_.IncrementIndex();

        WriteNoise(
            Constants::ServiceResolverSource, 
            Root.TraceId, 
            "{0}: StoreUpdate: known ranges={1}, new data: generation={2}, endVersion={3}, count={4}, upToDate={5}",
            nextNotificationId_,
            knownVersions_,
            generationNumber.Value,
            endVersion, 
            newEntries.size(),
            isUpToDate);

        auto cacheEntries = cacheIndex_->AddOrUpdateEntries(
            nextNotificationId_,
            indexedCacheEntries);

        if (handler_ && !cacheEntries.empty())
        {
            // This handler is intentionally called under ServiceLookupTableLock.
            // This hooks into the ServiceNotificationManager for matching against
            // registered notification filters. Any resulting matches must be
            // enqueued for delivery under the lock before processing the next broadcast
            // for correct management of versions in delivered notifications.
            //
            // For example, given two incoming broadcasts:
            //
            // A: new known versions = [0, 10]
            // B: new known versions = [0, 15]
            //
            // B cannot be enqueued before A because the cache range of B
            // covers A, which makes it look like A contains duplicates.
            // ServiceNotificationSender takes care of delivering notifications
            // with the correct versions out of order (using coveredRanges) 
            // as long as they are enqueued in the correct order.
            //
            handler_(
                nextNotificationId_,
                generationNumber_,
                knownVersions_,
                cacheEntries,
                coveredRanges);
        }
       
        return isUpToDate;
    }

    bool ServiceResolverCache::TryGetEntryWithCacheMode(
        VersionedCuid const & partition, 
        CacheMode::Enum refreshMode,
        __out ServiceTableEntry & cachedEntry) const
    {
        if (refreshMode == CacheMode::BypassCache || !this->TryGetEntry(partition.Cuid, cachedEntry))
        {            
            return false;
        }
        else
        {
            if (refreshMode == CacheMode::UseCached)
            {
                return true;
            }
            else
            {
                // if the cache's version is equal to the last used version, then the cache is likely stale
                if (partition.Generation == generationNumber_)
                {
                    return partition.Version < cachedEntry.Version;
                }
                else
                {
                    return partition.Generation < generationNumber_;
                }
            }
        }
    }

    VersionRangeCollection ServiceResolverCache::GetKnownVersions()
    {
        AcquireReadLock lock(this->ServiceLookupTableLock);
        return knownVersions_;
    }

    //
    // IServiceTableEntryCache
    //
    
    void ServiceResolverCache::SetUpdateHandler(IServiceTableEntryCache::UpdateHandler const & handler) 
    { 
        AcquireWriteLock lock(this->ServiceLookupTableLock);

        handler_ = handler;
    }

    void ServiceResolverCache::ClearUpdateHandler()
    { 
        AcquireWriteLock lock(this->ServiceLookupTableLock);

        handler_ = nullptr;
    }

    RwLock & ServiceResolverCache::GetCacheLock()
    {
        return this->ServiceLookupTableLock;
    }

    MatchedServiceTableEntryMap ServiceResolverCache::GetUpdatesCallerHoldsCacheLock(
        ActivityId const & activityId,
        IClientRegistration const & clientRegistration,
        std::vector<ServiceNotificationFilterSPtr> const & filters,
        GenerationNumber const & clientGeneration,
        Common::VersionRangeCollectionSPtr const & clientVersions,
        __out GenerationNumber & cacheGeneration,
        __out Common::VersionRangeCollectionSPtr & cacheVersions,
        __out int64 & lastDeletedEmptyPartitionVersion) 
    { 
        auto results = cacheIndex_->GetMatches(
            activityId,
            clientRegistration,
            filters,
            (clientGeneration == generationNumber_ ? clientVersions : nullptr));

        cacheGeneration = generationNumber_;
        cacheVersions = make_shared<VersionRangeCollection>(knownVersions_);

        lastDeletedEmptyPartitionVersion = cacheIndex_->GetLastDeletedEmptyPartitionVersion();

        return results;
    }

    void ServiceResolverCache::GetUpdateMetadata(
        __out GenerationNumber & cacheGeneration,
        __out int64 & lastDeletedEmptyPartitionVersion) 
    { 
        AcquireReadLock lock(this->ServiceLookupTableLock);

        cacheGeneration = generationNumber_;
        lastDeletedEmptyPartitionVersion = cacheIndex_->GetLastDeletedEmptyPartitionVersion();
    }

    void ServiceResolverCache::GetDeletedVersions(
        GenerationNumber const & clientGeneration,
        std::vector<VersionedConsistencyUnitId> const & partitionsTocheck,
        __out GenerationNumber & cacheGeneration,
        __out std::vector<int64> & deletedVersions)
    {
        AcquireReadLock lock(this->ServiceLookupTableLock);

        cacheGeneration = generationNumber_;

        if (clientGeneration == generationNumber_)
        {
            deletedVersions = cacheIndex_->GetDeletedVersions(partitionsTocheck);
        }
        else
        {
            deletedVersions = vector<int64>();
        }
    }

    void ServiceResolverCache::GetCurrentVersions(
        __out GenerationNumber & cacheGeneration,
        __out Common::VersionRangeCollectionSPtr & cacheVersions)
    {
        AcquireReadLock lock(this->ServiceLookupTableLock);

        cacheGeneration = generationNumber_;
        cacheVersions = make_shared<VersionRangeCollection>(knownVersions_);
    }
}
