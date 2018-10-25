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

    internal sealed class ComposeDeploymentStatusListWrapper : PagedList<ComposeDeploymentStatusResultItemWrapper>
    {
        public ComposeDeploymentStatusListWrapper() : base()
        {
        }

        internal ComposeDeploymentStatusListWrapper(IList<ComposeDeploymentStatusResultItemWrapper> list) : base(list)
        {
        }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe ComposeDeploymentStatusListWrapper CreateFromNativeListResult(
            NativeClient.IFabricGetComposeDeploymentStatusListResult result)
        {
            var nativeList = (NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_RESULT_LIST*)result.get_ComposeDeploymentStatusQueryList();
            var nativePagingStatus = (NativeTypes.FABRIC_PAGING_STATUS*)result.get_PagingStatus();

            var retval = CreateFromNativeList(nativeList, nativePagingStatus);
            GC.KeepAlive(result);

            return retval;
        }

        internal static unsafe ComposeDeploymentStatusListWrapper CreateFromNativeList(
            NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_RESULT_LIST* nativeList,
            NativeTypes.FABRIC_PAGING_STATUS* nativePagingStatus)
        {
            var retval = new ComposeDeploymentStatusListWrapper();

            if (nativeList != null)
            {
                var nativeItemArray = (NativeTypes.FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_RESULT_ITEM*)nativeList->Items;
                for (int i = 0; i < nativeList->Count; ++i)
                {
                    var nativeItem = *(nativeItemArray + i);
                    retval.Add(ComposeDeploymentStatusResultItemWrapper.CreateFromNative(nativeItem));
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