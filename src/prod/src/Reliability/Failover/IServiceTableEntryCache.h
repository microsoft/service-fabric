// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class IServiceTableEntryCache
    {
    public:
        typedef std::function<void(
            Common::ActivityId const &,
            GenerationNumber const &, 
            Common::VersionRangeCollection const & cacheVersions, 
            std::vector<CachedServiceTableEntrySPtr> const &,
            Common::VersionRangeCollection const & updateVersions)> UpdateHandler;

        virtual void SetUpdateHandler(UpdateHandler const &) = 0;

        virtual void ClearUpdateHandler() = 0;

        virtual Common::RwLock & GetCacheLock() = 0;

        virtual MatchedServiceTableEntryMap GetUpdatesCallerHoldsCacheLock(
            Common::ActivityId const &,
            IClientRegistration const &,
            std::vector<ServiceNotificationFilterSPtr> const &,
            GenerationNumber const & clientGeneration,
            Common::VersionRangeCollectionSPtr const & clientVersions,
            __out GenerationNumber & cacheGeneration,
            __out Common::VersionRangeCollectionSPtr & cacheVersions,
            __out int64 & lastDeletedEmptyPartitionVersion) = 0;

        virtual void GetUpdateMetadata(
            __out GenerationNumber & cacheGeneration,
            __out int64 & lastDeletedEmptyPartitionVersion) = 0;

        virtual void GetDeletedVersions(
            GenerationNumber const & clientGeneration,
            std::vector<VersionedConsistencyUnitId> const & partitionsTocheck,
            __out GenerationNumber & cacheGeneration,
            __out std::vector<int64> & deletedVersions) = 0;

        virtual void GetCurrentVersions(
            __out GenerationNumber & cacheGeneration,
            __out Common::VersionRangeCollectionSPtr & cacheVersions) = 0;
    };
}
