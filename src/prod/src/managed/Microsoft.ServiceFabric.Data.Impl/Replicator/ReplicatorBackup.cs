// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Fabric;

    internal class ReplicatorBackup
    {
        private readonly long accumulatedLogSize;

        private readonly Epoch epochOfFirstLogicalLogRecord;

        private readonly Epoch epochOfHighestBackedUpLogRecord;

        private readonly uint logCount;

        private readonly long logSize;

        private readonly LogicalSequenceNumber lsnOfFirstLogicalLogRecord;

        private readonly LogicalSequenceNumber lsnOfHighestBackedUpLogRecord;

        public ReplicatorBackup(
            Epoch epochOfFirstLogicalLogRecord,
            LogicalSequenceNumber lsnOfFirstLogicalLogRecord,
            Epoch epochOfHighestBackedUpLogRecord,
            LogicalSequenceNumber lsnOfHighestBackedUpLogRecord,
            uint logCount,
            long logSize,
            long accumulatedLogSize)
        {
            this.epochOfFirstLogicalLogRecord = epochOfFirstLogicalLogRecord;
            this.lsnOfFirstLogicalLogRecord = lsnOfFirstLogicalLogRecord;

            this.epochOfHighestBackedUpLogRecord = epochOfHighestBackedUpLogRecord;
            this.lsnOfHighestBackedUpLogRecord = lsnOfHighestBackedUpLogRecord;

            this.logCount = logCount;
            this.logSize = logSize;
            this.accumulatedLogSize = accumulatedLogSize;

            Utility.Assert(accumulatedLogSize >= this.logSize, "accumulated log size is larger.");
        }

        public long AccumulatedLogSize
        {
            get { return this.accumulatedLogSize; }
        }

        public uint AccumulatedLogSizeInKB
        {
            get { return (uint) Math.Ceiling(this.accumulatedLogSize/(double) 1024); }
        }

        public long AccumulatedLogSizeInMB
        {
            get { return (long) Math.Ceiling(this.accumulatedLogSize/((double) 1024*(double) 1024)); }
        }

        public Epoch EpochOfFirstLogicalLogRecord
        {
            get { return this.epochOfFirstLogicalLogRecord; }
        }

        public Epoch EpochOfHighestBackedUpLogRecord
        {
            get { return this.epochOfHighestBackedUpLogRecord; }
        }

        public uint LogCount
        {
            get { return this.logCount; }
        }

        public long LogSize
        {
            get { return this.logSize; }
        }

        public LogicalSequenceNumber LsnOfFirstLogicalLogRecord
        {
            get { return this.lsnOfFirstLogicalLogRecord; }
        }

        public LogicalSequenceNumber LsnOfHighestBackedUpLogRecord
        {
            get { return this.lsnOfHighestBackedUpLogRecord; }
        }
    }
}