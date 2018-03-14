// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ServiceResolverCache 
        : public IServiceTableEntryCache
        , public ServiceLookupTable
        , public Common::TextTraceComponent<Common::TraceTaskCodes::Reliability>
    {
        DENY_COPY(ServiceResolverCache);
    public:
        explicit ServiceResolverCache(Common::ComponentRoot const & root);

        virtual ~ServiceResolverCache();

        __declspec(property(get=get_GenerationNumber)) GenerationNumber Generation;
        GenerationNumber get_GenerationNumber() const;

        __declspec(property(get=get_ServiceLookupTableLock)) Common::RwLock& ServiceLookupTableLock;
        Common::RwLock& get_ServiceLookupTableLock() const { return this->LockObject; }

        bool StoreUpdate_CallerHoldsLock(
            std::vector<ServiceTableEntry> const & newEntries, 
            GenerationNumber const & generationNumber, 
            Common::VersionRangeCollection const & coveredRanges, 
            int64 endVersion,
            __inout ServiceCuidMap & updatedCuids,
            __inout ServiceCuidMap & removedCuids);

        bool TryGetEntryWithCacheMode(VersionedCuid const & partition, CacheMode::Enum refreshMode, __out ServiceTableEntry & cachedEntry) const;

        bool IsUpdateNeeded_CallerHoldsLock(ServiceTableEntry const & entry, GenerationNumber const & generationNumber) const;

        Common::VersionRangeCollection GetKnownVersions();

    public:

        //
        // IServiceTableEntryCache
        //
        
        virtual void SetUpdateHandler(IServiceTableEntryCache::UpdateHandler const &);

        virtual void ClearUpdateHandler();

        virtual Common::RwLock & GetCacheLock();

        virtual MatchedServiceTableEntryMap GetUpdatesCallerHoldsCacheLock(
            Common::ActivityId const &,
            IClientRegistration const &,
            std::vector<ServiceNotificationFilterSPtr> const &,
            GenerationNumber const &,
            Common::VersionRangeCollectionSPtr const &,
            __out GenerationNumber &,
            __out Common::VersionRangeCollectionSPtr &,
            __out int64 &);

        virtual void GetUpdateMetadata(
            __out GenerationNumber &,
            __out int64 &);

        virtual void GetDeletedVersions(
            GenerationNumber const &,
            std::vector<VersionedConsistencyUnitId> const &,
            __out GenerationNumber &,
            __out std::vector<int64> &);

        virtual void GetCurrentVersions(
            __out GenerationNumber &,
            __out Common::VersionRangeCollectionSPtr &);

    private:
        GenerationNumber generationNumber_;
        Common::VersionRangeCollection knownVersions_;

        std::unique_ptr<NotificationCacheIndex> cacheIndex_;
        IServiceTableEntryCache::UpdateHandler handler_;
        Common::ActivityId nextNotificationId_;
    };
}
