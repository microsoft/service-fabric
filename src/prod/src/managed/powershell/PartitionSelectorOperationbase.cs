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

    public abstract class PartitionSelectorOperationBase : CommonCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "PartitionId")]
        public Guid PartitionId
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "PartitionId")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNameRandomPartition")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionSingleton")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionNamed")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionUniformedInt")]
        public Uri ServiceName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "ServiceNamePartitionSingleton")]
        public SwitchParameter PartitionKindSingleton
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "ServiceNamePartitionUniformedInt")]
        public SwitchParameter PartitionKindUniformInt64
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "ServiceNamePartitionNamed")]
        public SwitchParameter PartitionKindNamed
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionNamed")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "ServiceNamePartitionUniformedInt")]
        public string PartitionKey
        {
            get;
            set;
        }

        protected PartitionSelector GetPartitionSelector()
        {
            if (this.ParameterSetName == "PartitionId")
            {
                return PartitionSelector.PartitionIdOf(this.ServiceName, this.PartitionId);
            }
            else
            {
                switch (this.ParameterSetName)
                {
                case "ServiceNameRandomPartition":
                    return PartitionSelector.RandomOf(this.ServiceName);

                case "ServiceNamePartitionSingleton":
                    return PartitionSelector.SingletonOf(this.ServiceName);

                case "ServiceNamePartitionNamed":
                    return PartitionSelector.PartitionKeyOf(this.ServiceName, this.PartitionKey);

                case "ServiceNamePartitionUniformedInt":
                    long partitionKeyLong;
                    if (!long.TryParse(this.PartitionKey, out partitionKeyLong))
                    {
                        throw new ArgumentException(StringResources.Error_InvalidPartitionKey);
                    }

                    return PartitionSelector.PartitionKeyOf(this.ServiceName, partitionKeyLong);

                default:
                    throw new ArgumentException(StringResources.Error_CouldNotParsePartitionSelector);
                }
            }
        }
    }
}