// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        // Base class for health entities
        class HealthEntity
        {
            DENY_COPY(HealthEntity);

        public:
            virtual ~HealthEntity();

            __declspec (property(get=get_EntityIdString)) std::wstring const & EntityIdString;
            std::wstring const & get_EntityIdString() const { return entityIdString_; }

            __declspec (property(get=get_HealthManagerReplica)) HealthManagerReplica & HealthManagerReplicaObj;
            HealthManagerReplica & get_HealthManagerReplica() { return healthManagerReplica_; }

            __declspec (property(get=get_HealthManagerCounters)) HealthManagerCountersSPtr const & HealthManagerCounters;
            HealthManagerCountersSPtr const & get_HealthManagerCounters() const { return healthManagerReplica_.HealthManagerCounters; }

            __declspec(property(get=get_PartitionedReplicaId)) Store::PartitionedReplicaId const & PartitionedReplicaId;
            Store::PartitionedReplicaId const & get_PartitionedReplicaId() const { return healthManagerReplica_.PartitionedReplicaId; }

            __declspec (property(get=get_JobQueueManager)) EntityJobQueueManager & JobQueueManager;
            EntityJobQueueManager & get_JobQueueManager() { return healthManagerReplica_.JobQueueManager; }

            __declspec (property(get=get_EntityManager)) EntityCacheManagerUPtr const & EntityManager;
            EntityCacheManagerUPtr const & get_EntityManager() const { return healthManagerReplica_.EntityManager; }

            __declspec (property(get=get_State)) EntityState State;
            EntityState const & get_State() const { Common::AcquireReadLock lock(lock_); return entityState_; }

            __declspec (property(get=get_EntityKind)) HealthEntityKind::Enum EntityKind;
            virtual HealthEntityKind::Enum get_EntityKind() const = 0;

            // Get the attributes (should be protected by lock)
            __declspec (property(get=get_InternalAttributes)) AttributesStoreDataSPtr const & InternalAttributes;
            AttributesStoreDataSPtr const & get_InternalAttributes() const { return attributes_; }

            bool Match(AttributesMap const & matchAttributes) const;

            bool IsCleanedUp(Common::ActivityId const & activityId, __out bool & hasPendingWork);

            bool ShouldBeCleanedUp() const;

            bool ShouldBeDeletedDueToParentState(Common::ActivityId const & activityId);

            bool HasExpiredEventChanges();

            bool ShouldAutoCleanupEvents() const;

            bool IsDeletedOrCleanedUp() const;

            Common::ErrorCode TryReserveCleanupJob(Common::ActivityId const & activityId);
            void OnCleanupJobCompleted();

            // Get the attributes (under the lock)
            AttributesStoreDataSPtr GetAttributesCopy() const { Common::AcquireReadLock lock(lock_); return attributes_; }

            void PrepareLoadFromStore(AttributesStoreDataSPtr && attributes);

            bool ShouldInvalidate(
                std::wstring const & sourceId,
                FABRIC_SEQUENCE_NUMBER invalidateLsn,
                Common::ActivityId const & activityId) const;

            Common::ErrorCode GetEventsHealthState(
                Common::ActivityId const & activityId,
                bool considerWarningAsError,
                __out FABRIC_HEALTH_STATE & eventsHealthState);

            Common::ErrorCode GetEventsHealthState(
                Common::ActivityId const & activityId,
                bool considerWarningAsError,
                __out FABRIC_HEALTH_STATE & eventsHealthState,
                __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations);

            // For each request:
            // Check whether the new request can be accepted.
            // If so, create transaction and start persist data.
            // Notify the job item if the work is done async,
            // so it will know to call the End method when done
            // Optimization: first checks parent state: if this entity needs to be deleted due to parent issue,
            // do not process current report, instead report is rejected with Entity deleted error.
            // The cleanup will happen when the cleanup task runs.
            void StartProcessReport(
                IHealthJobItemSPtr const & jobItem,
                __in std::vector<ReportRequestContext> & contexts,
                bool isDeleteRequest);

            void EndProcessReport(
                __in IHealthJobItem & jobItem,
                Common::AsyncOperationSPtr const & operation,
                bool isDeleteRequest);

            // Delete entity due to parent state
            void StartDeleteEntity(
                IHealthJobItemSPtr const & jobItem);

            void EndDeleteEntity(
                __in IHealthJobItem & jobItem,
                Common::AsyncOperationSPtr const & operation);

            // Check whether the entity should be removed from store.
            // If so, create transaction and start persist data.
            // Notify the job item if the work is done async,
            // so it will know to call the End method when done
            void StartCleanupEntity(
                IHealthJobItemSPtr const & jobItem,
                std::wstring const & invalidateSource,
                FABRIC_SEQUENCE_NUMBER invalidateLsn);

            void EndCleanupEntity(
                __in IHealthJobItem & jobItem,
                Common::AsyncOperationSPtr const & operation);

            // Cleanup expired transient events from store
            void StartCleanupExpiredTransientEvents(
                IHealthJobItemSPtr const & jobItem);

            void EndCleanupExpiredTransientEvents(
                __in IHealthJobItem & jobItem,
                Common::AsyncOperationSPtr const & operation);

            // Auto-cleanup of events when the entity has too many reports.
            // Only events that do not impact health can be removed.
            // The events that impact health must be deleted by users or admins and not handled automatically by HM.
            void StartAutoCleanupEvents(
                IHealthJobItemSPtr const & jobItem);

            void EndAutoCleanupEvents(
                __in IHealthJobItem & jobItem,
                Common::AsyncOperationSPtr const & operation);

            // Process queries
            // First check parent state: If the entity needs to be deleted/unhealthy/not set,
            // complete query with error.
            // Otherwise, serve immediately based on in-memory events and attributes
            Common::ErrorCode ProcessQuery(__in QueryRequestContext & context);

            // Gets the health events read by the cache and populates internal structures
            // It's done before any events are processed, so there's nothing in the cache yet.
            Common::ErrorCode LoadStoreData(
                Common::ActivityId const & activityId,
                std::vector<HealthEventStoreDataUPtr> && storeEvents);

            // Load store data returned events but no attributes.
            // Create a dummy entity in order to cleanup the leaked events from store.
            // If a new report is received for this entity and it needs to be persisted to disk,
            // delete the old events in the same transaction as inserting the new attributes.
            Common::ErrorCode AddStoreLeakedEvents(
                Common::ActivityId const & activityId,
                std::vector<HealthEventStoreDataUPtr> && storeEvents);

            bool ShouldCheckConsistencyBetweenMemoryAndStore(Common::ActivityId const & activityId);

            void CheckInMemoryAgainstStoreData(
                IHealthJobItemSPtr const & jobItem);

            // Change entity state to not accept any more requests
            virtual Common::ErrorCode Close();

            //
            // Static helper templated classes
            //
            template <class TEntityId>
            static Common::ErrorCode LoadStoreEvents(
                __in HealthManagerReplica & hm,
                Common::ActivityId const & activityId,
                std::wstring const & entityIdString,
                __inout std::vector<HealthEventStoreDataUPtr> & healthEvents);

            template <class TAttributesType>
            static TAttributesType & GetCastedAttributes(AttributesStoreDataSPtr const & attributes);

            // Traces error from adding to ListPager if appropriate
            // errorBenign value is only valid if hasError is returned as true
            // entityString is usually this->EntityIdString
            static void CheckListPagerErrorAndTrace(
                Common::ErrorCode const & error,
                std::wstring const & entityString,
                Common::ActivityId const & activityId,
                std::wstring const & listPagerItemId,
                __out bool & hasError,
                __out bool & errorBenign);

            // *******************************************
            // Test hooks
            // *******************************************
            bool Test_GetData(
                __out HealthEntityState::Enum & entityState,
                __inout AttributesStoreDataSPtr & attributes,
                __inout std::vector<ServiceModel::HealthEvent> & events) const;
            bool Test_HasHierarchySystemReport() const;
            bool Test_CorruptEntity(
                Common::ActivityId const & activityId,
                bool changeEntityState,
                bool changeHasSystemReport,
                bool changeSystemErrorCount,
                std::vector<ServiceModel::HealthInformation> && addReports,
                std::vector<std::pair<std::wstring, std::wstring>> const & deleteEventKeys,
                std::vector<std::tuple<std::wstring, std::wstring, FABRIC_HEALTH_STATE>> const & scrambleEventKeys,
                __inout HealthEntityState::Enum & entityState,
                __inout size_t & eventCount);

        protected:
            HealthEntity(
                AttributesStoreDataSPtr && attributes,
                __in HealthManagerReplica & healthManagerReplica,
                bool considerSystemErrorUnhealthy,
                bool expectSystemReports,
                HealthEntityState::Enum entityState);

            // Returns true if a system report for all parents is received
            __declspec (property(get=get_HasHierarchySystemReport)) bool HasHierarchySystemReport;
            virtual bool get_HasHierarchySystemReport() const { return true; }

            void SetAttributesCallerHoldsLock(AttributesStoreDataSPtr && attributes) { attributes_ = std::move(attributes); }

            virtual Common::ErrorCode LoadEventsFromStore(
                Common::ActivityId const & activityId,
                __inout std::vector<HealthEventStoreDataUPtr> & events) = 0;

            HealthEventStoreDataUPtr GetStoreData(ReportRequestContext const & context) const { return GetStoreData(context.Report, context.Priority, context.ReplicaActivityId); }

            virtual HealthEventStoreDataUPtr GetStoreData(
                ServiceModel::HealthInformation const & healthInfo,
                ServiceModel::Priority::Enum priority,
                Store::ReplicaActivityId const & replicaActivityId) const = 0;

            virtual Common::ErrorCode HasAttributeMatchCallerHoldsLock(
                std::wstring const & attributeName,
                std::wstring const & attributeValue,
                __out bool & hasMatch) const;

            virtual void UpdateParents(Common::ActivityId const &) {}
            virtual bool DeleteDueToParentsCallerHoldsLock() const { return false; }

            // Called when the entity was successfully loaded data from memory (on primary failover)
            // or when the first report that triggered creating the entity is successfully persisted.
            // This is used by derived classes to execute specific operations.
            virtual void OnEntityReadyToAcceptRequests(Common::ActivityId const &) {}

            virtual Common::ErrorCode UpdateContextHealthPoliciesCallerHoldsLock(
                __in QueryRequestContext & context) = 0;

            virtual Common::ErrorCode SetDetailQueryResult(
                __in QueryRequestContext & context,
                FABRIC_HEALTH_STATE entityEventsState,
                std::vector<ServiceModel::HealthEvent> && queryEvents,
                std::vector<ServiceModel::HealthEvaluation> && unhealthyEvaluations) = 0;

            virtual Common::ErrorCode SetEntityHealthState(
                __in QueryRequestContext & context,
                FABRIC_HEALTH_STATE entityEventsState);

            virtual Common::ErrorCode SetChildrenAggregatedHealthQueryResult(
                __in QueryRequestContext & context);

            // If the entity is not persisted to store and has no children, is considered cleaned up,
            // so the cache can remove it from its list
            virtual bool CleanupChildren() { return true; }

            // Entities that have no system reports are deleted when they have no children
            virtual bool ShouldBeDeletedDueToChildrenState() { return false; }

            // When all information is present, create or update the parent which is non-persisted
            virtual void CreateOrUpdateNonPersistentParents(Common::ActivityId const &) {}

            // NOTE: Called ONLY for entities that are not reported on, nor by user, nor by System components
            // Otherwise, all attributes must be updated through a job item
            void ReplaceInMemoryAttributes(AttributesStoreDataSPtr && attributes);

            Common::ErrorCode GetEventsHealthState(
                Common::ActivityId const & activityId,
                bool considerWarningAsError,
                bool setUnhealthyEvaluations,
                __out FABRIC_HEALTH_STATE & eventsHealthState,
                __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations);

            bool UpdateHealthState(
                Common::ActivityId activityId,
                HealthEntityKind::Enum childrenType,
                HealthCount const & healthCount,
                BYTE maxUnhealthy,
                __inout FABRIC_HEALTH_STATE & aggregatedHealthState);

        private:
            bool HasSystemReportCallerHoldsLock();

            void CreatePendingAttributesCallerHoldsLock(
                ReportRequestContext const & context,
                bool replaceAttributes,
                bool isDeleteRequest,
                __out bool & hasAttributeChanges);

            bool IsDeletedOrCleanedUpCallerHoldsLock() const;

            bool TryCreatePendingReportInformationCallerHoldsLock(
                __in ReportRequestContext & context,
                bool isDeleteRequest);

            void UpdateInMemoryDataCallerHoldsLock(Store::ReplicaActivityId const & replicaActivityId);

            bool NoAttributeChangeInGracePeriodCallerHoldsLock() const;

            //
            // Persist report to store
            //
            Common::ErrorCode PreparePersistCurrentDataTx(
                Store::ReplicaActivityId const & replicaActivityId,
                __out Store::IStoreBase::TransactionSPtr & tx);

            void StartPersistCurrentData(
                IHealthJobItemSPtr const & jobItem,
                Common::TimeSpan const & timeout);

            void OnPersistCurrentDataCompleted(
                __in IHealthJobItem & jobItem,
                Common::AsyncOperationSPtr const & operation);

            void OnProcessReportFailed(
                __in IHealthJobItem & jobItem,
                Common::ErrorCode const & error);

            //
            // Persist delete entity request to store:
            // mark this instance of the entity for tombstoning
            //
            Common::ErrorCode PreparePersistDeleteEntityTx(
                Store::ReplicaActivityId const & replicaActivityId,
                __out Store::IStoreBase::TransactionSPtr & tx);

            void StartPersistDeleteEntity(
                IHealthJobItemSPtr const & jobItem,
                Common::TimeSpan const & timeout);

            void OnPersistDeleteEntityCompleted(
                __in IHealthJobItem & jobItem,
                Common::AsyncOperationSPtr const & operation);

            //
            // Persist cleanup entity:
            // Remove the entity from the store
            //
            Common::ErrorCode PreparePersistCleanupEntityTx(
                Store::ReplicaActivityId const & replicaActivityId,
                __out Store::IStoreBase::TransactionSPtr & tx);

            void StartPersistCleanupEntity(
                IHealthJobItemSPtr const & jobItem);

            void OnPersistCleanupEntityCompleted(
                __in IHealthJobItem & jobItem,
                Common::AsyncOperationSPtr const & operation);

            //
            // Persist cleanup expired transient events: Remove the events from the store
            // Persist events that have in memory only changes that should be known after failover
            //
            Common::ErrorCode PreparePersistExpiredEventChangesTx(
                Store::ReplicaActivityId const & replicaActivityId,
                __out Store::IStoreBase::TransactionSPtr & tx);

            void StartPersistExpiredEventChanges(
                IHealthJobItemSPtr const & jobItem);

            void OnPersistExpiredEventChangesCompleted(
                __in IHealthJobItem & jobItem,
                Common::AsyncOperationSPtr const & operation);

            //
            // Auto cleanup of events when there are too many events.
            //
            Common::ErrorCode PreparePersistAutoCleanupEventsTx(
                Store::ReplicaActivityId const & replicaActivityId,
                __out Store::IStoreBase::TransactionSPtr & tx);

            void StartPersistAutoCleanupEvents(
                IHealthJobItemSPtr const & jobItem);

            void OnPersistAutoCleanupEventsCompleted(
                __in IHealthJobItem & jobItem,
                Common::AsyncOperationSPtr const & operation);

            void ResetPendingEventsCallerHoldsLock();

            FABRIC_HEALTH_STATE GetEventsHealthStateCallerHoldsLock(
                Common::ActivityId const & activityId,
                bool considerWarningAsError,
                bool setUnhealthyEvaluations,
                __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations,
                __inout std::vector<ServiceModel::HealthEvent> & nonPersistedEvents);

            std::wstring GetMissingAuthorityReportHealthDescription() const;

            std::wstring GetTooManyReportsHealthDescription(
                int totalEvents,
                int maxSuggested) const;

            void AddGeneratedEventsCallerHoldsLock(
                Common::ActivityId const & activityId,
                std::wstring const & reportProperty,
                FABRIC_HEALTH_STATE reportState,
                std::wstring && reportDescription,
                bool setUnhealthyEvaluations,
                bool considerWarningAsError,
                __inout FABRIC_HEALTH_STATE & aggregatedState,
                __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations,
                __inout std::vector<ServiceModel::HealthEvent> & nonPersistedEvents);
            
            ServiceModel::HealthEvent GenerateNonPersistedEvent(
                Common::ActivityId const & activityId,
                std::wstring const & property,
                std::wstring && description,
                FABRIC_HEALTH_STATE state) const;
            
            // The memory state must be in sync with store data.
            // Otherwise, we need to bring the replica down.
            bool VerifyInMemoryAgainstStoreEventsCallerHoldsLock(
                Common::ActivityId const & activityId,
                Common::ErrorCode const & eventsLoadError,
                std::vector<HealthEventStoreDataUPtr> const & storeEvents);

            void UpdateSystemDataCallerHoldsLock(
                HealthEventStoreData const & event,
                __out bool & hasSystemReport,
                __out int & systemErrorCount) const;

            // Update the health report in the provided context so that the EntityInformation field is congruent with the
            // latest instance of the entity. This will not override existing values, but instead, update to the latest
            // instance only if an instance is not already provided.
            void UpdateEntityInstanceInHealthReport(
                __in ReportRequestContext & context);

            Common::ErrorCode AddDeleteEventsToTx(
                Common::ActivityId const & activityId,
                __in Store::IStoreBase::TransactionSPtr & tx);

            void UpdateDeletedEventsCallerHoldsLock();

            void AppendEventDataForTrace(
                __in Common::StringWriter & writer,
                HealthEventStoreDataUPtr const & entry,
                __inout int & traceCount);

        private:
            // Id of the entity.
            std::wstring entityIdString_;

            EntityState entityState_;

            // Reference to the health manager replica
            // Note: the owner of the replicated store must be kept alive to ensure correct access to the store when executing transactions.
            HealthManagerReplica & healthManagerReplica_;

            // List of attributes currently active in the entity.
            // Set after persistence to the store.
            // Note that before reading all from memory at startup,
            // the attributes are not set.
            AttributesStoreDataSPtr attributes_;
            // The attributes that have been accepted and currently are written to store.
            // The attributes are set and destroyed under the lock,
            // and accessed freely when the state is changed to processing,
            // as they can't change during this time.
            AttributesStoreDataSPtr pendingAttributes_;

            // The list of events that have been accepted and persisted.
            std::list<HealthEventStoreDataUPtr> events_;
            // The events that have been accepted and currently are written to store.
            // The events are set and destroyed under the lock,
            // and accessed freely when the state is changed to processing,
            // as they can't change during this time.
            std::vector<HealthEventStoreDataUPtr> pendingEvents_;

            // Lock that protects the queue of pending operations and the state of the entity
            MUTABLE_RWLOCK(HM.HealthEntity, lock_);

            bool considerSystemErrorUnhealthy_;

            Common::atomic_bool hasPendingCleanupTask_;

            // The time at which the last consistency check was performed
            Common::DateTime checkConsistencyBetweenMemoryAndStoreTime_;

            // When load from store finds events that are not associated with an entity (attributes),
            // the entity is created in PendingFirstReport and cleanup task must remove the leaked events.
            // Do no cleanup up while the replica is running, as it may have in memory state corruption
            // and it's better to detect that first with the in-memory vs. store state consistency checks.
            bool hasLeakedEvents_;
        };

        //
        // Implement helper templated methods
        //
        template <class TEntityId>
        Common::ErrorCode HealthEntity::LoadStoreEvents(
            __in HealthManagerReplica & hm,
            Common::ActivityId const & activityId,
            std::wstring const & entityIdString,
            __inout std::vector<HealthEventStoreDataUPtr> & healthEvents)
        {
            auto tx = hm.StoreWrapper.CreateTransaction(activityId);

            vector<EntityHealthEventStoreData<TEntityId>> events;
            auto error = hm.StoreWrapper.ReadPrefix<EntityHealthEventStoreData<TEntityId>>(
                tx,
                EntityHealthEventStoreData<TEntityId>::GeneratePrefix(entityIdString),
                /*out*/events);

            tx.CommitReadOnly();

            if (error.IsSuccess())
            {
                for (auto it = events.begin(); it != events.end(); ++it)
                {
                    healthEvents.push_back(Common::make_unique<EntityHealthEventStoreData<TEntityId>>(move(*it)));
                }
            }

            return error;
        }

        // This does not do a safety check during cast in non-debug mode. Does not check for nullptr in debug mode.
        template <class TAttributesType>
        TAttributesType & HealthEntity::GetCastedAttributes(AttributesStoreDataSPtr const & attributes)
        {
#ifdef DBG
            return *dynamic_cast<TAttributesType*>(attributes.get());
#else
            return *static_cast<TAttributesType*>(attributes.get());
#endif
        }

#define HEALTH_ENTITY_TEMPLATED_METHODS_DECLARATIONS( ATTRIB_TYPE ) \
    public: \
        static ATTRIB_TYPE & GetCastedAttributes(AttributesStoreDataSPtr const & attributes); \
    protected: \
        virtual Common::ErrorCode LoadEventsFromStore(Common::ActivityId const & activityId, __inout std::vector<HealthEventStoreDataUPtr> & events) override; \
        virtual HealthEventStoreDataUPtr GetStoreData(ServiceModel::HealthInformation const & healthInfo, ServiceModel::Priority::Enum priority, Store::ReplicaActivityId const & replicaActivityId) const override; \


#define HEALTH_ENTITY_TEMPLATED_METHODS_DEFINITIONS( ATTRIB_TYPE, ENTITY, ENTITY_ID ) \
    Common::ErrorCode ENTITY::LoadEventsFromStore(Common::ActivityId const & activityId, __inout std::vector<HealthEventStoreDataUPtr> & healthEvents) { \
        return HealthEntity::LoadStoreEvents<ENTITY_ID>(this->HealthManagerReplicaObj, activityId, this->EntityIdString, healthEvents); \
    } \
    HealthEventStoreDataUPtr ENTITY::GetStoreData(ServiceModel::HealthInformation const & healthInfo, ServiceModel::Priority::Enum priority, Store::ReplicaActivityId const & replicaActivityId) const { \
        return make_unique<EntityHealthEventStoreData<ENTITY_ID>>(this->EntityId, healthInfo, priority, replicaActivityId); \
    } \
    ATTRIB_TYPE & ENTITY::GetCastedAttributes(AttributesStoreDataSPtr const & attributes) {  \
        return HealthEntity::GetCastedAttributes<ATTRIB_TYPE>(attributes); \
    } \


    }
}
