// ----------------------------------------------------------------------
//  <copyright file="Scenario.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------
namespace ClusterInsight.InsightCore.DataSetLayer.CallbackStore
{
    /// <summary>
    /// Describes different scenarios
    /// </summary>
    public enum Scenario
    {
        /// <summary>
        /// Quorum loss across cluster
        /// </summary>
        QuorumLoss,

        /// <summary>
        /// Unexpected process crashes.
        /// </summary>
        ProcessCrash,

        /// <summary>
        /// Unhandled Exception
        /// </summary>
        RunAsyncUnhandledException,

        /// <summary>
        /// Reconfiguration Event
        /// </summary>
        Reconfiguration,

        /// <summary>
        /// Failed to open Replica
        /// </summary>
        ReplicaOpenFailed,

        /// <summary>
        /// All Partition health events
        /// </summary>
        NewPartitionHealthEvent,

        /// <summary>
        /// All Partition health events
        /// </summary>
        ExpiredPartitionHealthEvent,

        /// <summary>
        /// All Partition health events
        /// </summary>
        NewReplicaHealthEvent,

        /// <summary>
        /// All Replica health events.
        /// </summary>
        ExpiredReplicaHealthEvent,

        /// <summary>
        /// All Node Health Events
        /// </summary>
        NewNodeHealthEvent,

        /// <summary>
        /// When a Node health event expires
        /// </summary>
        ExpiredNodeHealthEvent,

        /// <summary>
        /// All Cluster Health Events
        /// </summary>
        NewClusterHealthEvent,

        /// <summary>
        /// When a Cluster health event expires
        /// </summary>
        ExpiredClusterHealthEvent
    }
}