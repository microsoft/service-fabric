// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.TraceAccessLayer.StoreReaders
{
    using System;
    using ClusterAnalysis.Common;
    using ClusterAnalysis.Common.Log;
    using ClusterAnalysis.Common.Util;
    using ClusterAnalysis.TraceAccessLayer.StoreConnection;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Parsers;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Reader;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.AzureTableReader;

    /// <summary>
    /// Base class for all Azure store Readers
    /// </summary>
    public abstract class AzureStoreReader : TraceStoreReader
    {
        /// <summary>
        /// Azure connection information.
        /// </summary>
        private AzureTraceStoreConnectionInformation azureConnectionInformation;

        private AzureTableStoreStorageAccessInformation azureTableAccessInfo;

        protected AzureStoreReader(ILogProvider logProvider, AzureTraceStoreConnectionInformation connectionInformation) : base(logProvider)
        {
            Assert.IsNotNull(connectionInformation, "connectionInformation != null");
            this.azureConnectionInformation = connectionInformation;
        }

        protected AzureStoreReader(ILogProvider logProvider, AzureTableStoreStorageAccessInformation azureTableStorageAccessInfo) : base(logProvider)
        {
            Assert.IsNotNull(azureTableStorageAccessInfo, "azureTableStorageAccessInfo != null");
            this.azureTableAccessInfo = azureTableStorageAccessInfo;
        }

        /// <inheritdoc />
        public override bool IsPropertyLevelFilteringSupported()
        {
            return true;
        }

        protected override TraceRecordSession GetTraceRecordSession(Duration duration)
        {
            if (this.azureTableAccessInfo != null)
            {
                return new ServiceFabricAzureTableRecordSession(
                    this.azureTableAccessInfo.AzureTableStorageAccess,
                    this.azureTableAccessInfo.TableNamePrefix,
                    this.azureTableAccessInfo.DeploymentId);
            }

            return new ServiceFabricAzureTableRecordSession(
                this.azureConnectionInformation.AccountName,
                HandyUtil.ConvertToUnsecureString(this.azureConnectionInformation.AccountKey),
                true,
                this.azureConnectionInformation.TableNamePrefix,
                this.azureConnectionInformation.DeploymentId);
        }
    }
}