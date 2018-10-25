// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class HealthClientEventSource;

    // HealthReportingComponent; used to send health reports to HealthManager
    class HealthReportingComponent:
        public Common::FabricComponent,
        public Common::ComponentRoot,
        public Common::TextTraceComponent<Common::TraceTaskCodes::HealthClient>
    {
        DENY_COPY(HealthReportingComponent);

    public:
        HealthReportingComponent(
            __in HealthReportingTransport & transport,
            FabricClientInternalSettingsHolder && settingsHolder,
            std::wstring const & traceContext);

        HealthReportingComponent(
            __in HealthReportingTransport & transport,
            std::wstring const & traceContext,
            bool enableMaxNumberOfReportThrottle);

        virtual ~HealthReportingComponent();

        __declspec(property(get=get_HealthReportingTransport)) HealthReportingTransport & HealthReportingTransportObj;
        HealthReportingTransport & get_HealthReportingTransport() const { return transport_; };

        Common::ErrorCode AddHealthReport(
            ServiceModel::HealthReport && healthReport,
            ServiceModel::HealthReportSendOptionsUPtr && sendOptions = nullptr);

        Common::ErrorCode AddHealthReports(
            std::vector<ServiceModel::HealthReport> && reports,
            ServiceModel::HealthReportSendOptionsUPtr && sendOptions = nullptr);

        Common::ErrorCode HealthPreInitialize(std::wstring const & sourceId, ::FABRIC_HEALTH_REPORT_KIND kind, FABRIC_INSTANCE_ID instance);
        Common::ErrorCode HealthPostInitialize(std::wstring const & sourceId, ::FABRIC_HEALTH_REPORT_KIND kind, FABRIC_SEQUENCE_NUMBER sequence, FABRIC_SEQUENCE_NUMBER invalidateSequence);
        Common::ErrorCode HealthGetProgress(std::wstring const & sourceId, ::FABRIC_HEALTH_REPORT_KIND kind, __out FABRIC_SEQUENCE_NUMBER & progress);
        Common::ErrorCode HealthSkipSequence(std::wstring const & sourceId, ::FABRIC_HEALTH_REPORT_KIND kind, FABRIC_SEQUENCE_NUMBER sequenceToSkip);

        static Common::Global<HealthClientEventSource> Trace;

        int Test_GetCurrentReportCount(__out int & consecutiveHMBusyCount) const;

        // Manages sequence stream operations. Used by the trace class, so it needs to be public.
        class SequenceStream;
        using SequenceStreamSPtr = std::shared_ptr<SequenceStream>;

    protected:
        Common::ErrorCode OnOpen();
        Common::ErrorCode OnClose();
        void OnAbort();

    private:
        Common::AsyncOperationSPtr CreateTransportOperationRoot(Common::ComponentRootSPtr const & transportRoot) const;
        Common::TimeSpan GetServiceTooBusyBackoff() const;
        void ReplaceTimerValueCallerHoldsLock(Common::TimeSpan const dueTime);
        void UpdateTimerIfSoonerCallerHoldsLock(Common::TimeSpan const dueTime);
        void ReportHealthTimerCallback();
        void ReportHealth(
            Common::ActivityId const & activityId,
            Management::HealthManager::ReportHealthMessageBody && messageBody,
            Common::ComponentRootSPtr const & transportRoot);
        void OnReportHealthCompleted(
            Common::AsyncOperationSPtr const & asyncOperation,
            bool expectedCompletedSynchronously);
        void GetSequenceStreamProgress(
            Common::ActivityId const & activityId,
            Management::HealthManager::GetSequenceStreamProgressMessageBody && messageBody,
            Common::ComponentRootSPtr const & transportRoot);
        void OnGetSequenceStreamProgressCompleted(
            Common::AsyncOperationSPtr const & asyncOperation,
            Common::ActivityId const & activityId,
            bool expectedCompletedSynchronously);

        static bool IsReportResultErrorNonActionable(Common::ErrorCodeValue::Enum const errorCodeValue);
        static bool IsRetryableError(Common::ErrorCodeValue::Enum const errorCodeValue);
        static bool RepresentsHMBusyError(Common::ErrorCodeValue::Enum const errorCodeValue);

        // Updates the report count without using the lock.
        void UpdateReportCount(long udpateCount);

        // If the number of reports is less than max, accepts the report and optimistically updates the count of reports
        bool AcceptReportPerMaxCount(ServiceModel::HealthReport const & report);

        // Accepts reports until max queued reports is reached.
        // Returns the number of accepted reports.
        long AcceptReportsPerMaxCount(std::vector<ServiceModel::HealthReport> const & reports);

    private:

        // Map with sequence streams, keyed with sequence stream id.
        // NOT thread safe, it must be protected by the caller.
        class SequenceStreamMap
        {
            DENY_COPY(SequenceStreamMap)
        public:
            SequenceStreamMap();
            ~SequenceStreamMap();

            std::vector<SequenceStreamSPtr> GetSequenceStreamList() const;
            SequenceStreamSPtr GetSequenceStream(Management::HealthManager::SequenceStreamId const & sequenceStreamId) const;
            SequenceStreamSPtr GetOrCreateSequenceStream(
                Management::HealthManager::SequenceStreamId && sequenceStreamId,
                __in HealthReportingComponent & owner);

            template <class TResult>
            bool TryAppendSequenceStreamWithResult(
                TResult && result,
                __inout std::vector<std::pair<SequenceStreamSPtr, TResult>> & sequenceStreamResults) const
            {
                Management::HealthManager::SequenceStreamId ssId(result.Kind, result.SourceId);
                auto sequenceStream = GetSequenceStream(ssId);
                if (sequenceStream)
                {
                    sequenceStreamResults.push_back(std::make_pair(std::move(sequenceStream), std::move(result)));
                    return true;
                }

                return false;
            }

            size_t Size() const { return entries_.size(); }
            void Clear() { entries_.clear(); }

        private:
            std::map<Management::HealthManager::SequenceStreamId, SequenceStreamSPtr> entries_;
        };

    private:
        SequenceStreamMap sequenceStreams_;
        bool open_;
        Common::TimerSPtr reportTimerSPtr_;
        Common::DateTime scheduledFireTime_;
        mutable Common::RwLock lock_;
        HealthReportingTransport & transport_;
        FabricClientInternalSettingsHolder settingsHolder_;
        Common::ComponentRootSPtr transportRoot_;
        std::wstring traceContext_;
        bool isEnabled_; // bool for if health reporting is enabled
        int maxNumberOfReports_;

        // The current number of reports queued. Updated when reports are added or completed
        // based on HM replies. Used to reject reports when the max queue count is reached.
        Common::atomic_long currentReportCount_;

        bool isMessageLimitReached_;
        int consecutiveHMBusyCount_;
    };
}
