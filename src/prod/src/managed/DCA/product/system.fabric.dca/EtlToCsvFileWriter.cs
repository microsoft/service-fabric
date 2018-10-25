// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca.EtlConsumerHelper
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Common;
    using System.Fabric.Dca;
    using System.Globalization;
    using System.IO;
    using System.Text.RegularExpressions;
    using Tools.EtlReader;
    
    /// <summary>
    /// This class implements the logic to write filtered ETW traces to CSV files on disk. 
    /// 
    /// Note that it derives from the event processor class.
    /// </summary>
    internal class EtlToCsvFileWriter : EventProcessor, IEtlFileSink, ITraceOutputWriter
    {
        // Constants
        internal const string BootstrapTracesFolderName = "Bootstrap";
        internal const string FabricTracesFolderName = "Fabric";
        internal const string LeaseTracesFolderName = "Lease";

        #region Private Fields

        private static readonly string DtrPattern = string.Concat(".", EtlConsumerConstants.FilteredEtwTraceFileExtension, "[0-9]*$");

        // Any dtr file name matches the regular expression below.
        private static readonly Regex DtrRegEx = new Regex(DtrPattern);

        // The dtr file name for the last chunk of an ETL file matches the regular expression below.
        private static readonly Regex LastEtlChunkDtrRegEx = new Regex(
                                                            string.Concat(
                                                                "_",
                                                                int.MaxValue.ToString("D10"),
                                                                DtrPattern));

        // The compressed dtr file name for the last chunk of an ETL file matches the regular expression below.
        private static readonly Regex LastEtlChunkCompressedDtrRegEx = new Regex(
                                                                        string.Concat(
                                                                            "_",
                                                                            int.MaxValue.ToString("D10"),
                                                                            ".",
                                                                            EtlConsumerConstants.FilteredEtwTraceFileExtension,
                                                                            "[0-9]*",
                                                                            EtlConsumerConstants.CompressedFilteredEtwTraceFileExtension,
                                                                            "$"));

        /// <summary>
        /// Directory containing filtered ETW CSV files
        /// </summary>
        private readonly string filteredTraceDirName;
#if !DotNetCoreClr
        // Helper object to log performance-related information
        private readonly EtlToCsvPerformance perfHelper;
#endif
        // Fabric node ID
        private readonly string fabricNodeId;

        // Whether or not the consumer using this object has disabled DTR compression
        private readonly bool dtrCompressionDisabledByConsumer;

        private readonly IEtlToCsvFileWriterConfigReader configReader;
        private readonly DiskSpaceManager diskSpaceManager;

        /// <summary>
        /// This object handles the file related function so as abstract away the file specific functionality
        /// in order to allow in memory traces.
        /// </summary>
        private InternalFileSink fileSink;

        /// <summary>
        /// The index up to which we have already processed events within the 
        /// current source ETL file. These events would have been processed in
        /// previous ETL read passes and we can ignore these events.
        /// </summary>
        private EventIndex maxIndexAlreadyProcessed;

        /// <summary>
        /// The index of the last event we processed within the current source 
        /// ETL file.
        /// </summary>
        private EventIndex lastEventIndex;

        // Whether or not Windows Fabric traces should be organized into sub-folders 
        // based on type, i.e. bootstrap, fabric and lease
        private bool organizeWindowsFabricTracesByType;

        // Whether or not the CSV files should be compressed
        private bool compressCsvFiles;

        // Whether or not the object has been disposed
        private bool disposed;

        private string lastDeletedDtrName;
        private DateTime lastDeletedDtrEventTime;

        #endregion

        #region Public Constructors

        internal EtlToCsvFileWriter(
            ITraceEventSourceFactory traceEventSourceFactory,
            string logSourceId,
            string fabricNodeId,
            string etwCsvFolder,
            bool dtrCompressionDisabled,
            DiskSpaceManager diskSpaceManager)
            : this(
            traceEventSourceFactory,
            logSourceId,
            fabricNodeId,
            etwCsvFolder,
            dtrCompressionDisabled,
            diskSpaceManager,
            new EtlToCsvFileWriterConfigReader())
        {
        }

        internal EtlToCsvFileWriter(
            ITraceEventSourceFactory traceEventSourceFactory,
            string logSourceId,
            string fabricNodeId,
            string etwCsvFolder,
            bool dtrCompressionDisabled,
            DiskSpaceManager diskSpaceManager,
            IEtlToCsvFileWriterConfigReader configReader)
            : base(traceEventSourceFactory, logSourceId)
        {
            this.organizeWindowsFabricTracesByType = true;
            this.fabricNodeId = fabricNodeId;
            this.dtrCompressionDisabledByConsumer = dtrCompressionDisabled;
            this.diskSpaceManager = diskSpaceManager;
            this.configReader = configReader;
#if !DotNetCoreClr
            this.perfHelper = new EtlToCsvPerformance(this.TraceSource, this.LogSourceId);
#endif
            // Create the directory that containing filtered traces, in case it 
            // doesn't already exist
            this.filteredTraceDirName = etwCsvFolder;
            FabricDirectory.CreateDirectory(this.filteredTraceDirName);

            this.TraceSource.WriteInfo(
                this.LogSourceId,
                "Directory containing filtered ETW traces: {0}",
                this.filteredTraceDirName);

            // Create a timer to delete old logs
            // Figure out the retention time for the CSV files
            // Time after which the CSV file on disk becomes a candidate for deletion
            // Read this value from config every time. Do not cache it. That's how
            // we pick up the latest value when an update happens.
            //
            // From the above files, get the ones whose corresponding ETL files
            // have already been fully processed. We should delete only the files 
            // whose corresponding ETL files have been fully processed. All other
            // files should be kept around because their file name gives us the 
            // bookmark up to which we have processed events.
            var deletionAge = configReader.GetDtrDeletionAge();
            diskSpaceManager.RegisterFolder(
                logSourceId, 
                () =>
                    {
                        // Get the filtered ETW trace files that are old enough to be deleted
                        var dirInfo = new DirectoryInfo(this.filteredTraceDirName);
                        return dirInfo.EnumerateFiles(EtlConsumerConstants.FilteredEtwTraceSearchPattern, SearchOption.AllDirectories);
                    },
                f => f.LastWriteTimeUtc < DateTime.UtcNow - deletionAge, // we don't have any indication whether work is done currently, use timer to estimate
                f => f.LastWriteTimeUtc >= DateTime.UtcNow - deletionAge,
                f =>
                {
                    DateTime lastEventTimeStamp;
                    GetLastEventTimestamp(f.Name, out lastEventTimeStamp);
                    this.lastDeletedDtrName = f.Name;
                    this.lastDeletedDtrEventTime = lastEventTimeStamp;
                    try
                    {
                        FabricFile.Delete(f.FullName);
                        return true;
                    }
                    catch (Exception)
                    {
                        return false;
                    }
                });
            diskSpaceManager.RetentionPassCompleted += () =>
            {
                this.TraceSource.WriteInfo(
                    this.LogSourceId,
                    "The last dtr file deleted during disk space manager pass {0} with events up til {1}.",
                    this.lastDeletedDtrName, 
                    this.lastDeletedDtrEventTime);
            };

            diskSpaceManager.RegisterFolder(
                logSourceId, 
                () => new DirectoryInfo(this.filteredTraceDirName)
                    .EnumerateFiles(EtlConsumerConstants.BootstrapTraceSearchPattern, SearchOption.AllDirectories), 
                f => true, 
                f => f.LastWriteTimeUtc >= DateTime.UtcNow - deletionAge);
        }
        #endregion

        public bool NeedsDecodedEvent
        {
            get
            {
                return true;
            }
        }

        // Whether or not Windows Fabric traces should be organized into separate sub-folders
        internal bool OrganizeWindowsFabricTracesByType
        {
            set
            {
                this.organizeWindowsFabricTracesByType = value;
            }
        }

        public void OnEtwEventProcessingPeriodStart()
        {
#if !DotNetCoreClr            
            this.perfHelper.EtlReadPassBegin();
#endif
            // Retrieve settings again, in case they have changed since the last 
            // time we checked.
            this.compressCsvFiles = FileCompressor.CompressionEnabled &&
                                    (false == this.dtrCompressionDisabledByConsumer) && 
                                    (false == this.configReader.IsDtrCompressionDisabledGlobally());
        }

        public void OnEtwEventProcessingPeriodStop()
        {
#if !DotNetCoreClr            
            this.perfHelper.EtlReadPassEnd();
#endif            
        }

        public void SetEtwEventFilter(string filterString, string defaultFilterString, string summaryFilterString, bool isWindowsFabricEventFilter)
        {
            // Create the filter for Windows Fabric events
            lock (this.FilterLock)
            {
                this.Filter = new EtwTraceFilter(
                                        this.TraceSource,
                                        this.LogSourceId,
                                        filterString,
                                        defaultFilterString,
                                        summaryFilterString,
                                        isWindowsFabricEventFilter);
            }
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            this.disposed = true;

            this.Stopping = true;

            // Stop and dispose timers
            this.diskSpaceManager.UnregisterFolders(this.LogSourceId);

            GC.SuppressFinalize(this);
        }
        
        public void SetEtwManifestCache(ManifestCache manifestCache)
        {
            // We ignore the manifest cache because we don't decode the events
            // ourselves. Instead, we rely on the producer to decode the events
            // for us.
        }
        
        public void OnEtwEventsAvailable(IEnumerable<EventRecord> etwEvents)
        {
            // We are not interested in processing raw ETW events
        } 

        public void OnEtwEventsAvailable(IEnumerable<DecodedEtwEvent> etwEvents)
        {
            foreach (DecodedEtwEvent producerEvent in etwEvents)
            {
                this.QueueEtwTrace(new DecodedEventWrapper(producerEvent));
            }
        }

        public void OnBootstrapTraceFileScanStart()
        {
        }

        public void OnBootstrapTraceFileScanStop()
        {
        }

        public bool OnBootstrapTraceFileAvailable(string fileName)
        {
            // Copy the bootstrap trace file to the ETW CSV directory
            //
            // Add the fabric node ID as a prefix, so that we can later associate
            // the traces with the Fabric node ID.
            string bootstrapTraceFileNameWithoutPath = string.Format(
                                                           CultureInfo.InvariantCulture,
                                                           "{0}_{1}",
                                                           this.fabricNodeId,
                                                           Path.GetFileName(fileName));
            string bootstrapTraceDestinationFileName = Path.Combine(
                                                           this.filteredTraceDirName,
                                                           this.organizeWindowsFabricTracesByType ? BootstrapTracesFolderName : string.Empty,
                                                           bootstrapTraceFileNameWithoutPath);

            try
            {
                InternalFileSink.CopyFile(fileName, bootstrapTraceDestinationFileName, true, false);

                // logging bytes read and written
                var fileInfo = new FileInfo(fileName);
#if !DotNetCoreClr                
                this.perfHelper.BytesRead(fileInfo.Length);
                this.perfHelper.BytesWritten(fileInfo.Length);
#endif                

                this.TraceSource.WriteNoise(
                    this.LogSourceId,
                    "Copied bootstrap trace file {0} to the directory {1}.",
                    fileName,
                    this.filteredTraceDirName);
                return true;
            }
            catch (Exception e)
            {
                // Failed to copy bootstrap trace file. Just skip this file and continue.
                this.TraceSource.WriteExceptionAsError(
                    this.LogSourceId,
                    e,
                    "Failed to copy file. Source: {0}, destination: {1}.",
                    fileName,
                    bootstrapTraceDestinationFileName);
                return false;
            }
        }

        internal static bool GetLastEventTimestamp(string fileName, out DateTime lastEventTimestamp)
        {
            lastEventTimestamp = DateTime.MinValue;

            string[] fileNameParts = fileName.Split('_');
            if (fileNameParts.Length < 4)
            {
                return false;
            }

            // The second-last part is the timestamp ticks
            long ticks;
            if (false == long.TryParse(fileNameParts[fileNameParts.Length - 2], out ticks))
            {
                return false;
            }

            lastEventTimestamp = new DateTime(ticks);
            return true;
        }

        internal static bool GapsFoundInDtrs(FileInfo[] dtrFiles, int startIndex, int endIndex, out List<string> unprocessedEtls)
        {
            unprocessedEtls = new List<string>();

            if (startIndex > endIndex)
            {
                return false;
            }

            Dictionary<string, bool> sourceEtlFullyProcessed = new Dictionary<string, bool>();
            string mostRecentEtl = string.Empty;
            for (int i = startIndex; i <= endIndex; i++)
            {
                // Get the source ETL file
                string etlFile = GetEtlFileNameFromTraceFileName(dtrFiles[i].Name);

                // Update the most recent ETL file
                mostRecentEtl = etlFile;

                // Figure out if the ETL-to-CSV writer has fully processed the source ETL file
                bool fullyProcessed = LastEtlChunkDtrRegEx.IsMatch(dtrFiles[i].Name) || LastEtlChunkCompressedDtrRegEx.IsMatch(dtrFiles[i].Name);
                sourceEtlFullyProcessed[etlFile] = fullyProcessed;
            }

            // It is expected that the most recent ETL file may not be fully processed. We
            // don't consider that as a gap. So let's remove the most recent ETL file.
            sourceEtlFullyProcessed.Remove(mostRecentEtl);

            // Figure out if there are any ETL files that the ETL-to-CSV file writer has not fully
            // processed yet.
            foreach (string etlFile in sourceEtlFullyProcessed.Keys)
            {
                if (false == sourceEtlFullyProcessed[etlFile])
                {
                    unprocessedEtls.Add(etlFile);
                }
            }

            // If the count of unprocessed ETL files is greater than zero, then it means that
            // we have gaps in the DTR files.
            return unprocessedEtls.Count > 0;
        }

        internal static string GetEtlFileNameFromTraceFileName(string fileName)
        {
            string[] fileNameParts = fileName.Split('_');
            Debug.Assert(fileNameParts.Length >= 4, "All file name schemas have at least 4 '_' seperated parts.");

            // The first part is the fabric node ID, the second-last part is the
            // timestamp ticks and the last part is the timestamp differentiator. 
            // Everything in between in the ETL file name (without extension).
            string[] etlFileNameParts = new string[fileNameParts.Length - 3];
            Array.Copy(fileNameParts, 1, etlFileNameParts, 0, etlFileNameParts.Length);
            string etlFileNameWithoutExtension = string.Join("_", etlFileNameParts);

            return string.Concat(etlFileNameWithoutExtension, ".etl");
        }

        internal void SetEtlProducer(IEtlProducer etlProducer)
        {
            lock (this.EtlProducerLock)
            {
                this.EtlProducer = etlProducer;
                if (null != this.EtlProducer)
                {
                    this.IsProcessingWindowsFabricEtlFiles = this.EtlProducer.IsProcessingWindowsFabricEtlFiles();
                    if (this.organizeWindowsFabricTracesByType && this.IsProcessingWindowsFabricEtlFiles)
                    {
                        // For Windows Fabric traces, we create sub-folders based on whether
                        // it is a bootstrap trace, fabric trace or lease trace
                        this.CreateWindowsFabricTraceSubFolders();
                    }
                }
            }
        }

        internal void OnEtlFileReadStart(string fileName, bool isActiveEtl, string currentBookmark)
        {
            // Reset the last event index
            this.lastEventIndex.Set(DateTime.MinValue, -1);

            // Get the index up to which we have already processed events from 
            // this file.
            this.maxIndexAlreadyProcessed = this.GetMaxIndexAlreadyProcessed(
                                                fileName);

            // Create a new file sink.
            this.fileSink = new InternalFileSink(
                                            this.TraceSource,
                                            this.LogSourceId);
            this.fileSink.Initialize();
            this.TraceSource.WriteInfo(
                this.LogSourceId,
                "Filtered traces from {0} will be written to {1}.",
                fileName,
                this.fileSink.TempFileName);
        }

        internal void OnEtlFileReadStop(string fileName, bool isActiveEtl, string currentBookmark, string nextBookmark, bool fileReadAborted)
        {
            if (null == this.fileSink)
            {
                // An error occurred during the creation of the temporary file
                // sink. So just return immediately without doing anything.
                return;
            }

            if (false == fileReadAborted)
            {
                // Flush pending events
                this.FlushPendingEvents(isActiveEtl, currentBookmark, nextBookmark);
            }

            // Close the file sink
            this.fileSink.Close();

            // logging bytes written
            var fileInfo = new FileInfo(this.fileSink.TempFileName);
#if !DotNetCoreClr            
            this.perfHelper.BytesWritten(fileInfo.Length);
#endif            

            if (this.fileSink.WriteStatistics.EncoderErrors != 0)
            {
                this.TraceSource.WriteError(
                    this.LogSourceId,
                    "Encoder exceptions occurred while writing filtered ETW event to {0}. Error count: {1}. This may indicate a mismatch in .etl and .man files.",
                    this.fileSink.TempFileName,
                    this.fileSink.WriteStatistics.EncoderErrors);
            }

            if (false == fileReadAborted)
            {
                // Copy the trace file to the ETW CSV directory
                this.CopyTraceFileForUpload(fileName, isActiveEtl);
            }

            this.fileSink.Delete();
        }

        protected override void ProcessTraceEvent(DecodedEventWrapper eventWrapper)
        {
#if !DotNetCoreClr            
            this.perfHelper.EtwEventProcessed();
#endif
            if (null == this.fileSink)
            {
                // An error occurred during the creation of the temporary file
                // sink. So just return immediately without doing anything.
                return;
            }

            // Record the index of this event 
            this.lastEventIndex.Set(eventWrapper.Timestamp, eventWrapper.TimestampDifferentiator);

            // 1. Check if the event has already been processed in a previous pass.
            //    This check would effectively drop out of order timestamp events.
            // 2. If we are using compression, then we don't filter the logs. Just 
            //    include everything.
            // 3. If we're not using compression and it is a Windows Fabric event, 
            //    then check the filters to see if the event should be included in 
            //    the ETW traces.
            if ((this.lastEventIndex.CompareTo(this.maxIndexAlreadyProcessed) > 0) &&
                (this.compressCsvFiles || this.FiltersAllowEventInclusion(eventWrapper)))
            {
                // According to the filters, the event should be included.
                // Write the event to the ETW CSV file.
                try
                {
                    this.fileSink.WriteEvent(eventWrapper.StringRepresentation.Replace("\r\n", "\r\t").Replace("\n", "\t"));
#if !DotNetCoreClr                    
                    this.perfHelper.EventWrittenToCsv();
#endif
                }
                catch (Exception e)
                {
                    // Log an error and move on
                    this.TraceSource.WriteExceptionAsError(
                        this.LogSourceId,
                        e,
                        "Failed to write filtered event {0} to trace file {1}.",
                        eventWrapper.StringRepresentation,
                        this.fileSink.TempFileName);
                }
            }
        }

        private void CreateWindowsFabricTraceSubFolders()
        {
            string subFolder = Path.Combine(this.filteredTraceDirName, BootstrapTracesFolderName);
            FabricDirectory.CreateDirectory(subFolder);
            subFolder = Path.Combine(this.filteredTraceDirName, FabricTracesFolderName);
            FabricDirectory.CreateDirectory(subFolder);
            subFolder = Path.Combine(this.filteredTraceDirName, LeaseTracesFolderName);
            FabricDirectory.CreateDirectory(subFolder);
        }

        private string GetTraceFileSubFolder(string etlfileName)
        {
            if (this.organizeWindowsFabricTracesByType && this.IsProcessingWindowsFabricEtlFiles)
            {
                if (etlfileName.StartsWith(
                        WinFabricManifestManager.FabricEtlPrefix,
                        StringComparison.OrdinalIgnoreCase))
                {
                    return FabricTracesFolderName;
                }
                else if (etlfileName.StartsWith(
                            WinFabricManifestManager.LeaseLayerEtlPrefix,
                            StringComparison.OrdinalIgnoreCase))
                {
                    return LeaseTracesFolderName;
                }
                else
                {
                    return string.Empty;
                }
            }
            else
            {
                return string.Empty;
            }
        }

        private void CopyTraceFileForUpload(string fileName, bool isActiveEtl)
        {            
            // If the temporary trace file does not contain events and if the ETL
            // file that is being processed is an active ETL file, then don't 
            // copy the temporary file to the ETW CSV directory.
            //
            // In contrast, temporary trace files corresponding to inactive ETL  
            // files are copied even if they don't have events in them (i.e. a 
            // zero-byte file is copied) because it makes it easier to identify
            // gaps in traces if the DCA happens to fall behind on trace processing.
            // For details, please see comments later in this function.
            if (0 == this.fileSink.WriteStatistics.EventsWritten && isActiveEtl)
            {
                return;
            }

            // Build the destination name for the filtered trace file
            //
            //         !!! WARNING !!! 
            // The trace viewer tool parses the file names of the filtered trace files.
            // Changing the file name format might require a change to trace viewer as well.
            //         !!! WARNING !!!
            //
            // If the ETL file is an active ETL file, the trace file name is of the form:
            // <FabricNodeID>_<etlFileName>_<TimestampOfLastEventProcessed>_<TimestampDifferentiatorOfLastEventProcessed>.dtr
            // For containers the file structure is:
            // <FabricNodeID>_<ContainerName>_<etlFileName>_<TimestampOfLastEventProcessed>_<TimestampDifferentiatorOfLastEventProcessed>.dtr
            //
            // If the ETL file is an inactive ETL file, the trace file name is of the form:
            // <FabricNodeID>_<etlFileName>_<TimestampOfLastEventProcessed>_<Int32.MaxValue>.dtr
            // For containers the file structure is:
            // <FabricNodeID>_<ContainerName>_<etlFileName>_<TimestampOfLastEventProcessed>_<Int32.MaxValue>.dtr
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
                                        isActiveEtl ? this.lastEventIndex.TimestampDifferentiator : int.MaxValue);
            string traceFileNamePrefix = string.Format(
                                                  CultureInfo.InvariantCulture,
                                                  "{0}_{1}_{2:D20}_{3}.",
                                                  this.fabricNodeId,
                                                  Path.GetFileNameWithoutExtension(fileName),
                                                  this.lastEventIndex.Timestamp.Ticks,
                                                  differentiator);

            var applicationInstanceId = ContainerEnvironment.GetContainerApplicationInstanceId(this.LogSourceId);
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
                                                Path.GetFileNameWithoutExtension(fileName),
                                                this.lastEventIndex.Timestamp.Ticks,
                                                differentiator);
            }

            string traceFileNameWithoutPath = string.Concat(
                                                  traceFileNamePrefix,
                                                  EtlConsumerConstants.FilteredEtwTraceFileExtension);
            string compressedTraceFileNameWithoutPath = string.Concat(
                                                            traceFileNamePrefix,
                                                            EtlConsumerConstants.FilteredEtwTraceFileExtension,
                                                            EtlConsumerConstants.CompressedFilteredEtwTraceFileExtension);
            string subFolder = this.GetTraceFileSubFolder(fileName);
            string traceFileDestinationPath = Path.Combine(
                                                  this.filteredTraceDirName,
                                                  subFolder);
            string traceFileDestinationName = Path.Combine(
                                                traceFileDestinationPath,
                                                this.compressCsvFiles ? compressedTraceFileNameWithoutPath : traceFileNameWithoutPath);
            string alternateTraceFileDestinationName = Path.Combine(
                                                          traceFileDestinationPath,
                                                          this.compressCsvFiles ? traceFileNameWithoutPath : compressedTraceFileNameWithoutPath);

            // If a file with the same name already exists at the destination, 
            // then don't copy the file over. This is because if the file already
            // existed at the destination, then the file that are about to copy
            // over must be a zero-byte file because we always ignore events that
            // we have already processed. Therefore, we don't want to overwrite
            // a file that contains events with a zero-byte file.
            if (InternalFileSink.FileExists(traceFileDestinationName) ||
                InternalFileSink.FileExists(alternateTraceFileDestinationName))
            {
                Debug.Assert(0 == this.fileSink.WriteStatistics.EventsWritten, "The temporary trace file must be a zero-byte file.");

                // Also, the ETL file must be an inactive ETL file because if it
                // had been an active ETL file and the temporary trace file was
                // empty, then we would have already returned from this method
                // due to a check made at the beginning of this method.
                Debug.Assert(false == isActiveEtl, "File must be inactive.");
                return;
            }

            // Copy the file
            try
            {
                InternalFileSink.CopyFile(this.fileSink.TempFileName, traceFileDestinationName, false, this.compressCsvFiles);

                // logging bytes read and written 
                var fileInfo = new FileInfo(this.fileSink.TempFileName);
#if !DotNetCoreClr                
                this.perfHelper.BytesRead(fileInfo.Length);
                this.perfHelper.BytesWritten(fileInfo.Length);
#endif                

                this.TraceSource.WriteInfo(
                    this.LogSourceId,
                    "Filtered traces from {0} are ready. They have been moved from {1} to {2}.",
                    fileName,
                    this.fileSink.TempFileName,
                    traceFileDestinationName);
            }
            catch (Exception e)
            {
                // Log an error and move on
                this.TraceSource.WriteExceptionAsError(
                    this.LogSourceId,
                    e,
                    "Failed to copy file. Source: {0}, destination: {1}.",
                    this.fileSink.TempFileName,
                    traceFileDestinationName);
            }
        }

        private EventIndex GetMaxIndexAlreadyProcessed(string etlFileName)
        {
            // From the ETW CSV directory, retrieve the names of all CSV files 
            // that we generated from this ETL file
            string traceFilePattern = string.Format(
                                          CultureInfo.InvariantCulture,
                                          "{0}_{1}_*.{2}*",
                                          this.fabricNodeId,
                                          Path.GetFileNameWithoutExtension(etlFileName),
                                          EtlConsumerConstants.FilteredEtwTraceFileExtension);
            string subFolder = this.GetTraceFileSubFolder(etlFileName);
            string folderToSearch = Path.Combine(this.filteredTraceDirName, subFolder);
            EventIndex index = new EventIndex();
            string[] traceFiles;
            try
            {
                InternalFileSink.GetFilesMatchingPattern(
                    folderToSearch,
                    traceFilePattern,
                    out traceFiles);
            }
            catch (Exception e)
            {
                this.TraceSource.WriteError(
                    this.LogSourceId,
                    "Failed to retrieve files matching pattern {0} in directory {1}. This may cause events to be duplicated in multiple files. {2}",
                    traceFilePattern,
                    this.filteredTraceDirName,
                    e);

                // We'll assume that no events from this file have been processed yet.
                // This may end up being an incorrect assumption, in which case some 
                // events can end up duplicated in multiple files.
                index.Set(DateTime.MinValue, -1);
                return index;
            }

            if (traceFiles.Length == 0)
            {
                // No events from this file have been processed yet
                index.Set(DateTime.MinValue, -1);
                return index;
            }

            // Sort the files based on file name and get the name of the newest 
            // file
            Array.Sort(traceFiles);
            string newestFile = traceFiles[traceFiles.Length - 1];

            // From the name of the newest file, extract the maximum index already 
            // processed
            string newestFileWithoutExtension = Path.GetFileNameWithoutExtension(newestFile);
            if (DtrRegEx.IsMatch(newestFileWithoutExtension))
            {
                newestFileWithoutExtension = Path.GetFileNameWithoutExtension(newestFileWithoutExtension);
            }

            string[] fileNameParts = newestFileWithoutExtension.Split('_');
            Debug.Assert(fileNameParts.Length >= 4, "All file name schemas have at least 4 '_' seperated parts.");

            long timestampTicks = long.Parse(
                                      fileNameParts[fileNameParts.Length - 2],
                                      CultureInfo.InvariantCulture);
            DateTime timestamp = new DateTime(timestampTicks);
            int timestampDifferentiator = int.Parse(
                                              fileNameParts[fileNameParts.Length - 1],
                                              CultureInfo.InvariantCulture);
            index.Set(timestamp, timestampDifferentiator);
            return index;
        }

        /// <summary>
        /// This structure represents the index of an event within an ETL file
        /// </summary>
        private struct EventIndex
        {
            /// <summary>
            /// Timestamp of the event.
            /// </summary>
            internal DateTime Timestamp;

            /// <summary>
            /// Value that distinguishes this event from other events in the same ETL
            /// file that also bear the same timestamp.
            /// </summary>
            internal int TimestampDifferentiator;

            internal void Set(DateTime timestamp, int timestampDifferentiator)
            {
                this.Timestamp = timestamp;
                this.TimestampDifferentiator = timestampDifferentiator;
            }

            internal int CompareTo(EventIndex index)
            {
                int dateTimeCompareResult = this.Timestamp.CompareTo(index.Timestamp);
                if (dateTimeCompareResult != 0)
                {
                    return dateTimeCompareResult;
                }

                if (this.TimestampDifferentiator < index.TimestampDifferentiator)
                {
                    return -1;
                }
                else if (this.TimestampDifferentiator == index.TimestampDifferentiator)
                {
                    return 0;
                }
                else
                {
                    return 1;
                }
            }
        }
    }
}