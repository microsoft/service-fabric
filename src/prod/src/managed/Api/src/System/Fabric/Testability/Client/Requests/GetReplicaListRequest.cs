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

    public class GetReplicaListRequest : FabricRequest
    {
        public GetReplicaListRequest(IFabricClient fabricClient, Guid partitionId, long replicaIdOrInstanceIdFilter, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.PartitionId = partitionId;
            this.ReplicaIdOrInstanceIdFilter = replicaIdOrInstanceIdFilter;
        }

        public Guid PartitionId
        {
            get;
            private set;
        }

        public long ReplicaIdOrInstanceIdFilter
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
            return string.Format(CultureInfo.InvariantCulture, "GetReplicaListRequest for partition {0} filter {1} with timeout {2}, continuation token {3}", this.PartitionId, this.ReplicaIdOrInstanceIdFilter, this.Timeout, this.ContinuationToken);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetReplicaListAsync(this.PartitionId, this.ReplicaIdOrInstanceIdFilter, this.ContinuationToken, this.Timeout, cancellationToken);
        }
    }
}

#pragma warning restore 1591