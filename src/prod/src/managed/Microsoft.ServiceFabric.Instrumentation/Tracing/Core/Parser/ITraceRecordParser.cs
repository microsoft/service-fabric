// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Parsers
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Reader;

    /// <summary>
    /// Contract for a parser
    /// </summary>
    public interface ITraceRecordParser
    {
        /// <summary>
        /// Get the Session the parser is attached to.
        /// </summary>
        TraceRecordSession TraceSession { get; }

        /// <summary>
        /// Gets a collection of traces that this parser can parse.
        /// </summary>
        /// <returns></returns>
        IEnumerable<TraceIdentity> GetKnownTraces();

        /// <summary>
        /// Is a particular type of trace supported by this parser
        /// </summary>
        /// <param name="type"></param>
        /// <returns></returns>
        bool IsTypeSupported(Type type);

        /// <summary>
        /// Subscribe to a particular trace record
        /// </summary>
        /// <param name="type"></param>
        /// <param name="callback">Action that should be invoked when the specific trace is seen</param>
        /// <param name="token"></param>
        /// <returns>Subscription PartitionId</returns>
        Task SubscribeAsync(Type type, Func<TraceRecord, CancellationToken, Task> callback, CancellationToken token);

        /// <summary>
        /// Subscribe to a particular trace record
        /// </summary>
        /// <param name="type"></param>
        /// <param name="callback">Action that should be invoked when the specific trace is seen</param>
        /// <param name="token"></param>
        /// <returns>Subscription PartitionId</returns>
        Task UnSubscribeAsync(Type type, Func<TraceRecord, CancellationToken, Task> callback, CancellationToken token);

        /// <summary>
        /// Subscribe to a particular trace record
        /// </summary>
        /// <param name="callback">Action that should be invoked when the specific trace is seen</param>
        /// <param name="token"></param>
        /// <returns>Subscription PartitionId</returns>
        Task SubscribeAsync<T>(Func<T, CancellationToken, Task> callback, CancellationToken token) where T : TraceRecord;

        /// <summary>
        /// Remove a subscription to a particular trace record.
        /// </summary>
        /// <param name="callback">The action that was registered to be invoked when subscription was added</param>
        /// <param name="token"></param>
        Task UnSubscribeAsync<T>(Func<T, CancellationToken, Task> callback, CancellationToken token);
    }
}