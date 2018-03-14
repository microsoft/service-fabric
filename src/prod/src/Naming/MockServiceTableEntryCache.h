// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class MockServiceTableEntryCache : public Reliability::IServiceTableEntryCache
    {
    public:
        MockServiceTableEntryCache();
        explicit MockServiceTableEntryCache(uint64 generation);

        Common::VersionRangeCollection const & GetVersions() const 
        { 
            return *versions_; 
        }

        size_t GetEmptyPartitionsCount() const
        {
            return cacheIndex_->GetEmptyPartitionsCount();
        }

        size_t GetAllPartitionsCount() const
        {
            return cacheIndex_->GetAllPartitionsCount();
        }

    public:

        //
        // IServiceTableEntryCache
        //
        virtual void SetUpdateHandler(UpdateHandler const & handler);

        virtual void ClearUpdateHandler();

        virtual Common::RwLock & GetCacheLock();

        virtual Reliability::MatchedServiceTableEntryMap GetUpdatesCallerHoldsCacheLock(
            Common::ActivityId const & activityId,
            Reliability::IClientRegistration const &,
            std::vector<Reliability::ServiceNotificationFilterSPtr> const & filters,
            Reliability::GenerationNumber const & clientGeneration,
            Common::VersionRangeCollectionSPtr const & clientVersions,
            __out Reliability::GenerationNumber & cacheGeneration,
            __out Common::VersionRangeCollectionSPtr & cacheVersions,
            __out int64 & lastDeletedEmptyPartitionVersion);

        virtual void GetUpdateMetadata(
            __out Reliability::GenerationNumber & cacheGeneration,
            __out int64 & lastDeletedEmptyPartitionVersion);

        virtual void GetDeletedVersions(
            Reliability::GenerationNumber const & clientGeneration,
            std::vector<Reliability::VersionedConsistencyUnitId> const & partitionsTocheck,
            __out Reliability::GenerationNumber & cacheGeneration,
            __out std::vector<int64> & deletedVersions);

        virtual void GetCurrentVersions(
            __out Reliability::GenerationNumber & cacheGeneration,
            __out Common::VersionRangeCollectionSPtr & cacheVersions);

    public:

        //
        // Test functions
        //

        void UpdateCacheEntries(std::vector<Reliability::ServiceTableEntrySPtr> const & partitions);

    private:

        UpdateHandler GetHandler();

        UpdateHandler handler_;
        Common::RwLock handlerLock_;
        Reliability::GenerationNumber generation_;
        Common::VersionRangeCollectionSPtr versions_;
        std::unique_ptr<Reliability::NotificationCacheIndex> cacheIndex_;
        Common::RwLock cacheLock_;
    };
}
