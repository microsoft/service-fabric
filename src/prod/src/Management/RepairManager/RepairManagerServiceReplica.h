// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        class RepairManagerServiceReplica;
        typedef std::function<void(Common::Guid const &)> CloseReplicaCallback;

        class RepairManagerServiceReplica 
            : public Store::KeyValueStoreReplica
            , protected Common::TextTraceComponent<Common::TraceTaskCodes::RepairManager>
        {
            using Common::TextTraceComponent<Common::TraceTaskCodes::RepairManager>::WriteNoise;
            using Common::TextTraceComponent<Common::TraceTaskCodes::RepairManager>::WriteInfo;
            using Common::TextTraceComponent<Common::TraceTaskCodes::RepairManager>::WriteWarning;
            using Common::TextTraceComponent<Common::TraceTaskCodes::RepairManager>::WriteError;
            using Common::TextTraceComponent<Common::TraceTaskCodes::RepairManager>::WriteTrace;

            DENY_COPY(RepairManagerServiceReplica);

        public:
            RepairManagerServiceReplica(
                Common::Guid const & partitionId,
                FABRIC_REPLICA_ID replicaId,
                std::wstring const & serviceName,
                __in SystemServices::ServiceRoutingAgentProxy & routingAgentProxy,
                Api::IClientFactoryPtr const & clientFactory,
                Federation::NodeInstance const & nodeInstance,
                RepairManagerServiceFactoryHolder const & factoryHolder);

            virtual ~RepairManagerServiceReplica();

            __declspec(property(get=get_NodeInstance)) Federation::NodeInstance const & NodeInstance;
            Federation::NodeInstance const & get_NodeInstance() const { return nodeInstance_; }

            __declspec(property(get=get_ServiceName)) std::wstring const & ServiceName;
            std::wstring const & get_ServiceName() const { return serviceName_; }

            __declspec(property(get=get_ClusterManagementClient)) Api::IClusterManagementClientPtr const & ClusterManagementClient;
            Api::IClusterManagementClientPtr const & get_ClusterManagementClient() const { return clusterMgmtClient_; }

            __declspec(property(get = get_HealthClient)) Api::IHealthClientPtr const & HealthClient;
            Api::IHealthClientPtr const & get_HealthClient() const { return healthClient_; }

            __declspec(property(get=get_State)) RepairManagerState::Enum State;
            RepairManagerState::Enum get_State() const;

            CloseReplicaCallback OnCloseReplicaCallback;

            Common::AsyncOperationSPtr BeginAcceptCreateRepairRequest(
                RepairTask &&,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptCreateRepairRequest(
                Common::AsyncOperationSPtr const & op,
                __out int64 & commitVersion) { return EndAcceptRequest(op, commitVersion); }

            Common::AsyncOperationSPtr BeginAcceptCancelRepairRequest(
                RepairScopeIdentifierBase const &,
                std::wstring const &,
                int64 const,
                bool const,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptCancelRepairRequest(
                Common::AsyncOperationSPtr const & op,
                __out int64 & commitVersion) { return EndAcceptRequest(op, commitVersion); }

            Common::AsyncOperationSPtr BeginAcceptForceApproveRepair(
                RepairScopeIdentifierBase const &,
                std::wstring const &,
                int64 const,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptForceApproveRepair(
                Common::AsyncOperationSPtr const & op,
                __out int64 & commitVersion) { return EndAcceptRequest(op, commitVersion); }

            Common::AsyncOperationSPtr BeginAcceptDeleteRepairRequest(
                RepairScopeIdentifierBase const &,
                std::wstring const &,
                int64 const,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptDeleteRepairRequest(
                Common::AsyncOperationSPtr const & op) { return EndAcceptRequest(op); }

            Common::AsyncOperationSPtr BeginAcceptUpdateRepairExecutionState(
                RepairTask const &,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptUpdateRepairExecutionState(
                Common::AsyncOperationSPtr const & op,
                __out int64 & commitVersion) { return EndAcceptRequest(op, commitVersion); }

            Common::AsyncOperationSPtr BeginAcceptUpdateRepairTaskHealthPolicy(
                RepairScopeIdentifierBase const &,
                std::wstring const &,
                int64 const,                
                std::shared_ptr<bool> const &,
                std::shared_ptr<bool> const &,
                ClientRequestSPtr &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptUpdateRepairTaskHealthPolicy(
                Common::AsyncOperationSPtr const & op,
                __out int64 & commitVersion) { return EndAcceptRequest(op, commitVersion); }

            Common::ErrorCode ProcessQuery(
                Query::QueryNames::Enum queryName,
                ServiceModel::QueryArgumentMap const & queryArgs,
                Common::ActivityId const & activityId,
                __out Transport::MessageUPtr & reply);

            Common::AsyncOperationSPtr RejectInvalidMessage(
                ClientRequestSPtr &&,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            Common::TimeSpan GetRandomizedOperationRetryDelay(Common::ErrorCode const &) const;

            Common::ErrorCode BeginOperation(Store::ReplicaActivityId const & activityId);
            void EndOperation(Store::ReplicaActivityId const & activityId);

            void CreditCompletedTask(Store::ReplicaActivityId const & activityId);
            void TryRemoveTask(RepairTaskContext const & context, Store::ReplicaActivityId const & activityId);

            Store::StoreTransaction CreateTransaction(Common::ActivityId const &) const;            
            Common::ErrorCode GetHealthCheckStoreData(
                __in Store::ReplicaActivityId const & replicaActivityId,
                __out HealthCheckStoreData & healthCheckStoreData);
            Common::ErrorCode GetHealthCheckStoreData(
                __in Store::StoreTransaction const & storeTx,
                __in Store::ReplicaActivityId const & replicaActivityId, 
                __out HealthCheckStoreData & healthCheckStoreData);

            Common::DateTime GetHealthLastQueriedTime() const;
            void UpdateHealthLastQueriedTime();

            void BeginGetNodeList(
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& parent);
            Common::ErrorCode OnGetNodeListComplete(
                Store::ReplicaActivityId const & activityId,
                Common::AsyncOperationSPtr const & operation);

        protected:
            // *******************
            // StatefulServiceBase
            // *******************
            virtual Common::ErrorCode OnOpen(Common::ComPointer<IFabricStatefulServicePartition> const & servicePartition);
            virtual Common::ErrorCode OnChangeRole(::FABRIC_REPLICA_ROLE newRole, __out std::wstring & serviceLocation);
            virtual Common::ErrorCode OnClose();
            virtual void OnAbort();

        private:
            
            class FinishAcceptRequestAsyncOperation;

            Common::ErrorCode EndAcceptRequest(Common::AsyncOperationSPtr const &);
            Common::ErrorCode EndAcceptRequest(Common::AsyncOperationSPtr const &, int64 &);

            // ****************
            // Message Handling
            // ****************
            void RegisterMessageHandler();
            void UnregisterMessageHandler();

            void RequestMessageHandler(Transport::MessageUPtr &&, Transport::IpcReceiverContextUPtr &&);
            void OnProcessRequestComplete(Common::ActivityId const &, Transport::MessageId const &, Common::AsyncOperationSPtr const &);

            static Common::GlobalWString RejectReasonMissingActivityHeader;
            static Common::GlobalWString RejectReasonMissingTimeoutHeader;
            static Common::GlobalWString RejectReasonIncorrectActor;
            static Common::GlobalWString RejectReasonInvalidAction;

            bool ValidateClientMessage(__in Transport::MessageUPtr &, __out std::wstring & rejectReason);

            Common::AsyncOperationSPtr FinishAcceptRequest(
                Store::StoreTransaction &&,
                std::shared_ptr<RepairTaskContext> &&,
                Common::TimeSpan const,
                Common::AsyncCallback const &, 
                Common::AsyncOperationSPtr const &);

            Common::AsyncOperationSPtr RejectRequest(
                ClientRequestSPtr &&,
                Common::ErrorCode &&,
                Common::AsyncCallback const &,
                Common::AsyncOperationSPtr const &);

            Common::AsyncOperationSPtr BeginProcessRequest(
                Transport::MessageUPtr && request,
                Transport::IpcReceiverContextUPtr && receiverContext,
                Common::ActivityId const & activityId,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndProcessRequest(
                Common::AsyncOperationSPtr const & asyncOperation,
                Transport::MessageUPtr & reply,
                Transport::IpcReceiverContextUPtr & receiverContext);

            Common::AsyncOperationSPtr BeginProcessQuery(
                Query::QueryNames::Enum queryName,
                ServiceModel::QueryArgumentMap const & queryArgs,
                Common::ActivityId const & activityId,
                Common::TimeSpan timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndProcessQuery(
                Common::AsyncOperationSPtr const & operation,
                Transport::MessageUPtr & reply);

            ServiceModel::QueryResult GetRepairList(
                ServiceModel::QueryArgumentMap const & queryArgs,
                Store::ReplicaActivityId const & replicaActivityId);

            Common::ErrorCode GetRepairList(
                RepairScopeIdentifierBase const & scope,
                std::vector<RepairTask> & resultList,
                std::wstring const & taskIdPrefix,
                RepairTaskState::Enum const stateMask,
                std::wstring const & executorName,
                Store::ReplicaActivityId const & replicaActivityId);
            
            class ChargeTask;

            bool TryChargeNewTask(Store::ReplicaActivityId const & activityId);
            void CreditDeletedTask(Store::ReplicaActivityId const & activityId);
            void CreditTask(Store::ReplicaActivityId const & activityId, bool creditActive, bool creditTotal);

            void SetState(RepairManagerState::Enum const newState);
            void Start();
            void Stop();
            void StartRecovery();
            void ScheduleRecovery(Common::TimeSpan const delay, Store::ReplicaActivityId const & activityId);
            
            void ScheduleCleanup(Common::TimeSpan const delay, Store::ReplicaActivityId const & activityId);
            void RunCleanup(Store::ReplicaActivityId const & activityId);
            void OnCleanupCompleted(Store::ReplicaActivityId const & activityId, Common::AsyncOperationSPtr const & operation);
            
            void RecoverNodeList(Store::ReplicaActivityId const & activityId);
            void OnRecoverNodeListComplete(
                Store::ReplicaActivityId const & activityId,
                Common::AsyncOperationSPtr const & operation);

            void RecoverState(Store::ReplicaActivityId const & activityId);

            void FinishRecovery(Store::ReplicaActivityId const & activityId, Common::ErrorCode const & error);

            void BeginOperationInternal(Store::ReplicaActivityId const & activityId);
            void EndOperationInternal(Store::ReplicaActivityId const & activityId);

            Common::ErrorCode ValidateImpact(RepairTask const & task) const;

            bool TaskNeedsProcessing(RepairTaskContext const &) const;
            void TryEnqueueTask(std::shared_ptr<RepairTaskContext> &&, Store::ReplicaActivityId const & activityId);
            void EnqueueTask(std::shared_ptr<RepairTaskContext> &&, Store::ReplicaActivityId const & activityId);
            void ScheduleBackgroundProcessing(Common::TimeSpan const delay, Store::ReplicaActivityId const & parentActivityId);
            void SnapshotTasks(std::vector<std::shared_ptr<RepairTaskContext>> & tasks);
            void ProcessTasks(Store::ReplicaActivityId const & activityId);
            void OnProcessTasksCompleted(Store::ReplicaActivityId const & activityId, Common::AsyncOperationSPtr const & operation);

            void ScheduleHealthCheck(Common::TimeSpan const delay, Store::ReplicaActivityId const & parentActivityId);            
            void ProcessHealthCheck(Store::ReplicaActivityId const & activityId);
            void OnProcessHealthCheckCompleted(Store::ReplicaActivityId const & activityId, Common::AsyncOperationSPtr const & operation);            
            
            Common::ErrorCode ValidateRepairTaskHealthCheckFlags(__in bool performPreparingHealthCheck, __in bool performRestoringHealthCheck);

            static std::wstring ToWString(FABRIC_REPLICA_ROLE);

            std::wstring const serviceName_;
            Federation::NodeInstance const nodeInstance_;
            SystemServices::SystemServiceLocation serviceLocation_;

            Api::IClientFactoryPtr clientFactory_;
            Api::IQueryClientPtr queryClient_;
            Api::IClusterManagementClientPtr clusterMgmtClient_;
            Api::IHealthClientPtr healthClient_;

            // The reference of the RoutingAgentProxy is held by RepairManagerServiceFactory
            SystemServices::ServiceRoutingAgentProxy & routingAgentProxy_;
            RepairManagerServiceFactoryHolder factoryHolder_;

            Query::QueryMessageHandlerUPtr queryMessageHandler_;

            mutable Common::RwLock lock_;
            RepairManagerState::Enum state_;
            int outstandingOperations_;
            Common::TimerSPtr recoveryTimer_;

            std::set<std::wstring> knownFabricNodes_;

            uint activeRepairTaskCount_;
            uint totalRepairTaskCount_;

            std::map<std::wstring, std::shared_ptr<RepairTaskContext>> busyTasks_;
            Common::TimerSPtr backgroundProcessingTimer_;
            Common::TimerSPtr cleanupTimer_;
            Common::TimerSPtr healthCheckTimer_;

            /// <summary>
            /// Keeps track of when cluster health was last queried (by a repair task). If no repair task has queried
            /// since a long time beyond (config key: HealthCheckOnIdleDuration), then health checks are stopped.
            /// </summary>
            Common::DateTime healthLastQueriedAt_;
        };
    }
}
