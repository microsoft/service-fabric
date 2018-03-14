// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    #define DECLARE_TR_STRUCTURED_TRACE(trace_name, ...) Common::TraceEventWriter<__VA_ARGS__> trace_name
    #define TR_STRUCTURED_TRACE(trace_name, trace_id, trace_level, ...) \
                trace_name( \
                    Common::TraceTaskCodes::TR, \
                    trace_id, \
                    #trace_name, \
                    Common::LogLevel::trace_level, \
                    __VA_ARGS__)

    class TREventSource
    {
    public: 
        DECLARE_TR_STRUCTURED_TRACE(Exception, Common::Guid, LONG64, std::wstring, int, std::wstring);
        DECLARE_TR_STRUCTURED_TRACE(ExceptionError, Common::Guid, LONG64, std::wstring, int, std::wstring);
        DECLARE_TR_STRUCTURED_TRACE(Ctor, Common::Guid, LONG64, std::wstring, uintptr_t);
        DECLARE_TR_STRUCTURED_TRACE(Dtor, Common::Guid, LONG64, std::wstring, uintptr_t);

        // TransactionalReplicator 
        DECLARE_TR_STRUCTURED_TRACE(SkippingRecovery, Common::Guid, LONG64);
        DECLARE_TR_STRUCTURED_TRACE(CloseAsync, Common::Guid, LONG64);
        DECLARE_TR_STRUCTURED_TRACE(Abort, Common::Guid, LONG64);
        DECLARE_TR_STRUCTURED_TRACE(ChangeRole, Common::Guid, LONG64, int, int);
        DECLARE_TR_STRUCTURED_TRACE(OnServiceOpenAsync, Common::Guid, LONG64);
        DECLARE_TR_STRUCTURED_TRACE(OnServiceCloseAsync, Common::Guid, LONG64);

        // ApiMonitoringWrapper
        DECLARE_TR_STRUCTURED_TRACE(ApiMonitoringWrapperInfo, Common::Guid, LONG64, std::wstring, std::wstring);
        DECLARE_TR_STRUCTURED_TRACE(ApiMonitoringWrapperWarning, Common::Guid, LONG64, std::wstring, std::wstring);

        // IOMonitor
        DECLARE_TR_STRUCTURED_TRACE(TRHealthTrackerWarning, Common::Guid, LONG64, std::wstring, std::wstring, Common::ErrorCode);

        TREventSource() :
            // Structured Trace reserved areas.
            // 1. TraceType: [0, 3]
            // 2. General [4, 10]
            // 3. TransactionalReplicator [16, 35]
            //
            // Note: TraceId starts from 4, this is because 0-3 has already been defined as trace type: Info, Warning, Error, Noise.
            TR_STRUCTURED_TRACE(Exception, 4, Warning, "{1}: {2} Failed. Exception {3:x}. {4}", "id", "replicaid", "message", "exceptioncode", "action"),
            TR_STRUCTURED_TRACE(ExceptionError, 5, Error, "{1}: {2} Failed. Exception {3:x}. {4}", "id", "replicaid", "message", "exceptioncode", "action"),
            TR_STRUCTURED_TRACE(Ctor, 6, Info, "{1}: {2} this={3}", "id", "replicaid", "className", "this"),
            TR_STRUCTURED_TRACE(Dtor, 7, Info, "{1}: {2} this={3}", "id", "replicaid", "className", "this"),

            // TransactionalReplicator 
            TR_STRUCTURED_TRACE(SkippingRecovery, 16, Info, "{1}: OnServiceOpenAsync: SkippingRecovery", "id", "replicaid"),
            TR_STRUCTURED_TRACE(CloseAsync, 17, Info, "{1}: CloseAsync", "id", "replicaid"),
            TR_STRUCTURED_TRACE(Abort, 18, Info, "{1}", "id", "replicaid"),
            TR_STRUCTURED_TRACE(ChangeRole, 19, Info, "{1}: ChangeRole from {2} to {3}", "id", "replicaid", "oldrole", "newrole"),
            TR_STRUCTURED_TRACE(OnServiceOpenAsync, 20, Info, "{1}", "id", "replicaid"),
            TR_STRUCTURED_TRACE(OnServiceCloseAsync, 21, Info, "{1}", "id", "replicaid"),

            // ApiMonitoringWrapper
            TR_STRUCTURED_TRACE(ApiMonitoringWrapperInfo, 25, Info, "{1} : Property {2}. HealthState {3}", "id", "replicaid", "property", "healthstate"),
            TR_STRUCTURED_TRACE(ApiMonitoringWrapperWarning, 26, Warning, "{1} : Property {2}. HealthState {3}", "id", "replicaid", "property", "healthstate"),

            // IOMonitor
            TR_STRUCTURED_TRACE(TRHealthTrackerWarning, 30, Warning, "{1} : {2} - {3}. ReportReplicatorHealth returned {4}", "id", "replicaid", "operationname", "healthreportdescription", "healthreportstatus")
        {
        }

        static Common::Global<TREventSource> Events;
    };
}
