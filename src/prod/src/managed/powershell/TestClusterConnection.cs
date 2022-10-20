// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Management.Automation;

    [Cmdlet(VerbsDiagnostic.Test, "ServiceFabricClusterConnection")]
    public sealed class TestClusterConnection : ClusterCmdletBase
    {
        [Parameter(Mandatory = false)]
        public SwitchParameter AllowNetworkConnectionOnly
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            this.TestClusterConnection(this.GetClusterConnection(), this.AllowNetworkConnectionOnly);
        }
    }
}