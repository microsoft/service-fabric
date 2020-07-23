// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Management.Automation;

    [Cmdlet(VerbsLifecycle.Register, "ServiceFabricClusterPackage", DefaultParameterSetName = "Both")]
    public sealed class RegisterClusterPackage : ClusterCmdletBase
    {
        [Parameter(Mandatory = true, ParameterSetName = "Code")]
        public SwitchParameter Code
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Config")]
        public SwitchParameter Config
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Code")]
        [Parameter(Mandatory = false, ParameterSetName = "Config")]
        [Parameter(Mandatory = true, ParameterSetName = "Both")]
        public string CodePackagePath
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Code")]
        [Parameter(Mandatory = true, ParameterSetName = "Config")]
        [Parameter(Mandatory = true, ParameterSetName = "Both")]
        public string ClusterManifestPath
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            this.RegisterClusterPackage(this.CodePackagePath, this.ClusterManifestPath);
        }
    }
}