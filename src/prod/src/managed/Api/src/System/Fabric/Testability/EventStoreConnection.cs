// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability
{
    using System.Fabric.Strings;
    using System.Linq;
    using TraceDiagnostic.Store;

    /// <summary>
    /// Container of connection to event store
    /// </summary>
    internal class EventStoreConnection
    {
        private const int MinimumLengthToProvide = 16;

        private const string DeploymentTableName = "DeploymentIds";

        public EventStoreConnection(EventStoreConnectionInformation info)
        {
            this.InitializeStore(info);
        }

        public IEventStore Store { get; private set; }

        /// <summary>
        /// Validates deployment Id, in case it is needed
        /// </summary>
        /// <param name="credentials">Azure/Mds Table credentials</param>
        /// <param name="tableNamePrefix">Table name prefix</param>
        /// <param name="providedId">Caller provided deployment Id</param>
        /// <param name="validatedId">Validated deployment Id</param>
        private static void ValidateDeploymentId(
            IStorageConnection credentials,
            string tableNamePrefix,
            string providedId,
            out string validatedId)
        {
            validatedId = null;

            string deploymentIdTableName = tableNamePrefix + DeploymentTableName;
            bool deploymentTableHasEntities;

            bool deploymentTableExists = credentials.ValidateTable(deploymentIdTableName, out deploymentTableHasEntities);

            if (!deploymentTableExists || !deploymentTableHasEntities)
            {
                return;
            }

            var throwAway = new AzureTableEventStore(credentials, tableNamePrefix, providedId);

            var deployments = throwAway.GetDeploymentList();

            var ids = deployments.Select(x => x.DeploymentId).ToList();

            if (providedId != null)
            {
                if (providedId.Length < MinimumLengthToProvide)
                {
                    throw new InvalidOperationException(
                        string.Format(
                            StringResources.EventStoreError_DeploymentIdNotLongEnough, MinimumLengthToProvide));
                }

                validatedId = ids.Find(providedId.StartsWith);

                if (string.IsNullOrEmpty(validatedId))
                {
                    throw new InvalidOperationException(StringResources.EventStoreError_DeploymentIdDoesNotMatch);
                }
            }
            else
            {
                if (ids.Count > 1)
                {
                    throw new InvalidOperationException(StringResources.EventStoreError_AmbiguousDeploymentId);
                }

                validatedId = ids.First();
            }
        }

        private void InitializeLocalStore(LocalEventStoreConnectionInformation localInfo)
        {
            this.Store = new LocalEventStore();
        }

        private void InitializeAzureStore(AzureEventStoreConnectionInformation azureInfo)
        {
            if (azureInfo == null)
            {
                throw new InvalidOperationException(StringResources.EventStoreError_IncompleteConnectionInformation);
            }

            IStorageConnection credentials;
            string providedDeploymentId;
            string validatedDeploymentId;

            var azureTableInfo = azureInfo as AzureTableEventStoreConnectionInformation;
            if (azureTableInfo == null)
            {
                var mdsTableInfo = azureInfo as MdsTableEventStoreConnectionInformation;

                if (mdsTableInfo == null)
                {
                    throw new InvalidOperationException(StringResources.EventStoreError_UnexpectedConnectionType);
                }

                credentials = new MdsInfo(mdsTableInfo.Moniker, mdsTableInfo.InstanceUrl, null/*tableUrl*/);

                providedDeploymentId = mdsTableInfo.DeploymentId;
            }
            else
            {
                credentials = new AccountAndKey(azureTableInfo.AccountName, azureTableInfo.AccountKey, true/*useHttps*/);

                providedDeploymentId = azureTableInfo.DeploymentId;
            }

            ValidateDeploymentId(credentials, azureInfo.TableNamePrefix, providedDeploymentId, out validatedDeploymentId);

            this.Store = new AzureTableEventStore(credentials, azureInfo.TableNamePrefix, validatedDeploymentId);
        }

        private void InitializeStore(EventStoreConnectionInformation connInfo)
        {
            var localInfo = connInfo as LocalEventStoreConnectionInformation;
            if (localInfo == null)
            {
                this.InitializeAzureStore(connInfo as AzureEventStoreConnectionInformation); 
            }
            else
            {
                this.InitializeLocalStore(localInfo);
            }
        }
    }
}

#pragma warning restore 1591