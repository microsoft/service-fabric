// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Common/PerformanceCounter.h"

namespace Management
{
    namespace HealthManager
    {
        class HealthManagerCounters
        {
            DENY_COPY(HealthManagerCounters)

        public:

            BEGIN_COUNTER_SET_DEFINITION(
                L"FD727CD0-1C26-46D9-81D8-A81B3B5C5D9E",
                L"Health Manager Component",
                L"Counters for Health Manager Component",
                Common::PerformanceCounterSetInstanceType::Multiple)

                COUNTER_DEFINITION(1, Common::PerformanceCounterType::RawData64, L"# of Received Health Reports", L"Number of received health reports")
                COUNTER_DEFINITION(2, Common::PerformanceCounterType::RawData64, L"# of Received Health Queries", L"Number of received health queries")
                COUNTER_DEFINITION(3, Common::PerformanceCounterType::RawData64, L"# of Entities", L"Number of active health entities. Includes health entities persisted in the health store and in-memory entities created by Health Manager based on the entity hierarchy")
                COUNTER_DEFINITION(4, Common::PerformanceCounterType::RawData64, L"# of Successful Health Reports", L"Number of successful health reports")
                COUNTER_DEFINITION(5, Common::PerformanceCounterType::RawData64, L"Avg. Health Query Processing ms/Operation", L"Average time for health query execution in milliseconds")
                COUNTER_DEFINITION(6, Common::PerformanceCounterType::RawData64, L"Avg. Health Report Processing ms/Operation", L"Average time for health report execution in milliseconds")
                COUNTER_DEFINITION(7, Common::PerformanceCounterType::RawData64, L"# of Dropped Health Reports", L"Number of received health reports dropped because the max pending report limit was reached")
                COUNTER_DEFINITION(8, Common::PerformanceCounterType::RawData64, L"# of Dropped Health Queries", L"Number of received health queries dropped because the max pending query count was reached")
                COUNTER_DEFINITION(9, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Received Health Reports/sec", L"Received health reports per second")
                COUNTER_DEFINITION(10, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Successful Health Reports/sec", L"Successfully processed health reports per second")
                COUNTER_DEFINITION(11, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Dropped Health Reports/sec", L"Dropped health reports per second")
                COUNTER_DEFINITION(12, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Received Health Queries/sec", L"Received health queries per second")
                COUNTER_DEFINITION(13, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Dropped Health Queries/sec", L"Dropped health queries per second")
                COUNTER_DEFINITION(14, Common::PerformanceCounterType::RateOfCountPerSecond32, L"Successful Health Queries/sec", L"Successfully processed health queries per second")
                COUNTER_DEFINITION(15, Common::PerformanceCounterType::RawData64, L"# of Optimized Health Reports", L"Number of successful health reports that are not immediately persisted to improve health store scalability")
                COUNTER_DEFINITION(16, Common::PerformanceCounterType::RawData64, L"# of Delayed Persisted Health Reports", L"Number of health reports that are delayed persisted to make expiration information available in case of health store primary change")
                COUNTER_DEFINITION(17, Common::PerformanceCounterType::RawData64, L"# of Expired Transient Health Reports", L"Number of expired transient health reports")
                COUNTER_DEFINITION(18, Common::PerformanceCounterType::RawData64, L"# of Queued Health Reports", L"Number of health reports accepted and queued for processing. The requests are counted until they are completed, either successfully or with error")
                COUNTER_DEFINITION(19, Common::PerformanceCounterType::RawData64, L"Size of Queued Health Reports", L"The size of the health reports accepted and queued for processing. The requests are queued until they are completed, either successfully or with error")
                COUNTER_DEFINITION(20, Common::PerformanceCounterType::RawData64, L"# of Critical Queued Health Reports", L"Number of critical health reports accepted and queued for processing. The requests are counted until they are completed, either successfully or with error. The critical reports are from System components that represent the authority for the reported entity type")
                COUNTER_DEFINITION(21, Common::PerformanceCounterType::RawData64, L"Size of Critical Queued Health Reports", L"The size of the critical health reports accepted and queued for processing. The requests are queued until they are completed, either successfully or with error. The critical reports are from System components that represent the authority for the reported entity type")
                COUNTER_DEFINITION(22, Common::PerformanceCounterType::RawData64, L"# of Dropped Critical Health Reports", L"Number of received critical health reports dropped because the max pending report limit was reached. The critical reports are from System components that represent the authority for the reported entity type")
                COUNTER_DEFINITION(23, Common::PerformanceCounterType::RawData64, L"Avg. Cluster Health Evaluation ms/Operation", L"Average time for evaluating the cluster health statistics in milliseconds. Doesn not include any time the query waited before processing started")

            END_COUNTER_SET_DEFINITION()

            DECLARE_COUNTER_INSTANCE(NumberOfReceivedHealthReports)
            DECLARE_COUNTER_INSTANCE(NumberOfReceivedHealthQueries)
            DECLARE_COUNTER_INSTANCE(NumberOfEntities)
            DECLARE_COUNTER_INSTANCE(NumberOfSuccessfulHealthReports)
            DECLARE_AVERAGE_TIME_COUNTER_INSTANCE(AverageQueryProcessTime)
            DECLARE_AVERAGE_TIME_COUNTER_INSTANCE(AverageReportProcessTime)
            DECLARE_COUNTER_INSTANCE(NumberOfDroppedHealthReports)
            DECLARE_COUNTER_INSTANCE(NumberOfDroppedHealthQueries)
            DECLARE_COUNTER_INSTANCE(RateOfReceivedHealthReports)
            DECLARE_COUNTER_INSTANCE(RateOfSuccessfulHealthReports)
            DECLARE_COUNTER_INSTANCE(RateOfDroppedHealthReports)
            DECLARE_COUNTER_INSTANCE(RateOfReceivedHealthQueries)
            DECLARE_COUNTER_INSTANCE(RateOfDroppedHealthQueries)
            DECLARE_COUNTER_INSTANCE(RateOfSuccessfulHealthQueries)
            DECLARE_COUNTER_INSTANCE(NumberOfOptimizedHealthReports)
            DECLARE_COUNTER_INSTANCE(NumberOfDelayedPersistedHealthReports)
            DECLARE_COUNTER_INSTANCE(NumberOfExpiredTransientHealthReports)
            DECLARE_COUNTER_INSTANCE(NumberOfQueuedReports)
            DECLARE_COUNTER_INSTANCE(SizeOfQueuedReports)
            DECLARE_COUNTER_INSTANCE(NumberOfQueuedCriticalReports)
            DECLARE_COUNTER_INSTANCE(SizeOfQueuedCriticalReports)
            DECLARE_COUNTER_INSTANCE(NumberOfDroppedCriticalHealthReports)
            DECLARE_AVERAGE_TIME_COUNTER_INSTANCE(AverageClusterHealthEvaluationTime)

            BEGIN_COUNTER_SET_INSTANCE(HealthManagerCounters)
                DEFINE_COUNTER_INSTANCE(NumberOfReceivedHealthReports, 1)
                DEFINE_COUNTER_INSTANCE(NumberOfReceivedHealthQueries, 2)
                DEFINE_COUNTER_INSTANCE(NumberOfEntities, 3)
                DEFINE_COUNTER_INSTANCE(NumberOfSuccessfulHealthReports, 4)
                DEFINE_COUNTER_INSTANCE(AverageQueryProcessTime, 5)
                DEFINE_COUNTER_INSTANCE(AverageReportProcessTime, 6)
                DEFINE_COUNTER_INSTANCE(NumberOfDroppedHealthReports, 7)
                DEFINE_COUNTER_INSTANCE(NumberOfDroppedHealthQueries, 8)
                DEFINE_COUNTER_INSTANCE(RateOfReceivedHealthReports, 9)
                DEFINE_COUNTER_INSTANCE(RateOfSuccessfulHealthReports, 10)
                DEFINE_COUNTER_INSTANCE(RateOfDroppedHealthReports, 11)
                DEFINE_COUNTER_INSTANCE(RateOfReceivedHealthQueries, 12)
                DEFINE_COUNTER_INSTANCE(RateOfDroppedHealthQueries, 13)
                DEFINE_COUNTER_INSTANCE(RateOfSuccessfulHealthQueries, 14)
                DEFINE_COUNTER_INSTANCE(NumberOfOptimizedHealthReports, 15)
                DEFINE_COUNTER_INSTANCE(NumberOfDelayedPersistedHealthReports, 16)
                DEFINE_COUNTER_INSTANCE(NumberOfExpiredTransientHealthReports, 17)
                DEFINE_COUNTER_INSTANCE(NumberOfQueuedReports, 18)
                DEFINE_COUNTER_INSTANCE(SizeOfQueuedReports, 19)
                DEFINE_COUNTER_INSTANCE(NumberOfQueuedCriticalReports, 20)
                DEFINE_COUNTER_INSTANCE(SizeOfQueuedCriticalReports, 21)
                DEFINE_COUNTER_INSTANCE(NumberOfDroppedCriticalHealthReports, 22)
                DEFINE_COUNTER_INSTANCE(AverageClusterHealthEvaluationTime, 23)
            END_COUNTER_SET_INSTANCE()

        public:
            void OnHealthReportsReceived(size_t count);
            void OnSuccessfulHealthReport(int64 startTime);
            void OnHealthQueryReceived();
            void OnHealthQueryDropped();
            void OnSuccessfulHealthQuery(int64 startTime);
            void OnRequestContextTryAcceptCompleted(RequestContext const & context);
            void OnRequestContextCompleted(RequestContext const & context);
            void OnEvaluateClusterHealthCompleted(int64 startTime);

        private:
            void OnRequestContextAcceptedForProcessing(RequestContext const & context);
            void OnRequestContextDropped(RequestContext const & context);
        };

        typedef std::shared_ptr<HealthManagerCounters> HealthManagerCountersSPtr;
    }
}
