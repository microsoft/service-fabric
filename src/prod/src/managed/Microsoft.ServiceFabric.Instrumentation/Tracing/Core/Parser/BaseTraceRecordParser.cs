// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Parsers
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Reader;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    /// <summary>
    /// Base Implementation of Trace Record Parser. All inbuilt traces will inherit from this class.
    /// </summary>
    public abstract class BaseTraceRecordParser : ITraceRecordParser
    {
        protected BaseTraceRecordParser(TraceRecordSession traceSession)
        {
            this.TraceSession = traceSession ?? throw new ArgumentNullException("traceSession");
        }

        /// <inheritdoc />
        public TraceRecordSession TraceSession { get; }

        /// <inheritdoc />
        public IEnumerable<TraceIdentity> GetKnownTraces()
        {
            return this.GetKnownTraceRecords().Select(item => new TraceIdentity(item.TraceId, item.TaskName));
        }

        /// <inheritdoc />
        public bool IsTypeSupported(Type type)
        {
            return this.GetKnownTraceRecords().Any(item => item.GetType() == type);
        }

        public Task SubscribeAsync(Type type, Func<TraceRecord, CancellationToken, Task> callback, CancellationToken token)
        {
            var traceRecordObj = (TraceRecord)Activator.CreateInstance(type, callback);

            var allknownTraces = this.GetKnownTraces();
            if (!allknownTraces.Contains(traceRecordObj.Identity))
            {
                throw new NotSupportedException(
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "Parser of type: {0} doesn't not support Trace Type: {1} and Identity: {2}",
                        this.GetType(),
                        traceRecordObj.GetType(),
                        traceRecordObj.Identity));
            }

            return this.TraceSession.RegisterKnownTraceAsync(traceRecordObj, token);
        }

        public Task UnSubscribeAsync(Type type, Func<TraceRecord, CancellationToken, Task> callback, CancellationToken token)
        {
            if (callback == null)
            {
                throw new ArgumentNullException("callback");
            }

            var traceRecordObj = (TraceRecord)Activator.CreateInstance(type, callback);
            return this.TraceSession.UnregisterKnownTraceAsync(traceRecordObj, token);
        }

        /// <inheritdoc />
        public Task SubscribeAsync<T>(Func<T, CancellationToken, Task> callback, CancellationToken token) where T : TraceRecord
        {
            if (callback == null)
            {
                throw new ArgumentNullException("callback");
            }

            var traceRecordObj = (TraceRecord)Activator.CreateInstance(typeof(T), callback);

            var allknownTraces = this.GetKnownTraces();
            if (!allknownTraces.Contains(traceRecordObj.Identity))
            {
                throw new NotSupportedException(
                    string.Format(
                        CultureInfo.InvariantCulture,
                        "Parser of type: {0} doesn't not support Trace Type: {1} and Identity: {2}",
                        this.GetType(),
                        traceRecordObj.GetType(),
                        traceRecordObj.Identity));
            }

            return this.TraceSession.RegisterKnownTraceAsync(traceRecordObj, token);
        }

        /// <inheritdoc />
        public Task UnSubscribeAsync<T>(Func<T, CancellationToken, Task> callback, CancellationToken token)
        {
            if (callback == null)
            {
                throw new ArgumentNullException("callback");
            }

            var traceRecordObj = (TraceRecord)Activator.CreateInstance(typeof(T), callback);
            return this.TraceSession.UnregisterKnownTraceAsync(traceRecordObj, token);
        }

        /// <summary>
        ///  Gets all the known trace record objects
        /// </summary>
        /// <returns></returns>
        protected abstract IEnumerable<TraceRecord> GetKnownTraceRecords();
    }
}