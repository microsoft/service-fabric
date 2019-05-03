// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Remove, "ServiceFabricNodeConfiguration", SupportsShouldProcess = true)]
    public sealed class RemoveNodeConfiguration : ClusterCmdletBase
    {
        [Parameter(Mandatory = false)]
        public SwitchParameter DeleteLog
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

        [Parameter(Mandatory = false)]
        public string MachineName
        {
            get;
            set;
        }

        private new int? TimeoutSec
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            if (this.ShouldProcess(string.Empty))
            {
                if (this.Force || this.ShouldContinue(string.Empty, string.Empty))
                {
                    this.RemoveNodeConfiguration(this.DeleteLog, false, this.MachineName);
                }
            }
        }
    }
}