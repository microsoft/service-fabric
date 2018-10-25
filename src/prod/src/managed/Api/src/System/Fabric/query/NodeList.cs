// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Collections.Generic;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a list of Service Fabric nodes retrieved by calling <see cref="System.Fabric.FabricClient.QueryClient.GetNodeListAsync()" />.</para>
    /// </summary>
    public sealed class NodeList : PagedList<Node>
    {
        /// <summary>
        /// <para>
        /// Creates an empty node list.
        /// </para>
        /// </summary>
        public NodeList() : base()
        {
        }

        internal NodeList(IList<Node> list) : base(list)
        {
        }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe NodeList CreateFromNativeListResult(
            NativeClient.IFabricGetNodeListResult2 result)
        {
            var nativeNodeList = (NativeTypes.FABRIC_NODE_QUERY_RESULT_LIST*)result.get_NodeList();
            var nativePagingStatus = (NativeTypes.FABRIC_PAGING_STATUS*)result.get_PagingStatus();

            var retval = CreateFromNativeList(nativeNodeList, nativePagingStatus);
            GC.KeepAlive(result);

            return retval;
        }

        internal static unsafe NodeList CreateFromNativeList(
            NativeTypes.FABRIC_NODE_QUERY_RESULT_LIST* nativeList,
            NativeTypes.FABRIC_PAGING_STATUS* nativePagingStatus)
        {
            var retval = new NodeList();

            if (nativeList != null)
            {
                var nativeItemArray = (NativeTypes.FABRIC_NODE_QUERY_RESULT_ITEM*)nativeList->Items;
                for (int i = 0; i < nativeList->Count; ++i)
                {
                    var nativeItem = *(nativeItemArray + i);
                    retval.Add(Node.CreateFromNative(nativeItem));
                }
            }

            if (nativePagingStatus != null)
            {
                retval.ContinuationToken = NativeTypes.FromNativeString(nativePagingStatus->ContinuationToken);
            }

            return retval;
        }
    }
}