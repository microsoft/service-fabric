// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.DataStructures
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents the task that Chaos is presently executing.</para>
    /// </summary>
    public enum ChaosStatus
    {
        /// <summary>
        /// <para>Indicates that the current task is invalid.</para>
        /// </summary>
        None = NativeTypes.FABRIC_CHAOS_STATUS.FABRIC_CHAOS_STATUS_INVALID,

        /// <summary>
        /// <para>Indicates that Chaos is executing actions as part of fault inducing.</para>
        /// </summary>
        Running = NativeTypes.FABRIC_CHAOS_STATUS.FABRIC_CHAOS_STATUS_RUNNING,

        /// <summary>
        /// <para>Indicates that Chaos has finished inducing faults and now it is validating if the Cluster is stable and healthy.</para>
        /// </summary>
        Stopped = NativeTypes.FABRIC_CHAOS_STATUS.FABRIC_CHAOS_STATUS_STOPPED
    }
}