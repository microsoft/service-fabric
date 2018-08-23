// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca.EtlConsumerHelper
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Globalization;
    using System.IO;
    using Tools.EtlReader;

    /// <summary>
    /// This class implements the logic to write filtered ETW traces to an in-memory buffer. 
    /// Note that it derives from the event processor class.
    /// </summary>
    internal class EtlToInMemoryBufferWriter : EventProcessor, IEtlInMemorySink, ITraceOutputWriter
    {
        #region Constants

        internal const string BootstrapTracesFolderName = "Bootstrap";
        internal const string FabricTracesFolderName = "Fabric";
        internal const string LeaseTracesFolderName = "Lease";

        #endregion

        #region Private fields

        // Fabric node ID
        private readonly string fabricNodeId;
#if !DotNetCoreClr
        // Helper object to log performance related information
        private readonly EtlToInMemoryBufferPerformance perfHelper;
#endif
        /// <summary>
        /// This directory contains actual bootstrap logs.
        /// Fabric and lease logs are processed in-memory
        /// without persisting to disk.
        /// </summary>
        private readonly string etwLogDirName;

        /// <summary>
        /// Set by consumer when it needs compressed data
        /// </summary>
        private readonly bool compressionEnabled;

        /// <summary>
        /// In-memory consumer instance
        /// </summary>
        private readonly IDcaInMemoryConsumer inMemoryConsumer;

        /// <summary>
        /// This object handles the buffering of filtered traces
        /// in memory so that consumers can consume directly,
        /// without persisting to disk.
        /// </summary>
        private InMemorySink inMemorySink;

        // Whether or not the object has been disposed
        private bool disposed;

        /// <summary>
        /// The index up to which we have already processed events.
        /// These events would have been processed in previous ETL
        /// read passes and we can ignore these events.
        /// </summary>
        private EventIndex maxIndexAlreadyProcessed;

        /// <summary>
        /// The index of the last event we processed within the current source 
        /// ETL file.
        /// </summary>
        private EventIndex lastEventIndex;

        /// <summary>
        /// Trace sub folder corresponding to the ETL file currently being
        /// processed. This is used to figure out where to write bookmarks
        /// for the current processing cycle.
        /// </summary>
        private string traceFileSubFolder;

        #endregion

        #region Public Constructors

        internal EtlToInMemoryBufferWriter(
            ITraceEventSourceFactory traceEventSourceFactory,
            string logSourceId,
            string fabricNodeId,
            string etwLogDirName,
            bool compressionEnabled,
            IDcaInMemoryConsumer inMemoryConsumer)
            : base(traceEventSourceFactory, logSourceId)
        {
            this.fabricNodeId = fabricNodeId;
            this.etwLogDirName = etwLogDirName;
            this.compressionEnabled = compressionEnabled;
            this.inMemoryConsumer = inMemoryConsumer;
#if !DotNetCoreClr
            this.perfHelper = new EtlToInMemoryBufferPerformance(this.TraceSource, this.LogSourceId);
#endif            
        }

        #endregion

        public bool NeedsDecodedEvent
        {
            get
            {
                return true;
            }
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }

            this.disposed = true;

            GC.SuppressFinalize(this);
        }

        public void SetEtwManifestCache(ManifestCache manifestCache)
        {
            // We ignore the manifest cache because we don't decode the events
            // ourselves. Instead, we rely on the producer to decode the events
            // for us.
        }

        public void OnEtwEventProcessingPeriodStart()
        {
#if !DotNetCoreClr
            this.perfHelper.EtlReadPassBegin();
#endif
        }

        public void OnEtwEventProcessingPeriodStop()
        {
#if !DotNetCoreClr
            this.perfHelper.EtlReadPassEnd();
#endif
        }

        public void OnEtwEventsAvailable(IEnumerable<EventRecord> etwEvents)
        {
            // We are not interested in processing raw ETW events
        }

        public void OnEtwEventsAvailable(IEnumerable<DecodedEtwEvent> etwEvents)
        {
            foreach (var producerEvent in etwEvents)
            {
                this.QueueEtwTrace(new DecodedEventWrapper(producerEvent));
            }
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

        public void OnBootstrapTraceFileScanStart()
        {
        }

        public void OnBootstrapTraceFileScanStop()
        {
        }

        public bool OnBootstrapTraceFileAvailable(string fileName)
        {
            // Copy the bootstrap trace file to the ETW directory
            // Add the fabric node ID as a prefix, so that we can later associate
            // the traces with the Fabric node ID.
            string bootstrapTraceFileNameWithoutPath = string.Format(
                                                           CultureInfo.InvariantCulture,
                                                           "{0}_{1}",
                                                           this.fabricNodeId,
                                                           Path.GetFileName(fileName));
            string bootstrapTraceDestinationFileName = Path.Combine(
                                                           this.etwLogDirName,
                                                           BootstrapTracesFolderName,
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
                    bootstrapTraceDestinationFileName);
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

        internal void SetEtlProducer(IEtlProducer etlProducer)
        {
            lock (this.EtlProducerLock)
            {
                this.EtlProducer = etlProducer;
                if (null != this.EtlProducer)
                {
                    this.IsProcessingWindowsFabricEtlFiles = this.EtlProducer.IsProcessingWindowsFabricEtlFiles();
                    if (this.IsProcessingWindowsFabricEtlFiles)
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
            // Trace file subfolder to write bookmarks to
            this.traceFileSubFolder = this.GetTraceFileSubFolder(fileName);

            // Reset the last event index
            this.lastEventIndex.Set(DateTime.MinValue, -1);

            // Get the index up to which we have already processed.
            this.maxIndexAlreadyProcessed = this.GetMaxIndexAlreadyProcessed(this.traceFileSubFolder);

            // Create a new in-memory sink.
            this.inMemorySink = new InMemorySink(
                                    this.TraceSource,
                                    this.LogSourceId,
                                    this.traceFileSubFolder,
                                    this.inMemoryConsumer.ConsumerProcessTraceEventAction);

            // Signal start of processing to consumer
            this.inMemoryConsumer.OnProcessingPeriodStart(fileName, isActiveEtl, this.traceFileSubFolder);
            
            this.TraceSource.WriteInfo(
                this.LogSourceId,
                "Filtered traces from {0} will be written to an in-memory buffer.",
                fileName);
        }

        internal void OnEtlFileReadStop(string fileName, bool isActiveEtl, string currentBookmark, string nextBookmark, bool fileReadAborted)
        {
            if (false == fileReadAborted)
            {
                // Flush pending events
                this.FlushPendingEvents(isActiveEtl, currentBookmark, nextBookmark);
            }

            // Mark completion on the in-memory sink since we are done sending events to it
            this.inMemorySink.CompleteAdding();

            // Wait for the consumer to finish processing 
            this.inMemorySink.WaitForConsumerProcessingToComplete();

            // Signal end of processing to consumer
            this.inMemoryConsumer.OnProcessingPeriodStop(fileName, isActiveEtl, this.traceFileSubFolder);
        }

        protected override void ProcessTraceEvent(DecodedEventWrapper eventWrapper)
        {
#if !DotNetCoreClr
            this.perfHelper.EtwEventProcessed();
#endif
            if (null == this.inMemorySink)
            {
                // An error occurred during the creation of the sink. 
                // So just return immediately without doing anything.
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
               (this.compressionEnabled || this.FiltersAllowEventInclusion(eventWrapper)))
            {
                // According to the filters, the event should be included.
                // Write the event to the in-memory buffer.
                try
                {
                    this.inMemorySink.Add(eventWrapper);
#if !DotNetCoreClr
                    this.perfHelper.EtwEventWrittenToInMemoryBuffer();
#endif
                }
                catch (Exception e)
                {
                    // Log an error and move on
                    this.TraceSource.WriteExceptionAsError(
                        this.LogSourceId,
                        e,
                        "Failed to write filtered event {0} to in-memory buffer.",
                        eventWrapper.StringRepresentation);
                }
            }
        }

        private EventIndex GetMaxIndexAlreadyProcessed(string traceFileSubFolder)
        {
            return this.inMemoryConsumer.MaxIndexAlreadyProcessed(traceFileSubFolder);
        }

        private void CreateWindowsFabricTraceSubFolders()
        {
            string subFolder = Path.Combine(this.etwLogDirName, BootstrapTracesFolderName);
            FabricDirectory.CreateDirectory(subFolder);
            subFolder = Path.Combine(this.etwLogDirName, FabricTracesFolderName);
            FabricDirectory.CreateDirectory(subFolder);
            subFolder = Path.Combine(this.etwLogDirName, LeaseTracesFolderName);
            FabricDirectory.CreateDirectory(subFolder);
        }

        private string GetTraceFileSubFolder(string etlfileName)
        {
            string traceFileSubFolder = string.Empty;

            if (this.IsProcessingWindowsFabricEtlFiles)
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
            }

            return traceFileSubFolder;
        }
    }
}