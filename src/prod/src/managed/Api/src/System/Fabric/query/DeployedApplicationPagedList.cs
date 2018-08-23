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
    /// <para>Represents the list of <see cref="System.Fabric.Query.DeployedApplication" />.
    /// Paged lists consist of the return results it holds (a list), along with a continuation token which may be used to access subsequent pages.</para>
    /// </summary>
    /// <remarks>
    ///     <para>
    ///         If there is a subsequent page, the continuation token provided in this object can be used to access it.
    ///         For continuation token usage instructions, see <see cref="System.Fabric.Description.PagedQueryDescriptionBase.ContinuationToken" />.
    ///     </para>
    /// </remarks>
    public sealed class DeployedApplicationPagedList : PagedList<DeployedApplication>
    {
        /// <summary>
        /// <para>
        /// Creates an empty paged <see cref="System.Fabric.Query.DeployedApplication" /> list.
        /// </para>
        /// </summary>
        public DeployedApplicationPagedList() : base()
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Query.DeployedApplicationPagedList" /> class. The continuation
        /// token is null until explicitly set.</para>
        /// </summary>
        internal DeployedApplicationPagedList(IList<DeployedApplication> list) : base(list)
        {
        }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe DeployedApplicationPagedList CreateFromNativeListResult(
            NativeClient.IFabricGetDeployedApplicationPagedListResult result)
        {
            var nativePagedList = (NativeTypes.FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_LIST*)result.get_DeployedApplicationPagedList();
            var nativePagingStatus = (NativeTypes.FABRIC_PAGING_STATUS*)result.get_PagingStatus();

            var retval = CreateFromNativeList(nativePagedList, nativePagingStatus);
            GC.KeepAlive(result);

            return retval;
        }

        internal static unsafe DeployedApplicationPagedList CreateFromNativeList(
            NativeTypes.FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_LIST* nativeList,
            NativeTypes.FABRIC_PAGING_STATUS* nativePagingStatus)
        {
            var retval = new DeployedApplicationPagedList();

            var nativeItemArray = (NativeTypes.FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_ITEM*)nativeList->Items;
            for (int i = 0; i < nativeList->Count; ++i)
            {
                var nativeItem = *(nativeItemArray + i);
                retval.Add(DeployedApplication.CreateFromNative(nativeItem));
            }

            if (nativePagingStatus != null)
            {
                retval.ContinuationToken = NativeTypes.FromNativeString(nativePagingStatus->ContinuationToken);
            }

            return retval;
        }
    }
}