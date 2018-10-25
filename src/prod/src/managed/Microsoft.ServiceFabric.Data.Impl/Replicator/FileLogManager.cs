// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Data.Log;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Common.Tracing;
    using Microsoft.ServiceFabric.Data.Testability;
    using Microsoft.ServiceFabric.Replicator.Diagnostics;

    internal class FileLogManager : LogManager
    {
        private readonly LogWriteFaultInjectionParameters logWriteFaultInjectionParameters;

        internal FileLogManager(
            string baseLogFileAlias,
            IncomingBytesRateCounterWriter incomingBytesRateCounter,
            LogFlushBytesRateCounterWriter logFlushBytesRateCounter,
            AvgBytesPerFlushCounterWriter bytesPerFlushCounterWriter,
            AvgFlushLatencyCounterWriter avgFlushLatencyCounterWriter,
            AvgSerializationLatencyCounterWriter avgSerializationLatencyCounterWriter)
            : this(
                baseLogFileAlias,
                incomingBytesRateCounter,
                logFlushBytesRateCounter,
                bytesPerFlushCounterWriter,
                avgFlushLatencyCounterWriter,
                avgSerializationLatencyCounterWriter,
                new LogWriteFaultInjectionParameters())
        {
        }

        internal FileLogManager(
            string baseLogFileAlias,
            IncomingBytesRateCounterWriter incomingBytesRateCounter,
            LogFlushBytesRateCounterWriter logFlushBytesRateCounter,
            AvgBytesPerFlushCounterWriter bytesPerFlushCounterWriter,
            AvgFlushLatencyCounterWriter avgFlushLatencyCounterWriter,
            AvgSerializationLatencyCounterWriter avgSerializationLatencyCounterWriter,
            LogWriteFaultInjectionParameters logWriteFaultInjectionParameters)
            : base(
                baseLogFileAlias,
                incomingBytesRateCounter,
                logFlushBytesRateCounter,
                bytesPerFlushCounterWriter,
                avgFlushLatencyCounterWriter,
                avgSerializationLatencyCounterWriter)
        {
            this.logWriteFaultInjectionParameters = logWriteFaultInjectionParameters;
        }

        ~FileLogManager()
        {
            this.DisposeInternal(false);
        }

        internal override async Task<IndexingLogRecord> CreateCopyLogAsync(
            Epoch startingEpoch,
            LogicalSequenceNumber startingLsn)
        {
            var flushCallback = this.PhysicalLogWriter.CallbackManager.Callback;
            await this.CloseCurrentLogAsync().ConfigureAwait(false);

            this.CurrentLogFileAlias = this.BaseLogFileAlias + CopySuffix;
            try
            {
                FabricFile.Delete(FileLogicalLog.GetFullPathToLog(this.LogFileDirectoryPath, this.CurrentLogFileAlias));
            }
            catch (Exception ex)
            {
                FabricEvents.Events.LogManager(
                    this.Tracer.Type,
                    "CreateCopyLog: Delete logical log: " + this.CurrentLogFileAlias + " failed: " + ex);
            }

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

            FabricEvents.Events.LogManager(
                this.Tracer.Type,
                "DeleteLog: Try delete log file " + this.BaseLogFileAlias + BackupSuffix);

            await this.DeleteLogFileAsync(this.BaseLogFileAlias + BackupSuffix).ConfigureAwait(false);

            FabricEvents.Events.LogManager(
                this.Tracer.Type,
                "DeleteLog: Try delete log file " + this.BaseLogFileAlias + CopySuffix);

            await this.DeleteLogFileAsync(this.BaseLogFileAlias + CopySuffix).ConfigureAwait(false);

            FabricEvents.Events.LogManager(
                this.Tracer.Type,
                "DeleteLog: Try delete log file " + this.BaseLogFileAlias);

            await this.DeleteLogFileAsync(this.BaseLogFileAlias).ConfigureAwait(false);
        }

        internal override async Task InitializeAsync(ITracer paramTracer, LogManagerInitialization init, TransactionalReplicatorSettings settings)
        {
            await base.InitializeAsync(paramTracer, init, settings).ConfigureAwait(false);
        }

        internal override async Task RenameCopyLogAtomicallyAsync()
        {
            try
            {
                // Retrieve head and tail records of the copy log
                var tailRecord = this.PhysicalLogWriter.CurrentLogTailRecord;
                var callbackManager = this.PhysicalLogWriter.CallbackManager;
                this.PhysicalLogWriter.ResetCallbackManager = true;
                var closedException = this.PhysicalLogWriter.ClosedException;
                var exitTcs = this.PhysicalLogWriter.ExitTcs;

                // Now, close the copy log
                await this.CloseCurrentLogAsync().ConfigureAwait(false);

                var fullcurrentLogfileName = FileLogicalLog.GetFullPathToLog(
                    this.LogFileDirectoryPath,
                    this.CurrentLogFileAlias);
                var fullBaseLogFileName = FileLogicalLog.GetFullPathToLog(
                    this.LogFileDirectoryPath,
                    this.BaseLogFileAlias);
                var fullbackupLogFileName = FileLogicalLog.GetFullPathToLog(
                    this.LogFileDirectoryPath,
                    this.BaseLogFileAlias + BackupSuffix);

                FabricFile.Replace(fullcurrentLogfileName, fullBaseLogFileName, fullbackupLogFileName, false);

                // opens using currentLogFileAlias
                this.CurrentLogFileAlias = this.BaseLogFileAlias;
                this.LogicalLog = await this.CreateLogFileAsync(false, CancellationToken.None).ConfigureAwait(false);

                // Write cursor is auto-position to eos
                this.PhysicalLogWriter = new PhysicalLogWriter(
                    this.LogicalLog,
                    tailRecord,
                    callbackManager,
                    closedException,
                    this.Tracer,
                    this.MaxWriteCacheSizeInMB,
                    this.IncomingBytesRateCounterWriter,
                    this.LogFlushBytesRateCounterWriter,
                    this.BytesPerFlushCounterWriter,
                    this.AvgFlushLatencyCounterWriter,
                    this.AvgSerializationLatencyCounterWriter)
                {
                    ExitTcs = exitTcs
                };

                var message =
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "RenameCopyLogAtomically: Renamed log. Head record Position: {0}" + Environment.NewLine
                        + "             Tail record. Type: {1} LSN: {2} PSN: {3} Position: {4}",
                        this.LogHeadRecordPosition,
                        tailRecord.RecordType,
                        tailRecord.Lsn.LSN,
                        tailRecord.Psn.PSN,
                        tailRecord.RecordPosition);

                FabricEvents.Events.LogManager(this.Tracer.Type, message);
            }
            catch (Exception e)
            {
                Utility.ProcessUnexpectedException(
                    "LogManager::RenameCopyLogAtomically",
                    this.Tracer,
                    "renaming log",
                    e);
            }
        }

        protected override async Task<ILogicalLog> CreateLogFileAsync(
            bool createNew,
            CancellationToken cancellationToken)
        {
            var fullLogfileName = FileLogicalLog.GetFullPathToLog(
                this.LogFileDirectoryPath,
                this.CurrentLogFileAlias);

            if (!FabricFile.Exists(fullLogfileName))
            {
                // If the current log does not exist, try to use the backup file.
                var backupfullLogfileName = FileLogicalLog.GetFullPathToLog(
                    this.LogFileDirectoryPath,
                    this.CurrentLogFileAlias + BackupSuffix);

                if (FabricFile.Exists(backupfullLogfileName))
                {
                    // if the backup file exists, rename it to the current log file name.
                    FabricFile.Move(backupfullLogfileName, fullLogfileName);
                }
            }

            return await FileLogicalLog.CreateFileLogicalLog(
                fullLogfileName,
                this.logWriteFaultInjectionParameters).ConfigureAwait(false);
        }

        private Task DeleteLogFileAsync(string fileName)
        {
            var path = FileLogicalLog.GetFullPathToLog(this.LogFileDirectoryPath, fileName);

            if (FabricFile.Exists(path))
            {
                try
                {
                    FabricFile.Delete(path);
                }
                catch (Exception ex)
                {
                    // TODO: Exception/HRESULT work needs to be finished: catch (FabricElementNotFoundException)
                    FabricEvents.Events.LogManagerExceptionInfo(
                        this.Tracer.Type,
                        "DeleteLogFileAsync: Delete logical log: " + fileName,
                        ex.GetType().ToString(),
                        ex.Message,
                        ex.HResult,
                        ex.StackTrace);
                }
            }

            return Task.FromResult(0);
        }
    }
}