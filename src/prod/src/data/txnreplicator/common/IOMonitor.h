// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
        //
        // Monitors the duration of a given event and reports health if necessary
        // Reports are initiated when a threshold of slow events is met within a specific time threshold
        //
        class IOMonitor final
            : public KObject<IOMonitor>
            , public KShared<IOMonitor>
            , public Data::Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::TR>
        {
            K_FORCE_SHARED(IOMonitor)

        public:
            static IOMonitor::SPtr Create(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in std::wstring const & operationName,
                __in Common::SystemHealthReportCode::Enum healthReportCode,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
                __in KAllocator & allocator);

            static Common::TimeSpan ComputeRefreshTime(__in Common::TimeSpan const & timeThreshold);

            //
            // Called by classes using the IOMonitor to indicate a slow operation has occurred (ref. PhysicalLogWriter/LogRecords.cpp)
            // Note that IOMonitor has no notion of what defines 'slow' but relies on the caller for correctness
            // Health is reported when 'x' slow events are detected within time period 'y'
            // Currently, x and y are limited to SlowLogIOCountThreshold and SlowLogIOTimeThreshold
            // NOTE: This function is NOT thread safe so caller must verify potential race conditions in each use case
            //
            void OnSlowOperation();
            void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

        private:
            IOMonitor(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in std::wstring const & operationName,
                __in Common::SystemHealthReportCode::Enum healthReportCode,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient);

            // Validate configuration values
            bool IsValidConfiguration();

            // Initialize KArray of stopwatch timestamps
            NTSTATUS InitializeIfNecessary();

            // Pointer to a config object shared throughout this replicator instance
            // SlowLogIODuration represents the duration after which a log operation is considered 'slow'
            // SlowLogIOCountThreshold represents the # of slow operations that must occurr prior to a health report
            // SlowLogIOTimeThreshold represents the time period in which SlowLogIOCountThreshold events must be reported 
            //  prior to a health report.
            // SlowLogIODuration > SlowLogIOTimeThreshold is required to ensure health reports are fired when necessary
            TxnReplicator::TRInternalSettingsSPtr const transactionalReplicatorConfig_;

            // Component to report health, provided from v1 replicator
            Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const healthClient_;

            // Operation name
            std::wstring const operationName_;

            // Health report code associated with this component
            Common::SystemHealthReportCode::Enum const healthReportCode_;

            // Array of timestamps updated with each slow operation timestamp
            // Initialized to count threshold with grow by percent 0 
            KArray<Common::StopwatchTime> slowOperationTimestamps_;

            // Time of latest slow operation health report
            Common::StopwatchTime lastSlowHealthReportTimestamp_;

            // Time in between health reports
            Common::TimeSpan refreshHealthReportTimeThreshold_;

            // Current index into the slow operations KArray
            ULONG currentSlowOpIndex_;
        };
}
