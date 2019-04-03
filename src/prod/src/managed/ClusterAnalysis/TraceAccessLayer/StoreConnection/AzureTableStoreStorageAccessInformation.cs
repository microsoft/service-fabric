// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.TraceAccessLayer.StoreConnection
{
    using ClusterAnalysis.Common.Util;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.AzureTableReader;

    public class AzureTableStoreStorageAccessInformation : TraceStoreConnectionInformation
    {
        public AzureTableStoreStorageAccessInformation(
            IAzureTableStorageAccess azureTableAccess,
            string tableNamePrefix,
            string deploymentId)
        {
            Assert.IsNotNull(azureTableAccess, "azureTableAccess != null");
            Assert.IsNotEmptyOrNull(tableNamePrefix, "tableNamePrefix != null/empty");

            this.AzureTableStorageAccess = azureTableAccess;
            this.TableNamePrefix = tableNamePrefix;
            this.DeploymentId = deploymentId;
        }

        public IAzureTableStorageAccess AzureTableStorageAccess { get; private set; }

        /// <summary>
        /// Table name prefix
        /// </summary>
        internal string TableNamePrefix { get; private set; }

        /// <summary>
        /// Current deployment ID
        /// </summary>
        internal string DeploymentId { get; private set; }
    }
}