// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Management.Automation;

    [Cmdlet(VerbsDiagnostic.Test, "ServiceFabricConfiguration")]
    public sealed class TestConfig : ClusterCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public string ClusterConfigurationFilePath
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string OldClusterConfigurationFilePath
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string FabricRuntimePackagePath
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

        protected override void ProcessRecord()
        {
            this.TestConfig(
                this.ClusterConfigurationFilePath, 
                this.OldClusterConfigurationFilePath, 
                this.FabricRuntimePackagePath, 
                this.MaxPercentFailedNodes);
        }
    }
}