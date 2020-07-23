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

    public class GetServiceManifestRequest : FabricRequest
    {
        public GetServiceManifestRequest(IFabricClient fabricClient, string applicationTypeName, string applicationTypeVersion, string serviceManifestName, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.ApplicationTypeName = applicationTypeName;
            this.ApplicationTypeVersion = applicationTypeVersion;
            this.ServiceManifestName = serviceManifestName;
        }

        public string ApplicationTypeName
        {
            get;
            private set;
        }

        public string ApplicationTypeVersion
        {
            get;
            private set;
        }

        public string ServiceManifestName
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "GetServiceManifestRequest with timeout {0}", this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetServiceManifestAsync(this.ApplicationTypeName, this.ApplicationTypeVersion, this.ServiceManifestName, this.Timeout, cancellationToken);
        }
    }
}

#pragma warning restore 1591