// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Enumerates the states of an upgrade domain.</para>
    /// </summary>
    public enum UpgradeDomainState
    {
        /// <summary>
        /// <para>Indicates that this upgrade is not valid. All Service Fabric enumerations have the invalid type.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_UPGRADE_DOMAIN_STATE.FABRIC_UPGRADE_DOMAIN_STATE_INVALID,
        /// <summary>
        /// <para>Indicates that the upgrade is pending and has not started.</para>
        /// </summary>
        Pending = NativeTypes.FABRIC_UPGRADE_DOMAIN_STATE.FABRIC_UPGRADE_DOMAIN_STATE_PENDING,
        /// <summary>
        /// <para>Indicates that the upgrade is in progress.</para>
        /// </summary>
        InProgress = NativeTypes.FABRIC_UPGRADE_DOMAIN_STATE.FABRIC_UPGRADE_DOMAIN_STATE_IN_PROGRESS,
        /// <summary>
        /// <para>Indicates that the upgrade is completed.</para>
        /// </summary>
        Completed = NativeTypes.FABRIC_UPGRADE_DOMAIN_STATE.FABRIC_UPGRADE_DOMAIN_STATE_COMPLETED
    }
}