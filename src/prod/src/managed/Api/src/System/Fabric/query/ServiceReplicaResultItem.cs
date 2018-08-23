// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using System.Runtime.Serialization;

    /// <summary>
    /// <para>Represents a query replica.</para>
    /// </summary>
    [KnownType(typeof(StatelessServiceInstance))]
    [KnownType(typeof(StatefulServiceReplica))]
    public abstract class Replica
    {
        ///<summary>
        /// <para>Initializes a replica</para>
        /// </summary>
        /// <param name="serviceKind">
        /// <para>The type of the replica</para>
        /// </param>
        /// <param name="id">
        /// <para>The replica ID</para>
        /// </param>
        /// <param name="replicaStatus">
        /// <para>The status replica will be initialized with.</para>
        /// </param>
        /// <param name="healthState">
        /// <para>The health state replica will be initialized with</para>
        /// </param>
        /// <param name="replicaAddress">
        /// <para>The address replica will be initialized with.</para>
        /// </param>
        /// <param name="nodeName">
        /// <para>The node name replica will be initialized with</para>
        /// </param>
        /// <param name="lastInBuildDuration">
        /// <para>The last in build duration replica will be initialized with.</para>
        /// </param>
        protected Replica(
            ServiceKind serviceKind,
            long id,
            ServiceReplicaStatus replicaStatus,
            HealthState healthState,
            string replicaAddress,
            string nodeName,
            TimeSpan lastInBuildDuration)
        {
            this.ServiceKind = serviceKind;
            this.Id = id;
            this.ReplicaStatus = replicaStatus;
            this.HealthState = healthState;
            this.ReplicaAddress = replicaAddress;
            this.NodeName = nodeName;
            this.LastInBuildDuration = lastInBuildDuration;
        }

        /// <summary>
        /// Gets the service kind.
        /// </summary>
        /// <value>The service kind.</value>
        [JsonCustomization(AppearanceOrder = -2)]
        public ServiceKind ServiceKind { get; private set; }

        /// <summary>
        /// <para>Gets the replica identifier.</para>
        /// </summary>
        /// <value>
        /// <para>The replica identifier.</para>
        /// </value>
        [JsonCustomization(IsIgnored = true)]
        public long Id { get; protected set; }

        /// <summary>
        /// <para>Gets the status of the replica.</para>
        /// </summary>
        /// <value>
        /// <para>The status of the replica.</para>
        /// </value>
        public ServiceReplicaStatus ReplicaStatus { get; private set; }

        /// <summary>
        /// <para>Gets the health state of the replica.</para>
        /// </summary>
        /// <value>
        /// <para>The health state of the replica.</para>
        /// </value>
        public HealthState HealthState { get; private set; }

        /// <summary>
        /// <para>Gets the address the replica is listening on.</para>
        /// </summary>
        /// <value>
        /// <para>The address the replica is listening on.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Address)]
        public string ReplicaAddress { get; private set; }

        /// <summary>
        /// <para>Gets the node name the replica is running on.</para>
        /// </summary>
        /// <value>
        /// <para>The node name the replica is running on.</para>
        /// </value>
        public string NodeName { get; private set; }

        /// <summary>
        /// <para>Gets last in build duration.</para>
        /// </summary>
        /// <value>
        /// <para>The last in build duration.</para>
        /// </value>
        [JsonCustomization(IsIgnored = true)]
        public TimeSpan LastInBuildDuration { get; private set; }

        /// <summary>
        /// Gets last in build duration in seconds.
        /// </summary>
        /// <value>Last in build duration in seconds.</value>
        protected internal long LastInBuildDurationInSeconds
        {
            get { return (long)this.LastInBuildDuration.TotalSeconds; }
            private set { this.LastInBuildDuration = TimeSpan.FromTicks(value * TimeSpan.TicksPerSecond); }
        }

        internal static unsafe Replica CreateFromNative(
           NativeTypes.FABRIC_SERVICE_REPLICA_QUERY_RESULT_ITEM nativeResultItem)
        {
            if ((ServiceKind)nativeResultItem.Kind == ServiceKind.Stateless)
            {
                NativeTypes.FABRIC_STATELESS_SERVICE_INSTANCE_QUERY_RESULT_ITEM nativeStatelessInstanceQueryResult =
                    *(NativeTypes.FABRIC_STATELESS_SERVICE_INSTANCE_QUERY_RESULT_ITEM*)nativeResultItem.Value;
                return StatelessServiceInstance.FromNative(nativeStatelessInstanceQueryResult);
            }
            else if ((ServiceKind)nativeResultItem.Kind == ServiceKind.Stateful)
            {
                NativeTypes.FABRIC_STATEFUL_SERVICE_REPLICA_QUERY_RESULT_ITEM nativeStatefulServiceQueryResult =
                    *(NativeTypes.FABRIC_STATEFUL_SERVICE_REPLICA_QUERY_RESULT_ITEM*)nativeResultItem.Value;
                return StatefulServiceReplica.FromNative(nativeStatefulServiceQueryResult);
            }
            else
            {
                AppTrace.TraceSource.WriteNoise(
                    "Replica.CreateFromNative",
                    "Ignoring the result with unsupported ServiceKind value {0}",
                    (int)nativeResultItem.Kind);
                return null;
            }
        }
    }
}