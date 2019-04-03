// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.TraceAccessLayer
{
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using ClusterAnalysis.Common;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    public interface ITraceStoreReader
    {
        /// <summary>
        /// Read trace records in specified duration and matching the given filter
        /// </summary>
        /// <param name="duration"></param>
        /// <param name="filter"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<IEnumerable<TraceRecord>> ReadTraceRecordsAsync(Duration duration, ReadFilter filter, CancellationToken token);

        /// <summary>
        /// Gets if Property level filter is supported by the Reader.
        /// </summary>
        /// <returns></returns>
        bool IsPropertyLevelFilteringSupported();

        /// <summary>
        /// Create a Bounded trace reader
        /// </summary>
        /// <param name="duration"></param>
        /// <returns></returns>
        BoundedTraceStoreReader WithinBound(Duration duration);

        /// <summary>
        /// Create a Bounded Trace reader with default duration.
        /// </summary>
        /// <returns></returns>
        BoundedTraceStoreReader WithinBound();
    }
}