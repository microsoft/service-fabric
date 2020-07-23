// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.TraceAccessLayer.StoreConnection
{
    using System;
    using System.Globalization;
    using ClusterAnalysis.Common.Log;
    using ClusterAnalysis.Common.Util;
    using ClusterAnalysis.TraceAccessLayer.EventStoreReader;
    using ClusterAnalysis.TraceAccessLayer.QueryStoreReader;
    using ClusterAnalysis.TraceAccessLayer.StoreReaders.EventStoreReader;
    using ClusterAnalysis.TraceAccessLayer.StoreReaders.QueryStoreReader;

    /// <summary>
    /// Container of connection to event store
    /// </summary>
    public class TraceStoreConnection
    {
        private ILogger logger;

        public TraceStoreConnection(TraceStoreConnectionInformation traceConnectionInformation, ILogProvider logProvider)
        {
            Assert.IsNotNull(traceConnectionInformation, "Trace connection information can't be null");
            Assert.IsNotNull(logProvider, "Log Provider can't be null");
            this.logger = logProvider.CreateLoggerInstance("TraceStoreConnection");
            this.InitializeStore(traceConnectionInformation, logProvider);
        }

        /// <summary>
        /// Access to structured store
        /// </summary>
        public ITraceStoreReader EventStoreReader { get; private set; }

        public ITraceStoreReader QueryStoreReader { get; private set; }

        private void InitializeStore(TraceStoreConnectionInformation connInfo, ILogProvider logProvider)
        {
            if (connInfo is LocalTraceStoreConnectionInformation)
            {
                this.InitializeLocalStore((LocalTraceStoreConnectionInformation)connInfo, logProvider);
                return;
            }

            if (connInfo is AzureTraceStoreConnectionInformation)
            {
                this.InitializeAzureStore((AzureTraceStoreConnectionInformation)connInfo, logProvider);
                return;
            }

            if (connInfo is AzureTableStoreStorageAccessInformation)
            {
                this.InitializeAzureStore((AzureTableStoreStorageAccessInformation)connInfo, logProvider);
                return;
            }

            throw new NotSupportedException(
                string.Format(CultureInfo.InvariantCulture, "Connection Information of Type '{0}' is Not supported", connInfo.GetType()));
        }

        private void InitializeLocalStore(LocalTraceStoreConnectionInformation localEventStoreConnectionInformation, ILogProvider logProvider)
        {
            this.QueryStoreReader = new LocalQueryStoreReader(logProvider, localEventStoreConnectionInformation);
            this.EventStoreReader = new LocalEventStoreReader(logProvider, localEventStoreConnectionInformation);
        }

        private void InitializeAzureStore(AzureTraceStoreConnectionInformation azureConnInfo, ILogProvider logProvider)
        {
            this.QueryStoreReader = new AzureTableQueryStoreReader(logProvider, azureConnInfo);
            this.EventStoreReader = new AzureTableEventStoreReader(logProvider, azureConnInfo);
        }

        private void InitializeAzureStore(AzureTableStoreStorageAccessInformation azureStorageAccessInfo, ILogProvider logProvider)
        {
            this.QueryStoreReader = new AzureTableQueryStoreReader(logProvider, azureStorageAccessInfo);
            this.EventStoreReader = new AzureTableEventStoreReader(logProvider, azureStorageAccessInfo);
        }
    }
}