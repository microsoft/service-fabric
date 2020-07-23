// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.TraceAccessLayer.StoreConnection
{
    using System.Security;
    using ClusterAnalysis.Common.Log;
    using ClusterAnalysis.Common.Util;

    /// <summary>
    /// Encapsulates information on Azure store account
    /// </summary>
    public class AzureTraceStoreConnectionInformation : TraceStoreConnectionInformation
    {
        public AzureTraceStoreConnectionInformation(
            string accountName,
            SecureString accountKey,
            string tableNamePrefix,
            string blobContainer,
            string deploymentId,
            ILogProvider logProvider)
        {
            Assert.IsNotEmptyOrNull(accountName, "accountName != null/empty");
            Assert.IsNotEmptyOrNull(tableNamePrefix, "tableNamePrefix != null/empty");
            Assert.IsNotNull(accountKey, "accountKey != null");
            Assert.IsNotNull(logProvider, "logProvider != null");

            this.AccountName = accountName;
            this.AccountKey = accountKey;
            this.TableNamePrefix = tableNamePrefix;
            this.BlobContainerName = blobContainer;
            this.DeploymentId = deploymentId;
        }

        /// <summary>
        /// Account Name
        /// </summary>
        internal string AccountName { get; private set; }

        /// <summary>
        /// Azure store account key
        /// </summary>
        internal SecureString AccountKey { get; private set; }

        /// <summary>
        /// Azure blob container name
        /// </summary>
        internal string BlobContainerName { get; private set; }

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