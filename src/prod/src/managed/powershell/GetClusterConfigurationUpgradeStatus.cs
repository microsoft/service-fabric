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
    using System.Numerics;

    [Cmdlet(VerbsCommon.Get, Constants.GetClusterConfigurationUpgradeStatus)]

    public sealed class GetClusterConfigurationUpgradeStatus : CommonCmdletBase
    {
        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                var progress = clusterConnection.GetClusterConfigurationUpgradeStatusAsync(
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
                        Constants.GetClusterConfigurationUpgradeStatusErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        protected override object FormatOutput(object output)
        {
            var item = output as System.Fabric.FabricOrchestrationUpgradeProgress;

            if (item == null)
            {
                return base.FormatOutput(output);
            }

            var itemPsObj = new PSObject(item);

            // State
            var statePropertyPsObj = new PSObject(item.UpgradeState);
            statePropertyPsObj.Members.Add(
                new PSCodeMethod(
                    Constants.ToStringMethodName,
                    typeof(OutputFormatter).GetMethod(Constants.FormatObjectMethodName)));

            itemPsObj.Properties.Add(
                new PSNoteProperty(
                    Constants.UpgradeStatePropertyName,
                    statePropertyPsObj));

            return itemPsObj;
        }
    }
}