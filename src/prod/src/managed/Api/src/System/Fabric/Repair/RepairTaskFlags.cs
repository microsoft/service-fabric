// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Repair
{
    using System;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Defines flags that provide extended status information about a repair task.</para>
    /// <para>This type supports the Service Fabric platform; it is not meant to be used directly from your code.</para>
    /// </summary>
    [Flags]
    public enum RepairTaskFlags
    {
        /// <summary>
        /// <para>No flags are specified.</para>
        /// </summary>
        None = NativeTypes.FABRIC_REPAIR_TASK_FLAGS.FABRIC_REPAIR_TASK_FLAGS_NONE,

        /// <summary>
        /// <para>A user has requested cancellation of the repair task.</para>
        /// </summary>
        CancelRequested = NativeTypes.FABRIC_REPAIR_TASK_FLAGS.FABRIC_REPAIR_TASK_FLAGS_CANCEL_REQUESTED,

        /// <summary>
        /// <para>A user has requested an abort of the repair task.</para>
        /// </summary>
        AbortRequested = NativeTypes.FABRIC_REPAIR_TASK_FLAGS.FABRIC_REPAIR_TASK_FLAGS_ABORT_REQUESTED,

        /// <summary>
        /// <para>A user has forced the approval of the repair task, so it may have executed without normal safety guarantees.</para>
        /// </summary>
        ForcedApproval = NativeTypes.FABRIC_REPAIR_TASK_FLAGS.FABRIC_REPAIR_TASK_FLAGS_FORCED_APPROVAL,

        /// <summary>
        /// <para>A mask that includes all valid repair task flags.</para>
        /// </summary>
        ValidMask = NativeTypes.FABRIC_REPAIR_TASK_FLAGS.FABRIC_REPAIR_TASK_FLAGS_VALID_MASK,
    }
}