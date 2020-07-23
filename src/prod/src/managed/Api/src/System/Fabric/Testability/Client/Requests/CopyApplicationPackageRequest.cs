// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Fabric.Testability.Common;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public class CopyApplicationPackageRequest : FabricRequest
    {
        public CopyApplicationPackageRequest(
            IFabricClient fabricClient, 
            string applicationPackagePath, 
            string imageStoreConnectionString, 
            string applicationPackagePathInImageStore, 
            bool compress,
            TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(applicationPackagePath, "applicationPackagePath");
            ThrowIf.Null(imageStoreConnectionString, "imageStoreConnectionString");

            this.ApplicationPackagePath = applicationPackagePath;
            this.ImageStoreConnectionString = imageStoreConnectionString;
            this.ApplicationPackagePathInImageStore = applicationPackagePathInImageStore;
            this.Compress = compress;
        }

        public string ApplicationPackagePath
        {
            get;
            private set;
        }

        public string ImageStoreConnectionString
        {
            get;
            private set;
        }

        public string ApplicationPackagePathInImageStore
        {
            get; 
            private set; 
        }

        public bool Compress
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Copy application package request (ApplicationPackagePath: {0} imagestoreConnectionString {1} Compress {2} timeoutInSec {3})", this.ApplicationPackagePath, this.ImageStoreConnectionString, this.Compress, this.Timeout.TotalSeconds);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.CopyApplicationPackageAsync(this.ApplicationPackagePath, this.ImageStoreConnectionString, this.ApplicationPackagePathInImageStore, this.Compress, this.Timeout, cancellationToken);
        }
    }
}


#pragma warning restore 1591