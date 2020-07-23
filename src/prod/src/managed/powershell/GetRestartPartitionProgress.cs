// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricPartitionRestartProgress")]
    public sealed class GetRestartPartitionProgress : CommonCmdletBase
    {
        [Parameter(Mandatory = true)]
        public Guid OperationId
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                var invokeDataLossProgress = clusterConnection.GetRestartPartitionProgressAsync(
                                                 this.OperationId,
                                                 this.GetTimeout(),
                                                 this.GetCancellationToken()).GetAwaiter().GetResult();
                this.WriteObject(this.FormatOutput(invokeDataLossProgress));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetRestartPartitionProgressCommandErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected override object FormatOutput(object output)
        {
            var item = output as System.Fabric.PartitionRestartProgress;

            if (item == null)
            {
                return base.FormatOutput(output);
            }

            var itemPsObj = new PSObject(item);

            // State
            var statePropertyPsObj = new PSObject(item.State);
            statePropertyPsObj.Members.Add(
                new PSCodeMethod(
                    Constants.ToStringMethodName,
                    typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

            itemPsObj.Properties.Add(
                new PSNoteProperty(
                    Constants.StatePropertyName,
                    statePropertyPsObj));

            // Result
            if (item.Result != null)
            {
                var progressResultPropertyPsObj = new PSObject(item.Result);
                progressResultPropertyPsObj.Members.Add(
                    new PSCodeMethod(
                        Constants.ToStringMethodName,
                        typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

                itemPsObj.Properties.Add(
                    new PSNoteProperty(
                        Constants.ProgressResultPropertyName,
                        progressResultPropertyPsObj));
            }

            return itemPsObj;
        }
    }
}