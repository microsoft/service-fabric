// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.New, "ServiceFabricCluster")]
    public sealed class NewCluster : ClusterCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public string ClusterConfigurationFilePath
        {
            get;
            set;
        }

        [Parameter(Mandatory = true)]
        public string FabricRuntimePackagePath
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public SwitchParameter NoCleanupOnFailure
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
        [ValidateRange(0, 100)]
        public int MaxPercentFailedNodes
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public uint TimeoutInSeconds
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            this.NewCluster(
                this.ClusterConfigurationFilePath,
                this.FabricRuntimePackagePath,
                this.NoCleanupOnFailure,
                this.Force,
                this.MaxPercentFailedNodes,
                this.TimeoutInSeconds);
        }
    }
}