// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Repair
{
    using Interop;

    /// <summary>
    /// <para>Specifies the workflow state of a repair task's health check when the repair 
    /// task enters either the Preparing or Restoring state.</para>
    /// </summary>
    /// <remarks>Separate health checks are done when a repair task enters the Preparing and Restoring states.</remarks>
    public enum RepairTaskHealthCheckState : int
    {
        /// <summary>
        /// Indicates that the health check hasn't started.
        /// </summary>
        NotStarted = NativeTypes.FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE.FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE_NOT_STARTED,

        /// <summary>
        /// Indicates that the health check is in progress
        /// </summary>
        InProgress = NativeTypes.FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE.FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE_IN_PROGRESS,

        /// <summary>
        /// Indicates that the health check succeeded.
        /// </summary>
        Succeeded = NativeTypes.FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE.FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE_SUCCEEDED,

        /// <summary>
        /// Indicates that the health check was skipped.
        /// </summary>
        Skipped = NativeTypes.FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE.FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE_SKIPPED,

        /// <summary>
        /// Indicates that the health check timed out.
        /// </summary>
        TimedOut = NativeTypes.FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE.FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE_TIMEDOUT,
    }
}