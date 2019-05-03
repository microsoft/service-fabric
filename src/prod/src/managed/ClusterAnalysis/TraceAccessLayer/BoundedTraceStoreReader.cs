// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.TraceAccessLayer
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    public sealed class BoundedTraceStoreReader
    {
        private static readonly TimeSpan MaxBoundInterval = TimeSpan.FromDays(3);

        private static readonly TimeSpan StartingQueryInterval = TimeSpan.FromMinutes(10);

        private ITraceStoreReader traceStoreReader;

        private DateTime boundStart;

        private DateTime boundEnd;

        internal BoundedTraceStoreReader(ITraceStoreReader reader, DateTime lowerBound, DateTime upperBound)
        {
            if (reader == null)
            {
                throw new ArgumentNullException("reader");
            }

            if (upperBound <= lowerBound)
            {
                throw new ArgumentException(string.Format("Bound Start : {0} >= Bound End: {1}", lowerBound, upperBound));
            }

            if (upperBound - lowerBound > MaxBoundInterval)
            {
                throw new ArgumentOutOfRangeException(string.Format("Max Bound supported: {0}, Requested: {1}", MaxBoundInterval, upperBound - lowerBound));
            }

            this.traceStoreReader = reader;
            this.boundStart = lowerBound;
            this.boundEnd = upperBound;
        }

        public async Task<TraceRecord> ReadForwardSingleAsync(ReadFilter filter, CancellationToken token)
        {
            return (await this.ReadForwardAsync(filter, 1, token).ConfigureAwait(false)).SingleOrDefault();
        }

        public async Task<TraceRecord> ReadBackwardSingleAsync(ReadFilter filter, CancellationToken token)
        {
            return (await this.ReadBackwardAsync(filter, 1, token).ConfigureAwait(false)).SingleOrDefault();
        }

        public Task<IEnumerable<TraceRecord>> ReadForwardAsync(ReadFilter filter, int count, CancellationToken token)
        {
            return this.ReadRecordsAsync(filter, count, true, token);
        }

        public Task<IEnumerable<TraceRecord>> ReadBackwardAsync(ReadFilter filter, int count, CancellationToken token)
        {
            return this.ReadRecordsAsync(filter, count, false, token);
        }

        private async Task<IEnumerable<TraceRecord>> ReadRecordsAsync(ReadFilter filter, int count, bool forwardDirection, CancellationToken token)
        {
            var iterationStartTime = this.boundStart;
            var iterationEndTime = this.boundEnd;
            var traceResults = new List<TraceRecord>();
            var timeSpanIncrement = StartingQueryInterval;
            while (true)
            {
                if (forwardDirection)
                {
                    iterationEndTime = iterationStartTime + timeSpanIncrement;
                    if (iterationEndTime > this.boundEnd)
                    {
                        iterationEndTime = this.boundEnd;
                    }
                }
                else
                {
                    iterationStartTime = iterationEndTime - timeSpanIncrement;
                    if (iterationStartTime < this.boundStart)
                    {
                        iterationStartTime = this.boundStart;
                    }
                }

                if (iterationStartTime >= iterationEndTime)
                {
                    break;
                }

                var results = await this.traceStoreReader.ReadTraceRecordsAsync(new Common.Duration(iterationStartTime, iterationEndTime), filter, token).ConfigureAwait(false);

                traceResults.AddRange(results);

                // We already have enough results.
                if (traceResults.Count >= count)
                {
                    break;
                }

                // Double the query interval with each iteration
                timeSpanIncrement = TimeSpan.FromTicks(timeSpanIncrement.Ticks * 2);

                if (forwardDirection)
                {
                    iterationStartTime = iterationEndTime;
                }
                else
                {
                    iterationEndTime = iterationStartTime;
                }
            }

            if (forwardDirection)
            {
                return traceResults.Take(count).OrderBy(item => item.TimeStamp);
            }
            else
            {
                return traceResults.Take(count).OrderByDescending(item => item.TimeStamp);
            }
        }
    }
}