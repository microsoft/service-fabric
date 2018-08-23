// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    internal sealed class ServiceNameQueryDescription
    {
        public ServiceNameQueryDescription(Guid partitionId)
        {
            Requires.Argument<Guid>("partitionId", partitionId).NotNull();
            this.PartitionId = partitionId;
        }

        public Guid PartitionId { get; private set; }

        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeDescription = new NativeTypes.FABRIC_SERVICE_NAME_QUERY_DESCRIPTION();

            nativeDescription.PartitionId = this.PartitionId;
            nativeDescription.Reserved = IntPtr.Zero;
            return pinCollection.AddBlittable(nativeDescription);
        }
    }
}