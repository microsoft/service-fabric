// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent 
    { 
        class ProxyOutgoingMessage;

        // Portion of reconfiguration agent that resides in the service host
        class ReconfigurationAgentProxy : 
            public Common::RootedObject, 
            public Common::TextTraceComponent<Common::TraceTaskCodes::RAP>, 
            public IReconfigurationAgentProxy,
            public IMessageHandler
        {
            DENY_COPY(ReconfigurationAgentProxy);

        public:
            ReconfigurationAgentProxy(ReconfigurationAgentProxyConstructorParameters & parameters);

            virtual ~ReconfigurationAgentProxy();

            typedef Common::JobQueue< MessageProcessingJobItem<ReconfigurationAgentProxy>, ReconfigurationAgentProxy > RAPCommonJobQueue;
            typedef std::unique_ptr<RAPCommonJobQueue> RAPCommonJobQueueUPtr;
            typedef std::shared_ptr<RAPCommonJobQueue> RAPCommonJobQueueSPtr;

            __declspec (property(get=get_LocalFailoverUnitProxyMap)) LocalFailoverUnitProxyMap & LocalFailoverUnitProxyMapObj;
            LocalFailoverUnitProxyMap & get_LocalFailoverUnitProxyMap() { return *lfuProxyMap_; }

            __declspec(property(get=get_LocalLoadReportingComponent)) LocalLoadReportingComponent & LocalLoadReportingComponentObj;
            LocalLoadReportingComponent & get_LocalLoadReportingComponent() { return *lfuLoadReportingComponent_; }

            __declspec(property(get=get_LocalMessageSenderComponent)) LocalMessageSenderComponent & LocalMessageSenderComponentObj;
            LocalMessageSenderComponent & get_LocalMessageSenderComponent() const { return *lfuMessageSenderComponent_; }

            __declspec(property(get=get_LocalHealthReportingComponent)) LocalHealthReportingComponent & LocalHealthReportingComponentObj;
            LocalHealthReportingComponent & get_LocalHealthReportingComponent() { return *lfuHealthReportingComponent_; }

            // This will be used to send in User Health Reports. 
            __declspec(property(get = get_throttledHealthReportingClient)) Client::IpcHealthReportingClient & ThrottledHealthReportingClientObj;
            Client::IpcHealthReportingClient & get_throttledHealthReportingClient() { return *throttledHealthReportingClient_; }

            __declspec(property(get = get_ipcHealthReportingClient)) Client::IpcHealthReportingClient & IpcHealthReportingClientObj;
            Client::IpcHealthReportingClient & get_ipcHealthReportingClient() { return *ipcHealthReportingClient_; }

            __declspec(property(get=get_ReplicatorFactoryObj)) Reliability::ReplicationComponent::IReplicatorFactory & ReplicatorFactory;
            Reliability::ReplicationComponent::IReplicatorFactory & get_ReplicatorFactoryObj() const { return replicatorFactory_; }

            __declspec(property(get=get_TransactionalReplicatorFactoryObj)) TxnReplicator::ITransactionalReplicatorFactory & TransactionalReplicatorFactory;
            TxnReplicator::ITransactionalReplicatorFactory & get_TransactionalReplicatorFactoryObj() const { return transactionalReplicatorFactory_; }

            __declspec(property(get=get_ApplicationHostObj)) Hosting2::IApplicationHost & ApplicationHostObj;
            Hosting2::IApplicationHost & get_ApplicationHostObj() const { return applicationHost_; }

            __declspec(property(get=get_Id)) ReconfigurationAgentProxyId const & Id;
            ReconfigurationAgentProxyId const & get_Id() const { return id_; }

            __declspec(property(get=get_NodeId)) Federation::NodeId const & NodeId;
            Federation::NodeId const & get_NodeId() const { return id_.NodeId; }

            bool ReportLoad(
                FailoverUnitId const& fuId, 
                std::wstring && serviceName, 
                bool isStateful, 
                Reliability::ReplicaRole::Enum replicaRole, 
                std::vector<LoadBalancingComponent::LoadMetric> && loadMetrics);

            bool SendMessageToRA(ProxyOutgoingMessageUPtr && message);

            Common::AsyncOperationSPtr OnBeginOpen(
                Common::TimeSpan timeout,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent);
            Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const & openAsyncOperation);

            Common::AsyncOperationSPtr OnBeginClose(
                Common::TimeSpan timeout,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent);
            Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const & closeAsyncOperation);

            void OnAbort();

            void ProcessRequest(
                IMessageMetadata const * metadata,
                Transport::MessageUPtr && requestPtr,
                Federation::OneWayReceiverContextUPtr && context) override;

            void ProcessIpcMessage(
                IMessageMetadata const * metadata,
                Transport::MessageUPtr && messagePtr,
                Transport::IpcReceiverContextUPtr && context) override;

            void ProcessRequestResponseRequest(
                IMessageMetadata const * metadata,
                Transport::MessageUPtr && requestPtr,
                Federation::RequestReceiverContextUPtr && context) override;

        private:
            class ActionListExecutorAsyncOperation;
            class ActionListExecutorReplicatorQueryAsyncOperation;
            class CloseAsyncOperation;

            // Message handlers
            void ReplicaOpenMessageHandler(FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext, __out Transport::MessageUPtr & reply, __out bool & isRemoveFailoverUnitProxy);
            void ReplicaCloseMessageHandler(FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext, __out Transport::MessageUPtr & reply, __out bool & isRemoveFailoverUnitProxy);
            void StatefulServiceReopenMessageHandler(FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext, __out Transport::MessageUPtr & reply, __out bool & isRemoveFailoverUnitProxy);
            void UpdateConfigurationMessageHandler(FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext, __out Transport::MessageUPtr & reply);
            void ReplicatorBuildIdleReplicaMessageHandler(FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext, __out Transport::MessageUPtr & reply);
            void ReplicatorRemoveIdleReplicaMessageHandler(FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext, __out Transport::MessageUPtr & reply);
            void ReplicatorGetStatusMessageHandler(FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext, __out Transport::MessageUPtr & reply);
            void ReplicatorUpdateEpochAndGetStatusMessageHandler(FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext, __out Transport::MessageUPtr & reply);
            void CancelCatchupReplicaSetMessageHandler(FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext, __out Transport::MessageUPtr & reply);
            void ReplicaEndpointUpdatedReplyMessageHandler(FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext, __out Transport::MessageUPtr & reply);
            void ReadWriteStatusRevokedNotificationReplyMessageHandler(FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext, __out Transport::MessageUPtr & reply);
            void UpdateServiceDescriptionMessageHandler(FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext, __out Transport::MessageUPtr & reply);
            void QueryMessageHandler(FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext, __out Transport::MessageUPtr & reply);

            // Callbacks
            void ReplicatorGetQueryCompletionCallback(Common::AsyncOperationSPtr const & ActionListExecutorAsyncOperation);

            // Finish handling async processing of various messages
            void FinishReplicaOpenMessageHandling(Common::AsyncOperationSPtr const & actionListExecutorAsyncOperation);
            void FinishReplicaCloseMessageHandling(Common::AsyncOperationSPtr const & actionListExecutorAsyncOperation);
            void FinishStatefulServiceReopenMessageHandling(Common::AsyncOperationSPtr const & actionListExecutorAsyncOperation);
            void FinishUpdateConfigurationMessageHandling(Common::AsyncOperationSPtr const & actionListExecutorAsyncOperation);
            void FinishReplicatorBuildIdleReplicaMessageHandling(Common::AsyncOperationSPtr const & actionListExecutorAsyncOperation);
            void FinishReplicatorRemoveIdleReplicaMessageHandling(Common::AsyncOperationSPtr const & actionListExecutorAsyncOperation);
            void FinishReplicatorGetStatusMessageHandling(Common::AsyncOperationSPtr const & actionListExecutorAsyncOperation);
            void FinishReplicatorUpdateEpochAndGetStatusMessageHandling(Common::AsyncOperationSPtr const & actionListExecutorAsyncOperation);
            void FinishCancelCatchupReplicaSetMessageHandling(Common::AsyncOperationSPtr const & actionListExecutorAsyncOperation);
            void FinishReplicatorGetQueryMessageHandling(Common::AsyncOperationSPtr const & actionListExecutorAsyncOperation);
            void FinishUpdateServiceDescriptionMessageHandling(Common::AsyncOperationSPtr const & actionListExecutorAsyncOperation);

            void RemoveFailoverUnitProxy(FailoverUnitProxySPtr const & faiolverUnitProxySPtr);
            void FinishMessageProcessing(FailoverUnitProxyContext<ProxyRequestMessageBody> & context, Common::ErrorCode result, Transport::MessageUPtr && reply);

            void SendReplyAndTrace(Transport::MessageUPtr && msg);
            void SendReplyAndTrace(Transport::MessageUPtr && msg, Transport::IpcReceiverContext * receiverContext);

            static bool DoesUpdateConfigurationMessageImpactServiceAvailability(ProxyRequestMessageBody const & body);

            Hosting2::IApplicationHost & applicationHost_;
            Transport::IpcClient & ipcClient_;
            Reliability::ReplicationComponent::IReplicatorFactory & replicatorFactory_;
            TxnReplicator::ITransactionalReplicatorFactory & transactionalReplicatorFactory_;

            LocalFailoverUnitProxyMapUPtr lfuProxyMap_;
            LocalLoadReportingComponentUPtr lfuLoadReportingComponent_;
            LocalMessageSenderComponentUPtr lfuMessageSenderComponent_;
            LocalHealthReportingComponentUPtr lfuHealthReportingComponent_;
            Client::IpcHealthReportingClientSPtr throttledHealthReportingClient_;
            Client::IpcHealthReportingClientSPtr ipcHealthReportingClient_;
            ReconfigurationAgentProxyId id_;
            Common::atomic_bool isCloseAlreadyCalled_;

            RAPCommonJobQueueSPtr messageQueue_;
        };
    }
}
