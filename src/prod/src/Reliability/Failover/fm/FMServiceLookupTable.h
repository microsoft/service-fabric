// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class FMServiceLookupTable : public ServiceLookupTable
        {
        public:
            FMServiceLookupTable(
                FailoverManager& fm,
                std::vector<FailoverUnitUPtr>& failoverUnits,
                int64 savedLookupVersion,
                Common::ComponentRoot const & root);

            __declspec(property(get=get_EndVersion, put=set_EndVersion)) int64 EndVersion;
            int64 get_EndVersion() const { return endVersion_; }
            void set_EndVersion(int64 value);

            // Updates the LookupVersion of the FailoverUnit and increments the EndVersion.
            void UpdateLookupVersion(FailoverUnit & failoverUnit);

            // Updates the version range when the persistence of a FailoverUnit fails
            void OnUpdateFailed(FailoverUnit const& failoverUnit);

            // Inserts/Updates the lookup table with a new Failover Unit.
            void Update(FailoverUnit const& failoverUnit);

            // Adjusts version ranges and removes a deleted FailoverUnit.
            void RemoveEntry(FailoverUnit const& failoverUnit);

            void GetUpdatesCallerHoldingLock(
                size_t pageSizeLimit,
                Common::VersionRangeCollection const& knownVersionRangeCollection,
                __out std::vector<ServiceTableEntry> & entriesToUpdate,
                __out Common::VersionRangeCollection & versionRangesToUpdate) const;

            bool GetUpdates(
                size_t pageSizeLimit,
                GenerationNumber const& generation,
                std::vector<ConsistencyUnitId> const& consistencyUnitIds,
                Common::VersionRangeCollection && knownVersionRangeCollection,
                __out std::vector<ServiceTableEntry> & entriesToUpdate,
                __out Common::VersionRangeCollection & versionRangesToUpdate,
                __out int64 & endVersion) const;

            bool TryGetServiceTableUpdateMessageBody(__out ServiceTableUpdateMessageBody & body);

            void Dispose();

        private:
            FailoverManager& fm_;

            // The previous broadcast version ranges. The index [0, n) stores the ranges at the time of broadcast.
            std::vector<Common::VersionRangeCollection> broadcastVersionRangeCollections_;

            // The next available version number. It is one greater than the known version in the table.
            int64 endVersion_;

            // The known collection of version ranges. Typically, this is only one range from 0 to endVersion_.
            // But when there are persistence failures, this will have multiple version ranges in which gaps
            // indicate the versions that caused persistence failures. These gaps are eventually filled after
            // a successful persistence of the corresponding Failover Unit.
            Common::VersionRangeCollection versionRangeCollection_;

            Common::DateTime lastBroadcast_;

            // Timer for broadcast lookup table updates.
            Common::TimerSPtr broadcastTimer_;

            void UpdateEntryCallerHoldingLock(FailoverUnit const& failoverUnit);

            // Update the collection of version ranges when we are sure that the Failover Unit is persisted.
            // This removes all the holes in the version range collection for the given FailoverUnit.
            void UpdateVersionRangesCallerHoldingLock(FailoverUnit const& failoverUnit);

            void StartBroadcastTimer();
            void BroadcastTimerCallback();
        };
    }
}
