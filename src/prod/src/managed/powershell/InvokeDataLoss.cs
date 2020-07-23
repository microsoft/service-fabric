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

    [Cmdlet(VerbsLifecycle.Invoke, "ServiceFabricPartitionDataLoss")]
    [Obsolete("This cmdlet is deprecated.  Use Start-ServiceFabricPartitionDataLoss instead.  Start-ServiceFabricPartitionDataLoss requires the FaultAnalysisService")]
    public sealed class InvokeDataLoss : PartitionSelectorOperationBase
    {
        [Parameter(Mandatory = true)]
        public DataLossMode DataLossMode
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
                var invokeDataLossResult = clusterConnection.InvokeDataLossAsync(
                                               partitionSelector,
                                               this.DataLossMode,
                                               this.TimeoutSec,
                                               this.GetCancellationToken()).GetAwaiter().GetResult();
                this.WriteObject(this.FormatOutput(invokeDataLossResult));
            }
            catch (AggregateException aggregateException)
            {
                this.ThrowTerminatingError(
                    aggregateException.InnerException,
                    Constants.InvokeDataLossCommandErrorId,
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
            var item = output as InvokeDataLossResult;

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