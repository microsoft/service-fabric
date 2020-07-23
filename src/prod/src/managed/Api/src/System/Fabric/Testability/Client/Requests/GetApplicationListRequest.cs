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
    using System.Fabric.Description;

    public class GetApplicationListRequest : FabricRequest
    {
        public GetApplicationListRequest(IFabricClient fabricClient, TimeSpan timeout)
            : this(fabricClient, new ApplicationQueryDescription(), timeout)
        {
        }

        public GetApplicationListRequest(IFabricClient fabricClient, ApplicationQueryDescription description, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.ApplicationQueryDescription = description;
        }

        public ApplicationQueryDescription ApplicationQueryDescription
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture,
                "GetApplicationList with applicationNameFilter {0}, applicationTypeNameFilter {1}, applicationDefinitionKindFilter {2} timeout {3}, continuation token {4}",
                this.ApplicationQueryDescription.ApplicationNameFilter,
                this.ApplicationQueryDescription.ApplicationTypeNameFilter,
                this.ApplicationQueryDescription.ApplicationDefinitionKindFilter,
                this.Timeout, 
                this.ApplicationQueryDescription.ContinuationToken);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetApplicationPagedListAsync(this.ApplicationQueryDescription, this.Timeout, cancellationToken);
        }
    }
}

#pragma warning restore 1591