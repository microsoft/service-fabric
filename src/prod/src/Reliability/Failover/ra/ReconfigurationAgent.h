// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    { 
        // Responsible for local FailoverUnit management and interaction with both FM and Replication
        class ReconfigurationAgent : 
            public Common::RootedObject, 
            public Common::TextTraceComponent<Common::TraceTaskCodes::RA>, 
            public IReconfigurationAgent,
            public IMessageHandler
        {
            DENY_COPY(ReconfigurationAgent);

        public:
            static Federation::NodeInstance InvalidNode;

            ReconfigurationAgent(
                ReconfigurationAgentConstructorParameters const &,
                IRaToRapTransportUPtr && rapTransport,
                Storage::KeyValueStoreFactory::Parameters const & lfumStoreParameters,
                Health::HealthSubsystemWrapperUPtr && health,
                FailoverConfig & config,
                std::shared_ptr<Infrastructure::EntityPreCommitNotificationSink<FailoverUnit>> const & ftPreCommitNotificationSink,
                ThreadpoolFactory threadpoolFactory,
                Diagnostics::IEventWriterUPtr && eventWriter);

            ~ReconfigurationAgent();

            // Used by reliability to instantiate the RA
            static ReconfigurationAgentUPtr Create(
                Common::ComponentRoot const & root,
                IReliabilitySubsystemWrapperSPtr const & reliabilityWrapper,
                IFederationWrapper * federationWrapper,
                Transport::IpcServer & ipcServer,
                Hosting2::IHostingSubsystem & hosting);

           
#pragma region Helper Classes

            // Stores IsOpen/IsReady for FM/FMM replica set
            class State
            {
                DENY_COPY(State);
            public:
                State(ReconfigurationAgent &);

                __declspec(property(get=get_IsOpen)) bool IsOpen;
                bool get_IsOpen() const 
                { 
                    Common::AcquireReadLock grab(lock_);
                    AssertInvariants();
                    return isOpen_;
                }

                __declspec(property(get = get_IsOpenOrClosing)) bool IsOpenOrClosing;
                bool get_IsOpenOrClosing() const
                {
                    Common::AcquireReadLock grab(lock_);
                    AssertInvariants();
                    return isClosing_ || isOpen_;
                }

                __declspec(property(get = get_IsClosing)) bool IsClosing;
                bool get_IsClosing() const
                {
                    Common::AcquireReadLock grab(lock_);
                    AssertInvariants();
                    return isClosing_;
                }

                // Return whether NodeUpAck From FM and FMM have been processed
                __declspec(property(get=get_IsReady)) bool IsReady;
                bool get_IsReady() const 
                { 
                    Common::AcquireReadLock grab(lock_);
                    AssertInvariants();
                    return nodeUpAckFromFMProcessed_ && nodeUpAckFromFmmProcessed_;; 
                }

                __declspec(property(get=get_IsFMReady)) bool IsFMReady;
                bool get_IsFMReady() const 
                { 
                    Common::AcquireReadLock grab(lock_);
                    AssertInvariants();
                    return nodeUpAckFromFMProcessed_;
                }

                __declspec(property(get=get_IsFmmReady)) bool IsFmmReady;
                bool get_IsFmmReady() const 
                { 
                    Common::AcquireReadLock grab(lock_);
                    AssertInvariants();
                    return nodeUpAckFromFmmProcessed_;
                }

                __declspec(property(get = get_IsUpgrading)) bool IsUpgrading;
                bool get_IsUpgrading() const { return isUpgrading_; }

                bool GetIsReady(bool isFMM) const
                {
                    Common::AcquireReadLock grab(lock_);
                    AssertInvariants();
                    return isFMM ? nodeUpAckFromFmmProcessed_ : nodeUpAckFromFMProcessed_;
                }

                bool GetIsReady(FailoverManagerId const & fm) const
                {
                    return GetIsReady(fm.IsFmm);
                }

                void OnOpenBegin();

                void OnOpenComplete();

                void OnNodeUpAckProcessed(Reliability::FailoverManagerId const & fm);

                void OnCloseBegin();

                void OnReplicasClosed();

                void OnCloseComplete();

                void OnFabricCodeUpgradeStart();

                void OnFabricCodeUpgradeRollback();

                void Test_SetIsOpen(bool value)
                {
                    isOpen_ = value;
                }

                void Test_SetIsClosing(bool value)
                {
                    isClosing_ = value;
                }

                void Test_SetNodeUpAckFromFMProcessed(bool value)
                {
                    nodeUpAckFromFMProcessed_ = value;
                }

                void Test_SetNodeUpAckFromFmmProcessed(bool value)
                {
                    nodeUpAckFromFmmProcessed_ = value;
                }

                void Test_SetUpgrading(bool value)
                {
                    isUpgrading_ = value;
                }

            private:
                void AssertInvariants() const;

                ReconfigurationAgent & ra_;

                mutable Common::RwLock lock_;

                bool isClosing_;
                bool isOpen_;
                bool nodeUpAckFromFMProcessed_;
                bool nodeUpAckFromFmmProcessed_;
                bool isUpgrading_;
            };        

#pragma endregion

            __declspec (property(get=get_IsOpen)) bool IsOpen;
            bool get_IsOpen() const { return state_.IsOpen; }

            __declspec (property(get=get_IsFMReady)) bool IsFMReady;
            bool get_IsFMReady() const { return state_.IsFMReady; }

            __declspec(property(get = get_LfumStore)) Storage::Api::IKeyValueStoreSPtr LfumStore;
            Storage::Api::IKeyValueStoreSPtr get_LfumStore() const { return raStoreSPtr_; }

            __declspec (property(get=get_LocalFailoverUnitMap)) Infrastructure::LocalFailoverUnitMap & LocalFailoverUnitMapObj;
            Infrastructure::LocalFailoverUnitMap & get_LocalFailoverUnitMap() { return *lfum_; }

            __declspec(property(get = get_ProcessTerminationService)) Common::ProcessTerminationService & ProcessTerminationService;
            Common::ProcessTerminationService & get_ProcessTerminationService() const { return *processTerminationService_; }

            __declspec(property(get = get_Threadpool)) Infrastructure::IThreadpool & Threadpool;
            Infrastructure::IThreadpool & get_Threadpool() { return *threadpool_; }   
            void Test_SetThreadpool(Infrastructure::IThreadpoolUPtr && threadpool) { threadpool_ = std::move(threadpool);}

            __declspec(property(get=get_JobQueueManager)) Infrastructure::RAJobQueueManager & JobQueueManager;
            Infrastructure::RAJobQueueManager & get_JobQueueManager() { return *jobQueueManager_; }

            __declspec (property(get=get_NodeId)) Federation::NodeId const NodeId;
            Federation::NodeId const get_NodeId() const { return nodeInstance_.Id; }

            __declspec (property(get=get_NodeInstance)) Federation::NodeInstance const& NodeInstance;
            Federation::NodeInstance const& get_NodeInstance() const { return nodeInstance_; }

            // For Tracing
            __declspec (property(get=get_NodeInstanceIdStr)) std::wstring const& NodeInstanceIdStr;
            std::wstring const & get_NodeInstanceIdStr() const { return nodeInstanceIdStr_; }

            __declspec (property(get=get_FederationWrapper)) IFederationWrapper & Federation;
            IFederationWrapper & get_FederationWrapper() { return *federationWrapper_; }

            __declspec(property(get = get_FMTransportObj)) Communication::FMTransport &  FMTransportObj;
            Communication::FMTransport & get_FMTransportObj() { return fmTransport_; }

            __declspec (property(get=get_reliabilityWrapper)) IReliabilitySubsystemWrapper & Reliability;
            IReliabilitySubsystemWrapper & get_reliabilityWrapper() { return *reliability_; }

            __declspec (property(get=get_HostingSubsystem)) Hosting2::IHostingSubsystem & HostingObj;
            Hosting2::IHostingSubsystem & get_HostingSubsystem() { return hostingSubsystem_; }

            __declspec(property(get = get_HostingAdapterObj)) Hosting::HostingAdapter & HostingAdapterObj;
            Hosting::HostingAdapter & get_HostingAdapterObj() { return *hostingAdapter_; }

            __declspec (property(get=get_RAPTransport)) IRaToRapTransport & RAPTransport;
            IRaToRapTransport & get_RAPTransport() { return *rapTransport_; }

            __declspec(property(get = get_Config)) FailoverConfig & Config;
            FailoverConfig & get_Config() { return failoverConfig_; }

            __declspec(property(get = get_Clock)) Infrastructure::IClock& Clock;
            Infrastructure::IClock& get_Clock() const { return *clock_; }

            __declspec(property(get = get_ClockSPtr)) Infrastructure::IClockSPtr ClockSPtr;
            Infrastructure::IClockSPtr get_ClockSPtr() const { return clock_; }

            __declspec(property(get=get_PerfCounters)) Diagnostics::RAPerformanceCounters & PerfCounters;
            Diagnostics::RAPerformanceCounters & get_PerfCounters() { return *perfCounters_; }

            __declspec(property(get = get_PerfCountersSPtr)) Diagnostics::PerformanceCountersSPtr & PerfCountersSPtr;
            Diagnostics::PerformanceCountersSPtr & get_PerfCountersSPtr() { return perfCounters_; }

            __declspec(property(get = get_EventWriter)) Diagnostics::IEventWriter & EventWriter;
            Diagnostics::IEventWriter & get_EventWriter() { return *eventWriter_;  }

            __declspec(property(get = get_MessageMetadataTable)) Communication::MessageMetadataTable const & MessageMetadataTable;
            Communication::MessageMetadataTable const & get_MessageMetadataTable() const { return messageMetadataTable_; }

            __declspec(property(get = get_MessageHandlerObj)) MessageHandler & MessageHandlerObj;
            MessageHandler & get_MessageHandlerObj() { return messageHandler_; }

            __declspec(property(get=get_ReconfigurationMessageRetryWorkManager)) Infrastructure::MultipleEntityBackgroundWorkManager& ReconfigurationMessageRetryWorkManager;
            Infrastructure::MultipleEntityBackgroundWorkManager& get_ReconfigurationMessageRetryWorkManager() { return *reconfigurationMessageRetryWorkManager_; }

            __declspec(property(get=get_StateCleanupWorkManager)) Infrastructure::MultipleEntityBackgroundWorkManager& StateCleanupWorkManager;
            Infrastructure::MultipleEntityBackgroundWorkManager& get_StateCleanupWorkManager() { return *stateCleanupWorkManager_; }

            __declspec(property(get=get_ReplicaCloseRetryWorkManager)) Infrastructure::MultipleEntityBackgroundWorkManager & ReplicaCloseMessageRetryWorkManager;
            Infrastructure::MultipleEntityBackgroundWorkManager & get_ReplicaCloseRetryWorkManager() const { return *replicaCloseMessageRetryWorkManager_; }

            __declspec(property(get=get_ReplicaOpenRetryWorkManager)) Infrastructure::MultipleEntityBackgroundWorkManager &  ReplicaOpenRetryWorkManager;
            Infrastructure::MultipleEntityBackgroundWorkManager &  get_ReplicaOpenRetryWorkManager() { return *replicaOpenWorkManager_; }

            __declspec(property(get=get_UpdateServiceDescriptionMessageRetryWorkManager)) Infrastructure::MultipleEntityBackgroundWorkManager & UpdateServiceDescriptionMessageRetryWorkManager;
            Infrastructure::MultipleEntityBackgroundWorkManager & get_UpdateServiceDescriptionMessageRetryWorkManager() { return *updateServiceDescriptionMessageRetryWorkManager_; }

            __declspec(property(get=get_HealthSubsystem)) Health::IHealthSubsystemWrapper & HealthSubsystem;
            Health::IHealthSubsystemWrapper & get_HealthSubsystem() { return *health_; }

            __declspec(property(get=get_QueryHelperObj)) Query::QueryHelper & QueryHelperObj;
            Query::QueryHelper & get_QueryHelperObj() { return *queryHelper_; }

            __declspec(property(get=get_UpgradeMessageProcessorObj)) Upgrade::UpgradeMessageProcessor & UpgradeMessageProcessorObj;
            Upgrade::UpgradeMessageProcessor & get_UpgradeMessageProcessorObj() { return *upgradeMessageProcessor_; }

            __declspec(property(get=get_NodeDeactivationStateObj)) Node::NodeState & NodeStateObj;
            Node::NodeState & get_NodeDeactivationStateObj() { return *nodeState_; }

            __declspec(property(get=get_GenerationStateManagerObj)) GenerationStateManager & GenerationStateManagerObj;
            GenerationStateManager & get_GenerationStateManagerObj() { return generationStateManager_; }

            __declspec(property(get=get_NodeUpAckProcessorObj)) NodeUpAckProcessor & NodeUpAckProcessorObj;
            NodeUpAckProcessor & get_NodeUpAckProcessorObj() { return *nodeUpAckProcessor_; }

            __declspec(property(get = get_NodeDeactivationMessageProcessorObj)) Node::NodeDeactivationMessageProcessor & NodeDeactivationMessageProcessorObj;
            Node::NodeDeactivationMessageProcessor & get_NodeDeactivationMessageProcessorObj() const { return *nodeDeactivationMessageProcessor_; }

            __declspec(property(get=get_ServiceTypeUpdateProcessorObj)) Node::ServiceTypeUpdateProcessor & ServiceTypeUpdateProcessorObj;
            Node::ServiceTypeUpdateProcessor & get_ServiceTypeUpdateProcessorObj() { return *serviceTypeUpdateProcessor_; }

            __declspec(property(get=get_StateObj)) State & StateObj;
            State & get_StateObj() { return state_; }

            __declspec(property(get = get_CloseReplicasAsyncOperation)) MultipleReplicaCloseAsyncOperationSPtr CloseReplicasAsyncOperation;
            MultipleReplicaCloseAsyncOperationSPtr get_CloseReplicasAsyncOperation() const { return closeReplicasAsyncOp_.lock(); }

            __declspec(property(get = get_EntitySetCollectionObj)) Infrastructure::EntitySetCollection const & EntitySetCollectionObj;
            Infrastructure::EntitySetCollection const & get_EntitySetCollectionObj() const { return entitySets_; }
            Infrastructure::EntitySetCollection & Test_GetSetCollection() { return entitySets_; }

            Store::ILocalStore & Test_GetLocalStore();

#pragma region RA LifeCycle
            Common::ErrorCode Open(NodeUpOperationFactory & nodeUpFactory);
            void Close();            

            void OnFabricUpgradeStart();
            void OnFabricUpgradeRollback();

            bool IsReady(FailoverUnitId const & ftId) const;

#pragma endregion

#pragma region Test

            void Test_SetIsOpen(bool isOpen)
            {
                state_.Test_SetIsOpen(isOpen);

                if (!isOpen)
                {
                    state_.Test_SetNodeUpAckFromFmmProcessed(false);
                    state_.Test_SetNodeUpAckFromFMProcessed(false);
                }
            }

            void Test_SetNodeUpAckFromFMProcessed(bool value)
            {
                state_.Test_SetNodeUpAckFromFMProcessed(value);
            }

            void Test_SetNodeUpAckFromFmmProcessed(bool value)
            {
                state_.Test_SetNodeUpAckFromFmmProcessed(value);
            }

            static ServiceModel::ServicePackageVersionInstance Test_GetServicePackageVersionInstance(
                ServiceModel::ServicePackageVersionInstance const & ftVersionInstance,
                bool isFTUp,
                bool hasRegistration,
                ServiceModel::ServicePackageVersionInstance const & messageVersion,
                ServiceModel::ServicePackageVersionInstance const & upgradeTableVersion)
            {
                return GetServicePackageVersionInstance(ftVersionInstance, isFTUp, hasRegistration, messageVersion, upgradeTableVersion);
            }

#pragma endregion

#pragma region Child Objects
            Infrastructure::EntitySet & GetSet(Infrastructure::EntitySetName::Enum setName);
            Infrastructure::RetryTimer & GetRetryTimer(Infrastructure::EntitySetName::Enum setName);
            Infrastructure::MultipleEntityBackgroundWorkManager & GetMultipleFailoverUnitBackgroundWorkManager(Infrastructure::EntitySetName::Enum setName);
#pragma endregion

#pragma region Message Handlers

#pragma region Functions invoked on transport thread
            void ProcessTransportRequest(
                Transport::MessageUPtr & message, 
                Federation::OneWayReceiverContextUPtr && context);

            void ProcessTransportIpcRequest(
                Transport::MessageUPtr & message, 
                Transport::IpcReceiverContextUPtr && context);

            void ProcessTransportRequestResponseRequest(
                Transport::MessageUPtr & message, 
                Federation::RequestReceiverContextUPtr && context);
#pragma endregion

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

            GenerationProposalReplyMessageBody ProcessGenerationProposal(Transport::Message & request, bool & isGfumUploadOut);
            void ProcessLFUMUploadReply(Reliability::GenerationHeader const & generationHeader);

            void ProcessNodeUpAck(Transport::MessageUPtr && nodeUpReply, bool isFromFMM);
            void NodeUpAckMessageHandler(Transport::Message & request, bool isFromFMM);

            typedef MessageContext<ReplicaMessageBody, FailoverUnit> ReplicaMessageContext;
            typedef MessageContext<DeleteReplicaMessageBody, FailoverUnit> DeleteReplicaMessageContext;
            typedef MessageContext<RAReplicaMessageBody, FailoverUnit> RAReplicaMessageContext;
            typedef MessageContext<GetLSNReplyMessageBody, FailoverUnit> GetLSNReplyMessageContext;
            typedef MessageContext<DoReconfigurationMessageBody, FailoverUnit> DoReconfigurationMessageContext;
            typedef MessageContext<DeactivateMessageBody, FailoverUnit> DeactivateMessageContext;
            typedef MessageContext<ActivateMessageBody, FailoverUnit> ActivateMessageContext;
            typedef MessageContext<ReplicaReplyMessageBody, FailoverUnit> ReplicaReplyMessageContext;
            typedef MessageContext<ReportFaultMessageBody, FailoverUnit> ReportFaultMessageContext;
            typedef MessageContext<ProxyReplyMessageBody, FailoverUnit> ProxyReplyMessageContext;
            typedef MessageContext<ProxyUpdateServiceDescriptionReplyMessageBody, FailoverUnit> ProxyUpdateServiceDescriptionReplyMessageContext;
            typedef MessageContext<FailoverUnitReplyMessageBody, FailoverUnit> FailoverUnitReplyMessageContext;

            // These are received by Primary RA

            void AddInstanceMessageProcessor(Infrastructure::HandlerParameters & parameters, ReplicaMessageContext & context);
            void RemoveInstanceMessageProcessor(Infrastructure::HandlerParameters & parameters, DeleteReplicaMessageContext & context);
            void AddPrimaryMessageProcessor(Infrastructure::HandlerParameters & parameters, ReplicaMessageContext & context);
            void AddReplicaMessageProcessor(Infrastructure::HandlerParameters & parameters, ReplicaMessageContext & context);
            void RemoveReplicaMessageProcessor(Infrastructure::HandlerParameters & parameters, ReplicaMessageContext & context);
            void DeleteReplicaMessageProcessor(Infrastructure::HandlerParameters & parameters, DeleteReplicaMessageContext & context);
            void DoReconfigurationMessageProcessor(Infrastructure::HandlerParameters & parameters, DoReconfigurationMessageContext & context);
            void ReplicaDroppedReplyMessageProcessor(Infrastructure::HandlerParameters & parameters, ReplicaReplyMessageContext & context);
            void ReplicaEndpointUpdatedReplyMessageProcessor(Infrastructure::HandlerParameters & parameters, ReplicaReplyMessageContext & context);

            // These are sent by Primary RA

            void CreateReplicaMessageProcessor(Infrastructure::HandlerParameters & parameters, RAReplicaMessageContext & context);
            void GetLSNMessageProcessor(Infrastructure::HandlerParameters & parameters, ReplicaMessageContext & context);
            void DeactivateMessageProcessor(Infrastructure::HandlerParameters & parameters, DeactivateMessageContext & context);
            void ActivateMessageProcessor(Infrastructure::HandlerParameters & parameters, ActivateMessageContext & context);

            void CreateReplicaReplyMessageProcessor(Infrastructure::HandlerParameters & parameters, ReplicaReplyMessageContext & context);
            void GetLSNReplyMessageProcessor(Infrastructure::HandlerParameters & parameters, GetLSNReplyMessageContext & context);
            void DeactivateReplyMessageProcessor(Infrastructure::HandlerParameters & parameters, ReplicaReplyMessageContext & context);
            void ActivateReplyMessageProcessor(Infrastructure::HandlerParameters & parameters, ReplicaReplyMessageContext & context);

            // These are received from RA-proxy
            void ReplicaOpenReplyHandler(Infrastructure::HandlerParameters & parameters, ProxyReplyMessageContext & msgContext);
            void ReplicaCloseReplyHandler(Infrastructure::HandlerParameters & parameters, ProxyReplyMessageContext & msgContext);
            void UpdateConfigurationReplyHandler(Infrastructure::HandlerParameters & parameters, ProxyReplyMessageContext & msgContext);
            void StatefulServiceReopenReplyHandler(Infrastructure::HandlerParameters & parameters, ProxyReplyMessageContext & msgContext);
            void ReplicatorBuildIdleReplicaReplyHandler(Infrastructure::HandlerParameters & parameters, ProxyReplyMessageContext & msgContext);
            void ReplicatorRemoveIdleReplicaReplyHandler(Infrastructure::HandlerParameters & parameters, ProxyReplyMessageContext & msgContext);
            void ReplicatorGetStatusMessageHandler(Infrastructure::HandlerParameters & parameters, ProxyReplyMessageContext & msgContext);
            void ReplicatorCancelCatchupReplicaSetReplyHandler(Infrastructure::HandlerParameters & parameters, ProxyReplyMessageContext & msgContext);
            void ProxyReplicaEndpointUpdatedMessageHandler(Infrastructure::HandlerParameters & parameters, ReplicaMessageContext & msgContext);
            void ReadWriteStatusRevokedNotificationMessageHandler(Infrastructure::HandlerParameters & parameters, ReplicaMessageContext & msgContext);
            void ProxyUpdateServiceDescriptionReplyMessageHandler(Infrastructure::HandlerParameters & parameters, ProxyUpdateServiceDescriptionReplyMessageContext & msgContext);

            void ReplicaOpenReplyHandlerHelper(
                Infrastructure::LockedFailoverUnitPtr & failoverUnit,
                Reliability::ReplicaDescription const & replicaDescription,
                TransitionErrorCode const & transitionErrorCode,
                Infrastructure::HandlerParameters & parameters);

            void ReplicaCloseReplyHandlerHelper(
                Infrastructure::LockedFailoverUnitPtr & failoverUnit,
                TransitionErrorCode const & transitionErrorCode,
                Infrastructure::HandlerParameters & parameters);

            void ReportLoadMessageHandler(Transport::Message & message);
            void ReportHealthReportMessageHandler(Transport::MessageUPtr && messagePtr, Transport::IpcReceiverContextUPtr && context);
            void ReportResourceUsageHandler(Transport::Message & message);

            void ReportFaultMessageHandler(Infrastructure::HandlerParameters & parameters, ReportFaultMessageContext & msgContext);
            void ReportFaultHandler(Infrastructure::HandlerParameters & parameters, FaultType::Enum faultType, Common::ActivityDescription const & activityDescription);

            bool CanProcessNodeFabricUpgrade(std::wstring const & activityId, ServiceModel::FabricUpgradeSpecification const & newVersion);
            void ProcessNodeFabricUpgradeHandler(std::wstring const & activityId, ServiceModel::FabricUpgradeSpecification const & newVersion);

            void ProcessGenerationUpdate(Transport::Message & request, Federation::NodeInstance const& from);

            void ClientReportFaultRequestHandler(Transport::MessageUPtr && requestPtr, Federation::RequestReceiverContextUPtr && context);

            void ProcessLoadReport(ReportLoadMessageBody const & loadReport);

            void CreateReplicaUpReplyJobItemAndAddToList(
                Reliability::FailoverUnitInfo && ftInfo,
                bool isInDroppedList,
                Infrastructure::MultipleEntityWorkSPtr const & work,
                Infrastructure::RAJobQueueManager::EntryJobItemList & jobItemList,
                std::vector<Reliability::FailoverUnitId> & acknowledgedFTs);
            void ProcessReplicaUpReplyList(
                std::vector<Reliability::FailoverUnitInfo> & ftInfos,
                bool isDroppedList,
                Infrastructure::MultipleEntityWorkSPtr const & work,
                Infrastructure::RAJobQueueManager::EntryJobItemList & jobItemList,
                std::vector<Reliability::FailoverUnitId> & acknowledgedFTs);
            void ReplicaUpReplyMessageHandler(Transport::Message & request);
            void ProcessNodeUpgradeHandler(Transport::Message & request);
            void ProcessNodeUpdateServiceRequest(Transport::Message & request);
            void ProcessNodeFabricUpgradeHandler(Transport::Message & request);
            void ReplicaDroppedReplyHandler(Transport::Message & request);
            void ProcessCancelApplicationUpgradeHandler(Transport::Message & request);
            void ProcessCancelFabricUpgradeHandler(Transport::Message & request);

            void ServiceTypeNotificationReplyHandler(Transport::Message & request);

#pragma endregion

#pragma region Hosting Event Handler
            void ProcessServiceTypeRegistered(Hosting2::ServiceTypeRegistrationSPtr const & registration);

            void ProcessServiceTypeDisabled(
                ServiceModel::ServiceTypeIdentifier const& serviceTypeId,
                uint64 sequenceNumber,
                ServiceModel::ServicePackageActivationContext const& activationContext);

            void ProcessServiceTypeEnabled(
                ServiceModel::ServiceTypeIdentifier const& serviceTypeId,
                uint64 sequenceNumber,
                ServiceModel::ServicePackageActivationContext const& activationContext);
            
            void ProcessRuntimeClosed(std::wstring const & hostId, std::wstring const & runtimeId);
            bool RuntimeClosedProcessor(Infrastructure::HandlerParameters & handlerParamters, JobItemContextBase &);

            void ProcessAppHostClosed(std::wstring const & hostId, Common::ActivityDescription const & acitivityDescription);
            bool AppHostClosedProcessor(Infrastructure::HandlerParameters & handlerParameters, JobItemContextBase &);

            bool ServiceTypeRegisteredProcessor(Infrastructure::HandlerParameters & handlerParameters, JobItemContextBase &);
#pragma endregion

#pragma region Message Sending Functions

            MessageRetry::FMMessageRetryComponent & GetFMMessageRetryComponent(
                Reliability::FailoverManagerId const & target);

#pragma region Remote RA Message Functions

            static void AddActionSendMessageToRA(
                    Infrastructure::StateMachineActionQueue & actionQueue,
                    RSMessage const & message,
                    Federation::NodeInstance const nodeInstance,
                    Reliability::FailoverUnitDescription const & fuDesc, 
                    Reliability::ReplicaDescription const & replicaDesc,
                    Reliability::ServiceDescription const & serviceDesc);

            static void AddActionSendCreateReplicaMessageToRA(
                    Infrastructure::StateMachineActionQueue & actionQueue,
                    Federation::NodeInstance const nodeInstance,
                    Reliability::FailoverUnitDescription const & fuDesc,
                    Reliability::ReplicaDescription const & replicaDesc,
                    Reliability::ServiceDescription const & serviceDesc,
                    Reliability::ReplicaDeactivationInfo const & deactivationInfo);

            static void AddActionSendContinueSwapPrimaryMessageToRA(
                    Infrastructure::StateMachineActionQueue & actionQueue,
                    Federation::NodeInstance const nodeInstance,
                    Reliability::FailoverUnitDescription const & fuDesc, 
                    Reliability::ServiceDescription const & serviceDesc,
                    std::vector<Reliability::ReplicaDescription> && replicaDescList,
                    Common::TimeSpan phase0Duration);

            static void AddActionSendActivateMessageToRA(
                    Infrastructure::StateMachineActionQueue & actionQueue,
                    Federation::NodeInstance const nodeInstance,
                    Reliability::FailoverUnitDescription const & fuDesc,
                    Reliability::ServiceDescription const & serviceDesc,
                    std::vector<Reliability::ReplicaDescription> && replicaDescList,
                    Reliability::ReplicaDeactivationInfo const & deactivationInfo);

            static void AddActionSendDeactivateMessageToRA(
                    Infrastructure::StateMachineActionQueue & actionQueue,
                    Federation::NodeInstance const nodeInstance,
                    Reliability::FailoverUnitDescription const & fuDesc, 
                    Reliability::ServiceDescription const & serviceDesc,
                    std::vector<Reliability::ReplicaDescription> && replicaDescList,
                    Reliability::ReplicaDeactivationInfo const & deactivationINfo,
                    bool isForce);

            static void AddActionSendMessageReplyToRA(
                    Infrastructure::StateMachineActionQueue & actionQueue,
                    RSMessage const & message,
                    Federation::NodeInstance const nodeInstance,
                    Reliability::FailoverUnitDescription const & fuDesc,
                    Reliability::ReplicaDescription const & replicaDesc,
                    Common::ErrorCodeValue::Enum  errCodeValue);

            static void AddActionSendMessageReplyToRA(
                    Infrastructure::StateMachineActionQueue & actionQueue,
                    RSMessage const & message,
                    Federation::NodeInstance const nodeInstance,
                    ReplicaReplyMessageBody && reply);

            static void AddActionSendGetLsnReplicaNotFoundMessageReplyToRA(
                    Infrastructure::StateMachineActionQueue & actionQueue,
                    Federation::NodeInstance const nodeInstance,
                    ReplicaMessageBody const & getLSNBody);

            static void AddActionSendGetLsnMessageReplyToRA(
                    Infrastructure::StateMachineActionQueue & actionQueue,
                    Federation::NodeInstance const nodeInstance,
                    FailoverUnitDescription const & ftDesc,
                    Reliability::ReplicaDescription const & replicaDesc,
                    Reliability::ReplicaDeactivationInfo const & replicaDeactivationInfo,
                    Common::ErrorCodeValue::Enum  errCodeValue);

            static void AddActionSendMessageReplyToRA(
                    Infrastructure::StateMachineActionQueue & actionQueue,
                    RSMessage const & message,
                    Federation::NodeInstance const nodeInstance,
                    Reliability::FailoverUnitDescription const & fuDesc, 
                    std::vector<Reliability::ReplicaDescription> && replicaDescList,
                    Common::ErrorCodeValue::Enum  errCodeValue);
#pragma endregion

#pragma region RAP Message Functions

            // RA -RA-proxy interaction

            static void AddActionSendMessageToRAProxy(
                    Infrastructure::StateMachineActionQueue & actionQueue,
                    RAMessage const & message,
                    Hosting2::ServiceTypeRegistrationSPtr const & serviceTypeRegistration,
                    Reliability::FailoverUnitDescription const & fuDesc, 
                    Reliability::ReplicaDescription const & localReplicaDesc,
                    Reliability::ServiceDescription const & serviceDesc = Reliability::ServiceDescription(),
                    ProxyMessageFlags::Enum flags = ProxyMessageFlags::None);
            
            static void AddActionSendMessageToRAProxy(
                    Infrastructure::StateMachineActionQueue & actionQueue,
                    RAMessage const & message,
                    Hosting2::ServiceTypeRegistrationSPtr const & serviceTypeRegistration,
                    Reliability::FailoverUnitDescription const & fuDesc, 
                    Reliability::ReplicaDescription const & localReplicaDesc,
                    ProxyMessageFlags::Enum flags);
            
            static void AddActionSendMessageToRAProxy(
                    Infrastructure::StateMachineActionQueue & actionQueue,
                    RAMessage const & message,                    
                    Hosting2::ServiceTypeRegistrationSPtr const & serviceTypeRegistration,
                    Reliability::FailoverUnitDescription const & fuDesc, 
                    Reliability::ReplicaDescription const & localReplicaDesc,
                    Reliability::ReplicaDescription const & remoteReplicaDesc);

            static void AddActionSendMessageToRAProxy(
                    Infrastructure::StateMachineActionQueue & actionQueue,
                    RAMessage const & message,                    
                    Hosting2::ServiceTypeRegistrationSPtr const & serviceTypeRegistration,
                    Reliability::FailoverUnitDescription const & fuDesc, 
                    Reliability::ReplicaDescription localReplicaDesc,
                    std::vector<Reliability::ReplicaDescription> && replicaDescList,
                    ProxyMessageFlags::Enum flags = ProxyMessageFlags::None);

#pragma endregion

#pragma endregion

#pragma region Local Replica LifeCycle

            void CloseLocalReplica(
                Infrastructure::HandlerParameters & handlerParameters,
                ReplicaCloseMode closeMode,
                Common::ActivityDescription const & activityDescription);

            void CloseLocalReplica(
                Infrastructure::HandlerParameters & handlerParameters,
                ReplicaCloseMode closeMode,
                Federation::NodeInstance const & senderNode,
                Common::ActivityDescription const & activityDescription);

            bool ReopenDownReplica(
                Infrastructure::HandlerParameters & handlerParameters);

            bool ReopenDownReplica(
                Hosting2::ServiceTypeRegistrationSPtr const & registration,
                Infrastructure::HandlerParameters & handlerParameters);

#pragma endregion

            bool AnyReplicaFound(bool excludeFMServiceReplica);

            bool UpdateServiceDescriptionMessageRetryProcessor(Infrastructure::HandlerParameters & handlerParameters, JobItemContextBase &);

            bool FailoverUnitCleanupProcessor(
                Infrastructure::HandlerParameters & handlerParameters,
                JobItemContextBase &);

            bool ReconfigurationMessageRetryProcessor(Infrastructure::HandlerParameters & handlerParameters, JobItemContextBase &);
            
            bool NodeUpdateServiceDescriptionProcessor(NodeUpdateServiceRequestMessageBody const & body, Infrastructure::HandlerParameters & handlerParameters, NodeUpdateServiceJobItemContext & context);
            void OnNodeUpdateServiceDescriptionProcessingComplete(Infrastructure::MultipleEntityWork & work, NodeUpdateServiceRequestMessageBody const & body);

            bool ReplicaUpReplyProcessor(Infrastructure::HandlerParameters & handlerParameters, ReplicaUpReplyJobItemContext & context);

            bool ClientReportFaultRequestProcessor(Infrastructure::HandlerParameters & handlerParameters, ClientReportFaultJobItemContext & context);
            void CompleteReportFaultRequest(ClientReportFaultJobItemContext & context, Common::ErrorCode const & error);

            void CompleteReportFaultRequest(
                Common::ActivityId const & activityId,
                Federation::RequestReceiverContext & context,
                Common::ErrorCode const & error);

            bool TryUpdateServiceDescription(
                Infrastructure::LockedFailoverUnitPtr & failoverUnit,
                Infrastructure::StateMachineActionQueue & queue,
                ServiceDescription const & newServiceDescription);

            bool CheckIfAddFailoverUnitMessageCanBeProcessed(
                Reliability::ReplicaMessageBody const & msgBody,
                std::function<void (ReplicaReplyMessageBody)> const & replyFunc);

            Common::ErrorCode GetServiceTypeRegistration(
                Reliability::FailoverUnitId const & ftId,
                Reliability::ServiceDescription const & description,
                Hosting2::ServiceTypeRegistrationSPtr & registration);

        private:

#pragma region RA LifeCycle
            Common::ErrorCode InitializeLFUM(NodeUpOperationFactory & factory);
            void CloseAllReplicas();
#pragma endregion

#pragma region Local Replica LifeCycle
            void CreateLocalReplica(
                Infrastructure::HandlerParameters & handlerParameters,
                Reliability::ReplicaMessageBody const & msgBody,
                Reliability::ReplicaRole::Enum localReplicaRole,
                Reliability::ReplicaDeactivationInfo const & deactivationInfo,
                bool isRebuild = false);

            bool ReplicaOpenMessageRetryProcessor(
                Infrastructure::HandlerParameters & handlerParameters, 
                JobItemContextBase &);

            bool ProcessReplicaOpenMessageRetry(
                Infrastructure::HandlerParameters & handlerParameters);

            bool ProcessReplicaOpenMessageRetry(
                Hosting2::ServiceTypeRegistrationSPtr const & registration,
                Infrastructure::HandlerParameters & handlerParameters);

            void FinishOpenLocalReplica(
                Infrastructure::LockedFailoverUnitPtr & failoverUnit,
                Infrastructure::HandlerParameters & handlerParameters,
                RAReplicaOpenMode::Enum openMode,
                Federation::NodeInstance const & senderNode,
                Reliability::ReplicaDescription const & replicaDescription,
                Common::ErrorCodeValue::Enum errorCode);
            
            bool ReplicaCloseMessageRetryProcessor(
                Infrastructure::HandlerParameters & handlerParameters, 
                JobItemContextBase &);

            bool ProcessReplicaCloseMessageRetry(
                Infrastructure::HandlerParameters & handlerParameters);

            bool ProcessReplicaCloseMessageRetry(
                Hosting2::ServiceTypeRegistrationSPtr const & registration,
                Infrastructure::HandlerParameters & handlerParameters);

            void FinishCloseLocalReplica(
                Infrastructure::HandlerParameters & handlerParameters,
                ReplicaCloseMode closeMode,
                Federation::NodeInstance const & senderNode);

            void DeleteLocalReplica(
                Infrastructure::HandlerParameters & handlerParameters,
                Reliability::DeleteReplicaMessageBody const & msgBody);

            static ServiceModel::ServicePackageVersionInstance GetServicePackageVersionInstance(
                ServiceModel::ServicePackageVersionInstance const & ftVersionInstance,
                bool isFTUp,
                bool hasRegistration,
                ServiceModel::ServicePackageVersionInstance const & messageVersion,
                ServiceModel::ServicePackageVersionInstance const & upgradeTableVersion);

#pragma endregion

            void RemoveReplica(
                    Infrastructure::LockedFailoverUnitPtr & failoverUnit, 
                    Reliability::ReplicaMessageBody const & msgBody,
                    Replica & replica,
                    Infrastructure::StateMachineActionQueue & actionQueue);

            bool TryDropFMReplica(
                Infrastructure::HandlerParameters & handlerParameters,
                ReplicaMessageBody const & body);

            void UpdateServiceCodeVersions(std::vector<Reliability::UpgradeDescription> const & upradeDescriptions);

            bool MessageSendCallback();

            ServiceModel::ServicePackageVersionInstance GetServicePackageVersionInstance(
                Infrastructure::LockedFailoverUnitPtr const & ft,
                ServiceModel::ServicePackageIdentifier const & servicePackageId,
                ServiceModel::ServicePackageVersionInstance const & messageVersion);                

            bool TryCreateFailoverUnit(
                ReplicaMessageBody const & messageBody,
                Infrastructure::LockedFailoverUnitPtr & ft);
        
            bool TryCreateFailoverUnit(
                DoReconfigurationMessageBody const & messageBody,
                Infrastructure::LockedFailoverUnitPtr & ft);

#pragma region Functions for helping with Open
            void UpdateNodeInstanceOnOpen();
            void RegisterIpcMessageHandler();
            void CreateJobQueueManager();
            void CreateNodeState();
            void CreateServiceTypeUpdateProcessor();
            void CreateUpgradeMessageProcessor();
            void CreateQueryHelper();
            void CreateStateCleanupWorkManager();
            void CreateReconfigurationMessageRetryWorkManager();
            void CreateReplicaCloseMessageRetryWorkManager();
            void CreateReplicaOpenWorkManager();
            void CreateUpdateServiceDescriptionMessageRetryWorkManager();
            void CreatePerformanceCounters();
            void CreateNodeUpAckProcessor();
            void CreateHostingAdapter();
            void CreateFMMessageRetryComponent();
            void CreateResourceMonitorComponent();
#pragma endregion

            // Member variables
            State state_;
            MultipleReplicaCloseAsyncOperationWPtr closeReplicasAsyncOp_;

            // Dependent components
            IReliabilitySubsystemWrapperSPtr reliability_;
            IFederationWrapper * federationWrapper_;
            IRaToRapTransportUPtr rapTransport_;
            Communication::FMTransport fmTransport_;
            Hosting2::IHostingSubsystem & hostingSubsystem_;
            Hosting::HostingAdapterUPtr hostingAdapter_;

            // Lock for synchronizing access to state
            mutable Common::RwLock lock_;

            GenerationStateManager generationStateManager_;
            NodeUpAckProcessorUPtr nodeUpAckProcessor_;

            // Application upgrades
            std::map<ServiceModel::ServicePackageIdentifier, ServiceModel::ServicePackageVersionInstance> upgradeVersions_;
            bool upgradeTableClosed_;
            mutable Common::RwLock appUpgradeslock_;

            Infrastructure::EntitySetCollection entitySets_;

#pragma region Timers
           

#pragma endregion

#pragma region Multiple FT BGMR
            Infrastructure::MultipleEntityBackgroundWorkManagerUPtr replicaOpenWorkManager_;
            Infrastructure::MultipleEntityBackgroundWorkManagerUPtr reconfigurationMessageRetryWorkManager_;
            Infrastructure::MultipleEntityBackgroundWorkManagerUPtr stateCleanupWorkManager_;
            Infrastructure::MultipleEntityBackgroundWorkManagerUPtr replicaCloseMessageRetryWorkManager_;
            Infrastructure::MultipleEntityBackgroundWorkManagerUPtr updateServiceDescriptionMessageRetryWorkManager_;
#pragma endregion

            Diagnostics::IEventWriterUPtr eventWriter_;

            Diagnostics::PerformanceCountersSPtr perfCounters_;

            Infrastructure::JobQueueManagerUPtr jobQueueManager_;

            Upgrade::UpgradeMessageProcessorUPtr upgradeMessageProcessor_;

            Federation::NodeInstance nodeInstance_;
            std::wstring nodeInstanceIdStr_;

            Health::HealthSubsystemWrapperUPtr health_;

            Query::QueryHelperUPtr queryHelper_;

            Node::NodeStateUPtr nodeState_;
            Node::NodeDeactivationMessageProcessorUPtr nodeDeactivationMessageProcessor_;
            
            Node::ServiceTypeUpdateProcessorUPtr serviceTypeUpdateProcessor_;

            FailoverConfig & failoverConfig_;
            Infrastructure::IThreadpoolUPtr threadpool_;
            Infrastructure::IClockSPtr clock_;
            Common::ProcessTerminationServiceSPtr processTerminationService_;

            std::unique_ptr<MessageRetry::FMMessageRetryComponent> fmMessageRetryComponent_;
            std::unique_ptr<MessageRetry::FMMessageRetryComponent> fmmMessageRetryComponent_;

            // LFUM
            Storage::Api::IKeyValueStoreSPtr raStoreSPtr_;
            Infrastructure::LocalFailoverUnitMapUPtr lfum_;
            std::shared_ptr<Infrastructure::EntityPreCommitNotificationSink<FailoverUnit>> ftPreCommitNotificationSink_;
            Storage::KeyValueStoreFactory::Parameters lfumStoreParameters_;
            
            ThreadpoolFactory threadpoolFactory_;

            Communication::MessageMetadataTable messageMetadataTable_;
            MessageHandler messageHandler_;

            ResourceMonitor::ResourceComponentUPtr resourceMonitorComponent_;
        };
    }
}
