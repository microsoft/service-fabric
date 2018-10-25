// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Repair
{
    using System;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Specifies the result of the repair task.</para>
    /// <para>This type supports the Service Fabric platform; it is not meant to be used directly from your code.</para>
    /// </summary>
    [Flags]
    public enum RepairTaskResult
    {
        /// <summary>
        /// <para>Indicates that the repair task result is invalid.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_REPAIR_TASK_RESULT.FABRIC_REPAIR_TASK_RESULT_INVALID,

        /// <summary>
        /// <para>Indicates that the repair task completed execution successfully.</para>
        /// </summary>
        Succeeded = NativeTypes.FABRIC_REPAIR_TASK_RESULT.FABRIC_REPAIR_TASK_RESULT_SUCCEEDED,

        /// <summary>
        /// <para>Indicates that the repair task was cancelled prior to execution.</para>
        /// </summary>
        Cancelled = NativeTypes.FABRIC_REPAIR_TASK_RESULT.FABRIC_REPAIR_TASK_RESULT_CANCELLED,

        /// <summary>
        /// <para>Indicates that execution of the repair task was interrupted by a cancellation request after some work had 
        /// already been performed.</para>
        /// </summary>
        Interrupted = NativeTypes.FABRIC_REPAIR_TASK_RESULT.FABRIC_REPAIR_TASK_RESULT_INTERRUPTED,

        /// <summary>
        /// <para>Indicates that there was a failure during execution of the repair task. Some work may have been performed.</para>
        /// </summary>
        Failed = NativeTypes.FABRIC_REPAIR_TASK_RESULT.FABRIC_REPAIR_TASK_RESULT_FAILED,

        /// <summary>
        /// <para>Indicates that the repair task result is not yet available, because the repair task has not finished executing.</para>
        /// </summary>
        Pending = NativeTypes.FABRIC_REPAIR_TASK_RESULT.FABRIC_REPAIR_TASK_RESULT_PENDING,
    }
}