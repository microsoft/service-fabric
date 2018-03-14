// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        /// <summary>
        /// This class acts as a write-through cache for the FailoverUnit
        /// information in the store.
        /// </summary>
        class FailoverUnitCache
        {
            DENY_COPY(FailoverUnitCache);

        public:

            /// <summary>
            /// Class used to enumerate all failoverUnits in the FailoverUnit
            /// cache. User of this class should make sure the FailoverUnit cache
            /// is alive when using visitor
            /// Note that this does not provide consistency in the sense
            /// that if failoverUnits are inserted/removed the enumeration
            /// may not provide a snapshot of the table any time.
            /// </summary>
            class Visitor
            {
                DENY_COPY(Visitor);

            public:
                Visitor(FailoverUnitCache const& cache, bool randomAccess, Common::TimeSpan timeout, bool executeStateMachine);

                LockedFailoverUnitPtr MoveNext();
                LockedFailoverUnitPtr MoveNext(__out bool & result, FailoverUnitId & failoverUnitId);

                void Reset();

            private:
                FailoverUnitCache const& cache_;
                std::vector<FailoverUnitId> shuffleTable_;
                LONG index_;
                Common::TimeSpan timeout_;
                bool executeStateMachine_;
            };

            typedef std::shared_ptr<FailoverUnitCache::Visitor> VisitorSPtr;

            FailoverUnitCache(
                FailoverManager& fm,
                std::vector<FailoverUnitUPtr> & failoverUnits,
                int64 savedLookupVersion,
                Common::ComponentRoot const & root);
        
            __declspec(property(get=get_Count)) size_t Count;
            size_t get_Count() const;

            __declspec(property(get=get_ServiceLookupTable)) FMServiceLookupTable& ServiceLookupTable;
            FMServiceLookupTable& get_ServiceLookupTable() { return serviceLookupTable_; }

            __declspec(property(get = get_SavedLookupVersion)) int64 SavedLookupVersion;
            int64 get_SavedLookupVersion() const { return savedLookupVersion_; }

            __declspec(property(get=get_AckedHealthSequence)) FABRIC_SEQUENCE_NUMBER AckedHealthSequence;
            FABRIC_SEQUENCE_NUMBER get_AckedHealthSequence() const { return ackedSequence_; }

            __declspec(property(get = get_IsHealthInitialized)) bool IsHealthInitialized;
            bool get_IsHealthInitialized() const { return healthInitialized_; }

            __declspec(property(get=get_InvalidatedHealthSequence)) FABRIC_SEQUENCE_NUMBER InvalidatedHealthSequence;
            FABRIC_SEQUENCE_NUMBER get_InvalidatedHealthSequence() const { return invalidateSequence_; }

            Common::ErrorCode UpdateFailoverUnit(LockedFailoverUnitPtr & failoverUnit);

            void UpdateFailoverUnitAsync(
                LockedFailoverUnitPtr && failoverUnit,
                std::vector<StateMachineActionUPtr> && actions,
                bool isBackground);

            void InsertFailoverUnitInCache(FailoverUnitUPtr&& failoverUnit);

            VisitorSPtr CreateVisitor(bool randomAccess, Common::TimeSpan timeout, bool executeStateMachine = false) const;
            VisitorSPtr CreateVisitor(bool randomAccess = false) const;

            bool TryProcessTaskAsync(FailoverUnitId failoverUnitId, DynamicStateMachineTaskUPtr & task, Federation::NodeInstance const & from, bool const isFromPLB = false) const;

            bool TryGetLockedFailoverUnit(
                FailoverUnitId const& failoverUnitId,
                __out LockedFailoverUnitPtr & failoverUnit) const;
            bool TryGetLockedFailoverUnit(
                FailoverUnitId const& failoverUnitId,
                __out LockedFailoverUnitPtr & failoverUnit,
                Common::TimeSpan timeout,
                bool executeStateMachine = false) const;

            bool IsFailoverUnitValid(FailoverUnitId const& failoverUnitId) const;

            bool FailoverUnitExists(FailoverUnitId const& failoverUnitId) const;

            bool IsSafeToRemove(FailoverUnit const& failoverUnit) const;
            void SaveLookupVersion(int64 lookupVersion);

            Common::ErrorCode GetConsistencyUnitDescriptions(std::wstring const& serviceName, __out std::vector<ConsistencyUnitDescription> & consistencyUnitDescriptions) const;

            void AddThreadContexts();

            void CompleteHealthInitialization();
            void InternalCompleteHealthInitialization();

            FABRIC_SEQUENCE_NUMBER GetHealthSequence();
            void ReportHealthAfterFailoverUnitUpdate(FailoverUnit const & failoverUnit, bool updated);

            Common::ErrorCode PostUpdateFailoverUnit(
                LockedFailoverUnitPtr & failoverUnit,
                PersistenceState::Enum persistenceState,
                bool shouldUpdateLookupVersion,
                bool shouldReportHealth,
                Common::ErrorCode error,
                __out int64 & plbDuration);

        private:

            void PreUpdateFailoverUnit(
                LockedFailoverUnitPtr & failoverUnit,
                __out PersistenceState::Enum & persistenceState,
                __out bool & shouldUpdateLookupVersion,
                __out bool & shouldReportHealth);

            void UpdatePlacementAndLoadBalancer(LockedFailoverUnitPtr & failoverUnit, PersistenceState::Enum pstate, __out int64 & plbDuration) const;

            FailoverManager& fm_;
            FailoverManagerStore& fmStore_;
            InstrumentedPLB & plb_;

            NodeCache const& nodeCache_;
            ServiceCache & serviceCache_;
            LoadCache & loadCache_;

            std::map<FailoverUnitId, FailoverUnitCacheEntrySPtr> failoverUnits_;

            FMServiceLookupTable serviceLookupTable_;
            int64 savedLookupVersion_;

            FABRIC_SEQUENCE_NUMBER healthSequence_;
            FABRIC_SEQUENCE_NUMBER initialSequence_;
            FABRIC_SEQUENCE_NUMBER ackedSequence_;
            FABRIC_SEQUENCE_NUMBER invalidateSequence_;
            bool healthInitialized_;

            MUTABLE_RWLOCK(FM.FailoverUnitCache, lock_);
        };
    }
}
