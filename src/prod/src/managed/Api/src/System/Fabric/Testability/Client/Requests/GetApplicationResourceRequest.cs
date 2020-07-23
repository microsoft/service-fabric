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
    using System.Fabric.Description;

    public class GetApplicationResourceRequest : FabricRequest
    {
        public GetApplicationResourceRequest(IFabricClient fabricClient, TimeSpan timeout)
            : this(fabricClient, null, timeout)
        {
        }

        public GetApplicationResourceRequest(IFabricClient fabricClient, string applicationName, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.ApplicationName = applicationName;
        }

        public string ApplicationName
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
                "GetApplicationResource with applicationName {0} timeout {1}, continuation token {2}",
                this.ApplicationName,
                this.Timeout, 
                this.ContinuationToken);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetApplicationResourceListAsync(this.ApplicationName, this.ContinuationToken, this.Timeout, cancellationToken);
        }
    }
}

#pragma warning restore 1591