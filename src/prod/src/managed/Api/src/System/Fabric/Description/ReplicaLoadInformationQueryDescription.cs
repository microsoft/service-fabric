// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;

    internal sealed class ReplicaLoadInformationQueryDescription
    {
        public ReplicaLoadInformationQueryDescription(Guid partitionId, long replicaOrInstanceId)
        {
            this.PartitionId = partitionId;
            this.ReplicaOrInstanceId = replicaOrInstanceId;
        }

        public Guid PartitionId { get; set; }
        public long ReplicaOrInstanceId { get; set; }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_REPLICA_LOAD_INFORMATION_QUERY_DESCRIPTION();
            nativeDescription.Reserved = IntPtr.Zero;

            nativeDescription.PartitionId = PartitionId;
            nativeDescription.ReplicaOrInstanceId = ReplicaOrInstanceId;
            return pinCollection.AddBlittable(nativeDescription);
        }
    }

}