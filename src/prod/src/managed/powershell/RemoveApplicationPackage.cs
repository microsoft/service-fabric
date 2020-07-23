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

    [Cmdlet(VerbsCommon.Remove, "ServiceFabricApplicationPackage", SupportsShouldProcess = true)]
    public sealed class RemoveApplicationPackage : ApplicationCmdletBase
    {
        public RemoveApplicationPackage()
        {
            this.CertStoreLocation = StoreLocation.LocalMachine;
        }

        [Parameter(Mandatory = true, Position = 0)]
        public string ApplicationPackagePathInImageStore
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, Position = 1)]
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
            if (this.ShouldProcess(this.ApplicationPackagePathInImageStore))
            {
                this.ThrowIfInvalidReservedImageStoreOperation(this.ApplicationPackagePathInImageStore);

                if (!this.ValidateImageStorePath(this.ApplicationPackagePathInImageStore))
                {
                    return;
                }

                this.RemoveApplicationPackage(
                    this.TryFetchImageStoreConnectionString(this.ImageStoreConnectionString, this.CertStoreLocation),
                    this.ApplicationPackagePathInImageStore);
            }
        }
    }
}