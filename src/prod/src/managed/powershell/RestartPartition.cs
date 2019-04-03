// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric;
    using System.Fabric.Result;
    using System.Management.Automation;

    [Cmdlet(VerbsLifecycle.Restart, "ServiceFabricPartition")]
    [Obsolete("This cmdlet is deprecated.  Use Start-ServiceFabricPartitionRestart instead.  Start-ServiceFabricPartitionRestart requires the FaultAnalysisService")]
    public sealed class RestartPartition : PartitionSelectorOperationBase
    {
        [Parameter(Mandatory = true)]
        public RestartPartitionMode RestartPartitionMode
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                var partitionSelector = this.GetPartitionSelector();

                var restartParatitionResult = clusterConnection.RestartPartitionAsync(
                                                  partitionSelector,
                                                  this.RestartPartitionMode,
                                                  this.TimeoutSec,
                                                  this.GetCancellationToken()).Result;
                this.WriteObject(this.FormatOutput(restartParatitionResult));
            }
            catch (AggregateException aggregateException)
            {
                this.ThrowTerminatingError(
                    aggregateException.InnerException,
                    Constants.RestartPartitionCommandErrorId,
                    clusterConnection);
            }
        }

        protected override object FormatOutput(object output)
        {
            var item = output as RestartPartitionResult;

            var parametersPropertyPsObj = new PSObject(item.SelectedPartition);
            parametersPropertyPsObj.Members.Add(
                new PSCodeMethod(
                    Constants.ToStringMethodName,
                    typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

            var itemPsObj = new PSObject(item);

            itemPsObj.Properties.Add(
                new PSNoteProperty(
                    Constants.SelectedPartitionPropertyName,
                    parametersPropertyPsObj));

            return itemPsObj;
        }
    }
}