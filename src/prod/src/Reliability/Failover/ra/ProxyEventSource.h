// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        #define DECLARE_RAP_STRUCTURED_TRACE(trace_name, ...) Common::TraceEventWriter<__VA_ARGS__> trace_name
        #define RAP_STRUCTURED_TRACE(trace_name, trace_id, trace_type, trace_level, ...) \
            trace_name( \
                Common::TraceTaskCodes::RAP, \
                trace_id, \
                #trace_name "_" trace_type, \
                Common::LogLevel::trace_level, \
                __VA_ARGS__)

        #define RAP_STRUCTURED_TRACE_QUERY(trace_name, trace_id, trace_type, trace_level, ...) \
            trace_name( \
                Common::TraceTaskCodes::RAP, \
                trace_id, \
                trace_type, \
                Common::LogLevel::trace_level, \
                Common::TraceChannelType::Debug, \
                TRACE_KEYWORDS2(Default, ForQuery), \
                __VA_ARGS__)

        class RAPEventSource
        {
            public:

                DECLARE_RAP_STRUCTURED_TRACE(UpdateAccessStatus,
                    Common::Guid, // id - fuid
                    ReconfigurationAgentProxyId,
                    int64, // replicaId
                    AccessStatus::Trace, // read status
                    AccessStatus::Trace // write status
                    );

                DECLARE_RAP_STRUCTURED_TRACE(FUPPreTrace,
                    Common::Guid,
                    ReconfigurationAgentProxyId,
                    Common::ApiMonitoring::InterfaceName::Trace,
                    Common::ApiMonitoring::ApiName::Trace,
                    Reliability::ReconfigurationAgentComponent::FailoverUnitProxy
                    );

                DECLARE_RAP_STRUCTURED_TRACE(FUPPreTraceWithData,
                    Common::Guid,
                    ReconfigurationAgentProxyId,
                    Common::ApiMonitoring::InterfaceName::Trace,
                    Common::ApiMonitoring::ApiName::Trace,
                    Common::TraceCorrelatedEventBase,
                    Reliability::ReconfigurationAgentComponent::FailoverUnitProxy);

                DECLARE_RAP_STRUCTURED_TRACE(LifeCycleDestructor,                    
                    ReconfigurationAgentProxyId
                    );

                DECLARE_RAP_STRUCTURED_TRACE(LifeCycleOpened,
                    ReconfigurationAgentProxyId
                    );              

                DECLARE_RAP_STRUCTURED_TRACE(LifeCycleClosing,
                    ReconfigurationAgentProxyId
                    );              

                DECLARE_RAP_STRUCTURED_TRACE(LifeCycleAborting,
                    ReconfigurationAgentProxyId
                    ); 

                DECLARE_RAP_STRUCTURED_TRACE(MessageDropNotOpen,
                    ReconfigurationAgentProxyId,
                    std::wstring  // action
                    );

                DECLARE_RAP_STRUCTURED_TRACE(MessageDropEnqueueFailed,
                    ReconfigurationAgentProxyId,
                    std::wstring // action
                    );

                DECLARE_RAP_STRUCTURED_TRACE(MessageDropInvalidWorkInProgress,
                    ReconfigurationAgentProxyId,
                    std::wstring // action
                    );

                DECLARE_RAP_STRUCTURED_TRACE(MessageDropStale,
                    ReconfigurationAgentProxyId,
                    std::wstring, // action
                    Common::TraceCorrelatedEventBase, // body
                    Reliability::ReconfigurationAgentComponent::FailoverUnitProxy
                    );

                DECLARE_RAP_STRUCTURED_TRACE(CancelPendingOperations,
                    ReconfigurationAgentProxyId,
                    std::wstring // action
                    );

                DECLARE_RAP_STRUCTURED_TRACE(RAPFailoverUnitContext,
                    Common::Guid, // id - fuid
                    ReconfigurationAgentProxyId,
                    std::wstring, // action
                    Common::TraceCorrelatedEventBase, // body
                    Reliability::ReconfigurationAgentComponent::FailoverUnitProxy // post state
                    );

               DECLARE_RAP_STRUCTURED_TRACE(ProxyMessageReceived,
                    ReconfigurationAgentProxyId,
                    std::wstring,  // action
                    std::wstring);

                DECLARE_RAP_STRUCTURED_TRACE(ReplicaProxy,
                    uint16, // context seq id
                    ReplicaProxyStates::Trace, // failover unit proxy state
                    ReplicaProxyStates::Trace, // replicator state
                    char, 
                    Common::DateTime, // state update time
                    Reliability::ReplicaDescription
                    );

                DECLARE_RAP_STRUCTURED_TRACE(OperationManagerCancelOperations,
                    std::string // operations
                    );

                DECLARE_RAP_STRUCTURED_TRACE(ActionListStart,
                    Common::Guid, // id - fuid
                    ReconfigurationAgentProxyId,
                    ProxyActionsListTypes::Trace // action list name
                    );

                DECLARE_RAP_STRUCTURED_TRACE(ActionListEnd,
                    Common::Guid, // id - fuid
                    ReconfigurationAgentProxyId,
                    ProxyActionsListTypes::Trace // actions                    
                    );

                DECLARE_RAP_STRUCTURED_TRACE(Action,
                    Common::Guid, // id - fuid
                    ReconfigurationAgentProxyId,
                    ProxyActions::Trace, // action 
                    ProxyActionsListTypes::Trace // action list name
                    );

                DECLARE_RAP_STRUCTURED_TRACE(ActionFailure,
                    Common::Guid, // id - fuid
                    ReconfigurationAgentProxyId,
                    ProxyActions::Trace, // action 
                    std::wstring, // error
                    ProxyActionsListTypes::Trace // action list name
                    );

                DECLARE_RAP_STRUCTURED_TRACE(ActionDataLoss,
                    Common::Guid, // id - fuid
                    ReconfigurationAgentProxyId,
                    Reliability::Epoch, // pcEpoch
                    Reliability::Epoch // ccEpoch
                    );

                DECLARE_RAP_STRUCTURED_TRACE(CloseServiceReplicatorRoleMismatch,
                    Common::Guid, // id - fuid
                    ReconfigurationAgentProxyId
                    );

                DECLARE_RAP_STRUCTURED_TRACE(CloseExecutingNextStep,
                    Common::Guid, // id - fuid
                    ReconfigurationAgentProxyId,
                    int // next step
                    );

                DECLARE_RAP_STRUCTURED_TRACE(CloseMarkingFinished,
                    Common::Guid, // id - fuid
                    ReconfigurationAgentProxyId
                    );

                DECLARE_RAP_STRUCTURED_TRACE(CloseOperationFailed,
                    Common::Guid, // id - fuid
                    ReconfigurationAgentProxyId,
                    Common::StringLiteral, // operation
                    int // err
                    );

                DECLARE_RAP_STRUCTURED_TRACE(FailoverUnitProxyDestructor,
                    Common::Guid, // id - fuid
                    ReconfigurationAgentProxyId,
                    int64, // replica description
                    int64 // instance id
                    );

                DECLARE_RAP_STRUCTURED_TRACE(FailoverUnitProxyAbort,
                    Common::Guid, // id - fuid
                    ReconfigurationAgentProxyId,
                    Common::StringLiteral // tag (replica/instance)
                    );

                DECLARE_RAP_STRUCTURED_TRACE(FailoverUnitProxyAbortServiceIsNull,
                    Common::Guid, // id - fuid
                    ReconfigurationAgentProxyId,
                    Common::StringLiteral // tag (replica/instance)
                    );
                    
                DECLARE_RAP_STRUCTURED_TRACE(FailoverUnitProxyCancel,
                    Common::Guid, // id - fuid
                    ReconfigurationAgentProxyId,
                    std::wstring  // current
                    );     

                DECLARE_RAP_STRUCTURED_TRACE(FailoverUnitProxyInvalidStateForReportLoad,
                    Common::Guid, // id - fuid
                    ReconfigurationAgentProxyId,
                    FailoverUnitProxyStates::Trace // current state
                    );     

                DECLARE_RAP_STRUCTURED_TRACE(FailoverUnitProxyInvalidStateForReportFault,
                    Common::Guid, // id - fuid
                    ReconfigurationAgentProxyId,
                    FailoverUnitProxyStates::Trace // current state
                    );     

                DECLARE_RAP_STRUCTURED_TRACE(SFPartitionLeaseExpiredStatus,
                    Common::StringLiteral,
                    AccessStatus::Trace,
                    AccessStatus::Trace
                    );

                DECLARE_RAP_STRUCTURED_TRACE(SFPartitionCreatingReplicator,
                    int64
                    );

                DECLARE_RAP_STRUCTURED_TRACE(SFPartitionInvalidReplica,
                    int64
                    );

                DECLARE_RAP_STRUCTURED_TRACE(SFPartitionReplicaAlreadyOpen,
                    int64,
                    bool,
                    bool
                    );

                DECLARE_RAP_STRUCTURED_TRACE(SFPartitionCouldNotGetFUP,
                    int64
                    );

                DECLARE_RAP_STRUCTURED_TRACE(SFPartitionFailedToCreateReplicator,
                    int64,
                    int
                    );

                DECLARE_RAP_STRUCTURED_TRACE(LFUPM,
                    uint16, // context seq id
                    ReconfigurationAgentProxyId,
                    size_t, 
                    size_t,
                    size_t
                    );

                DECLARE_RAP_STRUCTURED_TRACE(LoadReportingOpen,
                    ReconfigurationAgentProxyId,
                    size_t
                    );

                DECLARE_RAP_STRUCTURED_TRACE(LoadReportingClose,
                    ReconfigurationAgentProxyId,
                    size_t
                    );

                DECLARE_RAP_STRUCTURED_TRACE(LoadReportingSetTimer,
                    ReconfigurationAgentProxyId,
                    size_t
                    );

                DECLARE_RAP_STRUCTURED_TRACE(LoadReportingUpdateTimer,
                    ReconfigurationAgentProxyId,
                    size_t
                    );

                DECLARE_RAP_STRUCTURED_TRACE(LoadReportingReportCallback,
                    ReconfigurationAgentProxyId,
                    size_t
                    );

                DECLARE_RAP_STRUCTURED_TRACE(LoadReportingAddLoad,
                    ReconfigurationAgentProxyId,
                    size_t, 
                    Common::Guid, // fuid
                    ReplicaRole::Trace, // replicarole
                    std::wstring, // metricname
                    uint          // metricvalue
                    );

                DECLARE_RAP_STRUCTURED_TRACE(LoadReportingInvalidLoad,
                    ReconfigurationAgentProxyId,
                    Common::Guid, // fuid
                    ReplicaRole::Trace // replicarole
                    );

                DECLARE_RAP_STRUCTURED_TRACE(LoadReportingRemoveLoad,
                    ReconfigurationAgentProxyId,
                    size_t, 
                    Common::Guid // fuid
                    );

                DECLARE_RAP_STRUCTURED_TRACE(MessageSenderOpen,
                    ReconfigurationAgentProxyId
                    );

                DECLARE_RAP_STRUCTURED_TRACE(MessageSenderClose,
                    ReconfigurationAgentProxyId,
                    size_t
                    );

                DECLARE_RAP_STRUCTURED_TRACE(MessageSenderSetTimer,
                    ReconfigurationAgentProxyId,
                    size_t
                    );

                DECLARE_RAP_STRUCTURED_TRACE(MessageSenderUpdateTimer,
                    ReconfigurationAgentProxyId,
                    size_t
                    );

                DECLARE_RAP_STRUCTURED_TRACE(MessageSenderCallback,
                    ReconfigurationAgentProxyId,
                    size_t
                    );

                DECLARE_RAP_STRUCTURED_TRACE(MessageSenderAdd,
                    ReconfigurationAgentProxyId,
                    size_t
                    );

                DECLARE_RAP_STRUCTURED_TRACE(MessageSenderSend,
                    ReconfigurationAgentProxyId,
                    std::wstring  // action
                    );

                DECLARE_RAP_STRUCTURED_TRACE(MessageSenderRemove,
                    ReconfigurationAgentProxyId,
                    std::wstring  // action
                    );

                DECLARE_RAP_STRUCTURED_TRACE(LFUPMTimerChange,
                    Common::StringLiteral, // tag
                    Reliability::ReconfigurationAgentComponent::LocalFailoverUnitProxyMap 
                    );

                DECLARE_RAP_STRUCTURED_TRACE(FailoverUnitProxyEndpointUpdate,
                    Common::Guid, // fuid
                    ReconfigurationAgentProxyId,
                    Common::ApiMonitoring::InterfaceName::Trace, // tag
                    std::wstring // value
                    );

                DECLARE_RAP_STRUCTURED_TRACE(ApiReportFault,
                    Common::Guid, // fuid
                    std::wstring, // node id
                    int64, // replica id
                    int64, // replica instance
                    FaultType::Trace,
                    Common::ActivityDescription);

                DECLARE_RAP_STRUCTURED_TRACE(LoadReportingUndefinedMetric,
                        std::wstring, // metric name
                        Common::Guid // fuid
                        );

                RAPEventSource() :
                    RAP_STRUCTURED_TRACE(UpdateAccessStatus,                                                4,      "UpdateAccessStatus",                                   Info,           "RAP on {1} updated access status status for replica {2}. Read = {3}. Write = {4}", "id", "rapId", "replicaId", "readStatus", "writeStatus"),

                    RAP_STRUCTURED_TRACE(FUPPreTrace,                                                       10,     "Pre_FT",                                               Noise,          "RAP on {1} processing action {2}.{3}. FT:\r\n{4}", "id", "rapId", "interface", "api", "ft"),
                    RAP_STRUCTURED_TRACE(FUPPreTraceWithData,                                               11,     "PreWithBody_FT",                                       Noise,          "RAP on {1} processing action {2}.{3}. Input:\r\n{4}\r\nFT:\r\n{5}", "id", "rapId", "interface", "api", "input", "ft"),

                    RAP_STRUCTURED_TRACE(LifeCycleDestructor,                                               60,     "LifeCycle",                                            Info,           "RAP on {0}: dtor", "rapId"),
                    RAP_STRUCTURED_TRACE(LifeCycleOpened,                                                   61,     "LifeCycle",                                            Info,           "RAP on {0}: opened", "rapId"),
                    RAP_STRUCTURED_TRACE(LifeCycleAborting,                                                 62,     "LifeCycle",                                            Info,           "RAP on {0}: is aborting", "rapId"),
                    RAP_STRUCTURED_TRACE(LifeCycleClosing,                                                  63,     "LifeCycle",                                            Info,           "RAP on {0}: is closing", "rapId"),

                    RAP_STRUCTURED_TRACE(ProxyMessageReceived,                                              74,     "ProxyMessage",                                         Noise,          "RAP on {0} received proxy message {1} {2}", "rapId", "action", "messageId"),
                    RAP_STRUCTURED_TRACE(MessageDropNotOpen,                                                75,     "MessageDropNotOpen_Drop",                              Info,           "RAP on {0} dropped message {1} - not open", "rapId", "action"),
                    RAP_STRUCTURED_TRACE(MessageDropEnqueueFailed,                                          76,     "MessageDropEnqFailed_Drop",                            Warning,        "RAP on {0} dropped message {1} - enqueue failed", "rapId", "action"),
                    RAP_STRUCTURED_TRACE(MessageDropInvalidWorkInProgress,                                  77,     "MessageDropInvalidWork_Drop",                          Noise,          "RAP on {0} dropped message {1} - incompatible in-progress work on the corresponding FUP", "rapId", "action"),
                    RAP_STRUCTURED_TRACE(MessageDropStale,                                                  81,     "MessageDropStale_Drop",                                Noise,          "RAP on {0} ignoring stale message {1}: {2} for {3}", "rapId", "action", "body", "fup"),
                    RAP_STRUCTURED_TRACE(CancelPendingOperations,                                           82,     "CancelPendingOperations",                              Info,           "RAP on {0} canceling pending operations before proceeding with {1} ", "rapId", "action"),

                    RAP_STRUCTURED_TRACE(RAPFailoverUnitContext,                                            89,     "FT",                                                   Info,           "RAP on {1} processed message {2}:\r\n{3}\r\n[New] {4}", "id", "rapId", "action", "body", "post"),
                    RAP_STRUCTURED_TRACE(ReplicaProxy,                                                      95,     "ReplicaProxy",                                         Info,           "{1}:{2} {3} {4} {5}", "contextSequenceId", "fupstate", "replicatorstate", "buildIdle", "stateupdatetime", "replicaDescription"),
                    RAP_STRUCTURED_TRACE(OperationManagerCancelOperations,                                  98,     "OperationManagerCancelOperations",                     Info,           "({0})", "outstanding"),
                    
                    RAP_STRUCTURED_TRACE(ActionListStart,                                                   105,    "ActionListStart",                                      Noise,          "RAP on {1}: Starting Action list: Action List Type:{2}", "id", "rapId", "typeName"),
                    RAP_STRUCTURED_TRACE(ActionListEnd,                                                     106,    "ActionListEnd",                                        Noise,          "RAP on {1}: Completed action list: {2}", "id", "rapId", "actions"),
                    RAP_STRUCTURED_TRACE(Action,                                                            107,    "ActionExecute_Action",                                 Noise,          "RAP on {1}: Executing action: {2}, action list: {3}", "id", "rapId", "action", "list"),
                    RAP_STRUCTURED_TRACE(ActionFailure,                                                     108,    "ActionFailure_Action",                                 Noise,          "RAP on {1}: Encountered a failure executing action: {2}, ErrorCode = {3}, list = {4}", "id", "rapId", "action", "err", "list"),
                    RAP_STRUCTURED_TRACE(ActionDataLoss,                                                    109,    "ActionDataLoss_Action",                                Noise,          "RAP on {1}: ReportAnyDataLoss: There was data loss (PC Epoch = {2}, CC Epoch = {3}) and this is the primary so reporting to the replicator", "id", "rapId", "pcEpoch", "ccEpoch"),

                    RAP_STRUCTURED_TRACE(CloseServiceReplicatorRoleMismatch,                                115,    "CloseRoleMismatch_CloseOp",                            Info,           "RAP on {1}: FailoverUnitProxy::CloseAsyncOperation::ProcessActions: Service/Replicator role mismatch", "id", "rapId"),
                    RAP_STRUCTURED_TRACE(CloseExecutingNextStep,                                            116,    "CloseExecutingNextStep_CloseOp",                       Info,           "RAP on {1}: FailoverUnitProxy::CloseAsyncOperation::ProcessActions: Execuing step {2}", "id", "rapId", "step"),
                    RAP_STRUCTURED_TRACE(CloseMarkingFinished,                                              117,    "CloseMarkingFinished_CloseOp",                         Info,           "RAP on {1}: FailoverUnitProxy::CloseAsyncOperation::ProcessActions: Marking finished", "id", "rapId"),
                    RAP_STRUCTURED_TRACE(CloseOperationFailed,                                              118,    "CloseOperationFailed_CloseOp",                         Info,           "RAP on {1}: FailoverUnitProxy::CloseAsyncOperation::{2} failed {3}", "id", "rapId", "operationTag", "err"),

                    RAP_STRUCTURED_TRACE(FailoverUnitProxyDestructor,                                       145,    "FailoverUnitProxy_Destructor",                         Noise,          "RAP on {1}: FailoverUnitProxy::Dtor for replica {2}:{3}", "id", "rapId", "replicaid", "instanceid"),
                    RAP_STRUCTURED_TRACE(FailoverUnitProxyAbort,                                            147,    "FailoverUnitProxy_Abort",                              Noise,          "RAP on {1}: FailoverUnitProxy::Abort{2}", "id", "rapId", "tag"),
                    RAP_STRUCTURED_TRACE(FailoverUnitProxyAbortServiceIsNull,                               148,    "FailoverUnitProxyServiceNull_Abort",                   Noise,          "RAP on {1}: FailoverUnitProxy::Abort{2} service is null", "id", "rapId", "tag"),
                    RAP_STRUCTURED_TRACE(FailoverUnitProxyCancel,                                           155,    "FailoverUnitProxy_Cancel",                             Info,           "RAP on {1}: Drain actions {2}", "id", "rapId", "actions"),
                    RAP_STRUCTURED_TRACE(FailoverUnitProxyInvalidStateForReportLoad,                        156,    "FailoverUnitProxy_ReportLoad",                         Info,           "RAP on {1}: FailoverUnitProxy::ReportLoad invalid state. current {2}", "id", "rapId", "state"),
                    RAP_STRUCTURED_TRACE(FailoverUnitProxyInvalidStateForReportFault,                       158,    "FailoverUnitProxy_ReportFault",                        Info,           "RAP on {1}: FailoverUnitProxy::ReportFault invalid state. current {2}", "id", "rapId", "state"),

                    RAP_STRUCTURED_TRACE(SFPartitionLeaseExpiredStatus,                                     165,    "SFP_LeaseExpired_ComStatefulServicePartition",         Info,           "Lease expired, reporting {0} status as {1} instead of current status: {2}", "tag", "s1", "s2"),
                    RAP_STRUCTURED_TRACE(SFPartitionCreatingReplicator,                                     166,    "SFP_CreateReplicator_ComStatefulServicePartition",     Noise,          "Creating replicator for replica {0}", "replicaId"),
                    RAP_STRUCTURED_TRACE(SFPartitionInvalidReplica,                                         167,    "SFP_InvalidReplica_ComStatefulServicePartition",       Noise,          "Replica {0} is not valid", "replicaId"),
                    RAP_STRUCTURED_TRACE(SFPartitionReplicaAlreadyOpen,                                     168,    "SFP_ReplicaOpen_ComStatefulServicePartition",          Error,          "Replica {0} is already opened (opened = {1}) or already has a replicator created (replicatorCreated = {2}).", "replicaId", "isOpened", "isCreateCalled"),
                    RAP_STRUCTURED_TRACE(SFPartitionCouldNotGetFUP,                                         169,    "SFP_CouldNotGetFUP_ComStatefulServicePartition",       Noise,          "Replica {0} could not get the FUP", "replicaId"),
                    RAP_STRUCTURED_TRACE(SFPartitionFailedToCreateReplicator,                               170,    "SFP_FailedReplicator_ComStatefulServicePartition",     Warning,          "Replica {0} failed creating the replicator (hr = {1:x}).", "replicaId", "hr"),
                    RAP_STRUCTURED_TRACE(LFUPM,                                                             175,    "LFUPM",                                                Info,           "{1} {2} {3} {4}", "contextSequenceId", "rapId", "size", "removeSize", "proxycleanupsize"),
                    RAP_STRUCTURED_TRACE(LFUPMTimerChange,                                                  176,    "LFUPM_FailoverUnitProxyMap",                           Noise,          "{0}: {1}", "tag", "obj"),
                    RAP_STRUCTURED_TRACE(FailoverUnitProxyEndpointUpdate,                                   181,    "FUP_Endpoint",                                         Info,           "RAP on {1}: updated {2} endpoint to {3}", "id", "rapId", "tag", "endpoint"),
                    
                    RAP_STRUCTURED_TRACE(LoadReportingOpen,                                                 192,    "Open_LoadReporting",                                   Info,           "Open: {0} {1}", "rapId", "loadcount"),
                    RAP_STRUCTURED_TRACE(LoadReportingClose,                                                193,    "Close_LoadReporting",                                  Info,           "Close: {0} {1}", "rapId", "loadcount"),
                    RAP_STRUCTURED_TRACE(LoadReportingSetTimer,                                             194,    "SetTimer_LoadReporting",                               Noise,          "SetTimer: {0} {1}", "rapId", "loadcount"),
                    RAP_STRUCTURED_TRACE(LoadReportingUpdateTimer,                                          195,    "UpdateTimer_LoadReporting",                            Noise,          "UpdateTimer: {0} {1}", "rapId", "loadcount"),
                    RAP_STRUCTURED_TRACE(LoadReportingReportCallback,                                       196,    "ReportCallback_LoadReporting",                         Info,          "ReportCallback: {0} {1}", "rapId", "loadcount"),
                    RAP_STRUCTURED_TRACE(LoadReportingAddLoad,                                              197,    "AddLoad_LoadReporting",                                Noise,          "AddLoad: {0} {1} Adding metric {4} load {5} for {2} role:{3}", "rapId", "loadcount", "fuid", "replicarole", "metricname", "metricvalue"),
                    RAP_STRUCTURED_TRACE(LoadReportingInvalidLoad,                                          199,    "InvalidLoad_LoadReporting",                            Info,           "InvalidLoad: {0} Invalid load: fuid: {1} role:{2}", "rapId", "fuid", "replicarole"),
                    RAP_STRUCTURED_TRACE(LoadReportingRemoveLoad,                                           201,    "RemoveLoad_LoadReporting",                             Noise,          "RemoveLoad: {0} {1} Removing load for {2}", "rapId", "loadcount", "fuid"),

                    RAP_STRUCTURED_TRACE(MessageSenderOpen,                                                 222,    "Open_MessageSender",                                   Info,           "Open: {0}", "rapId"),
                    RAP_STRUCTURED_TRACE(MessageSenderClose,                                                223,    "Close_MessageSender",                                  Info,           "Close: {0} {1}", "rapId", "messagecount"),
                    RAP_STRUCTURED_TRACE(MessageSenderSetTimer,                                             224,    "SetTimer_MessageSender",                               Noise,          "SetTimer: {0} {1}", "rapId", "messagecount"),
                    RAP_STRUCTURED_TRACE(MessageSenderUpdateTimer,                                          225,    "UpdateTimer_MessageSender",                            Noise,          "UpdateTimer: {0} {1}", "rapId", "messagecount"),
                    RAP_STRUCTURED_TRACE(MessageSenderCallback,                                             226,    "Callback_MessageSender",                               Noise,          "Callback: {0} {1}", "rapId", "messagecount"),
                    RAP_STRUCTURED_TRACE(MessageSenderAdd,                                                  227,    "Add_MessageSender",                                    Noise,          "Add: {0} {1}", "rapId", "messagecount"),
                    RAP_STRUCTURED_TRACE(MessageSenderSend,                                                 228,    "Send_MessageSender",                                   Noise,          "Send: {0} {1}", "rapId", "action"),
                    RAP_STRUCTURED_TRACE(MessageSenderRemove,                                               229,    "Remove_MessageSender",                                 Noise,          "Remove: {0} {1}", "rapId", "action"),
                                                                                                                                                                                            
                    RAP_STRUCTURED_TRACE_QUERY(ApiReportFault,                                              247,    "_Api_ApiReportFault",                                       Info,           "API ReportFault {4} for FT {0} on node {1} with Replica Id {2} and Replica Instance Id {3} and ReasonActivityDescription {5}", "id", "nodeId", "replicaId", "replicaInstanceId", "faultType", "reasonActivityDescription"),
                    RAP_STRUCTURED_TRACE(LoadReportingUndefinedMetric,                                      248,    "LoadReporting_UndefinedMetric",                        Warning,        "UndefinedMetric: {0} reported for fuid: {1}", "metricName", "fuid")
                {
                }

            public:
                static Common::Global<RAPEventSource> Events;
        };
    }
}
