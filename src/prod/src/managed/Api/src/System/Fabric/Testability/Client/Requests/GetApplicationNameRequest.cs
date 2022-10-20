// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Fabric.Description;
    using System.Fabric.Testability.Common;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// This class is derived from the FabricRequest class and represents a GetApplicationName query request.
    /// The derived class needs to implement the ToString() and PerformCoreAsync() methods.
    /// </summary>
    public class GetApplicationNameRequest : FabricRequest
    {
        /// <summary>
        /// Initializes a new instance of the GetApplicationNameRequest.
        /// </summary>
        /// <param name="fabricClient"></param>
        /// <param name="serviceName"></param>
        /// <param name="timeout"></param>
        public GetApplicationNameRequest(IFabricClient fabricClient, Uri serviceName, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.ServiceName = serviceName;
        }

        /// <summary>
        /// Get the service name for the request.
        /// </summary>
        public Uri ServiceName
        {
            get;
            private set;
        }

        /// <summary>
        /// A string represent the GetApplicationNameRequest object including the service name property.
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "Get application name for {0} with timeout {1}",
                this.ServiceName,
                this.Timeout);
        }

        /// <summary>
        /// Perform the GetApplicationName operation asynchronously.
        /// </summary>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetApplicationNameAsync(this.ServiceName, this.Timeout, cancellationToken);
        }
    }
}

#pragma warning restore 1591