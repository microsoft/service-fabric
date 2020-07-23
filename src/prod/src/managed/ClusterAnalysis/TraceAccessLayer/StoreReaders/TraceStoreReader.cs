// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.TraceAccessLayer.StoreReaders
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using ClusterAnalysis.Common;
    using ClusterAnalysis.Common.Log;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Parsers;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Reader;

    /// <summary>
    /// Base class for all Readers.
    /// </summary>
    public abstract class TraceStoreReader : ITraceStoreReader
    {
        public static readonly Task CompletedTask = Task.FromResult(true);

        protected TraceStoreReader(ILogProvider logProvider)
        {
            Assert.IsNotNull(logProvider, "logProvider != null");
            this.Logger = logProvider.CreateLoggerInstance(this.GetType().Name);
        }

        protected ILogger Logger { get; }

        /// <inheritdoc />
        public abstract bool IsPropertyLevelFilteringSupported();

        /// <inheritdoc />
        public async Task<IEnumerable<TraceRecord>> ReadTraceRecordsAsync(Duration duration, ReadFilter filter, CancellationToken token)
        {
            var parser = this.GetParser(this.GetTraceRecordSession(duration));

            var typesInFilter = filter.TraceRecordFilters.Select(item => item.RecordType).ToList();

            var allRecords = new List<TraceRecord>();
            Func<TraceRecord, CancellationToken, Task> traceReceiver = (record, cancellationToken) =>
            {
                if (filter.TraceFilter != null)
                {
                    if (filter.TraceFilter(record))
                    {
                        return CompletedTask;
                    }
                }

                allRecords.Add(record.Clone());
                return CompletedTask;
            };

            foreach (var type in typesInFilter)
            {
                await parser.SubscribeAsync(type, traceReceiver, token).ConfigureAwait(false);
            }

            if (parser.TraceSession.IsServerSideFilteringAvailable)
            {
                await parser.TraceSession.StartReadingAsync(filter.TraceRecordFilters, duration.StartTime, duration.EndTime, token).ConfigureAwait(false);
            }
            else
            {
                await parser.TraceSession.StartReadingAsync(duration.StartTime, duration.EndTime, token).ConfigureAwait(false);
            }

            return allRecords;
        }

        /// <inheritdoc />
        public BoundedTraceStoreReader WithinBound(Duration duration)
        {
            return new BoundedTraceStoreReader(this, duration.StartTime, duration.EndTime);
        }

        /// <inheritdoc />
        public BoundedTraceStoreReader WithinBound()
        {
            return new BoundedTraceStoreReader(this, DateTime.UtcNow - TimeSpan.FromDays(1), DateTime.UtcNow);
        }

        /// <summary>
        /// Get the Parser Object
        /// </summary>
        /// <param name="traceSession"></param>
        /// <returns></returns>
        protected abstract ITraceRecordParser GetParser(TraceRecordSession traceSession);

        protected abstract TraceRecordSession GetTraceRecordSession(Duration duration);
    }
}