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
    using System.Fabric.Testability.Client.Structures;

    public class GetReplicaLoadRequest : FabricRequest
    {
        public GetReplicaLoadRequest(IFabricClient fabricClient, Guid partitionId, long replicaId, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.PartitionId = partitionId;
            this.ReplicaId = replicaId;
        }

        public Guid PartitionId
        {
            get;
            private set;
        }

        public long ReplicaId
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "GetReplicaLoadRequest for partition {0} filter {1} with timeout {2}", this.PartitionId, this.ReplicaId, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetReplicaLoadAsync(this.PartitionId, this.ReplicaId, this.Timeout, cancellationToken);
        }
    }
}


#pragma warning restore 1591