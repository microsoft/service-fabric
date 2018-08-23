// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using Interop;
    using System.Collections.Generic;

    /// <summary>
    /// Represents a list that contains <see cref="System.Fabric.Health.DeployedApplicationHealthStateChunk"/> items.
    /// </summary>
    /// <remarks>The list can be obtained through health state chunk queries. The queries may have as result more chunks that can fit a message.
    /// In this case, the list of items that fit the message is returned plus a count that shows how many total items are available.</remarks>
    public sealed class DeployedApplicationHealthStateChunkList : HealthStateChunkList<DeployedApplicationHealthStateChunk>
    {
        /// <summary>
        /// Instantiates an empty <see cref="System.Fabric.Health.DeployedApplicationHealthStateChunkList"/>.
        /// </summary>
        public DeployedApplicationHealthStateChunkList() : base()
        {
        }

        internal DeployedApplicationHealthStateChunkList(IList<DeployedApplicationHealthStateChunk> list) : base(list)
        {
        }

        internal static unsafe DeployedApplicationHealthStateChunkList CreateFromNativeList(IntPtr nativeListPtr)
        {
            var retval = new DeployedApplicationHealthStateChunkList();

            if (nativeListPtr != null)
            {
                var nativeList = (NativeTypes.FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_CHUNK_LIST*)nativeListPtr;

                var nativeItemArray = (NativeTypes.FABRIC_DEPLOYED_APPLICATION_HEALTH_STATE_CHUNK*)nativeList->Items;
                for (int i = 0; i < nativeList->Count; ++i)
                {
                    var nativeItem = *(nativeItemArray + i);
                    retval.Add(DeployedApplicationHealthStateChunk.FromNative(nativeItem));
                }

                retval.TotalCount = (long)nativeList->TotalCount;
            }
            
            return retval;
        }
    }
}