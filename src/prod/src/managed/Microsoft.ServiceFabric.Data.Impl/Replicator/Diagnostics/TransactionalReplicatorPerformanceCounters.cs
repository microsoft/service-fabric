// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator.Diagnostics
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;

    internal class TransactionalReplicatorPerformanceCounters : IFabricPerformanceCountersDefinition
    {
        internal const string CategoryName = "Service Fabric Transactional Replicator";

        internal const string AbortTransactionRateCounterName = "Abort Txns/sec";

        internal const string AddAtomicOperationRateCounterName = "Atomic Operations/sec";

        internal const string AddOperationRateCounterName = "Txn Operations/sec";

        internal const string BeginTransactionOperationRateCounterName = "Begin Txn Operations/sec";

        internal const string AvgBytesPerFlush = "Number of bytes flushed per IO";

        internal const string AvgBytesPerFlushBase = "Base counter for Number of bytes flushed per IO";

        internal const string CheckpointCountCounterName = "# Checkpoints";

        internal const string CommitTransactionRateCounterName = "Commit Txns/sec";

        internal const string IncomingBytesPerSecondCounterName = "Incoming Bytes/sec";

        internal const string InflightVisibilitySequenceNumbersCountCounterName =
            "# Inflight visibility sequence numbers";

        internal const string LogFlushBytesPerSecondCounterName = "Log Flush Bytes/sec";

        internal const string LogRecoveryCounterName = "Last Log Recovery duration seconds";

        internal const string StateManagerRecoveryCounterName = "Last State Manager Recovery duration seconds";

        internal const string ThrottledOperationsPerSecondCounterName = "Throttled Operations/sec";

        internal const string AvgTransactionCommitTimeMsBase = "Base counter for Average Transaction Commit time";

        internal const string AvgTransactionCommitTimeMs = "Avg. Transaction ms/Commit";

        internal const string AvgSerializationLatency = "Avg. Serialization Latency (ms)";

        internal const string AvgSerializationLatencyBase = "Base counter for Avg. Serialization Latency (ms)";

        internal const string AvgFlushLatency = "Avg. Flush Latency (ms)";

        internal const string AvgFlushLatencyBase = "Base counter for Avg. Flush Latency (ms)";

        private static readonly Dictionary<Tuple<string, string>, FabricPerformanceCounterType> CounterTypes =
            new Dictionary<Tuple<string, string>, FabricPerformanceCounterType>()
            {
                {
                    Tuple.Create(CategoryName, BeginTransactionOperationRateCounterName),
                    FabricPerformanceCounterType.RateOfCountsPerSecond32
                },
                {
                    Tuple.Create(CategoryName, AddOperationRateCounterName),
                    FabricPerformanceCounterType.RateOfCountsPerSecond32
                },
                {
                    Tuple.Create(CategoryName, CommitTransactionRateCounterName),
                    FabricPerformanceCounterType.RateOfCountsPerSecond32
                },
                {
                    Tuple.Create(CategoryName, AbortTransactionRateCounterName),
                    FabricPerformanceCounterType.RateOfCountsPerSecond32
                },
                {
                    Tuple.Create(CategoryName, AddAtomicOperationRateCounterName),
                    FabricPerformanceCounterType.RateOfCountsPerSecond32
                },
                {
                    Tuple.Create(CategoryName, InflightVisibilitySequenceNumbersCountCounterName),
                    FabricPerformanceCounterType.NumberOfItems32
                },
                {
                    Tuple.Create(CategoryName, CheckpointCountCounterName),
                    FabricPerformanceCounterType.NumberOfItems32
                },
                {
                    Tuple.Create(CategoryName, LogRecoveryCounterName),
                    FabricPerformanceCounterType.NumberOfItems32
                },
                {
                    Tuple.Create(CategoryName, StateManagerRecoveryCounterName),
                    FabricPerformanceCounterType.NumberOfItems32
                },
                {
                    Tuple.Create(CategoryName, IncomingBytesPerSecondCounterName),
                    FabricPerformanceCounterType.RateOfCountsPerSecond64
                },
                {
                    Tuple.Create(CategoryName, LogFlushBytesPerSecondCounterName),
                    FabricPerformanceCounterType.RateOfCountsPerSecond64
                },
                {
                    Tuple.Create(CategoryName, ThrottledOperationsPerSecondCounterName),
                    FabricPerformanceCounterType.RateOfCountsPerSecond32
                },
                {
                    Tuple.Create(CategoryName, AvgBytesPerFlush),
                    FabricPerformanceCounterType.AverageCount64
                },
                {
                    Tuple.Create(CategoryName, AvgBytesPerFlushBase),
                    FabricPerformanceCounterType.AverageBase
                },
                {
                    Tuple.Create(CategoryName, AvgTransactionCommitTimeMs),
                    FabricPerformanceCounterType.AverageCount64
                },
                {
                    Tuple.Create(CategoryName, AvgTransactionCommitTimeMsBase),
                    FabricPerformanceCounterType.AverageBase
                },
                {
                    Tuple.Create(CategoryName, AvgFlushLatency),
                    FabricPerformanceCounterType.AverageCount64
                },
                {
                    Tuple.Create(CategoryName, AvgFlushLatencyBase),
                    FabricPerformanceCounterType.AverageBase
                },
                {
                    Tuple.Create(CategoryName, AvgSerializationLatency),
                    FabricPerformanceCounterType.AverageCount64
                },
                {
                    Tuple.Create(CategoryName, AvgSerializationLatencyBase),
                    FabricPerformanceCounterType.AverageBase
                }
            };

        private static readonly
            Dictionary<FabricPerformanceCounterSetDefinition, IEnumerable<FabricPerformanceCounterDefinition>> CounterSets =
                new Dictionary<FabricPerformanceCounterSetDefinition, IEnumerable<FabricPerformanceCounterDefinition>>()
                {
                    {
                        new FabricPerformanceCounterSetDefinition(
                            CategoryName,
                            "Counters for Service Fabric Transactional Replicator.",
                            FabricPerformanceCounterCategoryType.MultiInstance,
                            Guid.Parse("0DFA27E8-8703-4093-97E1-8B94EC1008A8"),
                            "ServiceFabricTransactionalReplicator"),
                        new[]
                        {
                            new FabricPerformanceCounterDefinition(
                                1,
                                BeginTransactionOperationRateCounterName,
                                "The number of BeginTransaction operations initiated per second.",
                                GetType(CategoryName, BeginTransactionOperationRateCounterName),
                                "BeginTransactionOperationRate"),
                            new FabricPerformanceCounterDefinition(
                                2,
                                AddOperationRateCounterName,
                                "The number of Add operations initiated commit per second.",
                                GetType(CategoryName, AddOperationRateCounterName),
                                "AddOperationRate"),
                            new FabricPerformanceCounterDefinition(
                                3,
                                CommitTransactionRateCounterName,
                                "The number of transaction commits initiated per second.",
                                GetType(CategoryName, CommitTransactionRateCounterName),
                                "CommitTransactionRate"),
                            new FabricPerformanceCounterDefinition(
                                4,
                                AbortTransactionRateCounterName,
                                "The number of transaction aborts initiated per second.",
                                GetType(CategoryName, AbortTransactionRateCounterName),
                                "AbortTransactionRate"),
                            new FabricPerformanceCounterDefinition(
                                5,
                                AddAtomicOperationRateCounterName,
                                "The number of Add atomic (with and without redo) operations initiated commit per second.",
                                GetType(CategoryName, AddAtomicOperationRateCounterName),
                                "AtomicOperationRate"),
                            new FabricPerformanceCounterDefinition(
                                6,
                                InflightVisibilitySequenceNumbersCountCounterName,
                                "The current number of inflight visibility sequence numbers.",
                                GetType(CategoryName, InflightVisibilitySequenceNumbersCountCounterName),
                                "InflightVisibilitySequenceNumberCount"),
                            new FabricPerformanceCounterDefinition(
                                7,
                                CheckpointCountCounterName,
                                "The total number of perform checkpoints initiated.",
                                GetType(CategoryName, CheckpointCountCounterName),
                                "CheckpointCount"),
                            new FabricPerformanceCounterDefinition(
                                8,
                                StateManagerRecoveryCounterName,
                                "Duration for the State Manager to recover the checkpoint state of all state providers in the replica",
                                GetType(CategoryName, StateManagerRecoveryCounterName),
                                "StateManagerRecovery"),
                            new FabricPerformanceCounterDefinition(
                                9,
                                LogRecoveryCounterName,
                                "Duration for the Replicator to recover the transactions in the log",
                                GetType(CategoryName, LogRecoveryCounterName),
                                "LogRecovery"),
                            new FabricPerformanceCounterDefinition(
                                10,
                                IncomingBytesPerSecondCounterName,
                                "The number of incoming bytes to the replicator per second.",
                                GetType(CategoryName, IncomingBytesPerSecondCounterName),
                                "IncomingBytesPerSecond"),
                            new FabricPerformanceCounterDefinition(
                                11,
                                LogFlushBytesPerSecondCounterName,
                                "The number of bytes being flushed to the disk by the replicator per second.",
                                GetType(CategoryName, LogFlushBytesPerSecondCounterName),
                                "LogFlushBytes"),
                            new FabricPerformanceCounterDefinition(
                                12,
                                ThrottledOperationsPerSecondCounterName,
                                "The number of operations rejected every second by the replicator due to throttling.",
                                GetType(CategoryName, ThrottledOperationsPerSecondCounterName),
                                "ThrottledOpsPerSecond"),
                            new FabricPerformanceCounterDefinition(
                                13,
                                14,
                                AvgBytesPerFlush,
                                "The number of bytes flushed in every IO.",
                                GetType(CategoryName, AvgBytesPerFlush),
                                "AvgBytesPerFlush"),
                            new FabricPerformanceCounterDefinition(
                                14,
                                AvgBytesPerFlushBase,
                                string.Empty,
                                GetType(CategoryName, AvgBytesPerFlushBase),
                                "AvgBytesPerFlushBase",
                                new[] {"noDisplay"}),
                            new FabricPerformanceCounterDefinition(
                                15,
                                16,
                                AvgTransactionCommitTimeMs,
                                "Average Commit Latency per transaction",
                                GetType(CategoryName, AvgTransactionCommitTimeMs),
                                "AvgCommitLatency"),
                            new FabricPerformanceCounterDefinition(
                                16,
                                AvgTransactionCommitTimeMsBase,
                                string.Empty,
                                GetType(CategoryName, AvgTransactionCommitTimeMsBase),
                                "AvgCommitLatencyBase",
                                new[] {"noDisplay"}),
                            new FabricPerformanceCounterDefinition(
                                17,
                                18,
                                AvgFlushLatency,
                                "Average duration of PhysicalLogWriter flush",
                                GetType(CategoryName, AvgFlushLatency),
                                "AvgFlushLatency"),
                            new FabricPerformanceCounterDefinition(
                                18,
                                AvgFlushLatencyBase,
                                string.Empty,
                                GetType(CategoryName, AvgFlushLatencyBase),
                                "AvgFlushLatencyBase",
                                new[] {"noDisplay"}),
                            new FabricPerformanceCounterDefinition(
                                19,
                                20,
                                AvgSerializationLatency,
                                "Average duration of serialization in PhysicalLogWriter flush",
                                GetType(CategoryName, AvgSerializationLatency),
                                "AvgSerializationLatency"),
                            new FabricPerformanceCounterDefinition(
                                20,
                                AvgSerializationLatencyBase,
                                string.Empty,
                                GetType(CategoryName, AvgSerializationLatencyBase),
                                "AvgSerializationLatencyBase",
                                new[] {"noDisplay"})
                        }
                    }
                };

        public Dictionary<FabricPerformanceCounterSetDefinition, IEnumerable<FabricPerformanceCounterDefinition>> GetCounterSets()
        {
            return CounterSets;
        }

        internal static FabricPerformanceCounterType GetType(string categoryName, string counterName)
        {
            return CounterTypes[Tuple.Create(categoryName, counterName)];
        }
    }
}