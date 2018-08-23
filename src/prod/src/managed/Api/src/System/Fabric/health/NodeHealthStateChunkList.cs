// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using Interop;
    using System.Collections.Generic;

    /// <summary>
    /// Represents a list that contains <see cref="System.Fabric.Health.NodeHealthStateChunk"/> items.
    /// </summary>
    /// <remarks>The list can be obtained through health state chunk queries. The queries may have as result more chunks that can fit a message.
    /// In this case, the list of items that fit the message is returned plus a count that shows how many total items are available.</remarks>
    public sealed class NodeHealthStateChunkList : HealthStateChunkList<NodeHealthStateChunk>
    {
        /// <summary>
        /// Instantiates an empty <see cref="System.Fabric.Health.NodeHealthStateChunkList"/>.
        /// </summary>
        public NodeHealthStateChunkList() : base()
        {
        }

        internal NodeHealthStateChunkList(IList<NodeHealthStateChunk> list) : base(list)
        {
        }

        internal static unsafe NodeHealthStateChunkList CreateFromNativeList(IntPtr nativeListPtr)
        {
            var retval = new NodeHealthStateChunkList();

            if (nativeListPtr != null)
            {
                var nativeList = (NativeTypes.FABRIC_NODE_HEALTH_STATE_CHUNK_LIST*)nativeListPtr;

                var nativeItemArray = (NativeTypes.FABRIC_NODE_HEALTH_STATE_CHUNK*)nativeList->Items;
                for (int i = 0; i < nativeList->Count; ++i)
                {
                    var nativeItem = *(nativeItemArray + i);
                    retval.Add(NodeHealthStateChunk.FromNative(nativeItem));
                }

                retval.TotalCount = (long)nativeList->TotalCount;
            }
            
            return retval;
        }
    }
}