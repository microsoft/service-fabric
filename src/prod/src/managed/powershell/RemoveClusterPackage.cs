// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Strings;
    using System.Management.Automation;
    using System.Security.Cryptography.X509Certificates;

    [Cmdlet(VerbsCommon.Remove, "ServiceFabricClusterPackage", SupportsShouldProcess = true)]
    public sealed class RemoveClusterPackage : ClusterCmdletBase
    {
        public RemoveClusterPackage()
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
        public string CodePackagePathInImageStore
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Code")]
        [Parameter(Mandatory = true, ParameterSetName = "Config")]
        [Parameter(Mandatory = true, ParameterSetName = "Both")]
        public string ClusterManifestPathInImageStore
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

        [Parameter(Mandatory = false)]
        public StoreLocation CertStoreLocation
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            if (!string.IsNullOrEmpty(this.CodePackagePathInImageStore) && !this.ShouldProcess(this.CodePackagePathInImageStore))
            {
                return;
            }

            if (!string.IsNullOrEmpty(this.ClusterManifestPathInImageStore) && !this.ShouldProcess(this.ClusterManifestPathInImageStore))
            {
                return;
            }

            if (!string.IsNullOrEmpty(this.CodePackagePathInImageStore))
            {
                if (!this.ValidateImageStorePath(this.CodePackagePathInImageStore))
                {
                    return;
                }
            }

            if (!string.IsNullOrEmpty(this.ClusterManifestPathInImageStore))
            {
                if (!this.ValidateImageStorePath(this.ClusterManifestPathInImageStore))
                {
                    return;
                }
            }

            this.RemoveClusterPackage(
                this.CodePackagePathInImageStore, 
                this.ClusterManifestPathInImageStore,
                this.TryFetchImageStoreConnectionString(this.ImageStoreConnectionString, this.CertStoreLocation));
        }
    }
}