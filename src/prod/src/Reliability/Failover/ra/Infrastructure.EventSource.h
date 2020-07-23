// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#include "Diagnostics.IEventData.h"

#define DECLARE_RA_STRUCTURED_TRACE(trace_name, ...) Common::TraceEventWriter<__VA_ARGS__> trace_name
#define RA_STRUCTURED_TRACE(trace_name, trace_id, trace_type, trace_level, ...) \
            trace_name( \
                Common::TraceTaskCodes::RA, \
                trace_id, \
                #trace_name "_" trace_type, \
                Common::LogLevel::trace_level, \
                __VA_ARGS__)

#define RA_STRUCTURED_TRACE_QUERY(trace_name, trace_id, trace_type, trace_level, ...) \
            trace_name( \
                Common::TraceTaskCodes::RA, \
                trace_id, \
                trace_type, \
                Common::LogLevel::trace_level, \
                Common::TraceChannelType::Debug, \
                TRACE_KEYWORDS2(Default, ForQuery), \
                __VA_ARGS__)

#define RA_STRUCTURED_TRACE_OPERATIONAL(trace_name, public_name, category, trace_id, trace_type, trace_level, ...) \
            trace_name( \
                Common::ExtendedEventMetadata::Create(public_name, category), \
                Common::TraceTaskCodes::RA, \
                trace_id, \
                trace_type, \
                Common::LogLevel::trace_level, \
                Common::TraceChannelType::Operational, \
                __VA_ARGS__)

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            class RAEventSource
            {
            public:

                DECLARE_RA_STRUCTURED_TRACE(LifeCycleOpen,
                    std::wstring // node id
                    );

                DECLARE_RA_STRUCTURED_TRACE(LifeCycleOpenFailed,
                    std::wstring, // node id
                    Common::ErrorCode);

                DECLARE_RA_STRUCTURED_TRACE(LifeCycleOpenSuccess,
                    std::wstring, //node id
                    uint64
                    );
                
                DECLARE_RA_STRUCTURED_TRACE(LifeCycleCloseBegin,
                    std::wstring // node id
                    );

                DECLARE_RA_STRUCTURED_TRACE(LifeCycleCloseEnd,
                    std::wstring // node id
                    );

                DECLARE_RA_STRUCTURED_TRACE(LifeCycleRAIgnoringNodeUpAck,
                    std::wstring
                    );

                DECLARE_RA_STRUCTURED_TRACE(LifeCycleNodeUpAckReceived,
                    std::wstring,
                    Common::StringLiteral,
                    Common::FabricVersionInstance, // node
                    Reliability::NodeUpAckMessageBody,
                    std::wstring
                    );

                DECLARE_RA_STRUCTURED_TRACE(LifeCycleNodeUpAckProcessed,
                    std::wstring,
                    Common::StringLiteral);

                DECLARE_RA_STRUCTURED_TRACE(LifeCycleNodeActivationStateChange,
                    std::wstring,
                    Node::NodeDeactivationInfo,
                    Reliability::FailoverManagerId);

                DECLARE_RA_STRUCTURED_TRACE(LifeCycleNodeActivationMessageProcess,
                    std::wstring,
                    Node::NodeDeactivationInfo,
                    Reliability::FailoverManagerId,
                    Node::NodeDeactivationInfo,
                    Node::NodeDeactivationInfo,
                    std::wstring);

                DECLARE_RA_STRUCTURED_TRACE(LifeCycleNodeUpgradingStateChange,
                    std::wstring,
                    bool);

                DECLARE_RA_STRUCTURED_TRACE(LifeCycleNodeDeactivationProgress,
                    std::wstring, // node instance
                    std::wstring, // activity id
                    Infrastructure::EntityEntryBaseList);

                DECLARE_RA_STRUCTURED_TRACE(LifeCycleCloseReplicaInProgress,
                    std::wstring, // node instance
                    std::wstring, // activity id
                    Infrastructure::EntityEntryBaseList);

                DECLARE_RA_STRUCTURED_TRACE(LifeCycleLastReplicaUpState,
                    std::wstring, // node instance
                    std::wstring, // activity id
                    Node::PendingReplicaUploadState::TraceData);                    
                    
                DECLARE_RA_STRUCTURED_TRACE(GenerationSet, 
                    std::wstring, 
                    ReconfigurationAgentComponent::GenerationState,
                    Reliability::GenerationHeader,
                    std::wstring
                    );

                DECLARE_RA_STRUCTURED_TRACE(GenerationUpdateRejected,
                    std::wstring,
                    ReconfigurationAgentComponent::GenerationState,
                    Reliability::GenerationHeader,
                    std::wstring
                    );

                DECLARE_RA_STRUCTURED_TRACE(GenerationUpdate,
                    std::wstring,
                    Reliability::Epoch,                
                    std::wstring
                    );

                DECLARE_RA_STRUCTURED_TRACE(GenerationUploadingLfum,
                    std::wstring,
                    Reliability::GenerationHeader,
                    std::wstring,
                    std::vector<Reliability::FailoverUnitInfo>
                    );

                DECLARE_RA_STRUCTURED_TRACE(GenerationIgnoreMessage,
                    std::wstring,
                    std::wstring, // action,
                    Federation::NodeInstance, // from
                    Reliability::GenerationHeader, // generation header
                    ReconfigurationAgentComponent::GenerationState
                    );

                DECLARE_RA_STRUCTURED_TRACE(GenerationIgnoreLFUMUploadReply,
                    Federation::NodeInstance,
                    ReconfigurationAgentComponent::GenerationState,
                    Reliability::GenerationHeader
                    );

                DECLARE_RA_STRUCTURED_TRACE(GenerationUpdatedSendGeneration,
                    std::wstring,
                    ReconfigurationAgentComponent::GenerationState,
                    Reliability::GenerationHeader
                    );

                DECLARE_RA_STRUCTURED_TRACE(MessageReceive,
                    std::wstring,  // node instance id
                    std::wstring,  // action
                    Federation::NodeInstance, 
                    Transport::MessageId
                    );

                DECLARE_RA_STRUCTURED_TRACE(MessageProcessStart,
                    std::string,
                    Federation::NodeInstance,
                    std::wstring,
                    std::wstring,
                    Common::TraceCorrelatedEventBase,
                    FailoverUnit
                    );

                DECLARE_RA_STRUCTURED_TRACE(MessageIgnore,
                    std::wstring, // node id
                    Common::StringLiteral, // reason,
                    std::wstring, // action
                    Transport::MessageId);

                DECLARE_RA_STRUCTURED_TRACE(RAUnknownAction,
                    std::wstring, 
                    Federation::NodeInstance
                    );

                // the trace for a RA Failover unit is composed of multiple traces
                // this is the parent trace
                DECLARE_RA_STRUCTURED_TRACE(RAFailoverUnit,
                    uint16,
                    FailoverUnit::TraceWriter,
                    std::vector<Replica>
                    );

                DECLARE_RA_STRUCTURED_TRACE(RAReplica,
                    uint16,
                    Replica::TraceWriter);

                DECLARE_RA_STRUCTURED_TRACE(ProxyReplyMessageBody,
                    uint16, // context seq id
                    Reliability::FailoverUnitDescription, 
                    Reliability::ReplicaDescription, // local replica
                    std::vector<Reliability::ReplicaDescription>, // remote replicas                    
                    std::wstring, // message flags
                    ProxyErrorCode
                    );

                DECLARE_RA_STRUCTURED_TRACE(ProxyRequestMessageBody,
                    uint16, // context seq id
                    Reliability::FailoverUnitDescription, 
                    Reliability::ReplicaDescription, // local replica
                    std::vector<Reliability::ReplicaDescription>, // remote replicas
                    std::wstring, 
                    ProxyMessageFlags::Trace // flags
                    );

                DECLARE_RA_STRUCTURED_TRACE(ProxyUpdateServiceDescriptionReplyMessageBody,
                    uint16, // context seq id
                    Reliability::FailoverUnitDescription, 
                    Reliability::ServiceDescription,
                    Reliability::ReplicaDescription, // local replica
                    std::wstring // Error
                    );

                DECLARE_RA_STRUCTURED_TRACE(ServiceType,
                    std::wstring, // servicetypeid
                    Federation::NodeInstance,
                    ServiceModel::ServicePackageVersionInstance,
                    std::wstring,
                    int,
                    bool);

                DECLARE_RA_STRUCTURED_TRACE(HostingFindServiceTypeRegistrationCompleted,
                    std::wstring, // node instance
                    ServiceModel::ServiceTypeIdentifier, // type id
                    Common::ErrorCode, 
                    ServiceModel::ServicePackageVersionInstance, // in version instance
                    ServiceModel::ServicePackageVersionInstance, // out version instance
                    std::wstring, // host id
                    std::wstring // runtime id
                    );

                DECLARE_RA_STRUCTURED_TRACE(HostingServiceTypeRegistered,
                    std::wstring, // node instance
                    Hosting::HostingEventName::Trace,
                    ServiceModel::VersionedServiceTypeIdentifier, // VersionedServiceTypeId
                    std::wstring);

                DECLARE_RA_STRUCTURED_TRACE(HostingTerminateServiceHost,
                    std::wstring, // node instance
                    std::wstring, // host id
                    int, // PID);
                    Hosting::TerminateServiceHostReason::Trace,
                    std::wstring // activityId
                    );

                DECLARE_RA_STRUCTURED_TRACE(HostingIgnoreEvent,
                    std::wstring, // node instance
                    std::wstring, // activity id
                    Common::StringLiteral // reason
                    );
                    
                DECLARE_RA_STRUCTURED_TRACE(HostingFinishEvent,
                    std::wstring, // node instance
                    std::wstring, // activity id
                    Diagnostics::FilterEntityMapPerformanceData
                    );

                DECLARE_RA_STRUCTURED_TRACE(HostingProcessServiceTypeEvent,
                    std::wstring, // node instance
                    ServiceModel::ServiceTypeIdentifier,
                    ServiceTypeUpdateKind::Trace, // event type tag
                    ReconfigurationAgentComponent::Node::ServiceTypeUpdateResult::Trace, // result
                    uint64 // sequence number
                    );

                DECLARE_RA_STRUCTURED_TRACE(HostingProcessPartitionEvent,
                    std::wstring, // node instance
                    PartitionId,
                    ServiceTypeUpdateKind::Trace, // event type tag
                    ReconfigurationAgentComponent::Node::ServiceTypeUpdateResult::Trace, // result
                    uint64 // sequence number
                    );
            
                DECLARE_RA_STRUCTURED_TRACE(HostingProcessClosedEvent,
                    std::wstring, // node instance
                    Hosting::HostingEventName::Trace, // tag
                    std::wstring, // hostid
                    std::wstring, // runtimeid
                    std::wstring // activity id);
                    );

                DECLARE_RA_STRUCTURED_TRACE(HostingSendServiceTypeMessage,
                    std::wstring, // node instance
                    ServiceTypeUpdateKind::Trace, // tag
                    uint64);

                DECLARE_RA_STRUCTURED_TRACE(HostingSendPartitionNotificationMessage,
                    std::wstring, // node instance
                    uint64);

                DECLARE_RA_STRUCTURED_TRACE(HostingProcessServiceTypeReplyMessage,
                    std::wstring, // node instance
                    ServiceTypeUpdateKind::Trace, // event type tag
                    std::wstring,
                    uint64);

                DECLARE_RA_STRUCTURED_TRACE(HostingIgnoreStaleServiceTypeReplyMessage,
                    std::wstring, // node instance
                    ServiceTypeUpdateKind::Trace, // event type tag
                    std::wstring //tag
                    );

                DECLARE_RA_STRUCTURED_TRACE(ApplicationUpgradePrintServiceTypeStatus,
                    std::wstring,
                    ServiceModel::ServicePackageVersionInstance,
                    std::wstring);

                DECLARE_RA_STRUCTURED_TRACE(InvalidMessageDropped,
                    std::wstring, // node instance
                    Transport::MessageId,
                    std::wstring,
                    uint64);

                DECLARE_RA_STRUCTURED_TRACE(NodeActivationDeactivationMessageDrop,
                    std::wstring, // node instance
                    Transport::MessageId,
                    bool, // isActivate
                    int64);            

                DECLARE_RA_STRUCTURED_TRACE(NodeActivationDeactivationDropBecauseStale,
                    std::wstring, // nodeinstance
                    Transport::MessageId,
                    bool, // isactivate
                    int64, 
                    int64);

                DECLARE_RA_STRUCTURED_TRACE(HostingSendingReplicaUpMessage,
                    std::wstring, // node instance
                    Common::StringLiteral,
                    Reliability::ReplicaUpMessageBody
                    );

                DECLARE_RA_STRUCTURED_TRACE(ReportFaultCannotProcessDueToNotReady,
                    Common::Guid,
                    Federation::NodeInstance,
                    ReconfigurationAgentComponent::FailoverUnit
                    );

                DECLARE_RA_STRUCTURED_TRACE(ReportFaultProcessingBegin,
                    Common::Guid, // local activity id
                    std::wstring, // node instance id
                    Common::Guid, // partition id
                    int64, // replica id
                    Reliability::FaultType::Trace,
                    bool, // is force 
                    Common::ActivityDescription
                    );

                DECLARE_RA_STRUCTURED_TRACE(HostingSendingReplicaDownMessage,
                    std::wstring, // node instance
                    Reliability::ReplicaListMessageBody
                    );

                DECLARE_RA_STRUCTURED_TRACE(RetryTimerCreate, 
                    std::wstring,
                    Common::TimeSpan
                    );

                DECLARE_RA_STRUCTURED_TRACE(RetryTimerTryCancel,
                    std::wstring, // id
                    int64, // passed in seq number
                    int64 // timer sequence number
                    );

                DECLARE_RA_STRUCTURED_TRACE(RetryTimerFire,
                    std::wstring // id
                    );

                DECLARE_RA_STRUCTURED_TRACE(RetryTimerSetCalled,
                    std::wstring, // id
                    int64, // current sequence number
                    Common::StringLiteral
                    );

                DECLARE_RA_STRUCTURED_TRACE(RetryTimerClosed,
                    std::wstring // id
                    );

                DECLARE_RA_STRUCTURED_TRACE(FMMessageRetry,
                    std::wstring, // node instance id
                    std::wstring, // activity id
                    Reliability::FailoverManagerId, // target fm
                    Diagnostics::FMMessageRetryPerformanceData
                    );

                DECLARE_RA_STRUCTURED_TRACE(ServiceLocationUpdate,
                    Common::Guid, // FT id
                    Federation::NodeInstance, // node instance
                    int64, // replica id
                    int64, // instance id
                    std::wstring, // new endpoint
                    std::wstring  // old endpoint
                    );

                DECLARE_RA_STRUCTURED_TRACE(ReplicatorLocationUpdate,
                    Common::Guid, // FT id
                    Federation::NodeInstance, // node instance
                    int64, // replica id
                    int64, // instance id
                    std::wstring, // new endpoint
                    std::wstring  // old endpoint
                    );

                DECLARE_RA_STRUCTURED_TRACE(SendMessageToFM,
                    std::string, // id
                    Federation::NodeInstance,
                    std::wstring, // activity id
                    std::wstring, // message
                    Common::TraceCorrelatedEventBase // body
                    );

                DECLARE_RA_STRUCTURED_TRACE(SendMessageToFMM,
                    std::string, // id
                    Federation::NodeInstance,
                    std::wstring, // activity id
                    std::wstring, // message
                    Common::TraceCorrelatedEventBase // body
                    );

                DECLARE_RA_STRUCTURED_TRACE(SendMessageToRA,
                    std::string, // id
                    Federation::NodeInstance, // source
                    std::wstring, // activity id
                    std::wstring, // message
                    Federation::NodeInstance, // target
                    Common::TraceCorrelatedEventBase // body
                    );

                DECLARE_RA_STRUCTURED_TRACE(SendMessageToRAP,
                    std::string, // id
                    Federation::NodeInstance,
                    std::wstring, // activity id
                    std::wstring, // message
                    std::wstring, // host id
                    int,
                    Common::TraceCorrelatedEventBase // body
                    );

                DECLARE_RA_STRUCTURED_TRACE(SendNonFTMessageToFM,
                    std::wstring, // node inst
                    std::wstring, // activity id
                    std::wstring, // message
                    Reliability::FailoverManagerId, 
                    Common::TraceCorrelatedEventBase // body
                    );

                DECLARE_RA_STRUCTURED_TRACE(EntityJobItemTraceInformation,
                    uint16, // id
                    std::wstring, // trace metadata
                    JobItemName::Trace,
                    std::wstring // activityId
                    );

                DECLARE_RA_STRUCTURED_TRACE(EntityDeleted,
                    std::string, // id
                    Federation::NodeInstance,
                    Common::ErrorCode, // commit result
                    std::vector<Infrastructure::EntityJobItemTraceInformation>
                    );

                DECLARE_RA_STRUCTURED_TRACE(FTUpdate,
                    Common::Guid,
                    Federation::NodeInstance,
                    FailoverUnit,
                    std::vector<Infrastructure::EntityJobItemTraceInformation>,
                    Common::ErrorCode,
                    Diagnostics::ScheduleEntityPerformanceData,
                    Diagnostics::ExecuteEntityJobItemListPerformanceData,
                    Diagnostics::CommitEntityPerformanceData
                    );

                DECLARE_RA_STRUCTURED_TRACE(ReplicaUpReplyAction,
                    std::string,
                    Federation::NodeInstance,
                    std::wstring,
                    bool, // is dropped list
                    Reliability::FailoverUnitDescription,
                    std::vector<Reliability::ReplicaInfo>);

                DECLARE_RA_STRUCTURED_TRACE(MessageAction,
                    std::string,
                    Federation::NodeInstance,
                    std::wstring, // activity id
                    std::wstring, // action
                    Common::TraceCorrelatedEventBase
                    );


                DECLARE_RA_STRUCTURED_TRACE(FTJobItemDroppingMessageBecauseEntryNotFound,
                    Common::Guid, // fuid
                    Federation::NodeInstance, // node instance
                    std::wstring, // activity id
                    std::wstring, // action
                    Common::TraceCorrelatedEventBase
                    );

                DECLARE_RA_STRUCTURED_TRACE(FTJobItemDroppingMessageBecauseRANotReady,
                    Common::Guid, // fuid
                    Federation::NodeInstance, // node instance
                    std::wstring, // activity id
                    std::wstring, // action
                    Common::TraceCorrelatedEventBase
                    );                


                DECLARE_RA_STRUCTURED_TRACE(BackgroundWorkManagerDroppingWork,
                    std::wstring, // id
                    std::wstring, // request activity id
                    Common::StringLiteral // tag
                    );

                DECLARE_RA_STRUCTURED_TRACE(BackgroundWorkManagerProcess,
                    std::wstring, // id
                    std::wstring // activity id
                    );

                DECLARE_RA_STRUCTURED_TRACE(BackgroundWorkManagerOnComplete,
                    std::wstring, // id
                    std::wstring // activity id
                    );

                DECLARE_RA_STRUCTURED_TRACE(MultipleFTWorkBegin,
                    std::wstring, // id
                    std::wstring, // activity id
                    uint64
                    );

                DECLARE_RA_STRUCTURED_TRACE(MultipleFTWorkCompletionCallbackEnqueued,
                    std::wstring, // id
                    std::wstring // activity id
                    );

                DECLARE_RA_STRUCTURED_TRACE(MultipleFTWorkComplete,
                    std::wstring, // id
                    std::wstring  // activity id
                    );

                DECLARE_RA_STRUCTURED_TRACE(MultipleFTWorkLifecycle,
                    std::wstring, // activity id
                    Common::StringLiteral // tag
                    );

                DECLARE_RA_STRUCTURED_TRACE(FTSetOperation,
                    std::wstring, // node instance
                    std::wstring, // activity id
                    std::string, // name
                    bool, // tag
                    uint64 // count
                    );

                DECLARE_RA_STRUCTURED_TRACE(UpgradeStateMachineEnterState,
                    std::wstring, // node instance
                    std::wstring,
                    Upgrade::UpgradeStateName::Trace, // current state
                    Upgrade::UpgradeStateName::Trace // target
                    );

                DECLARE_RA_STRUCTURED_TRACE(UpgradeStateMachineOnTimer,
                    std::wstring, // node instance
                    std::wstring, // activity id
                    Upgrade::UpgradeStateName::Trace, // current state
                    Upgrade::UpgradeStateName::Trace
                    );

                DECLARE_RA_STRUCTURED_TRACE(UpgradeMessageProcessorClose,
                    std::wstring // node instance
                    );

                DECLARE_RA_STRUCTURED_TRACE(UpgradeAsyncOperationStateChange,
                    std::wstring, // node instance
                    std::wstring, // activity id
                    Common::StringLiteral, // tag
                    Upgrade::UpgradeStateName::Trace, // operation name
                    Common::ErrorCode, // error
                    uintptr_t
                    );

                DECLARE_RA_STRUCTURED_TRACE(UpgradeCloseCompletionCheckResult,
                    std::wstring, // node instance
                    std::wstring, // activity id
                    uint64, // still open fts
                    Infrastructure::EntityEntryBaseList, // FTs to be closed
                    Infrastructure::EntityEntryBaseList // FTs to be dropped
                    );

                DECLARE_RA_STRUCTURED_TRACE(UpgradeReplicaDownCompletionCheckResult,
                    std::wstring, // node instance
                    std::wstring, // activity Id
                    uint64, // still pending count
                    std::vector<FailoverUnitId> // pending
                    );

                DECLARE_RA_STRUCTURED_TRACE(FabricUpgradeValidateDecision,
                    std::wstring, // node instance
                    std::wstring, // activity id
                    bool
                    );
                
                DECLARE_RA_STRUCTURED_TRACE(UpgradeLifeCycle,
                    std::wstring, // node instance
                    std::wstring, // activity Id
                    Common::StringLiteral, // tag
                    Common::TraceCorrelatedEventBase);

                DECLARE_RA_STRUCTURED_TRACE(FabricUpgrade,
                    uint16, 
                    std::wstring,
                    bool,
                    ServiceModel::FabricUpgradeSpecification);

                DECLARE_RA_STRUCTURED_TRACE(ApplicationUpgrade,
                    uint16,
                    std::wstring,
                    ServiceModel::ApplicationUpgradeSpecification);

                DECLARE_RA_STRUCTURED_TRACE(UpgradeMessageProcessorAction,
                    std::wstring, // node instance
                    Common::StringLiteral, // source
                    Common::StringLiteral, // tag
                    uint64, // incoming cancel instance id
                    std::wstring, // incoming upgrade message
                    uint64, // current cancel instance id
                    std::wstring,  // current
                    Upgrade::UpgradeStateName::Trace // current state
                    );

                DECLARE_RA_STRUCTURED_TRACE(UpgradeStaleMessage,
                    std::wstring, // node instance
                    std::wstring, // activity id
                    Common::FabricVersionInstance, // fabric version
                    ServiceModel::FabricUpgradeSpecification // upgrade spec
                    );

                DECLARE_RA_STRUCTURED_TRACE(UpgradePrintAnalyzeResult,
                    std::wstring, // node instance
                    std::wstring, // activity id
                    Common::ErrorCode, // error code
                    uint64 // count
                    );

                DECLARE_RA_STRUCTURED_TRACE(ReplicaHealth,
                    Common::Guid, // fuid
                    Federation::NodeInstance, 
                    Health::ReplicaHealthEvent::Trace, // state
                    Common::ErrorCode,
                    uint64,
                    uint64);

                DECLARE_RA_STRUCTURED_TRACE(QueryResponse,
                    std::wstring, // node instance
                    std::wstring, // activity id
                    Common::ErrorCode, // error
                    uint64 // count
                    );

                DECLARE_RA_STRUCTURED_TRACE(DeployedReplicaDetailQueryResponse,
                    std::wstring, // node instance
                    std::wstring, // activity id
                    std::wstring // result
                    );

                // Eventually we shall deprecate this Query channel one in favor of Operational channel.
                DECLARE_RA_STRUCTURED_TRACE(ReconfigurationCompleted,
                    Common::Guid,
                    std::wstring,
                    std::wstring, // node id
                    std::wstring, // ServiceType
                    Reliability::Epoch, 
                    ReconfigurationType::Trace, 
                    ReconfigurationResult::Trace, 
                    Diagnostics::ReconfigurationPerformanceData
                    );

                DECLARE_RA_STRUCTURED_TRACE(ReconfigurationCompletedOperational,
                    Common::Guid, // eventInstanceId
                    Common::Guid, // partitionId
                    std::wstring, // nodeName
                    std::wstring, // node id
                    std::wstring, // ServiceType
                    Reliability::Epoch,
                    ReconfigurationType::Trace,
                    ReconfigurationResult::Trace,
                    Diagnostics::ReconfigurationPerformanceData
                    );

                DECLARE_RA_STRUCTURED_TRACE(ReconfigurationSlow,
                    Common::Guid, // ft id
                    std::wstring, // node instance
                    std::wstring, // reconfiguration phase
                    std::wstring  // details
                    );

                DECLARE_RA_STRUCTURED_TRACE(ResourceUsageReportExclusiveHost,
                    Common::Guid, // partition id
                    int64, // replica id
                    ReplicaRole::Trace, // replica rolerep
                    std::wstring, // node instance
                    double, // cpu usage weighted average
                    int64,  // memory usage weighted average
                    double, // cpu usage raw
                    int64 // memory usage raw
                    );

                DECLARE_RA_STRUCTURED_TRACE(ReplicaStateChange,
                    std::wstring, // node instance id
                    Common::Guid, // partition id
                    int64, // replica id
                    Reliability::Epoch,
                    ReplicaLifeCycleState::Trace,
                    ReplicaRole::Trace,
                    Common::ActivityDescription
                    );

                RAEventSource() :
                    RA_STRUCTURED_TRACE(LifeCycleOpen,                                              4,      "LifeCycle",                Info,       "RA opening", "id"),
                    RA_STRUCTURED_TRACE(LifeCycleOpenFailed,                                        5,      "LifeCycle",                Error,      "RA encountered error opening {1}", "id", "error"),
                    RA_STRUCTURED_TRACE(LifeCycleOpenSuccess,                                       6,      "LifeCycle",                Info,       "RA opened. FTs in LFUM {1}", "id", "ftCount"),
                    RA_STRUCTURED_TRACE(LifeCycleCloseBegin,                                        7,      "LifeCycle",                Info,       "RA closing", "id"),
                    RA_STRUCTURED_TRACE(LifeCycleCloseEnd,                                          8,      "LifeCycle",                Info,       "RA closed", "id"),
                    RA_STRUCTURED_TRACE(LifeCycleRAIgnoringNodeUpAck,                               11,     "LifeCycle",                Info,       "RA not open, ignoring NodeUpAck message", "id"),
                    RA_STRUCTURED_TRACE(LifeCycleNodeUpAckReceived,                                 12,     "LifeCycle",                Info,       "RA on node {0} received NodeUpAck from {1} [Node Version = {2}]: {3}\r\n [ActivityId: {4}]", "id", "source", "nodeVersion", "msg", "activityId"),
                    RA_STRUCTURED_TRACE(LifeCycleNodeUpAckProcessed,                                13,     "LifeCycle",                Info,       "RA on node {0} processed NodeUpAck from {1}", "id", "source"),
                    RA_STRUCTURED_TRACE(LifeCycleNodeActivationStateChange,                         14,     "LifeCycle",                Info,       "RA on node {0} ActivationStateChange. New = {1}. {2}", "id", "activationState", "fmId"),
                    RA_STRUCTURED_TRACE(LifeCycleNodeActivationMessageProcess,                      15,     "LifeCycle",                Info,       "RA on node {0} processing NodeActivation message. Incoming = {1}. Sender = {2}\r\nNode FM State: {3}\r\nNode FMM State: {4}\r\n [ActivityId: {5}]", "id", "incoming", "sender", "fm", "fmm", "activityId"),
                    RA_STRUCTURED_TRACE(LifeCycleNodeUpgradingStateChange,                          16,     "LifeCycle",                Info,       "RA on node {0} Node Upgrading State Change. New = {1}", "id", "value"),
                    RA_STRUCTURED_TRACE(LifeCycleNodeDeactivationProgress,							17,		"LifeCycle",				Info,		"RA on node {0} Node deactivation progress. [ActivityId: {1}]. Pending FTs:\r\n {2}", "id", "activityId", "fts"),
                    RA_STRUCTURED_TRACE(LifeCycleCloseReplicaInProgress,                            18,     "LifeCycle",                Info,       "RA on node {0} Node shutdown progress. [ActivityId: {1}]. Pending FTs:\r\n {2}", "id", "activityId", "fts"),
                    RA_STRUCTURED_TRACE(LifeCycleLastReplicaUpState,                                19,     "LifeCycle",                Info,       "RA on node {0} Last Replica Up State {2}\r\n[ActivityId: {1}]", "id", "state", "activityId"),

                    RA_STRUCTURED_TRACE(GenerationUploadingLfum,                                    20,     "Generation",               Info,       "RA on node {0} returning LFUM for request with generation header {1} [ActivityId: {2}] :\r\n{3}", "id", "header", "activityId", "failoverUnits"),
                    RA_STRUCTURED_TRACE(GenerationIgnoreMessage,                                    21,     "Generation",               Info,       "RA on node {0} dropped {1} message from {2} as incoming generation {3} less than {4}", "id", "action", "fromNodeInstance", "generationHeader", "generations"),
                    RA_STRUCTURED_TRACE(GenerationIgnoreLFUMUploadReply,                            22,     "Generation",               Info,       "RA on node {0} ignored stale LFUM upload reply message, local generation: {1}, incoming: {2}", "id", "generations", "generationHeader"),
                    RA_STRUCTURED_TRACE(GenerationUpdatedSendGeneration,                            23,     "Generation",               Info,       "RA on node {0} processed LFUM upload reply message and updated send generation to {1}, incoming: {2}", "id", "generations", "generationHeader"),
                    RA_STRUCTURED_TRACE(GenerationSet,                                              24,     "Generation",               Info,       "RA on node {0} updated generation to {1}, incoming: {2} \r\n [ActivityId: {3}]", "id", "generations", "generationHeader", "activityId"),
                    RA_STRUCTURED_TRACE(GenerationUpdateRejected,                                   25,     "Generation",               Info,       "RA on node {0} rejected generation update, local generation: {1}, incoming: {2}\r\n [ActivityId: {3}", "id", "generations", "generationHeader", "activityId"),
                    RA_STRUCTURED_TRACE(GenerationUpdate,                                           26,     "Generation",               Info,       "RA on node {0} processed GenerationUpdate message for FM epoch {1} and dropping FT. \r\n [ActivityId: {2}", "id", "epoch", "activityId"),

                    RA_STRUCTURED_TRACE(MessageReceive,                                             30,     "Receive",                  Noise,      "RA on node {0} received message {1} from node {2} with message id {3}", "id", "action", "from", "messageId"),
                    RA_STRUCTURED_TRACE(RAUnknownAction,                                            31,     "Receive",                  Warning,    "RA on node {1} received message with unknown action {0}", "action", "nodeInstance"),
                    RA_STRUCTURED_TRACE(MessageProcessStart,                                        32,     "Receive",                  Noise,      "RA on node {1} start processing message {3} [{2}] with body \r\n{4} FT: \r\n {5}", "id", "nodeInstance", "activityId", "action", "body", "ft"),

                    RA_STRUCTURED_TRACE(RAFailoverUnit,                                             40,     "FT",                       Info,       "{1}\r\n{2}", "contextSequenceId", "detail", "replicas"),
                
                    RA_STRUCTURED_TRACE(ProxyReplyMessageBody,                                      46,     "ProxyReplyMessageBody",    Info,       "{1}\r\n{2}{3}State: {4}. EC: {5}", "contextSequenceId", "fuDesc", "localReplicaDesc", "remoteReplicas", "flags", "proxyErrorCode"),
                    RA_STRUCTURED_TRACE(ProxyRequestMessageBody,                                    47,     "ProxyRequestMessageBody",  Info,       "{1}\r\n{2}{3}{4} Flags: {5}", "contextSequenceId", "fudesc", "replicadesc", "remoteReplicas", "serviceDesc", "flags"),
                    RA_STRUCTURED_TRACE(ProxyUpdateServiceDescriptionReplyMessageBody,              48,     "ProxyUSReplyMessageBody",  Info,       "{1}\r\n{2}\r\n{3} EC: {4}", "contextSequenceId", "fuDesc", "sd", "localReplicaDesc", "errorCode"),


                    RA_STRUCTURED_TRACE(RAReplica,                                                  70,     "Replica",                  Info,       "{1}\r\n", "contextSequenceId", "replica"),

                    RA_STRUCTURED_TRACE(ApplicationUpgradePrintServiceTypeStatus,                   95,     "ServiceTypeUpdate",        Info,       "RA on node {0} Update version to {1} for {2}", "id", "version", "type"),
                    RA_STRUCTURED_TRACE(MessageIgnore,                                              110,    "Ignore",                   Info,       "RA on node {0} ignored message {2}, {1}. Id {3}", "nodeInstance", "tag", "action", "messageId"),
                    RA_STRUCTURED_TRACE(InvalidMessageDropped,                                      118,    "Ignore",                   Warning,    "RA on node {0} dropping message {1} {2} because of a deserialization error {3:x}", "id", "messageId", "action", "error"),
                    RA_STRUCTURED_TRACE(NodeActivationDeactivationMessageDrop,                      119,    "Ignore",                   Info,       "RA on node {0} dropping message {1} [isActivate = {2}] with sequence number {3} because RA is not ready", "id", "messageId", "isActivate", "sequenceNumber"),
                    RA_STRUCTURED_TRACE(NodeActivationDeactivationDropBecauseStale,                 120,    "Ignore",                   Info,       "RA on node {0} dropping message {1} [isActivate = {2}] with sequence number {3} because RA sequence number is {4}", "id", "messageId", "isActivate", "sequenceNumber", "raSequenceNumber"),
                
                    RA_STRUCTURED_TRACE(ServiceType,                                                123,    "ServiceType",              Info,       "RA on node {1} updated service type {0}. Version = {2}. CodePackage = {3}. Count = {4}. RetryPending = {5}", "id", "nodeInstance", "spvi", "cpName", "count", "isPending"),
                    RA_STRUCTURED_TRACE(HostingFindServiceTypeRegistrationCompleted,                124,    "Hosting",                  Noise,      "RA on node {0} find service type registration completed. ServiceType = {1}. Error = {2}. RequestedVersion = {3}. ReturnedVersion = {4}. Host={5}. Runtime={6}", "id", "typeId", "error", "requestedVersion", "actualVersion", "hostId", "runtimeId"),
                    RA_STRUCTURED_TRACE(HostingServiceTypeRegistered,                               125,    "Hosting",                  Info,       "RA on node {0} registered service type: {2}. \r\n [ActivityId: {3}]", "id", "name", "versionedServiceTypeId", "activityId"),
                    RA_STRUCTURED_TRACE(HostingProcessClosedEvent,                                  126,    "Hosting",                  Info,       "RA on node {0} performing {1} processing for apphost: {2}/{3}\r\n [ActivityId: {4}]", "id", "tag", "hostId", "runtimeId", "activityId"),
                    RA_STRUCTURED_TRACE(HostingIgnoreEvent,                                         127,    "Hosting",                  Noise,      "RA on node {0} ignoring hosting event due to {2}\r\n [ActivityId: {1}]", "id", "activityId", "reason"),
                    RA_STRUCTURED_TRACE(HostingFinishEvent,                                         128,    "Hosting",                  Info,      	"RA on node {0} finished hosting event. {2}\r\n [ActivityId: {1}]", "id", "activityId", "perfData"),
                    RA_STRUCTURED_TRACE(HostingProcessServiceTypeEvent,                             129,    "Hosting",                  Info,       "RA on node {0} processed {1} for ServiceType {2} with result {3}. SeqNum = {4}", "id", "updateKind", "serviceTypeId", "result", "sequenceNumber"),
                    RA_STRUCTURED_TRACE(HostingProcessPartitionEvent,                               130,    "Hosting",                  Info,       "RA on node {0} processed {1} for partition {2} with result {3}. SeqNum = {4}", "id", "updateKind", "partitionId", "result", "sequenceNumber"),
                    RA_STRUCTURED_TRACE(HostingSendServiceTypeMessage,                              131,    "Hosting",                  Info,       "RA on node {0} sending {1} message with SeqNum {2}", "id", "tag", "serviceTypeUpdateMessageBody"),
                    RA_STRUCTURED_TRACE(HostingProcessServiceTypeReplyMessage,                      132,    "Hosting",                  Info,       "RA on node {0} processing {1} service type update with action {2}. SeqNum = {3}", "id", "updateKind", "tag", "sequenceNumber"),
                    RA_STRUCTURED_TRACE(HostingIgnoreStaleServiceTypeReplyMessage,                  133,    "Hosting",                  Info,       "RA on node {0} ignored stale service type {1} message: {2}", "id", "updateKind", "tag"),
                    RA_STRUCTURED_TRACE(HostingSendPartitionNotificationMessage,                    134,    "Hosting",                  Info,       "RA on node {0} sending partition notification message with SeqNum {1}", "id", "serviceTypeUpdateMessageBody"),
                    RA_STRUCTURED_TRACE(HostingSendingReplicaUpMessage,                             135,    "ReplicaUp",                Info,       "RA on node {0} sending ReplicaUp message to {1}. Affected Replicas:\r\n{2}\r\n", "id", "tag", "replicas"),
                    RA_STRUCTURED_TRACE(HostingSendingReplicaDownMessage,                           136,    "ReplicaDown",              Info,       "RA on node {0} sending ReplicaDown message. Affected Replicas:\r\n{1}\r\n", "id", "replicas"),
                    RA_STRUCTURED_TRACE(HostingTerminateServiceHost,                                137,    "Hosting",                  Warning,    "RA on node {0} terminating Host {1} PID = {2} due to {3}\r\n [ActivityId: {4}]", "id", "hostId", "pid", "reason", "activityId"),

                    RA_STRUCTURED_TRACE(UpgradeStateMachineEnterState,                              140,    "StateMachineUpgrade",      Noise,      "RA on node {0} USM: Enter - Current = ({2}). Target = ({3}) [Activity: {1}]", "id", "activityId", "current", "target"),
                    RA_STRUCTURED_TRACE(UpgradeStateMachineOnTimer,                                 141,    "StateMachineUpgrade",      Info,       "RA on node {0} USM: OnTimer {2}. Transitioning to: {3} [Activity: {1}]", "id", "activity", "state", "targetState"),
                    RA_STRUCTURED_TRACE(UpgradeMessageProcessorClose,                               142,    "Upgrade",                  Noise,      "RA on node {0} UpgradeMessageProcessor:  Close", "id"),
                    RA_STRUCTURED_TRACE(UpgradeAsyncOperationStateChange,                           143,    "Upgrade",                  Info,       "RA on node {0} Upgrade has {2} async operation in state {3} op={5:x} Error = {4}.\r\n[Activity: {1}]", "id", "activity", "tag", "state", "error", "handle"),
                    RA_STRUCTURED_TRACE(UpgradeCloseCompletionCheckResult,                          145,    "Upgrade",                  Info,       "RA on node {0} Upgrade close completion check. {2} fts pending [Activity: {1}]. \r\n To Be Closed FT List: {3}\r\n To Be Dropped FT List: {4} \r\n", "id", "activity", "openFTCount", "ftsClosed", "ftsDropped"),
                    RA_STRUCTURED_TRACE(UpgradeReplicaDownCompletionCheckResult,                    146,    "Upgrade",                  Info,       "RA on node {0} Upgrade Replica Down Completion Check. {2} fts pending [Activity: {1}]. \r\n To Be Ackd: {3}\r\n", "id", "activity", "openCount", "pending"),
                    RA_STRUCTURED_TRACE(FabricUpgradeValidateDecision,                              147,    "Upgrade",                  Info,       "RA on node {0} Validate Decision: shouldCloseReplicas = {2} [Activity: {1}]", "id", "activity", "shouldCloseReplicas"),
                    RA_STRUCTURED_TRACE(UpgradePrintAnalyzeResult,                                  148,    "Upgrade",                  Info,       "RA on node {0} Upgrade Analyze from Hosting [ActivityId: {1}]. EC: {2}. Count: {3}", "id", "activityId", "error", "count"),
                    RA_STRUCTURED_TRACE(UpgradeLifeCycle,                                           149,    "StateMachineUpgrade",      Noise,      "RA on node {0} Upgrade is {2}\r\n {3} \r\n [ActivityId: {1}]", "id", "aid", "tag", "upgrade"),
                    RA_STRUCTURED_TRACE(FabricUpgrade,                                              150,    "Upgrade",                  Info,       "FabricUpgrade (ActivityId: {3}). IsNodeUp = ({1}). Specification = [{2}]", "contextSequenceId", "isNodeUp", "spec", "aid"),
                    RA_STRUCTURED_TRACE(ApplicationUpgrade,                                         151,    "Upgrade",                  Info,       "ApplicationUpgrade (ActivityId: {2}). Specification = [{1}]", "contextSequenceId", "spec", "aid"),
                    RA_STRUCTURED_TRACE(UpgradeStaleMessage,                                        152,    "Upgrade",                  Info,       "RA on node {0} dropping stale upgrade message. Current Fabric Version = ({2}). Message: {3}\r\n[Activity: {1}]", "id", "activityId", "fabricversion", "message"),
                    RA_STRUCTURED_TRACE(UpgradeMessageProcessorAction,                              153,    "Upgrade",                  Info,       "RA on node {0} Request Type: {1}. Action: {2} \r\n[Incoming]\r\nCancel Instance Id: {3}\r\nContext: {4}\r\n[Previous]\r\nInstance Id: {5}\r\nState: {7}\r\nContext: {6}", "id", "source", "tag", "incomingCancel", "incomingUpgrade", "currentCancel", "currentUpgrade", "currentState"),

                    RA_STRUCTURED_TRACE(ReportFaultCannotProcessDueToNotReady,                      158,    "ReportFault",              Noise,      "RA on node {1} cannot process ReportFault as FT not ready: {2}", "id", "nodeInstance", "rafu"),
                    RA_STRUCTURED_TRACE_QUERY(ReportFaultProcessingBegin,                           159,    "_Api_ReportFault",         Info,       "RA on node {1} processing ReportFault: PartitionId = {2}. ReplicaId = {3}. FaultType = {4}. IsForce = {5}. ReasonActivityDescription = {6}.", "id", "nodeInstance", "partitionId", "replicaId", "faultType", "isForce", "reasonActivityDescription"),

                    RA_STRUCTURED_TRACE(RetryTimerCreate,                                           160,    "RetryTimer",               Info,       "Created with interval {1}", "id", "timespan"),
                    RA_STRUCTURED_TRACE(RetryTimerTryCancel,                                        161,    "RetryTimer",               Noise,       "TryCancel. Passed in SeqNum {1}. Timer SeqNum {2}", "id", "inSeqNum", "curSeqNum"),
                    RA_STRUCTURED_TRACE(RetryTimerFire,                                             162,    "RetryTimer",               Noise,      "Retry Timer event", "id"),
                    RA_STRUCTURED_TRACE(RetryTimerSetCalled,                                        163,    "RetryTimer",               Noise,       "Retry timer set called, SeqNum: {1}. Result: {2}", "id", "curSeqNum", "tag"),
                    RA_STRUCTURED_TRACE(RetryTimerClosed,                                           165,    "RetryTimer",               Info,       "Closed", "id"),

                    RA_STRUCTURED_TRACE(FMMessageRetry,                                             170,    "FMMessageRetry",           Info,       "RA on node {0} [ActivityId = {1}] FMMessageRetry {2}. Data:\r\n{3}", "id", "activityId", "name", "data"),

                    RA_STRUCTURED_TRACE(ServiceLocationUpdate,                                      180,    "Service_Endpoint",         Noise,      "RA on node {1} updated service endpoint for {2}:{3} \r\n [New] {4}\r\n [Old] {5}", "id", "nodeinst", "replicaId", "instanceId", "new", "old"),
                    RA_STRUCTURED_TRACE(ReplicatorLocationUpdate,                                   181,    "Replicator_Endpoint",      Noise,      "RA on node {1} updated replicator endpoint for {2}:{3} \r\n [New] {4}\r\n [Old] {5}", "id", "nodeinst", "replicaId", "instanceId", "new", "old"),

                    RA_STRUCTURED_TRACE(SendMessageToFM,                                            185,    "SendFM",                   Info,       "RA on node {1} sending message {3} to FM, body:\r\n {4}\r\n[Activity: {2}]", "id", "inst", "activityId", "action", "body"),
                    RA_STRUCTURED_TRACE(SendMessageToFMM,                                           186,    "SendFMM",                  Info,       "RA on node {1} sending message {3} to FMM, body:\r\n {4}\r\n[Activity: {2}]", "id", "inst", "activityId", "action", "body"),
                    RA_STRUCTURED_TRACE(SendMessageToRA,                                            187,    "SendRA",                   Info,       "RA on node {1} sending message {3} to node {4}, body:\r\n{5}\r\n\r\n[Activity: {2}]", "id", "inst", "activityId", "action", "target", "body"),
                    RA_STRUCTURED_TRACE(SendMessageToRAP,                                           188,    "SendRAP",                  Info,       "RA on node {1} sending message {3} to host {4} [PID = {5}], body:\r\n{6}\r\n\r\n[Activity: {2}]", "id", "inst", "activityId", "action", "target", "targetPID", "body"),
                    RA_STRUCTURED_TRACE(SendNonFTMessageToFM,                                       189,    "NonFT_SendFM",             Info,       "RA on node {0} sending message {2} to FM ({3}), body:\r\n {4}\r\n[Activity: {1}]", "id", "activityId", "action", "target", "body"),

                    RA_STRUCTURED_TRACE(EntityJobItemTraceInformation,                              200,    "EntityTraceInfo",          Info,       "{1}{2} {3}\r\n", "contextSequenceId", "metadata", "name", "activityId"),

                    RA_STRUCTURED_TRACE(EntityDeleted,                                              201,    "EntityDeleted",            Info,       "RA on node {1} deleted entity. ErrorCode: {2}. Activities: {3}", "id", "nodeInstance", "ec", "activities"),

                    RA_STRUCTURED_TRACE(FTUpdate,                                                   202,    "FTUpdate",                 Info,       "RA on node {1} updated \r\n{2}\r\n\r\nActivities:\r\n{3}\r\n\r\nEC: {4} {5} {6} {7}", "id", "nodeInstance", "state", "activities", "error", "scheduleResult", "executeResult", "commitResult"),
                    RA_STRUCTURED_TRACE(ReplicaUpReplyAction,                                       203,    "ReplicaUpReply_FTInput",   Info,       "RA on node {1} processed ReplicaUpReply IsInDroppedList = {3}. FTDesc: \r\n{4}\r\n{5}\r\n\r\n[ActivityId: {2}]", "id", "nodeInstance", "activityId", "droppedList", "ftDesc", "replicas"),
                    RA_STRUCTURED_TRACE(MessageAction,                                              204,    "Message_FTInput",          Info,       "RA on node {1} processed {3}\r\n{4}\r\n\r\n[Activity: {2}]", "id", "nodeInstance", "activityId", "contents", "aid"),

                    RA_STRUCTURED_TRACE(FTJobItemDroppingMessageBecauseEntryNotFound,               216,    "Ignore",                   Info,       "RA on node {1} dropped message {3} because entry not found\r\n{4}\r\n[Activity: {2}]", "id", "nodeInstance", "aid", "action", "message"),
                    RA_STRUCTURED_TRACE(FTJobItemDroppingMessageBecauseRANotReady,                  217,    "Ignore",                   Info,       "RA on node {1} dropped message {3} because RA Not ready \r\n{4}\r\n[Activity: {2}]", "id", "nodeInstance", "aid", "action", "message"),                
                    RA_STRUCTURED_TRACE(BackgroundWorkManagerDroppingWork,                          235,    "BGM",                      Noise,      "BGM dropping Activity: {1} as {2}", "id", "request", "reason"),
                    RA_STRUCTURED_TRACE(BackgroundWorkManagerProcess,                               236,    "BGM",                      Noise,      "BGM Process begin [Activity: {1}]", "id", "activity"),
                    RA_STRUCTURED_TRACE(BackgroundWorkManagerOnComplete,                            237,    "BGM",                      Noise,      "BGM Work complete [Activity: {1}]", "id", "activity"),
                    RA_STRUCTURED_TRACE(MultipleFTWorkBegin,                                        240,    "MultipleFT",               Info,       "Multiple FT Work Begin [Activity: {1}]. FTs {2}", "id", "activity", "ft count"),
                    RA_STRUCTURED_TRACE(MultipleFTWorkCompletionCallbackEnqueued,                   241,    "MultipleFT",               Info,       "Multiple FT Work Callback Enqueued [Activity: {1}]", "id", "activity"),
                    RA_STRUCTURED_TRACE(MultipleFTWorkComplete,                                     242,    "MultipleFT",               Info,       "Multiple FT Work End [Activity: {1}]", "id", "activity"),
                    RA_STRUCTURED_TRACE(MultipleFTWorkLifecycle,                                    243,    "MultipleFT",               Noise,      "Multiple FT Work {0} {1}", "activityId", "tag"),
                    RA_STRUCTURED_TRACE(FTSetOperation,                                             245,    "Set",                      Noise,      "Set {0} Partition {2} isAdd = {3}. Size = {4} [Activity: {1}]", "id", "activityId", "ftId", "tag", "size"),
                    RA_STRUCTURED_TRACE(ReplicaHealth,                                              246,    "Health",                   Noise,      "RA on node {1} report Health {2}. EC = {3}. ReplicaId = {4}. ReplicaInstanceId = {5}.", "id", "nodeInstance", "state", "error", "replicaId", "replicaInstanceId"),
                    RA_STRUCTURED_TRACE(QueryResponse,                                              247,    "Query",                    Info,       "RA on node {0} sent query response. EC: {2}. Result Count: {3}\r\n[Activity: {1}] ", "id", "activityId", "ec", "count"),
                    RA_STRUCTURED_TRACE(DeployedReplicaDetailQueryResponse,                         248,    "Query",                    Info,       "RA on node {0} sent query response {2}\r\n[Activity: {1}]", "id",  "activityId", "result"),
                    RA_STRUCTURED_TRACE(ReconfigurationSlow,                                        249,    "ReconfigurationSlow",      Warning,    "RA on node {1} detected slow reconfiguration. Phase = {2}. Description = {3}", "id", "nodeInstance", "reconfigurationPhase", "healthReportDescription"),
                    RA_STRUCTURED_TRACE_QUERY(ReconfigurationCompleted,                             250, "_Reconfig_ReconfigurationCompleted", Info, "RA on node {1}({2}) completed reconfiguration. PartitionId = {0}. ServiceType = {3}. Epoch = {4}. ReconfigurationType = {5}. Result = {6}. {7}", "id", "nodeName", "nodeInstance_id", "serviceType", "ccEpoch", "reconfigType", "result", "stats"),
                    RA_STRUCTURED_TRACE(ResourceUsageReportExclusiveHost,                           251,    "ResourceMonitor",          Info,        "Resource usage for replicaId = {1} with role = {2} on node = {3} weighted average cpu usage = {4} cores weighted average memory usage = {5} MB raw cpu usage {6} cores raw memory usage {7} MB", "id", "replicaId", "replicaRole", "nodeInstance", "cpuUsageWeightedAverage", "memoryUsageWeightedAverage", "cpuUsageRaw", "memoryUsageRaw"),
                    RA_STRUCTURED_TRACE_OPERATIONAL(ReconfigurationCompletedOperational,            L"PartitionReconfigured", OperationalStateTransitionCategory, 252, "_PartitionsOps_ReconfigurationCompleted", Info, "EventInstanceId: {0}. RA on node {2}({3}) completed reconfiguration. PartitionId = {1}. ServiceType = {4}. Epoch = {5}. ReconfigurationType = {6}. Result = {7}. {8}", "eventInstanceId", "partitionId", "nodeName", "nodeInstance_id", "serviceType", "ccEpoch", "reconfigType", "result", "stats"),
                    RA_STRUCTURED_TRACE_QUERY(ReplicaStateChange,                                   253, "_Api_ReplicaStateChange",       Info,       "RA on node {0} closing replica with: PartitionId = {1}. ReplicaId = {2}. Epoch = {3}. Status = {4}. Role = {5}. ReasonActivityDescription = {6}.", "nodeInstance_id", "partitionId", "replicaId", "epoch", "status", "role", "reasonActivityDescription")
                {
                }

            public:
                static Common::Global<RAEventSource> Events;

                void TraceEventData(Diagnostics::IEventData const&& eventData, std::wstring const & nodeName);

            };
        }
    }
}
