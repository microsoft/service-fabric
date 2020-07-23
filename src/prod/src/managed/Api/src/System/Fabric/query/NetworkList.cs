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
    /// <para>Represents one page of <see cref="System.Fabric.Query.NetworkInformation" /> retrieved by calling
    /// <see cref="System.Fabric.FabricClient.NetworkManagementClient.GetNetworkListAsync()" />. Paged lists consist
    /// of the return results it holds (a list), along with a continuation token which may be used to access subsequent pages.</para>
    /// </summary>
    public sealed class NetworkList : PagedList<NetworkInformation>
    {
        internal NetworkList() : base()
        {
        }

        internal NetworkList(IList<NetworkInformation> list) : base(list)
        {
        }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe NetworkList CreateFromNativeListResult(
            NativeClient.IFabricGetNetworkListResult result)
        {
            var nativeNetworkList = (NativeTypes.FABRIC_NETWORK_QUERY_RESULT_LIST*)result.get_NetworkList();
            var nativePagingStatus = (NativeTypes.FABRIC_PAGING_STATUS*)result.get_PagingStatus();

            var retval = CreateFromNativeList(nativeNetworkList, nativePagingStatus);
            GC.KeepAlive(result);

            return retval;
        }

        internal static unsafe NetworkList CreateFromNativeList(
            NativeTypes.FABRIC_NETWORK_QUERY_RESULT_LIST* nativeList,
            NativeTypes.FABRIC_PAGING_STATUS* nativePagingStatus)
        {
            var retval = new NetworkList();

            if (nativeList != null)
            {
                var nativeItemArray = (NativeTypes.FABRIC_NETWORK_INFORMATION*)nativeList->Items;
                for (int i = 0; i < nativeList->Count; ++i)
                {
                    var nativeItem = *(nativeItemArray + i);
                    retval.Add(NetworkInformation.CreateFromNative(nativeItem));
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