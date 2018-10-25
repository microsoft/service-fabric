// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;
    using System.Fabric.Query;

    internal sealed class ReplicaQueryDescription
    {
        public ReplicaQueryDescription()
            : this(Guid.Empty, 0, ServiceReplicaStatusFilter.Default)
        {
        }

        public ReplicaQueryDescription(Guid partitionId, Int64 replicaIdFilter, ServiceReplicaStatusFilter replicaStatusFilter)
        {
            this.PartitionId = partitionId;
            this.ReplicaIdFilter = replicaIdFilter;
            this.ReplicaStatusFilter = replicaStatusFilter;
        }

        public Guid PartitionId { get; set; }
        public Int64 ReplicaIdFilter { get; set; }
        public ServiceReplicaStatusFilter ReplicaStatusFilter { get; set; }

        public string ContinuationToken { get; set; }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_SERVICE_REPLICA_QUERY_DESCRIPTION();
            nativeDescription.ReplicaIdOrInstanceIdFilter = ReplicaIdFilter;
            nativeDescription.PartitionId = PartitionId;

            var ex1 = new NativeTypes.FABRIC_SERVICE_REPLICA_QUERY_DESCRIPTION_EX1();
            ex1.ReplicaStatusFilter = (UInt32)this.ReplicaStatusFilter;
            
            var ex2 = new NativeTypes.FABRIC_SERVICE_REPLICA_QUERY_DESCRIPTION_EX2();
            ex2.ContinuationToken = pinCollection.AddObject(this.ContinuationToken);
            ex2.Reserved = IntPtr.Zero;

            ex1.Reserved = pinCollection.AddBlittable(ex2);

            nativeDescription.Reserved = pinCollection.AddBlittable(ex1);

            return pinCollection.AddBlittable(nativeDescription);
        }
    }
}