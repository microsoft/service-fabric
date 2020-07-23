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

    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Justification = "ToDo")]
    public class GetServiceTypeListRequest : FabricRequest
    {
        public GetServiceTypeListRequest(IFabricClient fabricClient, string applicationTypeName, string applicationTypeVersion, string serviceTypeNameFilter, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.ApplicationTypeName = applicationTypeName;
            this.ApplicationTypeVersion = applicationTypeVersion;
            this.ServiceTypeNameFilter = serviceTypeNameFilter;
        }

        public string ApplicationTypeName { get; private set; }

        public string ApplicationTypeVersion { get; private set; }

        public string ServiceTypeNameFilter { get; private set; }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "GetServiceTypeListRequest for {0}.{1} and filter '{2}' with timeout {3}", this.ApplicationTypeName, this.ApplicationTypeVersion, this.ServiceTypeNameFilter, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetServiceTypeListAsync(this.ApplicationTypeName, this.ApplicationTypeVersion, this.ServiceTypeNameFilter, this.Timeout, cancellationToken);
        }
    }
}


#pragma warning restore 1591