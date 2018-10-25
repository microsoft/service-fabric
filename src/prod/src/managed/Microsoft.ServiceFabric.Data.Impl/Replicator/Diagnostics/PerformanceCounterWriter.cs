// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator.Diagnostics
{
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;

    /// <summary>
    /// This class modifies the value of the performance counter that represents the
    /// average duration for State Provider Recovery
    /// </summary>
    internal class StateManagerRecoveryPerfomanceCounterWriter : FabricBaselessPerformanceCounterWriter
    {
        private readonly Stopwatch watch;

        internal StateManagerRecoveryPerfomanceCounterWriter(FabricPerformanceCounterSetInstance instance)
            : base(instance, TransactionalReplicatorPerformanceCounters.StateManagerRecoveryCounterName)
        {
            this.watch = new Stopwatch();
        }

        internal void StartMeasurement()
        {
            if (this.IsInitialized == false)
            {
                return;
            }

            this.watch.Start();
        }

        internal void StopMeasurement()
        {
            if (this.IsInitialized == false)
            {
                return;
            }

            this.Counter.SetValue(this.watch.Elapsed.Seconds);
            this.watch.Reset();
        }
    }

    /// <summary>
    /// This class modifies the value of the performance counter that represents the
    /// average duration for recovery in the LoggingReplicator
    /// </summary>
    [SuppressMessage("Microsoft.StyleCop.CSharp.MaintainabilityRules", "SA1402:FileMayOnlyContainASingleClass", Justification = "related classes")]
    internal class LogRecoveryPerfomanceCounterWriter : FabricBaselessPerformanceCounterWriter
    {
        private readonly Stopwatch watch;

        internal LogRecoveryPerfomanceCounterWriter(FabricPerformanceCounterSetInstance instance)
            : base(instance, TransactionalReplicatorPerformanceCounters.LogRecoveryCounterName)
        {
            this.watch = new Stopwatch();
        }

        internal void ContinueMeasurement()
        {
            if (this.IsInitialized == false)
            {
                return;
            }

            this.watch.Start();
        }

        internal void PauseMeasurement()
        {
            if (this.IsInitialized == false)
            {
                return;
            }

            this.watch.Stop();
        }

        internal void StartMeasurement()
        {
            if (this.IsInitialized == false)
            {
                return;
            }

            this.watch.Start();
        }

        internal void StopMeasurement()
        {
            if (this.IsInitialized == false)
            {
                return;
            }

            this.Counter.SetValue(this.watch.Elapsed.Seconds);
            this.watch.Reset();
        }
    }

    /// <summary>
    /// This class modifies the value of the performance counter that represents the
    /// frequency at which a begin transaction operation is invoked.
    /// </summary>
    internal class BeginTransactionOperationRatePerformanceCounterWriter : FabricBaselessPerformanceCounterWriter
    {
        internal BeginTransactionOperationRatePerformanceCounterWriter(FabricPerformanceCounterSetInstance instance)
            : base(instance, TransactionalReplicatorPerformanceCounters.BeginTransactionOperationRateCounterName)
        {
        }

        internal void Increment()
        {
            if (this.IsInitialized == false)
            {
                return;
            }

            this.Counter.Increment();
        }
    }

    /// <summary>
    /// This class modifies the value of the performance counter that represents the
    /// frequency at which a add operation is invoked.
    /// </summary>
    internal class AddOperationRatePerformanceCounterWriter : FabricBaselessPerformanceCounterWriter
    {
        internal AddOperationRatePerformanceCounterWriter(FabricPerformanceCounterSetInstance instance)
            : base(instance, TransactionalReplicatorPerformanceCounters.AddOperationRateCounterName)
        {
        }

        internal void Increment()
        {
            if (this.IsInitialized == false)
            {
                return;
            }

            this.Counter.Increment();
        }
    }

    // This class modifies the value of the performance counter that represents the
    // frequency at which transactions are committed.
    internal class TransactionCommitRatePerformanceCounterWriter : FabricBaselessPerformanceCounterWriter
    {
        internal TransactionCommitRatePerformanceCounterWriter(FabricPerformanceCounterSetInstance instance)
            : base(instance, TransactionalReplicatorPerformanceCounters.CommitTransactionRateCounterName)
        {
        }

        internal void Increment()
        {
            if (this.IsInitialized == false)
            {
                return;
            }

            this.Counter.Increment();
        }
    }

    // This class modifies the value of the performance counter that represents the
    // frequency at which transactions are aborted.
    internal class TransactionAbortRatePerformanceCounterWriter : FabricBaselessPerformanceCounterWriter
    {
        internal TransactionAbortRatePerformanceCounterWriter(FabricPerformanceCounterSetInstance instance)
            : base(instance, TransactionalReplicatorPerformanceCounters.AbortTransactionRateCounterName)
        {
        }

        internal void Increment()
        {
            if (this.IsInitialized == false)
            {
                return;
            }

            this.Counter.Increment();
        }
    }

    /// <summary>
    /// This class modifies the value of the performance counter that represents the
    /// frequency at which a add atomic (with and without redo) operation is invoked.
    /// </summary>
    internal class AddAtomicOperationRatePerformanceCounterWriter : FabricBaselessPerformanceCounterWriter
    {
        internal AddAtomicOperationRatePerformanceCounterWriter(FabricPerformanceCounterSetInstance instance)
            : base(instance, TransactionalReplicatorPerformanceCounters.AddAtomicOperationRateCounterName)
        {
        }

        internal void Increment()
        {
            if (this.IsInitialized == false)
            {
                return;
            }

            this.Counter.Increment();
        }
    }

    /// <summary>
    /// This class modifies the value of the performance counter that represents the
    /// number of inflight visibility sequence numbers.
    /// </summary>
    internal class InflightVisibilitySequenceNumberCountCounterWriter : FabricBaselessPerformanceCounterWriter
    {
        internal InflightVisibilitySequenceNumberCountCounterWriter(FabricPerformanceCounterSetInstance instance)
            : base(
                instance,
                TransactionalReplicatorPerformanceCounters.InflightVisibilitySequenceNumbersCountCounterName)
        {
        }

        internal void Decrement()
        {
            if (this.IsInitialized == false)
            {
                return;
            }

            this.Counter.Decrement();
        }

        internal void Increment()
        {
            if (this.IsInitialized == false)
            {
                return;
            }

            this.Counter.Increment();
        }
    }

    /// <summary>
    /// This class modifies the value of the performance counter that represents the
    /// total number of checkpoints.
    /// </summary>
    internal class CheckpointCountCounterWriter : FabricBaselessPerformanceCounterWriter
    {
        internal CheckpointCountCounterWriter(FabricPerformanceCounterSetInstance instance)
            : base(instance, TransactionalReplicatorPerformanceCounters.CheckpointCountCounterName)
        {
        }

        internal void Increment()
        {
            if (this.IsInitialized == false)
            {
                return;
            }

            this.Counter.Increment();
        }
    }

    internal class IncomingBytesRateCounterWriter : FabricBaselessPerformanceCounterWriter
    {
        internal IncomingBytesRateCounterWriter(FabricPerformanceCounterSetInstance instance)
            : base(instance, TransactionalReplicatorPerformanceCounters.IncomingBytesPerSecondCounterName)
        {
        }

        internal void IncrementBy(long value)
        {
            if (this.IsInitialized == false)
            {
                return;
            }

            this.Counter.IncrementBy(value);
        }
    }

    internal class LogFlushBytesRateCounterWriter : FabricBaselessPerformanceCounterWriter
    {
        internal LogFlushBytesRateCounterWriter(FabricPerformanceCounterSetInstance instance)
            : base(instance, TransactionalReplicatorPerformanceCounters.LogFlushBytesPerSecondCounterName)
        {
        }

        internal void IncrementBy(long value)
        {
            if (this.IsInitialized == false)
            {
                return;
            }

            this.Counter.IncrementBy(value);
        }
    }

    internal class ThrottleRateCounterWriter : FabricBaselessPerformanceCounterWriter
    {
        internal ThrottleRateCounterWriter(FabricPerformanceCounterSetInstance instance)
            : base(instance, TransactionalReplicatorPerformanceCounters.ThrottledOperationsPerSecondCounterName)
        {
        }

        internal void Increment()
        {
            if (this.IsInitialized == false)
            {
                return;
            }

            this.Counter.Increment();
        }
    }

    internal class AvgBytesPerFlushCounterWriter : FabricPerformanceCounterWriter
    {
        internal AvgBytesPerFlushCounterWriter(FabricPerformanceCounterSetInstance instance)
            : base(
                instance,
                TransactionalReplicatorPerformanceCounters.AvgBytesPerFlush,
                TransactionalReplicatorPerformanceCounters.AvgBytesPerFlushBase)
        {
        }

        internal void IncrementBy(long value)
        {
            if (this.IsInitialized == false)
            {
                return;
            }

            this.Counter.IncrementBy(value);
            this.CounterBase.Increment();
        }
    }

    internal class AvgTransactionCommitLatencyCounterWriter : FabricPerformanceCounterWriter
    {
        internal AvgTransactionCommitLatencyCounterWriter(FabricPerformanceCounterSetInstance instance)
            : base(
                instance,
                TransactionalReplicatorPerformanceCounters.AvgTransactionCommitTimeMs,
                TransactionalReplicatorPerformanceCounters.AvgTransactionCommitTimeMsBase)
        {
        }

        internal void IncrementBy(Stopwatch value)
        {
            if (this.IsInitialized == false)
            {
                return;
            }

            this.Counter.IncrementBy(value.ElapsedMilliseconds);
            this.CounterBase.Increment();
        }
    }

    internal class AvgFlushLatencyCounterWriter : FabricPerformanceCounterWriter
    {
        internal AvgFlushLatencyCounterWriter(FabricPerformanceCounterSetInstance instance)
            : base(
                instance,
                TransactionalReplicatorPerformanceCounters.AvgFlushLatency,
                TransactionalReplicatorPerformanceCounters.AvgFlushLatencyBase)
        {
        }

        internal void IncrementBy(Stopwatch value)
        {
            if (this.IsInitialized == false)
            {
                return;
            }

            this.Counter.IncrementBy(value.ElapsedMilliseconds);
            this.CounterBase.Increment();
        }
    }

    internal class AvgSerializationLatencyCounterWriter : FabricPerformanceCounterWriter
    {
        internal AvgSerializationLatencyCounterWriter(FabricPerformanceCounterSetInstance instance)
            : base(
                instance,
                TransactionalReplicatorPerformanceCounters.AvgSerializationLatency,
                TransactionalReplicatorPerformanceCounters.AvgSerializationLatencyBase)
        {
        }

        internal void IncrementBy(Stopwatch value)
        {
            if (this.IsInitialized == false)
            {
                return;
            }

            this.Counter.IncrementBy(value.ElapsedMilliseconds);
            this.CounterBase.Increment();
        }
    }
}