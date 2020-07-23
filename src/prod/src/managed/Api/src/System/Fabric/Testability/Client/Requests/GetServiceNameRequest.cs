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
    /// This class is derived from the FabricRequest class and represents a GetServiceNameRequest query request.
    /// The derived class needs to implement the ToString() and PerformCoreAsync() methods.
    /// </summary>
    public class GetServiceNameRequest : FabricRequest
    {
        /// <summary>
        /// Initializes a new instance of the GetServiceNameRequest.
        /// </summary>
        /// <param name="fabricClient"></param>
        /// <param name="partitionId"></param>
        /// <param name="timeout"></param>
        public GetServiceNameRequest(IFabricClient fabricClient, Guid partitionId, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.PartitionId = partitionId;
        }

        /// <summary>
        /// Get the partition id for the request.
        /// </summary>
        public Guid PartitionId
        {
            get;
            private set;
        }

        /// <summary>
        /// A string represent the GetServiceNameRequest object including the partition id property.
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "Get service name for {0} with timeout {1}",
                this.PartitionId,
                this.Timeout);
        }

        /// <summary>
        /// Perform the GetServiceNameRequest operation asynchronously.
        /// </summary>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetServiceNameAsync(this.PartitionId, this.Timeout, cancellationToken);
        }
    }
}

#pragma warning restore 1591