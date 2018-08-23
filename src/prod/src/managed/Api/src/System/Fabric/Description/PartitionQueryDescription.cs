// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Interop;

    internal sealed class PartitionQueryDescription
    {
        public PartitionQueryDescription()
            : this(null, Guid.Empty)
        {
        }

        public PartitionQueryDescription(Uri serviceName, Guid partitionIdFilter)
        {
            this.ServiceName = serviceName;
            this.PartitionIdFilter = partitionIdFilter;
        }

        public PartitionQueryDescription(Guid partitionIdFilter)
        {
            this.PartitionIdFilter = partitionIdFilter;
        }

        public Uri ServiceName { get; set; }

        public Guid PartitionIdFilter { get; set; }

        public string ContinuationToken { get; set; }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_SERVICE_PARTITION_QUERY_DESCRIPTION();

            if (this.ServiceName != null)
                nativeDescription.ServiceName = pinCollection.AddObject(this.ServiceName);

            nativeDescription.PartitionIdFilter = PartitionIdFilter;

            var ex1 = new NativeTypes.FABRIC_SERVICE_PARTITION_QUERY_DESCRIPTION_EX1();
            ex1.ContinuationToken = pinCollection.AddObject(this.ContinuationToken);
            ex1.Reserved = IntPtr.Zero;

            nativeDescription.Reserved = pinCollection.AddBlittable(ex1);

            return pinCollection.AddBlittable(nativeDescription);
        }
    }

}