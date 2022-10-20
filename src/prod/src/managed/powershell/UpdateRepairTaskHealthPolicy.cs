// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Management.Automation;

    [Cmdlet(
         VerbsData.Update,
         Constants.ServiceFabricRepairTaskHealthPolicy,
         SupportsShouldProcess = true,
         ConfirmImpact = ConfirmImpact.High)]
    public sealed class UpdateRepairTaskHealthPolicy : CommonCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        public string TaskId
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true)]
        public long Version
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true)]
        public bool? PerformPreparingHealthCheck
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true)]
        public bool? PerformRestoringHealthCheck
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public SwitchParameter Force
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            if (this.ShouldProcess(this.TaskId))
            {
                if (this.Force || this.ShouldContinue(string.Empty, string.Empty))
                {
                    var clusterConnection = this.GetClusterConnection();
                    try
                    {
                        clusterConnection.UpdateRepairTaskHealthPolicyAsync(
                            this.TaskId,
                            this.Version,
                            this.PerformPreparingHealthCheck,
                            this.PerformRestoringHealthCheck,
                            this.GetTimeout(),
                            this.GetCancellationToken()).Wait();
                    }
                    catch (AggregateException aggregateException)
                    {
                        aggregateException.Handle((ae) =>
                        {
                            this.ThrowTerminatingError(
                                ae,
                                Constants.UpdateRepairTaskHealthPolicyErrorId,
                                clusterConnection);
                            return true;
                        });
                    }
                    catch (Exception exception)
                    {
                        this.ThrowTerminatingError(
                            exception,
                            Constants.UpdateRepairTaskHealthPolicyErrorId,
                            clusterConnection);
                    }
                }
            }
        }
    }
}