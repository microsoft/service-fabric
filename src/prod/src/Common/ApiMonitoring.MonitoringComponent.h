// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    namespace ApiMonitoring
    {
        struct MonitoringComponentConstructorParameters
        {
            MonitoringComponentConstructorParameters()
                : SlowHealthReportCallback(nullptr),
                ClearSlowHealthReportCallback(nullptr),
                ScanInterval(Common::TimeSpan::Zero),
                Root(nullptr)
            {
            }

            HealthReportCallback SlowHealthReportCallback;
            HealthReportCallback ClearSlowHealthReportCallback;
            Common::TimeSpan ScanInterval;
            Common::ComponentRoot const * Root;
        };

        class MonitoringComponent
        {
            DENY_COPY(MonitoringComponent);

        public:
            static MonitoringComponentUPtr Create(MonitoringComponentConstructorParameters parameters);

            MonitoringComponent(
                MonitoringComponentConstructorParameters parameters,
                ApiEventTraceCallback apiStartTraceCallback,
                ApiEventTraceCallback apiSlowTraceCallback,
                ApiFinishTraceCallback apiEndTraceCallback);

            __declspec(property(get = get_Test_GetTimer)) Common::TimerSPtr Test_GetTimer;
            Common::TimerSPtr get_Test_GetTimer() const { return timer_; }

            void Open(MonitoringComponentMetadata const & metaData);

            void Close();

            void StartMonitoring(
                ApiCallDescriptionSPtr const & description);
            
            void StopMonitoring(
                ApiCallDescriptionSPtr const & description,
                Common::ErrorCode const & error);

            // Exposed for testing
            void Test_OnTimer(Common::StopwatchTime now);

        private:
            typedef std::set<ApiCallDescriptionSPtr> MonitoredApiStore;

            struct ExpiredApi
            {
                ApiCallDescriptionSPtr ApiDescription;
                FABRIC_SEQUENCE_NUMBER SequenceNumber;
            };

			typedef std::pair<std::vector<ExpiredApi>, std::vector<ExpiredApi>> MonitoringActionItems;

            void TraceBeginIfEnabled(
                ApiCallDescription const & description);

            void TraceEndIfEnabled(
                ApiCallDescription const & description,
                Common::ErrorCode const & error);

            void TraceSlowIfEnabled(
                std::vector<ExpiredApi> const & events);

            void AddToStoreCallerHoldsLock(
                ApiCallDescriptionSPtr const & description);
            
            void RemoveFromStoreCallerHoldsLock(
                ApiCallDescriptionSPtr const & description);

			MonitoringActionItems MonitoringComponent::FindExpiredItemsCallerHoldsLock(
                Common::StopwatchTime now);

            static std::vector<MonitoringHealthEvent> GenerateSlowHealthCallbackList(
                std::vector<ExpiredApi> const & expiredApis);

            static std::vector<MonitoringHealthEvent> GenerateClearHealthCallbackList(
                ApiCallDescription const & description,
                FABRIC_SEQUENCE_NUMBER sequenceNumber);

            static void InvokeHealthReportCallback(
                std::vector<MonitoringHealthEvent> const & items,
                HealthReportCallback const & callback,
				MonitoringComponentMetadata const & metaData);

            void OnTimer();

            void DisableTimerCallerHoldsLock();
            void EnableTimerCallerHoldsLock();


            RWLOCK(MonitoringComponent, lock_);
            Common::TimerSPtr timer_;

            
            MonitoredApiStore store_;
            bool isOpen_;
            ApiEventTraceCallback apiStartTraceCallback_;
            ApiEventTraceCallback apiSlowTraceCallback_;
            ApiFinishTraceCallback apiEndTraceCallback_;
            HealthReportCallback slowHealthReportCallback_;
            HealthReportCallback clearSlowHealthReportCallback_;
            Common::TimeSpan scanInterval_;
            Common::ComponentRoot const & root_;
            MonitoringComponentMetadata metaData_;
        };
    }
}


