// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Remove, "ServiceFabricNodeState", SupportsShouldProcess = true)]
    public sealed class RemoveNodeState : ClusterCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        public string NodeName
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
                    this.RemoveNodeState(this.NodeName);
                }
            }
        }
    }
}