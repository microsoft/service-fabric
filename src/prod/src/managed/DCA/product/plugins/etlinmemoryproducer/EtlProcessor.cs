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
    using System.Globalization;
    using System.IO;
    using System.Linq;
    using System.Threading;

    using Tools.EtlReader;

    // This class implements the logic to read events from an ETL file
    internal class EtlProcessor
    {
        // Constants
        private const string ManifestFilePattern = "*.man";

        // Repository of Windows Fabric ETW manifests
        private static readonly WFManifestRepository WinfabManifestRepository = new WFManifestRepository();

        // Object used for tracing
        private readonly FabricEvents.ExtensionsEvents traceSource;

        // Tag used to represent the source of the log message
        private readonly string logSourceId;

        // Whether we are processing Windows Fabric events or application events
        private readonly bool processingWinFabEvents;

        // Windows Fabric manifests that we know about
        private readonly WinFabricManifestManager winFabManifestMgr;

        private readonly ManifestCache etwManifestCache = new ManifestCache(Utility.DcaWorkFolder);

        // ETL in-memory sinks implemented by the EtlToInMemoryBufferWriter class
        private readonly ReadOnlyCollection<EtlToInMemoryBufferWriter> etlToInMemoryBufferWriters;

        // ETL in-memory sinks
        private readonly ReadOnlyCollection<IEtlInMemorySink> sinks;

        private readonly EtlInMemoryPerformance perfHelper;
        private readonly string markerFileDirectory;
        private readonly WinFabricEtlType windowsFabricEtlType;
        private readonly ITraceFileEventReaderFactory traceFileEventReaderFactory;

        // Timestamp of the latest event read by DCA
        private DateTime currentTimestamp;

        // Last timestamp we know all events have been processed up to.
        private DateTime lastCompletedTimestamp;

        // Whether or not any events have been read yet from the current ETL file
        private bool eventsReadFromCurrentEtl;

        internal EtlProcessor(
            bool loadWinFabManifests,
            bool dynamicWinFabManifestLoad,
            string markerFileDirectory,
            WinFabricEtlType winFabricEtlType,
            FabricEvents.ExtensionsEvents traceSource,
            string logSourceId,
            EtlInMemoryPerformance perfHelper,
            IEnumerable<IEtlInMemorySink> sinks,
            IEnumerable<EtlToInMemoryBufferWriter> etlToInMemoryBufferWriters,
            ITraceFileEventReaderFactory traceFileEventReaderFactory)
        {
            this.traceSource = traceSource;
            this.logSourceId = logSourceId;
            this.processingWinFabEvents = loadWinFabManifests;
            this.markerFileDirectory = markerFileDirectory;
            this.windowsFabricEtlType = winFabricEtlType;
            this.sinks = sinks.ToList().AsReadOnly();
            this.etlToInMemoryBufferWriters = etlToInMemoryBufferWriters.ToList().AsReadOnly();
            this.traceFileEventReaderFactory = traceFileEventReaderFactory;
            this.perfHelper = perfHelper;

            if (loadWinFabManifests)
            {
                if (dynamicWinFabManifestLoad)
                {
                    // We are going to be loading manifests dynamically based on the version
                    // in the ETL file. Initialize the manifest manager object to manage this
                    // for us.
                    this.winFabManifestMgr = this.InitializeWinFabManifestManager();
                }
                else
                {
                    // We'll load the default manifests now and just use them throughout
                    this.LoadDefaultManifests();
                }
            }

            // Provide the sinks a reference to the manifest cache.
            this.sinks.ForEach(sink => sink.SetEtwManifestCache(this.etwManifestCache));
        }

        internal void EnsureCorrectWinFabManifestVersionLoaded(string etlFileName)
        {
            try
            {
                bool exactMatch;
                if (this.winFabManifestMgr != null)
                {
                    this.winFabManifestMgr.EnsureCorrectWinFabManifestVersionLoaded(etlFileName, out exactMatch);
                    if (false == exactMatch)
                    {
                        this.traceSource.WriteWarning(
                            this.logSourceId,
                            "Exact manifest match not found for ETL file {0}. We will make a best-effort attempt to decode it with the manifests available.",
                            etlFileName);
                    }
                }
                else
                {
                    this.traceSource.WriteError(
                    this.logSourceId,
                    "Manifest manager has not been initialized");
                }
            }
            catch (Exception e)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Exception encountered while identifying the Windows Fabric manifests available. Exception information: {0}",
                    e);
            }
        }

        internal void ProcessActiveEtlFile(FileInfo etlFile, DateTime lastEndTime, CancellationToken cancellationToken, out DateTime newEndTime)
        {
            // When processing an active ETL file (i.e. an ETL file which ETW is
            // still writing to), the following needs to be kept in mind:
            //
            // - We need to process this ETL file again in our next pass 
            //   (because more events can be added to it after this pass). So 
            //   the file should *not* be moved to the archived traces folder 
            //   when we are done with it in this pass.
            //
            // - When we read the last event that is available from this ETL 
            //   file in this pass, we *cannot* safely assume that more events 
            //   with the exact same timestamp will not get added to this ETL 
            //   file in the future. Therefore, the ending timestamp for this 
            //   pass (which determines the starting timestamp for the next 
            //   pass) should be computed such that we handle this corner case,
            //   i.e. if more events with the same timestamp do get added, we 
            //   should be able to pick them up in our next pass.
            this.lastCompletedTimestamp = lastEndTime;
            this.currentTimestamp = DateTime.MinValue;

            // Raise an event to indicate that we're starting on a new ETL file
            this.OnEtlFileReadStart(etlFile.Name, true, lastEndTime);

            // Read events from the ETL file
            DateTime startTime = this.GetStartTime(lastEndTime);

            // We read ETW events only up to the point where we are fairly sure they
            // have been flushed to disk. To do this, we multiple the flush interval
            // by a constant (to give us some extra cushion) and then subtract the 
            // result from current time. 
            const double EndTimeOffsetSeconds = -1 * EtlInMemoryProducerConstants.DefaultTraceSessionFlushIntervalSeconds *
                                                EtlInMemoryProducerConstants.TraceSessionFlushIntervalMultiplier;
            DateTime endTime = DateTime.UtcNow.AddSeconds(EndTimeOffsetSeconds);

            // Avoid getting into retries when reading the ETL file too often.
            if (startTime > endTime)
            {
                newEndTime = this.lastCompletedTimestamp;
                return;
            }

            var eventReader = this.traceFileEventReaderFactory.CreateTraceFileEventReader(etlFile.FullName);

            eventReader.EventRead += (sender, args) => this.OnEventFromEtl(sender, args, cancellationToken);

            this.eventsReadFromCurrentEtl = false;
            ReadEventsParameters readParam = new ReadEventsParameters
            {
                EventReader = eventReader,
                StartTime = startTime,
                EndTime = endTime
            };
            try
            {
                Utility.PerformWithRetries(
                    ReadEventsWorker,
                    readParam,
                    new RetriableOperationExceptionHandler(this.ReadEventsExceptionHandler),
                    EtlInMemoryProducerConstants.MethodExecutionInitialRetryIntervalMs,
                    EtlInMemoryProducerConstants.MethodExecutionMaxRetryCount,
                    EtlInMemoryProducerConstants.MethodExecutionMaxRetryIntervalMs);
            }
            catch (Exception e)
            {
                this.traceSource.WriteExceptionAsError(
                    this.logSourceId,
                    e,
                    "Failed to read some or all of the events from ETL file {0}.",
                    etlFile.FullName);
            }
            finally
            {
                eventReader.Dispose();
            }

            // The end time for this pass is chosen as the latest timestamp for 
            // which we know for sure that we will not receive any more events. 
            // 
            // It is possible that we already read events with a later timestamp
            // than the chosen end time for this pass. This happens when we 
            // cannot be sure that we won't receive any more events with that 
            // later timestamp after we complete this pass. By choosing an 
            // earlier end time, we enable ourselves to pick up additional 
            // events with that later timestamp in our next pass. However, this
            // also means that some events that we already read in this pass may
            // be read again in the next pass. This is okay because processing 
            // of events is designed to be idempotent.
            newEndTime = this.lastCompletedTimestamp;

            // We'll need to read from this ETL file again in the next pass. 
            // Don't move it to the archived traces folder yet.

            // Raise an event to indicate that we're done with this ETL file
            // (for this pass)
            this.OnEtlFileReadStop(etlFile.Name, true, lastEndTime, this.lastCompletedTimestamp, cancellationToken.IsCancellationRequested);
        }

        internal void ProcessInactiveEtlFile(FileInfo etlFile, DateTime lastEndTime, CancellationToken cancellationToken, out DateTime newEndTime)
        {
            this.lastCompletedTimestamp = lastEndTime;
            this.currentTimestamp = DateTime.MinValue;

            // Raise an event to indicate that we're starting on a new ETL file
            this.OnEtlFileReadStart(etlFile.Name, false, lastEndTime);

            // Read events from the ETL file
            DateTime startTime = this.GetStartTime(lastEndTime);
            DateTime endTime = DateTime.MaxValue;

            ITraceFileEventReader eventReader;
            try
            {
                eventReader = this.traceFileEventReaderFactory.CreateTraceFileEventReader(etlFile.FullName);
            }
            catch (FileNotFoundException e)
            {
                // The file was not found, most likely because it was deleted by
                // the our old logs deletion routine.
                //
                // NOTE: The method that processes the active ETL file does not handle
                // this exception because we have logic to avoid deleting an active
                // ETL file. We don't want to delete the active ETL file because ETW is
                // still writing to it.
                this.traceSource.WriteWarning(
                    this.logSourceId,
                    "Exception occurred while trying to process ETL file {0}. The file may have been deleted. Exception information: {1}",
                    etlFile.FullName,
                    e);

                // Raise an event to indicate that we're done with this ETL file
                this.OnEtlFileReadStop(etlFile.Name, false, lastEndTime, this.lastCompletedTimestamp, true);
                newEndTime = this.lastCompletedTimestamp;
                return;
            }

            eventReader.EventRead += (sender, args) => this.OnEventFromEtl(sender, args, cancellationToken);

            this.eventsReadFromCurrentEtl = false;
            ReadEventsParameters readParam = new ReadEventsParameters()
            {
                EventReader = eventReader,
                StartTime = startTime,
                EndTime = endTime
            };
            try
            {
                var traceSessionMetadata = eventReader.ReadTraceSessionMetadata();
                FabricEvents.Events.TraceSessionStats(
                    traceSessionMetadata.TraceSessionName,
                    traceSessionMetadata.EventsLostCount,
                    traceSessionMetadata.StartTime,
                    traceSessionMetadata.EndTime);

                Utility.PerformWithRetries(
                    ReadEventsWorker,
                    readParam,
                    new RetriableOperationExceptionHandler(this.ReadEventsExceptionHandler),
                    EtlInMemoryProducerConstants.MethodExecutionInitialRetryIntervalMs,
                    EtlInMemoryProducerConstants.MethodExecutionMaxRetryCount,
                    EtlInMemoryProducerConstants.MethodExecutionMaxRetryIntervalMs);
            }
            catch (Exception e)
            {
                this.traceSource.WriteExceptionAsError(
                    this.logSourceId,
                    e,
                    "Failed to read some or all of the events from ETL file {0}.",
                    etlFile.FullName);
            }
            finally
            {
                eventReader.Dispose();
            }

            // Since events are read in chronological order maintain
            // the new end time so that next pass will calculate start
            // time correctly
            newEndTime = this.lastCompletedTimestamp;

            // Raise an event to indicate that we're done with this ETL file
            this.OnEtlFileReadStop(etlFile.Name, false, lastEndTime, this.lastCompletedTimestamp, cancellationToken.IsCancellationRequested);

            // We're done with this ETL file. Create a marker file for it.
            if (!cancellationToken.IsCancellationRequested)
            {
                string etlFileMarker = Path.Combine(
                                            this.markerFileDirectory,
                                            etlFile.Name);
                try
                {
                    Utility.PerformIOWithRetries(
                        () =>
                        {
                            // Create file
                            using (FileStream fs = FabricFile.Create(etlFileMarker))
                            {
                            }
                        },
                        EtlInMemoryProducerConstants.MethodExecutionInitialRetryIntervalMs,
                        EtlInMemoryProducerConstants.MethodExecutionMaxRetryCount,
                        EtlInMemoryProducerConstants.MethodExecutionMaxRetryIntervalMs);

                    this.traceSource.WriteInfo(
                        this.logSourceId,
                        "Marker file {0} created.",
                        etlFileMarker);
                }
                catch (Exception e)
                {
                    this.traceSource.WriteExceptionAsError(
                        this.logSourceId,
                        e,
                        "Failed to create marker file {0}.",
                        etlFileMarker);
                }
            }
            else
            {
                // The producer is being stopped. It is possible that we didn't fully
                // process this ETL file because we detected that the producer was 
                // being stopped. Therefore do not create a marker file for it in the
                // archived traces directory yet. We should process it again when the 
                // producer is restarted.
                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "The producer is being stopped. Therefore, no marker file will be created for ETL file {0} in the archived traces directory.",
                    etlFile.FullName);
            }
        }

        private static void ReadEventsWorker(ReadEventsParameters readParam)
        {
            readParam.EventReader.ReadEvents(readParam.StartTime, readParam.EndTime);
        }

        private static string GetBookmark(string fileName, DateTime endTime)
        {
            // The bookmark is of the form:
            // <ETLFileNameWithoutExtention>_<lastEndTime>
            var fileNameWithoutExt = Path.GetFileNameWithoutExtension(fileName);
            var bookmark = string.Concat(
                fileNameWithoutExt,
                "_",
                endTime.Ticks.ToString("D20", CultureInfo.InvariantCulture));
            return bookmark;
        }

        private void OnEventFromEtl(object sender, EventRecordEventArgs e, CancellationToken cancellationToken)
        {
            if (cancellationToken.IsCancellationRequested)
            {
                // We are in the process of stopping. Don't start any new work.
                return;
            }

            // At least one event has been read from the current ETL file
            this.eventsReadFromCurrentEtl = true;

            // Get the event's timestamp
            EventRecord eventRecord = e.Record;
            long eventTimestampFileTime = eventRecord.EventHeader.TimeStamp;
            DateTime eventTimestamp = DateTime.FromFileTimeUtc(eventTimestampFileTime);

            if (this.currentTimestamp == DateTime.MinValue)
            {
                // Current timestamp is not yet initialized. Initialize it now.
                this.currentTimestamp = eventTimestamp;
            }
            else
            {
                int timeComparisonResult = DateTime.Compare(
                                               this.currentTimestamp,
                                               eventTimestamp);
                if (0 > timeComparisonResult)
                {
                    // Event timestamp is different from that of the last event
                    // we received. So we shouldn't be receiving any more events
                    // with the same timestamp as that of the previous event, 
                    // i.e. we've moved forward to a new timestamp.
                    this.lastCompletedTimestamp = this.lastCompletedTimestamp > this.currentTimestamp
                        ? this.lastCompletedTimestamp : this.currentTimestamp;
                    this.currentTimestamp = eventTimestamp;
                }
                else if (0 < timeComparisonResult)
                {
                    // The event timestamp shouldn't be older than that of the
                    // previous event, i.e. we don't expect to move back in time.
                    // modifying the time of the latest event to be same as last known good event which was in order
                    eventRecord.EventHeader.TimeStamp = this.currentTimestamp.ToFileTimeUtc();

                    this.perfHelper.EtlUnorderedEventReceived();
                }

                // If eventTimestamp is same as currentTimestamp then we should process the events
                // we keep LastCompletedTimestamp as is
            }

            // Process the event
            this.OnEtwTraceAvailable(eventRecord);
        }

        private void OnEtwTraceAvailable(EventRecord eventRecord)
        {
            var decodedEvent = new DecodedEtwEvent();
            var decodingAttempted = false;
            var decodingFailed = false;

            List<EventRecord> rawEventList = null;
            List<DecodedEtwEvent> decodedEventList = null;
            foreach (var sink in this.sinks)
            {
                if (sink.NeedsDecodedEvent)
                {
                    // This consumer needs the decoded event
                    if (false == decodingAttempted)
                    {
                        // We haven't yet attempted to decode the event. Attempt
                        // it now.
                        try
                        {
                            if (false == this.DecodeEvent(eventRecord, out decodedEvent))
                            {
                                // Our attempt to decode the event failed
                                decodingFailed = true;
                            }
                        }
                        catch (Exception e)
                        {
                            this.traceSource.WriteError(
                                this.logSourceId,
                                "Exception encountered while decoding ETW event. The event will be skipped. Event ID: {0}, Task: {1}, Level: {2}. Exception information: {3}",
                                eventRecord.EventHeader.EventDescriptor.Id,
                                eventRecord.EventHeader.EventDescriptor.Task,
                                eventRecord.EventHeader.EventDescriptor.Level,
                                e);
                            decodingFailed = true;
                        }

                        if (false == decodingFailed)
                        {
                            // Our attempt to decode the event succeeded. Put it
                            // in the event list.
                            Debug.Assert(null == decodedEventList, "Decoded event list should not be initialized previously.");
                            decodedEventList = new List<DecodedEtwEvent> { decodedEvent };
                        }

                        decodingAttempted = true;
                    }

                    if (false == decodingFailed)
                    {
                        sink.OnEtwEventsAvailable(decodedEventList);
                    }
                }
                else
                {
                    // This consumer needs the raw event
                    if (null == rawEventList)
                    {
                        // Put the event in the event list
                        rawEventList = new List<EventRecord> { eventRecord };
                    }

                    sink.OnEtwEventsAvailable(rawEventList);
                }
            }

            this.perfHelper.EtwEventProcessed();
        }

        private bool DecodeEvent(EventRecord eventRecord, out DecodedEtwEvent decodedEvent)
        {
            decodedEvent = new DecodedEtwEvent { EventRecord = eventRecord };

            if (false == ManifestCache.IsStringEvent(eventRecord))
            {
                var eventDefinition = this.etwManifestCache.GetEventDefinition(
                    eventRecord);
                if (null == eventDefinition)
                {
                    if (eventRecord.EventHeader.EventDescriptor.Task != (ushort)FabricEvents.EventSourceTaskId)
                    {
                        // We couldn't decode this event. Skip it.
                        this.traceSource.WriteError(
                            this.logSourceId,
                            "Unable to decode ETW event. The event will be skipped. Event ID: {0}, Task: {1}, Level: {2}.",
                            eventRecord.EventHeader.EventDescriptor.Id,
                            eventRecord.EventHeader.EventDescriptor.Task,
                            eventRecord.EventHeader.EventDescriptor.Level);
                    }

                    return false;
                }

                if (eventDefinition.IsChildEvent)
                {
                    // Try to format the event. This causes the ETL reader to save information
                    // about this event. That information is later retrieved when the parent
                    // event is processed.
                    string childEventType;
                    string childEventText;
                    var childStringRepresentation = this.etwManifestCache.FormatEvent(
                        eventRecord,
                        out childEventType,
                        out childEventText);

                    // Only parent events can be decoded. Child events supply additional
                    // information about the parent event and cannot be decoded on their
                    // own.
                    return false;
                }

                decodedEvent.TaskName = eventDefinition.TaskName;
            }
            else
            {
                decodedEvent.TaskName = EventFormatter.StringEventTaskName;
            }

            decodedEvent.Timestamp = DateTime.FromFileTimeUtc(eventRecord.EventHeader.TimeStamp);
            decodedEvent.Level = eventRecord.EventHeader.EventDescriptor.Level;
            decodedEvent.ThreadId = eventRecord.EventHeader.ThreadId;
            decodedEvent.ProcessId = eventRecord.EventHeader.ProcessId;
            decodedEvent.StringRepresentation = this.etwManifestCache.FormatEvent(
                eventRecord,
                out decodedEvent.EventType,
                out decodedEvent.EventText,
                0);
            if (null == decodedEvent.StringRepresentation)
            {
                if (eventRecord.EventHeader.EventDescriptor.Task != (ushort)FabricEvents.EventSourceTaskId)
                {
                    // We couldn't decode this event. Skip it.
                    this.traceSource.WriteError(
                        this.logSourceId,
                        "Unable to decode ETW event. The event will be skipped. Event ID: {0}, Task: {1}, Level: {2}.",
                        eventRecord.EventHeader.EventDescriptor.Id,
                        eventRecord.EventHeader.EventDescriptor.Task,
                        eventRecord.EventHeader.EventDescriptor.Level);
                    return false;
                }
            }

            // If this is an FMM event, update the last timestamp of FMM events
            if ((WinFabricEtlType.DefaultEtl == this.windowsFabricEtlType) &&
                decodedEvent.TaskName.Equals(Utility.FmmTaskName))
            {
                Utility.LastFmmEventTimestamp = decodedEvent.Timestamp;
            }

            return true;
        }

        private DateTime GetStartTime(DateTime lastEndTime)
        {
            DateTime startTime;

            if (lastEndTime == DateTime.MinValue)
            {
                // We are processing Windows Fabric traces. Start retrieving
                // traces right from the beginning.
                startTime = DateTime.MinValue;
            }
            else
            {
                // We are processing Windows Fabric traces. Start retrieving
                // traces one tick after the timestamp where we left off previously.
                startTime = lastEndTime.AddTicks(1);
            }

            return startTime;
        }

        private RetriableOperationExceptionHandler.Response ReadEventsExceptionHandler(Exception e)
        {
            this.traceSource.WriteWarning(
                this.logSourceId,
                "Exception encountered while reading ETL file. Exception information: {0}.",
                e);
            if (false == this.eventsReadFromCurrentEtl)
            {
                // Retry
                this.traceSource.WriteWarning(
                    this.logSourceId,
                    "Read from ETL file will be retried if retry limit has not been reached.");
                return RetriableOperationExceptionHandler.Response.Retry;
            }
            else
            {
                // Abort
                // If we've already processed some events from this ETL file, retry is 
                // non-trivial. This is because we maintain state such as the timestamp 
                // of the latest event that we've read from the file so far. We would
                // need to reset all such state before we attempt to retry. In the interest
                // of keeping the implementation simple, we do not attempt a retry if we 
                // encounter a failure while we are in the middle of reading events from 
                // an ETL file. Instead, we abort the read. This means that the rest of the
                // ETL file is skipped.
                this.traceSource.WriteWarning(
                    this.logSourceId,
                    "No more events will be read from this ETL file.");
                return RetriableOperationExceptionHandler.Response.Abort;
            }
        }

        private void OnEtlFileReadStart(string fileName, bool isActiveEtl, DateTime currentEndTime)
        {
            var currentBookmark = GetBookmark(fileName, currentEndTime);

            foreach (var etlToInMemoryBufferWriter in this.etlToInMemoryBufferWriters)
            {
                etlToInMemoryBufferWriter.OnEtlFileReadStart(
                    fileName,
                    isActiveEtl,
                    currentBookmark);
            }
        }

        private void OnEtlFileReadStop(
            string fileName,
            bool isActiveEtl,
            DateTime currentEndTime,
            DateTime nextEndTime,
            bool fileReadAborted)
        {
            var currentBookmark = GetBookmark(fileName, currentEndTime);
            var nextBookmark = GetBookmark(fileName, nextEndTime);

            foreach (var etlToInMemoryBufferWriter in this.etlToInMemoryBufferWriters)
            {
                etlToInMemoryBufferWriter.OnEtlFileReadStop(
                    fileName,
                    isActiveEtl,
                    currentBookmark,
                    nextBookmark,
                    fileReadAborted);
            }
        }

        private WinFabricManifestManager InitializeWinFabManifestManager()
        {
            // Figure out which Windows Fabric manifests we have.
            // Try the repository first ...
            string manifestFileDirectory;
            try
            {
                manifestFileDirectory = EtlProcessor.WinfabManifestRepository.RepositoryPath;
            }
            catch (Exception)
            {
                // If we couldn't access the repository for any reason, fall back to
                // the manifests that we shipped with.
                string assemblyLocation = Process.GetCurrentProcess().MainModule.FileName;
                manifestFileDirectory = Path.GetDirectoryName(assemblyLocation);
            }

            string[] manifestFiles = FabricDirectory.GetFiles(manifestFileDirectory, "*.man");

            WinFabricManifestManager manifestManager = null;
            try
            {
                manifestManager = new WinFabricManifestManager(
                                               manifestFiles,
                                               this.LoadManifest,
                                               this.etwManifestCache.UnloadManifest);
            }
            catch (Exception e)
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "Exception encountered while identifying the Windows Fabric manifests available. Exception information: {0}",
                    e);
                return manifestManager;
            }

            if (!manifestManager.FabricManifests.Any())
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "No Fabric manifests were found.");
            }

            if (!manifestManager.LeaseLayerManifests.Any())
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "No lease layer manifests were found.");
            }

            if (!manifestManager.KtlManifests.Any())
            {
                this.traceSource.WriteError(
                    this.logSourceId,
                    "No KTL manifests were found.");
            }

            return manifestManager;
        }

        private void LoadDefaultManifests()
        {
            // Get the directory where the default manifests are located
            var manifestFileDirectory = Utility.DcaProgramDirectory;

            // Load the manifests
            foreach (string manifestFileName in WinFabricManifestManager.DefaultManifests)
            {
                string manifestFile = Path.Combine(
                                          manifestFileDirectory,
                                          manifestFileName);
                this.LoadManifest(manifestFile);
            }
        }

        private void LoadManifest(string manifestFileName)
        {
            try
            {
                Utility.PerformIOWithRetries(
                    this.LoadManifestWorker,
                    manifestFileName);

                this.traceSource.WriteInfo(
                    this.logSourceId,
                    "Loaded manifest {0}",
                    manifestFileName);
            }
            catch (Exception e)
            {
                this.traceSource.WriteExceptionAsError(
                    this.logSourceId,
                    e,
                    "Failed to load manifest {0}.",
                    manifestFileName);
            }
        }

        private void LoadManifestWorker(string manifestFileName)
        {
            this.etwManifestCache.LoadManifest(manifestFileName);
        }

        private class ReadEventsParameters
        {
            internal ITraceFileEventReader EventReader { get; set; }

            internal DateTime StartTime { get; set; }

            internal DateTime EndTime { get; set; }
        }
    }
}