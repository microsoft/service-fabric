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
    /// <para>Represents one page of <see cref="System.Fabric.Query.ApplicationType" /> retrieved by calling
    /// <see cref="System.Fabric.FabricClient.QueryClient.GetApplicationTypePagedListAsync()" />. Paged lists consist
    /// of the return results it holds (a list), along with a continuation token which may be used to access subsequent pages.</para>
    /// </summary>
    /// <remarks>
    ///     <para>
    ///         If there is a subsequent page, the continuation token provided in this object can be used to access it.
    ///         For continuation token usage instructions, see <see cref="System.Fabric.Description.PagedApplicationTypeQueryDescription.ContinuationToken" />.
    ///     </para>
    /// </remarks>
    public sealed class ApplicationTypePagedList : PagedList<ApplicationType>
    {
        /// <summary>
        /// <para>
        /// Creates an empty paged <see cref="System.Fabric.Query.ApplicationType" /> list.
        /// </para>
        /// </summary>
        public ApplicationTypePagedList() : base()
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Query.ApplicationTypePagedList" /> class. The continuation
        /// is null until explicitly set.</para>
        /// </summary>
        internal ApplicationTypePagedList(IList<ApplicationType> list) : base(list)
        {
        }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe ApplicationTypePagedList CreateFromNativeListResult(
            NativeClient.IFabricGetApplicationTypePagedListResult result)
        {
            var nativeApplicationTypePagedList = (NativeTypes.FABRIC_APPLICATION_TYPE_QUERY_RESULT_LIST*)result.get_ApplicationTypePagedList();
            var nativePagingStatus = (NativeTypes.FABRIC_PAGING_STATUS*)result.get_PagingStatus();

            var retval = CreateFromNativeList(nativeApplicationTypePagedList, nativePagingStatus);
            GC.KeepAlive(result);

            return retval;
        }

        internal static unsafe ApplicationTypePagedList CreateFromNativeList(
            NativeTypes.FABRIC_APPLICATION_TYPE_QUERY_RESULT_LIST* nativeList,
            NativeTypes.FABRIC_PAGING_STATUS* nativePagingStatus)
        {
            var retval = new ApplicationTypePagedList();

            if (nativeList != null)
            {
                var nativeItemArray = (NativeTypes.FABRIC_APPLICATION_TYPE_QUERY_RESULT_ITEM*)nativeList->Items;
                for (int i = 0; i < nativeList->Count; ++i)
                {
                    var nativeItem = *(nativeItemArray + i);
                    retval.Add(ApplicationType.CreateFromNative(nativeItem));
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