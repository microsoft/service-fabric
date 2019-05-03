// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{

#define HEALTH_CLIENT_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
    STRUCTURED_TRACE(trace_name, HealthClient, trace_id, trace_level, __VA_ARGS__)

    class HealthClientEventSource
    { 
    public:
        DECLARE_STRUCTURED_TRACE( Ctor, std::wstring);
        DECLARE_STRUCTURED_TRACE( Dtor, std::wstring);
        DECLARE_STRUCTURED_TRACE( Open, std::wstring);
        DECLARE_STRUCTURED_TRACE( Close, std::wstring);
        DECLARE_STRUCTURED_TRACE( CloseDropReports, std::wstring, uint64);
        DECLARE_STRUCTURED_TRACE( Send, std::wstring, Common::ActivityId, uint64, uint64, uint64, uint64, uint64, bool, Common::DateTime);
        DECLARE_STRUCTURED_TRACE( SendFailure, std::wstring, Common::ErrorCodeValue::Trace);
        DECLARE_STRUCTURED_TRACE( Reject, std::wstring, uint64, std::wstring, std::wstring );
        DECLARE_STRUCTURED_TRACE( ProcessReply, std::wstring, Common::ActivityId, uint64, uint64, int, Common::DateTime);
        DECLARE_STRUCTURED_TRACE( SequenceStreamCtor, std::wstring, HealthReportingComponent::SequenceStream);
        DECLARE_STRUCTURED_TRACE( SequenceStreamDtor, std::wstring, HealthReportingComponent::SequenceStream);
        DECLARE_STRUCTURED_TRACE( SequenceStream, uint16, std::wstring, FABRIC_INSTANCE_ID, SequenceStreamState::Trace, LONGLONG, std::wstring, uint64);
        DECLARE_STRUCTURED_TRACE( AddReport, std::wstring, HealthReportingComponent::SequenceStream, std::wstring, LONGLONG, bool);
        DECLARE_STRUCTURED_TRACE( IgnoredReport, std::wstring, HealthReportingComponent::SequenceStream, std::wstring, LONGLONG, LONGLONG);
        DECLARE_STRUCTURED_TRACE( CompleteReport, std::wstring, HealthReportingComponent::SequenceStream, std::wstring, LONGLONG, Common::ErrorCodeValue::Trace);
        DECLARE_STRUCTURED_TRACE( PreInitialize, std::wstring, HealthReportingComponent::SequenceStream, uint64);
        DECLARE_STRUCTURED_TRACE( PostInitialize, std::wstring, int64, int64, HealthReportingComponent::SequenceStream);
        DECLARE_STRUCTURED_TRACE( IgnoredReportDueToInstance, std::wstring, HealthReportingComponent::SequenceStream, std::wstring, FABRIC_INSTANCE_ID, FABRIC_INSTANCE_ID, bool );
        DECLARE_STRUCTURED_TRACE( CleanReports, std::wstring, std::wstring, std::wstring, FABRIC_INSTANCE_ID, FABRIC_INSTANCE_ID, bool );
        DECLARE_STRUCTURED_TRACE( IgnoredReportDueToEntityDeleted, std::wstring, HealthReportingComponent::SequenceStream, std::wstring, FABRIC_INSTANCE_ID );
        DECLARE_STRUCTURED_TRACE( CleanReportsHMEntityDeleted, std::wstring, std::wstring, std::wstring, FABRIC_INSTANCE_ID );
        DECLARE_STRUCTURED_TRACE( IgnoredReportDueToError, std::wstring, HealthReportingComponent::SequenceStream, std::wstring, Common::ErrorCode, std::wstring );
        DECLARE_STRUCTURED_TRACE( IgnoreReportResultRetryableError, std::wstring, HealthReportingComponent::SequenceStream, std::wstring, FABRIC_SEQUENCE_NUMBER, Common::ErrorCode );
        DECLARE_STRUCTURED_TRACE( SequenceStreamProcessed, std::wstring, Common::ActivityId, FABRIC_INSTANCE_ID, FABRIC_SEQUENCE_NUMBER, HealthReportingComponent::SequenceStream );
        DECLARE_STRUCTURED_TRACE( IgnoreSequenceStreamResult, std::wstring, Common::ActivityId, FABRIC_INSTANCE_ID, FABRIC_SEQUENCE_NUMBER, HealthReportingComponent::SequenceStream );
        DECLARE_STRUCTURED_TRACE( IgnoreReportResultNonActionableError, std::wstring, Common::ActivityId, std::wstring, FABRIC_SEQUENCE_NUMBER, Common::ErrorCode);

        HealthClientEventSource() :
        HEALTH_CLIENT_STRUCTURED_TRACE( Ctor, 4, Info, "Ctor", "id"),
        HEALTH_CLIENT_STRUCTURED_TRACE( Dtor, 5, Info, "Dtor", "id"),
        HEALTH_CLIENT_STRUCTURED_TRACE( Open, 6, Info, "Open", "id"),
        HEALTH_CLIENT_STRUCTURED_TRACE( Close, 7, Info, "Close", "id"),
        HEALTH_CLIENT_STRUCTURED_TRACE( CloseDropReports, 8, Warning, "Closing client. There are {1} reports pending that may not reach Health Manager.", "id", "droppedReports"),
        HEALTH_CLIENT_STRUCTURED_TRACE( Send, 10, Info, "{1}: sequence streams: {2}/{3}/{4}, reports: {5}/{6}, throttled: {7}. ScheduledFireTime: {8}", "id", "activityId", "sequenceStreamGetProgress", "sequenceStreamCount", "ssRequestCount", "sentCount", "reportCount", "throttled", "scheduledFireTime"),
        HEALTH_CLIENT_STRUCTURED_TRACE( SendFailure, 11, Warning, "Report failed with {1}", "id", "error"),
        HEALTH_CLIENT_STRUCTURED_TRACE( Reject, 13, Warning, "Cannot add report as max reports reached: {1}. Rejected report: {2}+{3}", "id", "reportCount", "reportSource", "reportProperty" ),
        HEALTH_CLIENT_STRUCTURED_TRACE( ProcessReply, 14, Info, "{1}: Reply processed: sequence streams {2}, reports {3}, serviceTooBusy={4}. ScheduledFireTime: {5}", "id", "activityId", "sequenceStreamCount", "reportCount", "serviceTooBusyCount", "scheduledFireTime"),
        HEALTH_CLIENT_STRUCTURED_TRACE( SequenceStreamCtor, 15, Info, "Ctor: {1}", "id", "sequencestream"),
        HEALTH_CLIENT_STRUCTURED_TRACE( SequenceStreamDtor, 16, Info, "Dtor: {1}", "id", "sequencestream"),
        HEALTH_CLIENT_STRUCTURED_TRACE( SequenceStream, 17, Info, "[{1}:{2}|{3}] acked: [0,{4}) ranges: {5}, reportsSize: {6}", "contextSequenceId", "sequencestreamid", "instance", "state", "ackeduptolsn", "ranges", "reportsSize"),
        HEALTH_CLIENT_STRUCTURED_TRACE( AddReport, 18, Info, "{1} adding {2} sn={3}, immediate={4}", "id", "sequencestream", "entityId", "sequenceNumber", "immediate"),
        HEALTH_CLIENT_STRUCTURED_TRACE( IgnoredReport, 20, Info, "{1} ignored {2} knownsn={3} receivedsn={4}", "id", "sequencestream", "entityId", "knownsequencenumber", "receivedsequencenumber"),
        HEALTH_CLIENT_STRUCTURED_TRACE( CompleteReport, 21, Noise, "{1} completed {2} sn={3} error {4}", "id", "sequencestream", "entityId", "sequenceNumber", "error"),
        HEALTH_CLIENT_STRUCTURED_TRACE( PreInitialize, 25, Info, "PreInitialize: {1}, new instance {2}", "id", "sequencestream", "instance"),
        HEALTH_CLIENT_STRUCTURED_TRACE( PostInitialize, 26, Info, "PostInitialize {1} invalidate {2}: {3}", "id", "sequence", "invalidateSequence", "sequencestream"),
        HEALTH_CLIENT_STRUCTURED_TRACE( IgnoredReportDueToInstance, 27, Info, "{1} ignored {2} knowninstance={3} receivedinstance={4}, isDelete={5}", "id", "sequencestream", "entityId", "knowninstance", "receivedinstance", "isDelete"),
        HEALTH_CLIENT_STRUCTURED_TRACE( CleanReports, 28, Info, "{1} clean reports due to {2}: knowninstance={3} receivedinstance={4}, isDelete={5}", "id", "sequencestream", "entityId", "knowninstance", "receivedinstance", "isDelete"),
        HEALTH_CLIENT_STRUCTURED_TRACE( IgnoredReportDueToEntityDeleted, 29, Info, "{1} ignored {2} knowninstance={3}", "id", "sequencestream", "entityId", "knowninstance"),
        HEALTH_CLIENT_STRUCTURED_TRACE( CleanReportsHMEntityDeleted, 30, Info, "{1} received HealthEntityDeleted for {2} knowninstance={3}", "id", "sequencestream", "entityId", "knowninstance"),
        HEALTH_CLIENT_STRUCTURED_TRACE( IgnoredReportDueToError, 31, Info, "{1} ignored {2}: error {3} {4}", "id", "sequencestream", "entityId", "error", "errorMessage" ),
        HEALTH_CLIENT_STRUCTURED_TRACE (IgnoreReportResultRetryableError, 32, Info, "{1}: ignore result {2} sn={3}, error={4} (retryable error).", "id", "sequencestream", "reportId", "sn", "error" ),
        HEALTH_CLIENT_STRUCTURED_TRACE( SequenceStreamProcessed, 33, Info, "{1}: SequenceResult with instance={2}, upToLsn={3} processed for {4}", "id", "activityId", "ssInstance", "ssUpToLsn", "sequencestream" ),
        HEALTH_CLIENT_STRUCTURED_TRACE( IgnoreSequenceStreamResult, 34, Info, "{1}: Ignore SequenceResult with instance={2}, upToLsn={3} processed for {4}, stale", "id", "activityId", "ssInstance", "ssUpToLsn", "sequencestream" ),
        HEALTH_CLIENT_STRUCTURED_TRACE( IgnoreReportResultNonActionableError, 35, Info, "{1}: ignore result {2} sn={3}, error={4}.", "id", "activityId", "reportId", "sn", "error")
        {
        }
    };
}
