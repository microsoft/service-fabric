// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Specified the status for a node deactivation task.</para>
    /// </summary>
    public enum NodeDeactivationStatus
    {
        /// <summary>
        /// <para>No status is associated with the task.</para>
        /// </summary>
        None = NativeTypes.FABRIC_NODE_DEACTIVATION_STATUS.FABRIC_NODE_DEACTIVATION_STATUS_NONE,
        /// <summary>
        /// <para>Safety checks are in progress for the task.</para>
        /// </summary>
        SafetyCheckInProgress = NativeTypes.FABRIC_NODE_DEACTIVATION_STATUS.FABRIC_NODE_DEACTIVATION_STATUS_SAFETY_CHECK_IN_PROGRESS,
        /// <summary>
        /// <para>All the safety checks have been completed for the task.</para>
        /// </summary>
        SafetyCheckComplete = NativeTypes.FABRIC_NODE_DEACTIVATION_STATUS.FABRIC_NODE_DEACTIVATION_STATUS_SAFETY_CHECK_COMPLETE,
        /// <summary>
        /// <para>The task is completed.</para>
        /// </summary>
        Completed = NativeTypes.FABRIC_NODE_DEACTIVATION_STATUS.FABRIC_NODE_DEACTIVATION_STATUS_COMPLETED
    }
}