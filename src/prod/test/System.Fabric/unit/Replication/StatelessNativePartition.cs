// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Test.Replication
{
    using System;
    using System.Fabric.Interop;

    internal class StatelessNativePartition : Stubs.StatelessPartitionStubBase
    {
        private static PinCollection pinCollection;

        static StatelessNativePartition()
        {
            StatelessNativePartition.pinCollection = new PinCollection();
            
            NativeTypes.FABRIC_SINGLETON_PARTITION_INFORMATION singletonPartitionInfo = new NativeTypes.FABRIC_SINGLETON_PARTITION_INFORMATION
            {
                Id = Guid.NewGuid()
            };

            StatelessNativePartition.pinCollection.AddBlittable(singletonPartitionInfo);

            StatelessNativePartition.pinCollection.AddBlittable(
                new NativeTypes.FABRIC_SERVICE_PARTITION_INFORMATION
                {
                    Kind = NativeTypes.FABRIC_SERVICE_PARTITION_KIND.FABRIC_SERVICE_PARTITION_KIND_SINGLETON,
                    Value = StatelessNativePartition.pinCollection.AddrOfPinnedObject()
                }
            );
        }

        public override IntPtr GetPartitionInfo()
        {
            return StatelessNativePartition.pinCollection.AddrOfPinnedObject();
        }
    }
}