// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, Constants.GetNodeTransitionProgressName)]
    public sealed class GetNodeTransitionProgress : CommonCmdletBase
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
                var progress = clusterConnection.GetNodeTransitionProgressAsync(
                                   this.OperationId,
                                   this.GetTimeout(),
                                   this.GetCancellationToken()).Result;

                this.WriteObject(this.FormatOutput(progress));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetNodeTransitionProgressCommandErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected override object FormatOutput(object output)
        {
            var item = output as System.Fabric.NodeTransitionProgress;

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