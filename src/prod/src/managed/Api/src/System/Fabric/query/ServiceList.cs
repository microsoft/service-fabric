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
    /// <para>Represents a list of services retrieved by calling <see cref="System.Fabric.FabricClient.QueryClient.GetServiceListAsync(System.Uri)" />.</para>
    /// </summary>
    public sealed class ServiceList : PagedList<Service>
    {
        /// <summary>
        /// <para>
        /// Creates a service list.
        /// </para>
        /// </summary>
        public ServiceList() : base()
        {
        }

        internal ServiceList(IList<Service> list) : base(list)
        {
        }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe ServiceList CreateFromNativeListResult(
            NativeClient.IFabricGetServiceListResult2 result)
        {
            var nativeList = (NativeTypes.FABRIC_SERVICE_QUERY_RESULT_LIST*)result.get_ServiceList();
            var nativePagingStatus = (NativeTypes.FABRIC_PAGING_STATUS*)result.get_PagingStatus();

            var retval = CreateFromNativeList(nativeList, nativePagingStatus);
            GC.KeepAlive(result);

            return retval;
        }

        internal static unsafe ServiceList CreateFromNativeList(
            NativeTypes.FABRIC_SERVICE_QUERY_RESULT_LIST* nativeList,
            NativeTypes.FABRIC_PAGING_STATUS* nativePagingStatus)
        {
            var retval = new ServiceList();

            if (nativeList != null)
            {
                var nativeItemArray = (NativeTypes.FABRIC_SERVICE_QUERY_RESULT_ITEM*)nativeList->Items;
                for (int i = 0; i < nativeList->Count; ++i)
                {
                    var nativeItem = *(nativeItemArray + i);
                    retval.Add(Service.CreateFromNative(nativeItem));
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