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

    [Cmdlet(VerbsLifecycle.Invoke, "ServiceFabricPartitionQuorumLoss")]
    [Obsolete("This cmdlet is deprecated.  Use Start-ServiceFabricPartitionQuorumLoss instead.  Start-ServiceFabricPartitionQuorumLoss equires the FaultAnalysisService")]
    public sealed class InvokeQuorumLoss : PartitionSelectorOperationBase
    {
        [Parameter(Mandatory = true)]
        public QuorumLossMode QuorumLossMode
        {
            get;
            set;
        }

        [Parameter(Mandatory = true)]
        public int QuorumLossDurationInSeconds
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

                var invokeQuorumLossResult = clusterConnection.InvokeQuorumLossAsync(
                                                 partitionSelector,
                                                 this.QuorumLossMode,
                                                 this.QuorumLossDurationInSeconds,
                                                 this.TimeoutSec,
                                                 this.GetCancellationToken()).Result;
                this.WriteObject(this.FormatOutput(invokeQuorumLossResult));
            }
            catch (AggregateException aggregateException)
            {
                this.ThrowTerminatingError(
                    aggregateException.InnerException,
                    Constants.InvokeQuorumLossCommandErrorId,
                    clusterConnection);
            }
        }

        /// <summary>
        /// The format output.
        /// </summary>
        /// <param name="output">
        /// The output.
        /// </param>
        /// <returns>
        /// The <see cref="object"/>.
        /// </returns>
        protected override object FormatOutput(object output)
        {
            var item = output as InvokeQuorumLossResult;

            var selectedPartitionPropertyPsObj = new PSObject(item.SelectedPartition);
            selectedPartitionPropertyPsObj.Members.Add(
                new PSCodeMethod(
                    Constants.ToStringMethodName,
                    typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

            var itemPsObj = new PSObject(item);
            itemPsObj.Properties.Add(
                new PSNoteProperty(
                    Constants.SelectedPartitionPropertyName,
                    selectedPartitionPropertyPsObj));

            return itemPsObj;
        }
    }
}