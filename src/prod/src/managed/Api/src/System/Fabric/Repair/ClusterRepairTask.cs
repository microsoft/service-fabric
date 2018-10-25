// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Repair
{
    /// <summary>
    /// <para>Represents a repair task that has Cluster scope.</para>
    /// <para>This class supports the Service Fabric platform; it is not meant to be called directly from your code.</para>
    /// </summary>
    public sealed class ClusterRepairTask : RepairTask
    {
        internal ClusterRepairTask()
            : base(new ClusterRepairScopeIdentifier())
        {
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Repair.ClusterRepairTask" /> class.</para>
        /// </summary>
        /// <param name="taskId">
        /// <para>The ID of the repair task.</para>
        /// </param>
        /// <param name="action">
        /// <para>The repair action being requested.</para>
        /// </param>
        public ClusterRepairTask(string taskId, string action)
            : base(new ClusterRepairScopeIdentifier(), taskId, action)
        {
        }
    }
}