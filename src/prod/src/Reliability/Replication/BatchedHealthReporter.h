// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // Class responsible for caching and sending health reports
        // If the replicator reports a warning/error, a timer fires after 'cacheInterval' and sends the report to RAP
        // In the meantime, if the replicator reports OK, this timer is cancelled and no health report is sent

        // The above logic holds true for any report that cancels out the previous report that has altered the state within the 'cacheInterval'
        
        class BatchedHealthReporter : public Common::ComponentRoot
        {
            DENY_COPY(BatchedHealthReporter)

        public:
            static BatchedHealthReporterSPtr Create(
                Common::Guid const & partitionId, 
                ReplicationEndpointId const & endpointId,
                HealthReportType::Enum reportType,
                Common::TimeSpan const & cacheInterval,
                IReplicatorHealthClientSPtr const & healthClient);

            ~BatchedHealthReporter();
            void Close();

            void ScheduleOKReport();
            void ScheduleWarningReport(std::wstring const & warningDescription);

            __declspec(property(get=get_Test_IsTimerRunning)) bool Test_IsTimerRunning;
            bool get_Test_IsTimerRunning() const 
            { 
                Common::AcquireReadLock grab(lock_);
                return timerActive_;
            }

            __declspec(property(get=get_Test_LastHealthState)) ::FABRIC_HEALTH_STATE Test_LastHealthState;
            ::FABRIC_HEALTH_STATE get_Test_LastHealthState() const 
            { 
                Common::AcquireReadLock grab(lock_);
                return lastReportedHealthState_;
            }

            __declspec(property(get=get_Test_LatestHealthState)) ::FABRIC_HEALTH_STATE Test_LatestHealthState;
            ::FABRIC_HEALTH_STATE get_Test_LatestHealthState() const 
            { 
                Common::AcquireReadLock grab(lock_);
                return latestHealthState_;
            }

            __declspec(property(get=get_Test_LatestDescription)) std::wstring const & Test_LatestDescription;
            std::wstring const & get_Test_LatestDescription() const 
            { 
                Common::AcquireReadLock grab(lock_);
                return latestDescription_;
            }


        private:
            enum NextAction 
            { 
                None = 0,
                StartTimer, 
                CancelTimer 
            };

            static NextAction GetNextAction(
                ::FABRIC_HEALTH_STATE lastReportedHealthState,
                ::FABRIC_HEALTH_STATE latestHealthState,
                ::FABRIC_HEALTH_STATE incomingReport);

            static void SendReport(
                Common::Guid const & partitionId,
                ReplicationEndpointId const & endpointId,
                HealthReportType::Enum reportType,
                ::FABRIC_HEALTH_STATE toReport,
                std::wstring const & extraDescription,
                FABRIC_SEQUENCE_NUMBER sequenceNumber,
                IReplicatorHealthClientSPtr & healthClient);

            BatchedHealthReporter(
                Common::Guid const & partitionId, 
                ReplicationEndpointId const & description,
                HealthReportType::Enum reportType,
                Common::TimeSpan const & cacheInterval,
                IReplicatorHealthClientSPtr const & healthClient);

            void Open();

            void ScheduleReportCallerHoldsLock(
                ::FABRIC_HEALTH_STATE incomingReport,
                std::wstring const & description);

            void OnTimerCallback();

            MUTABLE_RWLOCK(REBatchedHealthReporter, lock_);

            bool timerActive_;
            Common::TimerSPtr timer_;
            Common::TimeSpan timerInterval_;

            IReplicatorHealthClientSPtr healthClient_;
            HealthReportType::Enum const reportType_;
            ::FABRIC_HEALTH_STATE lastReportedHealthState_;
            
            ::FABRIC_HEALTH_STATE latestHealthState_;
            std::wstring latestDescription_;

            bool isActive_;
            ReplicationEndpointId const endpointUniqueId_; 
            Common::Guid const partitionId_;
        }; 

    } // end namespace ReplicationComponent
} // end namespace Reliability
