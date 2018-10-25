// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca.EtlConsumerHelper 
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common.Tracing;
    using System.Fabric.Strings;

    // This class implements the generic logic to process ETW traces, independent
    // of the destination to which they are being sent
    internal abstract class EventProcessor
    {
        // A batch of ETW events that have been read by the DCA and are queued  
        // up for filtering
        private PendingEtwEvents pendingEtwEvents;

        protected EventProcessor(ITraceEventSourceFactory traceEventSourceFactory, string logSourceId)
        {
            this.TraceSource = traceEventSourceFactory.CreateTraceEventSource(FabricEvents.Tasks.FabricDCA);
            this.LogSourceId = logSourceId;
            this.Stopping = false;
            this.Filter = null;
            this.FilterLock = new object();
            this.EtlProducerLock = new object();
            
            // Initialize pending events 
            this.pendingEtwEvents.EventList = new Queue<DecodedEventWrapper>();
            this.ResetPendingEvents();
        }

        // Object used for tracing
        protected FabricEvents.ExtensionsEvents TraceSource { get; private set; }

        // Tag used to represent the source of the log message
        protected string LogSourceId { get; private set; }

        // Lock used to guard access to filter
        protected object FilterLock { get; private set; }

        // Lock used to guard access to ETL producer interface
        protected object EtlProducerLock { get; private set; }

        // Whether or not the we are in the process of stopping
        protected bool Stopping { get; set; }

        // The filter which will check which events need to be included
        protected EtwTraceFilter Filter { get; set; }

        // ETL producer interface
        protected IEtlProducer EtlProducer { get; set; }

        // Whether or not we are processing Windows Fabric ETW events
        protected bool IsProcessingWindowsFabricEtlFiles { get; set; }

        protected abstract void ProcessTraceEvent(DecodedEventWrapper eventWrapper);

        protected void QueueEtwTrace(DecodedEventWrapper e)
        {
            // Events are always processed in batches where each batch contains
            // events that have the same timestamp. The reason is as follows.
            //
            // The last batch of events from an active ETL file (i.e. an ETL 
            // file that ETW is still writing to) might be read more than once.
            // This is because we don't know whether more events will get 
            // written to that ETL file with the exact same timestamp after the
            // current pass. Therefore, we make note of that timestamp and 
            // retrieve events from that timestamp in the next pass too. As a 
            // result, some of those events might get read more than once.
            // Because of this, special handling is required for the last batch  
            // of events in an active ETL file. Therefore, we process ETW events
            // in batches, where a batch is defined as a set of events bearing 
            // the same timestamp.

            // Check if we should process the ETW events that we have batched up
            // so far. 
            if (this.ShouldProcessPendingEtwTraces(e.Timestamp))
            {
                this.ProcessPendingEtwTraces();
            }

            // Add new ETW event to queue
            this.AddToPendingEtwTraces(e);
        }

        protected void FlushPendingEvents(bool isActiveEtl, string currentBookmark, string nextBookmark)
        {
            // Check if we need to process the list of pending ETW events
            if (ShouldProcessPendingEtwTracesOnEtlFileReadEnd(
                    isActiveEtl,
                    currentBookmark,
                    nextBookmark))
            {
                // Process the list of pending ETW events
                this.ProcessPendingEtwTraces();
            }
            else
            {
                // Clear the list of pending ETW events
                this.pendingEtwEvents.EventList.Clear();
            }

            // Reset pending events list
            this.ResetPendingEvents();
        }

        protected bool FiltersAllowEventInclusion(DecodedEventWrapper e)
        {
            lock (this.FilterLock)
            {
                if (null == this.Filter)
                {
                    // If the filter object is null, then the event gets included.
                    return true;
                }

                if (this.Filter.IsWindowsFabricEventFilter)
                {
                    if (this.IsProcessingWindowsFabricEtlFiles)
                    {
                        return this.Filter.ShouldIncludeEvent(e);
                    }

                    return true;
                }

                return this.Filter.ShouldIncludeEvent(e);
            }
        }

        private static bool ShouldProcessPendingEtwTracesOnEtlFileReadEnd(bool isActiveEtl, string currentBookmark, string nextBookmark)
        {
            // If this is not an active ETL file (i.e. not an ETL file that ETW 
            // is currently writing to), then we should always process the 
            // pending ETW traces on file close. This is because we know that no
            // more events will be added to the file.
            if (false == isActiveEtl)
            {
                return true;
            }

            // If this an active ETL file, then the pending ETW events will be
            // sent to us again during the next pass. Therefore, we should 
            // process them in the current pass only if we can guarantee that 
            // the processing would be idempotent, i.e. if we process them now 
            // and process them again in the next pass, the end result should be
            // the same.
            //
            // We can guarantee idempotence only if we know that bookmark for 
            // this ETL file in the next pass will be the same as the bookmark
            // of this ETL file in the current pass. If the bookmark were to 
            // change, the event will get processed again in the next pass with
            // a different bookmark. This results in duplicate events being 
            // processed and therefore breaks idempotence.
            if (currentBookmark.Equals(nextBookmark))
            {
                // Bookmark in the next pass will be the same as the bookmark
                // in the current pass. We can guarantee idempotence.
                return true;
            }

            // Bookmark in the next pass will be different from that in the
            // current pass. Processing of pending events will not be
            // idempotent, so don't process them.
            return false;
        }
        
        private void ResetPendingEvents()
        {
            System.Fabric.Interop.Utility.ReleaseAssert(0 == this.pendingEtwEvents.EventList.Count, StringResources.DCAError_NonZeroPendingEventCount);
            this.pendingEtwEvents.Timestamp = DateTime.MinValue;
        }
        
        private bool ShouldProcessPendingEtwTraces(DateTime newEventTimestamp)
        {
            if (this.pendingEtwEvents.Timestamp < newEventTimestamp)
            {
                // The timestamp for the event that we just received is later
                // than that of the pending events. This means that this event
                // belongs to the next batch. It also means that we can process
                // the previous batch because we won't be getting any more 
                // events for it.
                return true;
            }

            // We shouldn't receive an event with an earlier timestamp than
            // the events that we've already received;
            // Prior stage (EtlProcessor etc.) is responsible for streamlining any out of order events
            System.Fabric.Interop.Utility.ReleaseAssert(
                this.pendingEtwEvents.Timestamp == newEventTimestamp, 
                StringResources.Error_InvalidTimestamp);

            // The timestamp for the event that we just received is the same
            // as the timestamp for the pending events that we have. So the
            // new event belongs to the same batch as the pending events. We
            // might get more events for this batch, so we should not 
            // process the pending events yet.
            return false;
        }
        
        private void ProcessPendingEtwTraces()
        {
            while (this.pendingEtwEvents.EventList.Count > 0)
            {
                // Remove an event
                DecodedEventWrapper e = this.pendingEtwEvents.EventList.Dequeue(); 

                // Process the event
                this.ProcessTraceEvent(e);
            }
        }

        private void AddToPendingEtwTraces(DecodedEventWrapper e)
        {
            this.pendingEtwEvents.Timestamp = e.Timestamp;

            // Assign a timestamp differentiator to the event in order to distinguish
            // it from other events in the same ETL file that also bear the same 
            // timestamp. Timestamp differentiator values start at 0 for each 
            // timestamp and are incremented by 1 for each event bearing that 
            // timestamp.
            e.TimestampDifferentiator = this.pendingEtwEvents.EventList.Count;

            this.pendingEtwEvents.EventList.Enqueue(e);
        }

        // This structure represents a queue of pending ETW events
        private struct PendingEtwEvents
        {
            internal DateTime Timestamp;
            internal Queue<DecodedEventWrapper> EventList;
        }
    }
}