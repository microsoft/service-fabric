// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Reader
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Contract for any Reader
    /// </summary>
    public interface ITraceRecordReader
    {
        /// <summary>
        /// Start reading traces in this time range.
        /// </summary>
        /// <param name="startTime"></param>
        /// <param name="endTime"></param>
        /// <param name="token"></param>
        Task StartReadingAsync(DateTimeOffset startTime, DateTimeOffset endTime, CancellationToken token);

        /// <summary>
        /// Start reading traces in this time range and matching the specified filters.
        /// </summary>
        /// <param name="filters"></param>
        /// <param name="startTime"></param>
        /// <param name="endTime"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task StartReadingAsync(IList<TraceRecordFilter> filters, DateTimeOffset startTime, DateTimeOffset endTime, CancellationToken token);

        /// <summary>
        /// Subscribers of Completed will be called after processing is complete.
        /// </summary>
        event Action Completed;
    }
}