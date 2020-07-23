// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define DECLARE_BAP_STRUCTURED_TRACE(trace_name, ...) Common::TraceEventWriter<__VA_ARGS__> trace_name

#define BAP_STRUCTURED_TRACE(trace_name, trace_id, trace_type, trace_level, ...) \
            trace_name( \
                Common::TraceTaskCodes::BAP, \
                trace_id, \
                #trace_name "_" trace_type, \
                Common::LogLevel::trace_level, \
                __VA_ARGS__)

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class BAPEventSource
        {
        public:
            DECLARE_BAP_STRUCTURED_TRACE(Ctor);
            DECLARE_BAP_STRUCTURED_TRACE(Dtor, BackupRestoreAgentProxyId);
            DECLARE_BAP_STRUCTURED_TRACE(RegisterIpcMessageHandler, BackupRestoreAgentProxyId);
            DECLARE_BAP_STRUCTURED_TRACE(Open, BackupRestoreAgentProxyId);
            DECLARE_BAP_STRUCTURED_TRACE(Abort, BackupRestoreAgentProxyId);
            DECLARE_BAP_STRUCTURED_TRACE(Close, BackupRestoreAgentProxyId);
            
            DECLARE_BAP_STRUCTURED_TRACE(ProcessIpcMessage, BackupRestoreAgentProxyId, Transport::Actor::Trace, std::wstring, Common::ActivityId, Transport::MessageId);
            DECLARE_BAP_STRUCTURED_TRACE(EndSendSuccess, BackupRestoreAgentProxyId, Transport::Actor::Trace, std::wstring, Common::ActivityId, Transport::MessageId);
            DECLARE_BAP_STRUCTURED_TRACE(EndSendFailure, BackupRestoreAgentProxyId, Common::ActivityId, Common::ErrorCode);
            DECLARE_BAP_STRUCTURED_TRACE(BeginSend, BackupRestoreAgentProxyId, Common::ActivityId, Transport::Actor::Trace, std::wstring);
            DECLARE_BAP_STRUCTURED_TRACE(PartitionHeaderMissing, BackupRestoreAgentProxyId, Common::ActivityId);
            DECLARE_BAP_STRUCTURED_TRACE(UpdatePolicyFailure, BackupRestoreAgentProxyId, Common::ActivityId, Common::ErrorCode);
            DECLARE_BAP_STRUCTURED_TRACE(BackupNowRequestFailure, BackupRestoreAgentProxyId, Common::ActivityId, Common::ErrorCode);

            DECLARE_BAP_STRUCTURED_TRACE(RegisterPartition, BackupRestoreAgentProxyId, Common::Guid);
            DECLARE_BAP_STRUCTURED_TRACE(UnregisterPartition, BackupRestoreAgentProxyId, Common::Guid);

            DECLARE_BAP_STRUCTURED_TRACE(NotOpen, BackupRestoreAgentProxyId, Common::ActivityId);
            DECLARE_BAP_STRUCTURED_TRACE(PartitionNotRegistered, BackupRestoreAgentProxyId, Common::ActivityId, Common::Guid);
            DECLARE_BAP_STRUCTURED_TRACE(InvalidAction, BackupRestoreAgentProxyId, Common::ActivityId, std::wstring);

            BAPEventSource() :
                BAP_STRUCTURED_TRACE(Ctor,                        8, "Ctor", Info, "Constructing."),
                BAP_STRUCTURED_TRACE(Dtor,                        9, "Dtor", Info, "Destructing.", "bapId"),
                BAP_STRUCTURED_TRACE(RegisterIpcMessageHandler,  10, "RegisterIpcMessageHandler", Info, "Registering IpcMessageHandler.", "bapId"),
                BAP_STRUCTURED_TRACE(Open,                       11, "Open", Info, "Opened.", "bapId"),
                BAP_STRUCTURED_TRACE(Abort,                      12, "Abort", Info, "Aborted.", "bapId"),
                BAP_STRUCTURED_TRACE(Close,                      13, "Close", Info, "Closed.", "bapId"),

                BAP_STRUCTURED_TRACE(ProcessIpcMessage,          20, "ProcessIpcMessage",     Info, "BAP on {0}: Received message with Actor {1} Action {2} ActivityId {3} MessageId {4}.", "bapId", "actor", "action", "activityid", "messageid"),
                BAP_STRUCTURED_TRACE(EndSendSuccess,             21, "SendSuccess",           Info, "BAP on {0}: Received message reply with Actor {1} and Action {2} and ActivityId {3}, MessageId {4}.", "bapId", "actor", "action", "activityid", "messageid"),
                BAP_STRUCTURED_TRACE(EndSendFailure,             22, "SendFailure",        Warning, "BAP on {0}: End send request with ActivityId {1} failure {2}.", "bapId", "activityid", "errorcode"),
                BAP_STRUCTURED_TRACE(BeginSend,                  23, "BeginSend",             Info, "BAP on {0}: Begin send request with ActivityId: {1} Actor {2} and Action {3}.", "bapId", "activityid", "actor", "action"),
                BAP_STRUCTURED_TRACE(PartitionHeaderMissing,     24, "PartitionHeaderMissing",  Warning, "BAP on {0}: Required PartitionTarget header is missing in message {1}.", "bapId", "activityid"),
                BAP_STRUCTURED_TRACE(UpdatePolicyFailure,        25, "UpdatePolicyFailure",Warning, "BAP on {0}: Update policy message failure, activityid: {1}, errorcode {2}.", "bapId", "activityid", "errorcode"),
                BAP_STRUCTURED_TRACE(BackupNowRequestFailure,    26, "BackupNowRequestFailure", Warning, "BAP on {0}: Backup Now request message failure, activityid: {1}, errorcode {2}.", "bapId", "activityid", "errorcode"),

                BAP_STRUCTURED_TRACE(RegisterPartition,          40, "RegisterPartition",     Info, "BAP on {0}: Registering partition {1}.", "bapId", "partitionId"),
                BAP_STRUCTURED_TRACE(UnregisterPartition,        41, "UnregisterPartition",   Info, "BAP on {0}: Unregistering partition {1}.", "bapId", "partitionId"),

                BAP_STRUCTURED_TRACE(NotOpen,                    50, "NotOpen", Warning, "BAP on {0}: Dropping Message {1} as BAP is not open.", "bapId", "activityid"),
                BAP_STRUCTURED_TRACE(PartitionNotRegistered,     51, "PartitionNotRegistered", Warning, "BAP on {0}: ActivityId: {1} Specified partition {2} is not registered with BAP.", "bapId", "activityid", "partition"),
                BAP_STRUCTURED_TRACE(InvalidAction,              52, "InvalidAction", Warning, "BAP on {0}: ActivityId: {1} Message received with invalid action {2}.", "bapId", "activityid", "action")
            {
            }

            static Common::Global<BAPEventSource> Events;
        };


    }
}
