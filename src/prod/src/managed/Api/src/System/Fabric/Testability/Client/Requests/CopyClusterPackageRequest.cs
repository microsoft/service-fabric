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

    public class CopyClusterPackageRequest : FabricRequest
    {
        public CopyClusterPackageRequest(IFabricClient fabricClient, string codePackagePath, string clusterManifestPath, string imageStoreConnectionString, string codePackagePathInImageStore, string clusterManifestPathInImageStore)
            : base(fabricClient, TimeSpan.MaxValue/*API does not take a timeout*/)
        {
            ThrowIf.Null(imageStoreConnectionString, "imageStoreConnectionString");
            if (string.IsNullOrEmpty(codePackagePath) && string.IsNullOrEmpty(clusterManifestPath))
            {
                throw new InvalidProgramException("CopyCodePackage both code package path and cluster manifest path cannot be null at the same time");
            }

            this.CodePackagePath = codePackagePath;
            this.ClusterManifestPath = clusterManifestPath;
            this.ImageStoreConnectionString = imageStoreConnectionString;
            this.CodePackagePathInImageStore = codePackagePathInImageStore;
            this.ClusterManifestPathInImageStore = clusterManifestPathInImageStore;
        }


        public CopyClusterPackageRequest(IFabricClient fabricClient, string codePackagePath, string clusterManifestPath, string imageStoreConnectionString, string codePackagePathInImageStore, string clusterManifestPathInImageStore, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(imageStoreConnectionString, "imageStoreConnectionString");
            if (string.IsNullOrEmpty(codePackagePath) && string.IsNullOrEmpty(clusterManifestPath))
            {
                throw new InvalidProgramException("CopyCodePackage both code package path and cluster manifest path cannot be null at the same time");
            }

            this.CodePackagePath = codePackagePath;
            this.ClusterManifestPath = clusterManifestPath;
            this.ImageStoreConnectionString = imageStoreConnectionString;
            this.CodePackagePathInImageStore = codePackagePathInImageStore;
            this.ClusterManifestPathInImageStore = clusterManifestPathInImageStore;
        }
        public string CodePackagePath
        {
            get;
            private set;
        }

        public string ClusterManifestPath
        {
            get;
            private set;
        }

        public string ImageStoreConnectionString
        {
            get;
            private set;
        }

        public string CodePackagePathInImageStore
        {
            get;
            private set;
        }

        public string ClusterManifestPathInImageStore
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Copy cluster package request (CodePackage: {0} ClusterManifestPath: {1} imagestoreConnectionString {2} CodePackagePathInImageStore: {3} ClusterManifestPathInImageStore: {4} timeout {5})", this.CodePackagePath, this.ClusterManifestPath, this.ImageStoreConnectionString, this.CodePackagePathInImageStore, this.ClusterManifestPathInImageStore, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.CopyClusterPackageAsync(this.CodePackagePath, this.ClusterManifestPath, this.ImageStoreConnectionString, this.CodePackagePathInImageStore, this.ClusterManifestPathInImageStore, this.Timeout, cancellationToken);
        }
    }
}


#pragma warning restore 1591