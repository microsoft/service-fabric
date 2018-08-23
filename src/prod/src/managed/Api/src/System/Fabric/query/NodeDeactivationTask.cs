// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Specifies a node deactivation task.</para>
    /// </summary>
    public sealed class NodeDeactivationTask
    {
        [JsonCustomization]
        internal NodeDeactivationTask(
            NodeDeactivationTaskId taskId,
            NodeDeactivationIntent intent)
        {
            this.TaskId = taskId;
            this.Intent = intent;
        }

        private NodeDeactivationTask() { }

        /// <summary>
        /// <para>The ID for the node deactivation task.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.Query.NodeDeactivationTaskId" />.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.NodeDeactivationTaskId)]
        public NodeDeactivationTaskId TaskId { get; private set; }

        /// <summary>
        /// <para>The node deactivation intent for the node deactivation task.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.NodeDeactivationIntent" />.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.NodeDeactivationIntent)]
        public NodeDeactivationIntent Intent { get; private set; }

        internal static unsafe NodeDeactivationTask CreateFromNative(
           NativeTypes.FABRIC_NODE_DEACTIVATION_TASK nativeResult)
        {
            var task = new NodeDeactivationTask(
                NodeDeactivationTaskId.CreateFromNative(*(NativeTypes.FABRIC_NODE_DEACTIVATION_TASK_ID*)nativeResult.TaskId),
                (NodeDeactivationIntent)nativeResult.Intent);

            return task;
        }
    }
}