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

    public class GetDeployedReplicaListRequest : FabricRequest
    {
        public GetDeployedReplicaListRequest(IFabricClient fabricClient, string nodeName, Uri applicationName, string serviceManifestNameFilter, Guid? partitionIdFilter, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.NodeName = nodeName;
            this.ApplicationName = applicationName;
            this.ServiceManifestNameFilter = serviceManifestNameFilter;
            this.PartitionIdFilter = partitionIdFilter;
        }

        public string NodeName
        {
            get;
            private set;
        }

        public Uri ApplicationName
        {
            get;
            private set;
        }

        public string ServiceManifestNameFilter
        {
            get;
            private set;
        }

        public Guid? PartitionIdFilter
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "GetDeployedReplicaListRequest with timeout {0} for {1}:{2}", this.Timeout, this.NodeName, this.ApplicationName);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetDeployedReplicaListAsync(this.NodeName, this.ApplicationName, this.ServiceManifestNameFilter, this.PartitionIdFilter, this.Timeout, cancellationToken);
        }
    }
}

#pragma warning restore 1591