// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Management.Automation;

    [Cmdlet(VerbsData.Update, "ServiceFabricNodeConfiguration", SupportsShouldProcess = true)]
    public sealed class UpdateNodeConfiguration : ClusterCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public string ClusterManifestPath
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

        private new int? TimeoutSec
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            if (this.ShouldProcess(this.ClusterManifestPath))
            {
                if (this.Force || this.ShouldContinue(string.Empty, string.Empty))
                {
                    this.UpdateNodeNodeConfiguration(this.ClusterManifestPath);
                }
            }
        }
    }
}