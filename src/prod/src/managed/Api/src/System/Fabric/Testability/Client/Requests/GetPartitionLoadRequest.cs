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
    using System.Fabric.Interop;
    using System.Fabric.Testability.Common;
    using System.Threading;
    using System.Threading.Tasks;

    public class GetPartitionLoadRequest : FabricRequest
    {
        public GetPartitionLoadRequest(IFabricClient fabricClient, Guid partitionId, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(partitionId.ToString(), "partitionId");

            this.PartitionId = partitionId;
        }

        public Guid PartitionId
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "GetPartitionLoadRequest for partition {0} with timeout {1}", this.PartitionId, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetPartitionLoadAsync(this.PartitionId, this.Timeout, cancellationToken);
        }
    }
}


#pragma warning restore 1591