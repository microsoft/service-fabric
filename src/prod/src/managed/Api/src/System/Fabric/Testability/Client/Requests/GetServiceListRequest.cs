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

    public class GetServiceListRequest : FabricRequest
    {
        public GetServiceListRequest(IFabricClient fabricClient, ServiceQueryDescription serviceQueryDescription, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            // Allow null applicationName - implies ad-hoc service
            this.ServiceQueryDescription = serviceQueryDescription;
        }

        public ServiceQueryDescription ServiceQueryDescription
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, 
                "GetServiceList for application {0} serviceName {1} serviceTypeName {2} with timeout {3}, continuation token \"{4}\"", 
                this.ServiceQueryDescription.ApplicationName == null ? "null" : this.ServiceQueryDescription.ApplicationName.OriginalString,
                this.ServiceQueryDescription.ServiceNameFilter,
                this.ServiceQueryDescription.ServiceTypeNameFilter,
                this.Timeout,
                this.ServiceQueryDescription.ContinuationToken);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetServicePagedListAsync(this.ServiceQueryDescription, this.Timeout, cancellationToken);
        }
    }
}

#pragma warning restore 1591