// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Query;
    using System.Management.Automation;

    [Cmdlet(VerbsLifecycle.Stop, Constants.ServiceFabricTestCommand, SupportsShouldProcess = true)]
    public sealed class StopTestCommand : CommonCmdletBase
    {
        [Parameter(Mandatory = true)]
        public Guid OperationId
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public SwitchParameter ForceCancel
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
            if (this.ShouldProcess(this.OperationId.ToString()))
            {
                if (this.Force || this.ShouldContinue(string.Empty, string.Empty))
                {
                    var clusterConnection = this.GetClusterConnection();
                    try
                    {
                        clusterConnection.CancelTestCommandAsync(
                            this.OperationId,
                            this.ForceCancel,
                            this.GetTimeout(),
                            this.GetCancellationToken()).Wait();
                    }
                    catch (AggregateException aggregateException)
                    {
                        aggregateException.Handle((ae) =>
                        {
                            this.ThrowTerminatingError(
                                ae,
                                Constants.StopTestCommandCommandErrorId,
                                clusterConnection);
                            return true;
                        });
                    }
                }
            }
        }
    }
}