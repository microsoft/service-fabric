// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class NotificationCacheIndex
        : public Common::TextTraceComponent<Common::TraceTaskCodes::NamingGateway>        
    {
        DENY_COPY(NotificationCacheIndex)

    public:

        NotificationCacheIndex(std::wstring const & traceId);

        virtual ~NotificationCacheIndex();

        __declspec(property(get=get_TraceId)) std::wstring const & TraceId;
        std::wstring const & get_TraceId() const { return traceId_; }

        void Clear();

        std::vector<CachedServiceTableEntrySPtr> AddOrUpdateEntries(
            Common::ActivityId const &,
            std::vector<ServiceTableEntrySPtr> const &);

        CachedServiceTableEntrySPtr AddOrUpdateEntry(
            Common::ActivityId const &,
            ServiceTableEntrySPtr const &);

        MatchedServiceTableEntryMap GetMatches(
            Common::ActivityId const &,
            IClientRegistration const &,
            std::vector<ServiceNotificationFilterSPtr> const & filters,
            Common::VersionRangeCollectionSPtr const & clientVersions);

        int64 GetLastDeletedEmptyPartitionVersion();

        size_t GetEmptyPartitionsCount();

        size_t GetAllPartitionsCount();
    
        std::vector<int64> GetDeletedVersions(std::vector<VersionedConsistencyUnitId> const & partitionsToCheck);

    private:
        class NameIndexEntry;
        typedef std::shared_ptr<NameIndexEntry> NameIndexEntrySPtr;

        typedef std::pair<Common::NamingUri, NameIndexEntrySPtr> NameEntryPair;
        typedef std::map<Common::NamingUri, NameIndexEntrySPtr> NameEntriesMap;
        typedef std::map<int64, CachedServiceTableEntrySPtr> VersionedPartitionsMap;
        typedef std::unordered_set<ConsistencyUnitId, ConsistencyUnitId::Hasher> PartitionHash;

        void UpdateVersionMapAndTrimEmptyPartitions(
            Common::ActivityId const &,
            int64 previousVersion,
            CachedServiceTableEntrySPtr const & updatedEntry);

        void GetExactMatch(
            Common::ActivityId const &,
            ServiceNotificationFilterSPtr const &,
            Common::VersionRangeCollectionSPtr const & clientVersions,
            __inout MatchedServiceTableEntryMap &);

        void GetPrefixMatches(
            Common::ActivityId const &,
            IClientRegistration const &,
            ServiceNotificationFilterSPtr const &,
            Common::VersionRangeCollectionSPtr const & clientVersions,
            __inout MatchedServiceTableEntryMap &);

        void AddMatchedEntryResult(
            CachedServiceTableEntrySPtr const &,
            bool matchedPrimaryOnly,
            __in MatchedServiceTableEntryMap &);

        std::wstring traceId_;
        NameEntriesMap indexEntriesByName_;

        // Empty partitions can happen normally if all replicas go down but
        // are also used by the FM to indicate deleted services. Index all
        // observed empty partitions reported by the FM for
        // the purposes of delayed, threshold-based cleanup. We do not remove 
        // empty partitions from the index immediately to reduce the chances
        // of running a more expensive synchronization protocol for re-connecting
        // clients.
        //
        VersionedPartitionsMap emptyPartitionsByVersion_;
        PartitionHash allPartitionsByCuid_;
        int64 lastDeletedEmptyPartitionVersion_;
    };
}
