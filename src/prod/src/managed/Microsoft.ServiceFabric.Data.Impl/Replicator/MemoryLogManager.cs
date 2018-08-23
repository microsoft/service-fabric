// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Fabric;
    using System.Fabric.Data.Log;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Replicator.Diagnostics;

    internal class MemoryLogManager : LogManager
    {
        private int maxStreamSize = 0;

        internal MemoryLogManager(
            string baseLogFileAlias,
            IncomingBytesRateCounterWriter incomingBytesRateCounter,
            LogFlushBytesRateCounterWriter logFlushBytesRateCounter,
            AvgBytesPerFlushCounterWriter bytesPerFlushCounterWriter,
            AvgFlushLatencyCounterWriter avgFlushLatencyCounterWriter,
            AvgSerializationLatencyCounterWriter avgSerializationLatencyCounterWriter)
            : base(
                baseLogFileAlias,
                incomingBytesRateCounter,
                logFlushBytesRateCounter,
                bytesPerFlushCounterWriter,
                avgFlushLatencyCounterWriter,
                avgSerializationLatencyCounterWriter)
        {
        }

        ~MemoryLogManager()
        {
            this.DisposeInternal(false);
        }

        internal override async Task InitializeAsync(ITracer tracer, LogManagerInitialization init, TransactionalReplicatorSettings settings)
        {
            this.maxStreamSize = settings.PublicSettings.CheckpointThresholdInMB.Value * LogTruncationManager.ThrottleAfterPendingCheckpointCount;
            await base.InitializeAsync(tracer, init, settings).ConfigureAwait(false);
        }

        internal override async Task<IndexingLogRecord> CreateCopyLogAsync(
            Epoch startingEpoch,
            LogicalSequenceNumber startingLsn)
        {
            var flushCallback = this.PhysicalLogWriter.CallbackManager.Callback;
            await this.CloseCurrentLogAsync().ConfigureAwait(false);

            this.CurrentLogFileAlias = this.BaseLogFileAlias + CopySuffix;

            this.LogicalLog = await this.CreateLogFileAsync(true, CancellationToken.None).ConfigureAwait(false);

            var callbackManager = new PhysicalLogWriterCallbackManager(
                flushCallback,
                this.Tracer);
            this.PhysicalLogWriter = new PhysicalLogWriter(
                this.LogicalLog,
                callbackManager,
                this.Tracer,
                this.MaxWriteCacheSizeInMB,
                this.IncomingBytesRateCounterWriter,
                this.LogFlushBytesRateCounterWriter,
                this.BytesPerFlushCounterWriter,
                this.AvgFlushLatencyCounterWriter,
                this.AvgSerializationLatencyCounterWriter,
                false);

            var firstIndexingRecord = new IndexingLogRecord(startingEpoch, startingLsn, null);
            this.PhysicalLogWriter.InsertBufferedRecord(firstIndexingRecord);

            await this.PhysicalLogWriter.FlushAsync("CreateCopyLogAsync").ConfigureAwait(false);

            this.LogHeadRecordPosition = firstIndexingRecord.RecordPosition;
            return firstIndexingRecord;
        }

        internal override async Task DeleteLogAsync()
        {
            if (this.PhysicalLogWriter != null)
            {
                this.PhysicalLogWriter.Dispose();
                this.PhysicalLogWriter = null;
            }

            if (this.LogicalLog != null)
            {
                var log = this.LogicalLog;
                this.LogicalLog = null;
                await log.CloseAsync(CancellationToken.None).ConfigureAwait(false);
            }

            return;
        }

        internal override Task RenameCopyLogAtomicallyAsync()
        {
            return Task.FromResult(0);
        }

        protected override async Task<ILogicalLog> CreateLogFileAsync(
            bool createNew,
            CancellationToken cancellationToken)
        {
            return await MemoryLogicalLog.CreateMemoryLogicalLog(this.maxStreamSize).ConfigureAwait(false);
        }
    }
}