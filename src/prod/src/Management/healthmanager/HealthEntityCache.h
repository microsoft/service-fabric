// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        LOCKNAME_TEMPLATE_TRAITS(HealthEntityCacheLockNameTraits)
        LOCKNAME_TEMPLATE_TRAITS_LIST(HealthEntityCacheLockNameTraits, NodeHealthId, HM.NodeCache)
        LOCKNAME_TEMPLATE_TRAITS_LIST(HealthEntityCacheLockNameTraits, ClusterHealthId, HM.ClusterCache)
        LOCKNAME_TEMPLATE_TRAITS_LIST(HealthEntityCacheLockNameTraits, ReplicaHealthId, HM.ReplicaCache)
        LOCKNAME_TEMPLATE_TRAITS_LIST(HealthEntityCacheLockNameTraits, PartitionHealthId, HM.PartitionCache)
        LOCKNAME_TEMPLATE_TRAITS_LIST(HealthEntityCacheLockNameTraits, ServiceHealthId, HM.ServiceCache)
        LOCKNAME_TEMPLATE_TRAITS_LIST(HealthEntityCacheLockNameTraits, ApplicationHealthId, HM.ApplicationCache)
        LOCKNAME_TEMPLATE_TRAITS_LIST(HealthEntityCacheLockNameTraits, DeployedApplicationHealthId, HM.DeployedApplicationCache)
        LOCKNAME_TEMPLATE_TRAITS_LIST(HealthEntityCacheLockNameTraits, DeployedServicePackageHealthId, HM.DeployedServicePackageCache)

        // Cache for entities of certain type.
        template <class TEntityId>
        class HealthEntityCache
            : public Common::RootedObject
        {
            DENY_COPY(HealthEntityCache);

        public:
            HealthEntityCache(
                Common::ComponentRoot const & root,
                __in HealthManagerReplica & healthManagerReplica);

            virtual ~HealthEntityCache();

            __declspec (property(get=get_HealthManagerReplica)) HealthManagerReplica & HealthManagerReplicaObj;
            HealthManagerReplica & get_HealthManagerReplica() const { return healthManagerReplica_; }

            __declspec(property(get=get_PartitionedReplicaId)) Store::PartitionedReplicaId const & PartitionedReplicaId;
            Store::PartitionedReplicaId const & get_PartitionedReplicaId() const { return healthManagerReplica_.PartitionedReplicaId; }

            __declspec (property(get=get_JobQueueManager)) EntityJobQueueManager & JobQueueManager;
            EntityJobQueueManager & get_JobQueueManager() { return healthManagerReplica_.JobQueueManager; }

            __declspec(property(get=get_EntitiesType)) std::wstring const & EntitiesType;
            std::wstring const & get_EntitiesType() const { return EntitySequenceStreamStoreData<TEntityId>::GetEntityTypeString(); }

            Common::ErrorCode Open();
            Common::ErrorCode Close();

            // Get the entity with desired entityId
            HealthEntitySPtr GetEntity(TEntityId const & entityId) const;

            // Give report to the correct entity to process it
            void AddReport(
                ReportRequestContext && context);

            // Give query to correct entity for processing
            Common::ErrorCode ProcessQuery(
                TEntityId const & entityid,
                __in QueryRequestContext & context) const;

            // Persist sequence stream async
            void AddSequenceStream(
                SequenceStreamRequestContext && context);
            void StartProcessSequenceStream(
                IHealthJobItemSPtr const & jobItem,
                SequenceStreamRequestContext const & context);
            void EndProcessSequenceStream(
                __in IHealthJobItem & jobItem,
                Common::AsyncOperationSPtr const & operation);

            // Get sequence stream progress
            Common::ErrorCode GetSequenceStreamProgress(
                std::wstring const & sourceId,
                FABRIC_INSTANCE_ID instance,
                bool getHighestLsn,
                __inout SequenceStreamResult & sequenceStreamResult);

            // Validate instance before accepting reports
            Common::ErrorCode AcceptSequenceStream(
                Common::ActivityId const & activityId,
                std::wstring const & sourceId,
                FABRIC_INSTANCE_ID instance,
                FABRIC_SEQUENCE_NUMBER upToLsn);

            Common::ErrorCode GetEntities(
                Common::ActivityId const & activityId,
                __inout std::vector<HealthEntitySPtr> & entities) const;

            Common::ErrorCode GetEntities(
                Common::ActivityId const & activityId,
                TEntityId const & continuationToken,
                __inout std::vector<HealthEntitySPtr> & entities) const;

            Common::ErrorCode GetEntitiesByName(
                Common::ActivityId const & activityId,
                TEntityId const * name,
                __inout std::vector<HealthEntitySPtr> & entities) const;

            // Called by non-persisted parent entities to make sure that the children caches are loaded
            // Do not take lock here, since this may cause deadlocks
            bool IsReady() const { return allDataLoadedInMemory_; }

            // Execute entity work under cache lock.
            // If the entity doesn't exist, it's created in state PendingFirstReport (without any job items)
            // and work is executed on it.
            // If the cache is disabled, no action is needed.
            // Because work is executed under cache lock, the entity can't be cleaned up before the work is completed.
            void ExecuteEntityWorkUnderCacheLock(
                Common::ActivityId const & activityId,
                TEntityId const & entityId,
                EntityWorkCallback const & callback);

            FABRIC_SEQUENCE_NUMBER Test_GetInvalidatedLsn(std::wstring const & sourceId) const;

        protected:
            void AddEntityCallerHoldsLock(TEntityId const & entityId, HealthEntitySPtr const & entity);

            // Create default entities that are not reported on but should exist while the cache is enabled (like Cluster)
            virtual void CreateDefaultEntities() {}

            virtual HealthEntitySPtr AddNewEntityCallerHoldsLock(
                AttributesStoreDataSPtr && attributes,
                HealthEntityState::Enum entityState) = 0;

            virtual AttributesStoreDataSPtr CreateAttributes(
                TEntityId const & entityId,
                Store::ReplicaActivityId const & replicaActivityId) const = 0;

            virtual AttributesStoreDataSPtr CreateAttributes(
                TEntityId const & entityId,
                ReportRequestContext const & context) const = 0;

            virtual Common::ErrorCode LoadEntities(
                Common::ActivityId const & activityId,
                __inout std::vector<AttributesStoreDataSPtr> & attributes,
                __inout std::vector<SequenceStreamStoreDataUPtr> & sequenceStreams) const = 0;

            static std::vector<HealthEventStoreDataUPtr> GetEntityEvents(std::vector<EntityHealthEventStoreData<TEntityId>> && events);

        private:
            // Initial operation of reading all store entities.
            // Must be done before accepting any new operations.
            // Retries are executed on errors.
            void LoadStoreDataTimerCallback();
            void LoadEntitiesAttributes(__in Common::ActivityId & activityId);
            void LoadStoreEntitiesEvents(
                Common::ActivityId const & activityId,
                std::vector<HealthEntitySPtr> && entities);

            void CleanupEntitiesTimerCallback();

            bool TryGetEntityCallerHoldsLock(
                TEntityId const & entityId,
                __out HealthEntitySPtr & entity) const;

            // Either gets existing or creates new entity.
            void GetExistingOrCreateEntityCallerHoldsLock(
                TEntityId const & entityId,
                ReportRequestContext const & context,
                __out HealthEntitySPtr & entity);

            // Persist sequence stream async processing
            Common::ErrorCode PreparePersistSequenceStreamTx(
                Store::ReplicaActivityId const & replicaActivityId,
                bool updateExisting,
                __out Store::IStoreBase::TransactionSPtr & tx);

            void StartPersistSequenceStream(
                IHealthJobItemSPtr const & jobItem,
                SequenceStreamRequestContext const & context,
                bool updateExisting);

            void OnPersistSequenceStreamCompleted(
                __in IHealthJobItem & jobItem,
                Common::AsyncOperationSPtr const & operation);

            Common::ErrorCode TryReserveCleanupJobIfNotThrottled(
                Common::ActivityId const & activityId,
                __in HealthEntitySPtr & entity,
                int rejectedCount) const;

        private:
            HealthManagerReplica & healthManagerReplica_;

            using HealthEntityPair = std::pair<TEntityId, HealthEntitySPtr>;
            using HealthEntityMap = std::map<TEntityId, HealthEntitySPtr>;
            HealthEntityMap entities_;

            using SequenceStreamMap = std::map<std::wstring, SequenceStreamStoreDataUPtr>;
            SequenceStreamMap sequenceStreamMap_;
            SequenceStreamStoreDataUPtr pendingSequenceStream_;

            bool acceptRequests_;
            mutable GENERIC_RWLOCK(HealthEntityCacheLockNameTraits<TEntityId>, lock_);

            // If set, shows that all attributes and associated events are read from the store.
            volatile bool allDataLoadedInMemory_;
            
            // Entities pending load from store, when primary is opening
            std::vector<HealthEntitySPtr> nonInitializedEntities_;

            // Cleanup timer that periodically starts removing old entities and events on each of the cache type
            Common::TimerSPtr cleanupTimer_;

            Common::TimerSPtr loadStoreDataTimer_;
        };

        class HealthEntityCacheHelper
        {
        public:
            template <class TEntity>
            static TEntity * GetCastedEntityPtr(HealthEntitySPtr const & entity)
            {
#ifdef DBG
                return dynamic_cast<TEntity*>(entity.get());
#else
                return static_cast<TEntity*>(entity.get());
#endif
            }

            template <class TEntityId, class TAttributesType>
            static Common::ErrorCode LoadEntities(
                __in HealthManagerReplica & hm,
                Common::ActivityId const & activityId,
                __inout std::vector<AttributesStoreDataSPtr> & attributes,
                __inout std::vector<SequenceStreamStoreDataUPtr> & sequenceStreams)
            {
                // Read all attributes from store and create entities
                auto tx = hm.StoreWrapper.CreateTransaction(activityId);

                // Read all attributes for this entity type and build the entities for them.
                // Note that this operation is read only, so it performs no cleanup for expired entries
                std::vector<TAttributesType> entities;
                auto error = hm.StoreWrapper.ReadPrefix<TAttributesType>(
                    tx,
                    L"", // read all entries with this type, without any prefix
                    entities);

                // Read all sequence streams for this entity type
                std::vector<EntitySequenceStreamStoreData<TEntityId>> entitySequenceStreams;
                if (error.IsSuccess())
                {
                    error = hm.StoreWrapper.ReadPrefix<EntitySequenceStreamStoreData<TEntityId>>(
                        tx,
                        L"", // read all entries with this type, without any prefix
                        entitySequenceStreams);
                }

                tx.CommitReadOnly();

                for (auto it = entities.begin(); it != entities.end(); ++it)
                {
                    it->OnCompleteLoadFromStore();
                    attributes.push_back(std::make_shared<TAttributesType>(move(*it)));
                }

                for (auto it = entitySequenceStreams.begin(); it != entitySequenceStreams.end(); ++it)
                {
                    it->HighestLsn = it->UpToLsn;
                    sequenceStreams.push_back(Common::make_unique<EntitySequenceStreamStoreData<TEntityId>>(move(*it)));
                }

                return error;
            }
        };

#define HEALTH_ENTITY_CACHE_TEMPLATED_METHODS_DECLARATIONS( ENTITY_ID, ENTITY ) \
    public:\
        static ENTITY * GetCastedEntityPtr(HealthEntitySPtr const & entity); \
    protected: \
        virtual HealthEntitySPtr AddNewEntityCallerHoldsLock(AttributesStoreDataSPtr && attributes, HealthEntityState::Enum entityState) override; \
        virtual AttributesStoreDataSPtr CreateAttributes(ENTITY_ID const & entityId, Store::ReplicaActivityId const & replicaActivityId) const override; \
        virtual AttributesStoreDataSPtr CreateAttributes(ENTITY_ID const & entityId, ReportRequestContext const & context) const override; \
        virtual Common::ErrorCode LoadEntities(Common::ActivityId const & activityId, __inout std::vector<AttributesStoreDataSPtr> & entities, __inout std::vector<SequenceStreamStoreDataUPtr> & sequenceStreams) const override; \


#define HEALTH_ENTITY_CACHE_ENTITY_TEMPLATED_METHODS_DEFINITIONS( ENTITY_CACHE, ENTITY_ID, ENTITY, ATTRIB ) \
    Common::ErrorCode ENTITY_CACHE::LoadEntities(Common::ActivityId const & activityId, __inout std::vector<AttributesStoreDataSPtr> & entities, __inout std::vector<SequenceStreamStoreDataUPtr> & sequenceStreams) const { \
        return HealthEntityCacheHelper::LoadEntities<ENTITY_ID, ATTRIB>(this->HealthManagerReplicaObj, activityId, entities, sequenceStreams); \
    } \
    ENTITY * ENTITY_CACHE::GetCastedEntityPtr(HealthEntitySPtr const & entity) { \
        return HealthEntityCacheHelper::GetCastedEntityPtr<ENTITY>(entity); \
    } \
    HealthEntitySPtr ENTITY_CACHE::AddNewEntityCallerHoldsLock(AttributesStoreDataSPtr && attributes, HealthEntityState::Enum entityState) { \
        auto entity = std::make_shared<ENTITY>(std::move(attributes), this->HealthManagerReplicaObj, entityState); \
        this->AddEntityCallerHoldsLock(entity->EntityId, entity); \
        return entity; \
    } \
    AttributesStoreDataSPtr ENTITY_CACHE::CreateAttributes(ENTITY_ID const & entityId, Store::ReplicaActivityId const & replicaActivityId) const { \
        return std::make_shared<ATTRIB>(entityId, replicaActivityId); \
    } \
    AttributesStoreDataSPtr ENTITY_CACHE::CreateAttributes(ENTITY_ID const & entityId, ReportRequestContext const & context) const { \
        return std::make_shared<ATTRIB>(entityId, context); \
    } \


    }
}
