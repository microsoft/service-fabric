// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;

    internal sealed class DeployedServiceReplicaDetailQueryDescription
    {
        public DeployedServiceReplicaDetailQueryDescription(string nodeName, Guid partitionId, long replicaId)
        {
            this.NodeName = nodeName;
            this.PartitionId = partitionId;
            this.ReplicaId = replicaId;
        }

        public string NodeName { get; set; }
        public Guid PartitionId { get; set; }
        public long ReplicaId { get; set; }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_DEPLOYED_SERVICE_REPLICA_DETAIL_QUERY_DESCRIPTION();
            nativeDescription.Reserved = IntPtr.Zero;

            nativeDescription.NodeName = pinCollection.AddObject(this.NodeName);
            nativeDescription.PartitionId = this.PartitionId;
            nativeDescription.ReplicaId = this.ReplicaId;
            return pinCollection.AddBlittable(nativeDescription);
        }
    }
}