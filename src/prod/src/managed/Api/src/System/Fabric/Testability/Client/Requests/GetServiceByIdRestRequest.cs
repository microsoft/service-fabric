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
    using System.Fabric.Testability.Common;

    public class GetServiceByIdRestRequest : FabricRequest
    {
        public GetServiceByIdRestRequest(string applicationId, string serviceId, IFabricClient fabricClient, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.NullOrEmpty(applicationId, "applicationId");
            ThrowIf.NullOrEmpty(serviceId, "serviceId");
            this.ApplicationId = applicationId;
            this.ServiceId = serviceId;
        }

        public string ApplicationId { get; private set; }

        public string ServiceId { get; private set; }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture,
                "GetServiceByIdRestRequest with applicationId {0}, serviceId {1}, timeout {2}",
                this.ApplicationId,
                this.ServiceId,
                this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetServiceByIdRestAsync(this.ApplicationId, this.ServiceId, this.Timeout, cancellationToken);
        }
    }
}

#pragma warning restore 1591