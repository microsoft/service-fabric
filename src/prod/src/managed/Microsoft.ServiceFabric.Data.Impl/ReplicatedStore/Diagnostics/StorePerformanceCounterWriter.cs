// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ReplicatedStore.Diagnostics
{
    using System;
    using System.Diagnostics;
    using System.Fabric.Common;
    using System.Fabric.Store;

    // This class modifies the value of the performance counter that represents the
    // number of items in the TStore
    internal sealed class TStoreItemCountCounterWriter : FabricBaselessPerformanceCounterWriter
    {
        internal TStoreItemCountCounterWriter(FabricPerformanceCounterSetInstance instance)
            : base(instance, DifferentialStoreConstants.PerformanceCounters.ItemCountName)
        {
        }

        internal void Increment()
        {
            this.Counter.Increment();
        }

        internal void Decrement()
        {
            this.Counter.Decrement();
        }
        
        internal void SetCount(long value)
        {
            this.Counter.SetValue(value);
        }

        internal void ResetCount()
        {
            this.Counter.SetValue(0);
        }

        internal long GetCount()
        {
            return this.Counter.GetValue();
        }
    }

    // This class modifies the value of the performance counter that represents the
    // total disk size for the TStore
    internal sealed class TStoreDiskSizeCounterWriter : FabricBaselessPerformanceCounterWriter
    {
        internal TStoreDiskSizeCounterWriter(FabricPerformanceCounterSetInstance instance)
            : base(instance, DifferentialStoreConstants.PerformanceCounters.DiskSizeName)
        {
        }

        internal void SetSize(long bytes)
        {
            this.Counter.SetValue(bytes);
        }
    }

    // This class modifies the value of the performance counter that represents the
    // checkpoint file write speed for the TStore
    internal sealed class TStoreCheckpointRateCounterWriter : FabricBaselessPerformanceCounterWriter
    {
        private Stopwatch diskWriteWatch = new Stopwatch();

        internal TStoreCheckpointRateCounterWriter(FabricPerformanceCounterSetInstance instance)
            : base(instance, DifferentialStoreConstants.PerformanceCounters.CheckpointFileWriteBytesPerSecName)
        {
        }

        internal void StartMeasurement()
        {
            this.diskWriteWatch.Start();
        }

        internal void StopMeasurement()
        {
            this.diskWriteWatch.Stop();
        }

        internal void ResetMeasurement()
        {
            this.diskWriteWatch.Reset();
        }

        internal long UpdatePerformanceCounter(long bytesWritten)
        {
            long writeTicks = diskWriteWatch.ElapsedTicks;
            long writeBytesPerSec = bytesWritten * TimeSpan.TicksPerSecond / (writeTicks + 1);

            this.Counter.SetValue(writeBytesPerSec);

            return writeBytesPerSec;
        }
    }

    // This class modifies the value of the performance counter that represents the
    // store copy disk transfer speed for the TStore
    internal sealed class TStoreCopyRateCounterWriter : FabricBaselessPerformanceCounterWriter
    {
        private const long DiskTransferBytesThreshold = 10 * 1024 * 1024;

        private long totalDiskTransferBytes = 0;
        private long totalDiskTransferTicks = 0;
        private long deltaDiskTransferBytes = 0;
        private Stopwatch diskTransferWatch = new Stopwatch();

        internal TStoreCopyRateCounterWriter(FabricPerformanceCounterSetInstance instance)
            : base(instance, DifferentialStoreConstants.PerformanceCounters.CheckpointFileWriteBytesPerSecName)
        {
        }

        internal long AvgDiskTransferBytesPerSec
        {
            get
            {
                return GetBytesPerSecond(
                    this.totalDiskTransferBytes + this.deltaDiskTransferBytes,
                    this.totalDiskTransferTicks + this.diskTransferWatch.ElapsedTicks);
            }
        }

        internal void StartMeasurement()
        {
            this.diskTransferWatch.Start();
        }

        internal void StopMeasurement(long bytesTransferred = 0)
        {
            this.diskTransferWatch.Stop();
            this.deltaDiskTransferBytes += bytesTransferred;

            if (this.deltaDiskTransferBytes >= DiskTransferBytesThreshold)
            {
                UpdatePerformanceCounters();
            }
        }

        internal void UpdatePerformanceCounters()
        {
            var deltaTransferTicks = diskTransferWatch.ElapsedTicks;

            if (this.deltaDiskTransferBytes > 0)
            {
                this.Counter.SetValue(
                    GetBytesPerSecond(this.deltaDiskTransferBytes, deltaTransferTicks));
            }

            this.totalDiskTransferBytes += this.deltaDiskTransferBytes;
            this.totalDiskTransferTicks += deltaTransferTicks;
            this.diskTransferWatch.Reset();
            this.deltaDiskTransferBytes = 0;
        }

        private static long GetBytesPerSecond(long bytes, long ticks)
        {
            return bytes * TimeSpan.TicksPerSecond / (ticks + 1);
        }
    }
}