// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric;
    using System.Fabric.Strings;
    using System.Management.Automation;

    public abstract class ReplicaSelectorOperationBase : CommonCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "PartitionId")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "PartitionIdReplicaPrimary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "PartitionIdReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "PartitionIdReplicaId")]
        public Guid PartitionId
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "PartitionId")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "PartitionIdReplicaPrimary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "PartitionIdReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "PartitionIdReplicaId")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceName")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionSingleton")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionNamed")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionUniformedInt")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionSingletonReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionNamedReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionUniformedIntReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionSingletonReplicaPrimary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionNamedReplicaPrimary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionUniformedIntReplicaPrimary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionSingletonReplicaId")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionNamedReplicaId")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionUniformedIntReplicaId")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNameReplicaPrimary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNameReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNameReplicaId")]
        public Uri ServiceName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "ServiceNamePartitionSingleton")]
        [Parameter(Mandatory = true, ParameterSetName = "ServiceNamePartitionSingletonReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ParameterSetName = "ServiceNamePartitionSingletonReplicaPrimary")]
        [Parameter(Mandatory = true, ParameterSetName = "ServiceNamePartitionSingletonReplicaId")]
        public SwitchParameter PartitionKindSingleton
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "ServiceNamePartitionUniformedInt")]
        [Parameter(Mandatory = true, ParameterSetName = "ServiceNamePartitionUniformedIntReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ParameterSetName = "ServiceNamePartitionUniformedIntReplicaPrimary")]
        [Parameter(Mandatory = true, ParameterSetName = "ServiceNamePartitionUniformedIntReplicaId")]
        public SwitchParameter PartitionKindUniformInt64
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "ServiceNamePartitionNamed")]
        [Parameter(Mandatory = true, ParameterSetName = "ServiceNamePartitionNamedReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ParameterSetName = "ServiceNamePartitionNamedReplicaPrimary")]
        [Parameter(Mandatory = true, ParameterSetName = "ServiceNamePartitionNamedReplicaId")]
        public SwitchParameter PartitionKindNamed
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionNamed")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionNamedReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionNamedReplicaPrimary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionNamedReplicaId")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionUniformedInt")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionUniformedIntReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionUniformedIntReplicaPrimary")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionUniformedIntReplicaId")]
        public string PartitionKey
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "ServiceNamePartitionSingletonReplicaPrimary")]
        [Parameter(Mandatory = true, ParameterSetName = "ServiceNamePartitionNamedReplicaPrimary")]
        [Parameter(Mandatory = true, ParameterSetName = "ServiceNamePartitionUniformedIntReplicaPrimary")]
        [Parameter(Mandatory = true, ParameterSetName = "PartitionIdReplicaPrimary")]
        [Parameter(Mandatory = true, ParameterSetName = "ServiceNameReplicaPrimary")]
        public SwitchParameter ReplicaKindPrimary
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "PartitionIdReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ParameterSetName = "ServiceNamePartitionSingletonReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ParameterSetName = "ServiceNamePartitionNamedReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ParameterSetName = "ServiceNamePartitionUniformedIntReplicaRandomSecondary")]
        [Parameter(Mandatory = true, ParameterSetName = "ServiceNameReplicaRandomSecondary")]
        public SwitchParameter ReplicaKindRandomSecondary
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "PartitionIdReplicaId")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionSingletonReplicaId")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionNamedReplicaId")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionUniformedIntReplicaId")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNameReplicaId")]
        public long? ReplicaOrInstanceId
        {
            get;
            set;
        }

        internal static ReplicaSelector GetReplicaSelector(string partitionSetName, Guid partitionId, Uri serviceName, string partitionKey, long? replicaOrInstanceId)
        {
            ReplicaSelector replicaSelector = null;
            PartitionSelector partitionSelector = null;
            if (partitionSetName.Contains("PartitionId"))
            {
                partitionSelector = PartitionSelector.PartitionIdOf(serviceName, partitionId);
            }
            else
            {
                if (partitionSetName.Contains("PartitionSingleton"))
                {
                    partitionSelector = PartitionSelector.SingletonOf(serviceName);
                }
                else if (partitionSetName.Contains("PartitionNamed"))
                {
                    partitionSelector = PartitionSelector.PartitionKeyOf(serviceName, partitionKey);
                }
                else if (partitionSetName.Contains("PartitionUniformedInt"))
                {
                    long partitionKeyLong;
                    if (!long.TryParse(partitionKey, out partitionKeyLong))
                    {
                        throw new ArgumentException(StringResources.Error_InvalidPartitionKey);
                    }

                    partitionSelector = PartitionSelector.PartitionKeyOf(serviceName, partitionKeyLong);
                }
                else if (!partitionSetName.Contains("Partition"))
                {
                    partitionSelector = PartitionSelector.RandomOf(serviceName);
                }
            }

            if (partitionSelector == null)
            {
                throw new ArgumentException(StringResources.Error_CouldNotParsePartitionSelector);
            }

            if (partitionSetName.Contains("ReplicaPrimary"))
            {
                replicaSelector = ReplicaSelector.PrimaryOf(partitionSelector);
            }
            else if (partitionSetName.Contains("ReplicaRandomSecondary"))
            {
                replicaSelector = ReplicaSelector.RandomSecondaryOf(partitionSelector);
            }
            else if (partitionSetName.Contains("ReplicaId"))
            {
                replicaSelector = ReplicaSelector.ReplicaIdOf(partitionSelector, replicaOrInstanceId ?? 0);
            }
            else if (!partitionSetName.Contains("Replica"))
            {
                replicaSelector = ReplicaSelector.RandomOf(partitionSelector);
            }

            if (replicaSelector == null)
            {
                throw new ArgumentException(StringResources.Error_CouldNotParseReplicaSelector);
            }

            return replicaSelector;
        }

        protected ReplicaSelector GetReplicaSelector()
        {
            return ReplicaSelectorOperationBase.GetReplicaSelector(
                       this.ParameterSetName,
                       this.PartitionId,
                       this.ServiceName,
                       this.PartitionKey,
                       this.ReplicaOrInstanceId);
        }
    }
}