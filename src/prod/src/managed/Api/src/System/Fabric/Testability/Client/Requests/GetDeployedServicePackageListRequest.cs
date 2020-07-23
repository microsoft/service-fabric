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
    using System.Fabric.Testability.Common;
    using System.Threading.Tasks;

    public class GetDeployedServicePackageListRequest : FabricRequest
    {
        public GetDeployedServicePackageListRequest(IFabricClient fabricClient, string nodeName, Uri applicationName, string serviceManifestNameFilter, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.NullOrEmpty(nodeName, "nodeName");
            ThrowIf.Null(applicationName, "applicationName");

            this.NodeName = nodeName;
            this.ApplicationName = applicationName;
            this.ServiceManifestNameFilter = serviceManifestNameFilter;
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

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "GetDeployedServicePackageListRequest on Node {0} Application name {1} with timeout {2}", this.NodeName, this.ApplicationName, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetDeployedServicePackageListAsync(this.NodeName, this.ApplicationName, this.ServiceManifestNameFilter, this.Timeout, cancellationToken);
        }
    }
}

#pragma warning restore 1591