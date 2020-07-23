// ----------------------------------------------------------------------
//  <copyright file="SetChaosSchedule.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Chaos.DataStructures;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Set, "ServiceFabricChaosSchedule")]
    public sealed class SetChaosSchedule : CommonCmdletBase
    {
        /// <summary>
        /// Schedule for which Chaos will be scheduled with.
        /// </summary>
        [Parameter(Mandatory = true)]
        public ChaosScheduleDescription ChaosScheduleDescription
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                clusterConnection.SetChaosScheduleAsync(
                    this.ChaosScheduleDescription,
                    this.GetTimeout(),
                    this.GetCancellationToken()).GetAwaiter().GetResult();
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.SetChaosScheduleCommandErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }
    }
}