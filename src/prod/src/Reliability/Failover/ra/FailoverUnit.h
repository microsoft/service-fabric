// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        // Represents a FailoverUnit in RA
        class FailoverUnit : public Serialization::FabricSerializable, public Infrastructure::IEntityStateBase
        {
            DENY_COPY_ASSIGNMENT(FailoverUnit);

            struct ReplicaSetCountsAtGetLsn
            {
                ReplicaSetCountsAtGetLsn() :
                    ReplicaCount(0),
                    CompletedCount(0),
                    DownWaitingCount(0),
                    UpWaitingCount(0)
                {
                }

                __declspec(property(get = get_IsBelowReadQuorum)) bool IsBelowReadQuorum;
                bool get_IsBelowReadQuorum() const { return (CompletedCount < (ReplicaCount + 1) / 2); }

                __declspec(property(get = get_WaitingCount)) int WaitingCount;
                int get_WaitingCount() const { return UpWaitingCount + DownWaitingCount; }

                // The count of replicas in the configuration
                int ReplicaCount;

                // The number of replicas that have replied to lsn
                int CompletedCount;

                // The number of down replicas for which lsn is not received
                int DownWaitingCount;

                // The number of up replicas for which lsn is not received
                int UpWaitingCount;
            };

        public:

            class TraceWriter
            {
                DENY_COPY_ASSIGNMENT(TraceWriter);

            public:
                TraceWriter(FailoverUnit const &);
                TraceWriter(FailoverUnit const &, TraceWriter const &);

                static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
                void FillEventData(Common::TraceEventContext & context) const;
                void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

                FailoverUnitLifeCycleTraceState::Enum GetLifeCycleTraceState() const;                

                void Initialize();

            private:
                FailoverUnit const & ft_;
            };

            // Empty constructor, used for serialization to store
            FailoverUnit();

            FailoverUnit(
                FailoverConfig const & config,
                Reliability::FailoverUnitId const & ftId, 
                Reliability::ConsistencyUnitDescription const & cuid,
                Reliability::ServiceDescription const & serviceDescription);

            explicit FailoverUnit(FailoverUnit const & other);

            __declspec(property(get=get_FailoverUnitDescription)) Reliability::FailoverUnitDescription const & FailoverUnitDescription;
            Reliability::FailoverUnitDescription const & get_FailoverUnitDescription() const { return failoverUnitDesc_; }

            __declspec(property(get=get_FailoverUnitId)) Reliability::FailoverUnitId const & FailoverUnitId;
            Reliability::FailoverUnitId const & get_FailoverUnitId() const { return failoverUnitDesc_.FailoverUnitId; }

            __declspec(property(get = get_Owner)) Reliability::FailoverManagerId const & Owner;
            Reliability::FailoverManagerId const & get_Owner() const { return Reliability::FailoverManagerId::Get(FailoverUnitId); }

            __declspec(property(get = get_TransportUtility)) FailoverUnitTransportUtility TransportUtility;
            FailoverUnitTransportUtility get_TransportUtility() const { return FailoverUnitTransportUtility(Owner); }

            __declspec(property(get=get_CurrentConfigurationEpoch, put=set_CurrentConfigurationEpoch)) Reliability::Epoch CurrentConfigurationEpoch;
            Reliability::Epoch const & get_CurrentConfigurationEpoch() const { return failoverUnitDesc_.CurrentConfigurationEpoch; }
            void set_CurrentConfigurationEpoch(Reliability::Epoch const & value) { failoverUnitDesc_.CurrentConfigurationEpoch = value; }

            __declspec(property(get=get_PreviousConfigurationEpoch, put=set_PreviousConfigurationEpoch)) Reliability::Epoch PreviousConfigurationEpoch;
            Reliability::Epoch const & get_PreviousConfigurationEpoch() const { return failoverUnitDesc_.PreviousConfigurationEpoch; }
            void set_PreviousConfigurationEpoch(Reliability::Epoch const & value) 
            { 
                failoverUnitDesc_.PreviousConfigurationEpoch = value; 
                UpdateLastStableEpochForBackwardCompatibility();
            }

            __declspec(property(get=get_IntermediateConfigurationEpoch, put=set_IntermediateConfigurationEpoch)) Reliability::Epoch IntermediateConfigurationEpoch;
            Reliability::Epoch const & get_IntermediateConfigurationEpoch() const { return icEpoch_; }
            void set_IntermediateConfigurationEpoch(Reliability::Epoch const & value) { icEpoch_ = value; }

            __declspec(property(get = get_ReplicaDeactivationInfo)) Reliability::ReplicaDeactivationInfo const & DeactivationInfo;
            Reliability::ReplicaDeactivationInfo const & get_ReplicaDeactivationInfo() const { return deactivationInfo_; } 

            __declspec (property(get=get_TargetReplicaSetSize, put=set_TargetReplicaSetSize)) int const TargetReplicaSetSize;
            int const get_TargetReplicaSetSize() const { return serviceDesc_.TargetReplicaSetSize; }

            __declspec (property(get=get_MinReplicaSetSize, put=set_MinReplicaSetSize)) int const MinReplicaSetSize;
            int const get_MinReplicaSetSize() const { return serviceDesc_.MinReplicaSetSize; }

            __declspec (property(get=get_State, put=set_State)) FailoverUnitStates::Enum const State;
            FailoverUnitStates::Enum const get_State() const { return state_; }
            void Test_SetState(FailoverUnitStates::Enum value) { state_ = value; }

            __declspec(property(get=get_IsOpen)) bool IsOpen;
            bool get_IsOpen() const { return state_ == FailoverUnitStates::Open; }

            __declspec(property(get=get_IsClosed)) bool IsClosed;
            bool get_IsClosed() const { return state_ == FailoverUnitStates::Closed; }

            __declspec (property(get=get_HasPersistedState)) bool HasPersistedState;
            bool get_HasPersistedState() const { return serviceDesc_.HasPersistedState; }

            __declspec (property(get=get_ReconfigurationStage, put=set_ReconfigurationStage)) FailoverUnitReconfigurationStage::Enum const ReconfigurationStage;
            FailoverUnitReconfigurationStage::Enum const get_ReconfigurationStage() const { return reconfigState_.ReconfigurationStage; }
            ReconfigurationState & Test_GetReconfigurationState() { return reconfigState_; }
            void Test_SetReconfigurationState(ReconfigurationState const & state) { reconfigState_ = state; }

            __declspec (property(get = get_ReconfigurationType)) ReconfigurationType::Enum const ReconfigurationType;
            ReconfigurationType::Enum const get_ReconfigurationType() const { return reconfigState_.ReconfigType; }

            __declspec (property(get = get_ReconfigurationStartTime)) Common::StopwatchTime const ReconfigurationStartTime;
            Common::StopwatchTime const get_ReconfigurationStartTime() const { return reconfigState_.StartTime; }

            __declspec (property(get=get_IsExisting)) int const IsExisting;
            int const get_IsExisting() const { return failoverUnitDesc_.CurrentConfigurationEpoch != Epoch::InvalidEpoch(); }

            __declspec (property(get=get_LocalReplica)) Replica & LocalReplica;
            Replica const & get_LocalReplica() const { return replicaStore_.LocalReplica; }
            Replica & get_LocalReplica() { return replicaStore_.LocalReplica; }

            __declspec (property(get=get_LocalReplicaRole)) ReplicaRole::Enum const LocalReplicaRole;
            ReplicaRole::Enum const get_LocalReplicaRole() const { return IsClosed ? ReplicaRole::None : LocalReplica.CurrentConfigurationRole; }

            __declspec (property(get=get_LocalReplicaId, put=set_LocalReplicaId)) int64 LocalReplicaId;
            int64 get_LocalReplicaId() const { return localReplicaId_; }
            void set_LocalReplicaId(int64 value) { localReplicaId_ = value; }
            void Test_SetLocalReplicaId(int64 value) { localReplicaId_ = value; }

            __declspec (property(get=get_LocalReplicaInstanceId, put=set_LocalReplicaInstanceId)) int64 LocalReplicaInstanceId;
            int64 get_LocalReplicaInstanceId() const { return localReplicaInstanceId_; }
            void set_LocalReplicaInstanceId(int64 value) { localReplicaInstanceId_ = value; }
            void Test_SetLocalReplicaInstanceId(int64 value) { localReplicaInstanceId_ = value; }
            
            __declspec (property(get=get_ServiceDescription)) Reliability::ServiceDescription const & ServiceDescription;
            Reliability::ServiceDescription const & get_ServiceDescription() const { return serviceDesc_; }
            void Test_SetServiceDescription(const Reliability::ServiceDescription & value) { serviceDesc_ = value; }
            Reliability::ServiceDescription & Test_GetServiceDescription() { return serviceDesc_; }

            __declspec (property(get=get_Replicas)) std::vector<Replica> const & Replicas;
            std::vector<Replica> const & get_Replicas() const { return replicas_; }

            __declspec(property(get=get_SenderNode, put=set_SenderNode)) Federation::NodeInstance SenderNode;
            Federation::NodeInstance get_SenderNode() const { return senderNode_; }
            void set_SenderNode(Federation::NodeInstance value) { senderNode_ = value; }

            __declspec(property(get=get_IsUpdateReplicatorConfiguration, put=set_IsUpdateReplicatorConfiguration)) bool IsUpdateReplicatorConfiguration;
            bool get_IsUpdateReplicatorConfiguration() const { return updateReplicatorConfiguration_; }
            void set_IsUpdateReplicatorConfiguration(bool value) { updateReplicatorConfiguration_ = value; }

            __declspec (property(get=get_AppHostId)) std::wstring const & AppHostId;
            std::wstring const & get_AppHostId() const { return serviceTypeRegistration_.AppHostId; }

            __declspec (property(get=get_RuntimeId)) std::wstring const & RuntimeId;
            std::wstring const & get_RuntimeId() const { return serviceTypeRegistration_.RuntimeId; }

            __declspec (property(get=get_IsRuntimeActive)) bool IsRuntimeActive;
            bool get_IsRuntimeActive() const { return serviceTypeRegistration_.IsRuntimeActive; }

            __declspec (property(get=get_ServiceTypeRegistration)) Hosting2::ServiceTypeRegistrationSPtr const& ServiceTypeRegistration;
            Hosting2::ServiceTypeRegistrationSPtr const& get_ServiceTypeRegistration() const { return serviceTypeRegistration_.ServiceTypeRegistration;} 

            __declspec(property(get = get_IsServicePackageInUse)) bool IsServicePackageInUse;
            bool get_IsServicePackageInUse() const { return serviceTypeRegistration_.IsServicePackageInUse; }

            __declspec (property(get=get_IsStateful)) bool IsStateful;
            bool get_IsStateful() const { return serviceDesc_.IsStateful; }

            __declspec (property(get=get_IsEmpty)) bool IsEmpty;
            bool get_IsEmpty() const { return replicas_.size() == 0; }

            __declspec (property(get=get_IsReconfiguring)) bool IsReconfiguring;
            bool get_IsReconfiguring() const { return reconfigState_.IsReconfiguring; }

            __declspec(property(get=get_LocalReplicaOpen, put=set_LocalReplicaOpen)) bool LocalReplicaOpen;
            bool get_LocalReplicaOpen() const { return localReplicaOpen_; }
            void Test_SetLocalReplicaOpen(bool value) { localReplicaOpen_ = value; }

            __declspec(property(get=get_LocalReplicaDeleted, put=set_LocalReplicaDeleted)) bool LocalReplicaDeleted;
            bool get_LocalReplicaDeleted() const { return localReplicaDeleted_; }
            void Test_SetLocalReplicaDeleted(bool value) { localReplicaDeleted_ = value; }

            __declspec(property(get = get_FMMessageStateObj)) FMMessageState const & FMMessageStateObj;
            FMMessageState const & get_FMMessageStateObj() const { return fmMessageState_; }
            FMMessageState & get_FMMessageStateObj() { return fmMessageState_; }
            FMMessageState & Test_FMMessageStateObj() { return fmMessageState_; }

            __declspec(property(get = get_ReplicaUploadStateObj)) ReplicaUploadState const & ReplicaUploadStateObj;
            ReplicaUploadState const & get_ReplicaUploadStateObj() const { return replicaUploadState_; }
            ReplicaUploadState & Test_GetReplicaUploadState() { return replicaUploadState_; }

            __declspec(property(get = get_FMMessageStage)) FMMessageStage::Enum FMMessageStage;
            FMMessageStage::Enum get_FMMessageStage() const { return fmMessageState_.MessageStage; }

            __declspec(property(get=get_RetryableErrorStateObj)) RetryableErrorState & RetryableErrorStateObj;
            RetryableErrorState const & get_RetryableErrorStateObj() const { return retryableErrorState_; }
            RetryableErrorState & get_RetryableErrorStateObj() { return retryableErrorState_; }

            __declspec(property(get = get_ServiceTypeRegistrationRetryableErrorStateObj)) RetryableErrorState & ServiceTypeRegistrationRetryableErrorStateObj;
            RetryableErrorState const & get_ServiceTypeRegistrationRetryableErrorStateObj() const { return serviceTypeRegistration_.RetryableErrorStateObj; }
            RetryableErrorState & get_ServiceTypeRegistrationRetryableErrorStateObj() { return serviceTypeRegistration_.RetryableErrorStateObj; }

            __declspec(property(get=get_LastUpdatedTime)) Common::DateTime const & LastUpdatedTime;
            Common::DateTime const & get_LastUpdatedTime() const { return lastUpdated_; }


            __declspec(property(get=get_AllowBuildDuringSwapPrimary)) bool AllowBuildDuringSwapPrimary;
            bool get_AllowBuildDuringSwapPrimary() const;

            __declspec(property(get=get_MessageRetryActiveFlag)) Infrastructure::SetMembershipFlag & MessageRetryActiveFlag;
            Infrastructure::SetMembershipFlag const & get_MessageRetryActiveFlag() const { return messageRetryActiveFlag_; }
            Infrastructure::SetMembershipFlag & get_MessageRetryActiveFlag() { return messageRetryActiveFlag_; }            

            __declspec(property(get=get_CleanupPendingFlag)) Infrastructure::SetMembershipFlag & CleanupPendingFlag;
            Infrastructure::SetMembershipFlag & get_CleanupPendingFlag() { return cleanupPendingFlag_; }

            __declspec(property(get=get_LocalReplicaClosePending)) Infrastructure::SetMembershipFlag & LocalReplicaClosePending;
            Infrastructure::SetMembershipFlag & get_LocalReplicaClosePending() { return localReplicaClosePending_; }
            Infrastructure::SetMembershipFlag const & get_LocalReplicaClosePending() const { return localReplicaClosePending_; }

            __declspec(property(get=get_LocalReplicaOpenPending)) Infrastructure::SetMembershipFlag & LocalReplicaOpenPending;
            Infrastructure::SetMembershipFlag const & get_LocalReplicaOpenPending() const { return localReplicaOpenPending_; }
            Infrastructure::SetMembershipFlag & get_LocalReplicaOpenPending() { return localReplicaOpenPending_; }

            __declspec(property(get = get_LocalReplicaServiceDescriptionUpdatePending)) Infrastructure::SetMembershipFlag & LocalReplicaServiceDescriptionUpdatePending;
            Infrastructure::SetMembershipFlag const & get_LocalReplicaServiceDescriptionUpdatePending() const { return localReplicaServiceDescriptionUpdatePendingFlag_; }            
            Infrastructure::SetMembershipFlag & Test_GetLocalReplicaServiceDescriptionUpdatePending() { return localReplicaServiceDescriptionUpdatePendingFlag_; }

            __declspec(property(get = get_CloseMode)) ReplicaCloseMode CloseMode;
            ReplicaCloseMode get_CloseMode() const { return replicaCloseMode_; }
            void Test_SetReplicaCloseMode(ReplicaCloseMode value) { replicaCloseMode_ = value; }

            __declspec(property(get = get_OpenMode)) RAReplicaOpenMode::Enum OpenMode;
            RAReplicaOpenMode::Enum get_OpenMode() const;

            __declspec(property(get = get_IsEndpoingPublishPending)) bool IsEndpointPublishPending;
            bool get_IsEndpoingPublishPending() const { return endpointPublishState_.IsEndpointPublishPending; }

            ReplicaStore & Test_GetReplicaStore() { return replicaStore_; }

#pragma region Replica Store Management

            Replica const & GetReplica(Federation::NodeId id) const;
            Replica & GetReplica(Federation::NodeId id);

#pragma endregion

#pragma region Local Replica LifeCycle
            Replica & StartCreateLocalReplica(
                Reliability::FailoverUnitDescription const & fuDesc,                
                Reliability::ReplicaDescription const & replicaDesc,
                Reliability::ServiceDescription const & serviceDesc,
                ServiceModel::ServicePackageVersionInstance const & packageVersionInstance,
                Reliability::ReplicaRole::Enum localReplicaRole,
                Reliability::ReplicaDeactivationInfo const & deactivationInfo,
                bool isRebuild,
                FailoverUnitEntityExecutionContext & executionContext);

            void UpdateInstanceForClosedReplica(
                Reliability::FailoverUnitDescription const & incomingFailoverUnitDescription,
                Reliability::ReplicaDescription const & incomingReplicaDescription,
                bool isNewInstanceDeleted,
                FailoverUnitEntityExecutionContext & executionContext);

            void UpdateStateOnLFUMLoad(
                Federation::NodeInstance const & nodeInstance,
                FailoverUnitEntityExecutionContext & executionContext);

            void OnFabricCodeUpgrade(Upgrade::FabricCodeVersionProperties const & properties);

            void OnApplicationCodeUpgrade(
                ServiceModel::ApplicationUpgradeSpecification const & specification,
                FailoverUnitEntityExecutionContext & context);

            bool CanCloseLocalReplica(
                ReplicaCloseMode closeMode) const;

            bool IsLocalReplicaClosed(
                ReplicaCloseMode closeMode) const;

            void StartCloseLocalReplica(
                ReplicaCloseMode closeMode,
                Federation::NodeInstance const & senderNode,
                FailoverUnitEntityExecutionContext & executionContext,
                Common::ActivityDescription const & activityDescription);

            void Test_ReadWriteStatusRevokedNotification(
                FailoverUnitEntityExecutionContext & executionContext)
            {
                ReadWriteStatusRevokedNotification(executionContext);
            }

            Common::ErrorCode ProcessReplicaCloseRetry(
                Hosting2::ServiceTypeRegistrationSPtr const & registration,
                FailoverUnitEntityExecutionContext & executionContext);

            void ProcessReplicaCloseReply(
                TransitionErrorCode const & transitionErrorCode,
                FailoverUnitEntityExecutionContext & executionContext);

            bool ShouldRetryCloseReplica();

            bool CanReopenDownReplica() const;
            void ReopenDownReplica(FailoverUnitEntityExecutionContext & executionContext);

            Common::ErrorCode ProcessReplicaOpenRetry(
                Hosting2::ServiceTypeRegistrationSPtr const & registration,
                FailoverUnitEntityExecutionContext & executionContext);

            void ProcessReplicaOpenReply(
                Reliability::FailoverConfig const & config,
                Reliability::ReplicaDescription const & description,
                TransitionErrorCode const & transitionErrorCode,
                FailoverUnitEntityExecutionContext & executionContext);

            void ProcessReadWriteStatusRevokedNotification(
                Reliability::ReplicaMessageBody const & body,
                FailoverUnitEntityExecutionContext & context);

            void ProcessCatchupCompletedReply(
                ProxyErrorCode const & err,
                Reliability::ReplicaDescription const & localReplica,
                FailoverUnitEntityExecutionContext & context);
            
            void ProcessCancelCatchupReply(
                ProxyErrorCode const & err,
                Reliability::FailoverUnitDescription const & ftDesc,
                FailoverUnitEntityExecutionContext & context);

#pragma endregion

#pragma region Reconfiguration
            bool TryGetConfiguration(
                Federation::NodeInstance const & localNodeInstance, 
                bool shouldMarkClosingReplicaAsDown,
                Reliability::FailoverUnitInfo & rv) const;

            // TODO: When CreateLocalReplica is properly refactored this can be made private
            void CopyCCToPC();

            // TODO: make private when When UC handling is refactored
            void CheckReconfigurationProgressAndHealthReportStatus(FailoverUnitEntityExecutionContext & executionContext);

            bool IsConfigurationMessageBodyStale(Reliability::ConfigurationMessageBody const & body);

            void UpdateDeactivationInfo(
                Reliability::FailoverUnitDescription const & incomingFTDescription,
                Reliability::ReplicaDescription const & incomingReplica,
                Reliability::ReplicaDeactivationInfo const & incomingDeactivationInfo,
                FailoverUnitEntityExecutionContext & executionContext);

            void StartDeactivate(
                Reliability::ConfigurationMessageBody const & configBody,
                FailoverUnitEntityExecutionContext & executionContext);

            void FinishDeactivate(ReplicaDescription const & ccReplica);

            void StartActivate(
                Reliability::ActivateMessageBody const & configBody,
                FailoverUnitEntityExecutionContext & executionContext);

            void FinishActivate(
                Reliability::ReplicaDescription const & localReplicaDesc,
                bool updateEndpoints,
                FailoverUnitEntityExecutionContext & executionContext);

            void DeactivateCompleted(
                Reliability::ReplicaDescription const & replicaDesc,
                FailoverUnitEntityExecutionContext & executionContext);

            void ActivateCompleted(
                Reliability::ReplicaDescription const & replicaDesc,
                FailoverUnitEntityExecutionContext & executionContext);

            void RefreshConfiguration(
                Reliability::ConfigurationMessageBody const & configBody,
                bool resetIC,
                FailoverUnitEntityExecutionContext & executionContext);

            void ReplicatorConfigurationUpdateAfterDeactivateCompleted(
                FailoverUnitEntityExecutionContext & executionContext);

            void ProcessMsgResends(
                FailoverUnitEntityExecutionContext & executionContext);

            static bool ShouldSkipPhase3Deactivate(ReplicaStore::ConfigurationReplicaStore const & replicas);

            void DoReconfiguration(
                Reliability::DoReconfigurationMessageBody const & reconfigBody,
                FailoverUnitEntityExecutionContext & executionContext);

            void ProcessGetLSNMessage(
                Federation::NodeInstance const & from,
                Reliability::ReplicaMessageBody const & msgBody,
                FailoverUnitEntityExecutionContext & executionContext);

            bool CanProcessGetLSNReply(
                Reliability::FailoverUnitDescription const & fuDesc,
                Reliability::ReplicaDescription const & replicaDesc,
                Common::ErrorCodeValue::Enum errCodeValue) const;

            void ProcessGetLSNReply(
                Reliability::FailoverUnitDescription const & fuDesc,
                Reliability::ReplicaDescription const & replicaDesc,
                Reliability::ReplicaDeactivationInfo const & deactivationInfo,
                Common::ErrorCodeValue::Enum errCodeValue,
                FailoverUnitEntityExecutionContext & executionContext);

            void ProcessReplicatorGetStatusReply(
                Reliability::ReplicaDescription const & replicaDescription,
                FailoverUnitEntityExecutionContext & executionContext);

            void ProcessReplicationConfigurationUpdate(ProxyReplyMessageBody const & updateConfigurationReply);

#pragma endregion

#pragma region Message Sending
            void SendReplicaDroppedMessage(
                Federation::NodeInstance const & nodeInstance, 
                Infrastructure::StateMachineActionQueue & queue) const;

            Reliability::ReplicaDescription CreateDroppedReplicaDescription(
                Federation::NodeInstance const & nodeInstance) const;

            bool SendUpdateConfigurationMessage(
                Infrastructure::StateMachineActionQueue & actionQueue) const;

            // TODO: To be made private
            bool SendActivateMessage(Replica const & replica, Infrastructure::StateMachineActionQueue & actionQueue) const;
#pragma endregion

#pragma region Idle Replica Management
            Replica & StartAddReplicaForBuild(Reliability::ReplicaDescription const & replicaDesc);

            void FinishAddIdleReplica(
                Replica & replica, 
                Reliability::ReplicaDescription const & replicaDesc,
                FailoverUnitEntityExecutionContext & executionContext);

            void AbortAddIdleReplica(Replica & replica);

            void StartBuildIdleReplica(Replica & replica);
            
            void FinishBuildIdleReplica(
                Replica & replica, 
                FailoverUnitEntityExecutionContext & executionContext, 
                __out bool & sendToFM);

            void StartRemoveIdleReplica(Replica & replica);
            void FinishRemoveIdleReplica(Replica & replica);
#pragma endregion

#pragma region Tracing
            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
            std::wstring ToString() const override;
            void WriteToEtw(uint16 contextSequenceId) const;
#pragma endregion

#pragma region Service Description 
            bool ValidateUpdateServiceDescription(Reliability::ServiceDescription const & serviceDescription);
            void UpdateServiceDescription(Infrastructure::StateMachineActionQueue & queue, Reliability::ServiceDescription const & serviceDescription);

            bool CanProcessUpdateServiceDescriptionReply(ProxyUpdateServiceDescriptionReplyMessageBody const & body) const;
            void ProcessUpdateServiceDescriptionReply(Infrastructure::StateMachineActionQueue & queue, ProxyUpdateServiceDescriptionReplyMessageBody const & body);
#pragma endregion

#pragma region FM Messages
            void ProcessNodeUpAck(
                vector<UpgradeDescription> const & upgrades,
                bool isActivated,
                FailoverUnitEntityExecutionContext & context);

            void ProcessReplicaEndpointUpdatedReply(
                ReplicaReplyMessageBody const& msgBody,
                FailoverUnitEntityExecutionContext& executionContext);

            void UploadReplicaForInitialDiscovery(
                FailoverUnitEntityExecutionContext & context);

            void ProcessReplicaDroppedReply(
                Common::ErrorCode const & error,
                Reliability::ReplicaDescription const & incomingDesc,
                FailoverUnitEntityExecutionContext & context);

            void ProcessReplicaUpReply(
                Reliability::ReplicaDescription const & desc,
                FailoverUnitEntityExecutionContext & context);

            // Called when a message from the fm about this partition is received
            void OnReplicaUploaded(
                FailoverUnitEntityExecutionContext & context);

            void AssertIfReplicaNotUploaded(
                std::wstring const & action) const;

            bool TryComposeFMMessage(
                Federation::NodeInstance const & nodeInstance,
                Common::StopwatchTime now,
                __out Communication::FMMessageData & description) const;

            void OnFMMessageSent(
                Common::StopwatchTime now,
                int64 sequenceNumber,
                FailoverUnitEntityExecutionContext & context);
#pragma endregion

#pragma region RAP Messages
            
            void ProcessProxyReplicaEndpointUpdate(
                ReplicaMessageBody const& msgBody,
                FailoverUnitEntityExecutionContext& executionContext);

#pragma endregion

            void AssertInvariants() const;

#pragma region Unit Test Support
            FMMessageState & Test_GetFMMessageState();
            EndpointPublishState & Test_GetEndpointPublishState();
            void Test_SetMessageRetryFlag(Infrastructure::SetMembershipFlag const & flag);
            void Test_SetCleanupPendingFlag(Infrastructure::SetMembershipFlag const & flag);
            void Test_SetLastUpdatedTime(Common::DateTime time);
            void Test_SetServiceTypeRegistration(Hosting2::ServiceTypeRegistrationSPtr const & registration);            
            void Test_SetDeactivationInfo(Reliability::ReplicaDeactivationInfo const & deactivationInfo);
            void Test_SetLastStableEpoch(Reliability::Epoch const & epoch);

            void Test_UpdateStateOnLocalReplicaDown(Hosting::HostingAdapter & hosting, Infrastructure::StateMachineActionQueue & queue)
            {
                UpdateStateOnLocalReplicaDown(hosting, queue);
            }

            void Test_UpdateStateOnLocalReplicaDropped(Hosting::HostingAdapter & hosting, Infrastructure::StateMachineActionQueue & queue)
            {
                UpdateStateOnLocalReplicaDropped(hosting, queue);
            }

            void Test_ResetLocalState(Infrastructure::StateMachineActionQueue & queue) { ResetLocalState(queue); }

#pragma endregion

            FABRIC_FIELDS_11(
                failoverUnitDesc_, 
                serviceDesc_, 
                icEpoch_,
                localReplicaId_,
                localReplicaInstanceId_,
                state_,
                localReplicaDeleted_,
                lastUpdated_,
                replicas_,
                lastStableEpoch_,
                deactivationInfo_);


        private:

#pragma region Local Replica Management
            void FinalConstruct(FailoverConfig const & config);
            
            void Initialize();

            void FinishCloseLocalReplica(
                FailoverUnitEntityExecutionContext & executionContext);

            void ReadWriteStatusRevokedNotification(
                FailoverUnitEntityExecutionContext & executionContext);

            void UpdateStateOnLocalReplicaDown(Hosting::HostingAdapter & hosting, Infrastructure::StateMachineActionQueue & queue);
            void UpdateStateOnLocalReplicaDropped(Hosting::HostingAdapter & hosting, Infrastructure::StateMachineActionQueue & queue);

            ReconfigurationProgressStages::Enum CheckReconfigurationProgress(FailoverUnitEntityExecutionContext & executionContext);

            void ResetLocalState(Infrastructure::StateMachineActionQueue & queue);
            void ResetFlags(Infrastructure::StateMachineActionQueue & queue);
            void ResetNonFlagState();

            void MarkLocalReplicaDeleted(FailoverUnitEntityExecutionContext& context);

            Common::ErrorCode TryGetAndAddServiceTypeRegistration(
                Hosting2::ServiceTypeRegistrationSPtr const & registration,
                FailoverUnitEntityExecutionContext & executionContext);

            void ProcessOpenFailure(
                FailoverUnitEntityExecutionContext & executionContext);

            void ProcessFindServiceTypeRegistrationFailure(
                FailoverUnitEntityExecutionContext & executionContext);

            void EnqueueHealthEvent(
                Health::ReplicaHealthEvent::Enum ev,
                Infrastructure::StateMachineActionQueue & queue,
                Health::IHealthDescriptorSPtr const & descriptor) const;


            void ProcessHealthOnRoleTransition(
                ProxyErrorCode const & proxyErrorCode,
                FailoverUnitEntityExecutionContext & executionContext);
                
            Communication::FMMessageData ComposeReplicaUpMessageBody(
                Federation::NodeInstance const & localNodeInstance,
                int64 msgSequenceNumber) const;

#pragma endregion

#pragma region Reconfiguration

            void OnReconfigurationPhaseHealthReport(Infrastructure::StateMachineActionQueue & queue,
               ReconfigurationStuckDescriptorSPtr const & newReport);

            void OnPhaseChanged(Infrastructure::StateMachineActionQueue & queue);

            Reliability::ReplicaDeactivationInfo GetDeactivationInfoWithBackwardCompatibilitySupport(
                Reliability::FailoverUnitDescription const & incomingFTDescription,
                Reliability::ReplicaDescription const & incomingReplica,
                Reliability::ReplicaDeactivationInfo const & incomingDeactivationInfo);

            bool CanProcessDoReconfiguration(
                Reliability::DoReconfigurationMessageBody const & reconfigBody,
                FailoverUnitEntityExecutionContext & executionContext) const;

            bool TryGetConfigurationInternal(
                Federation::NodeInstance const & localNodeInstance, 
                bool shouldMarkClosingReplicaAsDown,
                Reliability::FailoverUnitInfo & rv) const;

            bool IsReplicatorConfigurationUpdated(ProxyReplyMessageBody const & updateConfigurationReply);

            void UpdateReconfigurationEpochs(
                Reliability::DoReconfigurationMessageBody const & incomingMessage);

            void UpdateReconfigurationState(
                ReconfigurationType::Enum reconfigType,
                Common::TimeSpan phase0Duration,
                FailoverUnitEntityExecutionContext & executionContext);

            void StartReconfiguration(
                FailoverUnitEntityExecutionContext & executionContext);

            bool TryAbortReconfiguration(
                Reliability::FailoverUnitDescription const & incomingFTDescription,
                FailoverUnitEntityExecutionContext & executionContext);

            void RevertConfiguration();

            void FinishDeactivationInfoUpdate(Replica & replica, FailoverUnitEntityExecutionContext & executionContext);

            void GetLocalReplicaLSNCompleted(
                Reliability::ReplicaDescription const & replicaDesc,
                FailoverUnitEntityExecutionContext & executionContext);

            void ResetReconfigurationStates();

            void CompleteReconfiguration(FailoverUnitEntityExecutionContext & executionContext);

            void UpdateRemoteReplicaRoles(
                Reliability::ConfigurationMessageBody const & reconfigBody,
                FailoverUnitEntityExecutionContext & executionContext);

            void UpdateRemoteReplicaStates(
                Reliability::ConfigurationMessageBody const & reconfigBody,
                bool isReconfigurationStarting,
                bool & updateReplicationConfiguration,
                FailoverUnitEntityExecutionContext & executionContext);

            ReconfigurationType::Enum IdentifyReconfigurationType(
                Reliability::DoReconfigurationMessageBody const & reconfigBody,
                Reliability::ReplicaDescription const & newReplicaDesc) const;

            void UpdateLocalReplicaStatesAndRoles(
                Reliability::ReplicaDescription const & newReplicaDesc);

            void MarkReplicationConfigurationUpdatePending();

            void MarkReplicationConfigurationUpdatePendingAndSendMessage(Infrastructure::StateMachineActionQueue & actionQueue);

            void FinishSwapPrimary(FailoverUnitEntityExecutionContext & executionContext);

            void CatchupCompleted(
                ProxyErrorCode const & err,
                Reliability::ReplicaDescription const & localReplicaDesc,
                FailoverUnitEntityExecutionContext & executionContext);

            void CancelCatchupCompleted(
                ProxyErrorCode const & result,
                FailoverUnitEntityExecutionContext & executionContext);

            bool ProcessReplicaMessageResends(ReconfigurationAgentComponent::Infrastructure::StateMachineActionQueue & actionQueue) const;
            bool ProcessReplicaMessageResend(Replica const & replica, ReconfigurationAgentComponent::Infrastructure::StateMachineActionQueue & actionQueue) const;
            bool ProcessPhaseMessageResends(ReconfigurationAgentComponent::Infrastructure::StateMachineActionQueue & actionQueue) const;

#pragma region Phase1
            typedef std::vector<Replica const *> ReplicaPointerList;
            bool TryFindPrimary(int64 & primary) const;
            bool TryFindPrimaryDuringDataLoss(int64 & primary) const;
			bool TryFindPrimary(ReplicaPointerList const & replicas, int64 & primary) const;
            ReplicaPointerList GetReplicasToBeConsideredForGetLSN(bool ignoreUnknownLSN) const;
            ReplicaPointerList GetReplicasToBeConsideredForGetLSNDuringDataLoss() const;
            ReplicaPointerList GetReplicasToBeConsideredForGetLSN() const;

            bool DoAllReplicasSupportDeactivationInfo(ReplicaPointerList const & replicas) const;
            void RemoveReplicasWithOldDeactivationEpoch(ReplicaPointerList & replicas) const;
            int64 FindReplicaWithHighestLastLSN(ReplicaPointerList const & replicas) const;
            int64 FindPrimaryWithBestCatchupCapability(ReplicaPointerList const & replicas) const;
			ReplicaPointerList::const_iterator FindReplicaIteratorWithHighestLastLSN(ReplicaPointerList const & replicas) const;

            void StartPhase1GetLSN(FailoverUnitEntityExecutionContext & executionContext);
            void UpdateStateAndSendGetLSNMessage(Infrastructure::StateMachineActionQueue & actionQueue);
            bool SendPhase1GetLSNMessage(Replica const & replica, Infrastructure::StateMachineActionQueue & actionQueue) const;
            ReconfigurationProgressStages::Enum CheckPhase1GetLSNProgress(__out int64 & primaryReplicaId, FailoverUnitEntityExecutionContext & executionContext);
            bool CheckDataLossAtPhase1GetLSN(ReplicaSetCountsAtGetLsn const & pcCounter, ReplicaSetCountsAtGetLsn const & ccCounter, Infrastructure::StateMachineActionQueue & actionQueue);
            void FinishPhase1GetLSN(int64 primaryReplicaId, FailoverUnitEntityExecutionContext & executionContext);
            bool IsRemoteReplicaRestartNeededAfterGetLsn(Replica const & remoteReplica) const;

            void UpdateReplicaSetCountsAtPhase1GetLSN(
                Replica const & replica,
                bool isInConfiguration,
                ReplicaSetCountsAtGetLsn & counter) const;

#pragma endregion

#pragma region Phase0/2
            void StartPhase2Catchup(FailoverUnitEntityExecutionContext & executionContext);
            void StartPhase2CatchupOnFailover(FailoverUnitEntityExecutionContext & executionContext);
            void UpdateLocalStateOnPhase2Catchup(FailoverUnitEntityExecutionContext & executionContext);
#pragma endregion

#pragma region Abort Phase 0
            void StartAbortPhase0Demote(FailoverUnitEntityExecutionContext & executionContext);
#pragma endregion

#pragma region Phase3
            void StartPhase3Deactivate(FailoverUnitEntityExecutionContext & executionContext);
            void UpdateStateAtPhase3Deactivate(Infrastructure::StateMachineActionQueue & actionQueue);
            ReconfigurationProgressStages::Enum CheckPhase3DeactivateProgress() const;
            bool ShouldSkipPhase3Deactivate(FailoverUnitEntityExecutionContext & executionContext) const;
            void FinishPhase3Deactivate(FailoverUnitEntityExecutionContext & executionContext);
#pragma endregion

#pragma region Phase4
            void StartPhase4Activate(FailoverUnitEntityExecutionContext & executionContext);
            void UpdateStateAtPhase4Activate(Infrastructure::StateMachineActionQueue & actionQueue);            
            bool SendEndReconfigurationMessage(Infrastructure::StateMachineActionQueue & actionQueue) const;
            void UpdateEndReconfigurationMessageState(ReconfigurationProgressStages::Enum, Infrastructure::StateMachineActionQueue & actionQueue);
            ReconfigurationProgressStages::Enum CheckPhase4ActivateProgress() const;
            bool AreAllUpReadyReplicasActivated() const;
            bool IsEndReconfigurationMessagePending(bool areAllUpReadyReplicasActivated) const;
            void FinishPhase4Activate(FailoverUnitEntityExecutionContext & executionContext);

#pragma endregion

#pragma region Replica Description List
            std::vector<Reliability::ReplicaDescription> CreateReplicationConfigurationReplicaDescriptionList() const;

            std::vector<Reliability::ReplicaDescription> CreateCurrentConfigurationReplicaDescriptionList() const;

            std::vector<Reliability::ReplicaDescription> CreateChangeConfigurationReplicaDescriptionList(int64 replicaIdForPrimary) const;

            std::vector<Reliability::ReplicaDescription> CreateConfigurationReplicaDescriptionListForRestartRemoteReplica() const;

            std::vector<Reliability::ReplicaDescription> CreateConfigurationDescriptionListForDeactivateOrActivate(
                int64 replicaId) const;
#pragma endregion

#pragma endregion

#pragma region Message Sending

#pragma region Reconfiguration
            bool SendPhase3DeactivateMessage(Replica const & replica, Infrastructure::StateMachineActionQueue & actionQueue) const;

            void SendContinueSwapPrimaryMessage(
                Reliability::DoReconfigurationMessageBody const * incomingMessage,
                FailoverUnitEntityExecutionContext const & context) const;

            void SendDoReconfigurationReply(
                FailoverUnitEntityExecutionContext const & context) const;

            bool SendReplicatorGetStatusMessage(Infrastructure::StateMachineActionQueue & queue) const;

            bool SendRestartRemoteReplicaMessage(Replica const & replica, Infrastructure::StateMachineActionQueue & actionQueue) const;

            bool SendCancelCatchupMessage(Infrastructure::StateMachineActionQueue & actionQueue) const;

            bool SendReplicaLifeCycleMessage(Replica const & replica, Infrastructure::StateMachineActionQueue & actionQueue) const;

            bool SendUpdateDeactivationInfoMessage(Replica const & replica, Infrastructure::StateMachineActionQueue & actionQueue) const;

            bool SendUpdateReplicationConfigurationMessage(Infrastructure::StateMachineActionQueue & actionQueue) const;

            bool SendUpdateConfigurationMessage(
                Infrastructure::StateMachineActionQueue & actionQueue,
                ProxyMessageFlags::Enum flags) const;

            void SendChangeConfiguration(Infrastructure::StateMachineActionQueue & actionQueue) const;

            bool SendDataLossReportMessage(Infrastructure::StateMachineActionQueue & actionQueue) const;

#pragma endregion

#pragma region LifeCycle
            void SendReplicaCloseMessage(Infrastructure::StateMachineActionQueue & queue) const;

            void SendReadWriteStatusRevokedNotificationReply(
                ReplicaMessageBody const& incomingBody, 
                Infrastructure::StateMachineActionQueue & queue) const;
#pragma endregion

#pragma endregion

#pragma region Backward Compatibility
            void UpdateStateForBackwardCompatibility();
            void UpdateLastStableEpochForBackwardCompatibility();
            void UpdateFailoverUnitDescriptionForBackwardCompatiblity();
            void FixupServiceDescriptionOnLfumLoad();
#pragma endregion

#pragma region Endpoint
            void SendPublishEndpointMessage(FailoverUnitEntityExecutionContext &);

            void UpdateEndpoints(
                Replica & replica,
                Reliability::ReplicaDescription const & newReplicaDesc,
                FailoverUnitEntityExecutionContext & executionContext);

            void UpdateServiceEndpoint(
                Replica & replica,
                Reliability::ReplicaDescription const & newReplicaDesc,
                FailoverUnitEntityExecutionContext & executionContext);

            void UpdateServiceEndpointInternal(
                Replica & replica,
                Reliability::ReplicaDescription const & newReplicaDesc);

            void UpdateReplicatorEndpoint(
                Replica & replica,
                Reliability::ReplicaDescription const & newReplicaDesc);
#pragma endregion

#pragma region Verification and Assertions
            void AssertRemoteReplicaInvariants(Replica const & replica) const;
            void AssertIdleReplicaInvariants(Replica const & replica) const;
            void AssertLocalReplicaInvariants() const;
            void AssertIfReplicaFlagMismatch(bool expected, bool actual, std::wstring const & flagName, Replica const & r) const;
            void AssertReplicaFlagMismatch(std::wstring const & flagName, Replica const & r) const;
            void AssertSenderNodeIsInvalid() const;
            void AssertEpochInvariants() const;
#pragma endregion

            void ArmMessageRetryTimerIfSent(bool isSent, Infrastructure::StateMachineActionQueue & queue);

            void InternalUpdateLocalReplicaPackageVersionInstance(ServiceModel::ServicePackageVersionInstance const & newVersionInstance);
            void InternalSetServiceDescription(Reliability::ServiceDescription const & serviceDescription);
            void InternalSetFailoverUnitDescription(Reliability::FailoverUnitDescription const & ftDesc);

            void UpdateServiceDescriptionUpdatePendingFlag(Infrastructure::StateMachineActionQueue & queue);

            // FailoverUnit and Service descriptions
            Reliability::FailoverUnitDescription failoverUnitDesc_;
            Reliability::ServiceDescription serviceDesc_;
            Reliability::Epoch icEpoch_;

            int64 dataLossVersionToReport_;

            // Replica-set
            std::vector<Replica> replicas_;
            ReplicaStore replicaStore_;

            // Local replica id and instance id
            int64 localReplicaId_;
            int64 localReplicaInstanceId_;

            // State
            FailoverUnitStates::Enum state_;

            ReconfigurationState reconfigState_;
            ReconfigurationHealthState reconfigHealthState_;
            ReconfigurationStuckHealthReportDescriptorFactory reconfigHealthDescriptorFactory_;

            // Node to which the reply needs to be sent, if not the FM
            Federation::NodeInstance senderNode_;

            // Flag indicating if reply pending for ReplicaClose messages
            Infrastructure::SetMembershipFlag localReplicaClosePending_;

            // Flag indicating if retry of replica open is pending
            Infrastructure::SetMembershipFlag localReplicaOpenPending_;

            // Flag indicating if local replica is open
            bool localReplicaOpen_;

            // Flag indicating if local replica has been deleted
            bool localReplicaDeleted_;

            // Flag indicating if the replicator configuration needs updating
            bool updateReplicatorConfiguration_;

            ServiceTypeRegistrationWrapper serviceTypeRegistration_;

            // Change configuration related information
            Reliability::FailoverUnitDescription  changeConfigFailoverUnitDesc_;
            std::vector<Reliability::ReplicaDescription> changeConfigReplicaDescList_;

            // Last update time
            Common::DateTime lastUpdated_;

            // flag indicating whether message retry timer is active
            Infrastructure::SetMembershipFlag messageRetryActiveFlag_;

            // flag indicating whether this FT is to be cleaned up or not
            Infrastructure::SetMembershipFlag cleanupPendingFlag_;

            // flag indicating whether update to RAP for service description is pending or not
            Infrastructure::SetMembershipFlag localReplicaServiceDescriptionUpdatePendingFlag_;

            RetryableErrorState retryableErrorState_;

            ReplicaCloseMode replicaCloseMode_;

            // This field is for backward compatibility only 
            // It is not used
            Reliability::Epoch lastStableEpoch_;

            TraceWriter tracer_;

            Reliability::ReplicaDeactivationInfo deactivationInfo_;

            FMMessageState fmMessageState_;
            ReplicaUploadState replicaUploadState_;
            EndpointPublishState endpointPublishState_;
        };
    }
}
