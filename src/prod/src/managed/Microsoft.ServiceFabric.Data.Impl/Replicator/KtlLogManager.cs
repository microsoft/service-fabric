// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Data.Log;
    using System.Globalization;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Common.Tracing;
    using Microsoft.ServiceFabric.Data.Testability;
    using Microsoft.ServiceFabric.Replicator.Diagnostics;

    internal class KtlLogManager : LogManager
    {
        private int maximumMetadataSizeInKB;

        private int maximumRecordSizeInKb;

        private int maximumStreamSizeInMb;

        private int maxWriteQueueDepthInKb;

        private bool optimizeForLocalSsd;

        private bool optimizeForLowerDiskUsage;

        private IPhysicalLog physicalLog;

        private Guid physicalLogId;

        private ILogManager physicalLogManager;

        private string physicalLogName;

        private readonly LogWriteFaultInjectionParameters logWriteDelayParameters;

        // TODO:  Harvest this value from cluster manifest or somewhere
#if DotNetCoreClrLinux
        private System.Fabric.Data.Log.LogManager.LoggerType globalLogManagerType = System.Fabric.Data.Log.LogManager.LoggerType.Inproc;
#else
        private System.Fabric.Data.Log.LogManager.LoggerType globalLogManagerType = System.Fabric.Data.Log.LogManager.LoggerType.Default;
#endif

        private bool useStagingLog;

#if DotNetCoreClrLinux
        private static readonly string PlatformPathPrefix = "";
#else
        private static readonly string PlatformPathPrefix = "\\??\\";
#endif
        private static readonly long SharedContainerSize = 8 * (long)1024 * 1024 * 1024; // 8 GB
        private static readonly uint SharedMaximumNumberOfStreams = 3 * 8192;

        private static readonly long StagingContainerSize = 256 * (long)1024 * 1024; // 256MB
        private static readonly uint StagingMaximumNumberOfStreams = 1024;

        internal KtlLogManager(
            string baseLogFileAlias,
            IncomingBytesRateCounterWriter incomingBytesRateCounter,
            LogFlushBytesRateCounterWriter logFlushBytesRateCounter,
            AvgBytesPerFlushCounterWriter bytesPerFlushCounterWriter,
            AvgFlushLatencyCounterWriter avgFlushLatencyCounterWriter,
            AvgSerializationLatencyCounterWriter avgSerializationLatencyCounterWriter,
            LogWriteFaultInjectionParameters logWriteDelayParameters)
            : base(
                baseLogFileAlias,
                incomingBytesRateCounter,
                logFlushBytesRateCounter,
                bytesPerFlushCounterWriter,
                avgFlushLatencyCounterWriter,
                avgSerializationLatencyCounterWriter)
        {
            this.physicalLogManager = null;
            this.physicalLog = null;

            this.logWriteDelayParameters = logWriteDelayParameters;
        }

        ~KtlLogManager()
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
                var aliasGuid =
                    await this.physicalLog.ResolveAliasAsync(this.CurrentLogFileAlias, CancellationToken.None).ConfigureAwait(false);

                FabricEvents.Events.LogManager(
                    this.Tracer.Type,
                    "CreateCopyLog: Attempt to delete logical log " + this.CurrentLogFileAlias + " guid: " + aliasGuid);

                await this.physicalLog.DeleteLogicalLogAsync(aliasGuid, CancellationToken.None).ConfigureAwait(false);
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

            await this.OpenPhysicalLogAsync(CancellationToken.None).ConfigureAwait(false);

            //
            // The order that logs are deleted is important. The backup log should be deleted first, then the
            // copy log and finally the current log. The reason for this is that when a ChangeRole(None) occurs
            // the replicator will write a record in the current log which will indicate that the ChangeRole(None)
            // has occured so that if the process crashes and restarts, the ChangeRole(None) process can continue.
            // Otherwise, if the current log is deleted before the backup log, the backup log will become the 
            // current log and the information about ChangeRole(None) and other data is lost.
            //
            FabricEvents.Events.LogManager(this.Tracer.Type, "DeleteLog: Try delete log file " + this.BaseLogFileAlias + BackupSuffix);
            await this.DeleteLogFileAsync(this.BaseLogFileAlias + BackupSuffix, CancellationToken.None).ConfigureAwait(false);

            FabricEvents.Events.LogManager(this.Tracer.Type, "DeleteLog: Try delete log file " + this.BaseLogFileAlias + CopySuffix);
            await this.DeleteLogFileAsync(this.BaseLogFileAlias + CopySuffix, CancellationToken.None).ConfigureAwait(false);

            FabricEvents.Events.LogManager(this.Tracer.Type, "DeleteLog: Try delete log file " + this.BaseLogFileAlias);
            await this.DeleteLogFileAsync(this.BaseLogFileAlias, CancellationToken.None).ConfigureAwait(false);

            await this.ClosePhysicalLogAsync(CancellationToken.None).ConfigureAwait(false);

            if (this.useStagingLog)
            {
                //
                // Delete staging log associated with this replica since there is a single staging log for each replica
                //
                await this.DeletePhysicalLogAsync(CancellationToken.None).ConfigureAwait(false);
            }
        }

        internal override async Task InitializeAsync(ITracer inputTracer, LogManagerInitialization init, TransactionalReplicatorSettings settings)
        {
            //
            // Determine if we are in a container and if so then get the host namespace for the application

            //
            // When running in a container, the container will have a different file system namespace as the
            // host. The filenames are generated from within the container, however the KTLLogger driver will 
            // create them while running in the host namespace. In order for the files to be created in the 
            // correct location, the filename needs to be mapped from the container namespace to the host
            // namespace. This mapping is done here.
            //
            // Fabric provides the mapping into the host namespace in the environment variable named
            // "Fabric_HostApplicationDirectory"
            //
            string FabricHostApplicationDirectory;

            try
            {
                FabricHostApplicationDirectory = Environment.GetEnvironmentVariable(Constants.Fabric_HostApplicationDirectory);
            }
            catch (Exception)
            {
                FabricHostApplicationDirectory = null;
            }
            
            string containerDedicatedLogPath = null;
            if (FabricHostApplicationDirectory != null)
            {
                var message = string.Format(CultureInfo.InvariantCulture,
                                         "Mapped dedicated log path for container: From: {0} To: {1}",
                                         init.dedicatedLogPath,
                                         FabricHostApplicationDirectory);

                FabricEvents.Events.LogManager(inputTracer.Type, message);
                containerDedicatedLogPath = FabricHostApplicationDirectory;
            }

            //
            // Make a whole bunch of decisions about which type of log manager and logs to use. At the end
            // this.useStagingLog and the correct path for dedicatedLogPath will be setup for this replica. 
            //
            string sharedLogPath = settings.PublicSettings.SharedLogPath;
            Guid sharedLogId = Guid.Parse(settings.PublicSettings.SharedLogId);

            bool isDefaultSettings = (settings.PublicSettings.SharedLogId == Constants.DefaultSharedLogContainerGuid);
            bool isInContainer = (containerDedicatedLogPath != null);

            if (this.physicalLogManager == null)
            {
                this.physicalLogManager = await System.Fabric.Data.Log.LogManager.OpenAsync(globalLogManagerType, CancellationToken.None).ConfigureAwait(false);
            }
            System.Fabric.Data.Log.LogManager.LoggerType logManagerType = this.physicalLogManager.GetLoggerType();

            if (isDefaultSettings)
            {
                if (logManagerType == System.Fabric.Data.Log.LogManager.LoggerType.Inproc)
                {
                    //
                    // When running inproc, we must use the staging log
                    // Format: <workDir>/<partitionId>_<replicaId>.stlog
                    //
                    this.useStagingLog = true;
                    sharedLogId = Guid.NewGuid();
                    sharedLogPath = Path.Combine(init.workDirectory,
                                                 init.partitionId.ToString() + "_" +
                                                 init.replicaId.ToString() + ".stlog");
                }
                else if (logManagerType == System.Fabric.Data.Log.LogManager.LoggerType.Driver)
                {
                    //
                    // When running with the driver then we do not use staging log
                    //
                    this.useStagingLog = false;

                    if (isInContainer)
                    {
                        //
                        // If running with the driver then the dedicated log path needs to be mapped
                        // into the host namespace.
                        //
                        init.dedicatedLogPath = containerDedicatedLogPath;
                    }
                }
                else
                {
                    //
                    // Only Inproc and Driver are supported, bail out
                    //
                    FabricEvents.Events.LogManager(
                        inputTracer.Type,
                        "KtlLogManager.InitializeAsync: Unknown LogManagerType " + (int)logManagerType + " " + logManagerType.ToString()); 
                    throw new InvalidOperationException();
                }
            } else {
                //
                // If we are not using default shared log settings then the application must know what they are doing
                // and so just go with what they want and hope for the best.
                //
                this.useStagingLog = false;

                if (isInContainer)
                {
                    FabricEvents.Events.LogManager(
                        inputTracer.Type,
                        "KtlLogManager.InitializeAsync: Default shared log is not be used within a container." + 
                        " LogManagerType: " + logManagerType.ToString() +
                        " Shared Log: " + settings.PublicSettings.SharedLogPath + " " + settings.PublicSettings.SharedLogId.ToString());
                }
            }

            FabricEvents.Events.LogManager(
                inputTracer.Type,
                "KtlLogManager.InitializeAsync: " +
                " globalLogManagerType: " + globalLogManagerType.ToString() +
                " LogManagerType: " + logManagerType.ToString() +
                " isDefaultSettings: " + isDefaultSettings.ToString() +
                " isInContainer: " + isInContainer.ToString() +
                " partitionId: " + init.partitionId.ToString() +
                " replicaId: " + init.replicaId.ToString() +
                " Shared Log: " + sharedLogPath + " " + sharedLogId.ToString());

            await base.InitializeAsync(inputTracer, init, settings).ConfigureAwait(false);

            this.physicalLogName = sharedLogPath;
            this.physicalLogId = sharedLogId;

            this.maximumMetadataSizeInKB = (int) settings.PublicSettings.MaxMetadataSizeInKB;
            this.maximumRecordSizeInKb = settings.PublicSettings.MaxRecordSizeInKB.Value;
            this.maximumStreamSizeInMb = (int) settings.PublicSettings.MaxStreamSizeInMB;
            this.optimizeForLocalSsd = settings.PublicSettings.OptimizeForLocalSSD.Value;
            this.optimizeForLowerDiskUsage = settings.PublicSettings.OptimizeLogForLowerDiskUsage.Value;
            this.maxWriteQueueDepthInKb = (int) settings.PublicSettings.MaxWriteQueueDepthInKB;

            await this.EnsureSharedLogContainerAsync(this.physicalLogName, this.physicalLogId).ConfigureAwait(false);
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

                // Intent is to replace the data log (LogFileName) with the current copy log (currentLogFileName)
                // Steps to do this:
                // 1. Delete any logical log with an alias LogFileName + "backup"
                // 2. Change the alias of the logical log with the guid associated with LogFileName to baseLogFileName + "backup"
                // 3. Change the alias of the logical log with guid associated with currentLogFilename to baseLogFileName
                await
                    this.physicalLog.ReplaceAliasLogsAsync(
                        this.CurrentLogFileAlias,
                        this.BaseLogFileAlias,
                        this.BaseLogFileAlias + BackupSuffix,
                        CancellationToken.None).ConfigureAwait(false);

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
                    this.AvgSerializationLatencyCounterWriter);

                this.PhysicalLogWriter.ExitTcs = exitTcs;

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

        [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1126:PrefixCallsCorrectly", Justification = "Reviewed.")]
        protected override async Task<ILogicalLog> CreateLogFileAsync(
            bool createNew,
            CancellationToken cancellationToken)
        {
            Utility.Assert(
                (this.LogicalLog == null) || (this.LogicalLog.IsFunctional == false),
                "(this.logicalLog == null) || (this.logicalLog.IsFunctional == false) Is logical log present: {0}",
                this.LogicalLog != null);

            // Open or create the logical log
            ILogicalLog log = null;

            await this.OpenPhysicalLogAsync(cancellationToken).ConfigureAwait(false);
            try
            {
                // NOTE: only works to fully recover when currentLogFileAlias == baseLogFileAlias
                var currentLogId =
                    await
                        this.physicalLog.RecoverAliasLogsAsync(
                            this.CurrentLogFileAlias + CopySuffix,
                            this.CurrentLogFileAlias,
                            this.CurrentLogFileAlias + BackupSuffix,
                            cancellationToken).ConfigureAwait(false);

                var message =
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "CreateLogFileAsync: attempt to open logical log streamId {0} alias {1}, use -- ExtractLogicalLog \"{2}\" {3} {0} outfile.log -- to extract",
                        currentLogId,
                        this.CurrentLogFileAlias,
                        this.physicalLogName,
                        this.physicalLogId);

                FabricEvents.Events.LogManager(this.Tracer.Type, message);

                log = await this.physicalLog.OpenLogicalLogAsync(currentLogId, this.Tracer.Type, cancellationToken).ConfigureAwait(false);
            }
            catch (Exception ex)
            {
                FabricEvents.Events.LogManager(
                    this.Tracer.Type,
                    "CreateLogFileAsync: Encountered Exception either during physical log recover alias or open logical log: " + ex);

                // TODO asnegi: RDBug #10278280 Adding 0xC0000225 as it is expected HResult for exception. However, it seems that it is not being
                // translated to 0x80070490 or generated properly in dotnet_coreclr_linux.
                if ((ex.InnerException == null)
                    || ((ex.InnerException.HResult != unchecked((int) 0x80070490)) && (ex.InnerException.HResult != unchecked((int)0xC0000225)))
                    || (ex.InnerException.InnerException != null && ex.InnerException.InnerException.HResult != unchecked((int)0xC0000225)))
                {
                    // Only ERROR_NOT_FOUND is an expected result code. Any other error should be passed back up
                    throw;
                }
            }

            if (log == null)
            {
                if (!createNew)
                {
                    FabricEvents.Events.LogManager(
                        this.Tracer.Type,
                        "CreateLogFileAsync: Expected to find log file but does not exist");
                }

                // Assume open failed above - try creation of new logical log
                var newLogicalLog = Guid.NewGuid();
                var fullLogfileName = Path.Combine(PlatformPathPrefix + this.LogFileDirectoryPath, newLogicalLog + ".SFlog");
                var trace = "CreateLogFileAsync: Attempt to create logical log path: " + fullLogfileName + " StreamId: "
                            + newLogicalLog + " Alias: " + this.CurrentLogFileAlias + "use -- ExtractLogicalLog \""
                            + this.physicalLogName + "\" " + this.physicalLogId + " " + newLogicalLog
                            + " outfile.log -- to extract";

                FabricEvents.Events.LogManager(
                    this.Tracer.Type,
                    trace);

                try
                {
                    // log created here - AMW
                    var maximumStreamSizeInBytes = this.maximumStreamSizeInMb*(long) (1024*1024);

                    log =
                        await
                            this.physicalLog.CreateLogicalLogAsync(
                                newLogicalLog,
                                this.CurrentLogFileAlias,
                                fullLogfileName,
                                null,
                                maximumStreamSizeInBytes,
                                checked((uint) this.maximumRecordSizeInKb*1024),
                                this.optimizeForLowerDiskUsage ? System.Fabric.Data.Log.LogManager.LogCreationFlags.UseSparseFile : 0,
                                this.Tracer.Type,
                                cancellationToken).ConfigureAwait(false);
                }
                catch (Exception ex)
                {
                    FabricEvents.Events.LogManagerExceptionError(
                        this.Tracer.Type,
                        "CreateLogFileAsync: Failed to create logical log path " + fullLogfileName + " StreamID: " + newLogicalLog + " alias: " +
                        this.CurrentLogFileAlias,
                        ex.GetType().ToString(),
                        ex.Message,
                        (ex.InnerException != null) ? ex.InnerException.HResult : ex.HResult,
                        ex.StackTrace);

                    throw;
                }
            }

            if (this.optimizeForLocalSsd)
            {
                // Only write to the dedicated log
                await log.ConfigureWritesToOnlyDedicatedLogAsync(CancellationToken.None).ConfigureAwait(false);
            }

            FabricEvents.Events.LogManager(this.Tracer.Type, "MaxWriteQueueDepth is " + this.maxWriteQueueDepthInKb);

            System.Fabric.Data.Log.LogManager.LogicalLog llog = log as System.Fabric.Data.Log.LogManager.LogicalLog;
            llog.SetWriteDelay(this.logWriteDelayParameters);

            return log;
        }

        protected override void DisposeInternal(bool isDisposing)
        {
            if (isDisposing != true || this.IsDisposed)
            {
                return;
            }

            base.DisposeInternal(true);

            if (this.physicalLog != null)
            {
                this.physicalLog.Dispose();
            }

            if (this.physicalLogManager != null)
            {
                this.physicalLogManager.Dispose();
            }
        }

        private async Task DeleteLogFileAsync(string alias, CancellationToken token)
        {
            try
            {
                var logId = await this.physicalLog.ResolveAliasAsync(alias, token).ConfigureAwait(false);
                await this.physicalLog.DeleteLogicalLogAsync(logId, token).ConfigureAwait(false);
            }
            catch (Exception ex)
            {
                // TODO: Exception/HRESULT work needs to be finished: catch (FabricElementNotFoundException)
                FabricEvents.Events.LogManagerExceptionInfo(
                    this.Tracer.Type,
                    "DeleteLogFileAsync: Delete logical log: " + alias,
                    ex.GetType().ToString(),
                    ex.Message,
                    ex.HResult,
                    ex.StackTrace);
            }
        }

        private async Task EnsureSharedLogContainerAsync(string pathToLogContainer, Guid logId)
        {
            try
            {
                FabricDirectory.CreateDirectory(Path.GetDirectoryName(pathToLogContainer));
            }
            catch (Exception ex)
            {
                FabricEvents.Events.LogManager(
                    this.Tracer.Type,
                    "EnsureSharedLogContainerAsync: Exception: " + ex.Message + ex);
            }

            IPhysicalLog logContainer;
            // See if the log container already exists
            try
            {
                logContainer =
                    await
                        this.physicalLogManager.OpenPhysicalLogAsync(
                            PlatformPathPrefix + pathToLogContainer,
                            logId,
                            this.useStagingLog,
                            CancellationToken.None).ConfigureAwait(false);
            }
            catch (Exception)
            {
                logContainer = null;
            }

            if (logContainer == null)
            {
                long containerSize;
                uint maximumNumberStreams;

                if (this.useStagingLog)
                {
                    containerSize = StagingContainerSize;
                    maximumNumberStreams = StagingMaximumNumberOfStreams;
                }
                else
                {
                    containerSize = SharedContainerSize;
                    maximumNumberStreams = SharedMaximumNumberOfStreams;
                }

                uint maxBlockSize = 0; // Use default

                // log container doesn't exist so let's create it

                try
                {
                    await this.physicalLogManager.CreatePhysicalLogAsync(
                        PlatformPathPrefix + pathToLogContainer,
                        logId,
                        containerSize,
                        maximumNumberStreams,
                        maxBlockSize,
                        0,
                        CancellationToken.None).ConfigureAwait(false);
                }
                catch (Exception ex)
                {
                    bool ignoreException;

                    ignoreException = ((ex is UnauthorizedAccessException) ||                         // Thrown from OnCreatePhysicalLog
                                       ((ex is System.Fabric.FabricException) &&
                                        (ex.InnerException != null) &&
                                        (ex.InnerException.HResult == unchecked((int)0x800700B7))));  // 800700B7 is ERROR_ALREADY_EXISTS

                    if (! ignoreException)
                    {
                        //
                        // If the create log returns any error other than ERROR_ALREADY_EXISTS or an UnauthorizedAccessException
                        // then we want to propogate this exception up as this will provide best chance for users to determine 
                        // the cause that the replica did not come up.
                        //
                        // ERROR_ALREADY_EXISTS and the UnauthorizedAccessException are both benign exceptions. This is due to the fact
                        // that there can be multiple replicas that are opening at the same time and each of them will try to ensure
                        // that the shared log is created. When they race, one of them may create the shared log file and managed code
                        // data structures while the other "loses" the race. ERROR_ALREADY_EXISTS will come from the file system while
                        // UnauthorizedAccessException will come from the logical log managed code in OnCreatePhysicalLog
                        //
                        FabricEvents.Events.LogManagerExceptionError(
                            this.Tracer.Type,
                            "EnsureSharedLogContainerAsync: Failed to create " + pathToLogContainer + " logID: " + logId,
                            ex.GetType().ToString(),
                            ex.Message,
                            (ex.InnerException != null) ? ex.InnerException.HResult : ex.HResult,
                            ex.StackTrace);
                        logContainer = null;
                        throw;
                    }

                    logContainer = null;
                }
            }
        }

        private async Task OpenPhysicalLogAsync(CancellationToken cancellationToken)
        {
            if (this.physicalLogManager == null)
            {
                this.physicalLogManager = await System.Fabric.Data.Log.LogManager.OpenAsync(globalLogManagerType, CancellationToken.None).ConfigureAwait(false);
            }

            // Assumption: an out-of-band management behavior has created the physical log
            if (this.physicalLog == null)
            {
                var physicalLogPathName = PlatformPathPrefix + this.physicalLogName;

                try
                {
                    FabricEvents.Events.LogManager(
                        this.Tracer.Type,
                        "OpenPhysicalLogAsync: Attempting to open physical log path " + physicalLogPathName + " logID: " + this.physicalLogId);

                    this.physicalLog =
                        await
                            this.physicalLogManager.OpenPhysicalLogAsync(
                                physicalLogPathName,
                                this.physicalLogId,
                                this.useStagingLog,
                                cancellationToken).ConfigureAwait(false);

                    FabricEvents.Events.LogManager(
                        this.Tracer.Type,
                        "OpenPhysicalLogAsync: Successfully opened physical log path " + physicalLogPathName + " logID: " + this.physicalLogId);
                }
                catch (Exception ex)
                {
                    FabricEvents.Events.LogManagerExceptionError(
                        this.Tracer.Type,
                        "OpenPhysicalLogAsync: Failed to open " + physicalLogPathName + " logID: " + this.physicalLogId,
                        ex.GetType().ToString(),
                        ex.Message,
                        ex.HResult,
                        ex.StackTrace);

                    throw;
                }
            }
        }

        private async Task ClosePhysicalLogAsync(CancellationToken cancellationToken)
        {
            // Close in the reverse order of open.
            var snapPhysicalLog = this.physicalLog;
            if (snapPhysicalLog != null)
            {
                this.physicalLog = null;
                await snapPhysicalLog.CloseAsync(cancellationToken).ConfigureAwait(false);
                snapPhysicalLog.Dispose();
            }

            var snapPhysicalLogManager = this.physicalLogManager;
            if (snapPhysicalLogManager != null)
            {
                this.physicalLogManager = null;
                await snapPhysicalLogManager.CloseAsync(cancellationToken).ConfigureAwait(false);
                snapPhysicalLogManager.Dispose();
            }
        }

        private async Task DeletePhysicalLogAsync(CancellationToken cancellationToken)
        {
            if (this.physicalLogManager == null)
            {
                this.physicalLogManager = await System.Fabric.Data.Log.LogManager.OpenAsync(globalLogManagerType, CancellationToken.None).ConfigureAwait(false);
            }

            // Assumption: an out-of-band management behavior has created the physical log
            if (this.physicalLog == null)
            {
                var physicalLogPathName = PlatformPathPrefix + this.physicalLogName;

                try
                {
                    FabricEvents.Events.LogManager(
                        this.Tracer.Type,
                        "DeletePhysicalLogAsync: Attempting to delete physical log path " + physicalLogPathName + " logID: " + this.physicalLogId);

                    await
                        this.physicalLogManager.DeletePhysicalLogAsync(
                                physicalLogPathName,
                                Guid.Empty,
                                cancellationToken).ConfigureAwait(false);

                    FabricEvents.Events.LogManager(
                        this.Tracer.Type,
                        "DeletePhysicalLogAsync: Successfully deleted physical log path " + physicalLogPathName + " logID: " + this.physicalLogId);
                }
                catch (Exception ex)
                {
                    FabricEvents.Events.LogManagerExceptionError(
                        this.Tracer.Type,
                        "DeletePhysicalLogAsync: Failed to delete " + physicalLogPathName + " logID: " + this.physicalLogId,
                        ex.GetType().ToString(),
                        ex.Message,
                        ex.HResult,
                        ex.StackTrace);

                    throw;
                }
            }
        }
    }
}