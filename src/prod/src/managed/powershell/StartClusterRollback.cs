// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Management.Automation;

    [Cmdlet(VerbsLifecycle.Start, "ServiceFabricClusterRollback")]
    public sealed class StartClusterRollback : ClusterCmdletBase
    {
        protected override void ProcessRecord()
        {
            try
            {
                this.RollbackClusterUpgrade();
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.RollbackClusterUpgradeErrorId,
                    null);
            }
        }
    }
}