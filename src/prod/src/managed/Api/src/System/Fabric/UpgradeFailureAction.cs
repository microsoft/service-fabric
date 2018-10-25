// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Enumerates the actions for Service Fabric to take if the upgrade fails.</para>
    /// </summary>
    public enum UpgradeFailureAction
    {
        /// <summary>
        /// <para>Indicates that this upgrade is not valid. All Service Fabric enumerations have the invalid type.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_MONITORED_UPGRADE_FAILURE_ACTION.FABRIC_MONITORED_UPGRADE_FAILURE_ACTION_INVALID,
        /// <summary>
        /// <para>Indicates that a rollback of the upgrade will be performed by Service Fabric if the upgrade fails.</para>
        /// </summary>
        Rollback = NativeTypes.FABRIC_MONITORED_UPGRADE_FAILURE_ACTION.FABRIC_MONITORED_UPGRADE_FAILURE_ACTION_ROLLBACK,
        /// <summary>
        /// <para>Indicates that a manual repair will need to be performed by the administrator if the upgrade fails. Service Fabric will 
        /// not proceed to the next upgrade domain automatically.</para>
        /// </summary>
        Manual = NativeTypes.FABRIC_MONITORED_UPGRADE_FAILURE_ACTION.FABRIC_MONITORED_UPGRADE_FAILURE_ACTION_MANUAL,
    }
}