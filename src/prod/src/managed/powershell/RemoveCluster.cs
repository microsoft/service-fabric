// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Fabric.Strings;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Remove, "ServiceFabricCluster")]
    public sealed class RemoveCluster : ClusterCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public string ClusterConfigurationFilePath
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

        [Parameter(Mandatory = false)]
        public SwitchParameter Force
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            if (!this.Force &&
                !this.ShouldContinue(StringResources.Warning_RemoveServiceFabricCluster, string.Empty))
            {
                return;
            }

            this.RemoveCluster(
                this.ClusterConfigurationFilePath,
                this.DeleteLog);
        }
    }
}