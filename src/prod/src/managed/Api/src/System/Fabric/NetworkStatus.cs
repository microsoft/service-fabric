// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Indicates status of container network. </para>
    /// </summary>
    public enum NetworkStatus
    {
        /// <summary>
        /// <para>Indicates container network status is invalid.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_NETWORK_STATUS.FABRIC_NETWORK_STATUS_INVALID,
        /// <summary>
        /// <para>Indicates container network is ready.</para>
        /// </summary>
        Ready = NativeTypes.FABRIC_NETWORK_STATUS.FABRIC_NETWORK_STATUS_READY,
        /// <summary>
        /// <para>Indicates container network is being created.</para>
        /// </summary>
        Creating = NativeTypes.FABRIC_NETWORK_STATUS.FABRIC_NETWORK_STATUS_CREATING,
        /// <summary>
        /// <para>Indicates container network is being deleted.</para>
        /// </summary>
        Deleting = NativeTypes.FABRIC_NETWORK_STATUS.FABRIC_NETWORK_STATUS_DELETING,
        /// <summary>
        /// <para>Indicates container network is being updated.</para>
        /// </summary>
        Updating = NativeTypes.FABRIC_NETWORK_STATUS.FABRIC_NETWORK_STATUS_UPDATING,
        /// <summary>
        /// <para>Indicates container network is in failed state.</para>
        /// </summary>
        Failed = NativeTypes.FABRIC_NETWORK_STATUS.FABRIC_NETWORK_STATUS_FAILED
    }
}
