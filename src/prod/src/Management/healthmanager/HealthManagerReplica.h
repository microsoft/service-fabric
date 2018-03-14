// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class ReportRequestContext;
        class SequenceStreamRequestContext;
        class IQueryProcessor;

        // HealthManager is co-located with ClusterManager.
        // The HM replica is created and managed by the CM replica. It only exists when the replica role is primary 
        // and is closed when the replica is demoted or closed/aborted
        // Because of this, HM and CM are using the same replicated store, created by the CM replica.
        // In the future, we may want to decouple the two,
        // in which case the HM Replica will have to be a Store::KeyValueStoreReplica to create its own replicated store.
        // Note that HMReplica is component root to ensure it is kept alive while processing messages/
        // It is also a rooted object, because it has to keep alive the objects held by reference (Federation) and kept alive by the parent root.
        class HealthManagerReplica 
            : public IHealthAggregator
            , public Store::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::HealthManager>
            , public Common::RootedObject
            , public Common::ComponentRoot
        {
            DENY_COPY(HealthManagerReplica);

        public:
            HealthManagerReplica(
                __in Store::KeyValueStoreReplica &,
                Common::Guid partitionId,
                FABRIC_REPLICA_ID replicaId,
                __in Federation::FederationSubsystem & federation, 
                Common::ComponentRoot const & parentRoot);

            virtual ~HealthManagerReplica();

            __declspec(property(get=get_NodeInstance)) Federation::NodeInstance const & NodeInstance;
            Federation::NodeInstance const & get_NodeInstance() const { return federation_.Instance; }

            __declspec(property(get=get_EntityManager)) EntityCacheManagerUPtr const & EntityManager;
            EntityCacheManagerUPtr const & get_EntityManager() const { return entityManager_; }

            __declspec (property(get=get_HealthManagerCounters)) HealthManagerCountersSPtr const & HealthManagerCounters;
            HealthManagerCountersSPtr const & get_HealthManagerCounters() const { return healthManagerCounters_; }
            
            __declspec (property(get=get_StoreWrapper)) ReplicatedStoreWrapper & StoreWrapper;
            ReplicatedStoreWrapper & get_StoreWrapper() { return *storeWrapper_; }

            __declspec (property(get=get_JobQueueManager)) EntityJobQueueManager & JobQueueManager;
            EntityJobQueueManager & get_JobQueueManager() { return *jobQueueManager_; }

            __declspec (property(get=get_QueryMessageHandlerObj)) Query::QueryMessageHandlerUPtr const & QueryMessageHandlerObj;
            Query::QueryMessageHandlerUPtr const & get_QueryMessageHandlerObj() const { return queryMessageHandler_; }

            Common::ErrorCode AddQuery(
                Common::AsyncOperationSPtr const & queryOperation,
                __in IQueryProcessor * queryProcessor);

            void QueueReportForCompletion(ReportRequestContext && context, Common::ErrorCode && error);
            void QueueSequenceStreamForCompletion(SequenceStreamRequestContext && context, Common::ErrorCode && error);

            // *****************
            // IHealthAggregator
            // *****************

            Common::AsyncOperationSPtr BeginUpdateApplicationsHealth(
                Common::ActivityId const & activityId,
                std::vector<ServiceModel::HealthReport> && reports,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent) override;
            Common::ErrorCode EndUpdateApplicationsHealth(
                Common::AsyncOperationSPtr const & operation) override;

            Common::AsyncOperationSPtr BeginUpdateClusterHealthPolicy(
                Common::ActivityId const & activityId,
                ServiceModel::ClusterHealthPolicySPtr const & healthPolicy,
                ServiceModel::ClusterUpgradeHealthPolicySPtr const & updateHealthPolicy,
                ServiceModel::ApplicationHealthPolicyMapSPtr const & applicationHealthPolicies,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent) override;
            Common::ErrorCode EndUpdateClusterHealthPolicy(
                Common::AsyncOperationSPtr const & operation) override;

            Common::ErrorCode IsApplicationHealthy(
                Common::ActivityId const & activityId,
                std::wstring const & applicationName,
                std::vector<std::wstring> const & upgradeDomains,
                ServiceModel::ApplicationHealthPolicy const * policy,
                __inout bool & result,
                __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations) const override;

            Common::ErrorCode GetClusterUpgradeSnapshot(
                Common::ActivityId const & activityId,
                __inout ClusterUpgradeStateSnapshot & snapshot) const override;

            Common::ErrorCode AppendToClusterUpgradeSnapshot(
                Common::ActivityId const & activityId,
                std::vector<std::wstring> const & upgradeDomains,
                __inout ClusterUpgradeStateSnapshot & snapshot) const override;

            Common::ErrorCode IsClusterHealthy(
                Common::ActivityId const & activityId,
                std::vector<std::wstring> const & upgradeDomains,
                ClusterUpgradeStateSnapshot const & baseline,
                __inout bool & result,
                __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations,
                __inout std::vector<std::wstring> & applicationsWithoutAppType) const override;

            // Method used by FabricTest. 
            // It needs to pass in specified policies rather than used the saved policies.
            Common::ErrorCode Test_IsClusterHealthy(
                Common::ActivityId const & activityId,
                std::vector<std::wstring> const & upgradeDomains,
                ServiceModel::ClusterHealthPolicySPtr const & policy,
                ServiceModel::ClusterUpgradeHealthPolicySPtr const & upgradePolicy,
                ServiceModel::ApplicationHealthPolicyMapSPtr const & applicationHealthPolicies,
                ClusterUpgradeStateSnapshot const & baseline,
                __inout bool & result,
                __inout std::vector<ServiceModel::HealthEvaluation> & unhealthyEvaluations,
                __inout std::vector<std::wstring> & applicationsWithoutAppType) const;
                        
            // *******************************************
            // State methods called by KeyValueStoreReplica methods 
            // of the CM replica that logically contains the HM replica
            // to map the stateful service state
            // *******************************************

            Common::ErrorCode OnOpen(
                __in Store::IReplicatedStore * store,
                SystemServices::SystemServiceMessageFilterSPtr const & messageFilter);

            Common::ErrorCode OnClose();
            
            void OnAbort();
            
            // *******************************************
            // Helper methods
            // *******************************************
            Common::ErrorCode StartCommitJobItem(
                IHealthJobItemSPtr const & jobItem,
                PrepareTransactionCallback const & prepareTxCallback,
                TransactionCompletedCallback const & txCompletedCallback,
                Common::TimeSpan const timeout);

            void OnCacheLoadCompleted();

            void ReportTransientFault(Common::ErrorCode const & fatalError);

            // *******************************************
            // Test hooks
            // *******************************************
            Common::AsyncOperationSPtr Test_BeginProcessRequest(
                Transport::MessageUPtr && message,
                Common::ActivityId const & activityId,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            Common::ErrorCode Test_EndProcessRequest(
                Common::AsyncOperationSPtr const &,
                __out Transport::MessageUPtr & reply);

            bool Test_IsOpened() const;

        protected:
            virtual Common::AsyncOperationSPtr BeginProcessQuery(
                Query::QueryNames::Enum queryName, 
                ServiceModel::QueryArgumentMap const & queryArgs, 
                Common::ActivityId const & activityId,
                Common::TimeSpan timeout,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent);
            virtual Common::ErrorCode EndProcessQuery(
                Common::AsyncOperationSPtr const & operation, 
                _Out_ Transport::MessageUPtr & reply);

        private:
            Common::ErrorCode RegisterMessageHandler();
            bool TryUnregisterMessageHandler();
            
            void RequestMessageHandler(
                Transport::MessageUPtr && message, 
                Federation::RequestReceiverContextUPtr && receiverContext);
                        
            Common::AsyncOperationSPtr StartProcessRequest(
                Transport::MessageUPtr && message,
                Federation::RequestReceiverContextUPtr && receiverContext,
                Common::ActivityId const & activityId,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent);
            void OnProcessRequestComplete(
                Common::ActivityId const & activityId, 
                Transport::MessageId const & message, 
                Common::AsyncOperationSPtr const & operation);

            Common::ErrorCode GetClusterEntity(
                Common::ActivityId const & activityId, 
                __inout ClusterEntitySPtr & cluster) const;

            Common::ErrorCode GetApplicationEntity(
                Common::ActivityId const & activityId,
                std::wstring const & applicationName,
                __inout HealthEntitySPtr & entity) const;
            
            Common::ErrorCode ToNamingGatewayReply(Common::ErrorCode const & error);

        private:
            class UpdateClusterHealthPolicyAsyncOperation;

            class JobQueueHolder;
            using JobQueueHolderUPtr = std::unique_ptr<JobQueueHolder>;

            // Reference to the parent (CM) base class
            // Needed to call base class methods, like ReportFault. 
            Store::KeyValueStoreReplica & kvsReplica_;

            // This replica is kept alive by the cluster manager replica. Since the lifetime of the two are different
            // but HM needs to access resources from CM (like the store), we need to keep the hierarchy alive
            // for as long as this replica is alive.
            Common::ComponentRootSPtr fabricRoot_;

            Federation::FederationSubsystem & federation_;
          
            SystemServices::SystemServiceMessageFilterSPtr messageFilterSPtr_;

            // Wrapper that holds a reference to the ReplicatedStore.
            // The store is initialized by CM replica, on Open.
            ReplicatedStoreWrapperUPtr storeWrapper_;

            Query::QueryMessageHandlerUPtr queryMessageHandler_;

            // Job queue manager that schedules entities work
            EntityJobQueueManagerUPtr jobQueueManager_;

            // Job queue holder that holds different job queues used for query processing 
            // and completing reports and sequence streams.
            JobQueueHolderUPtr jobQueueHolder_;

            // The entity manager that keeps track of all events and attributes for each entity.
            EntityCacheManagerUPtr entityManager_;
            
            Common::atomic_bool acceptRequests_;

            // Performance counters written by this partitioned replica
            HealthManagerCountersSPtr healthManagerCounters_;
        };
    }
}
