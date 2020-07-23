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

    public class GetApplicationTypeListRequest : FabricRequest
    {
        public GetApplicationTypeListRequest(IFabricClient fabricClient, TimeSpan timeout)
            : this(fabricClient, null, timeout)
        {
        }

        public GetApplicationTypeListRequest(IFabricClient fabricClient, string applicationTypeNameFilter, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.ApplicationTypeNameFilter = applicationTypeNameFilter;
        }

        public string ApplicationTypeNameFilter
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "GetApplicationTypeList with applicationTypeNameFilter {0}, timeout {1}", this.ApplicationTypeNameFilter, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetApplicationTypeListAsync(this.ApplicationTypeNameFilter, this.Timeout, cancellationToken);
        }
    }
}

#pragma warning restore 1591