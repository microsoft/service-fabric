// ----------------------------------------------------------------------
//  <copyright file="ITraceRecordReceiver.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Core
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    public interface ITraceRecordReceiver<T> : IEquatable<ITraceRecordReceiver<T>> where T : TraceRecord
    {
        Task ReceiveAsync(T traceRecord, CancellationToken token);
    }
}