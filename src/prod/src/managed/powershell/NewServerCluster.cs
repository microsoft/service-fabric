// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.New, "ServiceFabricServerCluster")]
    public sealed class NewServerCluster : ClusterCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public string ClusterManifestPath
        {
            get;
            set;
        }

        [Parameter(Mandatory = true)]
        public string FabricPackageSource
        {
            get;
            set;
        }

        // Copies to default location if not specified
        [Parameter(Mandatory = false)]
        public string FabricPackageDestination
        {
            get;
            set;
        }

        // If not specified, defaults to "Manual"
        [Parameter(Mandatory = false)]
        public bool FailureAction
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string FabricDataRoot
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string FabricLogRoot
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public PSCredential FabricHostCredential
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public SwitchParameter RunFabricHostServiceAsManual
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public SwitchParameter RemoveExistingConfiguration
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
            this.NewServerCluster(
                this.ClusterManifestPath,
                this.FabricPackageSource,
                this.FabricPackageDestination,
                this.FailureAction,
                this.FabricDataRoot,
                this.FabricLogRoot,
                this.FabricHostCredential,
                this.RunFabricHostServiceAsManual,
                this.RemoveExistingConfiguration);
        }
    }
}