// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Management.Automation;

    [Cmdlet(VerbsLifecycle.Enable, "ServiceFabricNode")]
    public sealed class EnableNode : ClusterCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public string NodeName
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            this.InternalEnableNode(this.NodeName);
        }
    }
}