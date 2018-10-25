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
    /// <para>Contains the partition information for a Service Fabric Service.</para>
    /// </summary>
    public sealed class ServicePartitionList : PagedList<Partition>
    {
        /// <summary>
        /// <para>
        /// Creates a service partition list.
        /// </para>
        /// </summary>
        public ServicePartitionList() : base()
        {
        }

        internal ServicePartitionList(IList<Partition> list) : base(list)
        {
        }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe ServicePartitionList CreateFromNativeListResult(
            NativeClient.IFabricGetPartitionListResult2 result)
        {
            var nativeList = (NativeTypes.FABRIC_SERVICE_PARTITION_QUERY_RESULT_LIST*)result.get_PartitionList();
            var nativePagingStatus = (NativeTypes.FABRIC_PAGING_STATUS*)result.get_PagingStatus();

            var retval = CreateFromNativeList(nativeList, nativePagingStatus);
            GC.KeepAlive(result);

            return retval;
        }

        internal static unsafe ServicePartitionList CreateFromNativeList(
            NativeTypes.FABRIC_SERVICE_PARTITION_QUERY_RESULT_LIST* nativeList,
            NativeTypes.FABRIC_PAGING_STATUS* nativePagingStatus)
        {
            var retval = new ServicePartitionList();

            var nativeItemArray = (NativeTypes.FABRIC_SERVICE_PARTITION_QUERY_RESULT_ITEM*)nativeList->Items;
            for (int i = 0; i < nativeList->Count; ++i)
            {
                var nativeItem = *(nativeItemArray + i);
                var item = Partition.CreateFromNative(nativeItem);
                if (item != null)
                {
                    retval.Add(item);
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