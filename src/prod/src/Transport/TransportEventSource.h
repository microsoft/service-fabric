// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
#define TRANSPORT_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
    STRUCTURED_TRACE(trace_name, Transport, trace_id, trace_level, __VA_ARGS__)

#define PENDING_RECEIVE_MISSING_DIAGNOSTIC_MESSAGE \
    "{1}-{2}: pending receive has been missing longer than {3}, "\
    "there could be a deadlock or unexpected long blocking of receiving thread. "\
    "Check the last few (process={4} &amp;&amp; type~Transport.Dispatch@{0}) traces for blocked/slow dispatch. "\
    "Last dispatched messages (ignore ordering): {5}"

    class TransportEventSource
    {
    public:
        // transport
        DECLARE_STRUCTURED_TRACE(TransportCreated, std::wstring, std::wstring, std::wstring, uint64);
        DECLARE_STRUCTURED_TRACE(TransportDestroyed, std::wstring, uint64);
        DECLARE_STRUCTURED_TRACE(TransportInstanceSet, std::wstring, std::wstring, uint64, uint64);
        DECLARE_STRUCTURED_TRACE(Started, std::wstring, std::wstring, uint64);
        DECLARE_STRUCTURED_TRACE(Stopped, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(State, std::wstring, std::wstring, std::wstring, uint64, std::wstring);
        DECLARE_STRUCTURED_TRACE(SuspendAccept, std::wstring, std::wstring, uint64);
        DECLARE_STRUCTURED_TRACE(ResumeAccept, uint64);
        DECLARE_STRUCTURED_TRACE(AcceptResumed, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(AcceptResumeFailed, std::wstring, std::wstring, Common::ErrorCode, Common::TimeSpan);
        DECLARE_STRUCTURED_TRACE(CreateListenInstanceMessage, std::wstring, ListenInstance, std::wstring);
        DECLARE_STRUCTURED_TRACE(TcpKeepAliveTimeoutSet, std::wstring, int64);
        DECLARE_STRUCTURED_TRACE(ConnectionIdleTimeoutSet, std::wstring, int64);
        DECLARE_STRUCTURED_TRACE(OutgoingMessageExpirationSet, std::wstring, Common::TimeSpan);
        DECLARE_STRUCTURED_TRACE(SendQueueSizeMaxIncreased, std::wstring, std::wstring, std::wstring, uint64);
        DECLARE_STRUCTURED_TRACE(Named_TargetAdded, std::wstring, std::wstring, std::wstring, std::wstring, uint64, uint64);
        DECLARE_STRUCTURED_TRACE(Anonymous_TargetAdded, std::wstring, std::wstring, std::wstring, std::wstring, uint64, uint64);
        DECLARE_STRUCTURED_TRACE(TargetRemoved, std::wstring, std::wstring, std::wstring, std::wstring);

        // target
        DECLARE_STRUCTURED_TRACE(TargetDown, std::wstring, std::wstring, std::wstring, uint64, bool, uint64);
        DECLARE_STRUCTURED_TRACE(TargetClose, std::wstring, std::wstring, std::wstring, bool);
        DECLARE_STRUCTURED_TRACE(TargetReset, std::wstring, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(TargetInstanceUpdated, std::wstring, std::wstring, std::wstring, uint64, uint64);
        DECLARE_STRUCTURED_TRACE(TargetInstanceIngored, std::wstring, std::wstring, std::wstring, uint64, uint64);
        DECLARE_STRUCTURED_TRACE(TargetConnectionsChanged, std::wstring, std::wstring, std::wstring, bool, uint64, size_t, std::wstring);
        DECLARE_STRUCTURED_TRACE(CannotSend, std::wstring, std::wstring, std::wstring, Common::ErrorCode, MessageId, Actor::Trace, std::wstring);
        DECLARE_STRUCTURED_TRACE(CloseConnection, std::wstring, std::wstring, std::wstring, std::wstring, bool);

        // connection
        DECLARE_STRUCTURED_TRACE(ConnectionCreated, std::wstring, std::wstring, std::wstring, std::wstring, bool, uint32, TransportPriority::Trace, uint64);
        DECLARE_STRUCTURED_TRACE(ConnectionDestroyed, std::wstring, bool, uint64);
        DECLARE_STRUCTURED_TRACE(ConnectionBoundLocally, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(ConnectionState, std::wstring, std::wstring, std::wstring, TcpConnectionState::Trace, std::wstring, bool, bool);
        DECLARE_STRUCTURED_TRACE(ConnectionStateTransit, std::wstring, std::wstring, std::wstring, TcpConnectionState::Trace, TcpConnectionState::Trace, bool, bool);
        DECLARE_STRUCTURED_TRACE(ConnectionAccepted, std::wstring, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(ConnectionEstablished, std::wstring, std::wstring, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(ConnectionFaulted, std::wstring, std::wstring, std::wstring, Common::ErrorCode);
        DECLARE_STRUCTURED_TRACE(ConnectionSetTarget, std::wstring, std::wstring, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(ConnectionCleanupScheduled, std::wstring, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(ConnectionThreadpoolIoCleanedUp, std::wstring, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(ConnectionClose, std::wstring, std::wstring, std::wstring, bool, TcpConnectionState::Trace, bool, bool);
        DECLARE_STRUCTURED_TRACE(Msg_InvalidStateForSend, std::wstring, std::wstring, std::wstring, TcpConnectionState::Trace, Common::ErrorCode, MessageId, Actor::Trace, std::wstring);
        DECLARE_STRUCTURED_TRACE(Null_InvalidStateForSend, std::wstring, std::wstring, std::wstring, TcpConnectionState::Trace, Common::ErrorCode);
        DECLARE_STRUCTURED_TRACE(ConnectionInstanceConfirmed, std::wstring, uint64, ListenInstance);
        DECLARE_STRUCTURED_TRACE(ConnectionInstanceInitialized, uint64, Common::Guid, uint64);
        DECLARE_STRUCTURED_TRACE(ConnectionIdleTimeout, std::wstring);
        DECLARE_STRUCTURED_TRACE(ConnectionInstanceObsolete, std::wstring);
        DECLARE_STRUCTURED_TRACE(Enqueue, std::wstring, MessageId, bool, std::wstring, uint32, uint32, uint32);
        DECLARE_STRUCTURED_TRACE(LTEnqueue, std::wstring, uint64, uint32, uint32, uint32);
        DECLARE_STRUCTURED_TRACE(Dequeue, std::wstring, MessageId, bool, uint64);
        DECLARE_STRUCTURED_TRACE(LTDequeue, std::wstring, uint64, uint64);
        DECLARE_STRUCTURED_TRACE(BeginConnect, std::wstring, std::wstring, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(BeginReceive, std::wstring);
        DECLARE_STRUCTURED_TRACE(ReceiveFailed, std::wstring, std::wstring, std::wstring, Common::ErrorCode);
        DECLARE_STRUCTURED_TRACE(ReceivedData, std::wstring, uint64, uint64);
        DECLARE_STRUCTURED_TRACE(DispatchMsg, std::wstring, MessageId, bool, uint32, uint32);
        DECLARE_STRUCTURED_TRACE(DispatchActivity, std::wstring, MessageId, bool, uint32, uint32, Actor::Trace, std::wstring, Common::ActivityId);
        DECLARE_STRUCTURED_TRACE(FailedToConnect, std::wstring, std::wstring, std::wstring, std::wstring, Common::ErrorCode, uint64, std::wstring);
        DECLARE_STRUCTURED_TRACE(BeginSend, std::wstring);
        DECLARE_STRUCTURED_TRACE(SendCompleted, std::wstring, uint64, uint64);
        DECLARE_STRUCTURED_TRACE(StartCloseDrainingSend, std::wstring, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(ReceiveMissingDetected, std::wstring, std::wstring, std::wstring, Common::TimeSpan, uint, std::wstring);
        DECLARE_STRUCTURED_TRACE(DropMessageOnAbort, MessageId, Actor::Trace, std::wstring, Common::ErrorCode);
        DECLARE_STRUCTURED_TRACE(SendQueueFull, std::wstring, std::wstring, std::wstring, uint64, int64, uint64, MessageId, uint32, Actor::Trace, std::wstring);

        // security
        DECLARE_STRUCTURED_TRACE(SecurityContextCreated, std::wstring, SecurityProvider::Trace, uint64, uint64, bool, bool, std::wstring);
        DECLARE_STRUCTURED_TRACE(SecurityNegotiationStatus, std::wstring, SecurityProvider::Trace, std::wstring, uint32);
        DECLARE_STRUCTURED_TRACE(SecurityNegotiationFailed, std::wstring, SecurityProvider::Trace, std::wstring, uint32);
        DECLARE_STRUCTURED_TRACE(SecurityNegotiationSucceeded, std::wstring, SecurityProvider::Trace, std::wstring, std::wstring, uint32, std::wstring);

        DECLARE_STRUCTURED_TRACE(SecurityContextDestructing, std::wstring, uint64, uint64);
        DECLARE_STRUCTURED_TRACE(SecurityNegotiationDispatch, std::wstring, MessageId, bool, Actor::Trace);
        DECLARE_STRUCTURED_TRACE(SecurityNegotiationSend, std::wstring, MessageId);
        DECLARE_STRUCTURED_TRACE(SecurityNegotiationOngoing, std::wstring, MessageId);
        DECLARE_STRUCTURED_TRACE(SecurityCredentialExpiration, std::wstring);
        DECLARE_STRUCTURED_TRACE(SecureSessionAuthorizationFailed, std::wstring, Common::ErrorCodeValue::Trace);
        DECLARE_STRUCTURED_TRACE(SecureSessionExpired, std::wstring);
        DECLARE_STRUCTURED_TRACE(SecuritySettingsSet, std::wstring, std::wstring);
        DECLARE_STRUCTURED_TRACE(SecuritySettingsSetFailure, std::wstring, std::wstring, Common::ErrorCode);

        // serialization
        DECLARE_STRUCTURED_TRACE(InvalidFrame, std::wstring, std::wstring, std::wstring, std::wstring, TcpFrameHeader);
        DECLARE_STRUCTURED_TRACE(IncomingMessageTooLarge, std::wstring, std::wstring, std::wstring, std::wstring, uint64, uint64);
        DECLARE_STRUCTURED_TRACE(OutgoingMessageTooLarge, std::wstring, std::wstring, std::wstring, uint64, uint64, MessageId, Actor::Trace, std::wstring);
        DECLARE_STRUCTURED_TRACE(OutgoingMessageExpired, std::wstring, std::wstring, std::wstring, MessageId, Actor::Trace, std::wstring);
        DECLARE_STRUCTURED_TRACE(MessageHeaderDeserializationFailure, uint64, int32);
        DECLARE_STRUCTURED_TRACE(MessageBodyDeserializationFailure, uint64, int32);
        DECLARE_STRUCTURED_TRACE(BiqueRangeStreamRangeCheckingFailure, uint64);
        DECLARE_STRUCTURED_TRACE(DroppingInvalidMessage, uint64, int32);
        DECLARE_STRUCTURED_TRACE(MessageHeaderAddFailure, uint64, uint64, uint64, std::wstring);

        // throttle
        DECLARE_STRUCTURED_TRACE(Throttle, int32, int32, int32, int64, int64, MessageId, Actor::Trace, std::wstring);
        DECLARE_STRUCTURED_TRACE(ThrottleReadThreadCount, unsigned int);
        DECLARE_STRUCTURED_TRACE(ThrottleExceeded, int64, unsigned int, unsigned int, int64, int64);
        DECLARE_STRUCTURED_TRACE(ProcessPerfDataIsOld);
        DECLARE_STRUCTURED_TRACE(MemoryThrottleIncresed, int64, int64);

        TransportEventSource()
            : TRANSPORT_STRUCTURED_TRACE(
                Throttle,
                4,
                Warning,
                "Throttling: threadCount = {0} (activeCallback = {1}), threadThreshold = {2}, memory = {3}, memoryThreshold = {4}, dropping message {5}, Actor = {6}, Action = '{7}'",
                "threadCount",
                "activeCallback",
                "threadThreshold",
                "memory",
                "memoryThreshold",
                "MessageId",
                "Actor",
                "Action"),
            TRANSPORT_STRUCTURED_TRACE(ProcessPerfDataIsOld, 5, Warning, "PerfCounter data is older than expected"),
            TRANSPORT_STRUCTURED_TRACE(
                InvalidFrame,
                6,
                Warning,
                "{1}-{2} target {3}: invalid frame: {4}",
                "id",
                "localAddress",
                "targetAddress",
                "target",
                "frame"),
            TRANSPORT_STRUCTURED_TRACE(
                Enqueue,
                7,
                Info,
                "{1} {2} {3} {4}B @ qsize {5}/{6}B",
                "id", 
                "message",
                "isReply",
                "localContext",
                "messageSize",
                "messagesToSend",
                "bytesToSend"),
            TRANSPORT_STRUCTURED_TRACE(
                Dequeue,
                8,
                Noise,
                "sent and dequeue {1} {2}, {3} msg sent",
                "id", 
                "message",
                "isReply",
                "sentCount"),
            TRANSPORT_STRUCTURED_TRACE(
                TargetRemoved,
                9,
                Info,
                "removed target {1} {2}-{3}",
                "id",
                "target",
                "localAddress",
                "targetAddress"),
            TRANSPORT_STRUCTURED_TRACE(
                ConnectionInstanceConfirmed,
                10,
                Info,
                "current instance = {1}, confirmed remote ListenInstance = {2}",
                "id",
                "currentInstance",
                "remoteListenInstance"),
            TRANSPORT_STRUCTURED_TRACE(
                ConnectionInstanceInitialized,
                11,
                Info,
                "ListenId = {1}, instance = {2}",
                "id",
                "listenerId",
                "connectionInstance"),
            TRANSPORT_STRUCTURED_TRACE(
                ConnectionThreadpoolIoCleanedUp,
                12,
                Info,
                "{1}-{2} finishing threadpool IO cleanup",
                "id",
                "localAddress",
                "sendTargetAddress"),
            TRANSPORT_STRUCTURED_TRACE(
                ConnectionClose,
                13,
                Info,
                "{1}-{2} abort={3}, state={4}, sendDrained={5}, receiveDrained={6}",
                "id",
                "localAddress",
                "targetAddress",
                "abort",
                "state",
                "sendDrained",
                "receiveDrained"),
            TRANSPORT_STRUCTURED_TRACE(
                CannotSend,
                14,
                Info,
                "{1}-{2}: error = {3}, dropping message {4}, Actor = {5}, Action = '{6}'",
                "id",
                "localAddress",
                "targetAddress",
                "errorCode",
                "message",
                "actor",
                "action"),
            TRANSPORT_STRUCTURED_TRACE(
                BeginConnect,
                16,
                Info,
                "{1}-{2} connecting to {3}",
                "id",
                "localAddress",
                "targetAddress",
                "connectToAddress"),
            TRANSPORT_STRUCTURED_TRACE(
                LTEnqueue,
                17,
                Info,
                "{1} {2}B @ qsize {3}/{4}B",
                "id", 
                "message",
                "messageSize",
                "messagesToSend",
                "bytesToSend"),
            TRANSPORT_STRUCTURED_TRACE(
                BeginReceive,
                18,
                Noise,
                "begin receive",
                "id"),
            TRANSPORT_STRUCTURED_TRACE(
                ReceiveFailed,
                19,
                Info,
                "{1}-{2} receive failed: {3}",
                "id",
                "localAddress",
                "targetAddress",
                "error"),
            TRANSPORT_STRUCTURED_TRACE(
                ConnectionState,
                20,
                Info,
                "{1}-{2} at {3}: {4}, sendDrained={5}, receiveDrained={6}",
                "id",
                "localAddress",
                "targetAddress",
                "state",
                "msg",
                "sendDrained",
                "receiveDrained"),
            TRANSPORT_STRUCTURED_TRACE(
                ConnectionBoundLocally,
                21,
                Info,
                "bound locally at {1}",
                "id",
                "addressBound"),
            TRANSPORT_STRUCTURED_TRACE(
                ReceivedData,
                22,
                Noise,
                "{1} bytes, previous total: {2}",
                "id",
                "byteTransfered",
                "previousTotal"),
            DispatchMsg(
                Common::TraceTaskCodes::Transport,
                23,
                "Msg_Dispatch",
                Common::LogLevel::Info,
                "{1} {2} {3} {4}B",
                "id",
                "message",
                "isReply",
                "dispatchCountForThisReceive",
                "size"),
            TRANSPORT_STRUCTURED_TRACE(
                ConnectionStateTransit,
                24,
                Info,
                "{1}-{2} {3} -> {4}, sendDrained={5}, receiveDrained={6}",
                "id",
                "localAddress",
                "targetAddress",
                "oldState",
                "newState",
                "sendDrained",
                "receiveDrained"),
            TRANSPORT_STRUCTURED_TRACE(
                FailedToConnect,
                25,
                Warning,
                "{1}-{2}/{3}: error = {4}, failureCount={5}. Filter by {6} to get listener lifecycle. Connect failure is expected if listener was never started, or listener/its process was stopped before/during connecting.",
                "id",
                "localAddress",
                "targetAddress",
                "connectToAddress",
                "error",
                "failureCount",
                "traceFilterForTargetTransport"),
            TRANSPORT_STRUCTURED_TRACE(
                LTDequeue,
                26,
                Noise,
                "sent and dequeue {1}, {2} msg sent",
                "id", 
                "message",
                "sentCount"),
            TRANSPORT_STRUCTURED_TRACE(
                BeginSend,
                27,
                Noise,
                "begin send",
                "id"),
            TRANSPORT_STRUCTURED_TRACE(
                Msg_InvalidStateForSend,
                28,
                Warning,
                "{1}-{2}: cannot send, state = {3}, fault = {4}, dropping message {5}, Actor = {6}, Action = '{7}'",
                "id",
                "localAddress",
                "remoteAddress",
                "state",
                "fault",
                "message",
                "actor",
                "action"),
            TRANSPORT_STRUCTURED_TRACE(
                SendCompleted,
                29,
                Noise,
                "sent {1} bytes, previousTotal: {2}",
                "id",
                "byteTransfered",
                "previousTotal"),
            TRANSPORT_STRUCTURED_TRACE(
                StartCloseDrainingSend,
                30,
                Info,
                "{1}-{2} start send for close draining",
                "id",
                "localAddress",
                "targetAddress"),
            TRANSPORT_STRUCTURED_TRACE(
                ConnectionCleanupScheduled,
                31,
                Info,
                "{1}-{2} clean up scheduled",
                "id",
                "localAddress",
                "targetAddress"),
            TRANSPORT_STRUCTURED_TRACE(
                CloseConnection,
                32,
                Info,
                "{1}-{2} closing {3}: abort={4}",
                "id",
                "localAddress",
                "targetAddress",
                "connection",
                "abort"),
            TRANSPORT_STRUCTURED_TRACE(
                TargetConnectionsChanged,
                33,
                Info,
                "{1}-{2}: anonymous={3}, instance={4}, {5} connections: {6}",
                "id",
                "localAddress",
                "address",
                "anonymous",
                "instance",
                "connectionCount",
                "connections"),
            TRANSPORT_STRUCTURED_TRACE(
                Named_TargetAdded,
                34,
                Info,
                "named target {1} {2}-{3}, totalNamed = {4}, total = {5}",
                "id",
                "targetId",
                "localAddress",
                "targetAddress",
                "totalNamed",
                "total"),
            TRANSPORT_STRUCTURED_TRACE(
                SecurityNegotiationFailed,
                15,
                Warning,
                "{1}: target={2}, error=0x{3:x}",
                "id",
                "provider",
                "targetName",
                "status"),
            TRANSPORT_STRUCTURED_TRACE(
                SecurityContextCreated,
                35,
                Info,
                "{1}: context pointer={2:x}, objCount={3}, inbound = {4}, FramingProtectionEnabled = {5}, credential expiration = {6}",
                "id",
                "provider",
                "securityContextPtr",
                "objCount",
                "inbound",
                "framingProtectionEnabled",
                "credentialExpiration"),
            TRANSPORT_STRUCTURED_TRACE(
                SecurityNegotiationDispatch,
                36,
                Info,
                "dispatch {1} {2} actor={3}",
                "id",
                "messageId",
                "isReply",
                "actor"),
            TRANSPORT_STRUCTURED_TRACE(
                SecurityNegotiationSend,
                37,
                Info,
                "sending negotiation message {1}",
                "id",
                "messageId"),
            TRANSPORT_STRUCTURED_TRACE(
                SecurityNegotiationOngoing,
                38,
                Info,
                "negotiation ongoing, queuing message {1} in pending queue",
                "id",
                "messageId"),
            TRANSPORT_STRUCTURED_TRACE(
                SecurityNegotiationStatus,
                39,
                Info,
                "{1}: target={2}, status=0x{3:x}",
                "id",
                "provider",
                "targetName",
                "status"),
            TRANSPORT_STRUCTURED_TRACE(
                SecurityNegotiationSucceeded,
                40,
                Info,
                "{1}: target={2}, package={3}, capabilities={4:x}, contextExpiration:{5}",
                "id",
                "provider",
                "target",
                "package",
                "capabilities",
                "expiration"),
           TRANSPORT_STRUCTURED_TRACE(
                ThrottleReadThreadCount,
                41,
                Info,
                "Thread count read: {0}",
                "threadCount"),
            TRANSPORT_STRUCTURED_TRACE(
                TargetDown,
                42,
                Info,
                "{1}-{2}: instance={3}, isDown={4}, being notified {5} is down",
                "id",
                "localAddress",
                "targetAddress",
                "instance",
                "isDown",
                "downInstnace"),
            TRANSPORT_STRUCTURED_TRACE(
                ConnectionCreated,
                43,
                Info,
                "{1}-{2} target {3} created, inbound={4}, receiveChunkSize={5}, priority={6}, count(same type)={7}",
                "id",
                "localAddress",
                "targetAddress",
                "target",
                "inbound",
                "receiveChunkSize",
                "priority",
                "count"),
            TRANSPORT_STRUCTURED_TRACE(
                ConnectionSetTarget,
                44,
                Info,
                "moved to new target {1} {2}-{3}",
                "id", 
                "target",
                "localAddress",
                "targetAddress"),
            TRANSPORT_STRUCTURED_TRACE(
                IncomingMessageTooLarge,
                45,
                Warning,
                "{1}-{2} target {3}: dropping message: incoming frame length {4} exceeds limit {5}",
                "id",
                "localAddress",
                "targetAddress",
                "target",
                "frameSize",
                "frameSizeLimit"),
            TRANSPORT_STRUCTURED_TRACE(
                OutgoingMessageTooLarge,
                46,
                Error,
                "{1}-{2} outgoing frame length {3} exceeds limit {4}, dropping message {5}, Actor = {6}, Action = '{7}'",
                "id",
                "localAddress",
                "targetAddress",
                "frameSize",
                "frameSizeLimit",
                "message",
                "actor",
                "action"),
            TRANSPORT_STRUCTURED_TRACE(
                MessageHeaderDeserializationFailure,
                47,
                Error,
                "HeaderReference {0:x}: header deserialization failure: {1}",
                "headerReferencePtr",
                "status"),
            TRANSPORT_STRUCTURED_TRACE(
                MessageBodyDeserializationFailure,
                48,
                Error,
                "Message {0:x}: body deserialization failure: {1}",
                "messagePtr",
                "status"),
            TRANSPORT_STRUCTURED_TRACE(
                BiqueRangeStreamRangeCheckingFailure,
                49,
                Error,
                "BiqueRangeStream {0:x}: range checking failed",
                "biqueRangeStreamPtr"),
            TRANSPORT_STRUCTURED_TRACE(
                DroppingInvalidMessage,
                50,
                Warning,
                "dropping message {0:x}, invalid status:{1}",
                "messagePtr",
                "status"),
            TRANSPORT_STRUCTURED_TRACE(
                MessageHeaderAddFailure,
                51,
                Error,
                "MessageHeaders {0:x}: failed to add a new header, current size = {1}, would-be new size = {2}, current headers: {3}",
                "messageHeadersPtr",
                "currentSize",
                "newSize",
                "currentHeaders"),
           TRANSPORT_STRUCTURED_TRACE(
                ReceiveMissingDetected,
                53,
                Error,
                PENDING_RECEIVE_MISSING_DIAGNOSTIC_MESSAGE,
                "id",
                "localAddress",
                "targetAddress",
                "threshold",
                "processId",
                "lastDispatchedMessages"),
            TRANSPORT_STRUCTURED_TRACE(
                SecureSessionExpired,
                54,
                Info,
                "session expired",
                "id"),
            TRANSPORT_STRUCTURED_TRACE(
                SecuritySettingsSet,
                55,
                Info,
                "security settings set to {1}",
                "id",
                "settings"),
            TRANSPORT_STRUCTURED_TRACE(
                SecuritySettingsSetFailure,
                56,
                Error,
                "failed to set security settings to {1}: {2}",
                "id",
                "settings",
                "errorcode"),
            TRANSPORT_STRUCTURED_TRACE(
                ConnectionIdleTimeout,
                57,
                Info,
                "connection idle timeout occurred: {0}",
                "connection"),
            TRANSPORT_STRUCTURED_TRACE(
                TargetClose,
                58,
                Info,
                "{1}-{2}: target close, abort = {3}",
                "id",
                "localAddress",
                "targetAddress",
                "abort"),
            TRANSPORT_STRUCTURED_TRACE(
                ConnectionIdleTimeoutSet,
                59,
                Info,
                "connection idle timeout set to {1} seconds",
                "id",
                "connectionIdleTimeout"),
            TRANSPORT_STRUCTURED_TRACE(
                ThrottleExceeded,
                60,
                Warning,
                "Throttle exceeded for {0} seconds: thread count = {1} throttle = {2}, memory = {3} throttle = {4}",
                "duration",
                "threadCount",
                "threadCountThreshold",
                "memory",
                "memoryThreshold"),
            TRANSPORT_STRUCTURED_TRACE(
                Started,
                62,
                Info,
                "listen address = '{1}', instance={2}",
                "id",
                "listenAddress",
                "instance"),
            TRANSPORT_STRUCTURED_TRACE(
                CreateListenInstanceMessage,
                63,
                Info,
                "create listen instance message {1} for connection {2}",
                "id",
                "listenInstance",
                "connectionId"),
            TRANSPORT_STRUCTURED_TRACE(
                TargetInstanceUpdated,
                64,
                Info,
                "{1}-{2}: instance updated: {3} -> {4}",
                "id",
                "localAddress",
                "targetAddress",
                "currentInstance",
                "newInstance"),
            TargetInstanceIngored(
                Common::TraceTaskCodes::Transport,
                65,
                "TargetInstanceIngored",
                Common::LogLevel::Warning,
                Common::TraceChannelType::Debug,
                "{1}-{2}: current instance {3} is newer, input {4} ignored",
                "id",
                "localAddress",
                "targetAddress",
                "currentInstance",
                "newInstance"),
            TRANSPORT_STRUCTURED_TRACE(
                State,
                67,
                Info,
                "owner = '{1}', listenAddress = '{2}', instance = {3}, securitySettings = {4}",
                "id",
                "owner",
                "listenAddress",
                "instance",
                "securitySettings"),
            TRANSPORT_STRUCTURED_TRACE(
                ConnectionInstanceObsolete,
                68,
                Info,
                "connection instance down or obsolete: {0}",
                "connection"),
            TRANSPORT_STRUCTURED_TRACE(
                TransportInstanceSet,
                70,
                Info,
                "{1}: set instance: current = {2}, new = {3}",
                "id",
                "listenAddress",
                "currentInstance",
                "newInstance"),
            TRANSPORT_STRUCTURED_TRACE(
                ConnectionDestroyed,
                71,
                Info,
                "inbound={1}, {2} remained",
                "id",
                "inbound",
                "remained"),
            TRANSPORT_STRUCTURED_TRACE(
                SecurityContextDestructing,
                72,
                Info,
                "context {1:x} is destructing, objCount = {2}",
                "id",
                "securityContextPtr",
                "objCount"),
            TRANSPORT_STRUCTURED_TRACE(
                MemoryThrottleIncresed,
                73,
                Info,
                "memory throttle limit increased from {0} to {1}",
                "oldLimit",
                "newLimit"),
            TRANSPORT_STRUCTURED_TRACE(
                Stopped,
                74,
                Info,
                "listen address = '{1}'",
                "id",
                "listenAddress"),
            TRANSPORT_STRUCTURED_TRACE(
                SecurityCredentialExpiration,
                75,
                Info,
                "local credential expiration: {0}",
                "expiration"),
            TRANSPORT_STRUCTURED_TRACE(
                Anonymous_TargetAdded,
                76,
                Info,
                "anonymous target {1} {2}-{3}, totalAnonymous= {4}, total = {5}",
                "id",
                "targetId",
                "localAddress",
                "targetAddress",
                "totalAnonymous",
                "total"),
            TRANSPORT_STRUCTURED_TRACE(
                SecureSessionAuthorizationFailed,
                77,
                Warning,
                "authorization failure: {1}",
                "id",
                "error"),
            TRANSPORT_STRUCTURED_TRACE(
                SendQueueFull,
                79,
                Warning,
                "{1}-{2}: {3} ({4} bytes) in queue, limit={5}B, dropping messge {6}({7}B), Actor = {8}, Action = '{9}'",
                "id",
                "localAddress",
                "targetAddress",
                "queuedMsgCount",
                "queuedMsgSize",
                "queueSizeLimit",
                "newMessageId",
                "newMessageSize",
                "actor",
                "action"),
            TRANSPORT_STRUCTURED_TRACE(
                DropMessageOnAbort,
                80,
                Info,
                "dropping message {0}, Actor = {1}, Action = '{2}', fault = {3}",
                "messageId",
                "actor",
                "action",
                "fault"),
            TRANSPORT_STRUCTURED_TRACE(
                TargetReset,
                81,
                Info,
                "{1}-{2}: target reset",
                "id",
                "localAddress",
                "targetAddress"),
            TRANSPORT_STRUCTURED_TRACE(
                ConnectionAccepted,
                82,
                Info,
                "{1}: accepted connection from {2}",
                "id",
                "listenAddress",
                "remoteAddress"),
            TRANSPORT_STRUCTURED_TRACE(
                TcpKeepAliveTimeoutSet,
                83,
                Info,
                "TCP keep alive set to {1} seconds",
                "id",
                "tcpKeepAliveTimeout"),
            TRANSPORT_STRUCTURED_TRACE(
            OutgoingMessageExpirationSet,
                84,
                Info,
                "outgoing message expiration set to {1}",
                "id",
                "expiration"),
            TRANSPORT_STRUCTURED_TRACE(
                SendQueueSizeMaxIncreased,
                87,
                Noise,
                "{1} of {2}: send queue size max increased to {3} bytes",
                "id",
                "transportId",
                "owner",
                "queueSizeMax"),
            TRANSPORT_STRUCTURED_TRACE(
                OutgoingMessageExpired,
                88,
                Warning,
                "{1}-{2}: dropping message {3}, Actor = {4}, Action = '{5}'",
                "id",
                "localAddress",
                "targetAddress",
                "message",
                "actor",
                "action"),
            TRANSPORT_STRUCTURED_TRACE(
                TransportCreated,
                89,
                Info,
                "input listen address = '{1}', owner = '{2}', objCount incremented to {3}",
                "id",
                "inputListenAddress",
                "owner",
                "objCount"),
            DispatchActivity(
                Common::TraceTaskCodes::Transport,
                90,
                "Activity_Dispatch",
                Common::LogLevel::Info,
                "{1} {2} {3} {4}B {5}:{6} {7}",
                "id",
                "message",
                "isReply",
                "dispatchCountForThisReceive",
                "size",
                "actor",
                "action",
                "activityId"),
            TRANSPORT_STRUCTURED_TRACE(
                ConnectionFaulted,
                91,
                Info,
                "{1}-{2} faulted: {3}",
                "id",
                "localAddress",
                "targetAddress",
                "fault"),
            TRANSPORT_STRUCTURED_TRACE(
                Null_InvalidStateForSend,
                92,
                Warning,
                "{1}-{2}: cannot send, state = {3}, fault = {4}",
                "id",
                "localAddress",
                "remoteAddress",
                "state",
                "fault"),
            TRANSPORT_STRUCTURED_TRACE(
                ConnectionEstablished,
                93,
                Info,
                "{1}-{2} : socket local name = {3}",
                "id",
                "listenAddress",
                "remoteAddress",
                "socketLocalName"),
            TRANSPORT_STRUCTURED_TRACE(
                TransportDestroyed,
                94,
                Info,
                "objCount decremented to {1}",
                "id",
                "objCount"),
            TRANSPORT_STRUCTURED_TRACE(
                SuspendAccept,
                95,
                Info,
                "{1}: incomingConnectionCount={2}",
                "id",
                "listenAddress",
                "incomingTotal"),
            TRANSPORT_STRUCTURED_TRACE(
                ResumeAccept,
                96,
                Info,
                "incomingConnectionCount={0}",
                "incomingConnectionCount"),
            TRANSPORT_STRUCTURED_TRACE(
                AcceptResumed,
                97,
                Info,
                "listen address = {1}",
                "id",
                "listenAddress"),
            TRANSPORT_STRUCTURED_TRACE(
                AcceptResumeFailed,
                98,
                Error,
                "{1}: error={2}, will retry in {3}",
                "id",
                "listenAddress",
                "erorr",
                "retryDelay")
        {
        }
    };

    extern TransportEventSource const trace;
    extern Common::TextTraceComponent<Common::TraceTaskCodes::Transport> textTrace;
}
