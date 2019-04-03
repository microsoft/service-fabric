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
    using System.Fabric.Testability.Client.Structures;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public class GetFabricConfigurationRequest : FabricRequest
    {
        public GetFabricConfigurationRequest(IFabricClient fabricClient, string apiVersion, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.ApiVersion = apiVersion;
        }

        public string ApiVersion
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Get configuration for fabric with apiVersion {0} and timeout {1}", this.ApiVersion, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetFabricConfigurationAsync(this.ApiVersion, this.Timeout, cancellationToken);
        }
    }
}


#pragma warning restore 1591