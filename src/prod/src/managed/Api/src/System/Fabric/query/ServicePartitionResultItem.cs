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
    /// <para>Represents a partition.</para>
    /// </summary>
    [KnownType(typeof(StatelessServicePartition))]
    [KnownType(typeof(StatefulServicePartition))]
    public abstract class Partition
    {
        /// <summary>
        /// <para>Initializes a new instance of the partition</para>
        /// </summary>
        /// <param name="serviceKind">
        /// <para>The type of the partition</para>
        /// </param>
        /// <param name="partitionInformation">
        /// <para>The Partition Information</para>
        /// </param>
        /// <param name="healthState">
        /// <para>Health State of the partition</para>
        /// </param>
        /// <param name="partitionStatus">
        /// <para>Status of the partition</para>
        /// </param>
        protected Partition(ServiceKind serviceKind, ServicePartitionInformation partitionInformation, HealthState healthState, ServicePartitionStatus partitionStatus)
        {
            this.ServiceKind = serviceKind;
            this.PartitionInformation = partitionInformation;
            this.HealthState = healthState;
            this.PartitionStatus = partitionStatus;
        }

        /// <summary>
        /// <para>The type of the partition (Stateful/Stateless)</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.Query.ServiceKind" />.</para>
        /// </value>
        [JsonCustomization(AppearanceOrder = -2)]
        public ServiceKind ServiceKind { get; private set; }

        /// <summary>
        /// <para>Gets the health state of the partition.</para>
        /// </summary>
        /// <value>
        /// <para>The health state of the partition.</para>
        /// </value>
        public HealthState HealthState { get; private set; }

        /// <summary>
        /// <para>Gets the partition information.</para>
        /// </summary>
        /// <value>
        /// <para>The information of the partition.</para>
        /// </value>
        public ServicePartitionInformation PartitionInformation { get; private set; }

        /// <summary>
        /// <para>Gets the status of the partition.</para>
        /// </summary>
        /// <value>
        /// <para>The status of the partition.</para>
        /// </value>
        public ServicePartitionStatus PartitionStatus { get; private set; }

        internal static unsafe Partition CreateFromNative(
           NativeTypes.FABRIC_SERVICE_PARTITION_QUERY_RESULT_ITEM nativeResultItem)
        {
            if ((ServiceKind)nativeResultItem.Kind == ServiceKind.Stateless)
            {
                NativeTypes.FABRIC_STATELESS_SERVICE_PARTITION_QUERY_RESULT_ITEM nativeStatelessPartitionQueryResult =
                    *(NativeTypes.FABRIC_STATELESS_SERVICE_PARTITION_QUERY_RESULT_ITEM*)nativeResultItem.Value;
                return StatelessServicePartition.FromNative(nativeStatelessPartitionQueryResult);
            }
            else if ((ServiceKind)nativeResultItem.Kind == ServiceKind.Stateful)
            {
                NativeTypes.FABRIC_STATEFUL_SERVICE_PARTITION_QUERY_RESULT_ITEM nativeStatefulServiceQueryResult =
                    *(NativeTypes.FABRIC_STATEFUL_SERVICE_PARTITION_QUERY_RESULT_ITEM*)nativeResultItem.Value;
                return StatefulServicePartition.FromNative(nativeStatefulServiceQueryResult);
            }
            else
            {
                AppTrace.TraceSource.WriteNoise(
                    "Partition.CreateFromNative",
                    "Ignoring the result with unsupported ServiceKind value {0}",
                    (int)nativeResultItem.Kind);
                return null;
            }
        }
    }
}