// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Enumerates the filters used for matching the status of container networks that should be returned by query.</para>
    /// </summary>
    public enum NetworkStatusFilter
    {
        /// <summary>
        /// <para>Indicates no filter is added on network status.</para>
        /// </summary>
        Default = NativeTypes.FABRIC_NETWORK_STATUS_FILTER.FABRIC_NETWORK_STATUS_FILTER_DEFAULT,
        /// <summary>
        /// <para>Indicates no filter is added on network status.</para>
        /// </summary>
        All = NativeTypes.FABRIC_NETWORK_STATUS_FILTER.FABRIC_NETWORK_STATUS_FILTER_ALL,
        /// <summary>
        /// <para>Indicates a filter that matches container networks that are ready.</para>
        /// </summary>
        Ready = NativeTypes.FABRIC_NETWORK_STATUS_FILTER.FABRIC_NETWORK_STATUS_FILTER_READY,
        /// <summary>
        /// <para>Indicates a filter that matches container networks that are being created.</para>
        /// </summary>
        Creating = NativeTypes.FABRIC_NETWORK_STATUS_FILTER.FABRIC_NETWORK_STATUS_FILTER_CREATING,
        /// <summary>
        /// <para>Indicates a filter that matches container networks that are being deleted.</para>
        /// </summary>
        Deleting = NativeTypes.FABRIC_NETWORK_STATUS_FILTER.FABRIC_NETWORK_STATUS_FILTER_DELETING,
        /// <summary>
        /// <para>Indicates a filter that matches container networks that are being updated.</para>
        /// </summary>
        Updating = NativeTypes.FABRIC_NETWORK_STATUS_FILTER.FABRIC_NETWORK_STATUS_FILTER_UPDATING,
        /// <summary>
        /// <para>Indicates a filter that matches container networks that are in a failed state.</para>
        /// </summary>
        Failed = NativeTypes.FABRIC_NETWORK_STATUS_FILTER.FABRIC_NETWORK_STATUS_FILTER_FAILED
    }
}
