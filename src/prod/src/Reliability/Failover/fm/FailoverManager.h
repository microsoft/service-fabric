// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class FailoverManager :
            public Common::ComponentRoot,
            public Reliability::LoadBalancingComponent::IFailoverManager,
            public IFailoverManager
        {
            DENY_COPY_ASSIGNMENT(FailoverManager);

        public:

            static FailoverManagerSPtr CreateFMM(
                Federation::FederationSubsystemSPtr && federation,
                Client::HealthReportingComponentSPtr const & healthClient,
                Common::FabricNodeConfigSPtr const& nodeConfig);

            static FailoverManagerSPtr CreateFM(
                Federation::FederationSubsystemSPtr && federation,
                Client::HealthReportingComponentSPtr const & healthClient,
                Api::IServiceManagementClientPtr serviceManagementClient,
                Api::IClientFactoryPtr const & clientFactory,
                Common::FabricNodeConfigSPtr const& nodeConfig,
                FailoverManagerStoreUPtr && fmStore,
                ComPointer<IFabricStatefulServicePartition> const& servicePartition,
                Common::Guid const& partitionId,
                int64 replicaId);

            FailoverManager(
                Federation::FederationSubsystemSPtr && federation,
                Client::HealthReportingComponentSPtr const & healthClient,
                Api::IServiceManagementClientPtr serviceManagementClient,
                Common::FabricNodeConfigSPtr const& nodeConfig,
                bool isMaster,
                FailoverManagerStoreUPtr && fmStore,
                ComPointer<IFabricStatefulServicePartition> const& servicePartition,
                std::wstring const& fmId,
                std::wstring const& performanceCounterInstanceName);

            ~FailoverManager();

            FailoverManagerSPtr GetFailoverManagerSPtr()
            {
                return std::static_pointer_cast<FailoverManager>(shared_from_this());
            }

            __declspec(property(get=get_NodeConfig)) Common::FabricNodeConfig const & NodeConfig;
            Common::FabricNodeConfig const & get_NodeConfig() const { return *nodeConfig_; }

            bool get_IsMaster() const { return isMaster_; }

            // Whether the FM is active.
            __declspec (property(get=get_IsActive)) bool IsActive;
            bool get_IsActive() const { return isActive_; }

            // Whether the FM is ready to serve requests. After being activated, the FM is not able to start
            // functioning immediately because it first needs to load data from the store to its cache. Once
            // that is completed, it becomes ready. Requests are rejected if the FM is not ready yet.
            bool get_IsReady() const { return isReady_; }

            __declspec (property(get=get_ReadyTime)) Common::DateTime ReadyTime;
            Common::DateTime get_ReadyTime() const { return readyTime_; }

            __declspec (property(get = get_ActivateTime)) Common::StopwatchTime const & ActivateTime;
            Common::StopwatchTime const & get_ActivateTime() const { return activateTime_; }

            __declspec (property(get=get_Generation, put=set_Generation)) Reliability::GenerationNumber const& Generation;
            Reliability::GenerationNumber const& get_Generation() const { return generation_; }
            Common::ErrorCode SetGeneration(Reliability::GenerationNumber const & value);

            __declspec (property(get = get_HealthReportFactory)) HealthReportFactory & HealthReportFactoryObj;
            HealthReportFactory & get_HealthReportFactory()  { return healthReportFactory_; }

            __declspec (property(get=get_PreOpenFMServiceEopch, put=set_PreOpenFMServiceEopch)) Reliability::Epoch const& PreOpenFMServiceEpoch;
            Reliability::Epoch const& get_PreOpenFMServiceEopch() const { return preOpenFMServiceEopch_; }
            void set_PreOpenFMServiceEopch(Reliability::Epoch const& value) { preOpenFMServiceEopch_ = value; }

            __declspec (property(get=get_Federation)) Federation::FederationSubsystem & Federation;
            Federation::FederationSubsystem & get_Federation() { return *federation_; }

            __declspec (property(get=get_FailoverManagerStore)) FailoverManagerStore& Store;
            FailoverManagerStore& get_FailoverManagerStore() const { return *fmStore_; }

            __declspec (property(get=get_ServiceCache)) ServiceCache& ServiceCacheObj;
            ServiceCache& get_ServiceCache() const { return *serviceCache_; }

            __declspec (property(get=get_FailoverUnitCache)) FailoverUnitCache& FailoverUnitCacheObj;
            FailoverUnitCache& get_FailoverUnitCache() const { return *failoverUnitCache_; }

            __declspec (property(get=get_InBuildFailoverUnitCache)) InBuildFailoverUnitCache& InBuildFailoverUnitCacheObj;
            InBuildFailoverUnitCache& get_InBuildFailoverUnitCache() const { return *inBuildFailoverUnitCache_; }

            __declspec (property(get=get_LoadCache)) LoadCache& LoadCacheObj;
            LoadCache& get_LoadCache() const { return *loadCache_; }

            __declspec (property(get=get_NodeCache)) NodeCache& NodeCacheObj;
            NodeCache& get_NodeCache() const { return *nodeCache_; }

            __declspec (property(get=get_MessageProcessor)) FailoverUnitMessageProcessor& MessageProcessor;
            FailoverUnitMessageProcessor& get_MessageProcessor() const { return *messageProcessor_; }

            __declspec (property(get=get_BackgroundManager)) BackgroundManager& BackgroundManagerObj;
            BackgroundManager& get_BackgroundManager() const { return *backgroundManager_; }

            __declspec (property(get=get_IPlacementAndLoadBalancing)) InstrumentedPLB & PLB;
            InstrumentedPLB & get_IPlacementAndLoadBalancing() const { return *plb_; }

            __declspec (property(get=get_FabricUpgradeManager)) FailoverManagerComponent::FabricUpgradeManager & FabricUpgradeManager;
            FailoverManagerComponent::FabricUpgradeManager & get_FabricUpgradeManager() const { return *fabricUpgradeManager_; }

            __declspec (property(get=get_ProcessingQueue)) FailoverManagerComponent::FailoverUnitJobQueue & ProcessingQueue;
            FailoverManagerComponent::FailoverUnitJobQueue & get_ProcessingQueue() const { return *processingQueue_; }

            __declspec (property(get=get_CommitQueue)) FailoverManagerComponent::CommitJobQueue & CommitQueue;
            FailoverManagerComponent::CommitJobQueue & get_CommitQueue() const { return *commitQueue_; }

            __declspec (property(get=get_CommonQueue)) CommonJobQueue & CommonQueue;
            CommonJobQueue & get_CommonQueue() const { return *commonQueue_; }

            __declspec (property(get = get_QueryQueue)) Common::CommonTimedJobQueue<FailoverManager> & QueryQueue;
            Common::CommonTimedJobQueue<FailoverManager> & get_QueryQueue() const { return *queryQueue_; }

            __declspec (property(get=get_Events)) FailoverManagerComponent::EventSource const& Events;
            FailoverManagerComponent::EventSource const& get_Events() const;

            __declspec (property(get=get_MessageEvents)) FailoverManagerComponent::MessageEventSource const& MessageEvents;
            FailoverManagerComponent::MessageEventSource const& get_MessageEvents() const;

            __declspec (property(get=get_FTEvents)) FailoverManagerComponent::FTEventSource const& FTEvents;
            FailoverManagerComponent::FTEventSource const& get_FTEvents() const;

            __declspec (property(get=get_ServiceEvents)) FailoverManagerComponent::ServiceEventSource const& ServiceEvents;
            FailoverManagerComponent::ServiceEventSource const& get_ServiceEvents() const;

            __declspec (property(get=get_NodeEvents)) FailoverManagerComponent::NodeEventSource const& NodeEvents;
            FailoverManagerComponent::NodeEventSource const& get_NodeEvents() const;

            __declspec (property(get=get_FailoverUnitCounters)) FailoverUnitCountersSPtr const& FailoverUnitCounters;
            FailoverUnitCountersSPtr const& get_FailoverUnitCounters() const { return failoverUnitCountersSPtr_; }

            __declspec (property(get=get_HealthClient)) Client::HealthReportingComponentSPtr const & HealthClient;
            Client::HealthReportingComponentSPtr const & get_HealthClient() const { return healthClient_; }

            __declspec (property(get=get_ClientFactory, put=set_ClientFactory)) Api::IClientFactoryPtr const & ClientFactory;
            Api::IClientFactoryPtr const & get_ClientFactory() const { return clientFactory_; }
            void set_ClientFactory(Api::IClientFactoryPtr const & clientFactory) { clientFactory_ = clientFactory; }

            __declspec (property(get = get_QueueFullThrottle)) FailoverThrottle & QueueFullThrottle;
            FailoverThrottle& get_QueueFullThrottle() { return queueFullThrottle_; }

            __declspec (property(get=get_NetworkInventoryService)) Management::NetworkInventoryManager::NetworkInventoryService & NIS;
            Management::NetworkInventoryManager::NetworkInventoryService & get_NetworkInventoryService() const { return *networkInventoryServiceUPtr_; }

            static Common::Global<FailoverManagerComponent::EventSource> FMEventSource;
            static Common::Global<FailoverManagerComponent::MessageEventSource> FMMessageEventSource;
            static Common::Global<FailoverManagerComponent::FTEventSource> FMFTEventSource;
            static Common::Global<FailoverManagerComponent::ServiceEventSource> FMServiceEventSource;
            static Common::Global<FailoverManagerComponent::NodeEventSource> FMNodeEventSource;

            static Common::Global<FailoverManagerComponent::EventSource> FMMEventSource;
            static Common::Global<FailoverManagerComponent::MessageEventSource> FMMMessageEventSource;
            static Common::Global<FailoverManagerComponent::FTEventSource> FMMFTEventSource;
            static Common::Global<FailoverManagerComponent::ServiceEventSource> FMMServiceEventSource;
            static Common::Global<FailoverManagerComponent::NodeEventSource> FMMNodeEventSource;

            Common::TextTraceWriter const WriteError;
            Common::TextTraceWriter const WriteWarning;
            Common::TextTraceWriter const WriteInfo;
            Common::TextTraceWriter const WriteNoise;

            void TraceWarningAndTestAssert(Common::StringLiteral const& type, std::wstring const& id, Common::StringLiteral format, ...);
            void TraceQueueCounts() const;

            // Open/Close the FM.
            void Open(Common::EventHandler const & readyEvent, Common::EventHandler const & failureEvent);
            SerializedGFUMUPtr Close(bool isStoreCloseNeeded);

            void CreateFMService();

            void CompleteLoad();

            void Broadcast(Transport::MessageUPtr && message);
            void SendToNodeAsync(Transport::MessageUPtr && message, Federation::NodeInstance target);
            void SendToNodeAsync(Transport::MessageUPtr && message, Federation::NodeInstance target, Common::TimeSpan retryTimeout, Common::TimeSpan timeout);

            Common::HHandler RegisterFailoverManagerReadyEvent(Common::EventHandler const & handler);
            bool UnRegisterFailoverManagerReadyEvent(Common::HHandler hHandler);

            Common::HHandler RegisterFailoverManagerFailedEvent(Common::EventHandler const & handler);
            bool UnRegisterFailoverManagerFailedEvent(Common::HHandler hHandler);

            void ProcessGFUMTransfer(GFUMMessageBody & body);

            void ProcessFailoverUnitMovements(
                std::map<Common::Guid, Reliability::LoadBalancingComponent::FailoverUnitMovement> && failoverUnitMovements,
                LoadBalancingComponent::DecisionToken && decisionToken);

            void UpdateAppUpgradePLBSafetyCheckStatus(ServiceModel::ApplicationIdentifier const& appId);

            void OnPLBSafetyCheckProcessingDone(Common::AsyncOperationSPtr const& operation, ServiceModel::ApplicationIdentifier const& appId);

            void UpdateFailoverUnitTargetReplicaCount(Common::Guid const &, int targetCount);

            void OnNodeSequenceNumberUpdated(
                ServiceTypeUpdateKind::Enum serviceTypeUpdateEvent,
                std::vector<ServiceModel::ServiceTypeIdentifier> && serviceTypes,
                uint64 sequenceNum,
                Transport::MessageId const& messageId,
                Federation::NodeInstance const& from);

            static Common::StringLiteral const TraceFabricUpgrade;
            static Common::StringLiteral const TraceApplicationUpgrade;
            static Common::StringLiteral const TraceDeactivateNode;

        private:

            class OneWayMessageHandlerJobItem;
            class RequestReplyMessageHandlerJobItem;
            class QueryMessageHandlerJobItem;

            class NodeSequenceNumberCommitJobItem;

            void OnProcessRequestComplete(
				Common::ActivityId const &,
				Common::AsyncOperationSPtr const &,
				TimedRequestReceiverContextUPtr && requestContext);

            void RouteCallback(Common::AsyncOperationSPtr const& routeOperation, std::wstring const & action, Federation::NodeInstance const & target);

            template<typename T>
            bool TryGetMessageBody(Transport::Message & message, T & body)
            {
                if (!message.GetBody(body))
                {
                    TraceInvalidMessage(message);
                    return false;
                }

                return true;
            }

            void TraceInvalidMessage(Transport::Message & message);

            void StartLoading(bool isRetry);
            void Load();
            Common::ErrorCode InitializeNetworkInventoryService();

            static void AdjustTimestamps(std::vector<LoadInfoSPtr> & loadInfos);

            InstrumentedPLBUPtr CreatePLB(
                std::vector<NodeInfoSPtr> const& nodes,
                std::vector<ApplicationInfoSPtr> const& applications,
                std::vector<ServiceTypeSPtr> const& serviceTypes,
                std::vector<ServiceInfoSPtr> const& services,
                std::vector<FailoverUnitUPtr> const& failoverUnits,
                std::vector<LoadInfoSPtr> const& loads);

            bool IsReadStatusGranted() const;

            void OneWayMessageHandler(Transport::MessageUPtr & message, Federation::OneWayReceiverContextUPtr & oneWayReceiverContext);
            void RequestMessageHandler(Transport::MessageUPtr & message, Federation::RequestReceiverContextUPtr & requestReceiverContext);
            bool ProcessFailoverUnitRequest(Transport::Message & request, Federation::NodeInstance const & from);
            Transport::MessageUPtr ProcessOneWayMessage(Transport::Message & request, Federation::NodeInstance const & from);
            Transport::MessageUPtr ProcessRequestReplyMessage(Transport::Message & request, Federation::NodeInstance const & from);

            void GenerationUpdateRejectMessageHandler(Transport::Message & request, Federation::NodeInstance const & from);
            void ReplicaUpAsyncMessageHandler(Transport::Message & request, Federation::NodeInstance const& from);
            void ReplicaDownAsyncMessageHandler(Transport::Message & request, Federation::NodeInstance const& from);
            void NodeFabricUpgradeReplyAsyncMessageHandler(Transport::Message & request, Federation::NodeInstance const& from);
            void NodeUpgradeReplyHandler(Transport::Message & request, Federation::NodeInstance const& from);
            void CancelFabricUpgradeReplyHandler(Transport::Message & request, Federation::NodeInstance const& from);
            void CancelApplicationUpgradeReplyHandler(Transport::Message & request, Federation::NodeInstance const& from);
            void NodeUpdateServiceReplyHandler(Transport::Message & request, Federation::NodeInstance const& from);
            void NodeDeactivateReplyAsyncMessageHandler(Transport::Message & request, Federation::NodeInstance const& from);
            void NodeActivateReplyAsyncMessageHandler(Transport::Message & request, Federation::NodeInstance const& from);
            void AvailableContainerImagesMessageHandler(Transport::Message & request);

            Transport::MessageUPtr NodeUpMessageHandler(Transport::Message & request, Federation::NodeInstance const & from); 
            Transport::MessageUPtr ChangeNotificationMessageHandler(Transport::Message & request, Federation::NodeInstance const & from);
            Transport::MessageUPtr ServiceTableRequestMessageHandler(Transport::Message & reqeust);
            Transport::MessageUPtr ReportLoadMessageHandler(Transport::Message & request);
            Transport::MessageUPtr DeactivateNodeMessageHandler(Transport::Message & request);
            Transport::MessageUPtr ActivateNodeMessageHandler(Transport::Message & request);
            Transport::MessageUPtr DeactivateNodesMessageHandler(Transport::Message & request);
            Transport::MessageUPtr NodeDeactivationStatusRequestMessageHandler(Transport::Message & request);
            Transport::MessageUPtr RemoveNodeDeactivationRequestMessageHandler(Transport::Message & request);
            Transport::MessageUPtr RecoverPartitionsMessageHandler(Transport::Message & request);
            Transport::MessageUPtr RecoverPartitionMessageHandler(Transport::Message & request);
            Transport::MessageUPtr RecoverServicePartitionsMessageHandler(Transport::Message & request);
            Transport::MessageUPtr RecoverSystemPartitionsMessageHandler(Transport::Message & request);
            Transport::MessageUPtr FabricUpgradeRequestHandler(Transport::Message & request);
            Transport::MessageUPtr LFUMUploadMessageHandler(Transport::Message & request, Federation::NodeInstance const & from);
            Transport::MessageUPtr ServiceDescriptionRequestMessageHandler(Transport::Message & request);
            Transport::MessageUPtr ResetPartitionLoadMessageHandler(Transport::Message & request);
            Transport::MessageUPtr ToggleVerboseServicePlacementHealthReportingMessageHandler(Transport::Message & request);

            void CreateServiceAsyncMessageHandler(Transport::Message & request, TimedRequestReceiverContextUPtr && context);
            void OnCreateServiceCompleted(
                TimedRequestReceiverContextUPtr && context,
                Common::ActivityId const& activityId,
                Common::ErrorCode error);

            void DeleteServiceAsyncMessageHandler(Transport::Message & request, TimedRequestReceiverContextUPtr && context);
            void OnDeleteServiceCompleted(
                TimedRequestReceiverContextUPtr && context,
                Common::ActivityId const& activityId,
                Common::ErrorCode error);

            void CreateApplicationAsyncMessageHandler(Transport::Message & request, TimedRequestReceiverContextUPtr && context);
            void OnCreateApplicationCompleted(
                TimedRequestReceiverContextUPtr && context,
                Common::ActivityId const& activityId,
                Common::ErrorCode error);

            void DeleteApplicationAsyncMessageHandler(Transport::Message & request, TimedRequestReceiverContextUPtr && context);
            void OnDeleteApplicationCompleted(
                TimedRequestReceiverContextUPtr && context,
                Common::ActivityId const& activityId,
                Common::ErrorCode error);

            void ServiceTypeNotificationAsyncMessageHandler(Transport::Message & request, Federation::NodeInstance const& from);
            void OnServiceTypeNotificationCompleted(
                Transport::MessageId const& messageId,
                std::vector<ServiceTypeInfo> && processedInfos,
                Federation::NodeInstance const& from);

            void UpdateServiceAsyncMessageHandler(Transport::Message & request, TimedRequestReceiverContextUPtr && context);
            void UpdateSystemServiceAsyncMessageHandler(Transport::Message & request, TimedRequestReceiverContextUPtr && context);
            void OnUpdateServiceCompleted(
                TimedRequestReceiverContextUPtr && context,
                Common::ActivityId const& activityId,
                Common::ErrorCode error);

            void UpdateApplicationAsyncMessageHandler(Transport::Message & request, TimedRequestReceiverContextUPtr && context);
            void OnUpdateApplicationCompleted(
                TimedRequestReceiverContextUPtr && context,
                Common::ActivityId const& activityId,
                Common::ErrorCode error);

            void ApplicationUpgradeAsyncMessageHandler(Transport::Message & request, TimedRequestReceiverContextUPtr && context);
            void OnApplicationUpgradeCompleted(
                Common::AsyncOperationSPtr const& operation,
                ServiceModel::ApplicationIdentifier const& applicationId,
                TimedRequestReceiverContextUPtr const& context);

            void GetQueryAsyncMessageHandler(
                Transport::MessageUPtr & message,
                TimedRequestReceiverContextUPtr && context,
                Common::TimeSpan const& timeout);

            void ServiceTypeEnabledAsyncMessageHandler(
                Transport::Message & request,
                Federation::NodeInstance const& from);
            void ServiceTypeDisabledAsyncMessageHandler(
                Transport::Message & request,
                Federation::NodeInstance const& from);
            void UpdateServiceTypeAsyncMessageHandler(
                Transport::Message & request,
                Federation::NodeInstance const& from,
                ServiceTypeUpdateKind::Enum serviceTypeUpdateEvent);
            void OnUpdateServiceTypeCompleted(
                ServiceTypeUpdateKind::Enum serviceTypeUpdateEvent,
                Transport::MessageId const& messageId,
                uint64 sequenceNum,
                Federation::NodeInstance const& from,
                Common::ErrorCode error);

            void NodeStateRemovedAsyncMessageHandler(Transport::Message & request, TimedRequestReceiverContextUPtr && context);
            void OnNodeStateRemovedCompleted(
                TimedRequestReceiverContextUPtr && context,
                Common::ActivityId const& activityId,
                Common::ErrorCode error);

            Common::ErrorCode RecoverPartitions(std::function<bool (FailoverUnit const& failoverUnit)> const& predicate);
            Common::ErrorCode RecoverPartition(LockedFailoverUnitPtr & failoverUnit);

            void CheckUpgrade(
                Federation::NodeInstance const& from,
                ServicePackageInfo const & package,
                std::wstring const & upgradeDomain,
                std::vector<UpgradeDescription> & upgrades);

            Common::ErrorCode NodeUp(
                Federation::NodeInstance const& from,
                NodeUpMessageBody const& body,
                _Out_ Common::FabricVersionInstance & targetVersionInstance,
                _Out_ std::vector<UpgradeDescription> & upgrades);
            bool NodeDown(Federation::NodeInstance const & nodeInstance);

            void RegisterEvents(Common::EventHandler const & readyEvent, Common::EventHandler const & failureEvent);
            Federation::FederationSubsystemSPtr federation_;
            Client::HealthReportingComponentSPtr healthClient_;
            Api::IClientFactoryPtr clientFactory_;

            Common::FabricNodeConfigSPtr nodeConfig_;

            const bool isMaster_;
            bool isActive_;
            bool isReady_;
            Common::DateTime readyTime_;
            Reliability::GenerationNumber generation_;
            HealthReportFactory healthReportFactory_;

            Reliability::Epoch preOpenFMServiceEopch_;
            Common::ComPointer<IFabricStatefulServicePartition> servicePartition_;

            const Transport::Actor::Enum messageProcessingActor_;

            std::shared_ptr<Common::Event> fmReadyEvent_;
            std::shared_ptr<Common::Event> fmFailedEvent_;

            // The background manager that runs periodically to scan through the failover units
            // and run the state machines to decide the actions to be taken.
            BackgroundManagerUPtr backgroundManager_;

            // The wrapper of the FM store that provides FailoverUnit and node state access.
            FailoverManagerStoreUPtr fmStore_;

            ServiceCacheUPtr serviceCache_;
            FailoverUnitCacheUPtr failoverUnitCache_;
            NodeCacheUPtr nodeCache_;
            LoadCacheUPtr loadCache_;
            InBuildFailoverUnitCacheUPtr inBuildFailoverUnitCache_;

            InstrumentedPLBUPtr plb_;

            FailoverUnitMessageProcessorUPtr messageProcessor_;

            RebuildContextUPtr rebuildContext_;
            FailoverUnitJobQueueUPtr processingQueue_;
            CommitJobQueueUPtr commitQueue_;
            CommonJobQueueUPtr commonQueue_;
            std::unique_ptr<Common::CommonTimedJobQueue<FailoverManager>> queryQueue_;

            FabricUpgradeManagerUPtr fabricUpgradeManager_;

            // This lock serializes important events for the FM, including:
            // - FM Open/Close
            // - Rebuild
            // - Load GFUM
            // Since these events are rare, serializing them should not be a performance concern
            // and simplifies the logic.
            RWLOCK(FM.FailoverManager, lockObject_);

            // Manages store open time
            Common::TimeoutHelper storeOpenTimeoutHelper_;
            Common::StopwatchTime activateTime_;

            FMQueryHelperUPtr fmQueryHelper_;

            FailoverUnitCountersSPtr failoverUnitCountersSPtr_;
            FailoverThrottle queueFullThrottle_;
            
            Api::IServiceManagementClientPtr serviceManagementClient_;

            Management::NetworkInventoryManager::NetworkInventoryServiceUPtr networkInventoryServiceUPtr_;
        };
    }
}
