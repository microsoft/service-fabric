// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Parsers
{
    using System.Collections.Generic;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Parsers;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Reader;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Hosting;

    /// <summary>
    /// Parser for all of the Hosting Traces
    /// </summary>
    public class HostingTraceRecordParser : BaseTraceRecordParser
    {
        private IList<TraceRecord> knownTraceRecords;

        /// <summary>
        /// Create an instance of <see cref="HostingTraceRecordParser"/>
        /// </summary>
        /// <param name="traceSession"></param>
        public HostingTraceRecordParser(TraceRecordSession traceSession) : base(traceSession)
        {
            this.knownTraceRecords = new List<TraceRecord> { new ProcessUnexpectedTerminationTraceRecord(null) };
        }

        protected override IEnumerable<TraceRecord> GetKnownTraceRecords()
        {
            return this.knownTraceRecords;
        }
    }
}