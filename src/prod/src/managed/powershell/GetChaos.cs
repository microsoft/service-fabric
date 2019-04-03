// ----------------------------------------------------------------------
//  <copyright file="GetChaos.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Chaos.DataStructures;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricChaos")]
    public sealed class GetChaos : CommonCmdletBase
    {
        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                ChaosDescription description = null;

                description = clusterConnection.GetChaosAsync(
                    this.GetTimeout(),
                    this.GetCancellationToken()).Result;

                this.WriteObject(this.FormatOutput(description));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetChaosCommandErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }
    }
}