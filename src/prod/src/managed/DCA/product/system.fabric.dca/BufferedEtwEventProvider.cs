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
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Threading;
    using Tools.EtlReader;

    using Utility = System.Fabric.Dca.Utility;

    /// <summary>
    /// This class implements the logic to buffer ETW events locally on disk and
    /// deliver the buffered events to the consumer
    /// </summary>
    internal class BufferedEtwEventProvider : EventProcessor, IEtlFileSink, IBufferedEtwEventProvider
    {
        // Constants        
        private const string OldLogDeletionTimerIdSuffix = "_OldBufferedEtwEventDeletionTimer";
        private const string EventDeliveryTimerIdSuffix = "_BufferedEtwEventDeliveryTimer";
        private const string TempCacheFileNamePrefix = "Cache_";
        private const string TempCacheFileExtension = "tmp";
        private const string TempCacheFileNameSearchPattern = "Cache_*.tmp";
        private const string CacheFileNameExtension = "dat";
        private const string CacheFileNameSearchPattern = "Cache_*.dat";

        #region Private Fields
        /// <summary>
        /// This is the directory in which ETW event information is buffered
        /// </summary>
        private readonly string etwEventCache;

        /// <summary>
        /// This is the amount of time we spend delivering buffered ETW events
        /// to the consumer in each pass.
        /// </summary>
        private readonly TimeSpan eventDeliveryPassLength;

        // Timer to deliver ETW events to consumer
        private readonly DcaTimer eventDeliveryTimer;

        // Timer to delete old logs
        private readonly DcaTimer oldLogDeletionTimer;

        // Helper object to log performance-related information
        private readonly BufferedEtwEventPerformance perfHelper;

        // Time after which the ETW trace file on disk becomes a candidate for deletion
        private readonly TimeSpan eventDeletionAge;

        // Sink for buffered events
        private readonly IBufferedEtwEventSink eventSink;

        /// <summary>
        /// Bookmark for the current source file.
        /// </summary>
        private string bookmark;

        /// <summary>
        /// Sequence number of the event within the current source file.
        /// </summary>
        private int eventSequenceNumber;

        /// <summary>
        /// StreamWriter object that is used to write events to the buffered event file
        /// </summary>
        private StreamWriter streamWriter;

        // Whether or not the custom consumer has asked for event delivery to be aborted
        private bool eventDeliveryPeriodAborted;

        // Whether or not the object has been disposed
        private bool disposed;

        #endregion

        #region Constructors
        internal BufferedEtwEventProvider(
            ITraceEventSourceFactory traceEventSourceFactory,
            string logSourceId, 
            string traceBufferingFolder, 
            TimeSpan uploadInterval, 
            TimeSpan eventDeletionAgeIn, 
            IBufferedEtwEventSink eventSink) 
                     : base(traceEventSourceFactory, logSourceId)
        {
            this.eventSink = eventSink;
            
            this.bookmark = null;
            this.perfHelper = new BufferedEtwEventPerformance(this.TraceSource, this.LogSourceId);
            this.eventDeletionAge = eventDeletionAgeIn;

            // Compute how much time we spend delivering buffered ETW events to 
            // the consumer in each pass. We define this to be roughly 1/4th of 
            // the interval between passes. This is an arbitrary choice.
            this.eventDeliveryPassLength = TimeSpan.FromMilliseconds(uploadInterval.TotalMilliseconds / 4);
            
            // Create the buffered event directory, in case it doesn't already exist.
            this.etwEventCache = traceBufferingFolder;
            FabricDirectory.CreateDirectory(this.etwEventCache);

            this.TraceSource.WriteInfo(
                this.LogSourceId,
                "Directory containing buffered events: {0}",
                this.etwEventCache);

            // Create a timer to schedule the delivery of ETW events to the consumer
            string timerId = string.Concat(
                                 this.LogSourceId,
                                 EventDeliveryTimerIdSuffix);
            this.eventDeliveryTimer = new DcaTimer(
                                            timerId,
                                            this.DeliverEventsToConsumer,
                                            uploadInterval);
            this.eventDeliveryTimer.Start();

            // Create a timer to delete old logs
            timerId = string.Concat(
                          this.LogSourceId,
                          OldLogDeletionTimerIdSuffix);
            var oldLogDeletionInterval =
                (this.eventDeletionAge < TimeSpan.FromDays(1))
                    ? EtlConsumerConstants.OldLogDeletionIntervalForTest
                    : EtlConsumerConstants.OldLogDeletionInterval;
            this.oldLogDeletionTimer = new DcaTimer(
                                               timerId,
                                               this.DeleteOldLogsHandler,
                                               oldLogDeletionInterval);
            this.oldLogDeletionTimer.Start();
        }
        #endregion

        private enum EventLineParts
        {
            UniqueId,
            Timestamp,
            ReadableTimestamp,
            Level,
            ThreadId,
            ProcessId,
            TaskName,
            EventType,
            EventText,

            // This is not a part of the string. It is just an enum value that
            // represents the total count of parts.
            Count
        }

        private enum EventLineV2Parts
        {
            ProviderId,
            EventId,
            EventVersion,
            Channel,
            Opcode,
            TaskId,
            Keyword,
            V1Part,

            // This is not a part of the string. It is just an enum value that
            // represents the total count of parts.
            Count
        }

        public bool NeedsDecodedEvent
        {
            get
            {
                return true;
            }
        }

        public void OnEtwEventProcessingPeriodStart()
        {
            this.streamWriter = null;
            this.perfHelper.EtlReadPassBegin();

            // Build the full path to our buffered event file
            string tempCacheFileName = string.Format(
                                           CultureInfo.InvariantCulture,
                                           "{0}{1}.{2}",
                                           TempCacheFileNamePrefix,
                                           DateTime.Now.Ticks,
                                           TempCacheFileExtension);
            string tempCacheFileFullPath = Path.Combine(this.etwEventCache, tempCacheFileName);

            // Open the file
            StreamWriter writer = null;
            try
            {
                Utility.PerformIOWithRetries(
                    ctx =>
                    {
                        string fileName = ctx;
                        FileStream fileStream = FabricFile.Open(fileName, FileMode.Create, FileAccess.Write);
#if !DotNetCoreClr                        
                        Helpers.SetIoPriorityHint(fileStream.SafeFileHandle, Kernel32Types.PRIORITY_HINT.IoPriorityHintVeryLow);
#endif                        
                        writer = new StreamWriter(fileStream);
                    },
                    tempCacheFileFullPath);
            }
            catch (Exception e)
            {
                // Log an error and move on. No events from this pass will be
                // written to the destination.
                this.TraceSource.WriteExceptionAsError(
                    this.LogSourceId,
                    e,
                    "Failed to create a new file in the buffered event directory. None of the events from this pass will be written to the event buffer.");
                return;
            }

            // Write the version number. This will help us read data from the file.
            try
            {
                Utility.PerformIOWithRetries(
                    () =>
                    {
                        writer.WriteLine(EtlConsumerConstants.EtwEventCacheFormatVersionString);
                    });
            }
            catch (Exception e)
            {
                // Log an error and move on. No events from this pass will be
                // written to the destination.
                this.TraceSource.WriteExceptionAsError(
                    this.LogSourceId,
                    e,
                    "Failed to write buffered event file format version number to file {0} in the buffered event file. None of the events from this pass will be written to the event buffer.",
                    tempCacheFileFullPath);
                return;
            }

            this.streamWriter = writer;
            this.TraceSource.WriteInfo(
                this.LogSourceId,
                "Filtered ETW events for consumer will be buffered in file {0}.",
                tempCacheFileFullPath);
        }

        public void OnEtwEventProcessingPeriodStop()
        {
            if (null == this.streamWriter)
            {
                // An error occurred during the creation of the buffered event
                // file. So just return immediately without doing anything.
                return;
            }

            // Close the buffered event file that we are currently working on.
            try
            {
                this.streamWriter.Dispose();
            }
            catch (System.Text.EncoderFallbackException ex)
            {
                // This can happen if the manifest file does not match the binary.
                // Write an error message and move on.
                this.TraceSource.WriteError(
                    this.LogSourceId,
                    "Exception occurred while closing buffered event file. Exception information: {0}",
                    ex);
            }

            // Make our buffered event files available for delivery to the consumer. 
            // This includes:
            //  - the buffered event file that we wrote in this pass 
            //  - any old buffered event files that we wrote in previous passes 
            //    (and got interrupted before we could rename them for delivery to
            //    the consumer).
            string[] cacheFiles = FabricDirectory.GetFiles(
                                      this.etwEventCache,
                                      TempCacheFileNameSearchPattern);
            foreach (string cacheFile in cacheFiles)
            {
                try
                {
                    Utility.PerformIOWithRetries(
                        ctx =>
                        {
                            string fileName = ctx;
                            FabricFile.Move(
                                fileName,
                                Path.ChangeExtension(fileName, CacheFileNameExtension));
                        },
                        cacheFile);
                    
                    this.TraceSource.WriteInfo(
                        this.LogSourceId,
                        "File containing filtered ETW events was renamed for delivery to consumer. Old name: {0}, new name: {1}.",
                        cacheFile,
                        Path.ChangeExtension(cacheFile, CacheFileNameExtension));
                }
                catch (Exception e)
                {
                    this.TraceSource.WriteExceptionAsError(
                        this.LogSourceId,
                        e,
                        "Buffered event file {0} could not be renamed for delivery to consumer.",
                        cacheFile);
                }
            }

            this.perfHelper.EtlReadPassEnd();
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

        public bool OnBootstrapTraceFileAvailable(string filename)
        {
            // We don't handle bootstrap files
            return true;
        }

        public void AbortEtwEventDelivery()
        {
            this.eventDeliveryPeriodAborted = true;
        }

        public void SetEtwEventFilter(string filterString, string defaultFilterString, string summaryFilterString, bool isWindowsFabricEventFilter)
        {
            lock (this.FilterLock)
            {
                // Create the filter for Windows Fabric events
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
            this.eventDeliveryTimer.StopAndDispose();
            this.oldLogDeletionTimer.StopAndDispose();

            // Wait for timer callbacks to finish executing
            WaitHandle[] timerDisposedEvents = new WaitHandle[2];
            timerDisposedEvents[0] = this.eventDeliveryTimer.DisposedEvent;
            timerDisposedEvents[1] = this.oldLogDeletionTimer.DisposedEvent;
            WaitHandle.WaitAll(timerDisposedEvents);

            GC.SuppressFinalize(this);
        }
        
        internal void SetEtlProducer(IEtlProducer etlProducer)
        {
            lock (this.EtlProducerLock)
            {
                this.EtlProducer = etlProducer;
                if (null != this.EtlProducer)
                {
                    this.IsProcessingWindowsFabricEtlFiles = this.EtlProducer.IsProcessingWindowsFabricEtlFiles();
                }
            }
        }

        internal void OnEtlFileReadStart(string fileName, bool isActiveEtl, string currentBookmark)
        {
            // Retrieve the bookmark for this ETL file. It is used as the prefix
            // in the unique ID for each of the events in this ETL file.
            this.bookmark = currentBookmark;

            // Reset the sequence number for events from this ETL file
            this.eventSequenceNumber = 0;
        }

        internal void OnEtlFileReadStop(string fileName, bool isActiveEtl, string currentBookmark, string nextBookmark, bool fileReadAborted)
        {
            if (null == this.streamWriter)
            {
                // An error occurred during the creation of the buffered event
                // file. So just return immediately without doing anything.
                return;
            }

            if (false == fileReadAborted)
            {
                // Flush pending events
                this.FlushPendingEvents(isActiveEtl, currentBookmark, nextBookmark);
            }

            // We're done processing an ETL file. Before this ETL file is moved 
            // to the archives, make sure all the events we've written so far are
            // flushed to the buffered event file.
            try
            {
                Utility.PerformIOWithRetries(
                    () =>
                    {
                        try
                        {
                            this.streamWriter.Flush();
                        }
                        catch (System.Text.EncoderFallbackException ex)
                        {
                            // This can happen if the manifest file does not match the binary.
                            // Write an error message and move on.
                            this.TraceSource.WriteError(
                                this.LogSourceId,
                                "Exception occurred while flushing filtered ETW events to buffered event file. Exception information: {0}",
                                ex);
                        }
                    });
            }
            catch (Exception e)
            {
                // Log an error and move on
                this.TraceSource.WriteExceptionAsError(
                    this.LogSourceId,
                    e,
                    "Failed to flush data to buffered event file.");
            }
        }

        protected override void ProcessTraceEvent(DecodedEventWrapper eventWrapper)
        {
            System.Fabric.Interop.Utility.ReleaseAssert(this.bookmark != null, StringResources.DCAError_ProcessTraceEventCalledUnexpectedly);

            this.perfHelper.EtwEventProcessed();

            // If it is a Windows Fabric event, then check the filters to see if
            // the event should be included in the ETW traces.
            if (this.FiltersAllowEventInclusion(eventWrapper))
            {
                // According to the filters, the event should be included.

                // Compute the row key for this event.
                //
                // The row key is in the following format:
                // <etlFileBookmark>_<eventSequenceNumberInEtlFile>
                //
                // By choosing the row key in this manner, we ensure the following:
                //   - The row key for an event is unique within the partition.
                //     Recall that the partition in this case is the deployment
                //     ID of the role instance. 
                //   - If an event happens to get processed multiple times (for
                //     example because the previous pass was interrupted), it 
                //     gets the same row key each time. Thus event processing is
                //     idempotent.
                //
                // Uniqueness and idempotence are ensured because the event 
                // sequence number for an ETL file starts at 0 on every pass and
                // increases for each event in the pass. If the same ETL file is
                // read in multiple passes, bookmark for that ETL file is 
                // different for every pass, unless a pass is interrupted. If a
                // pass is interrupted, the next pass has the same bookmark, but
                // it also starts reading events from the same point within the 
                // ETL file. Thus, the comination of bookmark and event sequence
                // number for an event is always unique within a role instance 
                // and always the same if that event is processed multiple times.
                string nodesUniqueEventId = string.Concat(
                                    this.bookmark,
                                    "_",
                                    this.eventSequenceNumber.ToString(CultureInfo.InvariantCulture));

                // Write the event to the cache
                this.WriteTraceEvent(eventWrapper, nodesUniqueEventId);
                this.perfHelper.EventWrittenToLocalBuffer();
            }

            // Increment the event sequence number. This needs to be done 
            // regardless of whether the event was included or not. This way, if
            // an event is processed in multiple passes (for example because a 
            // previous pass was interrupted) and the filtering criteria changes
            // in between passes, all events still have the same sequence numbers.
            // Thus, the sequence numbers are kept independent of the results of
            // filtering.
            this.eventSequenceNumber++;
        }

        private static void ReadLineFromFile(LineParam lineParam)
        {
            Utility.PerformIOWithRetries(
                ctx =>
                {
                    LineParam param = ctx;
                    param.Line = param.Reader.ReadLine();
                },
                lineParam);
        }

        private static int CompareFileLastWriteTimes(FileInfo file1, FileInfo file2)
        {
            // We want the file with the more recent creation time to come first
            return DateTime.Compare(file2.LastWriteTime, file1.LastWriteTime);
        }

        private void OpenEtwEventCacheFile(object context)
        {
            ReadEventsFromFileParam param = (ReadEventsFromFileParam)context;
            param.FileNotFound = false;
            try
            {
                FileStream fileStream = FabricFile.Open(param.FileName, FileMode.Open, FileAccess.Read);
#if !DotNetCoreClr                
                Helpers.SetIoPriorityHint(fileStream.SafeFileHandle, Kernel32Types.PRIORITY_HINT.IoPriorityHintVeryLow);
#endif                
                param.StreamReader = new StreamReader(fileStream);
            }
            catch (FileNotFoundException e)
            {
                // The file was not found, most likely because it was deleted by
                // the old logs deletion routine.
                this.TraceSource.WriteWarning(
                    this.LogSourceId,
                    "Exception occurred while trying to open buffered event file {0} for read. The file may have been deleted. Exception information: {1}",
                    param.FileName,
                    e);
                param.FileNotFound = true;
            }
        }

        private void WriteTraceEvent(DecodedEventWrapper eventWrapper, string nodesUniqueEventId)
        {
            if (null == this.streamWriter)
            {
                // An error occurred during the creation of the buffered event
                // file. So just return immediately without doing anything.
                return;
            }

            // NOTE: The order in which the information is written must match 
            // the order of the EventLineParts enumeration defined in this class.
            string eventInfo = string.Format(
                                   CultureInfo.InvariantCulture,
                                   "{0},{1},{2},{3},{4},{5},{6},{7},{8},{9},{10},{11},{12},{13},{14},{15}",
                                   eventWrapper.InternalEvent.EventRecord.EventHeader.ProviderId,
                                   eventWrapper.InternalEvent.EventRecord.EventHeader.EventDescriptor.Id,
                                   eventWrapper.InternalEvent.EventRecord.EventHeader.EventDescriptor.Version,
                                   eventWrapper.InternalEvent.EventRecord.EventHeader.EventDescriptor.Channel,
                                   eventWrapper.InternalEvent.EventRecord.EventHeader.EventDescriptor.Opcode,
                                   eventWrapper.InternalEvent.EventRecord.EventHeader.EventDescriptor.Task,
                                   eventWrapper.InternalEvent.EventRecord.EventHeader.EventDescriptor.Keyword,
                                   nodesUniqueEventId,
                                   eventWrapper.Timestamp.ToBinary(),
                                   eventWrapper.Timestamp, // We write the human-readable form for easier debugging
                                   eventWrapper.Level,
                                   eventWrapper.ThreadId,
                                   eventWrapper.ProcessId,
                                   eventWrapper.TaskName,
                                   eventWrapper.EventType,
                                   eventWrapper.EventText);
            eventInfo = eventInfo.Replace("\r\n", "\t").Replace("\n", "\t").Replace("\r", "\t");
            try
            {
                Utility.PerformIOWithRetries(
                    ctx =>
                    {
                        string eventString = ctx;
                        try
                        {
                            this.streamWriter.WriteLine(eventString);
                        }
                        catch (System.Text.EncoderFallbackException ex)
                        {
                            // This can happen if the manifest file does not match the binary.
                            // Write an error message and move on.
                            this.TraceSource.WriteError(
                                this.LogSourceId,
                                "Exception occurred while writing filtered ETW event to buffered event file. Exception information: {0}",
                                ex);
                        }
                    },
                    eventInfo);
            }
            catch (Exception e)
            {
                // Log an error and move on
                this.TraceSource.WriteExceptionAsError(
                    this.LogSourceId,
                    e,
                    "Failed to write ETW event to buffered event file.");
            }
        }
        
        private bool CanParseFile(StreamReader streamReader, string fileName, out int version)
        {
            version = -1;

            // Read the version information from the file
            LineParam versionParam = new LineParam();
            versionParam.Reader = streamReader;
            try
            {
                ReadLineFromFile(versionParam);
            }
            catch (Exception e)
            {
                this.TraceSource.WriteExceptionAsError(
                    this.LogSourceId,
                    e,
                    "Failed to get version information from file {0}. File will be skipped.",
                    fileName);
                return false;
            }

            if (null == versionParam.Line)
            {
                // End of file reached
                this.TraceSource.WriteError(
                    this.LogSourceId,
                    "Version information was not present in file {0}.",
                    fileName);
                return false;
            }

            // Figure out the version number
            string[] versionLineParts = versionParam.Line.Split(':');
            if (versionLineParts.Length != 2)
            {
                this.TraceSource.WriteError(
                    this.LogSourceId,
                    "Unable to parse the version information '{0}' in file {1}.",
                    versionParam,
                    fileName);
                return false;
            }

            if (false == int.TryParse(versionLineParts[1], out version))
            {
                this.TraceSource.WriteError(
                    this.LogSourceId,
                    "Unable to parse version number '{0}' as an integer.",
                    versionLineParts[1]);
                return false;
            }

            // Verify that the version number is in the range we expect
            if ((version < 1) || (version > 2))
            {
                this.TraceSource.WriteError(
                    this.LogSourceId,
                    "Unable to parse file {0} because it uses unknown version number {1}.",
                    fileName,
                    version);
                return false;
            }

            return true;
        }
        
        private bool ParseEventInfo(string fileName, int version, string line, ref DecodedEtwEvent etwEventInfo, ref string nodeUniqueEventId)
        {
            string lineV1 = line;

            if (version > 1)
            {
                // The new fields added in V2 were added at the beginning. So parse them first.
                if (false == this.ParseV2EventInfo(fileName, line, ref lineV1, ref etwEventInfo))
                {
                    return false;
                }
            }

            // The event text can also contain commas, so we pass in the maximum
            // substring count to the Split method below, so that the event text
            // does not get split.
            string[] lineParts = lineV1.Split(new[] { ',' }, (int)EventLineParts.Count);
            if (lineParts.Length != ((int)EventLineParts.Count))
            {
                this.TraceSource.WriteError(
                    this.LogSourceId,
                    "Event '{0}' in buffered event file {1} was not in the expected format. The event did not have the expected number of comma separators.",
                    line,
                    fileName);
                return false;
            }

            nodeUniqueEventId = lineParts[(int)EventLineParts.UniqueId];
            long timestampBinary;
            if (false == long.TryParse(
                             lineParts[(int)EventLineParts.Timestamp],
                             NumberStyles.Integer,
                             CultureInfo.InvariantCulture,
                             out timestampBinary))
            {
                this.TraceSource.WriteError(
                    this.LogSourceId,
                    "Event '{0}' in buffered event file {1} was not in the expected format. The event timestamp could not be parsed as a long data type.",
                    line,
                    fileName);
                return false;
            }

            etwEventInfo.Timestamp = DateTime.FromBinary(timestampBinary);

            if (false == byte.TryParse(
                             lineParts[(int)EventLineParts.Level],
                             NumberStyles.Integer,
                             CultureInfo.InvariantCulture,
                             out etwEventInfo.Level))
            {
                this.TraceSource.WriteError(
                    this.LogSourceId,
                    "Event '{0}' in buffered event file {1} was not in the expected format. The event level could not be parsed as an byte data type.",
                    line,
                    fileName);
                return false;
            }

            if (false == uint.TryParse(
                             lineParts[(int)EventLineParts.ThreadId],
                             NumberStyles.Integer,
                             CultureInfo.InvariantCulture,
                             out etwEventInfo.ThreadId))
            {
                this.TraceSource.WriteError(
                    this.LogSourceId,
                    "Event '{0}' in buffered event file {1} was not in the expected format. The thread ID could not be parsed as an int data type.",
                    line,
                    fileName);
                return false;
            }

            if (false == uint.TryParse(
                             lineParts[(int)EventLineParts.ProcessId],
                             NumberStyles.Integer,
                             CultureInfo.InvariantCulture,
                             out etwEventInfo.ProcessId))
            {
                this.TraceSource.WriteError(
                    this.LogSourceId,
                    "Event '{0}' in buffered event file {1} was not in the expected format. The process ID could not be parsed as an int data type.",
                    line,
                    fileName);
                return false;
            }
            
            etwEventInfo.TaskName = lineParts[(int)EventLineParts.TaskName];
            etwEventInfo.EventType = lineParts[(int)EventLineParts.EventType];
            etwEventInfo.EventText = lineParts[(int)EventLineParts.EventText];
            return true;
        }

        private bool ParseV2EventInfo(string fileName, string line, ref string lineV1, ref DecodedEtwEvent etwEventInfo)
        {
            // We pass in the maximum substring count to the Split method below, so that we
            // only parse the new V2 fields. The V1 fields will be parsed by our caller.
            string[] lineParts = line.Split(new char[] { ',' }, (int)EventLineV2Parts.Count);
            if (lineParts.Length != ((int)EventLineV2Parts.Count))
            {
                this.TraceSource.WriteError(
                    this.LogSourceId,
                    "Event '{0}' in buffered event file {1} was not in the expected format. The event did not have the expected number of comma separators.",
                    line,
                    fileName);
                return false;
            }

            // Parse the V2 fields
            if (false == Guid.TryParse(
                             lineParts[(int)EventLineV2Parts.ProviderId],
                             out etwEventInfo.EventRecord.EventHeader.ProviderId))
            {
                this.TraceSource.WriteError(
                    this.LogSourceId,
                    "Event '{0}' in buffered event file {1} was not in the expected format. The provider ID could not be parsed as a GUID.",
                    line,
                    fileName);
                return false;
            }

            if (false == ushort.TryParse(
                             lineParts[(int)EventLineV2Parts.EventId],
                             NumberStyles.Integer,
                             CultureInfo.InvariantCulture,
                             out etwEventInfo.EventRecord.EventHeader.EventDescriptor.Id))
            {
                this.TraceSource.WriteError(
                    this.LogSourceId,
                    "Event '{0}' in buffered event file {1} was not in the expected format. The event ID could not be parsed as a ushort data type.",
                    line,
                    fileName);
                return false;
            }

            if (false == byte.TryParse(
                             lineParts[(int)EventLineV2Parts.EventVersion],
                             NumberStyles.Integer,
                             CultureInfo.InvariantCulture,
                             out etwEventInfo.EventRecord.EventHeader.EventDescriptor.Version))
            {
                this.TraceSource.WriteError(
                    this.LogSourceId,
                    "Event '{0}' in buffered event file {1} was not in the expected format. The event version could not be parsed as a byte data type.",
                    line,
                    fileName);
                return false;
            }

            if (false == byte.TryParse(
                             lineParts[(int)EventLineV2Parts.Channel],
                             NumberStyles.Integer,
                             CultureInfo.InvariantCulture,
                             out etwEventInfo.EventRecord.EventHeader.EventDescriptor.Channel))
            {
                this.TraceSource.WriteError(
                    this.LogSourceId,
                    "Event '{0}' in buffered event file {1} was not in the expected format. The channel could not be parsed as a byte data type.",
                    line,
                    fileName);
                return false;
            }

            if (false == byte.TryParse(
                             lineParts[(int)EventLineV2Parts.Opcode],
                             NumberStyles.Integer,
                             CultureInfo.InvariantCulture,
                             out etwEventInfo.EventRecord.EventHeader.EventDescriptor.Opcode))
            {
                this.TraceSource.WriteError(
                    this.LogSourceId,
                    "Event '{0}' in buffered event file {1} was not in the expected format. The opcode could not be parsed as a byte data type.",
                    line,
                    fileName);
                return false;
            }

            if (false == ushort.TryParse(
                             lineParts[(int)EventLineV2Parts.TaskId],
                             NumberStyles.Integer,
                             CultureInfo.InvariantCulture,
                             out etwEventInfo.EventRecord.EventHeader.EventDescriptor.Task))
            {
                this.TraceSource.WriteError(
                    this.LogSourceId,
                    "Event '{0}' in buffered event file {1} was not in the expected format. The task ID could not be parsed as a ushort data type.",
                    line,
                    fileName);
                return false;
            }

            if (false == ulong.TryParse(
                             lineParts[(int)EventLineV2Parts.Keyword],
                             NumberStyles.Integer,
                             CultureInfo.InvariantCulture,
                             out etwEventInfo.EventRecord.EventHeader.EventDescriptor.Keyword))
            {
                this.TraceSource.WriteError(
                    this.LogSourceId,
                    "Event '{0}' in buffered event file {1} was not in the expected format. The event keyword could not be parsed as a ulong data type.",
                    line,
                    fileName);
                return false;
            }

            // Leave the V1 part of this line for the caller to parse.
            lineV1 = lineParts[(int)EventLineV2Parts.V1Part];

            return true;
        }

        private void ProcessEventsFromFile(string fileName)
        {
            this.TraceSource.WriteInfo(
                this.LogSourceId,
                "Processing ETW events from file {0}.",
                fileName);
            
            // Open the file
            ReadEventsFromFileParam readEventsParam = new ReadEventsFromFileParam();
            readEventsParam.StreamReader = null;
            readEventsParam.FileName = fileName;
            try
            {
                Utility.PerformIOWithRetries(
                    this.OpenEtwEventCacheFile,
                    readEventsParam);
            }
            catch (Exception e)
            {
                 this.TraceSource.WriteExceptionAsError(
                     this.LogSourceId,
                     e,
                     "Failed to open file {0} for read.",
                     fileName);
                 return;
            }

            if (readEventsParam.FileNotFound)
            {
                Debug.Assert(null == readEventsParam.StreamReader, "StreamReader should remain unset if file is not found.");
                return;
            }

            // Read and process events from the file
            try
            {
                // Check the DCA version to make sure we can parse this file
                int version;
                if (false == this.CanParseFile(readEventsParam.StreamReader, fileName, out version))
                {
                    return;
                }

                // Upload the ETW events that we just retrieved
                this.TraceSource.WriteInfo(
                    this.LogSourceId,
                    "Starting delivery of ETW events from file {0} ....",
                    fileName);

                LineParam lineParam = new LineParam();
                lineParam.Reader = readEventsParam.StreamReader;
                for (;;)
                {
                    // Read an event from the file
                    try
                    {
                        ReadLineFromFile(lineParam);
                    }
                    catch (Exception e)
                    {
                        this.TraceSource.WriteExceptionAsError(
                            this.LogSourceId,
                            e,
                            "Failed to read event from file {0}.",
                            fileName);
                        break;
                    }

                    if (null == lineParam.Line)
                    {
                        // End of file reached
                        break;
                    }

                    DecodedEtwEvent etwEventInfo = new DecodedEtwEvent();
                    string nodeUniqueEventId = null;
                    if (false == this.ParseEventInfo(fileName, version, lineParam.Line, ref etwEventInfo, ref nodeUniqueEventId))
                    {
                        // Couldn't parse this event, so skip it and continue with the
                        // remaining events.
                        continue;
                    }

                    // Deliver the event to the consumer
                    this.eventSink.OnEtwEventAvailable(etwEventInfo, nodeUniqueEventId);
                    this.perfHelper.EventDeliveredToConsumer();

                    // If the consumer has asked for the event delivery period to 
                    // be aborted, then do so immediately.
                    if (this.eventDeliveryPeriodAborted)
                    {
                        this.TraceSource.WriteInfo(
                            this.LogSourceId,
                            "The event delivery pass is being aborted. Therefore, no more events will be read from file {0}.",
                            fileName);
                        break;
                    }
                    
                    // If we are in the process of stopping, then don't process 
                    // any more events
                    if (this.Stopping)
                    {
                        this.TraceSource.WriteInfo(
                            this.LogSourceId,
                            "The consumer is being stopped. Therefore, no more events will be read from file {0}.",
                            fileName);
                        break;
                    }
                }

                this.TraceSource.WriteInfo(
                    this.LogSourceId,
                    "Finished delivery of ETW events from file {0}.",
                    fileName);
            }
            finally
            {
                readEventsParam.StreamReader.Dispose();
            }
        }

        private void DeliverEventsToConsumer(object state)
        {
            this.perfHelper.EventDeliveryPassBegin();

            this.eventDeliveryPeriodAborted = false;

            // Perform any tasks necessary at the beginning of the event delivery pass
            this.eventSink.OnEtwEventDeliveryStart();

            // Get the files from the buffered event directory
            // We ignore files that are so old that they need to be deleted
            DateTime cutoffTime = DateTime.UtcNow.Add(-this.eventDeletionAge);
            
            DirectoryInfo dirInfo = new DirectoryInfo(this.etwEventCache);
            FileInfo[] eventFilesInfo = dirInfo.GetFiles(CacheFileNameSearchPattern)
                                        .Where(file => file.LastWriteTimeUtc.CompareTo(cutoffTime) > 0)
                                        .ToArray();

            // Sort the files such that the file with the most recent last-write
            // time comes first. We'll process files in that order so that in 
            // case of huge backlogs the most recent (and hence likely to be 
            // most interesting) traces are processed first.
            Array.Sort(eventFilesInfo, CompareFileLastWriteTimes);

            // Process each of the files
            int filesProcessed = 0;
            DateTime processingEndTime = DateTime.Now.Add(this.eventDeliveryPassLength);
            List<string> filesToDelete = new List<string>();
            foreach (FileInfo eventFileInfo in eventFilesInfo)
            {
                // Process events from the current file
                string eventFile = eventFileInfo.FullName;
                this.ProcessEventsFromFile(eventFile);
                filesProcessed++;

                // If the event delivery pass is being aborted, then don't process
                // any more files. Also, don't delete the current file because 
                // its processing may have been interrupted. This file will be 
                // processed again in the next pass.
                if (this.eventDeliveryPeriodAborted)
                {
                    this.TraceSource.WriteInfo(
                        this.LogSourceId,
                        "The event delivery pass is being aborted. Therefore, no more files in the buffered event directory will be processed.");
                    break;
                }
                
                // If we are in the process of stopping, then don't process any 
                // more files. Also, don't delete the current file because its 
                // processing may have been interrupted. This file will be 
                // processed again when we are restarted.
                if (this.Stopping)
                {
                    this.TraceSource.WriteInfo(
                        this.LogSourceId,
                        "The consumer is being stopped. Therefore, no more files in the buffered event directory will be processed.");
                    break;
                }

                // Add the file to the list of files that we delete at the end
                // of the event delivery pass
                filesToDelete.Add(eventFile);
                
                if (0 > DateTime.Compare(processingEndTime, DateTime.Now))
                {
                    this.TraceSource.WriteInfo(
                        this.LogSourceId,
                        "Due to time limit on backlog processing, no more files in the buffered event directory will be processed in this pass.");
                    break;
                }
            }
            
            // Perform any tasks necessary at the end of the ETL processing pass
            this.eventSink.OnEtwEventDeliveryStop();

            // Delete the files that we successfully processed in this pass
            foreach (string fileToDelete in filesToDelete)
            {
                try
                {
                    Utility.PerformIOWithRetries(
                        ctx =>
                        {
                            string fileName = ctx;
                            FabricFile.Delete(fileName);
                        },
                        fileToDelete);
                }
                catch (Exception e)
                {
                    this.TraceSource.WriteExceptionAsError(
                        this.LogSourceId,
                        e,
                        "Failed to delete file {0}.",
                        fileToDelete);
                }
            }

            // Write performance-related information.
            this.perfHelper.RecordEventDeliveryBacklog(eventFilesInfo.Length - filesProcessed);
            this.perfHelper.EventDeliveryPassEnd();

            // Schedule the next pass
            this.eventDeliveryTimer.Start();
        }

        private void DeleteOldLogsHandler(object state)
        {
            this.TraceSource.WriteInfo(
                this.LogSourceId,
                "Starting deletion of old logs in directory {0} ...",
                this.etwEventCache);

            // Figure out the timestamp before which all files will be deleted
            DateTime cutoffTime = DateTime.UtcNow.Add(-this.eventDeletionAge);

            // Delete the files that are old enough to be deleted
            Utility.DeleteOldFilesFromFolder(
                this.LogSourceId,
                this.etwEventCache,
                null,
                cutoffTime,
                () => { return this.Stopping; },
                false);

            this.TraceSource.WriteInfo(
                this.LogSourceId,
                "Finished deletion of old logs in directory {0}.",
                this.etwEventCache);

            // Schedule the next pass
            this.oldLogDeletionTimer.Start();
        }

        private class ReadEventsFromFileParam
        {
            internal string FileName { get; set; }

            internal StreamReader StreamReader { get; set; }

            internal bool FileNotFound { get; set; }
        }

        private class LineParam
        {
            internal StreamReader Reader { get; set; }

            internal string Line { get; set; }
        }
    }
}