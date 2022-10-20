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
    /// <para>Represents one page of <see cref="System.Fabric.Query.NetworkNode" /> retrieved by calling
    /// <see cref="System.Fabric.FabricClient.NetworkManagementClient.GetNetworkNodeListAsync(Description.NetworkNodeQueryDescription)" />. Paged lists consist
    /// of the return results it holds (a list), along with a continuation token which may be used to access subsequent pages.</para>
    /// </summary>
    public sealed class NetworkNodeList : PagedList<NetworkNode>
    {
        internal NetworkNodeList() : base()
        {
        }

        internal NetworkNodeList(IList<NetworkNode> list) : base(list)
        {
        }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe NetworkNodeList CreateFromNativeListResult(
            NativeClient.IFabricGetNetworkNodeListResult result)
        {
            var nativeNetworkNodeList = (NativeTypes.FABRIC_NETWORK_NODE_QUERY_RESULT_LIST*)result.get_NetworkNodeList();
            var nativePagingStatus = (NativeTypes.FABRIC_PAGING_STATUS*)result.get_PagingStatus();

            var retval = CreateFromNativeList(nativeNetworkNodeList, nativePagingStatus);
            GC.KeepAlive(result);

            return retval;
        }

        internal static unsafe NetworkNodeList CreateFromNativeList(
            NativeTypes.FABRIC_NETWORK_NODE_QUERY_RESULT_LIST* nativeList,
            NativeTypes.FABRIC_PAGING_STATUS* nativePagingStatus)
        {
            var retval = new NetworkNodeList();

            if (nativeList != null)
            {
                var nativeItemArray = (NativeTypes.FABRIC_NETWORK_NODE_QUERY_RESULT_ITEM*)nativeList->Items;
                for (int i = 0; i < nativeList->Count; ++i)
                {
                    var nativeItem = *(nativeItemArray + i);
                    retval.Add(NetworkNode.CreateFromNative(nativeItem));
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