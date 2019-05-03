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
    using System.Fabric.Testability.Common;
    using System.Threading.Tasks;

    public class GetServiceDescriptionRequest : FabricRequest
    {
        public GetServiceDescriptionRequest(IFabricClient fabricClient, Uri serviceName, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(serviceName, "serviceName");

            this.ServiceName = serviceName;
        }

        public Uri ServiceName
        {
            get;
            private set;
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult =
                await this.FabricClient.GetServiceDescriptionAsync(this.ServiceName, this.Timeout, cancellationToken);
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "GetServiceDescription for service {0} with timeout {1}", this.ServiceName, this.Timeout);
        }
    }
}


#pragma warning restore 1591