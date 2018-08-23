// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Specifies the different types of node deactivation tasks.</para>
    /// </summary>
    public enum NodeDeactivationTaskType
    {
        /// <summary>
        /// <para>Invalid task type.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_NODE_DEACTIVATION_TASK_TYPE.FABRIC_NODE_DEACTIVATION_TASK_TYPE_INVALID,
        /// <summary>
        /// <para>Specifies the task created by the Azure MR.</para>
        /// </summary>
        Infrastructure = NativeTypes.FABRIC_NODE_DEACTIVATION_TASK_TYPE.FABRIC_NODE_DEACTIVATION_TASK_TYPE_INFRASTRUCTURE,
        /// <summary>
        /// <para>Specifies the task that was created by the Repair Manager service.</para>
        /// </summary>
        Repair = NativeTypes.FABRIC_NODE_DEACTIVATION_TASK_TYPE.FABRIC_NODE_DEACTIVATION_TASK_TYPE_REPAIR,
        /// <summary>
        /// <para>Specifies that the task was created by calling the public API.</para>
        /// </summary>
        Client = NativeTypes.FABRIC_NODE_DEACTIVATION_TASK_TYPE.FABRIC_NODE_DEACTIVATION_TASK_TYPE_CLIENT
    }
}