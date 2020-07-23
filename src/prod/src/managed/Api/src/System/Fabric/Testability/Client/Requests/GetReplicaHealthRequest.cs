// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Fabric.Description;
    using System.Fabric.Health;
    using System.Fabric.Testability.Client.Structures;
    using System.Fabric.Testability.Common;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public class GetReplicaHealthRequest : FabricRequest
    {
        public GetReplicaHealthRequest(IFabricClient fabricClient, ReplicaHealthQueryDescription queryDescription, TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(queryDescription, "queryDescription");
            this.QueryDescription = queryDescription;
        }

        public ReplicaHealthQueryDescription QueryDescription
        {
            get;
            private set;
        }

        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "Get replica health {0}:{1} with timeout {2}",
                this.QueryDescription.PartitionId,
                this.QueryDescription.ReplicaOrInstanceId,
                this.Timeout);
        }

        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.GetReplicaHealthAsync(this.QueryDescription, this.Timeout, cancellationToken);
        }
    }
}

#pragma warning restore 1591