// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common\CommonEventSource.h"

#define DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(trace_name, ...) Common::TraceEventWriter<__VA_ARGS__> trace_name
#define RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(trace_name, trace_id, trace_type, trace_level, ...) \
    trace_name( \
        Common::TraceTaskCodes::ReliableMessagingSession, \
        trace_id, \
        #trace_name, \
        Common::LogLevel::##trace_level, \
        __VA_ARGS__)

#define RMTRACE (ReliableMessagingEventSource::Events)

#define PARTITION_TRACE(trace_name,myPartition) \
	switch (myPartition->PartitionId.KeyType) \
	{ \
	case FABRIC_PARTITION_KEY_TYPE_NONE: \
	RMTRACE->trace_name##Singleton(myPartition->ServiceInstanceName); \
		break; \
	case FABRIC_PARTITION_KEY_TYPE_INT64: \
	RMTRACE->trace_name##Int64(myPartition->ServiceInstanceName,myPartition->PartitionId.Int64RangeKey.IntegerKeyLow,myPartition->PartitionId.Int64RangeKey.IntegerKeyHigh); \
		break; \
	case FABRIC_PARTITION_KEY_TYPE_STRING: \
	RMTRACE->trace_name##Named(myPartition->ServiceInstanceName,myPartition->PartitionId.StringKey); \
		break; \
	}

#define PARTITION_TRACE_WITH_HRESULT(trace_name,myPartition,hr) \
	switch (myPartition->PartitionId.KeyType) \
	{ \
	case FABRIC_PARTITION_KEY_TYPE_NONE: \
	RMTRACE->trace_name##Singleton(myPartition->ServiceInstanceName,hr); \
		break; \
	case FABRIC_PARTITION_KEY_TYPE_INT64: \
	RMTRACE->trace_name##Int64(myPartition->ServiceInstanceName,myPartition->PartitionId.Int64RangeKey.IntegerKeyLow,myPartition->PartitionId.Int64RangeKey.IntegerKeyHigh,hr); \
		break; \
	case FABRIC_PARTITION_KEY_TYPE_STRING: \
	RMTRACE->trace_name##Named(myPartition->ServiceInstanceName,myPartition->PartitionId.StringKey,hr); \
		break; \
	}

#define PARTITION_TRACE_WITH_DESCRIPTION(trace_name,myPartition,description) \
	switch (myPartition->PartitionId.KeyType) \
	{ \
	case FABRIC_PARTITION_KEY_TYPE_NONE: \
	RMTRACE->trace_name##Singleton(myPartition->ServiceInstanceName,description); \
		break; \
	case FABRIC_PARTITION_KEY_TYPE_INT64: \
	RMTRACE->trace_name##Int64(myPartition->ServiceInstanceName,myPartition->PartitionId.Int64RangeKey.IntegerKeyLow,myPartition->PartitionId.Int64RangeKey.IntegerKeyHigh,description); \
		break; \
	case FABRIC_PARTITION_KEY_TYPE_STRING: \
	RMTRACE->trace_name##Named(myPartition->ServiceInstanceName,myPartition->PartitionId.StringKey,description); \
		break; \
	}

#define DECLARE_PARTTION_TRACE(trace_name) \
	DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(trace_name##Singleton,Common::Uri);			\
	DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(trace_name##Int64,Common::Uri,LONGLONG,LONGLONG);		\
	DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(trace_name##Named,Common::Uri,std::wstring);

#define CONSTRUCT_PARTITION_TRACE(trace_name,trace_type,trace_id) \
	RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(trace_name##Singleton, trace_id, ##trace_name, trace_type, "ServiceInstanceName = {0}", "ServiceInstanceName"), \
	RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(trace_name##Int64, trace_id + 1, ##trace_name, trace_type, "ServiceInstanceName = {0} Int64PartitionIdLow = {1} Int64PartitionIdHigh = {2}", "ServiceInstanceName", "Int64PartitionIdLow", "Int64PartitionIdHigh"), \
	RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(trace_name##Named, trace_id + 2, ##trace_name, trace_type, "ServiceInstanceName = {0} StringPartitionId = {1}", "ServiceInstanceName", "StringPartitionId")

#define DECLARE_PARTTION_TRACE_WITH_HRESULT(trace_name) \
	DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(trace_name##Singleton,Common::Uri,int32);			\
	DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(trace_name##Int64,Common::Uri,LONGLONG,LONGLONG,int32);		\
	DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(trace_name##Named,Common::Uri,std::wstring,int32);

#define CONSTRUCT_PARTITION_TRACE_WITH_HRESULT(trace_name,trace_type,trace_id) \
	RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(trace_name##Singleton, trace_id, ##trace_name, trace_type, "ServiceInstanceName = {0} HRESULT = {1}", "ServiceInstanceName", "HRESULT"), \
	RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(trace_name##Int64, trace_id + 1, ##trace_name, trace_type, "ServiceInstanceName = {0} Int64PartitionIdLow = {1} Int64PartitionIdHigh = {2} HRESULT = {3}", "ServiceInstanceName", "Int64PartitionIdLow", "Int64PartitionIdHigh", "HRESULT"), \
	RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(trace_name##Named, trace_id + 2, ##trace_name, trace_type, "ServiceInstanceName = {0} StringPartitionId = {1} HRESULT = {2}", "ServiceInstanceName", "StringPartitionId", "HRESULT")

#define DECLARE_PARTTION_TRACE_WITH_DESCRIPTION(trace_name) \
	DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(trace_name##Singleton,Common::Uri,Common::StringLiteral);			\
	DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(trace_name##Int64,Common::Uri,LONGLONG,LONGLONG,Common::StringLiteral);		\
	DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(trace_name##Named,Common::Uri,std::wstring,Common::StringLiteral);

#define CONSTRUCT_PARTITION_TRACE_WITH_DESCRIPTION(trace_name,trace_type,trace_id) \
	RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(trace_name##Singleton, trace_id, ##trace_name, trace_type, "ServiceInstanceName = {0} {1}", "ServiceInstanceName", "Description"), \
	RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(trace_name##Int64, trace_id + 1, ##trace_name, trace_type, "ServiceInstanceName = {0} Int64PartitionIdLow = {1} Int64PartitionIdHigh = {2} {3}", "ServiceInstanceName", "Int64PartitionIdLow", "Int64PartitionIdHigh", "Description"), \
	RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(trace_name##Named, trace_id + 2, ##trace_name, trace_type, "ServiceInstanceName = {0} StringPartitionId = {1} {2}", "ServiceInstanceName", "StringPartitionId", "Description")

#define DECLARE_POOLING_QUOTA_TEMPLATE_TRACE(trace_name) \
	DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(trace_name##AsyncSendOperation,Common::Guid,int32,int32);			\
	DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(trace_name##AsyncReceiveOperation,Common::Guid,int32,int32);			\
	DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(trace_name##QueuedInboundMessage,Common::Guid,int32,int32);

#define	CONSTRUCT_POOLING_QUOTA_TEMPLATE_TRACE(trace_name,trace_type,trace_id) \
	RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(trace_name##AsyncSendOperation, trace_id, ##trace_name, trace_type, "SessionId = {0} Count = {1} Quota = {2}", "SessionId", "Count", "Quota"), \
	RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(trace_name##AsyncReceiveOperation, trace_id + 1, ##trace_name, trace_type, "SessionId = {0} Count = {1} Quota = {2}", "SessionId", "Count", "Quota"), \
	RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(trace_name##QueuedInboundMessage, trace_id + 2, ##trace_name, trace_type, "SessionId = {0} Count = {1} Quota = {2}", "SessionId", "Count", "Quota")



namespace ReliableMessaging
{

    class ReliableMessagingEventSource
    { 
    public:
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AsyncOpenOutboundSessionStarted,std::wstring,Common::Guid);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AsyncOpenInboundSessionStarted,std::wstring,Common::Guid);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AsyncSendStarted,Common::Guid,int64);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AsyncReceiveStarted,Common::Guid);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(OpenSessionOutboundCompleting,std::wstring,Common::Guid,std::wstring);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(OpenSessionInboundCompleting,std::wstring,Common::Guid,std::wstring);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AsyncSendCompleted,Common::Guid,int64,Common::StringLiteral);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AsyncReceiveCompleted,Common::Guid,std::wstring);
         DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(InvalidPartitionDescription,LPCWSTR,int32,int32);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AsyncOperationCreateFailed,Common::StringLiteral);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AsyncOperationCreateSucceeded,Common::StringLiteral);		 
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(InvalidParameterForAPI,Common::StringLiteral,Common::StringLiteral,Common::StringLiteral);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(EmptyComObjectApiCallError,Common::StringLiteral,Common::StringLiteral);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(ComObjectCreationFailed,Common::StringLiteral);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(MessageSendFailed,std::wstring,Common::Guid,int64,std::wstring);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(CreateDequeueOperationFailed,std::wstring,Common::Guid,std::wstring);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(TransportReceivedMessage,Common::StringLiteral,Common::Guid,int64);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(CompletedReceiveOpFailed,std::wstring,Common::Guid,int32);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(KtlObjectCreateFailed,Common::StringLiteral);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(MessagingServiceCreateFailed,Common::StringLiteral);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(TransportException,Common::StringLiteral);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(OutOfMemory,Common::StringLiteral);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(CommunicationFailure,Common::StringLiteral,Common::StringLiteral, std::wstring);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(ProtocolFailure,Common::StringLiteral,Common::StringLiteral);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(MessageReceived,std::wstring);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(MessageDropped,Common::Guid,int64,Common::StringLiteral);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(SessionEraseFailure,std::wstring,Common::StringLiteral);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(InvalidOperation,Common::StringLiteral,Common::StringLiteral);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(FailureCompletionFromInternalFunction,std::wstring,Common::StringLiteral,std::wstring);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(LastServiceActivityReleased,Common::StringLiteral);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(MultipleOpenSessionAcksReceived,Common::Guid,std::wstring);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(TransportStartupFailure,std::wstring);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(KtlObjectCreationFailed,Common::StringLiteral);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(InvalidMessage,std::wstring,Common::StringLiteral);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(SendWindowSegment,Common::StringLiteral,Common::Guid,int64,int64,int32);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(MessageAckDropped,Common::Guid,int64,Common::StringLiteral);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(StartingAckTimer,Common::Guid,int64,Common::StringLiteral);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(StoppingAckTimer,Common::Guid,Common::StringLiteral);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(SendingMessageAck,Common::Guid,int64,int32);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AsyncCloseOutboundSessionStarted,std::wstring,Common::Guid);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AsyncCloseInboundSessionStarted,std::wstring,Common::Guid);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(CloseSessionOutboundCompleting,std::wstring,Common::Guid,std::wstring);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(CloseSessionInboundCompleting,std::wstring,Common::Guid,std::wstring);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(MessageSendAfterSessionClose,std::wstring,Common::Guid,int64);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(MessageReceiveAfterSessionClose,std::wstring,Common::Guid,int64);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(MessageAckFailed,std::wstring,Common::Guid,int64,std::wstring);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(MultipleCloseSessionAcksReceived,Common::Guid,std::wstring);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(SessionRequestedCallbackStarting,std::wstring,Common::Guid);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(SessionRequestedCallbackReturned,std::wstring,Common::Guid,int32);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(SessionClosedCallbackStarting,std::wstring,Common::Guid);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(SessionClosedCallbackReturned,std::wstring,Common::Guid);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AbortInboundSessionStarted,std::wstring,Common::Guid,Common::StringLiteral);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AbortInboundSessionCompleting,std::wstring,Common::Guid,std::wstring);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AbortOutboundSessionStarted,std::wstring,Common::Guid,Common::StringLiteral);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AbortOutboundSessionCompleting,std::wstring,Common::Guid,std::wstring);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(InboundSessionClosureRace,Common::Guid);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(OutboundSessionClosureRace,Common::Guid);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(InboundMessageBuffered,Common::Guid,int64);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(CleanupInboundSessionStarted,std::wstring,Common::Guid,Common::StringLiteral);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(CleanupInboundSessionCompleted,std::wstring,Common::Guid,bool);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(CleanupOutboundSessionStarted,std::wstring,Common::Guid,Common::StringLiteral);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(CleanupOutboundSessionCompleted,std::wstring,Common::Guid,bool);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(StartedMessageAckProcessing,Common::Guid,int64);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(FinishedMessageAckProcessing,Common::Guid,int64);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(ConflictingInboundSessionAborted,std::wstring,Common::Guid,Common::Guid);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(ServiceActivityAcquired,std::wstring,Common::StringLiteral);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(ServiceActivityReleased,std::wstring,Common::StringLiteral);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(BeginCreateOutboundSessionStart,std::wstring, Common::Guid);
		 DECLARE_RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(BeginCreateOutboundSessionFinish,std::wstring, Common::Guid);
		 	 
		 DECLARE_PARTTION_TRACE(ComReliableSessionManagerCreateStarted);
		 DECLARE_PARTTION_TRACE(ComReliableSessionManagerCreateCompleted);
		 DECLARE_PARTTION_TRACE_WITH_HRESULT(CreateSessionManagerServiceFailed);
		 DECLARE_PARTTION_TRACE_WITH_DESCRIPTION(SessionManagerServiceUnableToCreate);
		 DECLARE_PARTTION_TRACE(BeginCreateOutboundSessionStarted);
		 DECLARE_PARTTION_TRACE_WITH_HRESULT(BeginCreateOutboundSessionCompleted);
		 DECLARE_PARTTION_TRACE(AsyncCreateOutboundSessionStarted);
		 DECLARE_PARTTION_TRACE(AsyncRegisterInboundSessionCallbackStarted);
		 DECLARE_PARTTION_TRACE(AsyncUnregisterInboundSessionCallbackStarted);
		 DECLARE_PARTTION_TRACE(SessionManagerShutdownStarted);
		 DECLARE_PARTTION_TRACE(SessionManagerShutdownCompleted);
		 DECLARE_PARTTION_TRACE(SessionManagerStartCloseStarted);
		 DECLARE_PARTTION_TRACE(SessionManagerFinishCloseStarted);

		 DECLARE_PARTTION_TRACE_WITH_HRESULT(AsyncCreateOutboundSessionCompleted);
		 DECLARE_PARTTION_TRACE_WITH_HRESULT(AsyncRegisterInboundSessionCallbackCompleted);
		 DECLARE_PARTTION_TRACE_WITH_HRESULT(AsyncUnregisterInboundSessionCallbackCompleted);
		 DECLARE_PARTTION_TRACE_WITH_HRESULT(SessionManagerStartCloseCompleted);
		 DECLARE_PARTTION_TRACE_WITH_HRESULT(SessionManagerFinishCloseCompleted);

		 DECLARE_POOLING_QUOTA_TEMPLATE_TRACE(GetFromQuota);
		 DECLARE_POOLING_QUOTA_TEMPLATE_TRACE(ReturnToQuota);

        ReliableMessagingEventSource() :
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AsyncOpenOutboundSessionStarted, 10, "Async Open Outbound Session Started", Info, "SessionId = {1}", "id", "SessionId"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AsyncOpenInboundSessionStarted, 11, "Async Open Inbound Session Started", Info, "SessionId = {1}", "id", "SessionId"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AsyncSendStarted, 12, "Async Send Started", Noise, "SessionId = {0} SequenceNumber = {1}", "SessionId", "SequenceNumber"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AsyncReceiveStarted, 13, "Async Receive Started", Noise, "SessionId = {0}", "SessionId"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AsyncSendCompleted, 14, "Async Send Completed", Noise, "SessionId = {0} SequenceNumber = {1} EraseReason = {2}", "SessionId", "SequenceNumber", "EraseReason"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AsyncReceiveCompleted, 15, "Async Receive Completed", Noise, "SessionId = {0} Result = {1}", "SessionId", "ErrorCode"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(InvalidPartitionDescription, 16, "Service Partition Description Parsing failure", Error, "ServiceInstanceName = {0} PartitionKeyType = {1} HRESULT = {2}", "ServiceInstanceName", "PartitionKeyType", "HRESULT"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AsyncOperationCreateFailed, 17, "AsyncOperationCreate Failed Insufficient Resources", Error, "Operation = {0}", "AsyncOperation"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AsyncOperationCreateSucceeded, 18, "AsyncOperationCreate Succeeded", Noise, "Operation = {0}", "AsyncOperation"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(InvalidParameterForAPI, 19, "API failed due to invalid parameter", Error, "API {0} Parameter {1} {2}", "API", "Parameter", "Comment"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(EmptyComObjectApiCallError, 20, "API failed due to Object Closed", Error, "{0} Called on an empty {1} ", "API", "COMObjectType"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(ComObjectCreationFailed, 21, "COM Object Creation Failed Insufficient Resources", Error, "Object Type {0} ", "COMObjectType"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(MessageSendFailed, 22, "Message Send Failed", Warning, "SessionId = {1} SequenceNumber = {2} ErrorCode = {3}", "id", "SessionId", "SequenceNumber", "ErrorCode"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(CreateDequeueOperationFailed, 23, "CreateDequeueOperation Failed", Warning, "SessionId = {1} ErrorCode = {2}", "id", "SessionId", "ErrorCode"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(TransportReceivedMessage, 24, "Transport Received Message", Noise, "ActionCode = {0} SessionId = {1} SequenceNumber = {2}", "ActionCode", "SessionId", "SequenceNumber"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(CompletedReceiveOpFailed, 25, "CompletedReceiveOp Failed", Error, "SessionId = {1} NTSTATUS = {2}", "id", "SessionId", "NTSTATUS"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(KtlObjectCreateFailed, 26, "AsyncQueueObjectCreate Failed Insufficient Resources", Error, "AsyncQueueObjectType = {0}", "AsyncQueueObjectType"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(MessagingServiceCreateFailed, 27, "MessagingServiceCreate Failed Insufficient Resources", Error, "MessagingServiceType = {0}", "MessagingServiceType"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(TransportException, 28, "Transport Function Failed Insufficient Resources", Error, "MessagingCodeContext = {0}", "MessagingCodeContext"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(OutOfMemory, 29, "Messaging Function Failed Insufficient Resources", Error, "MessagingCodeContext = {0}", "MessagingCodeContext"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(CommunicationFailure, 30, "Communication Failed", Warning, "CommunicationFailureType = {0} MessagingCodeContext = {1} Error = {2}", "CommunicationFailureType", "MessagingCodeContext", "ErrorCode"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(ProtocolFailure, 31, "Protocol Action Failed", Error, "ProtocolActionType = {0} MessagingCodeContext = {1} ", "ProtocolActionType", "MessagingCodeContext"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(OpenSessionOutboundCompleting, 32, "Async Open Session Completed", Info, "SessionId = {1} ErrorCode = {2}", "id", "SessionId", "ErrorCode"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(OpenSessionInboundCompleting, 33, "Async Open Session Completed", Info, "SessionId = {1} ErrorCode = {2}", "id", "SessionId", "ErrorCode"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(MessageReceived, 34, "Message Received", Noise, "Transport Message Received From Endpoint = {0}", "EndPoint"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(MessageDropped, 35, "Message Dropped", Noise, "SessionId: {0} Sequence Number: {1} Reason: {2}", "SessionId", "SequenceNumber", "Reason"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(SessionEraseFailure, 36, "SessionEraseFailure", Info, "SessionPartitionMap Erase Failed Due to Race In {1}", "id", "MessagingCodeContext"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(InvalidOperation, 37, "Invalid Operation", Error, "CodeContext = {0} Invalid {1}", "MessagingCodeContext", "InvalidReason"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(FailureCompletionFromInternalFunction, 38, "FailureCompletionFromInternalFunction", Warning, "CodeContext = {1} ErrorCode = {2}", "id", "MessagingCodeContext", "ErrorCode"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(LastServiceActivityReleased, 39, "LastServiceActivityReleased", Noise, "CodeContext = {0}", "MessagingCodeContext"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(MultipleOpenSessionAcksReceived, 40, "Multiple OpenSessionAcks Received", Info, "SessionId = {0} ErrorCode = {1}", "SessionId", "ErrorCode"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(TransportStartupFailure, 41, "Transport Startup Failure", Error, "ErrorCode = {0}", "ErrorCode"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(KtlObjectCreationFailed, 42, "KTL Object Creation Failed Insufficient Resources", Error, "Object Type {0} ", "KTLObjectType"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(InvalidMessage, 43, "Invalid Message", Error, "From: {0} Reason: {1}", "EndPoint", "Reason"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(SendWindowSegment, 44, "SendWindowSegment", Noise, "Context: {0} SessionId: {1} StartSequenceNumber: {2} EndSequenceNumber: {3} ActuallySent: {4}", "Context", "SessionId", "FirstSequenceNumber", "LastSequenceNumber", "ActuallySent"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(MessageAckDropped, 45, "Message Ack Dropped", Noise, "SessionId: {0} Sequence Number: {1} Reason: {2}", "SessionId", "SequenceNumber", "Reason"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(StartingAckTimer, 46, "Ack Timer Started", Noise, "SessionId: {0} Sequence Number: {1} Reason: {2}", "SessionId", "SequenceNumber", "Reason"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(StoppingAckTimer, 47, "Ack Timer Stopped", Noise, "SessionId: {0} Reason: {1}", "SessionId", "Reason"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(SendingMessageAck, 48, "Sending Message Ack", Noise, "SessionId: {0} Sequence Number: {1} Remaining BufferCount {2}", "SessionId", "SequenceNumber", "BufferCount"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AsyncCloseOutboundSessionStarted, 49, "Async Close Outbound Session Started", Info, "SessionId = {1}", "id", "SessionId"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AsyncCloseInboundSessionStarted, 50, "Async Close Inbound Session Started", Info, "SessionId = {1}", "id", "SessionId"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(CloseSessionOutboundCompleting, 51, "Async Open Session Completed", Info, "SessionId = {1} ErrorCode = {2}", "id", "SessionId", "ErrorCode"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(CloseSessionInboundCompleting, 52, "Async Open Session Completed", Info, "SessionId = {1} ErrorCode = {2}", "id", "SessionId", "ErrorCode"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(MessageSendAfterSessionClose, 53, "Message Send After Session Close Rejected", Warning, "SessionId = {1} SequenceNumber = {2}", "id", "SessionId", "SequenceNumber"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(MessageReceiveAfterSessionClose, 54, "Message Receive After Session Close Rejected", Warning, "SessionId = {1} SequenceNumber = {2}", "id", "SessionId", "SequenceNumber"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(MessageAckFailed, 55, "Message Ack Failed", Error, "SessionId = {1} SequenceNumber = {2} ErrorCodeValue = {3}", "id", "SessionId", "SequenceNumber", "ErrorCode"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(MultipleCloseSessionAcksReceived, 56, "Multiple CloseSessionAcks Received", Info, "SessionId = {0} ErrorCodeValue = {1}", "SessionId", "ErrorCodeValue"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(SessionRequestedCallbackStarting, 57, "SessionRequested Callback Starting", Info, "SessionId = {1}", "id", "SessionId"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(SessionRequestedCallbackReturned, 58, "SessionRequested Callback Returned", Info, "SessionId = {1} Response = {2}", "id", "SessionId", "Response"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(SessionClosedCallbackStarting, 59, "SessionClosed Callback Starting", Info, "SessionId = {1}", "id", "SessionId"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(SessionClosedCallbackReturned, 60, "SessionClosed Callback Returned", Info, "SessionId = {1}", "id", "SessionId"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AbortInboundSessionStarted, 61, "Abort Inbound Session Started", Info, "SessionId = {1} Signal = {2}", "id", "SessionId", "Signal"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AbortInboundSessionCompleting, 62, "Abort Inbound Session Completed", Info, "SessionId = {1} ErrorCode = {2}", "id", "SessionId", "ErrorCode"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AbortOutboundSessionStarted, 63, "Abort Outbound Session Started", Info, "SessionId = {1} Signal = {2}", "id", "SessionId", "Signal"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(AbortOutboundSessionCompleting, 64, "Abort Outbound Session Completed", Info, "SessionId = {1} ErrorCode = {2}", "id", "SessionId", "ErrorCode"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(InboundSessionClosureRace, 65, "Inbound Session Closure Race", Info, "SessionId = {0}", "SessionId"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(OutboundSessionClosureRace, 66, "Outbound Session Closure Race", Info, "SessionId = {0}", "SessionId"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(InboundMessageBuffered, 67, "Inbound Message Buffered", Noise, "SessionId = {0} SequenceNumber = {1}", "SessionId", "SequenceNumber"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(CleanupInboundSessionStarted, 68, "CleanupInboundSession Started", Info, "SessionId = {1} Signal = {2}", "id", "SessionId", "Signal"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(CleanupInboundSessionCompleted, 69, "CleanupInboundSession Completed", Info, "SessionId = {1} Success = {2}", "id", "SessionId", "Success"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(CleanupOutboundSessionStarted, 70, "CleanupOutboundSession Started", Info, "SessionId = {1} Signal = {2}", "id", "SessionId", "Signal"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(CleanupOutboundSessionCompleted, 71, "CleanupOutboundSession Completed", Info, "SessionId = {1} Success = {2}", "id", "SessionId", "Success"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(StartedMessageAckProcessing, 72, "Started Message Ack Processing", Noise, "SessionId = {0} SequenceNumber = {1}", "SessionId", "SequenceNumber"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(FinishedMessageAckProcessing, 73, "Finished Message Ack Processing", Noise, "SessionId = {0} SequenceNumber = {1}", "SessionId", "SequenceNumber"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(ConflictingInboundSessionAborted, 74, "Conflicting InboundSession Aborted", Info, "New SessionId = {1} Aborted SessionId = {2}", "id", "NewSessionId", "AbortedSessionId"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(ServiceActivityAcquired, 75, "ServiceActivityAcquired", Info, "CodeContext = {1}", "id", "MessagingCodeContext"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(ServiceActivityReleased, 76, "ServiceActivityReleased", Info, "CodeContext = {1}", "id", "MessagingCodeContext"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(BeginCreateOutboundSessionStart, 77, "BeginCreateOutboundSessionStart", Info, "SessionId = {1}", "id", "SessionId"),
			RELIABLE_MESSAGING_COMMON_STRUCTURED_TRACE(BeginCreateOutboundSessionFinish, 78, "BeginCreateOutboundSessionFinish", Info, "SessionId = {1}", "id", "SessionId"),
			
			CONSTRUCT_PARTITION_TRACE(ComReliableSessionManagerCreateStarted,Info,100),							// generates trace_ids 100, 101, 102
			CONSTRUCT_PARTITION_TRACE(ComReliableSessionManagerCreateCompleted,Info,103),						// generates trace_ids 103, 104, 105
			CONSTRUCT_PARTITION_TRACE_WITH_HRESULT(CreateSessionManagerServiceFailed,Error,106),				// generates trace_ids 106, 107, 108
			CONSTRUCT_PARTITION_TRACE_WITH_DESCRIPTION(SessionManagerServiceUnableToCreate,Error,109),			// generates trace_ids 109, 110, 111
			CONSTRUCT_PARTITION_TRACE(BeginCreateOutboundSessionStarted,Info,112),								// generates trace_ids 112, 113, 114
			CONSTRUCT_PARTITION_TRACE_WITH_HRESULT(BeginCreateOutboundSessionCompleted,Info,115),				// generates trace_ids 115, 116, 117
			CONSTRUCT_PARTITION_TRACE(AsyncCreateOutboundSessionStarted,Noise,118),								// generates trace_ids 118, 119, 120
			CONSTRUCT_PARTITION_TRACE(AsyncRegisterInboundSessionCallbackStarted,Noise,121),					// generates trace_ids 121, 122, 123
			CONSTRUCT_PARTITION_TRACE(AsyncUnregisterInboundSessionCallbackStarted,Noise,124),					// generates trace_ids 124, 125, 126
			CONSTRUCT_PARTITION_TRACE(SessionManagerShutdownStarted,Info,127),									// generates trace_ids 127, 128, 129
			CONSTRUCT_PARTITION_TRACE(SessionManagerShutdownCompleted,Info,130),								// generates trace_ids 130, 131, 132
			CONSTRUCT_PARTITION_TRACE(SessionManagerStartCloseStarted,Info,133),								// generates trace_ids 133, 134, 135
			CONSTRUCT_PARTITION_TRACE(SessionManagerFinishCloseStarted,Info,136),								// generates trace_ids 136, 137, 138
			
			CONSTRUCT_PARTITION_TRACE_WITH_HRESULT(AsyncCreateOutboundSessionCompleted,Noise,139),				// generates trace_ids 139, 140, 141
			CONSTRUCT_PARTITION_TRACE_WITH_HRESULT(AsyncRegisterInboundSessionCallbackCompleted,Noise,142),		// generates trace_ids 142, 143, 144
			CONSTRUCT_PARTITION_TRACE_WITH_HRESULT(AsyncUnregisterInboundSessionCallbackCompleted,Noise,145),	// generates trace_ids 145, 146, 147
			CONSTRUCT_PARTITION_TRACE_WITH_HRESULT(SessionManagerStartCloseCompleted,Info,148),					// generates trace_ids 148, 149, 150
			CONSTRUCT_PARTITION_TRACE_WITH_HRESULT(SessionManagerFinishCloseCompleted,Info,151),				// generates trace_ids 151, 152, 153
			

			CONSTRUCT_POOLING_QUOTA_TEMPLATE_TRACE(GetFromQuota,Noise,200),										// generates trace_ids 200, 201, 202
			CONSTRUCT_POOLING_QUOTA_TEMPLATE_TRACE(ReturnToQuota,Noise,203)										// generates trace_ids 203, 204, 205

        {
        }

		static Common::Global<ReliableMessagingEventSource> Events;
    };
        
}
