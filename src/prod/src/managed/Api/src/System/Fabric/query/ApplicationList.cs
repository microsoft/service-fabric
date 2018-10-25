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
    /// <para>Represents the list of <see cref="System.Fabric.Query.Application" /> retrieved by calling
    /// <see cref="System.Fabric.FabricClient.QueryClient.GetApplicationListAsync(System.Uri)" />.</para>
    /// </summary>
    public sealed class ApplicationList : PagedList<Application>
    {
        /// <summary>
        /// <para> Creates an application list. </para>
        /// </summary>
        public ApplicationList() : base()
        {
        }

        internal ApplicationList(IList<Application> list) : base(list)
        {
        }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe ApplicationList CreateFromNativeListResult(
            NativeClient.IFabricGetApplicationListResult2 result)
        {
            var nativeList = (NativeTypes.FABRIC_APPLICATION_QUERY_RESULT_LIST*)result.get_ApplicationList();
            var nativePagingStatus = (NativeTypes.FABRIC_PAGING_STATUS*)result.get_PagingStatus();

            var retval = CreateFromNativeList(nativeList, nativePagingStatus);
            GC.KeepAlive(result);

            return retval;
        }

        internal static unsafe ApplicationList CreateFromNativeList(
            NativeTypes.FABRIC_APPLICATION_QUERY_RESULT_LIST* nativeList,
            NativeTypes.FABRIC_PAGING_STATUS* nativePagingStatus)
        {
            var retval = new ApplicationList();

            if (nativeList != null)
            {
                var nativeItemArray = (NativeTypes.FABRIC_APPLICATION_QUERY_RESULT_ITEM*)nativeList->Items;
                for (int i = 0; i < nativeList->Count; ++i)
                {
                    var nativeItem = *(nativeItemArray + i);
                    retval.Add(Application.CreateFromNative(nativeItem));
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