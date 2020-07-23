// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Testability.Client.Structures;
    using System.Fabric.Testability.Common;

    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Justification = "Todo")]
    public class TestClusterManifestRequest : FabricRequest
    {
        public TestClusterManifestRequest(IFabricClient fabricClient, string clusterManifestPath)
            : base(fabricClient, TimeSpan.MaxValue/*API does not take a timeout*/)
        {
            ThrowIf.Null(clusterManifestPath, "clusterManifestPath");

            this.ClusterManifestPath = clusterManifestPath;
        }

        public string ClusterManifestPath
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Test clusterManifestPath request clusterManifestPath: {0} timeout {1})", this.ClusterManifestPath, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.TestClusterManifestAsync(this.ClusterManifestPath, cancellationToken);
        }
    }
}


#pragma warning restore 1591