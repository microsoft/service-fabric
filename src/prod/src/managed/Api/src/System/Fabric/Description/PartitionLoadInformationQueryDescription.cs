// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;

    internal sealed class PartitionLoadInformationQueryDescription
    {
        public PartitionLoadInformationQueryDescription()
            : this(Guid.Empty)
        {
        }

        public PartitionLoadInformationQueryDescription(Guid partitionId)
        {
            this.PartitionId = partitionId;
        }

        public Guid PartitionId { get; set; }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_PARTITION_LOAD_INFORMATION_QUERY_DESCRIPTION();
            nativeDescription.Reserved = IntPtr.Zero;

            nativeDescription.PartitionId = PartitionId;
            return pinCollection.AddBlittable(nativeDescription);
        }
    }

}