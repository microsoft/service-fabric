// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Repair
{
    using System;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Specifies the workflow state of a repair task.</para>
    /// <para>This type supports the Service Fabric platform; it is not meant to be used directly from your code.</para>
    /// </summary>
    public enum RepairTaskState
    {
        /// <summary>
        /// <para>Indicates that the repair task state is invalid.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_REPAIR_TASK_STATE.FABRIC_REPAIR_TASK_STATE_INVALID,

        /// <summary>
        /// <para>Indicates that the repair task has been created.</para>
        /// </summary>
        Created = NativeTypes.FABRIC_REPAIR_TASK_STATE.FABRIC_REPAIR_TASK_STATE_CREATED,

        /// <summary>
        /// <para>Indicates that the repair task has been claimed by a repair executor.</para>
        /// </summary>
        Claimed = NativeTypes.FABRIC_REPAIR_TASK_STATE.FABRIC_REPAIR_TASK_STATE_CLAIMED,

        /// <summary>
        /// <para>Indicates that the Repair Manager is preparing the system to handle the impact of the repair task, usually by 
        /// taking resources offline gracefully.</para>
        /// </summary>
        Preparing = NativeTypes.FABRIC_REPAIR_TASK_STATE.FABRIC_REPAIR_TASK_STATE_PREPARING,

        /// <summary>
        /// <para>Indicates that the repair task has been approved by the Repair Manager and is safe to execute.</para>
        /// </summary>
        Approved = NativeTypes.FABRIC_REPAIR_TASK_STATE.FABRIC_REPAIR_TASK_STATE_APPROVED,

        /// <summary>
        /// <para>Indicates that execution of the repair task is in progress.</para>
        /// </summary>
        Executing = NativeTypes.FABRIC_REPAIR_TASK_STATE.FABRIC_REPAIR_TASK_STATE_EXECUTING,

        /// <summary>
        /// <para>Indicates that the Repair Manager is restoring the system to its pre-repair state, usually by bringing 
        /// resources back online.</para>
        /// </summary>
        Restoring = NativeTypes.FABRIC_REPAIR_TASK_STATE.FABRIC_REPAIR_TASK_STATE_RESTORING,

        /// <summary>
        /// <para>Indicates that the repair task has completed, and no further state changes will occur.</para>
        /// </summary>
        Completed = NativeTypes.FABRIC_REPAIR_TASK_STATE.FABRIC_REPAIR_TASK_STATE_COMPLETED,
    }

    /// <summary>
    /// <para>Specifies values that can be combined as a bitmask to retrieve repair tasks filtered by their current workflow state.</para>
    /// </summary>
    [Flags]
    public enum RepairTaskStateFilter
    {
        /// <summary>
        /// <para>Includes all repair tasks, regardless of state.</para>
        /// </summary>
        Default = NativeTypes.FABRIC_REPAIR_TASK_STATE_FILTER.FABRIC_REPAIR_TASK_STATE_FILTER_DEFAULT,

        /// <summary>
        /// <para>Includes repair tasks in the Created state.</para>
        /// </summary>
        Created = NativeTypes.FABRIC_REPAIR_TASK_STATE_FILTER.FABRIC_REPAIR_TASK_STATE_FILTER_CREATED,

        /// <summary>
        /// <para>Includes repair tasks in the Claimed state.</para>
        /// </summary>
        Claimed = NativeTypes.FABRIC_REPAIR_TASK_STATE_FILTER.FABRIC_REPAIR_TASK_STATE_FILTER_CLAIMED,

        /// <summary>
        /// <para>Includes repair tasks in the Preparing state.</para>
        /// </summary>
        Preparing = NativeTypes.FABRIC_REPAIR_TASK_STATE_FILTER.FABRIC_REPAIR_TASK_STATE_FILTER_PREPARING,

        /// <summary>
        /// <para>Includes repair tasks in the Approved state.</para>
        /// </summary>
        Approved = NativeTypes.FABRIC_REPAIR_TASK_STATE_FILTER.FABRIC_REPAIR_TASK_STATE_FILTER_APPROVED,

        /// <summary>
        /// <para>Includes repair tasks in the Executing state.</para>
        /// </summary>
        Executing = NativeTypes.FABRIC_REPAIR_TASK_STATE_FILTER.FABRIC_REPAIR_TASK_STATE_FILTER_EXECUTING,

        /// <summary>
        /// <para>Includes repair tasks in the Restoring state.</para>
        /// </summary>
        Restoring = NativeTypes.FABRIC_REPAIR_TASK_STATE_FILTER.FABRIC_REPAIR_TASK_STATE_FILTER_RESTORING,

        /// <summary>
        /// <para>Includes repair tasks in the Completed state.</para>
        /// </summary>
        Completed = NativeTypes.FABRIC_REPAIR_TASK_STATE_FILTER.FABRIC_REPAIR_TASK_STATE_FILTER_COMPLETED,


        /// <summary>
        /// <para>Includes repair tasks in the Approved or Executing state.</para>
        /// </summary>
        ReadyToExecute = NativeTypes.FABRIC_REPAIR_TASK_STATE_FILTER.FABRIC_REPAIR_TASK_STATE_FILTER_READY_TO_EXECUTE,

        /// <summary>
        /// <para>Includes repair tasks that are not completed.</para>
        /// </summary>
        Active = NativeTypes.FABRIC_REPAIR_TASK_STATE_FILTER.FABRIC_REPAIR_TASK_STATE_FILTER_ACTIVE,

        /// <summary>
        /// <para>Includes all repair tasks, regardless of state.</para>
        /// </summary>
        All = NativeTypes.FABRIC_REPAIR_TASK_STATE_FILTER.FABRIC_REPAIR_TASK_STATE_FILTER_ALL,
    }
}