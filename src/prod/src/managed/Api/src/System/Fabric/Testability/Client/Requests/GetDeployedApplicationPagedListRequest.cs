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
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public class GetDeployedApplicationPagedListRequest : FabricRequest
    {
        public GetDeployedApplicationPagedListRequest(IFabricClient fabricClient, PagedDeployedApplicationQueryDescription queryDescription, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.QueryDescription = queryDescription;
        }

        public PagedDeployedApplicationQueryDescription QueryDescription
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "GetDeployedApplicationPagedListRequest with timeout {0} and query description {1}",
                this.Timeout,
                this.QueryDescription);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetDeployedApplicationPagedListAsync(this.QueryDescription, this.Timeout, cancellationToken);
        }
    }
}

#pragma warning restore 1591