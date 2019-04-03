// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Fabric.Testability.Common;

    public abstract class ReplicaControlRequest : FabricRequest
    {
        public ReplicaControlRequest(IFabricClient fabricClient,
            string nodeName,
            Guid partitionId,
            long replicaOrInstanceId,
            TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Null(nodeName, "nodeName");

            this.NodeName = nodeName;
            this.PartitionId = partitionId;
            this.ReplicaOrInstanceId = replicaOrInstanceId;
        }

        public string NodeName { get; private set; }

        public Guid PartitionId { get; private set; }

        public long ReplicaOrInstanceId { get; private set; }
    }
}


#pragma warning restore 1591