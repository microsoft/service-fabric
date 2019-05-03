// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Fabric;
    using System.Management.Automation;

    [Cmdlet(VerbsLifecycle.Disable, "ServiceFabricNode", SupportsShouldProcess = true)]
    public sealed class DisableNode : ClusterCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public string NodeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 1)]
        public NodeDeactivationIntent Intent
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
            if (this.ShouldProcess(this.NodeName))
            {
                if (this.Force || this.ShouldContinue(string.Empty, string.Empty))
                {
                    this.InternalDisableNode(this.NodeName, this.Intent);
                }
            }
        }
    }
}