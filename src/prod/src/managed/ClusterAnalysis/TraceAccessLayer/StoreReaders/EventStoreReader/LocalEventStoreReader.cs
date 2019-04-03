// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.TraceAccessLayer.StoreReaders.EventStoreReader
{
    using ClusterAnalysis.Common.Log;
    using ClusterAnalysis.TraceAccessLayer.StoreConnection;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Parsers;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Reader;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Parsers;

    public class LocalEventStoreReader : LocalStoreReader
    {
        private const string SubFolder = "OperationalTraces";

        public LocalEventStoreReader(ILogProvider logProvider, LocalTraceStoreConnectionInformation localConnectionInfo) : base(
            logProvider,
            localConnectionInfo)
        {
        }

        protected override ITraceRecordParser GetParser(TraceRecordSession traceSession)
        {
            return new EventStoreTraceRecordParser(traceSession);
        }

        protected override string GetRelativePathUnderLogRoot()
        {
            return SubFolder;
        }
    }
}