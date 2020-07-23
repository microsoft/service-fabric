// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Reader
{
    using System;
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    /// <summary>
    /// Base Implementation of a Reader
    /// </summary>
    /// TODO: Add more details like Time Taken, Traces Lost etc.
    public abstract class TraceRecordSession : ITraceRecordReader, ITraceRecordDispatcher
    {
        public const int MinimumVersionSupported = 1;

        private SemaphoreSlim singleAccessSemaphoreSlim = new SemaphoreSlim(1);

        private long traceReadCounter;

        // Even though we have a semaphore, we are still using Concurrent Dictionary so that
        // operation at the Dictionary level are thread-safe without lock. This is required 
        // to have an efficient implementation of Lookup routine.
        protected ConcurrentDictionary<TraceIdentity, TraceRecord> TraceIdentifierAndTraceTemplateMap;

        protected TraceRecordSession()
        {
            this.TraceIdentifierAndTraceTemplateMap = new ConcurrentDictionary<TraceIdentity, TraceRecord>();
        }

        /// <summary>
        /// Number of traces Read till now.
        /// </summary>
        public long TraceReadCount
        {
            get { return this.traceReadCounter; }
        }

        /// <summary>
        /// Gets if server side filtering of trace records is available.
        /// </summary>
        public abstract bool IsServerSideFilteringAvailable { get; }

        /// <inheritdoc />
        public async Task RegisterKnownTraceAsync(TraceRecord record, CancellationToken token)
        {
            if (record == null)
            {
                throw new ArgumentNullException("record");
            }

            var identifier = record.Identity;

            await this.singleAccessSemaphoreSlim.WaitAsync(token).ConfigureAwait(false);

            try
            {
                var mapEntry = this.TraceIdentifierAndTraceTemplateMap.GetOrAdd(identifier, record);

                // No existing entry and the one we have added is the first one.
                if (mapEntry.Target == record.Target)
                {
                    return;
                }

                // Few records already existed and in this case and we go the the end of the chain and insert the record there.
                // This ensures that callbacks are invoked in the order of subscription.
                while (mapEntry.Next != null)
                {
                    mapEntry = mapEntry.Next;
                }

                mapEntry.Next = record;
            }
            finally
            {
                this.singleAccessSemaphoreSlim.Release();
            }
        }

        /// <inheritdoc />
        public async Task UnregisterKnownTraceAsync(TraceRecord record, CancellationToken token)
        {
            if (record == null)
            {
                throw new ArgumentNullException("record");
            }

            var identity = record.Identity;

            await this.singleAccessSemaphoreSlim.WaitAsync(token).ConfigureAwait(false);
            try
            {
                TraceRecord existingRecord;
                if (!this.TraceIdentifierAndTraceTemplateMap.TryGetValue(identity, out existingRecord))
                {
                    throw new Exception(string.Format(CultureInfo.InvariantCulture, "Record : {0} never Registered", record.GetType()));
                }

                if (existingRecord.Target == record.Target)
                {
                    if (existingRecord.Next == null)
                    {
                        if (!this.TraceIdentifierAndTraceTemplateMap.TryRemove(identity, out existingRecord))
                        {
                            throw new Exception(string.Format(CultureInfo.InvariantCulture, "Failed to Remove Record {0}", record.GetType()));
                        }

                        return;
                    }

                    if (!this.TraceIdentifierAndTraceTemplateMap.TryUpdate(identity, existingRecord.Next, existingRecord))
                    {
                        throw new Exception(string.Format(CultureInfo.InvariantCulture, "Failed to Update Record {0}", record.GetType()));
                    }

                    return;
                }

                bool found = false;
                var previousRecord = existingRecord;
                var currentRecord = existingRecord.Next;
                while (currentRecord != null)
                {
                    if (currentRecord.Target == record.Target)
                    {
                        previousRecord.Next = currentRecord.Next;
                        found = true;
                        break;
                    }

                    previousRecord = currentRecord;
                    currentRecord = currentRecord.Next;
                }

                if (!found)
                {
                    throw new Exception(string.Format(
                        CultureInfo.InvariantCulture,
                        "Record of Type {0} with Dispatch Target {1} never registered",
                        existingRecord.GetType(),
                        record.Target));
                }
            }
            finally
            {
                this.singleAccessSemaphoreSlim.Release();
            }
        }

        /// <inheritdoc />
        public async Task DispatchAsync(TraceRecord record, CancellationToken token)
        {
            if (record == null)
            {
                throw new ArgumentNullException("record");
            }

            if (record.Target != null)
            {
                await record.DispatchAsync(token).ConfigureAwait(false);
            }

            // TODO : Do dispatching in Batches over a separate Task
            var currentRecord = record.Next;
            while (currentRecord != null)
            {
                // Remember that in our map, we only maintain the template of TraceRecord. They don't have full details (only have information on
                // what trace this is, and where should it be dispatched to, when a Record for this template shows up.
                // The TraceRecord object we receive when dispatch is called has all the data inside it. Now that we have dispatched it,
                // if there are more in the list, we go through them one by one, fill the details, and dispatch them.
                // Also note, we don't do a deep copy while dispatching. If the caller needs to keep the object around, they will 
                // have to call Clone on it.
                if (currentRecord.Target != null)
                {
                    currentRecord.Level = record.Level;
                    currentRecord.TracingProcessId = record.TracingProcessId;
                    currentRecord.ThreadId = record.ThreadId;
                    currentRecord.TimeStamp = record.TimeStamp;
                    currentRecord.PropertyValueReader = record.PropertyValueReader;

                    await currentRecord.DispatchAsync(token).ConfigureAwait(false);
                    currentRecord.PropertyValueReader = null;
                }

                currentRecord = currentRecord.Next;
            }
        }

        /// <inheritdoc />
        public abstract Task StartReadingAsync(DateTimeOffset startTime, DateTimeOffset endTime, CancellationToken token);

        public abstract Task StartReadingAsync(IList<TraceRecordFilter> filters, DateTimeOffset startTime, DateTimeOffset endTime, CancellationToken token);

        /// <inheritdoc />
        /// <remarks>TODO: Likley not needed. Will simply expose a task that can be awaited till done.</remarks>
        public event Action Completed;

        /// <summary>
        /// Called when processing is complete. 
        /// </summary>
        protected void OnCompleted()
        {
            var completed = this.Completed;
            this.Completed = null;
            completed?.Invoke();
        }

        /// <summary>
        /// Lookup a Trace to see if someone has registered an interest in it.
        /// </summary>
        /// <remarks>
        /// This routine will be called for each Trace read and needs to be very fast/efficient.
        /// As a result, we can't do any complicated processing or take locks.
        /// </remarks>
        /// <param name="identity"></param>
        /// <returns></returns>
        protected TraceRecord Lookup(TraceIdentity identity)
        {
            this.traceReadCounter++;
            TraceRecord record;
            return this.TraceIdentifierAndTraceTemplateMap.TryGetValue(identity, out record) ? record : null;
        }
    }
}