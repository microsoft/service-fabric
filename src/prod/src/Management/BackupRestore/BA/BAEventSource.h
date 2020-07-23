// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define DECLARE_BA_STRUCTURED_TRACE(trace_name, ...) Common::TraceEventWriter<__VA_ARGS__> trace_name

#define BA_STRUCTURED_TRACE(trace_name, trace_id, trace_type, trace_level, ...) \
            trace_name( \
                Common::TraceTaskCodes::BA, \
                trace_id, \
                #trace_name "_" trace_type, \
                Common::LogLevel::trace_level, \
                __VA_ARGS__)

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class BAEventSource
        {
        public:
            DECLARE_BA_STRUCTURED_TRACE(Ctor, std::wstring);
            DECLARE_BA_STRUCTURED_TRACE(Dtor, std::wstring);
            DECLARE_BA_STRUCTURED_TRACE(RegisterIpcMessageHandler, std::wstring);
            DECLARE_BA_STRUCTURED_TRACE(RegisterWithFederation, std::wstring);
            DECLARE_BA_STRUCTURED_TRACE(Open, std::wstring);
            DECLARE_BA_STRUCTURED_TRACE(Abort, std::wstring);
            DECLARE_BA_STRUCTURED_TRACE(Close, std::wstring);

            DECLARE_BA_STRUCTURED_TRACE(ProcessTransportRequestResponseRequest, std::wstring, Common::ActivityId, Transport::MessageId);
            DECLARE_BA_STRUCTURED_TRACE(FederationSendReply, std::wstring, Common::ActivityId);
            DECLARE_BA_STRUCTURED_TRACE(FederationReject, std::wstring, Common::ActivityId, Common::ErrorCode);
            DECLARE_BA_STRUCTURED_TRACE(ProcessTransportIpcRequest, std::wstring, Common::ActivityId, Transport::MessageId);
            DECLARE_BA_STRUCTURED_TRACE(IpcSendReply, std::wstring, Common::ActivityId, Transport::MessageId);
            DECLARE_BA_STRUCTURED_TRACE(IpcSendFailure, std::wstring, Common::ActivityId, Common::ErrorCode);


            // Backup Copier Proxy
            //
            DECLARE_BA_STRUCTURED_TRACE(BackupCopierCreating, Federation::NodeInstance, std::wstring, Common::TimeSpan, Common::TimeSpan);
            DECLARE_BA_STRUCTURED_TRACE(BackupCopierCreated, Federation::NodeInstance, int64, int);
            DECLARE_BA_STRUCTURED_TRACE(BackupCopierFailed, Federation::NodeInstance, Common::ErrorCode, std::wstring);

            BAEventSource() :
                BA_STRUCTURED_TRACE(Ctor,                       5, "Ctor", Info, "Constructing", "id"),
                BA_STRUCTURED_TRACE(Dtor,                       6, "Dtor", Info, "Destructing", "id"),
                BA_STRUCTURED_TRACE(RegisterIpcMessageHandler,  7, "RegisterIpcMessageHandler", Info, "Registering IpcMessageHandler", "id"),
                BA_STRUCTURED_TRACE(RegisterWithFederation,     8, "RegisterFederationSubsystemEvents", Info, "Registering for FederationSubsystemEvents", "id"),
                BA_STRUCTURED_TRACE(Open,                       9, "Open", Info, "Open", "id"),
                BA_STRUCTURED_TRACE(Abort,                     10, "Abort", Info, "Abort", "id"),
                BA_STRUCTURED_TRACE(Close,                     11, "Close", Info, "Close", "id"),

                BA_STRUCTURED_TRACE(ProcessTransportRequestResponseRequest, 20, "ProcessTransportRequestResponseRequest",    Info, "BA on {0}: Received routed message with activityId: {1} and messageid:{2}.", "baId", "activityid", "messageid"),
                BA_STRUCTURED_TRACE(FederationSendReply,                    21, "FederationSendReply",    Info, "BA on {0}: Replying to routed request with activityid {1}.", "baId", "activityid"),
                BA_STRUCTURED_TRACE(FederationReject,                       22, "FederationReject", Warning, "BA on {0}: Rejecting federation request with activity id {1} with error {2}.", "baId", "activityid", "error"),
                BA_STRUCTURED_TRACE(ProcessTransportIpcRequest,             23, "ProcessTransportIpcRequest", Info, "BA on {0}: Received IPC message with activityid: {1} and messageid:{2}.", "baId", "activityid", "messageid"),
                BA_STRUCTURED_TRACE(IpcSendReply,                           24, "IpcSendReply",    Info, "BA on {0}: Replying IPC request with activityid {1} and message id {2}.", "baId", "activityid", "messageid"),
                BA_STRUCTURED_TRACE(IpcSendFailure,                         25, "IpcSendFailure", Warning, "BA on {0}: Sending IPC failure message with activity id {1} error {2}.", "baId", "activityid", "error"),
                
                // Backup Copier Proxy
                //
                BA_STRUCTURED_TRACE(BackupCopierCreating,   30, "BackupCopierCreating", Noise, "[{0}] args = '{1}' timeout = {2} buffer = {3}", "node", "args", "timeout", "timeoutbuffer"),
                BA_STRUCTURED_TRACE(BackupCopierCreated,    31, "BackupCopierCreated", Info,  "[{0}] handle = {1} process id = {2}", "node", "handle", "processId"),
                BA_STRUCTURED_TRACE(BackupCopierFailed,     32, "BackupCopierFailed", Info,  "[{0}] error = {1} message = '{2}'", "node", "error", "message")
                {
                }

            static Common::Global<BAEventSource> Events;
        };
    }
}
