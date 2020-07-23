// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Diagnostics;
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
    internal class EtlProducerWorker : IDisposable, IEtlProducer
    {
        private const string EtlReadTimerIdSuffix = "_EtlReadTimer";
        private const string StarDotEtl = "*.etl";

        // List of directories containing marker files that indicate which ETL files have already been processed
        private static readonly List<string> InternalMarkerFileDirectoriesForApps = new List<string>();

        // Dictionary of ETL files being processed by various providers in the 
        // system. The key is the path to the folder containing the ETL file. The
        // value is the list of ETL file patterns being processed in that folder.
        // Whenever a producer instance figures out which ETL files it is supposed
        // to process it checks those files against this dictionary to make sure 
        // that no other producer instance is processing the same ETL files.
        private static readonly Dictionary<string, List<string>> KnownEtlFilePatterns =
            new Dictionary<string, List<string>>();

        // Block sync calls to FlushData while async timer read is happening.
        private readonly object etlReadCallbackLock = new object();

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // List of patterns that the current instance has added to the dictionary
        // of ETL file patterns. This list is used for removing those patterns later.
        private readonly ReadOnlyCollection<string> patternsAddedToKnownEtlFileSet;

        // ETL file sinks implemented by the BufferedEtwEventProvider class
        private readonly ReadOnlyCollection<BufferedEtwEventProvider> bufferedEtwEventProviders;

        // ETL file sinks implemented by the EtlToCsvFileWriter class
        private readonly ReadOnlyCollection<EtlToCsvFileWriter> etlToCsvFileWriters;

        // ETL file sinks
        private readonly ReadOnlyCollection<IEtlFileSink> sinks;

        // Settings
        private readonly EtlProducerWorkerSettings etlProducerSettings;

        private readonly ReadOnlyCollection<ProviderInfo> providers;

        // Directory where the DCA looks for trace files to process
        private readonly string traceDirectory;

        // Frequency at which the DCA performs its ETL processing pass 
        private readonly TimeSpan etlReadInterval;

        // Reference to object that implements the ETL processing functionality
        private readonly EtlProcessor etlProcessor;

        // Timer used to schedule ETL file processing
        private readonly DcaTimer etlReadTimer;

        private readonly bool isReadingFromApplicationManifest;

        private readonly BootstrapTraceProcessor bootstrapTraceProcessor;
        private readonly EtlPerformance perfHelper;
        private readonly CheckpointManager checkpointManager;
        private readonly DiskSpaceManager diskSpaceManager;

        // Directory containing marker files for ETL files processed by the DCA. 
        // Bootstrap traces processed by the DCA are also archived in this directory.
        private readonly string markerFileDirectory;

        // Indicates when processing should stop due to object being disposed.
        private readonly CancellationTokenSource cancellationTokenSource;

        // Amount of time spent by the DCA (during a single ETL-processing pass) 
        // on processing ETL files from each provider
        private TimeSpan perProviderProcessingTime;

        // Whether or not the object has been disposed
        private bool disposed;

        internal EtlProducerWorker(
            EtlProducerWorkerParameters initParam,
            DiskSpaceManager diskSpaceManager,
            ITraceFileEventReaderFactory traceFileEventReaderFactory)
        {
            this.logSourceId = initParam.ProducerInstanceId;
            this.traceSource = initParam.TraceSource;
            this.isReadingFromApplicationManifest = initParam.IsReadingFromApplicationManifest;
            this.perfHelper = new EtlPerformance(this.traceSource);
            this.cancellationTokenSource = new CancellationTokenSource();
            this.diskSpaceManager = diskSpaceManager;

            // Initialize the settings
            this.etlProducerSettings = EtlProducerWorkerSettingsHelper.InitializeSettings(initParam);
            if (WinFabricEtlType.DefaultEtl == this.etlProducerSettings.WindowsFabricEtlType)
            {
                // If we're processing the default ETL files, we should keep track of 
                // whether or not we're on the FMM node. This information is used by 
                // some other plugin types.
                Utility.LastFmmEventTimestamp = DateTime.MinValue;
            }

            // Initialize the sink list
            this.sinks = EtlProducerWorkerSettingsHelper.InitializeSinks(
                initParam.EtlProducers,
                message => this.traceSource.WriteError(this.logSourceId, message)).AsReadOnly();
            this.etlToCsvFileWriters =
                EtlProducerWorkerSettingsHelper.InitializeFileWriters(this.sinks, this).AsReadOnly();
            this.bufferedEtwEventProviders =
                EtlProducerWorkerSettingsHelper.InitializeBufferedEtwEventProviders(this.sinks, this).AsReadOnly();

            // Figure out where the ETL files are located
            this.traceDirectory = EtlProducerWorkerSettingsHelper.InitializeTraceDirectory(
                initParam.IsReadingFromApplicationManifest,
                this.etlProducerSettings.EtlPath,
                initParam.LogDirectory,
                this.etlProducerSettings.WindowsFabricEtlType);

            this.markerFileDirectory = EtlProducerWorkerSettingsHelper.InitializeMarkerFileDirectory(
                initParam.IsReadingFromApplicationManifest,
                this.etlProducerSettings.EtlPath,
                initParam.LogDirectory,
                this.etlProducerSettings.WindowsFabricEtlType,
                this.traceDirectory,
                initParam.ProducerInstanceId);

            if (initParam.IsReadingFromApplicationManifest)
            {
                lock (InternalMarkerFileDirectoriesForApps)
                {
                    InternalMarkerFileDirectoriesForApps.Add(this.markerFileDirectory);
                }
            }

            this.providers = EtlProducerWorkerSettingsHelper.InitializeProviders(
                initParam.IsReadingFromApplicationManifest,
                this.etlProducerSettings.EtlPath,
                this.etlProducerSettings.WindowsFabricEtlType,
                this.etlProducerSettings.EtlFilePatterns,
                message => this.traceSource.WriteError(this.logSourceId, message)).AsReadOnly();

            if (0 == this.providers.Count)
            {
                // No ETL files to read, so return immediately
                this.traceSource.WriteWarning(
                    this.logSourceId,
                    "No ETL files have been specified for processing.");
            }

            this.checkpointManager = new CheckpointManager(
                initParam.IsReadingFromApplicationManifest,
                this.etlProducerSettings.EtlPath,
                initParam.LogDirectory,
                this.traceDirectory,
                initParam.ProducerInstanceId,
                this.traceSource,
                this.logSourceId);

            if (false == initParam.IsReadingFromApplicationManifest)
            {
                // Ensure that no other instance of EtlProducerWorker is processing the 
                // same ETL files
                var patternsAdded = new List<string>();
                var isUnique = VerifyEtlFilesUniqueToCurrentInstance(
                    this.traceDirectory,
                    this.providers,
                    patternsAdded,
                    message => this.traceSource.WriteError(this.logSourceId, message));
                this.patternsAddedToKnownEtlFileSet = patternsAdded.AsReadOnly();

                if (!isUnique)
                {
                    throw new InvalidOperationException(
                        string.Format("{0} is already being monitored for files matching one of the file patterns.", this.traceDirectory));
                }
            }

            this.etlReadInterval = this.etlProducerSettings.EtlReadInterval;
            if (this.etlReadInterval > TimeSpan.Zero)
            {
                // Create the directory that contains the marker files.
                this.CreateDirectoriesForEtlProcessing();

                if (false == initParam.IsReadingFromApplicationManifest)
                {
                    // We need to collect bootstrap traces
                    this.bootstrapTraceProcessor = new BootstrapTraceProcessor(
                        this.traceDirectory,
                        this.markerFileDirectory,
                        this.etlToCsvFileWriters,
                        this.etlProducerSettings.EtlReadInterval,
                        this.traceSource,
                        this.logSourceId);
                    this.bootstrapTraceProcessor.Start();
                }

                // Create the ETL processor
                this.etlProcessor = new EtlProcessor(
                    false == initParam.IsReadingFromApplicationManifest,
                    this.IsProcessingWindowsFabricEtlFilesFromDefaultLocation(),
                    this.etlProducerSettings.CustomManifestPaths,
                    initParam.ApplicationType,
                    this.markerFileDirectory,
                    this.etlProducerSettings.WindowsFabricEtlType,
                    this.traceSource,
                    this.logSourceId,
                    this.perfHelper,
                    this.sinks,
                    this.etlToCsvFileWriters,
                    this.bufferedEtwEventProviders,
                    this.etlProducerSettings.AppEtwGuids,
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

                // If there is a huge backlog of ETL files to process, we limit the
                // amount of time that we spend on processing ETL files in each 
                // pass. Figure out how much processing time is available to each 
                // provider.
                this.ComputePerProviderEtlProcessingTimeSeconds();
            }

            foreach (var provider in this.providers)
            {
                var capturedProvider = provider;
                if (Directory.Exists(this.traceDirectory))
                {
                    diskSpaceManager.RegisterFolder(
                    this.logSourceId,
                    () => new DirectoryInfo(this.traceDirectory).EnumerateFiles(capturedProvider.EtlFileNamePattern),
                    f => FabricFile.Exists(Path.Combine(this.markerFileDirectory, f.Name)), // Safe to delete once marker file exists
                    f => f.LastWriteTimeUtc >= DateTime.UtcNow.Add(-initParam.LatestSettings.EtlDeletionAgeMinutes));
                }
            }

            if (Directory.Exists(this.markerFileDirectory))
            {
                diskSpaceManager.RegisterFolder(
                    this.logSourceId,
                    () => new DirectoryInfo(this.markerFileDirectory).EnumerateFiles(),
                    f => !FabricFile.Exists(Path.Combine(this.traceDirectory, f.Name)), // Safe to delete once original has been cleaned up
                    f => f.LastWriteTimeUtc >= DateTime.UtcNow.Add(-initParam.LatestSettings.EtlDeletionAgeMinutes));
            }
        }

        internal static List<string> MarkerFileDirectoriesForApps
        {
            get
            {
                List<string> currentMarkerFileDirectories;
                lock (InternalMarkerFileDirectoriesForApps)
                {
                    currentMarkerFileDirectories = new List<string>(InternalMarkerFileDirectoriesForApps);
                }

                return currentMarkerFileDirectories;
            }
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

            if (null == this.etlReadTimer)
            {
                return;
            }

            this.etlReadTimer.StopAndDispose();
            this.etlReadTimer.DisposedEvent.WaitOne();

            if (this.isReadingFromApplicationManifest)
            {
                lock (InternalMarkerFileDirectoriesForApps)
                {
                    InternalMarkerFileDirectoriesForApps.Remove(this.markerFileDirectory);
                }
            }
            else
            {
                // Remove any patterns that we added to the ETL file set
                RemovePatternsAddedToEtlFileSet(this.patternsAddedToKnownEtlFileSet, this.traceDirectory);
            }

            // Revoke our IEtlProducer interface
            foreach (var etlToCsvFileWriter in this.etlToCsvFileWriters)
            {
                etlToCsvFileWriter.SetEtlProducer(null);
            }

            foreach (var bufferedEtwEventProvider in this.bufferedEtwEventProviders)
            {
                bufferedEtwEventProvider.SetEtlProducer(null);
            }

            if (this.bootstrapTraceProcessor != null)
            {
                this.bootstrapTraceProcessor.Dispose();
            }

            this.diskSpaceManager.UnregisterFolders(this.logSourceId);

            GC.SuppressFinalize(this);
        }

        public bool IsProcessingWindowsFabricEtlFiles()
        {
            return this.IsProcessingWindowsFabricEtlFilesFromDefaultLocation() ||
                   this.etlProducerSettings.ProcessingWinFabEtlFiles;
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

        public void FlushData()
        {
            // Invoke the method that processes ETL files
            var tokenSource = new CancellationTokenSource();
            this.EtlReadCallback(tokenSource.Token);
        }

        private static bool VerifyEtlFilesUniqueToCurrentInstance(
            string traceDirectory,
            IEnumerable<ProviderInfo> providers,
            List<string> patternsAddedToKnownEtlFileSet,
            Action<string> onDuplicateFilePattern)
        {
            var unique = true;
            lock (KnownEtlFilePatterns)
            {
                // Check if other producers are processing ETL files in this folder
                if (KnownEtlFilePatterns.ContainsKey(traceDirectory))
                {
                    // Check if other producers are processing ETL files whose 
                    // patterns that match those of the ETL files that we are about
                    // to process.
                    foreach (var provider in providers)
                    {
                        var index = provider.EtlFileNamePattern.LastIndexOf(
                            StarDotEtl,
                            StringComparison.OrdinalIgnoreCase);
                        var patternPrefix = provider.EtlFileNamePattern.Remove(index);

                        foreach (var knownEtlFilePattern in KnownEtlFilePatterns[traceDirectory])
                        {
                            if (patternPrefix.StartsWith(knownEtlFilePattern, StringComparison.OrdinalIgnoreCase) ||
                                knownEtlFilePattern.StartsWith(patternPrefix, StringComparison.OrdinalIgnoreCase))
                            {
                                // Another producer is processing ETL files whose
                                // pattern matches that of the ETL files that we
                                // are about to process. We do not support this.
                                onDuplicateFilePattern(string.Format(
                                    "Cannot process ETL files with pattern {0} in directory {1} because another producer is processing ETL files with matching pattern {2} in the same directory.",
                                    provider.EtlFileNamePattern,
                                    traceDirectory,
                                    string.Concat(knownEtlFilePattern, StarDotEtl)));
                                unique = false;
                                break;
                            }
                        }

                        if (unique)
                        {
                            // No other producer is processing ETL files whose
                            // pattern matches that of the ETL files we are about
                            // to process.
                            // Add our ETL file pattern to the dictionary so that
                            // other producers do not try to process the same files.
                            KnownEtlFilePatterns[traceDirectory].Add(patternPrefix);
                            patternsAddedToKnownEtlFileSet.Add(patternPrefix);
                        }
                    }
                }
                else
                {
                    // Add our ETL file patterns to the dictionary so that other
                    // producers do not try to process the same files.
                    KnownEtlFilePatterns[traceDirectory] = new List<string>();
                    foreach (var provider in providers)
                    {
                        var index = provider.EtlFileNamePattern.LastIndexOf(
                            StarDotEtl,
                            StringComparison.OrdinalIgnoreCase);
                        var patternPrefix = provider.EtlFileNamePattern.Remove(index);
                        KnownEtlFilePatterns[traceDirectory].Add(patternPrefix);
                        patternsAddedToKnownEtlFileSet.Add(patternPrefix);
                    }
                }
            }

            if (false == unique)
            {
                // We will not be processing any ETL files because some of the
                // ETL files that match our patterns also match the patterns of
                // other producers. Therefore, if we already added any patterns
                // to the dictionary, remove them now.
                RemovePatternsAddedToEtlFileSet(patternsAddedToKnownEtlFileSet, traceDirectory);
            }

            return unique;
        }

        private static void RemovePatternsAddedToEtlFileSet(
            IEnumerable<string> patternsAddedToKnownEtlFileSet,
            string traceDirectory)
        {
            foreach (var pattern in patternsAddedToKnownEtlFileSet)
            {
                lock (KnownEtlFilePatterns)
                {
                    KnownEtlFilePatterns[traceDirectory].Remove(pattern);
                }
            }
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

        private void ComputePerProviderEtlProcessingTimeSeconds()
        {
            // The maximum amount of time we spend processing ETL files in a 
            // given pass is roughly 1/4th of the interval between passes. This
            // is an arbitrary choice.
            var maxProcessingTimeForAllProviders = TimeSpan.FromMilliseconds(this.etlReadInterval.TotalMilliseconds / 4);

            // The maximum processing time is distributed equally among providers
            this.perProviderProcessingTime =
                TimeSpan.FromMilliseconds(maxProcessingTimeForAllProviders.TotalMilliseconds / this.providers.Count);

            this.traceSource.WriteInfo(
                this.logSourceId,
                "In a given ETL-processing pass, each ETW trace provider has {0} seconds to process its ETL files.",
                this.perProviderProcessingTime);
        }

        private void ProcessEtlFilesWorker(
            string fileNamePattern,
            DateTime lastEndTime,
            out DateTime endTime,
            string friendlyName,
            CancellationToken cancellationToken)
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
            var cutoffTime = DateTime.UtcNow.Add(-this.etlProducerSettings.EtlDeletionAge);

            // Sort the files such that the file with the most recent creation
            // time comes first. We'll process files in that order so that in 
            // case of huge backlogs the most recent (and hence likely to be 
            // most interesting) traces are processed first.
            Array.Sort(etlFiles, (file1, file2) => DateTime.Compare(file2.CreationTime, file1.CreationTime));

            // Ignore ETL files whose last-write time is older than the timestamp
            // up to which we have already read all events. However, we never 
            // ignore the active ETL file because its timestamp is updated only
            // after it has reached its full size.
            var activeEtlFileName = etlFiles[0].Name;
            etlFiles = etlFiles
                .Where(f => f.Name.Equals(activeEtlFileName, StringComparison.OrdinalIgnoreCase) ||
                            (f.LastWriteTimeUtc.CompareTo(lastEndTime) >= 0))
                .ToArray();

            var processingEndTime = DateTime.MaxValue;
            var fileProcessingInterrupted = false;
            var filesProcessed = 0;
            foreach (var etlFile in etlFiles)
            {
                // If the file is not an active ETL file but is so old that it
                // needs to be deleted, then ignore that file. Also ignore the
                // remaining files because they will be even older than this one
                // because we had sorted the files in increasing order of age.
                if (0 != filesProcessed)
                {
                    if (etlFile.LastWriteTimeUtc.CompareTo(cutoffTime) <= 0)
                    {
                        this.traceSource.WriteInfo(
                            this.logSourceId,
                            "No more ETL files whose names match '{0}' in directory '{1}' will be processed because the remaining files are too old.",
                            fileNamePattern,
                            this.traceDirectory);
                        break;
                    }
                }

                // If the file has already been fully processed, then ignore
                // that file. Note that since we work on ETL files starting 
                // with the newest one first, we can have situations where
                // we have fully processed a newer file, but older files are
                // still unprocessed.
                if (FabricFile.Exists(Path.Combine(this.markerFileDirectory, etlFile.Name)))
                {
                    continue;
                }

                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Processing ETL file: {0}.",
                    etlFile.Name);

                // If we are processing Windows Fabric ETL files from their default location, then
                // make sure that we have loaded the correct manifest for them.
                if (this.IsProcessingWindowsFabricEtlFilesFromDefaultLocation())
                {
                    this.etlProcessor.EnsureCorrectWinFabManifestVersionLoaded(etlFile.Name);
                }

                if (0 == filesProcessed)
                {
                    // The first file is treated differently because it is the
                    // file with the most recent creation time. It is possible 
                    // that ETW is still writing to this file. Therefore, we'll
                    // need to revisit this file in our next pass too.
                    this.etlProcessor.ProcessActiveEtlFile(
                        etlFile,
                        lastEndTime,
                        out endTime,
                        cancellationToken);
                }
                else
                {
                    this.etlProcessor.ProcessInactiveEtlFile(etlFile, lastEndTime, cancellationToken);
                }

                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Finished processing ETL file: {0}.",
                    etlFile.Name);

                filesProcessed++;

                // We've already incremented filesProcessed, so compare with 1 to test for first file
                Debug.Assert(filesProcessed > 0, "File was just processed so total should be non-zero positive value.");
                if (1 == filesProcessed)
                {
                    // Set an end time for ETL file processing. We do this in order to limit 
                    // the amount of time spent by the DCA on backlog processing. 
                    // NOTE: Only inactive ETL files are considered to be part of the "backlog". 
                    // The active ETL file is the current ETL file and hence it is not considered
                    // as "backlog". Therefore, we start keeping track of the available time only
                    // after processing the first file, which is the active ETL file. Thus, the
                    // time spent on processing the active ETL file does not count towards the
                    // time spent on processing backlogged ETL files.
                    processingEndTime = DateTime.Now.Add(
                        this.perProviderProcessingTime);
                }
                else
                {
                    if (0 > DateTime.Compare(processingEndTime, DateTime.Now))
                    {
                        this.traceSource.WriteInfo(
                            this.logSourceId,
                            "Due to time limit on backlog processing, no more {0} files will be processed in this pass.",
                            fileNamePattern);
                        fileProcessingInterrupted = true;
                        break;
                    }
                }

                if (cancellationToken.IsCancellationRequested)
                {
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "The producer is being stopped, so no more {0} files will be processed in this pass.",
                        fileNamePattern);
                    fileProcessingInterrupted = true;
                    break;
                }
            }

            // Write the length of the DCA's ETL file backlog. The performance test consumes this information.
            this.perfHelper.RecordEtlPassBacklog(friendlyName, etlFiles.Length - filesProcessed);

            if (fileProcessingInterrupted)
            {
                // If file processing was interrupted, we shouldn't update the
                // end time in this pass because we haven't yet processed all 
                // events up to that time. So set the end time back to whatever
                // it was in the previous pass.
                endTime = lastEndTime;
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
            this.ProcessEtlFilesWorker(etlFileNamePattern, lastEndTime, out endTime, friendlyName, cancellationToken);

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

        private bool IsProcessingWindowsFabricEtlFilesFromDefaultLocation()
        {
            return string.IsNullOrEmpty(this.etlProducerSettings.EtlPath);
        }

        // Parameters passed to ETL producer worker object during initialization
        internal struct EtlProducerWorkerParameters
        {
            internal FabricEvents.ExtensionsEvents TraceSource;
            internal bool IsReadingFromApplicationManifest;
            internal string ApplicationType;
            internal string LogDirectory;
            internal string ProducerInstanceId;
            internal List<EtlProducer> EtlProducers;
            internal EtlProducerSettings LatestSettings;
        }

        // Settings related to ETL file based producers
        internal struct EtlProducerWorkerSettings
        {
            internal TimeSpan EtlReadInterval;
            internal TimeSpan EtlDeletionAge;
            internal WinFabricEtlType WindowsFabricEtlType;
            internal string EtlPath;
            internal string EtlFilePatterns;
            internal List<string> CustomManifestPaths;
            internal bool ProcessingWinFabEtlFiles;
            internal IEnumerable<Guid> AppEtwGuids;
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