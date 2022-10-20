// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca.LttConsumerHelper
{
    using System;
    using System.Globalization;
    using System.IO;
    using System.Diagnostics;
    using System.Collections.Generic;
    using System.Linq;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Dca;
    using System.Text.RegularExpressions;
    using System.Fabric.Dca.EtlConsumerHelper;
    using Reflection;
    /// <summary>
    /// This class implements the logic to move CSV traces to Azure upload folder on disk.
    /// </summary>
    class CsvToUploadFolderWriter
    {
        // Constants
        private const string moveFilesTimerIdSuffix = "_MoveCsvFileTimer";
        internal const string BootstrapTracesFolderName = "Bootstrap";
        internal const string FabricTracesFolderName = "Fabric";
        internal const string LeaseTracesFolderName = "Lease";
        private const string FileExtensionToMove = "dtr";
        private const string ClusterVersionFile = "ClusterVersion";

        #region Private Fields

        /// <summary>
        /// Whether or not we are in the process of stopping
        /// </summary>
        private bool stopping;

        /// <summary>
        /// Directory containing CSV files
        /// </summary>
        private string csvFolder;

        /// <summary>
        /// Version of the current code
        /// </summary>
        private string fabricVersion;

        /// <summary>
        /// Time when the first log file for written
        /// </summary>
        private long logStartTime;

        /// <summary>
        /// Directory containing trace files
        /// </summary>
        private string sourceFolder;

        /// <summary>
        // Object used for tracing
        /// </summary>
        private FabricEvents.ExtensionsEvents traceSource;

        /// <summary>
        // Tag used to represent the source of the log message
        /// </summary>
        protected string logSourceId;

        // Timer to delete old logs
        private DcaTimer csvMoveFilesTimer;

        // Fabric node ID
        private string fabricNodeId;

        // Whether or not Windows Fabric traces should be organized into sub-folders 
        // based on type, i.e. bootstrap, fabric and lease
        private bool organizeWindowsFabricTracesByType;

        // Whether or not the consumer using this object has disabled DTR compression
        private bool dtrCompressionDisabledByConsumer;

        // Whether or not the CSV files should be compressed
        private bool compressCsvFiles;

        // Whether or not the object has been disposed
        private bool disposed;

        private static string dtrPattern = String.Concat(".", "dtr", "[0-9]*$");

        // Any dtr file name matches the regular expression below.
        private static Regex dtrRegEx = new Regex(dtrPattern);

        // The dtr file name for the last chunk of an ETL file matches the regular expression below.
        private static Regex lastEtlChunkDtrRegEx = new Regex(
                                                            String.Concat(
                                                                "_",
                                                                Int32.MaxValue.ToString("D10"),
                                                                dtrPattern));

        // The compressed dtr file name for the last chunk of an ETL file matches the regular expression below.
        private static Regex lastEtlChunkCompressedDtrRegEx = new Regex(
                                                                        String.Concat(
                                                                            "_",
                                                                            Int32.MaxValue.ToString("D10"),
                                                                            ".",
                                                                            "dtr",
                                                                            "[0-9]*",
                                                                            ".zip",
                                                                            "$"));

        // Disk space manager instance.
        private readonly DiskSpaceManager diskSpaceManager;

        /// <summary>
        // Config Reader
        /// </summary>
        private readonly IEtlToCsvFileWriterConfigReader configReader;

        #endregion

        private object disposeLock = new object();

        // Whether or not Windows Fabric traces should be organized into separate sub-folders
        internal bool OrganizeWindowsFabricTracesByType 
        {
            set
            {
                this.organizeWindowsFabricTracesByType = value;
            }
        }

        #region Public Constructors

        internal CsvToUploadFolderWriter(
            string logSourceId,
            string fabricNodeId,
            string csvFolder,
            string sourceFolder,
            DiskSpaceManager diskSpaceManager,
            bool dtrCompressionDisabled)
            : this(
            logSourceId,
            fabricNodeId,
            csvFolder,
            sourceFolder,
            diskSpaceManager,
            dtrCompressionDisabled,
            new EtlToCsvFileWriterConfigReader())
        {
        }

        internal CsvToUploadFolderWriter(
            string logSourceId,
            string fabricNodeId,
            string csvFolder,
            string sourceFolder,
            DiskSpaceManager diskSpaceManager,
            bool dtrCompressionDisabled,
            IEtlToCsvFileWriterConfigReader configReader)
        {
            this.traceSource = new FabricEvents.ExtensionsEvents(FabricEvents.Tasks.FabricDCA);
            this.logSourceId = logSourceId;
            this.organizeWindowsFabricTracesByType = true;
            this.fabricNodeId = fabricNodeId;
            this.dtrCompressionDisabledByConsumer = dtrCompressionDisabled;
            this.configReader = configReader;

            this.disposed = false;
            this.stopping = false;

            this.compressCsvFiles = FileCompressor.CompressionEnabled &&
                                    (false == this.dtrCompressionDisabledByConsumer) &&
                                    (false == this.configReader.IsDtrCompressionDisabledGlobally());

            this.diskSpaceManager = diskSpaceManager;

            try
            {
                string currentAssemblyLocation = typeof(CsvToUploadFolderWriter).GetTypeInfo().Assembly.Location;
                string versionFile = Path.Combine(Path.GetDirectoryName(currentAssemblyLocation), ClusterVersionFile);
                this.fabricVersion = File.ReadAllText(versionFile);
            }
            catch (Exception e)
            {
                this.traceSource.WriteExceptionAsError(
                    this.logSourceId,
                    e,
                    "Could not find the version of current service fabric code");
                throw;
            }

            // Create the directory that containing filtered traces, in case it
            // doesn't already exist
            this.csvFolder = csvFolder;
            FabricDirectory.CreateDirectory(this.csvFolder);

            CreateWindowsFabricTraceSubFolders();

            this.sourceFolder = sourceFolder;

            this.traceSource.WriteInfo(
                this.logSourceId,
                "Directory containing trace files: {0} Directory containing dtr traces: {1}",
                this.sourceFolder,
                this.csvFolder);

            diskSpaceManager.RegisterFolder(
                this.logSourceId, 
                () => new DirectoryInfo(this.csvFolder).EnumerateFiles("*.dtr*", SearchOption.AllDirectories),
                null,
                f => f.LastWriteTimeUtc > (DateTime.UtcNow - configReader.GetDtrDeletionAge()));

            string timerIdMove = string.Concat(
                                 this.logSourceId,
                                 moveFilesTimerIdSuffix);
            this.csvMoveFilesTimer = new DcaTimer(
                                               timerIdMove,
                                               this.CsvMoveFilesHandler,
                                               1 * 60 * 1000);

            this.csvMoveFilesTimer.Start();
        }
        #endregion

        #region Private Methods

        private void CreateWindowsFabricTraceSubFolders()
        {
            string subFolder = Path.Combine(this.csvFolder, FabricTracesFolderName);
            FabricDirectory.CreateDirectory(subFolder);
        }

        // On Linux, this field is of no use.
        private long GetFirstEventTicks(string fileName)
        {
            return 100000000000000000L;
        }

        private long GetLastEventTicks(string fileName)
        {
            const long fileChunkSizeToRead = 2048;

            long index = 0;
            int lastTraceBeginIndex = 0;
            int lastTraceLength = 0;
            byte[] buffer = new byte[fileChunkSizeToRead];
            bool lastTraceFound = false;
            bool lastTraceBeginFound = false;
            string lastTrace = "";

            using (var traces = new FileStream(fileName, FileMode.Open))
            {
                traces.Seek(-1, SeekOrigin.End);

                while (index < traces.Length)
                {
                    var readBytes = Math.Min(fileChunkSizeToRead, traces.Length - (index));
                    index += readBytes;
                    traces.Seek(-index, SeekOrigin.End);

                    // readBytes will never be more than fileChunkSizeToRead,
                    // so it is safe to typecast
                    traces.Read(buffer, 0, (int)readBytes);

                    for (int j = (int)readBytes - 1; j >= 0; j--)
                    {
                        lastTraceBeginIndex++;

                        if (lastTraceFound)
                        {
                            if (buffer[j] == '\n')
                            {
                                // Found the beginning of the last trace
                                // Remove this '\n'
                                lastTraceBeginIndex--;
                                // Move the file reader to beginning of trace
                                traces.Seek(-lastTraceBeginIndex, SeekOrigin.End);
                                lastTraceBeginFound = true;
                                break;
                            }
                            lastTraceLength++;
                            continue;
                        }

                        if (buffer[j] == '\n')
                        {
                            // Found the ending of the last trace
                            lastTraceFound = true;
                        }
                    }

                    if (lastTraceBeginFound)
                    {
                        break;
                    }
                }

                byte[] lastTraceBytes = new byte[lastTraceLength];
                traces.Read(lastTraceBytes, 0, lastTraceLength);

                // The traces are in ASCII encoding
                lastTrace = System.Text.Encoding.ASCII.GetString(lastTraceBytes);
            }

            var timeStampString = lastTrace.Split(new char[] {','}, 2) [0];
            DateTime timeStamp = DateTime.Parse(timeStampString, CultureInfo.InvariantCulture, DateTimeStyles.AssumeLocal);
            
            return timeStamp.Ticks;
        }

        private string GetTraceFileSubFolder(string fileName)
        {
            return FabricTracesFolderName;
        }

        private void CopyTraceFileForUpload(string fileName)
        {
            // Build the destination name for the filtered trace file
            //
            //         !!! WARNING !!! 
            // The trace viewer tool parses the file names of the filtered trace files.
            // Changing the file name format might require a change to trace viewer as well.
            //         !!! WARNING !!!
            //
            // If the ETL file is an active ETL file, the trace file name is of the form:
            // <FabricNodeID>_<etlFileName>_<TimestampOfLastEventProcessed>_<TimestampDifferentiatorOfLastEventProcessed>.dtr
            //
            // If the ETL file is an inactive ETL file, the trace file name is of the form:
            // <FabricNodeID>_<etlFileName>_<TimestampOfLastEventProcessed>_<Int32.MaxValue>.dtr
            //
            // Using Int32.MaxValue as a component of the trace file name makes 
            // it easy to identify gaps in the filtered traces if the DCA is 
            // falling behind on trace processing. Recall that an inactive ETL 
            // file is always processed fully. Only active ETL files are processed
            // in chunks. Therefore, the presence of Int32.MaxValue indicates that
            // the corresponding ETL file is inactive and has been fully processed
            // by the DCA. Thus, gaps **within** an ETL file (i.e. unprocessed 
            // chunks within the file) can be identified by the absence of a file
            // containing Int32.MaxValue in its name.
            //
            // It is also worth noting that ETL file names are sequentially
            // numbered, which helps in identifying gaps **between** ETL files 
            // (i.e. ETL files that were not processed at all). And the use of 
            // Int32.MaxValue is an enhancement that enables us to identify gaps
            // within an ETL file. Using these two concepts, we can look at a set
            // of filtered trace files and determine whether they are complete.
            // And if not complete, we can also identify where all the gaps are.
            string differentiator = string.Format(
                                        CultureInfo.InvariantCulture,
                                        "{0:D10}",
                                        0);

            string newTraceFileName = "";
            long lastEventTicks;
            try
            {
                if (logStartTime == 0)
                {
                    // We just need the start of log tracing file generation time stamp,
                    // for alligning with etw logs
                    //this.logStartTime = DateTime.Parse(Path.GetFileNameWithoutExtension(fileName).Replace("_", " ")).Ticks;
                    this.logStartTime = GetFirstEventTicks(fileName);
                }

                lastEventTicks = GetLastEventTicks(fileName);
            }
            catch (Exception e)
            {
                var fileNameDiscard = fileName + ".discard";

                this.traceSource.WriteExceptionAsWarning(
                    this.logSourceId,
                    e,
                    "Could not create filename for trace files for upload. Renaming file to {0}",
                    fileNameDiscard);

                // Rename the file and do not process it.
                try
                {
                    // Delete the file so that storage is not blocked
                    FabricFile.Delete(fileName);
                }
                catch (Exception ex)
                {
                    this.traceSource.WriteExceptionAsWarning(
                        this.logSourceId,
                        ex,
                        "Failed to rename file to {0}",
                        fileNameDiscard);
                }

                return;
            }

            // Create the filename which TraceViewer understands
            newTraceFileName = string.Format(
                CultureInfo.InvariantCulture,
                "fabric_traces_{0}_{1}_{2:D6}",
                fabricVersion,
                this.logStartTime,
                1);

            string traceFileNamePrefix = string.Format(
                                                  CultureInfo.InvariantCulture,
                                                  "{0}_{1}_{2:D20}_{3}.",
                                                  this.fabricNodeId,
                                                  newTraceFileName,
                                                  lastEventTicks,
                                                  differentiator);

            var applicationInstanceId = ContainerEnvironment.GetContainerApplicationInstanceId(this.logSourceId);
            if (ContainerEnvironment.IsContainerApplication(applicationInstanceId))
            {
                // Note that the a hash of the applicationInstanceId is being used to reduce file name length in around 70 characters
                // This is done to workaround PathTooLong exception in FileUploaderBase.cs since we don't have an interop for FileSystemWatcher
                // and .NET 4.5 used does not support long paths yet.
                traceFileNamePrefix = string.Format(
                                                CultureInfo.InvariantCulture,
                                                "{0}_{1:X8}_{2}_{3:D20}_{4}.",
                                                this.fabricNodeId,
                                                Path.GetFileName(applicationInstanceId).GetHashCode(),
                                                newTraceFileName,
                                                lastEventTicks,
                                                differentiator);
            }

            string traceFileNameWithoutPath = string.Concat(
                                                  traceFileNamePrefix,
                                                  "dtr");

            string compressedTraceFileNameWithoutPath = string.Concat(
                                                            traceFileNamePrefix,
                                                            "dtr.zip");

            string subFolder = GetTraceFileSubFolder(fileName);

            string traceFileDestinationPath = Path.Combine(
                                                  this.csvFolder,
                                                  subFolder);

            string traceFileDestinationName = Path.Combine(
                                                traceFileDestinationPath,
                                                this.compressCsvFiles ?
                                                  compressedTraceFileNameWithoutPath : 
                                                  traceFileNameWithoutPath);

            string alternateTraceFileDestinationName = Path.Combine(
                                                          traceFileDestinationPath,
                                                          this.compressCsvFiles ?
                                                            traceFileNameWithoutPath :
                                                            compressedTraceFileNameWithoutPath);

            try
            {
                InternalFileSink.CopyFile(fileName, traceFileDestinationName, false, this.compressCsvFiles);
                FabricFile.Delete(fileName);
                this.traceSource.WriteInfo(
                            this.logSourceId,
                            "Traces are ready. They have been moved from {0} to {1}.",
                            fileName,
                            traceFileDestinationName);
            }
            catch (Exception e)
            {
                this.traceSource.WriteExceptionAsError(
                    this.logSourceId,
                    e,
                    "Failed to move file from {0} to {1}.",
                    fileName,
                    traceFileDestinationName);
            }
        }

        internal void SetSourceLogFolders(string folderName)
        {
            this.sourceFolder = folderName;

            this.traceSource.WriteInfo(
                this.logSourceId,
                "Update: Directory containing trace files: {0} Directory containing dtr traces: {1}",
                this.sourceFolder,
                this.csvFolder);
        }

        private void CsvMoveFilesHandler(object state)
        {
            lock (this.disposeLock)
            {
                if (this.stopping)
                {
                    return;
                }

                string sourceFolderLocal = this.sourceFolder;

                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Scanning directory {0} for trace files.",
                    sourceFolderLocal);
                try
                {
                    // Get the trace files that are old enough to be deleted
                    string searchPattern = string.Format(
                                            CultureInfo.InvariantCulture,
                                            "*.{0}",
                                            FileExtensionToMove);
                    DirectoryInfo dirInfo = new DirectoryInfo(sourceFolderLocal);
                    List<FileInfo> lttTraceFiles = dirInfo.GetFiles(searchPattern, SearchOption.AllDirectories)
                                        .ToList();

                    foreach (FileInfo file in lttTraceFiles)
                    {
                        this.traceSource.WriteInfo(
                            this.logSourceId,
                            "Moving file {0} for upload.",
                            file.Name);

                        // Copy the trace file to the CSV directory
                        CopyTraceFileForUpload(file.FullName);
                    }
                } catch (DirectoryNotFoundException)
                {
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Unable to find trace source folder at: {0}.",
                        sourceFolderLocal
                    );
                }
                finally
                {
                    // Schedule the next pass
                    this.csvMoveFilesTimer.Start();
                }
            }
        }
        #endregion
        
        public void Dispose()
        {
            lock (this.disposeLock)
            {
                if (this.disposed)
                {
                    return;
                }
                this.disposed = true;

                this.stopping = true;

                this.diskSpaceManager.UnregisterFolders(this.logSourceId);

                this.csvMoveFilesTimer.StopAndDispose();
                this.csvMoveFilesTimer.DisposedEvent.WaitOne();
            }

            GC.SuppressFinalize(this);
            return;
        }
    }
}