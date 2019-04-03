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
    /// <para>Represents one page of <see cref="System.Fabric.Query.DeployedNetworkCodePackage" /> retrieved by calling
    /// <see cref="System.Fabric.FabricClient.NetworkManagementClient.GetDeployedNetworkCodePackageListAsync(Description.DeployedNetworkCodePackageQueryDescription)" />. Paged lists consist
    /// of the return results it holds (a list), along with a continuation token which may be used to access subsequent pages.</para>
    /// </summary>
    public sealed class DeployedNetworkCodePackageList : PagedList<DeployedNetworkCodePackage>
    {
        internal DeployedNetworkCodePackageList() : base()
        {
        }

        internal DeployedNetworkCodePackageList(IList<DeployedNetworkCodePackage> list) : base(list)
        {
        }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe DeployedNetworkCodePackageList CreateFromNativeListResult(
            NativeClient.IFabricGetDeployedNetworkCodePackageListResult result)
        {
            var nativeDeployedNetworkCodePackageList = (NativeTypes.FABRIC_DEPLOYED_NETWORK_CODE_PACKAGE_QUERY_RESULT_LIST*)result.get_DeployedNetworkCodePackageList();
            var nativePagingStatus = (NativeTypes.FABRIC_PAGING_STATUS*)result.get_PagingStatus();

            var retval = CreateFromNativeList(nativeDeployedNetworkCodePackageList, nativePagingStatus);
            GC.KeepAlive(result);

            return retval;
        }

        internal static unsafe DeployedNetworkCodePackageList CreateFromNativeList(
            NativeTypes.FABRIC_DEPLOYED_NETWORK_CODE_PACKAGE_QUERY_RESULT_LIST* nativeList,
            NativeTypes.FABRIC_PAGING_STATUS* nativePagingStatus)
        {
            var retval = new DeployedNetworkCodePackageList();

            if (nativeList != null)
            {
                var nativeItemArray = (NativeTypes.FABRIC_DEPLOYED_NETWORK_CODE_PACKAGE_QUERY_RESULT_ITEM*)nativeList->Items;
                for (int i = 0; i < nativeList->Count; ++i)
                {
                    var nativeItem = *(nativeItemArray + i);
                    retval.Add(DeployedNetworkCodePackage.CreateFromNative(nativeItem));
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