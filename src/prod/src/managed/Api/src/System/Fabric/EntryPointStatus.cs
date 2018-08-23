// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

        /// <summary>
        /// <para>Specifies the status of the CodePackage EntryPoint deployed on a node.</para>
        /// </summary>
    public enum EntryPointStatus
    {
        /// <summary>
        /// <para>The status of the entry point is unknown or invalid.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_ENTRY_POINT_STATUS.FABRIC_ENTRY_POINT_STATUS_INVALID,
        /// <summary>
        /// <para>The entry point is scheduled to be started.</para>
        /// </summary>
        Pending = NativeTypes.FABRIC_ENTRY_POINT_STATUS.FABRIC_ENTRY_POINT_STATUS_PENDING,
        /// <summary>
        /// <para>The entry point is being started.</para>
        /// </summary>
        Starting = NativeTypes.FABRIC_ENTRY_POINT_STATUS.FABRIC_ENTRY_POINT_STATUS_STARTING,
        /// <summary>
        /// <para>The entry point was started successfully and is running.</para>
        /// </summary>
        Started = NativeTypes.FABRIC_ENTRY_POINT_STATUS.FABRIC_ENTRY_POINT_STATUS_STARTED,
        /// <summary>
        /// <para>The entry point is being stopped.</para>
        /// </summary>
        Stopping = NativeTypes.FABRIC_ENTRY_POINT_STATUS.FABRIC_ENTRY_POINT_STATUS_STOPPING,
        /// <summary>
        /// <para>The entry point is not running.</para>
        /// </summary>
        Stopped = NativeTypes.FABRIC_ENTRY_POINT_STATUS.FABRIC_ENTRY_POINT_STATUS_STOPPED,
    }
}