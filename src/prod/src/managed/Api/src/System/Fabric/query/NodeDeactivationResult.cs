// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Contains the detailed deactivation information about a node.</para>
    /// </summary>
    public sealed class NodeDeactivationResult
    {
        internal NodeDeactivationResult(
            NodeDeactivationIntent effectiveIntent,
            NodeDeactivationStatus status,
            IList<NodeDeactivationTask> tasks,
            IList<SafetyCheck> pendingSafetyChecks)
        {
            this.EffectiveIntent = effectiveIntent;
            this.Status = status;
            this.Tasks = tasks;
            this.PendingSafetyChecks = pendingSafetyChecks;
        }

        private NodeDeactivationResult() { }

        /// <summary>
        /// <para>A node may get deactivated by multiple tasks at the same time. Each task may specify a different node 
        /// deactivation intent. In this case, effective intent is highest intent among all deactivation tasks, where ordering 
        /// is defined as Pause &lt; Restart &lt; RemoveData.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.NodeDeactivationIntent" />.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.NodeDeactivationIntent)]
        public NodeDeactivationIntent EffectiveIntent { get; private set; }

        /// <summary>
        /// <para>Specifies the deactivation status for a node.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.NodeDeactivationStatus" />.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.NodeDeactivationStatus)]
        public NodeDeactivationStatus Status { get; private set; }

        /// <summary>
        /// <para>Contains information about all the node deactivation tasks for a node.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Collections.Generic.IList{T}" />.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.NodeDeactivationTask)]
        public IList<NodeDeactivationTask> Tasks { get; private set; }

        /// <summary>
        /// <para>
        /// Gets a list of safety checks that are currently failing.
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The list of failing safety checks.</para>
        /// </value>
        public IList<SafetyCheck> PendingSafetyChecks { get; private set; }

        internal static unsafe NodeDeactivationResult CreateFromNative(
           NativeTypes.FABRIC_NODE_DEACTIVATION_QUERY_RESULT_ITEM nativeResult)
        {
            IList<NodeDeactivationTask> tasks = new List<NodeDeactivationTask>();
            IList<SafetyCheck> pendingSafetyChecks = new List<SafetyCheck>();

            var nativeTasks = (NativeTypes.FABRIC_NODE_DEACTIVATION_TASK_LIST*)nativeResult.Tasks;
            var nativeItemArray = (NativeTypes.FABRIC_NODE_DEACTIVATION_TASK*)nativeTasks->Items;
            for (int i = 0; i < nativeTasks->Count; ++i)
            {
                var nativeItem = *(nativeItemArray + i);
                tasks.Add(NodeDeactivationTask.CreateFromNative(nativeItem));
            }

            if (nativeResult.Reserved != IntPtr.Zero)
            {
                var ex1 = (NativeTypes.FABRIC_NODE_DEACTIVATION_QUERY_RESULT_ITEM_EX1*)nativeResult.Reserved;
                pendingSafetyChecks = SafetyCheck.FromNativeList(
                    (NativeTypes.FABRIC_SAFETY_CHECK_LIST*)ex1->PendingSafetyChecks);
            }

            var nodeDeactivationResult = new NodeDeactivationResult(
                (NodeDeactivationIntent)nativeResult.EffectiveIntent,
                (NodeDeactivationStatus)nativeResult.Status,
                tasks,
                pendingSafetyChecks);

            return nodeDeactivationResult;
        }
    }
}