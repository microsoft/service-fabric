// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests.Internal
{
    using System;
    using System.Globalization;
    using System.Fabric.Testability.Common;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Testability.Client.Structures;

    public class RemoveReplicaRequest : ReplicaControlRequest
    {
        private readonly bool isForce;

        public RemoveReplicaRequest(IFabricClient fabricClient,
            string nodeName,
            Guid partitionId,
            long replicaOrInstanceId,
            bool isForce,
            TimeSpan timeout)
            : base(fabricClient, nodeName, partitionId, replicaOrInstanceId, timeout)
        {
            this.isForce = isForce;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Remove Replica with node {0}, partitionId: {1}, replicaId {2}, isForce {3} and timeout {4}", this.NodeName, this.PartitionId, this.ReplicaOrInstanceId, this.isForce, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.RemoveReplicaInternalAsync(
                this.NodeName,
                this.PartitionId,
                this.ReplicaOrInstanceId,
                this.isForce,
                this.Timeout,
                cancellationToken);
        }
    }
}


#pragma warning restore 1591