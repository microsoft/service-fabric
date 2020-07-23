// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Fabric.Dca.EtlConsumerHelper;
    using System.IO;
    using System.Linq;
    using System.Threading;
    using Tools.EtlReader;

    // This class implements the producer that reads ETL files and delivers events
    // from them to consumers
    internal class EtlInMemoryProducerWorker : IDisposable, IEtlProducer
    {
        private const string EtlReadTimerIdSuffix = "_EtlReadTimer";
        private const string StarDotEtl = "*.etl";
        
        // Block sync calls to FlushData while async timer read is happening.
        private readonly object etlReadCallbackLock = new object();

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // ETL in-memory sinks implemented by the EtlToInMemoryBufferWriter class
        private readonly ReadOnlyCollection<EtlToInMemoryBufferWriter> etlToInMemoryBufferWriters;

        // ETL in-memory sinks
        private readonly ReadOnlyCollection<IEtlInMemorySink> sinks;

        // Producer settings
        private readonly EtlInMemoryProducerWorkerSettings etlInMemoryProducerWorkerSettings;

        // Metadata representing each type of logs that we process
        // Example: fabric, lease
        private readonly ReadOnlyCollection<ProviderInfo> providers;

        // Directory where the DCA looks for trace files to process
        private readonly string traceDirectory;

        // Indicates when processing should stop due to object being disposed.
        private readonly CancellationTokenSource cancellationTokenSource;

        // Frequency at which the DCA performs its ETL processing pass 
        private readonly TimeSpan etlReadInterval;

        // Reference to object that implements the ETL processing functionality
        private readonly EtlProcessor etlProcessor;

        // Timer used to schedule ETL file processing
        private readonly DcaTimer etlReadTimer;

        private readonly BootstrapTraceProcessor bootstrapTraceProcessor;
        private readonly EtlInMemoryPerformance perfHelper;
        private readonly CheckpointManager checkpointManager;
        private readonly DiskSpaceManager diskSpaceManager;

        // Directory containing marker files for ETL files processed by the DCA. 
        // Bootstrap traces processed by the DCA are also archived in this directory.
        private readonly string markerFileDirectory;

        // Amount of time spent by the DCA (during a single ETL-processing pass) 
        // on processing ETL files from each provider
        private TimeSpan perProviderProcessingTime;

        // Whether or not the object has been disposed
        private bool disposed;

        internal EtlInMemoryProducerWorker(
            EtlInMemoryProducerWorkerParameters initParam,
            DiskSpaceManager diskSpaceManager,
            ITraceFileEventReaderFactory traceFileEventReaderFactory)
        {
            this.logSourceId = initParam.ProducerInstanceId;
            this.traceSource = initParam.TraceSource;
            this.cancellationTokenSource = new CancellationTokenSource();
            this.perfHelper = new EtlInMemoryPerformance(this.traceSource);
            this.diskSpaceManager = diskSpaceManager;

            // Initialize the settings
            this.etlInMemoryProducerWorkerSettings = EtlInMemoryProducerWorkerSettingsHelper.InitializeSettings(initParam);
            if (WinFabricEtlType.DefaultEtl == this.etlInMemoryProducerWorkerSettings.WindowsFabricEtlType)
            {
                // If we're processing the default ETL files, we should keep track of 
                // whether or not we're on the FMM node. This information is used by 
                // some other plugin types.
                Utility.LastFmmEventTimestamp = DateTime.MinValue;
            }

            // Initialize the sink list
            this.sinks = initParam.EtlInMemoryProducer.ConsumerSinks.Cast<IEtlInMemorySink>().ToList().AsReadOnly();
            this.etlToInMemoryBufferWriters = initParam.EtlInMemoryProducer.ConsumerSinks.OfType<EtlToInMemoryBufferWriter>().ToList().AsReadOnly();
            this.etlToInMemoryBufferWriters.ForEach(e => e.SetEtlProducer(this));

            // Figure out where the ETL files are located
            this.traceDirectory = EtlInMemoryProducerWorkerSettingsHelper.InitializeTraceDirectory(
                this.etlInMemoryProducerWorkerSettings.EtlPath,
                initParam.LogDirectory,
                this.etlInMemoryProducerWorkerSettings.WindowsFabricEtlType);

            this.markerFileDirectory = EtlInMemoryProducerWorkerSettingsHelper.InitializeMarkerFileDirectory(
                this.etlInMemoryProducerWorkerSettings.EtlPath,
                initParam.LogDirectory,
                this.traceDirectory,
                this.etlInMemoryProducerWorkerSettings.WindowsFabricEtlType);

            this.providers = EtlInMemoryProducerWorkerSettingsHelper.InitializeProviders(
                this.etlInMemoryProducerWorkerSettings.EtlPath,
                this.etlInMemoryProducerWorkerSettings.EtlFilePatterns,
                this.etlInMemoryProducerWorkerSettings.WindowsFabricEtlType,
                message => this.traceSource.WriteError(this.logSourceId, message)).AsReadOnly();

            if (0 == this.providers.Count)
            {
                // No ETL files to read, so return immediately
                this.traceSource.WriteWarning(
                    this.logSourceId,
                    "No ETL files have been specified for processing.");
            }

            this.checkpointManager = new CheckpointManager(
               this.etlInMemoryProducerWorkerSettings.EtlPath,
               initParam.LogDirectory,
               this.traceDirectory,
               this.traceSource,
               this.logSourceId);

            this.etlReadInterval = this.etlInMemoryProducerWorkerSettings.EtlReadInterval;
            if (this.etlReadInterval > TimeSpan.Zero)
            {
                // Create the directory that contains the marker files.
                this.CreateDirectoriesForEtlProcessing();

                // We need to collect bootstrap traces
                this.bootstrapTraceProcessor = new BootstrapTraceProcessor(
                    this.traceDirectory,
                    this.markerFileDirectory,
                    this.etlToInMemoryBufferWriters,
                    this.etlInMemoryProducerWorkerSettings.EtlReadInterval,
                    this.traceSource,
                    this.logSourceId);
                this.bootstrapTraceProcessor.Start();

                // Create the ETL processor
                this.etlProcessor = new EtlProcessor(
                    true,
                    this.IsProcessingWindowsFabricEtlFilesFromDefaultLocation(),
                    this.markerFileDirectory,
                    this.etlInMemoryProducerWorkerSettings.WindowsFabricEtlType,
                    this.traceSource,
                    this.logSourceId,
                    this.perfHelper,
                    this.sinks,
                    this.etlToInMemoryBufferWriters,
                    traceFileEventReaderFactory);

                // Create a periodic timer to read ETL files
                var timerId = string.Concat(
                    this.logSourceId,
                    EtlReadTimerIdSuffix);
                this.etlReadTimer = new DcaTimer(
                    timerId,
                    state => this.EtlReadCallback(this.cancellationTokenSource.Token),
                    this.etlReadInterval);
                this.etlReadTimer.Start();

                // Figure out how much processing time is available to each provider.
                this.ComputePerProviderEtlProcessingTimeSeconds();
            }

            // Disk manager set up for traces
            foreach (var provider in this.providers)
            {
                var capturedProvider = provider;
                this.diskSpaceManager.RegisterFolder(
                    this.logSourceId,
                    () => new DirectoryInfo(this.traceDirectory).EnumerateFiles(capturedProvider.EtlFileNamePattern),
                    f => FabricFile.Exists(Path.Combine(this.markerFileDirectory, f.Name)), // Safe to delete once marker file exists
                    f => f.LastWriteTimeUtc >= DateTime.UtcNow.Add(-initParam.LatestSettings.EtlDeletionAgeMinutes));
            }

            // Disk manager set up for marker files
            this.diskSpaceManager.RegisterFolder(
                this.logSourceId,
                () => new DirectoryInfo(this.markerFileDirectory).EnumerateFiles(),
                f => !FabricFile.Exists(Path.Combine(this.traceDirectory, f.Name)), // Safe to delete once original has been cleaned up
                f => f.LastWriteTimeUtc >= DateTime.UtcNow.Add(-initParam.LatestSettings.EtlDeletionAgeMinutes));
        }

        public void FlushData()
        {
            // Invoke the method that processes ETL files
            var tokenSource = new CancellationTokenSource();
            this.EtlReadCallback(tokenSource.Token);
        }

        public bool IsProcessingWindowsFabricEtlFiles()
        {
            return this.IsProcessingWindowsFabricEtlFilesFromDefaultLocation() ||
                   this.etlInMemoryProducerWorkerSettings.ProcessingWinFabEtlFiles;
        }

        public bool HasEtlFileBeenFullyProcessed(string etlFileName)
        {
            var etlFileFullPath = Path.Combine(
               this.traceDirectory,
               etlFileName);
            var markerFileFullPath = Path.Combine(
                this.markerFileDirectory,
                etlFileName);
            return (false == FabricFile.Exists(etlFileFullPath)) || FabricFile.Exists(markerFileFullPath);
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            this.disposed = true;

            // Set the cancellation token that indicates that the producer is being stopped
            this.cancellationTokenSource.Cancel();

            if (null != this.etlReadTimer)
            {
                this.etlReadTimer.StopAndDispose();
                this.etlReadTimer.DisposedEvent.WaitOne();
            }

            // Revoke our IEtlToInMemoryBufferWriter interface
            foreach (var etlToInMemoryBufferWriter in this.etlToInMemoryBufferWriters)
            {
                etlToInMemoryBufferWriter.SetEtlProducer(null);
            }

            if (this.bootstrapTraceProcessor != null)
            {
                this.bootstrapTraceProcessor.Dispose();
            }

            this.diskSpaceManager.UnregisterFolders(this.logSourceId);

            GC.SuppressFinalize(this);
        }

        private void EtlReadCallback(object cancellationToken)
        {
            lock (this.etlReadCallbackLock)
            {
                // Raise an event to indicate that we are beginning this ETL 
                // processing pass
                this.OnEtlProcessingPassBegin();

                var cancelToken = (CancellationToken)cancellationToken;

                // Process ETL files from each of the providers
                foreach (var provider in this.providers)
                {
                    this.perfHelper.EtlReadPassBegin();

                    this.ProcessEtlFiles(
                        provider.LastEtlReadFileName,
                        provider.EtlFileNamePattern,
                        provider.FriendlyName,
                        cancelToken);

                    this.perfHelper.EtlReadPassEnd(provider.FriendlyName);

                    if (cancelToken.IsCancellationRequested)
                    {
                        this.traceSource.WriteInfo(
                            this.logSourceId,
                            "The producer is being stopped, so no more ETL files will be processed in this pass.");
                        break;
                    }
                }

                // Raise an event to indicate that we are done with this ETL 
                // processing pass
                this.OnEtlProcessingPassEnd();

                this.etlReadTimer.Start(this.etlReadInterval);
            }
        }

        private void ProcessEtlFiles(string lastEtlReadFileName, string etlFileNamePattern, string friendlyName, CancellationToken cancellationToken)
        {
            // Figure out the last time stamp at which we stopped reading
            // ETW events. We need to continue from there.
            DateTime lastEndTime;
            if (false == this.checkpointManager.GetLastEndTime(lastEtlReadFileName, out lastEndTime))
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Failed to write the last timestamp up to which {0} ETW events have been read from file {1}. Due to this error, we will not be reading {2} ETW events in this pass.",
                    friendlyName,
                    lastEtlReadFileName,
                    friendlyName);
                return;
            }

            this.traceSource.WriteInfo(
                this.logSourceId,
                "Reading {0} ETW events after {1}.",
                friendlyName,
                lastEndTime);

            // Read ETW events from the available ETL files.
            DateTime endTime;
            this.ProcessEtlFilesWorker(etlFileNamePattern, lastEndTime, friendlyName, cancellationToken, out endTime);

            this.traceSource.WriteInfo(
                this.logSourceId,
                "Finished reading {0} ETW events up to {1}.",
                friendlyName,
                endTime);

            // Save the time stamp at which we stopped reading ETW events
            // now. We'll retrieve this value in our next pass.
            if (false == this.checkpointManager.SetLastEndTime(lastEtlReadFileName, endTime))
            {
                if (0 != lastEndTime.CompareTo(endTime))
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Failed to write the last timestamp up to which {0} ETW events have been read to file {1}. Due to this error, although we have read events up to {2}, in our next pass we will start reading events from {3}.",
                        friendlyName,
                        lastEtlReadFileName,
                        endTime,
                        lastEndTime);
                }
            }
        }

        private void ProcessEtlFilesWorker(
            string fileNamePattern,
            DateTime lastEndTime,
            string friendlyName,
            CancellationToken cancellationToken,
            out DateTime endTime)
        {
            // By default, initialize endTime to lastEndTime. If we run into a 
            // situation where we don't have any ETL files to process, the 
            // default value will not be updated.
            endTime = lastEndTime;

            if (!Directory.Exists(this.traceDirectory) &&
                this.traceDirectory.StartsWith(
                    (new ContainerEnvironment()).ContainerLogRootDirectory,
                    StringComparison.InvariantCultureIgnoreCase))
            {
                // A temporary handling for the containers case, where DCA could be processing an instance's
                // folder with a different older structure (if traces collectors or DCA itself got updated).
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Skipping a search cycle to match '{0}' as corresponding container instance directory '{1}' is not found.",
                    fileNamePattern,
                    this.traceDirectory);

                return;
            }

            // Get all files in the traces directory that match the pattern
            this.traceSource.WriteInfo(
                this.logSourceId,
                "Searching for ETL files whose names match '{0}' in directory '{1}'.",
                fileNamePattern,
                this.traceDirectory);

            var dirInfo = new DirectoryInfo(this.traceDirectory);
            var etlFiles = dirInfo.GetFiles(fileNamePattern);

            this.traceSource.WriteInfo(
                this.logSourceId,
                "Found {0} ETL files whose names match '{1}' in directory '{2}'.",
                etlFiles.Length,
                fileNamePattern,
                this.traceDirectory);

            if (0 == etlFiles.Length)
            {
                // No ETL files found. Nothing more to do here.
                return;
            }

            // We ignore files that are so old that they need to be deleted
            var cutoffTime = DateTime.UtcNow.Add(-this.etlInMemoryProducerWorkerSettings.EtlDeletionAge);

            // Reading in chronological order. Sort the files such that the
            // file with the least recent creation time comes first. 
            etlFiles = etlFiles.OrderBy(e => e.CreationTime).ToArray();

            // Ignore ETL files whose last-write time is older than the timestamp
            // up to which we have already read all events. However, we never 
            // ignore the active ETL file because its timestamp is updated only
            // after it has reached its full size.
            var activeEtlFileName = etlFiles[etlFiles.Length - 1].Name;
            etlFiles = etlFiles
                .Where(f => f.Name.Equals(activeEtlFileName, StringComparison.OrdinalIgnoreCase) ||
                            (f.LastWriteTimeUtc.CompareTo(lastEndTime) >= 0))
                .ToArray();

            // Processing start time
            var processingStartTime = DateTime.Now;
            var processingEndTime = processingStartTime.Add(this.perProviderProcessingTime);
            var filesProcessed = 0;
            for (int i = 0; i < etlFiles.Length; i++)
            {
                if (etlFiles[i].LastWriteTimeUtc.CompareTo(cutoffTime) <= 0)
                {
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "Skipping ETL file {0} in directory {1} because it is too old.",
                        etlFiles[i],
                        this.traceDirectory);
                    continue;
                }

                // If the file has already been fully processed, then ignore
                // that file.
                if (FabricFile.Exists(Path.Combine(this.markerFileDirectory, etlFiles[i].Name)))
                {
                    continue;
                }

                this.traceSource.WriteInfo(
                   this.logSourceId,
                   "Processing ETL file: {0}.",
                   etlFiles[i].Name);

                // If we are processing Windows Fabric ETL files from their default location, then
                // make sure that we have loaded the correct manifest for them.
                if (this.IsProcessingWindowsFabricEtlFilesFromDefaultLocation())
                {
                    this.etlProcessor.EnsureCorrectWinFabManifestVersionLoaded(etlFiles[i].Name);
                }

                bool isActiveEtl = i == etlFiles.Length - 1;

                if (isActiveEtl)
                {
                    // process active etl file and then break so that we can process additional
                    // events in the next pass.
                    this.etlProcessor.ProcessActiveEtlFile(
                           etlFiles[i],
                           lastEndTime,
                           cancellationToken,
                           out endTime);

                    filesProcessed++;

                    if (false == this.checkpointManager.SetLastEndTime(etlFiles[i].Name, endTime))
                    {
                        if (0 != lastEndTime.CompareTo(endTime))
                        {
                            this.traceSource.WriteError(
                                this.logSourceId,
                                "Failed to write the last timestamp up to which {0} ETW events have been read to file {1}. Due to this error, although we have read events up to {2}, in our next pass we will start reading events from {3}.",
                                friendlyName,
                                etlFiles[i].Name,
                                endTime,
                                lastEndTime);
                        }
                    }

                    break;
                }
                else
                {
                    this.etlProcessor.ProcessInactiveEtlFile(
                           etlFiles[i],
                           lastEndTime,
                           cancellationToken,
                           out endTime);

                    filesProcessed++;

                    if (false == this.checkpointManager.SetLastEndTime(etlFiles[i].Name, endTime))
                    {
                        if (0 != lastEndTime.CompareTo(endTime))
                        {
                            this.traceSource.WriteError(
                                this.logSourceId,
                                "Failed to write the last timestamp up to which {0} ETW events have been read to file {1}. Due to this error, although we have read events up to {2}, in our next pass we will start reading events from {3}.",
                                friendlyName,
                                etlFiles[i].Name,
                                endTime,
                                lastEndTime);
                        }
                    }
                    else
                    {
                        // set last end time so that the next iteration can calculate start
                        // time accurately
                        lastEndTime = endTime;
                    }
                }

                // check if time for this provider is up
                if (0 > DateTime.Compare(processingEndTime, DateTime.Now))
                {
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "Due to time limit on etl processing, no more {0} files will be processed in this pass.",
                        fileNamePattern);
                    break;
                }

                if (cancellationToken.IsCancellationRequested)
                {
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "The producer is being stopped, so no more {0} files will be processed in this pass.",
                        fileNamePattern);
                    break;
                }
            }

            // Write the length of the DCA's ETL file backlog. The performance test consumes this information.
            this.perfHelper.RecordEtlPassBacklog(friendlyName, etlFiles.Length - filesProcessed);
        }

        private void ComputePerProviderEtlProcessingTimeSeconds()
        {
            // The maximum amount of time we spend processing ETL files in a 
            // given pass is the interval between passes. This is an arbitrary choice.
            var maxProcessingTimeForAllProviders = TimeSpan.FromMilliseconds(this.etlReadInterval.TotalMilliseconds);

            // The maximum processing time is distributed equally among providers
            this.perProviderProcessingTime =
                TimeSpan.FromMilliseconds(maxProcessingTimeForAllProviders.TotalMilliseconds / this.providers.Count);

            this.traceSource.WriteInfo(
                this.logSourceId,
                "In a given ETL-processing pass, each ETW trace provider has {0} seconds to process its ETL files.",
                this.perProviderProcessingTime);
        }

        private bool IsProcessingWindowsFabricEtlFilesFromDefaultLocation()
        {
            return string.IsNullOrEmpty(this.etlInMemoryProducerWorkerSettings.EtlPath);
        }

        private void CreateDirectoriesForEtlProcessing()
        {
            // Create the directory to hold marker files for fully
            // processed ETL files
            FabricDirectory.CreateDirectory(this.markerFileDirectory);

            this.traceSource.WriteInfo(
                this.logSourceId,
                "Marker file directory: {0}",
                this.markerFileDirectory);
        }

        private void OnEtlProcessingPassBegin()
        {
            foreach (var sink in this.sinks)
            {
                sink.OnEtwEventProcessingPeriodStart();
            }
        }

        private void OnEtlProcessingPassEnd()
        {
            foreach (var sink in this.sinks)
            {
                sink.OnEtwEventProcessingPeriodStop();
            }
        }

        // Parameters passed to ETL producer worker object during initialization
        internal struct EtlInMemoryProducerWorkerParameters
        {
            internal FabricEvents.ExtensionsEvents TraceSource;
            internal string LogDirectory;
            internal string ProducerInstanceId;
            internal EtlInMemoryProducer EtlInMemoryProducer;
            internal EtlInMemoryProducerSettings LatestSettings;
        }

        // Settings related to ETL file based producers
        internal struct EtlInMemoryProducerWorkerSettings
        {
            internal TimeSpan EtlReadInterval;
            internal TimeSpan EtlDeletionAge;
            internal WinFabricEtlType WindowsFabricEtlType;
            internal string EtlPath;
            internal string EtlFilePatterns;
            internal bool ProcessingWinFabEtlFiles;
        }

        // Information about the providers for which we need to collect ETW traces
        internal class ProviderInfo
        {
            // String representing the file name pattern for ETL files from a provider
            internal readonly string EtlFileNamePattern;

            // Friendly name for the ETL provider. Used in logging.
            internal readonly string FriendlyName;

            internal ProviderInfo(string lastEtlReadFileName, string etlFileNamePattern, string friendlyName)
            {
                this.LastEtlReadFileName = lastEtlReadFileName;
                this.EtlFileNamePattern = etlFileNamePattern;
                this.FriendlyName = friendlyName;
            }

            // Name of the file that stores the latest timestamp up to which all
            // ETW events have been processed by the DCA
            internal string LastEtlReadFileName { get; }
        }
    }
}