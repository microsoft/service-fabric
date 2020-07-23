// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Management.Automation;

    [Cmdlet(VerbsLifecycle.Unregister, "ServiceFabricClusterPackage", SupportsShouldProcess = true, DefaultParameterSetName = "Both")]
    public sealed class UnregisterClusterPackage : ClusterCmdletBase
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
        public string CodePackageVersion
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Code")]
        [Parameter(Mandatory = true, ParameterSetName = "Config")]
        [Parameter(Mandatory = true, ParameterSetName = "Both")]
        public string ClusterManifestVersion
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
            if (this.ShouldProcess(string.Empty))
            {
                if (this.Force || this.ShouldContinue(string.Empty, string.Empty))
                {
                    this.UnRegisterClusterPackage(this.CodePackageVersion, this.ClusterManifestVersion);
                }
            }
        }
    }
}