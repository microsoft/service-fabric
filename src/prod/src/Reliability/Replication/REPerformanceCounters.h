// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        static void inline UpdateAverageTime(
            __in int64 elapsedTicks,
            __inout Common::TimeSpan & currentAverage,
            __inout int64 & currentOperationCount)
        {
            currentAverage = Common::TimeSpan::FromTicks(
                currentAverage.Ticks +
                ((elapsedTicks - currentAverage.Ticks) / ++currentOperationCount));
        }

        class REPerformanceCounters
        {
            DENY_COPY(REPerformanceCounters)

            public:

                BEGIN_COUNTER_SET_DEFINITION(
                    L"147456E7-23C6-4FEE-A458-F3643183FE93",
                    L"Replicator",
                    L"Counters for Replicator Component",
                    Common::PerformanceCounterSetInstanceType::Multiple)
                    COUNTER_DEFINITION(
                        1,
                        Common::PerformanceCounterType::RawData64,
                        L"# Bytes Replication Queue",
                        L"Counter for measuring the size of virtual memory (in bytes) currently occupied by the Replication Queue")
                    COUNTER_DEFINITION(
                        2,
                        Common::PerformanceCounterType::RawData64,
                        L"# Operations Replication Queue",
                        L"Counter for measuring the number of operations in the Replication Queue")
                    COUNTER_DEFINITION(
                        3,
                        Common::PerformanceCounterType::AverageBase,
                        L"Avg. Commit ms/Operation Base",
                        L"Base Counter for measuring the average time for each operation in the replication queue to get committed. On a Primary Replica, Commit time is the time taken to receive an acknowledgement for an operation from a quorum of secondaries. On a Secondary Replica, Commit time is the time needed to dispatch an operation to the service",
                        noDisplay)
                    COUNTER_DEFINITION_WITH_BASE(
                        4,
                        3,
                        Common::PerformanceCounterType::AverageCount64,
                        L"Avg. Commit ms/Operation",
                        L"Counter for measuring the average time for each operation in the replication queue to get committed. On a Primary Replica, Commit time is the time taken to receive an acknowledgement for an operation from a quorum of secondaries. On a Secondary Replica, Commit time is the time needed to dispatch an operation to the service")
                    COUNTER_DEFINITION(
                        5,
                        Common::PerformanceCounterType::AverageBase,
                        L"Avg. Complete ms/Operation Base",
                        L"Base Counter for measuring the average time for each operation in the replication queue to get completed. On a Primary Replica, Complete time is the time taken to receive an acknowledgement for an operation from ALL the secondaries. On a Secondary Replica, Complete time is the time taken by the service to acknowledge the operation",
                        noDisplay)
                    COUNTER_DEFINITION_WITH_BASE(
                        6,
                        5,
                        Common::PerformanceCounterType::AverageCount64,
                        L"Avg. Complete ms/Operation",
                        L"Counter for measuring the average time for each operation in the replication queue to get completed. On a Primary Replica, Complete time is the time taken to receive an acknowledgement for an operation from ALL the secondaries. On a Secondary Replica, Complete time is the time taken by the service to acknowledge the operation")
                    COUNTER_DEFINITION(
                        7,
                        Common::PerformanceCounterType::AverageBase,
                        L"Avg. Cleanup ms/Operation Base",
                        L"Base Counter for measuring the average time for each operation in the replication queue to get released and freed",
                        noDisplay)
                    COUNTER_DEFINITION_WITH_BASE(
                        8,
                        7,
                        Common::PerformanceCounterType::AverageCount64,
                        L"Avg. Cleanup ms/Operation",
                        L"Base Counter for measuring the average time for each operation in the replication queue to get released and freed")
                    COUNTER_DEFINITION(
                        9,
                        Common::PerformanceCounterType::RawData32,
                        L"% Replication Queue Usage",
                        L"Counter for measuring the percentage of Replication Queue being consumed")
                    COUNTER_DEFINITION(
                        10,
                        Common::PerformanceCounterType::RawData32,
                        L"Current Role",
                        L"Counter indicating the current role of the replicator. 0 is Unknown, 1 is None, 2 is Primary, 3 is Idle Secondary and 4 is Active Secondary.")
                    COUNTER_DEFINITION(
                        11,
                        Common::PerformanceCounterType::RateOfCountPerSecond64,
                        L"Enqueued Operations/Sec",
                        L"Counter indicating the number of enqueued ops/sec")
                    COUNTER_DEFINITION(
                        12,
                        Common::PerformanceCounterType::RateOfCountPerSecond64,
                        L"Enqueued Bytes/Sec",
                        L"Counter indicating the number of enqueued bytes/sec")

                END_COUNTER_SET_DEFINITION()
                
                DECLARE_COUNTER_INSTANCE(NumberOfBytesReplicationQueue)
                DECLARE_COUNTER_INSTANCE(NumberOfOperationsReplicationQueue)
                DECLARE_COUNTER_INSTANCE(AverageCommitTimeBase)
                DECLARE_COUNTER_INSTANCE(AverageCommitTime)
                DECLARE_COUNTER_INSTANCE(AverageCompleteTimeBase)
                DECLARE_COUNTER_INSTANCE(AverageCompleteTime)
                DECLARE_COUNTER_INSTANCE(AverageCleanupTimeBase)
                DECLARE_COUNTER_INSTANCE(AverageCleanupTime)
                DECLARE_COUNTER_INSTANCE(ReplicationQueueFullPercentage)
                DECLARE_COUNTER_INSTANCE(Role)
                DECLARE_COUNTER_INSTANCE(EnqueuedOpsPerSecond)
                DECLARE_COUNTER_INSTANCE(EnqueuedBytesPerSecond)

                BEGIN_COUNTER_SET_INSTANCE(REPerformanceCounters)
                    DEFINE_COUNTER_INSTANCE(
                        NumberOfBytesReplicationQueue, 
                        1)
                    DEFINE_COUNTER_INSTANCE(
                        NumberOfOperationsReplicationQueue, 
                        2)
                    DEFINE_COUNTER_INSTANCE(
                        AverageCommitTimeBase, 
                        3)
                    DEFINE_COUNTER_INSTANCE(
                        AverageCommitTime, 
                        4)
                    DEFINE_COUNTER_INSTANCE(
                        AverageCompleteTimeBase, 
                        5)
                    DEFINE_COUNTER_INSTANCE(
                        AverageCompleteTime, 
                        6)
                    DEFINE_COUNTER_INSTANCE(
                        AverageCleanupTimeBase, 
                        7)
                    DEFINE_COUNTER_INSTANCE(
                        AverageCleanupTime, 
                        8)
                    DEFINE_COUNTER_INSTANCE(
                        ReplicationQueueFullPercentage, 
                        9)
                    DEFINE_COUNTER_INSTANCE(
                        Role, 
                        10)
                    DEFINE_COUNTER_INSTANCE(
                        EnqueuedOpsPerSecond, 
                        11)
                    DEFINE_COUNTER_INSTANCE(
                        EnqueuedBytesPerSecond, 
                        12)
                END_COUNTER_SET_INSTANCE()

        public:
            static REPerformanceCountersSPtr CreateInstance(
                std::wstring const & partitionId,
                FABRIC_REPLICA_ID const & replicaId);
        };
    }
}
