// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.TraceAccessLayer.StoreReaders.QueryStoreReader
{
    using ClusterAnalysis.Common.Log;
    using ClusterAnalysis.TraceAccessLayer.StoreConnection;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Parsers;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Reader;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Parsers;

    public class LocalQueryStoreReader : LocalStoreReader
    {
        private const string SubFolder = "QueryTraces";

        public LocalQueryStoreReader(ILogProvider logProvider, LocalTraceStoreConnectionInformation localConnectionInfo) : base(
            logProvider,
            localConnectionInfo)
        {
        }

        protected override ITraceRecordParser GetParser(TraceRecordSession traceSession)
        {
            return new QueryStoreTraceRecordParser(traceSession);
        }

        protected override string GetRelativePathUnderLogRoot()
        {
            return SubFolder;
        }
    }
}