// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent 
    {
        class ReconfigurationAgentProxy;

        struct ActionListInfo
        {
            ProxyActionsListTypes::Enum Name;
            bool ImpactsServiceAvailability;

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        };

        // Represents a FailoverUnit in RA inside the service host
        class FailoverUnitProxy
            : public std::enable_shared_from_this<FailoverUnitProxy>
            , public ReplicationComponent::IReplicatorHealthClient

        {
            DENY_COPY(FailoverUnitProxy);

        public:
            typedef std::map<Federation::NodeId, Reliability::ReplicaDescription> ConfigurationReplicaStore;
            class ConfigurationUtility;

            FailoverUnitProxy(
                Reliability::FailoverUnitDescription const & failoverUnitDescription,
                ReconfigurationAgentProxy & reconfigurationAgentProxy,
                std::wstring const & runtimeId,
                Common::AsyncOperationSPtr const & root) 
                :   isOpenForBusiness_(true),
                    isMarkedForClose_(false),
                    isMarkedForAbort_(false),
                    currentlyExecutingActionsLists_(),
                    lock_(),
                    remoteReplicas_(),
                    configurationReplicas_(),
                    messageStage_(ProxyMessageStage::None),
                    replicaState_(FailoverUnitProxyStates::Closed),
                    replicatorState_(ReplicatorStates::Closed),
                    failoverUnitDescription_(failoverUnitDescription),
                    runtimeId_(runtimeId),
                    replicaDescription_(),
                    serviceDescription_(),
                    currentServiceRole_(ReplicaRole::Unknown),
                    currentReplicatorRole_(ReplicaRole::Unknown),
                    currentReplicaState_(ReplicaStates::InCreate),
                    lastUpdateEpochPrimaryEpochValue_(Epoch::InvalidEpoch()),
                    replicaOpenMode_(ReplicaOpenMode::Invalid),
                    statelessService_(),
                    statefulService_(),
                    statefulServicePartition_(),
                    statelessServicePartition_(),
                    replicator_(),
                    configurationStage_(ProxyConfigurationStage::Current),
                    catchupResult_(CatchupResult::NotStarted),
                    isCatchupCancel_(false),
                    reconfigurationAgentProxy_(reconfigurationAgentProxy),
                    root_(root),
                    isServiceAvailabilityImpacted_(false),
                    isDeleted_(false)
            {}

            virtual ~FailoverUnitProxy();

            __declspec(property(get = get_IsDeleted)) bool IsDeleted;
            bool get_IsDeleted() const {
                return isDeleted_;
            }

            __declspec(property(get=get_ReplicatorFactoryObj)) Reliability::ReplicationComponent::IReplicatorFactory & ReplicatorFactory;
            Reliability::ReplicationComponent::IReplicatorFactory & get_ReplicatorFactoryObj() const { 
                return reconfigurationAgentProxy_.ReplicatorFactory; 
            }

            __declspec(property(get=get_TransactionalReplicatorFactoryObj)) TxnReplicator::ITransactionalReplicatorFactory & TransactionalReplicatorFactory;
            TxnReplicator::ITransactionalReplicatorFactory & get_TransactionalReplicatorFactoryObj() const { 
                return reconfigurationAgentProxy_.TransactionalReplicatorFactory; 
            }
            
            __declspec(property(get=get_ApplicationHostObj)) Hosting2::IApplicationHost & ApplicationHostObj;
            Hosting2::IApplicationHost & get_ApplicationHostObj() const { 
                return reconfigurationAgentProxy_.ApplicationHostObj; 
            }

            __declspec(property(get=get_ReconfigurationAgentProxyId)) ReconfigurationAgentProxyId const & RAPId;
            ReconfigurationAgentProxyId const & get_ReconfigurationAgentProxyId() const { 
                return reconfigurationAgentProxy_.Id; 
            }

            // Property accessors
            __declspec(property(get=get_IsOpened)) bool const IsOpened;
             bool const get_IsOpened() const {
                return state_ == FailoverUnitProxyStates::Opened;
            }
            
             __declspec(property(get=get_IsOpening)) bool const IsOpening;
             bool const get_IsOpening() const {
                return state_ == FailoverUnitProxyStates::Opening;
            }

            __declspec(property(get=get_IsClosed)) bool const IsClosed;
            bool const get_IsClosed() const {
                return state_ == FailoverUnitProxyStates::Closed;
            }

             __declspec(property(get=get_IsClosing)) bool const IsClosing;
            bool const get_IsClosing() const {
                return state_ == FailoverUnitProxyStates::Closing;
            }
            __declspec(property(get=get_IsValid)) bool const IsValid;
             bool const get_IsValid() const {
                return failoverUnitDescription_.CurrentConfigurationEpoch != Epoch::InvalidEpoch(); 
            }

            __declspec(property(get=get_IsServiceCreated)) bool const IsServiceCreated;
            bool const get_IsServiceCreated() {
                return statelessService_ || statefulService_;
            }

            __declspec(property(get=get_IsReplicatorCreated)) bool const IsReplicatorCreated;
            bool const get_IsReplicatorCreated() const {
                return (replicator_ != nullptr);
            }

            __declspec(property(get = get_DoesReplicatorSupportQueries)) bool const DoesReplicatorSupportQueries;
            bool const get_DoesReplicatorSupportQueries() const {
                return replicator_ && replicator_->DoesReplicatorSupportQueries;
            }

            __declspec(property(get=get_CurrentConfigurationEpoch)) Reliability::Epoch const CurrentConfigurationEpoch;
            Reliability::Epoch const get_CurrentConfigurationEpoch() const { 
                return failoverUnitDescription_.CurrentConfigurationEpoch; 
            }

            __declspec(property(get=get_IsReconfiguring)) bool IsReconfiguring;
            bool get_IsReconfiguring() const { 
                return failoverUnitDescription_.PreviousConfigurationEpoch != Epoch::InvalidEpoch(); 
            }

            __declspec(property(get=get_IsSwapPrimary)) bool IsSwapPrimary;
            bool get_IsSwapPrimary() const { 
                return IsReconfiguring && CurrentConfigurationRole == ReplicaRole::Secondary && PreviousConfigurationRole == ReplicaRole::Primary; 
            }

            __declspec(property(get=get_FailoverUnitId)) Reliability::FailoverUnitId const FailoverUnitId;
            Reliability::FailoverUnitId const get_FailoverUnitId() const { 
                return failoverUnitDescription_.FailoverUnitId; 
            }

            __declspec(property(get=get_CurrentConfigurationRole)) ReplicaRole::Enum const CurrentConfigurationRole;
            ReplicaRole::Enum const get_CurrentConfigurationRole() const {
                return replicaDescription_.CurrentConfigurationRole;
            }

            __declspec(property(get=get_PreviousConfigurationRole)) ReplicaRole::Enum PreviousConfigurationRole;
            ReplicaRole::Enum get_PreviousConfigurationRole() const { 
                return replicaDescription_.PreviousConfigurationRole; }

            __declspec(property(get=get_CurrentReplicatorRole)) ReplicaRole::Enum const CurrentReplicatorRole;
            ReplicaRole::Enum const get_CurrentReplicatorRole() const {
                return currentReplicatorRole_;
            }

            __declspec(property(get=get_CurrentServiceRole)) ReplicaRole::Enum const CurrentServiceRole;
            ReplicaRole::Enum const get_CurrentServiceRole() const {
                return currentServiceRole_;
            }

            __declspec(property(get = get_AreServiceAndReplicatorRoleCurrent)) bool AreServiceAndReplicatorRoleCurrent;
            bool get_AreServiceAndReplicatorRoleCurrent() const {
                return currentServiceRole_ == replicaDescription_.CurrentConfigurationRole && 
                    currentReplicatorRole_ == replicaDescription_.CurrentConfigurationRole;
            }

            __declspec(property(get = get_IsStatefulService)) bool IsStatefulService;
            bool get_IsStatefulService() const {
                return serviceDescription_.IsStateful;
            }

            __declspec(property(get=get_State, put=set_State)) FailoverUnitProxyStates::Enum State;
            FailoverUnitProxyStates::Enum get_State() const {
                return state_;
            }
            void set_State(FailoverUnitProxyStates::Enum const & state) {
                state_ = state;
            }

            __declspec(property(get=get_IsRunningActions)) bool IsRunningActions;
            bool get_IsRunningActions() const {
                return currentlyExecutingActionsLists_.size() > 0;
            }

            __declspec(property(get=get_ConfigurationStage, put=set_ConfigurationStage)) ProxyConfigurationStage::Enum ConfigurationStage;
            ProxyConfigurationStage::Enum get_ConfigurationStage() const {
                return configurationStage_;
            }
            void set_ConfigurationStage(ProxyConfigurationStage::Enum const & configurationStage) {
                configurationStage_ = configurationStage;
            }

            __declspec(property(get=get_IsCatchupCancel)) bool IsCatchupCancel;
            bool get_IsCatchupCancel() const {
                return isCatchupCancel_;
            }

            __declspec(property(get=get_IsCatchupPending)) bool IsCatchupPending;
            bool get_IsCatchupPending() const;

            __declspec(property(get = get_CatchupResult)) CatchupResult::Enum CatchupResult;
            CatchupResult::Enum get_CatchupResult() const { 
                return catchupResult_; 
            }

            __declspec(property(get=get_ReplicaDescription, put=set_ReplicaDescription)) ReplicaDescription ReplicaDescription;
            Reliability::ReplicaDescription const& get_ReplicaDescription() const{
                return replicaDescription_;
            }
            void set_ReplicaDescription(Reliability::ReplicaDescription const & replicaDescription) {
                replicaDescription_ = replicaDescription;
            }

            __declspec(property(get=get_ServiceDescription, put=set_ServiceDescription)) ServiceDescription ServiceDescription;
            Reliability::ServiceDescription const & get_ServiceDescription() const {
                return serviceDescription_;
            }
            void set_ServiceDescription(Reliability::ServiceDescription const & serviceDescription) {
                serviceDescription_ = serviceDescription;
            }

            __declspec(property(get=get_FailoverUnitDescription, put=set_FailoverUnitDescription)) FailoverUnitDescription FailoverUnitDescription;
            Reliability::FailoverUnitDescription const & get_FailoverUnitDescription() const {
                return failoverUnitDescription_;
            }
            Reliability::FailoverUnitDescription & get_FailoverUnitDescription() {
                return failoverUnitDescription_;
            }
            void set_FailoverUnitDescription(Reliability::FailoverUnitDescription const & failoverUnitDescription) {
                failoverUnitDescription_ = failoverUnitDescription;
            }

            __declspec(property(get=get_CurrentReplicaState, put=set_CurrentReplicaState)) ReplicaStates::Enum CurrentReplicaState;
            ReplicaStates::Enum const get_CurrentReplicaState() {
                return currentReplicaState_;
            }
            void set_CurrentReplicaState(ReplicaStates::Enum const & currentReplicaState) {
                currentReplicaState_ = currentReplicaState;
            }

            __declspec(property(get=get_ReplicaOpenMode, put=set_ReplicaOpenMode)) Reliability::ReplicaOpenMode::Enum ReplicaOpenMode;
            Reliability::ReplicaOpenMode::Enum & get_ReplicaOpenMode() {
                return replicaOpenMode_;
            }
            void set_ReplicaOpenMode(Reliability::ReplicaOpenMode::Enum const & replicaOpenMode) {
                replicaOpenMode_ = replicaOpenMode;
            }

            __declspec(property(get=get_AreOperationManagersOpenForBusiness)) bool AreOperationManagersOpenForBusiness;
            bool get_AreOperationManagersOpenForBusiness() const
            {
                return isOpenForBusiness_;
            }

            __declspec(property(get = get_IsPreWriteStatusCatchupEnabled)) bool IsPreWriteStatusCatchupEnabled;
            bool get_IsPreWriteStatusCatchupEnabled() const
            {
                return FailoverConfig::GetConfig().IsPreWriteStatusRevokeCatchupEnabled ;
            }

            __declspec(property(get = get_RuntimeId)) std::wstring const & RuntimeId;
            std::wstring const & get_RuntimeId() const
            {
                return runtimeId_;
            }
            
            bool TryAddToCurrentlyExecutingActionsLists(
                ProxyActionsListTypes::Enum const & plannedActions, 
                bool impactServiceAvailability,
                FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext);

            bool TryAddToCurrentlyExecutingActionsLists(
                ProxyActionsListTypes::Enum const & plannedActions, 
                bool impactServiceAvailability,
                FailoverUnitProxyContext<ProxyRequestMessageBody> & msgContext,
                __out bool & cancelNeeded);

            ProxyOutgoingMessageUPtr ComposeReadWriteStatusRevokedNotification(Common::AcquireExclusiveLock & lock);
            void SendReadWriteStatusRevokedNotification(ProxyOutgoingMessageUPtr &&);

            void DoneCancelReplicatorCatchupReplicaSet();
            void Reuse(Reliability::FailoverUnitDescription const & failoverUnitDescription, std::wstring const & runtimeId);
            bool TryMarkForAbort();

            // Delete implies that the FUP shared_ptr has been removed from the LFUPM
            bool TryDelete();

            void AcquireLock();
            void ReleaseLock();            
            
            Common::AsyncOperationSPtr BeginOpenInstance(
                __in Hosting2::IApplicationHost & applicationHost,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            ProxyErrorCode EndOpenInstance(Common::AsyncOperationSPtr const & asyncOperation, __out std::wstring & serviceLocation);
            
            Common::AsyncOperationSPtr BeginCloseInstance(
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            ProxyErrorCode EndCloseInstance(Common::AsyncOperationSPtr const & asyncOperation);

            Common::AsyncOperationSPtr BeginOpenReplica(
                __in Hosting2::IApplicationHost & applicationHost,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            ProxyErrorCode EndOpenReplica(Common::AsyncOperationSPtr const & asyncOperation);
            
            Common::AsyncOperationSPtr BeginOpenReplicator(
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            ProxyErrorCode EndOpenReplicator(Common::AsyncOperationSPtr const & asyncOperation, __out std::wstring & replicationEndpoint);

            Common::AsyncOperationSPtr BeginChangeReplicaRole(
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            ProxyErrorCode EndChangeReplicaRole(Common::AsyncOperationSPtr const & asyncOperation, __out std::wstring & serviceLocation);

            Common::AsyncOperationSPtr BeginChangeReplicatorRole(
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            ProxyErrorCode EndChangeReplicatorRole(Common::AsyncOperationSPtr const & asyncOperation);

            Common::AsyncOperationSPtr BeginCloseReplica(
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            ProxyErrorCode EndCloseReplica(Common::AsyncOperationSPtr const & asyncOperation);

            Common::AsyncOperationSPtr BeginCloseReplicator(
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            ProxyErrorCode EndCloseReplicator(Common::AsyncOperationSPtr const & asyncOperation);

            void AbortPartition();
            void AbortReplicator();
            void AbortReplica();
            void AbortInstance();
            void Abort(bool keepFUPOpen);

            void CancelOperations();

            void Cleanup();
            void DoneExecutingActionsList(ProxyActionsListTypes::Enum const & completedActions);
            
            void PublishEndpoint(Transport::MessageUPtr && outMsg);
            void ProcessReplicaEndpointUpdatedReply(Reliability::ReplicaDescription const & msgReplica);
            void ProcessReadWriteStatusRevokedNotificationReply(Reliability::ReplicaDescription const & msgReplica);

            void UpdateServiceDescription(Reliability::ServiceDescription const & newServiceDescription);

            ProxyErrorCode UpdateConfiguration(
                ProxyRequestMessageBody const & msgBody,
                UpdateConfigurationReason::Enum reason);

            void ProcessUpdateConfigurationMessage(
                ProxyRequestMessageBody const & msgBody);
            void OnReconfigurationStarting();
            void OnReconfigurationEnding();
            void OpenPartition();
            void AssertCatchupNotStartedCallerHoldsLock() const;

            Common::AsyncOperationSPtr BeginReplicatorBuildIdleReplica(
                Reliability::ReplicaDescription const & idleReplicaDescription,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            ProxyErrorCode EndReplicatorBuildIdleReplica(Common::AsyncOperationSPtr const & asyncOperation);

            Common::AsyncOperationSPtr BeginReplicatorOnDataLoss(
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);            
            ProxyErrorCode EndReplicatorOnDataLoss(Common::AsyncOperationSPtr const & asyncOperation, int64 & lastLSN);

            ProxyErrorCode ReplicatorRemoveIdleReplica(Reliability::ReplicaDescription const & idleReplicaDescription);

            Common::AsyncOperationSPtr BeginReplicatorUpdateEpoch(
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            ProxyErrorCode EndReplicatorUpdateEpoch(Common::AsyncOperationSPtr const & asyncOperation);

            ProxyErrorCode ReplicatorGetStatus(__out FABRIC_SEQUENCE_NUMBER & firstLsn, __out FABRIC_SEQUENCE_NUMBER & lastLsn);
        
            ProxyErrorCode ReplicatorGetQuery(__out ServiceModel::ReplicatorStatusQueryResultSPtr & result);
            Common::ErrorCode ReplicaGetQuery(__out ServiceModel::ReplicaStatusQueryResultSPtr & result) const;
            
            Common::AsyncOperationSPtr BeginReplicatorCatchupReplicaSet(
                CatchupType::Enum type,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            ProxyErrorCode EndReplicatorCatchupReplicaSet(Common::AsyncOperationSPtr const & asyncOperation);

            ProxyErrorCode CancelReplicatorCatchupReplicaSet();
            
            Common::AsyncOperationSPtr BeginMarkForCloseAndDrainOperations(
                bool isAbort,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            Common::ErrorCode EndMarkForCloseAndDrainOperations(Common::AsyncOperationSPtr const & asyncOperation);

            // This method does not honor the locks
            // It is the callers reponsibility to ensure that there are no concurrent operations against the FUP
            Common::AsyncOperationSPtr BeginClose(
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndClose(Common::AsyncOperationSPtr const & asyncOperation);

            void CleanupEventHandler();
            void RegisterFailoverUnitProxyForCleanupEvent();
            void UnregisterFailoverUnitProxyForCleanupEvent();

            Common::ErrorCode ReportLoad(std::vector<LoadBalancingComponent::LoadMetric> &&);            
            Common::ErrorCode ReportReplicatorHealth(
                Common::SystemHealthReportCode::Enum reportCode,
                std::wstring const & dynamicProperty,
                std::wstring const & extraDescription,
                FABRIC_SEQUENCE_NUMBER sequenceNumber,
                Common::TimeSpan const & timeToLive);
            Common::ErrorCode ReportFault(FaultType::Enum);
            Common::ErrorCode ReportHealth(
                ServiceModel::HealthReport && healthReport,
                ServiceModel::HealthReportSendOptionsUPtr && sendOptions);

            Common::ApiMonitoring::ApiCallDescriptionSPtr CreateApiCallDescription(
                Common::ApiMonitoring::ApiNameDescription && nameDescription,
                Reliability::ReplicaDescription const & replicaDescription,
                bool isHealthReportEnabled,
                bool traceServiceType);

            void StartOperationMonitoring(
                Common::ApiMonitoring::ApiCallDescriptionSPtr const & description);

            void StopOperationMonitoring(
                Common::ApiMonitoring::ApiCallDescriptionSPtr const & description,
                Common::ErrorCode const & error);

            void TraceBeforeOperation(
                Common::AcquireExclusiveLock &,
                Common::ApiMonitoring::InterfaceName::Enum,
                Common::ApiMonitoring::ApiName::Enum api) const;

            void TraceBeforeOperation(
                Common::AcquireExclusiveLock &,
                Common::ApiMonitoring::InterfaceName::Enum ,
                Common::ApiMonitoring::ApiName::Enum api,
                Common::TraceCorrelatedEventBase const &) const;

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
            std::wstring ToString() const;

            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
            void FillEventData(Common::TraceEventContext & context) const;

            Common::ErrorCode IsQueryAllowed(bool & isReplicatorQueryRequired) const;

            ServiceModel::DeployedServiceReplicaDetailQueryResult GetQueryResult() const;

            bool IsConfigurationMessageBodyStaleCallerHoldsLock(
                std::vector<Reliability::ReplicaDescription> const & replicaDescriptions) const;

            bool CheckConfigurationMessageBodyForUpdatesCallerHoldsLock(
                std::vector<Reliability::ReplicaDescription> const & replicaDescriptions,
                bool shouldApply);

            void UpdateReadWriteStatus();

            ProxyErrorCode FinalizeDemoteToSecondary();

        private:

            // FUP async operations
            class CloseAsyncOperation;
            class WaitForDrainAsyncOperation;

            class UserApiInvoker;
            class UserApiInvokerAsyncOperationBase;

            // Service async operations
            class OpenInstanceAsyncOperation;
            class CloseInstanceAsyncOperation;

            class OpenReplicaAsyncOperation;
            class ChangeReplicaRoleAsyncOperation;
            class CloseReplicaAsyncOperation;

            // Replicator async operations
            class OpenReplicatorAsyncOperation;
            class ChangeReplicatorRoleAsyncOperation;
            class CloseReplicatorAsyncOperation;
            class ReplicatorBuildIdleReplicaAsyncOperation;
            class ReplicatorCatchupReplicaSetAsyncOperation;
            class ReplicatorOnDataLossAsyncOperation;
            class ReplicatorUpdateEpochAsyncOperation;

            class ReadWriteStatusCalculator;
            static const Common::Global<ReadWriteStatusCalculator> ReadWriteStatusCalculatorObj;

            // This class is used to access fup under lock
            friend class LockedFailoverUnitProxyPtr;

            // ProxyOutgoingMessage queries fup for message resend
            friend class ProxyOutgoingMessage;    
            
            bool ShouldResendMessage(Transport::MessageUPtr const& message);

            void GetReplicaSetConfiguration(
                ::FABRIC_REPLICA_SET_CONFIGURATION & replicaSetConfiguration,
                std::vector<::FABRIC_REPLICA_INFORMATION> & replicas,
                int setCount,
                int setNonDroppedCount);
            
            void UpdateReadAndWriteStatus(Common::AcquireExclusiveLock &);
            bool HasMinReplicaSetAndWriteQuorum(Common::AcquireExclusiveLock &, bool includePCCheck) const;

            void CancelBuildIdleReplicaOperations();

            void OpenForBusiness();
            void CloseForBusiness(bool isAbort);

            bool IsStatefulServiceFailoverUnitProxy() const { return serviceDescription_.IsStateful && statefulService_; }
            bool IsStatelessServiceFailoverUnitProxy() const { return !serviceDescription_.IsStateful && statelessService_; }

            void AddActionList(ProxyActionsListTypes::Enum const & plannedActions, bool impactsServiceAvailability);

            void TransitionReplicatorToClosed(Common::AcquireExclusiveLock &);
            void TransitionServiceToClosed(Common::AcquireExclusiveLock &);
           
            // Class member variables
            std::vector<ActionListInfo> currentlyExecutingActionsLists_;
            bool isServiceAvailabilityImpacted_;
            mutable Common::ExclusiveLock lock_;

            Reliability::FailoverUnitDescription failoverUnitDescription_;
            Reliability::ReplicaDescription replicaDescription_;
            Reliability::ServiceDescription serviceDescription_;
            std::wstring runtimeId_;

            ConfigurationReplicaStore configurationReplicas_;
            std::map<Federation::NodeId, Reliability::ReconfigurationAgentComponent::ReplicaProxy> remoteReplicas_;

            FailoverUnitProxyLifeCycleState state_;
            ReplicatorStates::Enum replicatorState_;
            FailoverUnitProxyStates::Enum replicaState_;
            ProxyMessageStage::Enum messageStage_;

            Reliability::ReplicaRole::Enum currentServiceRole_;
            Reliability::ReplicaRole::Enum currentReplicatorRole_;
            ReplicaStates::Enum currentReplicaState_;
            Epoch lastUpdateEpochPrimaryEpochValue_;
            Reliability::ReplicaOpenMode::Enum replicaOpenMode_;
            ProxyConfigurationStage::Enum configurationStage_;

            CatchupResult::Enum catchupResult_;
            bool isCatchupCancel_; // This is used to keep track catch up cancel requested state
            bool isOpenForBusiness_;
            bool isMarkedForClose_;
            bool isMarkedForAbort_;

            Common::AsyncOperationSPtr drainAsyncOperation_;

            ServiceOperationManagerUPtr serviceOperationManager_;
            ReplicatorOperationManagerUPtr replicatorOperationManager_;

            //FailoverUnitProxy holds on to the root which keeps it alive until it is destructed
            //which in turn keeps the ReconfigurationAgentProxy alive
            ReconfigurationAgentProxy & reconfigurationAgentProxy_;
            Common::AsyncOperationSPtr root_;

            ComProxyStatelessServiceUPtr statelessService_;
            ComProxyStatefulServiceUPtr statefulService_;
            mutable Common::ComPointer<ComStatefulServicePartition> statefulServicePartition_;
            Common::ComPointer<ComStatelessServicePartition> statelessServicePartition_;
            ComProxyReplicatorUPtr replicator_;

            bool isDeleted_;
            ReadWriteStatusState readWriteStatusState_;

            // Used to cache the loads reported by the FT for query
            class ReportedLoadStore
            {
            public:

                void AddLoad(std::vector<LoadBalancingComponent::LoadMetric> const & loads);

                void OnFTChangeRole();
                void OnFTOpen();

                std::vector<ServiceModel::LoadMetricReport> GetForQuery() const;

            private:
                void Clear();

                class Data
                {
                public:
                    Data();
                    Data(uint value, Common::DateTime timestamp);

                    ServiceModel::LoadMetricReport ToLoadValue(std::wstring const & metricName) const;
                private:
                    Common::DateTime timeStamp_;
                    uint value_;
                };

                std::map<std::wstring, Data> loads_;
            };

            ReportedLoadStore reportedLoadStore_;

        };
    }
}
