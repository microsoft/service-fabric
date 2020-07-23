// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Management.Automation;
    using Microsoft.ServiceFabric.Preview.Client.Description;

    [Cmdlet(VerbsLifecycle.Start, "ServiceFabricComposeDeploymentRollback")]
    public sealed class StartComposeDeploymentRollback : ComposeDeploymentCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public string DeploymentName
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            try
            {
                this.RollbackComposeDeploymentUpgrade(new ComposeDeploymentRollbackDescription(this.DeploymentName));
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.RollbackComposeDeploymentUpgradeErrorId,
                    null);
            }
        }
    }
}
