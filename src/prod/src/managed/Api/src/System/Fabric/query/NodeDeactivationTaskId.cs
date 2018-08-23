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
    /// <para>Specifies the task ID of a node deactivation task.</para>
    /// </summary>
    public sealed class NodeDeactivationTaskId
    {
        internal NodeDeactivationTaskId(
            string id,
            NodeDeactivationTaskType type)
        {
            this.Id = id;
            this.Type = type;
        }

        private NodeDeactivationTaskId() { }

        /// <summary>
        /// <para>The unique ID of the node deactivation task.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.String" />.</para>
        /// </value>
        public string Id { get; private set; }

        /// <summary>
        /// <para>The ID type of the node deactivation task.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.NodeDeactivationTaskType" />.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.NodeDeactivationTaskType)]
        public NodeDeactivationTaskType Type { get; private set; }

        internal static unsafe NodeDeactivationTaskId CreateFromNative(
           NativeTypes.FABRIC_NODE_DEACTIVATION_TASK_ID nativeResult)
        {
            var taskId = new NodeDeactivationTaskId(
                NativeTypes.FromNativeString(nativeResult.Id),
                (NodeDeactivationTaskType)nativeResult.Type);

            return taskId;
        }
    }
}