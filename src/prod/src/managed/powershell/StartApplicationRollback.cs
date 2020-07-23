// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Management.Automation;

    [Cmdlet(VerbsLifecycle.Start, "ServiceFabricApplicationRollback")]
    public sealed class StartApplicationRollback : ApplicationCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public Uri ApplicationName
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            try
            {
                this.RollbackApplicationUpgrade(this.ApplicationName);
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.RollbackApplicationUpgradeErrorId,
                    null);
            }
        }
    }
}