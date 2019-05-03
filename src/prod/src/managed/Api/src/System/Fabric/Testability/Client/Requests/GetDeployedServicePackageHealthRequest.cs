// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Fabric.Description;
    using System.Fabric.Testability.Common;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public class GetDeployedServicePackageHealthRequest : FabricRequest
    {        
        public GetDeployedServicePackageHealthRequest(IFabricClient fabricClient, DeployedServicePackageHealthQueryDescription queryDescription, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(queryDescription, "queryDescription");
            this.QueryDescription = queryDescription;
        }

        public DeployedServicePackageHealthQueryDescription QueryDescription
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "Get deployed service package health {0}+{1}+{2} with timeout {3}",
                this.QueryDescription.ApplicationName,
                this.QueryDescription.NodeName,
                this.QueryDescription.ServiceManifestName,
                this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetDeployedServicePackageHealthAsync(this.QueryDescription, this.Timeout, cancellationToken);
        }
    }
}

#pragma warning restore 1591