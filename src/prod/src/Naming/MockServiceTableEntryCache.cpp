// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "MockServiceTableEntryCache.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ServiceModel;
using namespace Federation;
using namespace Naming;


MockServiceTableEntryCache::MockServiceTableEntryCache()
    : handler_()
    , handlerLock_()
    , generation_(DateTime::Now().Ticks, NodeId::MinNodeId)
    , versions_(make_shared<VersionRangeCollection>())
    , cacheIndex_()
    , cacheLock_()
{ 
    cacheIndex_ = make_unique<NotificationCacheIndex>(generation_.ToString());
}

MockServiceTableEntryCache::MockServiceTableEntryCache(uint64 generation)
    : handler_()
    , handlerLock_()
    , generation_(generation, NodeId::MinNodeId)
    , versions_(make_shared<VersionRangeCollection>())
    , cacheIndex_()
    , cacheLock_()
{ 
    cacheIndex_ = make_unique<NotificationCacheIndex>(generation_.ToString());
}

void MockServiceTableEntryCache::SetUpdateHandler(UpdateHandler const & handler)
{
    AcquireWriteLock lock(handlerLock_);

    handler_ = handler;
}

void MockServiceTableEntryCache::ClearUpdateHandler()
{
    AcquireWriteLock lock(handlerLock_);

    handler_ = nullptr;
}

RwLock & MockServiceTableEntryCache::GetCacheLock()
{
    return cacheLock_;
}

MatchedServiceTableEntryMap MockServiceTableEntryCache::GetUpdatesCallerHoldsCacheLock(
    ActivityId const & activityId,
    IClientRegistration const & clientRegistration,
    vector<ServiceNotificationFilterSPtr> const & filters,
    GenerationNumber const & clientGeneration,
    VersionRangeCollectionSPtr const & clientVersions,
    __out GenerationNumber & cacheGeneration,
    __out VersionRangeCollectionSPtr & cacheVersions,
    __out int64 & lastDeletedEmptyPartitionVersion)
{
    auto results = cacheIndex_->GetMatches(
        activityId,
        clientRegistration,
        filters,
        (clientGeneration == generation_ ? clientVersions : nullptr));

    cacheGeneration = generation_;
    cacheVersions = versions_;

    lastDeletedEmptyPartitionVersion = cacheIndex_->GetLastDeletedEmptyPartitionVersion();

    return results;
}

void MockServiceTableEntryCache::GetUpdateMetadata(
    __out GenerationNumber & cacheGeneration,
    __out int64 & lastDeletedEmptyPartitionVersion)
{
    cacheGeneration = generation_;
    lastDeletedEmptyPartitionVersion = cacheIndex_->GetLastDeletedEmptyPartitionVersion();
}

void MockServiceTableEntryCache::GetDeletedVersions(
    GenerationNumber const & clientGeneration,
    vector<VersionedConsistencyUnitId> const & partitionsTocheck,
    __out GenerationNumber & cacheGeneration,
    __out vector<int64> & deletedVersions)
{
    cacheGeneration = generation_;

    if (clientGeneration == generation_)
    {
        deletedVersions = cacheIndex_->GetDeletedVersions(partitionsTocheck);
    }
    else
    {
        deletedVersions = vector<int64>();
    }
}

void MockServiceTableEntryCache::GetCurrentVersions(
    __out GenerationNumber & cacheGeneration,
    __out VersionRangeCollectionSPtr & cacheVersions)
{
    cacheGeneration = generation_;
    cacheVersions = versions_;
}

void MockServiceTableEntryCache::UpdateCacheEntries(vector<ServiceTableEntrySPtr> const & partitions)
{
    auto startVersion = partitions.front()->Version;
    auto endVersion = partitions.back()->Version + 1;

    {
        AcquireWriteLock lock(cacheLock_);

        versions_->Add(VersionRange(startVersion, endVersion));

        auto cacheEntries = cacheIndex_->AddOrUpdateEntries(ActivityId(), partitions);

        auto handler = this->GetHandler();

        if (handler)
        {
            handler(
                ActivityId(),
                generation_,
                *versions_,
                cacheEntries,
                *make_shared<VersionRangeCollection>(startVersion, endVersion));
        }
    }
}

IServiceTableEntryCache::UpdateHandler MockServiceTableEntryCache::GetHandler()
{
    UpdateHandler handler = nullptr;

    {
        AcquireReadLock lock(handlerLock_);

        handler = handler_;
    }

    return handler;
}
