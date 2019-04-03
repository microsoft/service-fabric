// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Diagnostics;
    using System.Fabric;
    using System.Management.Automation;

    [Cmdlet(VerbsDiagnostic.Resolve, "ServiceFabricService", DefaultParameterSetName = "Singleton NonRefresh")]
    public sealed class ResolveService : ServiceCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Singleton ForceRefresh")]
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Singleton NonRefresh")]
        public SwitchParameter PartitionKindSingleton
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "UniformInt64 ForceRefresh")]
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "UniformInt64 NonRefresh")]
        public SwitchParameter PartitionKindUniformInt64
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Named ForceRefresh")]
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Named NonRefresh")]
        public SwitchParameter PartitionKindNamed
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 1)]
        public Uri ServiceName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 2, ValueFromPipelineByPropertyName = true, ParameterSetName = "UniformInt64 ForceRefresh")]
        [Parameter(Mandatory = true, Position = 2, ValueFromPipelineByPropertyName = true, ParameterSetName = "UniformInt64 NonRefresh")]
        [Parameter(Mandatory = true, Position = 2, ValueFromPipelineByPropertyName = true, ParameterSetName = "Named ForceRefresh")]
        [Parameter(Mandatory = true, Position = 2, ValueFromPipelineByPropertyName = true, ParameterSetName = "Named NonRefresh")]
        public string PartitionKey
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, ParameterSetName = "Singleton NonRefresh")]
        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, ParameterSetName = "UniformInt64 NonRefresh")]
        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, ParameterSetName = "Named NonRefresh")]
        public ResolvedServicePartition PreviousResult
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "Singleton ForceRefresh")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "UniformInt64 ForceRefresh")]
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "Named ForceRefresh")]
        public SwitchParameter ForceRefresh
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            ResolvedServicePartition result;
            if (this.ForceRefresh)
            {
                var timeout = this.GetTimeout();
                var stopWatch = Stopwatch.StartNew();
                ResolvedServicePartition previous = null;

                while (true)
                {
                    result = this.ResolveService(
                                 this.PartitionKindUniformInt64,
                                 this.PartitionKindNamed,
                                 this.PartitionKindSingleton,
                                 this.ServiceName,
                                 this.PartitionKey,
                                 previous,
                                 timeout - stopWatch.Elapsed);

                    if (previous != null && result.CompareVersion(previous) == 0)
                    {
                        break;
                    }

                    previous = result;
                }
            }
            else
            {
                result = this.ResolveService(
                             this.PartitionKindUniformInt64,
                             this.PartitionKindNamed,
                             this.PartitionKindSingleton,
                             this.ServiceName,
                             this.PartitionKey,
                             this.PreviousResult,
                             this.GetTimeout());
            }

            this.WriteObject(this.FormatOutput(result));
        }

        protected override object FormatOutput(object output)
        {
            var result = new PSObject(output);

            if (output is ResolvedServicePartition)
            {
                var partitionResult = output as ResolvedServicePartition;
                result.Properties.Add(new PSNoteProperty(Constants.PartitionIdPropertyName, partitionResult.Info.Id));
                result.Properties.Add(new PSNoteProperty(Constants.PartitionKindPropertyName, partitionResult.Info.Kind));
                if (partitionResult.Info.Kind == ServicePartitionKind.Int64Range)
                {
                    var info = partitionResult.Info as Int64RangePartitionInformation;
                    if (info != null)
                    {
                        result.Properties.Add(new PSNoteProperty(Constants.PartitionLowKeyPropertyName, info.LowKey));
                        result.Properties.Add(new PSNoteProperty(Constants.PartitionHighKeyPropertyName, info.HighKey));
                    }
                }
                else if (partitionResult.Info.Kind == ServicePartitionKind.Named)
                {
                    var info = partitionResult.Info as NamedPartitionInformation;
                    if (info != null)
                    {
                        result.Properties.Add(new PSNoteProperty(Constants.PartitionNamePropertyName, info.Name));
                    }
                }

                var parametersPropertyPSObj = new PSObject(partitionResult.Endpoints);
                parametersPropertyPSObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                result.Properties.Add(
                    new PSNoteProperty(
                        Constants.EndPointsPropertyName,
                        parametersPropertyPSObj));
            }

            return base.FormatOutput(output);
        }
    }
}