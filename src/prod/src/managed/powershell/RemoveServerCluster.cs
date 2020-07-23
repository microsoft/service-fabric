// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Remove, "ServiceFabricServerCluster", SupportsShouldProcess = true)]
    public sealed class RemoveServerCluster: ClusterCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public string ClusterManifestPath
        {
            get;
            set;
        }

        [Parameter(Mandatory = true)]
        public string FabricPackageRoot
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public SwitchParameter DeleteLog
        {
            get;
            set;
        }

        /*
                [Parameter(Mandatory = false)]
                public SwitchParameter UsingFabricPackage
                {
                    get;
                    set;
                }
        */
        [Parameter(Mandatory = false)]
        public SwitchParameter Force
        {
            get;
            set;
        }

        /*
                [Parameter(Mandatory = false)]
                public string MachineName
                {
                    get;
                    set;
                }
        */
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
                    this.RemoveServerCluster(this.DeleteLog, this.ClusterManifestPath, this.FabricPackageRoot);
                }
            }
        }
    }
}