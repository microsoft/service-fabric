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
    using System.IO;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Globalization;
    using System.Linq;
    using System.Threading.Tasks;
    using System.Text;
    using Tools.EtlReader;

    // This class implements a wrapper around the ETL producer worker object
    internal class LttProducerWorker
    {
        // file and folder for persisting lastReadTimestampUnixEpochNanoSec
        private const string LastReadTimestampPersistFileName = "LastReadTimestamp.ini";
        private const string LastReadTimestampWorkSubDirectoryKey = "LttProducerWorker";
        private const string LastReadTimestampPersistenceFolder = "LastReadTimestampPersistence";
        private const string ServiceFabricSessionName = "ServiceFabric";
        private const string TableFileExtensionToProcess = "table";
        private const int PathMaxSize = 4096;

        // Tag used to represent the source of the log message
        private string logSourceId;

        // Object used for tracing
        private FabricEvents.ExtensionsEvents traceSource;

        private DcaTimer LttReadTimer;
        internal long LttReadIntervalMinutes { get; private set; }
        private string applicationInstanceId;
        private string lttngTracesDirectory;
        private string dtrTracesDirectory;
        private string activeLttTraceSessionOutputPath;

        private readonly ReadOnlyCollection<IEtlFileSink> sinksIEtlFile;
        private readonly ReadOnlyCollection<ICsvFileSink> sinksICsvFile;

        // timestamp to resume reading traces where it stopped last time
        private ulong lastReadTimestampUnixEpochNanoSec;
        private string lastReadTimestampPersistenceFilePath;

        private readonly LttTraceProcessor lttngTraceProcessor;

        private bool disposed = false;
        private bool stopping = false;
        private object disposeLock = new object();

        internal LttProducerWorker(LttProducerWorkerParameters initParam)
        {
            this.traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);
            this.logSourceId = string.Concat(initParam.LatestSettings.ApplicationInstanceId, "_", initParam.LatestSettings.SectionName);

            this.traceSource.WriteInfo(
                            this.logSourceId,
                            "LttProducer: ApplicationInstanceId:{0} SectionName:{1} LogDirectory:{2} WorkDirectory:{0}",
                            initParam.LatestSettings.ApplicationInstanceId,
                            initParam.LatestSettings.SectionName,
                            initParam.LogDirectory,
                            initParam.LatestSettings.WorkDirectory);

            this.applicationInstanceId = initParam.LatestSettings.ApplicationInstanceId;
            this.lttngTracesDirectory = Path.Combine(initParam.LogDirectory, LttProducerConstants.LttSubDirectoryUnderLogDirectory);
            this.dtrTracesDirectory = Path.Combine(initParam.LogDirectory, LttProducerConstants.DtrSubDirectoryUnderLogDirectory);
            this.LttReadIntervalMinutes = initParam.LttReadIntervalMinutes;

            // splitting sinks into their two different supported types
            var sinks = InitializeSinks(initParam.LttProducers,
                new List<string>() { this.dtrTracesDirectory },
                message => this.traceSource.WriteError(this.logSourceId, message)).AsReadOnly();

            this.sinksIEtlFile = (sinks.Where(s => s is IEtlFileSink)).Select(s => s as IEtlFileSink).ToList().AsReadOnly();
            this.sinksICsvFile = (sinks.Where(s => s is ICsvFileSink)).Select(s => s as ICsvFileSink).ToList().AsReadOnly();

            this.traceSource.WriteInfo(
                            this.logSourceId,
                            "Total number of sinks: {0}, IEtlFileSinks: {1}, ICsvFileSinks: {2}",
                            sinks.Count,
                            this.sinksIEtlFile.Count,
                            this.sinksICsvFile.Count);

            this.lastReadTimestampUnixEpochNanoSec = this.InitializeLastReadTimestamp(initParam.WorkDirectory);

            this.lttngTraceProcessor = new LttTraceProcessor(
                this.traceSource,
                this.logSourceId,
                this.dtrTracesDirectory,
                this.sinksIEtlFile,
                this.sinksICsvFile);

            // getting active service fabric lttng trace session
            this.activeLttTraceSessionOutputPath = this.GetServiceFabricSessionOutputPath();

            // try to process existing traces left from previous session
            this.ProcessPreviousLttSessionTraces(this.activeLttTraceSessionOutputPath);

            string timerId = string.Concat(initParam.LatestSettings.ApplicationInstanceId,
                LttProducerConstants.LttTimerPrefix);
            this.LttReadTimer = new DcaTimer(
                            timerId,
                            this.LttReadCallback,
                            this.LttReadIntervalMinutes * 60 * 1000);
            this.LttReadTimer.Start();
        }

        private string GetServiceFabricSessionOutputPath()
        {
            StringBuilder serviceFabricSessionName = new StringBuilder(LttProducerWorker.ServiceFabricSessionName);
            StringBuilder serviceFabricSessionOutputPath = new StringBuilder(LttProducerWorker.PathMaxSize);

            // If processing traces from container LTTng session the session is not accessible from host.
            // In this case the the active session is the one and only one folder inside the Log/Containers/sf-containerid/lttng-traces folder.
            string containerTraceFolder;
            if (this.TryGetContainerTraceFolder(out containerTraceFolder))
            {
                return containerTraceFolder;
            }

            LttngReaderStatusCode res = LttngReaderBindings.get_active_lttng_session_path(serviceFabricSessionName, serviceFabricSessionOutputPath, LttProducerWorker.PathMaxSize);
            if (res == LttngReaderStatusCode.SUCCESS)
            {
                return serviceFabricSessionOutputPath.ToString().TrimEnd(Path.DirectorySeparatorChar);
            }
            else
            {
                string errorMessage = LttngReaderStatusMessage.GetMessage(res);
                this.traceSource.WriteWarning(
                    this.logSourceId,
                    "Unable to find active Service Fabric Ltt trace session with name starting with: {0}.\nInner error : {1}",
                    LttProducerWorker.ServiceFabricSessionName,
                    errorMessage);
            }

            return null;
        }

        private bool TryGetContainerTraceFolder(out string containerTraceFolder)
        {
            containerTraceFolder = null;

            var applicationInstanceId = ContainerEnvironment.GetContainerApplicationInstanceId(this.logSourceId);
            if (ContainerEnvironment.IsContainerApplication(applicationInstanceId))
            {
                string containerTraceFolderParent = Path.Combine(
                                                ContainerEnvironment.GetContainerLogFolder(applicationInstanceId),
                                                LttProducerConstants.LttSubDirectoryUnderLogDirectory);

                IEnumerable<string> traceFolders = null;
                int numberOfTraceFolders;

                try
                {
                    traceFolders = Directory.EnumerateDirectories(
                        containerTraceFolderParent,
                        $"{LttProducerConstants.LttTraceSessionFolderNamePrefix}*");

                    numberOfTraceFolders = traceFolders.Count();
                }
                catch (OverflowException)
                {
                    this.traceSource.WriteWarning(this.logSourceId, $"Number of container trace folders found is too large.");
                    numberOfTraceFolders = int.MaxValue;
                }
                catch (Exception e)
                {
                    this.traceSource.WriteExceptionAsError(this.logSourceId, e, $"Exception when trying to get container trace folder");
                    return false;
                }

                if (traceFolders == null)
                {
                    return false;
                }

                switch (numberOfTraceFolders)
                {
                    case 0:
                        this.traceSource.WriteWarning(this.logSourceId, "No LTTng traces folder found for container.");
                        containerTraceFolder = null;
                        break;
                    case 1:
                        containerTraceFolder = traceFolders.First();
                        this.traceSource.WriteInfo(this.logSourceId, $"Processing LTTng traces from container at: {containerTraceFolder}");
                        break;
                    default:
                        containerTraceFolder = traceFolders.FirstOrDefault();
                        this.traceSource.WriteWarning(this.logSourceId, $"Found {numberOfTraceFolders} container trace folders. Using {containerTraceFolder}");
                        break;
                }
                return true;
            }

            return false;
        }

        private static bool HasCsvSink(IEnumerable<Object> sinks)
        {
            foreach (var s in sinks)
            {
                if(s is ICsvFileSink || s is IFolderSink)
                {
                    return true;
                }
            }

            return false;
        }

        public static List<Object> InitializeSinks(
            IEnumerable<LttProducer> lttngProducers,
            IEnumerable<string> folderNames,
            Action<string> onInvalidCast)
        {
            var consumerSinks = new List<object>();

            // Get the union of the consumer sink lists from each of the Ltt producers
            // that depend on us
            foreach (var producer in lttngProducers)
            {
                if (null != producer.ConsumerSinks)
                {
                    consumerSinks = consumerSinks.Union(producer.ConsumerSinks).ToList();
                }
            }

            var sinks = new List<Object>();

            // Go through the consumer sinks,
            // Deliver the folder list to IFolderSink
            foreach (var sinkAsObject in consumerSinks)
            {
                ICsvFileSink csvFileSink = sinkAsObject as ICsvFileSink;
                if (csvFileSink != null)
                {
                    sinks.Add(csvFileSink);
                    continue;
                }

                IFolderSink csvFolderSink = sinkAsObject as IFolderSink;
                if (csvFolderSink != null)
                {
                    csvFolderSink.RegisterFolders(folderNames);
                    continue;
                }

                // Using IEtlFileSink since Ltt trace events are being serialized in binary format as in ETW events.
                IEtlFileSink etlFileSink = sinkAsObject as IEtlFileSink;
                if (etlFileSink != null)
                {
                    sinks.Add(etlFileSink);
                    continue;
                }

                onInvalidCast(
                    string.Format(
                        "Could not cast an object of type {0} to interface ICsvFileSink, IFolderSink, IEtlFileSink.",
                        sinkAsObject.GetType()));
            }

            return sinks;
        }

        private void ProcessPreviousLttSessionTraces(string currentTraceSessionPath)
        {
            // Get all existing folders in the the lttng trace folder
            // This sessions should have only unstructured traces
            string[] oldLttSessionFoldersLegacy = System.IO.Directory.GetDirectories(
                this.lttngTracesDirectory,
                "ServiceFabric*",
                System.IO.SearchOption.TopDirectoryOnly);

            // Get all existing folders in the the lttng trace folder
            string[] oldLttSessionFolders = System.IO.Directory.GetDirectories(
                this.lttngTracesDirectory,
                "fabric_traces*",
                System.IO.SearchOption.TopDirectoryOnly);


            // remove currentSession from folders above using
            if (currentTraceSessionPath != null)
            {
                oldLttSessionFoldersLegacy = oldLttSessionFoldersLegacy.Where(s => s.TrimEnd(Path.DirectorySeparatorChar) != currentTraceSessionPath).ToArray();
                oldLttSessionFolders = oldLttSessionFolders.Where(s => s.TrimEnd(Path.DirectorySeparatorChar) != currentTraceSessionPath).ToArray();
            }

            // trying to process leftover traces from previous legacy Service Fabric Lttng sessions
             this.traceSource.WriteInfo(
                this.logSourceId,
                "Processing and deleting the following {0} previous legacy sessions trace folders: {1}",
                oldLttSessionFoldersLegacy.Count(),
                string.Join(",", oldLttSessionFoldersLegacy));

            if (oldLttSessionFoldersLegacy.Count() != 0)
            {
                this.lttngTraceProcessor.LoadDefaultManifests();
                this.ProcessPreviousSessionsHelper(oldLttSessionFoldersLegacy, x => { });
                this.lttngTraceProcessor.UnloadDefaultManifests();
            }

            // trying to process leftover traces from previous Service Fabric Lttng sessions
            this.traceSource.WriteInfo(
                this.logSourceId,
                "Processing and deleting the following {0} previous sessions trace folders: {1}",
                oldLttSessionFolders.Count(),
                string.Join(",", oldLttSessionFolders));

            this.ProcessPreviousSessionsHelper(oldLttSessionFolders, this.lttngTraceProcessor.EnsureCorrectWinFabManifestVersionLoaded);

            // loading the manifests for the current trace session
            if (currentTraceSessionPath != null)
            {
                this.lttngTraceProcessor.EnsureCorrectWinFabManifestVersionLoaded(currentTraceSessionPath);
            }
        }

        private void ProcessPreviousSessionsHelper(IEnumerable<string> lttngFolders, Action<string> ManifestLoaderFunc)
        {
            // TODO - Consider that folders are not ordered by events timestamps
            // process one entire folder at a time and delete it after finished.
            foreach (string folder in lttngFolders)
            {
                // making sure we use the right manifest for decoding folders from older sessions.
                ManifestLoaderFunc(folder);

                // not updating timestamp here since we don't know if we are reading folders in sequence
                // another approach would be to move all the inactive folders to another folder and process from there
                // this way libbabeltrace would take care of the order.
                // TODO cancellation token (if there are many folders and a lot of data)
                this.lttngTraceProcessor.ProcessTraces(folder, true, this.lastReadTimestampUnixEpochNanoSec);

                try
                {
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "Trying to delete previous trace session folder: {0}",
                        folder);

                    Directory.Delete(folder, true);
                }
                catch(Exception e)
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Error trying to delete old trace folder : {0} with exception {1}",
                        folder,
                        e);
                }
            }
        }

        private ulong ReadLastReadTimestampFromFile(string filePath)
        {
            ulong lastReadTimestamp = 0;

            if (File.Exists(filePath))
            {
                try
                {
                    using (StreamReader file = File.OpenText(filePath))
                    {
                        lastReadTimestamp = ulong.Parse(file.ReadLine());
                    }
                }
                catch (Exception e)
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Error loading persisted trace lastReadTimestamp from file: {0} - Trace processing will start from first event - Exception {1}",
                        filePath,
                        e);
                }
            }

            return lastReadTimestamp;
        }

        private void LttReadCallback(object state)
        {
            lock(this.disposeLock)
            {
                if (this.stopping)
                {
                    return;
                }

                this.traceSource.WriteInfo(
                                this.logSourceId,
                                "Starting the Ltt handler, ApplicationInstance: {0}",
                                applicationInstanceId);

                this.OnLttProcessingPassBegin();

                // try to get active tracing session if previously none was found.
                if (this.activeLttTraceSessionOutputPath == null)
                {
                    this.activeLttTraceSessionOutputPath = this.GetServiceFabricSessionOutputPath();

                    // trying to load the correct manifest file if session found.
                    if (this.activeLttTraceSessionOutputPath != null)
                    {
                        this.lttngTraceProcessor.EnsureCorrectWinFabManifestVersionLoaded(this.activeLttTraceSessionOutputPath);
                    }
                }

                // lttng_handler will be only executed by starthost at the beginning to start the tracing session.
                // Now the same session is used and we use Ltt tracefile-size trace-count to rotate the trace files.
                // Ltt-enable channel switch-timer is used to flush the traces and the metadata so it can be read here.
                if (this.activeLttTraceSessionOutputPath != null)
                {
                    this.lastReadTimestampUnixEpochNanoSec = this.lttngTraceProcessor.ProcessTraces(this.activeLttTraceSessionOutputPath, this.lastReadTimestampUnixEpochNanoSec != 0, this.lastReadTimestampUnixEpochNanoSec);
                }

                this.OnLttProcessingPassEnd();

                // Persist lastReadTimestamp in case process restarts.
                this.PersistToFileLastReadTimestamp(this.lastReadTimestampUnixEpochNanoSec);

                if (this.sinksICsvFile.Any())
                {
                    // Read the table file and send the events to consumer
                    var task = Task.Run(() => this.ProcessTableEvents());
                }

                this.LttReadTimer.Start(this.LttReadIntervalMinutes * 60 * 1000);
            }
        }

        private void ProcessTableEvents()
        {
            string searchPattern = string.Format(
                                       CultureInfo.InvariantCulture,
                                       "*.{0}",
                                       TableFileExtensionToProcess);
            DirectoryInfo dirInfo = new DirectoryInfo(this.dtrTracesDirectory);
            List<FileInfo> tableTraceFiles = dirInfo.GetFiles(searchPattern, SearchOption.AllDirectories)
                                   .ToList();
            this.traceSource.WriteInfo(
                this.logSourceId,
                "Found  {0} {1} files to process as table files.",
                tableTraceFiles.Count,
                TableFileExtensionToProcess);

            foreach (FileInfo file in tableTraceFiles)
            {
                try
                {
                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "Processing table file {0} with size {1} bytes.",
                        file.FullName,
                        file.Length);

                    CsvFileToSinkProcessor processor = new CsvFileToSinkProcessor(file, this.sinksICsvFile, this.logSourceId, this.traceSource);
                    processor.ProcessFile();
                    File.Delete(file.FullName);
                }
                catch (Exception e)
                {
                    this.traceSource.WriteExceptionAsWarning(
                        this.logSourceId,
                        e,
                        "Failed to process file for query events {0}",
                        file);
                }
            }

            return;
        }

        // Returns the last read timestamp as persisted on disk
        // if not found creates the needed folders and files for persistence.
        private ulong InitializeLastReadTimestamp(string workFolder)
        {
            // get or creates path to where last read timestamp will be or was persisted
            if (!Utility.CreateWorkSubDirectory(
                this.traceSource,
                this.logSourceId,
                LttProducerWorker.LastReadTimestampWorkSubDirectoryKey,
                LttProducerWorker.LastReadTimestampPersistenceFolder,
                workFolder,
                out this.lastReadTimestampPersistenceFilePath))
            {
                // failed to create folder this.lastReadTimestampPersistenceFilePath will be null
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Unable to create or access WorkSubDirectory for persisting trace processing persistence with: DCAWorkFolder: {0}, PersistenceFolder: {1}, WorkSubDirectoryKey: {2}. Trace processing will start from beginning on process restart.",
                    workFolder,
                    LttProducerWorker.LastReadTimestampPersistenceFolder,
                    LttProducerWorker.LastReadTimestampWorkSubDirectoryKey);

                return 0;
            }

            this.lastReadTimestampPersistenceFilePath = Path.Combine(
                this.lastReadTimestampPersistenceFilePath,
                LttProducerWorker.LastReadTimestampPersistFileName);

            // trying to read from file if file does not exist or it fails to read from it returns 0 (unix epoch)
            return this.ReadLastReadTimestampFromFile(this.lastReadTimestampPersistenceFilePath);
        }

        // this method first saves the data in a tmp file
        // and then replaces the existing one (if it exists) to reduce the chances of corrupting the file.
        public void PersistToFileLastReadTimestamp(ulong unixEpochTimestamp)
        {
            if (this.lastReadTimestampPersistenceFilePath != null)
            {
                try
                {
                    string filePathTemp = string.Format("{0}.tmp", this.lastReadTimestampPersistenceFilePath);

                    // saving current collection in a json serialization format on temporary file
                    using (StreamWriter file = File.CreateText(filePathTemp))
                    {
                        // writing timestamp to file
                        file.WriteLine(unixEpochTimestamp);
                    }

                    // create or replace file
                    File.Copy(filePathTemp, this.lastReadTimestampPersistenceFilePath, true);

                    // remove temporary file
                    File.Delete(filePathTemp);
                }
                catch (Exception e)
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Error when trying to save lastReadTimestamp to file: {0} - Exception: {1}",
                        this.lastReadTimestampPersistenceFilePath,
                        e);
                }
            }
            else
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Error when trying to save lastReadTimestamp to file. File path was not set during initialization.");
            }
        }

        private void OnLttProcessingPassBegin()
        {
            foreach (IEtlFileSink sink in this.sinksIEtlFile)
            {
                sink.OnEtwEventProcessingPeriodStart();
            }
        }

        private void OnLttProcessingPassEnd()
        {
            foreach (IEtlFileSink sink in this.sinksIEtlFile)
            {
                sink.OnEtwEventProcessingPeriodStop();
            }
        }
        
        public void Dispose()
        {
            lock (this.disposeLock)
            {
                this.stopping = true;

                if (this.disposed)
                {
                    return;
                }

                this.LttReadTimer.StopAndDispose();
                this.disposed = true;

                GC.SuppressFinalize(this);
            }
        }

        internal struct LttProducerWorkerParameters
        {
            internal FabricEvents.ExtensionsEvents TraceSource;
            internal string LogDirectory;
            internal string WorkDirectory;
            internal string ProducerInstanceId;
            internal List<LttProducer> LttProducers;
            internal ProducerInitializationParameters LatestSettings;
            internal long LttReadIntervalMinutes;
        }
    }
}
