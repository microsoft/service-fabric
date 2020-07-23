// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.TraceAccessLayer.QueryStoreReader
{
    using ClusterAnalysis.Common.Log;
    using ClusterAnalysis.TraceAccessLayer.StoreConnection;
    using ClusterAnalysis.TraceAccessLayer.StoreReaders;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Parsers;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Reader;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Parsers;

    public class AzureTableQueryStoreReader : AzureStoreReader
    {
        public AzureTableQueryStoreReader(ILogProvider logProvider, AzureTraceStoreConnectionInformation connectionInformation) : base(
            logProvider,
            connectionInformation)
        {
        }

        public AzureTableQueryStoreReader(ILogProvider logProvider, AzureTableStoreStorageAccessInformation connectionInformation) : base(
            logProvider,
            connectionInformation)
        {
        }

        protected override ITraceRecordParser GetParser(TraceRecordSession traceSession)
        {
            return new QueryStoreTraceRecordParser(traceSession);
        }
    }
}