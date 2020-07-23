// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Reader
{
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    /// <summary>
    /// Contract for Trace Dispatcher
    /// </summary>
    public interface ITraceRecordDispatcher
    {
        /// <summary>
        /// Register a new Trace with the Dispatcher
        /// </summary>
        /// <param name="record"></param>
        /// <param name="token"></param>
        Task RegisterKnownTraceAsync(TraceRecord record, CancellationToken token);

        /// <summary>
        /// Unregister a Trace with Dispatcher
        /// </summary>
        /// <param name="record"></param>
        /// <param name="token"></param>
        Task UnregisterKnownTraceAsync(TraceRecord record, CancellationToken token);

        /// <summary>
        /// Dispatch a Trace Record.
        /// </summary>
        /// <param name="record"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task DispatchAsync(TraceRecord record, CancellationToken token);
    }
}