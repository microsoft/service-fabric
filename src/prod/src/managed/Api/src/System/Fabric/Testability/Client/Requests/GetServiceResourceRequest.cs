// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public class GetServiceResourceRequest : FabricRequest
    {
        public GetServiceResourceRequest(IFabricClient fabricClient, TimeSpan timeout)
            : this(fabricClient, null, null, timeout)
        {
        }

        public GetServiceResourceRequest(IFabricClient fabricClient, string applicationName, string serviceName, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.ApplicationName = applicationName;
            this.ServiceName = serviceName;
        }

        public string ApplicationName
        {
            get;
            private set;
        }

        public string ServiceName
        {
            get;
            private set;
        }

        public string ContinuationToken
        {
            get;
            set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture,
                "GetServiceResource with applicationName {0} serviceName {1} timeout {2}, continuation token {3}",
                this.ApplicationName,
                this.ServiceName,
                this.Timeout, 
                this.ContinuationToken);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetServiceResourceListAsync(this.ApplicationName, this.ServiceName, this.ContinuationToken, this.Timeout, cancellationToken);
        }
    }
}

#pragma warning restore 1591