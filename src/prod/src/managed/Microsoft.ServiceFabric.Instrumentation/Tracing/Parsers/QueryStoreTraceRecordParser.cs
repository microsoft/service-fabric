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
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Client;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.CRM;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.Hosting;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RA;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RAP;

    /// <summary>
    /// Parser for all of the Traces in the Query Trace Store
    /// </summary>
    public class QueryStoreTraceRecordParser : BaseTraceRecordParser
    {
        private IList<TraceRecord> knownTraceRecords;

        /// <summary>
        /// Create an instance of <see cref="HostingTraceRecordParser"/>
        /// </summary>
        /// <param name="traceRecordSession"></param>
        public QueryStoreTraceRecordParser(TraceRecordSession traceRecordSession) : base(traceRecordSession)
        {
            this.knownTraceRecords = new List<TraceRecord>
            {
                new ProcessUnexpectedTerminationTraceRecord(null),
                new OperationTraceRecord(null),
                new ApplicationHostTerminatedTraceRecord(null),
                new OperationIgnoredTraceRecord(null),
                new BeginReportFaultTraceRecord(null),
                new ApiReportFaultTraceRecord(null),
                new ReportFaultTraceRecord(null),
                new ReplicaStateChangeTraceRecord(null)
            };
        }

        protected override IEnumerable<TraceRecord> GetKnownTraceRecords()
        {
            return this.knownTraceRecords;
        }
    }
}
