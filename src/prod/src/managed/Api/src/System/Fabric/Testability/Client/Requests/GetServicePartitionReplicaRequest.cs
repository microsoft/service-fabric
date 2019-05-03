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

    [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Performance", "CA1812:AvoidUninstantiatedInternalClasses", Justification = "TBD")]
    public class GetServicePartitionReplicaRequest : FabricRequest
    {
        public GetServicePartitionReplicaRequest(Guid partitionId, long id, bool isStateful, IFabricClient fabricClient, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            this.PartitionId = partitionId;
            this.Id = id;
            this.IsStateful = isStateful;
        }

        public Guid PartitionId
        {
            get;
            private set;
        }

        public long Id
        {
            get;
            private set;
        }

        public bool IsStateful
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "GetServicePartitionReplica for partition {0} replica {1} with timeout {2}", this.PartitionId, this.Id, this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetServicePartitionReplicaAsync(this.PartitionId, this.Id, this.IsStateful, this.Timeout, cancellationToken);
        }
    }
}

#pragma warning restore 1591