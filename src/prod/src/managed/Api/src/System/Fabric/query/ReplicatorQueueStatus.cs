// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Provides various statistics of the queue used in the Service Fabric Replicator.</para>
    /// </summary>
    /// <remarks>
    /// <para>Depending on the role of the replicator (<see cref="System.Fabric.ReplicaRole.Primary" /> 
    /// or <see cref="System.Fabric.ReplicaRole.ActiveSecondary" />), the properties in this type mean different things.</para>
    /// </remarks>
    public sealed class ReplicatorQueueStatus
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Query.ReplicatorQueueStatus" /> class.</para>
        /// </summary>
        public ReplicatorQueueStatus()
        {
        }

        /// <summary>
        /// <para>Gets the utilization of the queue.</para>
        /// </summary>
        /// <value>
        /// <para>The utilization of the queue.</para>
        /// </value>
        /// <remarks>
        /// <para>A value of ‘0’ indicates that the queue is empty and a value of ‘100’ indicates that the queue is full.</para>
        /// </remarks>
        public long QueueUtilizationPercentage { get; private set; }

        /// <summary>
        /// <para>Gets the number of virtual memory bytes being consumed by the queue.</para>
        /// </summary>
        /// <value>
        /// <para>The number of virtual memory bytes being consumed by the queue.</para>
        /// </value>
        public long QueueMemorySize { get; private set; }

        /// <summary>
        /// <para>See remarks.</para>
        /// </summary>
        /// <value>
        /// <para>The first sequence number.</para>
        /// </value>
        /// <remarks>
        ///   <list type="number">
        ///     <item>
        ///       <description>
        ///         <para>
        ///           <see cref="System.Fabric.ReplicaRole.Primary" /> – Represents the smallest sequence number of the operation that is 
        /// present in the queue. It’s value will be the same as <see cref="System.Fabric.Query.ReplicatorQueueStatus.CompletedSequenceNumber" />, 
        /// since the primary replicator discards operations once it receives an acknowledgement from all the secondary replicas.</para>
        ///       </description>
        ///     </item>
        ///     <item>
        ///       <description>
        ///         <para>
        ///           <see cref="System.Fabric.ReplicaRole.ActiveSecondary" /> – Represents the sequence number of the first operation 
        /// that is available in the queue.</para>
        ///       </description>
        ///     </item>
        ///   </list>
        /// </remarks>
        public long FirstSequenceNumber { get; private set; }

        /// <summary>
        /// <para>See remarks.</para>
        /// </summary>
        /// <value>
        /// <para>The completed sequence number.</para>
        /// </value>
        /// <remarks>
        ///   <list type="number">
        ///     <item>
        ///       <description>
        ///         <para>
        ///           <see cref="System.Fabric.ReplicaRole.Primary" /> – Represents the highest sequence number for which <b>all</b> the 
        /// secondary replicas have applied the operation.</para>
        ///       </description>
        ///     </item>
        ///     <item>
        ///       <description>
        ///         <para>
        ///           <see cref="System.Fabric.ReplicaRole.ActiveSecondary" /> – Represents the highest sequence number that has been 
        /// applied by the user service replica.</para>
        ///       </description>
        ///     </item>
        ///   </list>
        /// </remarks>
        public long CompletedSequenceNumber { get; private set; }

        /// <summary>
        /// <para>See remarks.</para>
        /// </summary>
        /// <value>
        /// <para>The committed sequence number.</para>
        /// </value>
        /// <remarks>
        ///   <list type="number">
        ///     <item>
        ///       <description>
        ///         <para>
        ///           <see cref="System.Fabric.ReplicaRole.Primary" /> – Represents the highest sequence number for which a <b>quorum</b> 
        /// of secondary replicas have applied the operation.</para>
        ///       </description>
        ///     </item>
        ///     <item>
        ///       <description>
        ///         <para>
        ///           <see cref="System.Fabric.ReplicaRole.ActiveSecondary" /> – Represents the highest sequence number that has been 
        /// received by the secondary replicator from the primary.</para>
        ///       </description>
        ///     </item>
        ///   </list>
        /// </remarks>
        public long CommittedSequenceNumber { get; private set; }

        /// <summary>
        /// <para>See remarks.</para>
        /// </summary>
        /// <value>
        /// <para>The last sequence number.</para>
        /// </value>
        /// <remarks>
        ///   <list type="number">
        ///     <item>
        ///       <description>
        ///         <para>
        ///           <see cref="System.Fabric.ReplicaRole.Primary" /> – Represents the latest sequence number of the operation that 
        /// is available in the queue.</para>
        ///       </description>
        ///     </item>
        ///     <item>
        ///       <description>
        ///         <para>
        ///           <see cref="System.Fabric.ReplicaRole.ActiveSecondary" /> – Represents the latest sequence number of the operation 
        /// that is available in the queue.</para>
        ///       </description>
        ///     </item>
        ///   </list>
        /// </remarks>
        public long LastSequenceNumber { get; private set; }

        internal static unsafe ReplicatorQueueStatus CreateFromNative(NativeTypes.FABRIC_REPLICATOR_QUEUE_STATUS* nativeEntryPoint)
        {
            return new ReplicatorQueueStatus
            {
                QueueUtilizationPercentage = nativeEntryPoint->QueueUtilizationPercentage,
                FirstSequenceNumber = nativeEntryPoint->FirstSequenceNumber,
                CompletedSequenceNumber = nativeEntryPoint->CompletedSequenceNumber,
                CommittedSequenceNumber = nativeEntryPoint->CommittedSequenceNumber,
                LastSequenceNumber = nativeEntryPoint->LastSequenceNumber,
                QueueMemorySize = nativeEntryPoint->QueueMemorySize,
            };
        }
    }
}