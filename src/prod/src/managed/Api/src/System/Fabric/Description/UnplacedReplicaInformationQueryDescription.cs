// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;

    internal sealed class UnplacedReplicaInformationQueryDescription
    {
        public UnplacedReplicaInformationQueryDescription(string serviceName, Guid partitionId, bool onlyQueryPrimaries)
        {
            this.ServiceName = serviceName;
            this.PartitionId = partitionId;
            this.OnlyQueryPrimaries = onlyQueryPrimaries;
        }
        public string ServiceName { get; set; }
        public Guid PartitionId { get; set; }
        public bool OnlyQueryPrimaries { get; set; }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_UNPLACED_REPLICA_INFORMATION_QUERY_DESCRIPTION();
            nativeDescription.Reserved = IntPtr.Zero;

            nativeDescription.ServiceName = pinCollection.AddObject(this.ServiceName);
            nativeDescription.PartitionId = PartitionId;
            nativeDescription.OnlyQueryPrimaries = NativeTypes.ToBOOLEAN(OnlyQueryPrimaries);
            return pinCollection.AddBlittable(nativeDescription);
        }
    }
}