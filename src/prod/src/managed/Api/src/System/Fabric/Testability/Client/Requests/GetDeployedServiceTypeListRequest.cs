// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public class GetDeployedServiceTypeListRequest : FabricRequest
    {
        public GetDeployedServiceTypeListRequest(IFabricClient fabricClient, string nodeName, Uri applicationName, string serviceManifestNameFilter, string serviceTypeNameFilter, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.NodeName = nodeName;
            this.ApplicationName = applicationName;
            this.ServiceManifestNameFilter = serviceManifestNameFilter;
            this.ServiceTypeNameFilter = serviceTypeNameFilter;

            this.RetryErrorCodes.Add((uint)NativeTypes.FABRIC_ERROR_CODE.FABRIC_E_APPLICATION_NOT_FOUND);
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

        public string ServiceTypeNameFilter
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "GetDeployedServiceTypeListRequest with timeout {0}", this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetDeployedServiceTypeListAsync(this.NodeName, this.ApplicationName, this.ServiceManifestNameFilter, this.ServiceTypeNameFilter, this.Timeout, cancellationToken);
        }
    }
}

#pragma warning restore 1591