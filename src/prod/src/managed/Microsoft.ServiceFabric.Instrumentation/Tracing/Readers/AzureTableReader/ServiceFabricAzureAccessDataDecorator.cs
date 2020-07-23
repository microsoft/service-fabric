// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.AzureTableReader
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Represents the event storage in Azure tables.
    /// </summary>
    public class ServiceFabricAzureAccessDataDecorator
    {
        /// <summary>
        /// Name of the deployment ID table (without prefix).
        /// </summary>
        public const string DeploymentIdTable = "DeploymentIds";

        /// <summary>
        /// The default table name prefix for events.
        /// </summary>
        private const string DefaultTableNamePrefix = "etwlogs";

        /// <summary>
        /// Maximum length of deployment PartitionId used.
        /// </summary>
        private const int MaxDeploymentIdLength = 16;

        /// <summary>
        /// To limit overall table name length, the prefix is limited to a max set of characters.
        /// </summary>
        private const int MaxTableNamePrefixLengthForDeploymentIdInclusion = 32;

        /// <summary>
        /// Information used to connect to Azure storage account.
        /// </summary>
        private readonly IAzureTableStorageAccess strorageTableAccess;

        /// <summary>
        /// The prefix of table names.
        /// </summary>
        private readonly string tableNamePrefix;

        /// <summary>
        /// User supplied Deployment ID
        /// </summary>
        private string userSuppliedDeploymentId;

        /// <summary>
        /// Effective deployment ID that we use.
        /// </summary>
        private string effectiveDeploymentId;

        /// <summary>
        /// Initializes a new instance of the <see cref="ServiceFabricAzureTableAccess" /> class.
        /// </summary>
        /// <param name="storageTableAccessor">Information used to connect to Azure storage account.</param>
        /// <param name="namePrefix">The prefix of table names.</param>
        /// <param name="deploymentId"></param>
        public ServiceFabricAzureAccessDataDecorator(IAzureTableStorageAccess tableAccess, string namePrefix, string deploymentId)
        {
            if (tableAccess == null)
            {
                throw new ArgumentNullException("tableAccess");
            }

            this.strorageTableAccess = tableAccess;
            this.tableNamePrefix = string.IsNullOrEmpty(namePrefix) ? DefaultTableNamePrefix : namePrefix; ;
            this.userSuppliedDeploymentId = deploymentId;
        }

        public async Task<string> DecorateTableNameAsync(string tableName, CancellationToken token)
        {
            await this.PopulateDeploymentId(token).ConfigureAwait(false);

            return this.tableNamePrefix + this.effectiveDeploymentId + tableName;
        }

        private async Task<IEnumerable<Deployment>> GetDeploymentListAsync(CancellationToken token)
        {
            string fullTableName = this.tableNamePrefix + DeploymentIdTable;
            var results = await this.strorageTableAccess.ExecuteQueryAsync(fullTableName, token).ConfigureAwait(false);

            return results.AzureTableProperties.Select(
                oneResult =>
                {
                    if (!oneResult.UserDataMap.ContainsKey("StartTime"))
                    {
                        throw new Exception("Wrong Event Type: TODO");
                    }

                    DateTimeOffset startTime;
                    if (!DateTimeOffset.TryParse(oneResult.UserDataMap["StartTime"], out startTime))
                    {
                        throw new Exception(string.Format(CultureInfo.InvariantCulture, "Invalid Start Type. Can't convert Start Time : {0} to DateTime", oneResult.UserDataMap["StartTime"]));
                    }

                    return new Deployment(oneResult.PartitionKey, startTime);
                }).OrderByDescending(item => item.StartTime);
        }

        // TODO: Refine this Logic.
        private async Task PopulateDeploymentId(CancellationToken token)
        {
            // It implies, we have already calculated effective deployment ID
            if (this.effectiveDeploymentId != null)
            {
                return;
            }

            this.effectiveDeploymentId = string.Empty;

            // This implies, we don't need to calculate. User has already specified the full table prefix.
            if (this.tableNamePrefix.Length > MaxTableNamePrefixLengthForDeploymentIdInclusion)
            {
                // The table name prefix specified by the user is too long to accomodate
                // the deployment ID.
                return;
            }

            if (this.userSuppliedDeploymentId == null)
            {
                string deploymentIdTableName = this.tableNamePrefix + DeploymentIdTable;
                var deploymentIdNeeded = await this.strorageTableAccess.DoesTableExistsAsync(deploymentIdTableName, token).ConfigureAwait(false);
                if (!deploymentIdNeeded)
                {
                    // Deployment Table doesn't Exist.
                    return;
                }

                // Get all Deployment from deployment Table
                var deployments = await this.GetDeploymentListAsync(token).ConfigureAwait(false);

                if (deployments.Count() == 1)
                {
                    this.effectiveDeploymentId = deployments.ElementAt(0).DeploymentId;
                }
                else
                {
                    throw new Exception(
                        string.Format(
                            CultureInfo.InvariantCulture,
                            "Multiple Deployment Ids : {0} Found. Please specify a Filter",
                            string.Join(",", deployments)));
                }
            }
            else
            {
                this.effectiveDeploymentId = this.userSuppliedDeploymentId;
            }

            if (this.effectiveDeploymentId.Length > MaxDeploymentIdLength)
            {
                this.effectiveDeploymentId = this.effectiveDeploymentId.Remove(MaxDeploymentIdLength);
            }
        }
    }
}