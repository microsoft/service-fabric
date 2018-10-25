// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Describes the reason why the node is being deactivated.</para>
    /// </summary>
    /// <remarks>
    /// <para>
    ///     The <see cref="System.Fabric.NodeDeactivationIntent" /> enumeration is provided as a part of the <see cref="System.Fabric.FabricClient.ClusterManagementClient.DeactivateNodeAsync(System.String,System.Fabric.NodeDeactivationIntent)" /> method. </para>
    /// <para>
    ///     Service Fabric uses this information to take the correct actions at the node to provide a graceful shutdown of the node. The intents have a general progression or severity. </para>
    /// <para>
    ///     A deactivation that is started with one intent can be increased to subsequent higher levels of intent. The general order of this progression is: Pause, Restart, Stop, ForceStop.</para>
    /// </remarks>
    public enum NodeDeactivationIntent
    {
        /// <summary>
        /// <para>Indicates that a deactivation intent is invalid. This value is not used.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_NODE_DEACTIVATION_INTENT.FABRIC_NODE_DEACTIVATION_INTENT_INVALID,

        /// <summary>
        /// <para>Indicates that the node should be paused. </para>
        /// </summary>
        /// <remarks>
        /// <para>
        ///     When this intent is used, Service Fabric prevents changes to the specified node. No new replicas are placed on the node, and existing replicas are not moved or shut down.</para>
        /// <para>
        ///     The <see cref="System.Fabric.NodeDeactivationIntent.Pause" /> intent is useful when one or more replicas on a node encounter issues and that node has to be isolated for further investigation</para>
        /// <para> 
        ///     This investigation could include accessing the remote machine to investigate such activities as reviewing local logs, taking memory dumps, and observing other information. </para>
        /// <para>
        ///     The purpose of this mode is to attempt to preserve the node so that additional debugging can be performed under the same conditions that existed when the error occurred.</para>
        /// <para>
        ///     Note that specifying this mode does not guarantee that all changes to the node can be prevented. </para>
        /// <para>
        ///     For example, replicas on the node might crash after the intent to pause the node has been received. </para>
        /// <para>
        ///     As another example, failures in another location in the cluster might cause a Secondary replica on the node to be promoted to the Primary replica.</para>
        /// <para>
        ///     In this mode, Service Fabric will disable Placement and Resource Balancing on the target node</para>
        /// <para>
        ///     In addition Safety Checks (see <see cref="System.Fabric.SafetyCheckKind" />) will be performed by Service Fabric</para>
        /// </remarks>
        Pause = NativeTypes.FABRIC_NODE_DEACTIVATION_INTENT.FABRIC_NODE_DEACTIVATION_INTENT_PAUSE,

        /// <summary>
        /// <para>Indicates that the intent is for the node to be restarted after a short period of time. Service Fabric does not restart the node - this action is done outside of Service Fabric.</para>
        /// </summary>
        /// <remarks>
        /// <para>
        ///     A node might be shut down, for example, to perform an OS update or a Service Fabric code update. </para>
        /// <para>
        ///     In this mode, Service Fabric prevents new replicas from being placed on the node. Additionally, Service Fabric takes the following actions: </para>
        /// <para>
        ///     Disable Placement and Resource balancing on the target node</para>
        /// <para>
        ///     Performs safety checks. The <see cref="System.Fabric.SafetyCheckKind.WaitForPrimaryPlacement" /> safety check is not performed for this intent. </para>
        /// <para>
        ///     Close all replicas and instances running on the node.</para>
        /// <para>
        ///     NOTE: Once replicas and instances are closed, Service Fabric will reactively create replacements for replicas of stateful volatile services and stateless services. </para>
        /// <para>
        ///     For Persisted replicas on the node, new replicas are <b>not</b> be built, because the intention is to restart this node and to recover the persistent state after the restart. The replicas are opened once the node is activated.</para>
        /// </remarks>
        Restart = NativeTypes.FABRIC_NODE_DEACTIVATION_INTENT.FABRIC_NODE_DEACTIVATION_INTENT_RESTART,

        /// <summary>
        /// <para>Indicates that the intent is to reimage the node. Service Fabric does not reimage the node - this action is done outside of Service Fabric.</para>
        /// </summary>
        /// <remarks>
        /// <para>
        ///     When Service Fabric receives this intent, it ensures that: </para>
        /// <para>
        ///     In this mode, Service Fabric prevents new replicas from being placed on the node. Additionally, Service Fabric takes the following actions: </para>
        /// <para>
        ///     Disable Placement and Resource balancing on the target node</para>
        /// <para>
        ///     Move all Up replicas out of the node. </para>
        /// <para>
        ///     For stateless instances this implies creating another instance on another node</para>
        /// <para>
        ///     For replicas of stateful services a replacement replica is built on another node (if there is sufficient capacity in the cluster)</para>
        /// <para>
        ///     If the replica is a primary, some other active secondary of the partition is made the primary prior to creating the replacement</para>
        /// <para>
        ///     Stateful replicas on the node receive notifications to clean up their state and close.</para>
        /// <para>
        ///     Performs a subset of safety checks that ensure that as a result of taking this node down no data loss can occur.</para>
        /// </remarks>
        RemoveData = NativeTypes.FABRIC_NODE_DEACTIVATION_INTENT.FABRIC_NODE_DEACTIVATION_INTENT_REMOVE_DATA,

        /// <summary>
        /// <para>Indicates that the node is being decommissioned and is not expected to return. Service Fabric does not decommission the node - this action is done outside of Service Fabric.</para>
        /// </summary>
        /// <remarks>
        /// <para>
        ///     When Service Fabric receives this intent, it ensures that: </para>
        /// <para>
        ///     In this mode, Service Fabric prevents new replicas from being placed on the node. Additionally, Service Fabric takes the following actions: </para>
        /// <para>
        ///     Disable Placement and Resource balancing on the target node</para>
        /// <para>
        ///     Move all Up replicas out of the node. </para>
        /// <para>
        ///     For stateless instances this implies creating another instance on another node</para>
        /// <para>
        ///     For replicas of stateful services a replacement replica is built on another node (if there is sufficient capacity in the cluster)</para>
        /// <para>
        ///     If the replica is a primary, some other active secondary of the partition is made the primary prior to creating the replacement</para>
        /// <para>
        ///     Stateful replicas on the node receive notifications to clean up their state and close.</para>
        /// <para>
        ///     Performs a subset of safety checks that ensure that as a result of taking this node down no data loss can occur.</para>
        /// </remarks>
        RemoveNode = NativeTypes.FABRIC_NODE_DEACTIVATION_INTENT.FABRIC_NODE_DEACTIVATION_INTENT_REMOVE_NODE
    }
}