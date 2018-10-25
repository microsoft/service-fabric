// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using Interop;
    using System.Collections.Generic;

    /// <summary>
    /// Represents a list that contains <see cref="System.Fabric.Health.PartitionHealthStateChunk"/> items.
    /// </summary>
    /// <remarks>The list can be obtained through health state chunk queries. The queries may have as result more chunks that can fit a message.
    /// In this case, the list of items that fit the message is returned plus a count that shows how many total items are available.</remarks>
    public sealed class PartitionHealthStateChunkList : HealthStateChunkList<PartitionHealthStateChunk>
    {
        /// <summary>
        /// Instantiates an empty <see cref="System.Fabric.Health.PartitionHealthStateChunkList"/>.
        /// </summary>
        public PartitionHealthStateChunkList() : base()
        {
        }

        internal PartitionHealthStateChunkList(IList<PartitionHealthStateChunk> list) : base(list)
        {
        }

        internal static unsafe PartitionHealthStateChunkList CreateFromNativeList(IntPtr nativeListPtr)
        {
            var retval = new PartitionHealthStateChunkList();

            if (nativeListPtr != IntPtr.Zero)
            {
                var nativeList = (NativeTypes.FABRIC_PARTITION_HEALTH_STATE_CHUNK_LIST*)nativeListPtr;

                var nativeItemArray = (NativeTypes.FABRIC_PARTITION_HEALTH_STATE_CHUNK*)nativeList->Items;
                for (int i = 0; i < nativeList->Count; ++i)
                {
                    var nativeItem = *(nativeItemArray + i);
                    retval.Add(PartitionHealthStateChunk.FromNative(nativeItem));
                }

                retval.TotalCount = (long)nativeList->TotalCount;
            }
            
            return retval;
        }
    }
}