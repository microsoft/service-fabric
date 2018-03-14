// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{
    // Internal representation of Transactional Replicator Counters
    class TRPerformanceCounters;
    typedef std::shared_ptr<TRPerformanceCounters> TRPerformanceCountersSPtr;

    static void inline UpdateAverageTime(
        __in int64 elapsedTicks,
        __inout Common::TimeSpan & currentAverage,
        __inout int64 & currentOperationCount)
    {
        currentAverage = Common::TimeSpan::FromTicks(
            currentAverage.Ticks +
            ((elapsedTicks - currentAverage.Ticks) / ++currentOperationCount));
    }

    class TRPerformanceCounters
    {
        DENY_COPY(TRPerformanceCounters)
        public:

            BEGIN_COUNTER_SET_DEFINITION(
                L"43103482-1BA3-4ECF-AF5A-9103AA08DE99",
                L"Native TransactionalReplicator",
                L"Counters for Native TransactionalReplicator",
                Common::PerformanceCounterSetInstanceType::Multiple)
                COUNTER_DEFINITION(
                    1,
                    Common::PerformanceCounterType::RateOfCountPerSecond32,
                    L"Begin Txn Operations/sec",
                    L"The number of BeginTransaction operations initiated per second")
                COUNTER_DEFINITION(
                    2,
                    Common::PerformanceCounterType::RateOfCountPerSecond32,
                    L"Txn Operations/sec",
                    L"The number of Add operations initiated commit per second.")
                COUNTER_DEFINITION(
                    3,
                    Common::PerformanceCounterType::RateOfCountPerSecond32,
                    L"Commit Txns/sec",
                    L"The number of transaction commits initiated per second..")
                COUNTER_DEFINITION(
                    4,
                    Common::PerformanceCounterType::RateOfCountPerSecond32,
                    L"Abort Txns/sec",
                    L"The number of transaction aborts initiated per second.")
                COUNTER_DEFINITION(
                    5,
                    Common::PerformanceCounterType::RateOfCountPerSecond32,
                    L"Atomic Operations/sec",
                    L"The number of Add atomic (with and without redo) operations initiated commit per second.")
                COUNTER_DEFINITION(
                    6,
                    Common::PerformanceCounterType::RawData32,
                    L"# Inflight visibility sequence numbers",
                    L"The current number of inflight visibility sequence numbers.")
                COUNTER_DEFINITION(
                    7,
                    Common::PerformanceCounterType::RawData32,
                    L"# Checkpoints",
                    L"The total number of perform checkpoints initiated.")
                COUNTER_DEFINITION(
                    8,
                    Common::PerformanceCounterType::RawData32,
                    L"Last State Manager Recovery duration seconds",
                    L"Duration for the State Manager to recover the checkpoint state of all state providers in the replica")
                COUNTER_DEFINITION(
                    9,
                    Common::PerformanceCounterType::RawData32,
                    L"Last Log Recovery duration seconds",
                    L"Duration for the Replicator to recover the transactions in the log")
                COUNTER_DEFINITION(
                    10,
                    Common::PerformanceCounterType::RateOfCountPerSecond64,
                    L"Incoming Bytes/sec",
                    L"Duration for the State Manager to recover the checkpoint state of all state providers in the replica")
                COUNTER_DEFINITION(
                    11,
                    Common::PerformanceCounterType::RateOfCountPerSecond64,
                    L"Log Flush Bytes/sec",
                    L"The number of bytes being flushed to the disk by the replicator per second.")
                COUNTER_DEFINITION(
                    12,
                    Common::PerformanceCounterType::RateOfCountPerSecond32,
                    L"Throttled Operations/sec",
                    L"The number of operations rejected every second by the replicator due to throttling.")
                COUNTER_DEFINITION_WITH_BASE(
                    13,
                    14,
                    Common::PerformanceCounterType::AverageCount64,
                    L"Avg. Bytes Flushed/IO",
                    L"The number of bytes flushed in every IO.")
                COUNTER_DEFINITION(
                    14,
                    Common::PerformanceCounterType::AverageBase,
                    L"Avg. Bytes Flushed/IO Base",
                    L"Base counter for Number of bytes flushed per IO",
                    noDisplay)
                COUNTER_DEFINITION_WITH_BASE(
                    15,
                    16,
                    Common::PerformanceCounterType::AverageCount64,
                    L"Avg. Transaction ms/Commit",
                    L"Average Commit Latency per transaction")
                COUNTER_DEFINITION(
                    16,
                    Common::PerformanceCounterType::AverageBase,
                    L"Avg. Transaction ms/Commit Base",
                    L"Base counter for Average Transaction Commit time",
                    noDisplay)
                COUNTER_DEFINITION_WITH_BASE(
                    17,
                    18,
                    Common::PerformanceCounterType::AverageCount64,
                    L"Avg. Flush Latency (ms)",
                    L"Average duration of PhysicalLogWriter flush")
                COUNTER_DEFINITION(
                    18,
                    Common::PerformanceCounterType::AverageBase,
                    L"Avg. Flush Latency (ms) base",
                    L"Base counter for Average duration of PhysicalLogWriter flush",
                    noDisplay)
                COUNTER_DEFINITION_WITH_BASE(
                    19,
                    20,
                    Common::PerformanceCounterType::AverageCount64,
                    L"Avg. Serialization Latency (ms)",
                    L"Average duration of serialization in PhysicalLogWriter flush")
                COUNTER_DEFINITION(
                    20,
                    Common::PerformanceCounterType::AverageBase,
                    L"Avg. Serialization Latency (ms) base",
                    L"Base counter for Average duration of serialization in PhysicalLogWriter flush",
                    noDisplay)

            END_COUNTER_SET_DEFINITION()

            DECLARE_COUNTER_INSTANCE(BeginTransactionOperationRate)
            DECLARE_COUNTER_INSTANCE(AddOperationRate)
            DECLARE_COUNTER_INSTANCE(CommitTransactionRate)
            DECLARE_COUNTER_INSTANCE(AbortTransactionRate)
            DECLARE_COUNTER_INSTANCE(AtomicOperationRate)
            DECLARE_COUNTER_INSTANCE(InflightVisibilitySequenceNumberCount)
            DECLARE_COUNTER_INSTANCE(CheckpointCount)
            DECLARE_COUNTER_INSTANCE(StateManagerRecovery)
            DECLARE_COUNTER_INSTANCE(LogRecovery)
            DECLARE_COUNTER_INSTANCE(IncomingBytesPerSecond)
            DECLARE_COUNTER_INSTANCE(LogFlushBytes)
            DECLARE_COUNTER_INSTANCE(ThrottledOpsPerSecond)
            DECLARE_COUNTER_INSTANCE(AvgBytesPerFlush)
            DECLARE_COUNTER_INSTANCE(AvgBytesPerFlushBase)
            DECLARE_COUNTER_INSTANCE(AvgCommitLatency)
            DECLARE_COUNTER_INSTANCE(AvgCommitLatencyBase)
            DECLARE_COUNTER_INSTANCE(AvgFlushLatency)
            DECLARE_COUNTER_INSTANCE(AvgFlushLatencyBase)
            DECLARE_COUNTER_INSTANCE(AvgSerializationLatency)
            DECLARE_COUNTER_INSTANCE(AvgSerializationLatencyBase)

            BEGIN_COUNTER_SET_INSTANCE(TRPerformanceCounters)
                DEFINE_COUNTER_INSTANCE(
                    BeginTransactionOperationRate,
                    1)
                DEFINE_COUNTER_INSTANCE(
                    AddOperationRate,
                    2)
                DEFINE_COUNTER_INSTANCE(
                    CommitTransactionRate,
                    3)
                DEFINE_COUNTER_INSTANCE(
                    AbortTransactionRate,
                    4)
                DEFINE_COUNTER_INSTANCE(
                    AtomicOperationRate,
                    5)
                DEFINE_COUNTER_INSTANCE(
                    InflightVisibilitySequenceNumberCount,
                    6)
                DEFINE_COUNTER_INSTANCE(
                    CheckpointCount,
                    7)
                DEFINE_COUNTER_INSTANCE(
                    StateManagerRecovery,
                    8)
                DEFINE_COUNTER_INSTANCE(
                    LogRecovery,
                    9)
                DEFINE_COUNTER_INSTANCE(
                    IncomingBytesPerSecond,
                    10)
                DEFINE_COUNTER_INSTANCE(
                    LogFlushBytes,
                    11)
                DEFINE_COUNTER_INSTANCE(
                    ThrottledOpsPerSecond,
                    12)
                DEFINE_COUNTER_INSTANCE(
                    AvgBytesPerFlush,
                    13)
                DEFINE_COUNTER_INSTANCE(
                    AvgBytesPerFlushBase,
                    14)
                DEFINE_COUNTER_INSTANCE(
                    AvgCommitLatency,
                    15)
                DEFINE_COUNTER_INSTANCE(
                    AvgCommitLatencyBase,
                    16)
                DEFINE_COUNTER_INSTANCE(
                    AvgFlushLatency,
                    17)
                DEFINE_COUNTER_INSTANCE(
                    AvgFlushLatencyBase,
                    18)
                DEFINE_COUNTER_INSTANCE(
                    AvgSerializationLatency,
                    19)
                DEFINE_COUNTER_INSTANCE(
                    AvgSerializationLatencyBase,
                    20)
            END_COUNTER_SET_INSTANCE()

            public:
                static TRPerformanceCountersSPtr CreateInstance(
                    std::wstring const & partitionId,
                    FABRIC_REPLICA_ID const & replicaId);
    };
}
