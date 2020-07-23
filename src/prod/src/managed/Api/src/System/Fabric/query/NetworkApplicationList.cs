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
    /// <para>Represents one page of <see cref="System.Fabric.Query.NetworkApplication" /> retrieved by calling
    /// <see cref="System.Fabric.FabricClient.NetworkManagementClient.GetNetworkApplicationListAsync(Description.NetworkApplicationQueryDescription)" />. Paged lists consist
    /// of the return results it holds (a list), along with a continuation token which may be used to access subsequent pages.</para>
    /// </summary>
    public sealed class NetworkApplicationList : PagedList<NetworkApplication>
    {
        internal NetworkApplicationList() : base()
        {
        }

        internal NetworkApplicationList(IList<NetworkApplication> list) : base(list)
        {
        }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe NetworkApplicationList CreateFromNativeListResult(
            NativeClient.IFabricGetNetworkApplicationListResult result)
        {
            var nativeNetworkApplicationList = (NativeTypes.FABRIC_NETWORK_APPLICATION_QUERY_RESULT_LIST*)result.get_NetworkApplicationList();
            var nativePagingStatus = (NativeTypes.FABRIC_PAGING_STATUS*)result.get_PagingStatus();

            var retval = CreateFromNativeList(nativeNetworkApplicationList, nativePagingStatus);
            GC.KeepAlive(result);

            return retval;
        }

        internal static unsafe NetworkApplicationList CreateFromNativeList(
            NativeTypes.FABRIC_NETWORK_APPLICATION_QUERY_RESULT_LIST* nativeList,
            NativeTypes.FABRIC_PAGING_STATUS* nativePagingStatus)
        {
            var retval = new NetworkApplicationList();

            if (nativeList != null)
            {
                var nativeItemArray = (NativeTypes.FABRIC_NETWORK_APPLICATION_QUERY_RESULT_ITEM*)nativeList->Items;
                for (int i = 0; i < nativeList->Count; ++i)
                {
                    var nativeItem = *(nativeItemArray + i);
                    retval.Add(NetworkApplication.CreateFromNative(nativeItem));
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