// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Management.Automation;
    using System.Security.Cryptography.X509Certificates;

    [Cmdlet(VerbsCommon.Copy, "ServiceFabricClusterPackage", DefaultParameterSetName = "Both")]
    public sealed class CopyClusterPackage : ClusterCmdletBase
    {
        public CopyClusterPackage()
        {
            this.CertStoreLocation = StoreLocation.LocalMachine;
        }

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

        [Parameter(Mandatory = false)]
        public string ImageStoreConnectionString
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Code")]
        [Parameter(Mandatory = false, ParameterSetName = "Both")]
        public string CodePackagePathInImageStore
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Config")]
        [Parameter(Mandatory = false, ParameterSetName = "Both")]
        public string ClusterManifestPathInImageStore
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public StoreLocation CertStoreLocation
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            this.CodePackagePath = this.GetAbsolutePath(this.CodePackagePath);
            this.ClusterManifestPath = this.GetAbsolutePath(this.ClusterManifestPath);

            this.CopyClusterPackage(
                this.CodePackagePath,
                this.ClusterManifestPath,
                this.TryFetchImageStoreConnectionString(this.ImageStoreConnectionString, this.CertStoreLocation),
                this.CodePackagePathInImageStore,
                this.ClusterManifestPathInImageStore);
        }
    }
}