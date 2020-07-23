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

    public class GetApplicationTypePagedListRequest : FabricRequest
    {
        public GetApplicationTypePagedListRequest(IFabricClient fabricClient, TimeSpan timeout)
            : this(fabricClient, null, timeout)
        {
        }

        public GetApplicationTypePagedListRequest(IFabricClient fabricClient, PagedApplicationTypeQueryDescription queryDescription, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.QueryDescription = queryDescription;
        }

        public GetApplicationTypePagedListRequest(
            IFabricClient fabricClient,
            PagedApplicationTypeQueryDescription queryDescription,
            TimeSpan timeout,
            bool getPagedResults = false)
            : base(fabricClient, timeout)
        {
            this.QueryDescription = queryDescription;
            this.GetPagedResults = getPagedResults;
            this.IsForPowerShell = true;
        }

        public PagedApplicationTypeQueryDescription QueryDescription
        {
            get;
            private set;
        }

        public bool GetPagedResults
        {
            get;
            private set;
        }

        private bool IsForPowerShell
        {
            get;
            set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "GetApplicationTypePagedList with query description {0}, timeout {1}", this.QueryDescription.ToString(), this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            if (this.IsForPowerShell)
            {
                this.OperationResult = await this.FabricClient.GetApplicationTypePagedListAsync(
                    this.QueryDescription, 
                    this.Timeout, 
                    cancellationToken,
                    this.GetPagedResults);
            }
            else
            {
                this.OperationResult = await this.FabricClient.GetApplicationTypePagedListAsync(this.QueryDescription, this.Timeout, cancellationToken);
            }
        }
    }
}

#pragma warning restore 1591