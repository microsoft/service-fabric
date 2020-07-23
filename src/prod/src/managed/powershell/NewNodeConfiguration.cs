// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.New, "ServiceFabricNodeConfiguration")]
    public sealed class NewNodeConfiguration : ClusterCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public string ClusterManifestPath
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string InfrastructureManifestPath
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

        [Parameter(Mandatory = false)]
        public string BootstrapMSIPath
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public SwitchParameter UsingFabricPackage
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string FabricPackageRoot
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
            this.NewNodeConfiguration(
                this.ClusterManifestPath,
                this.InfrastructureManifestPath,
                this.FabricDataRoot,
                this.FabricLogRoot,
                this.FabricHostCredential,
                this.RunFabricHostServiceAsManual,
                this.RemoveExistingConfiguration,
                this.UsingFabricPackage,
                this.FabricPackageRoot,
                this.MachineName,
                this.BootstrapMSIPath);
        }
    }
}