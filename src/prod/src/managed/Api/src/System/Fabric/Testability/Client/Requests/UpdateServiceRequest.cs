// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Fabric.Testability.Client.Structures;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Testability.Common;
    using System.Fabric.Description;

    public class UpdateServiceRequest : FabricRequest
    {
        public UpdateServiceRequest(IFabricClient fabricClient, Uri serviceName, ServiceUpdateDescription updateDescription, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(serviceName, "serviceName");
            ThrowIf.Null(updateDescription, "updateDescription");

            this.ServiceName = serviceName;
            this.UpdateDescription = updateDescription;
        }

        public Uri ServiceName
        {
            get;
            private set;
        }

        public ServiceUpdateDescription UpdateDescription
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Update service {0} with timeout {1}", this.ServiceName, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.UpdateServiceAsync(this.ServiceName, this.UpdateDescription, this.Timeout, cancellationToken);
        }
    }
}


#pragma warning restore 1591