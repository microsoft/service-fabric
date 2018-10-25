// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Runtime.Serialization;

    /// <summary>
    /// <para>Represents a replica running in the code package.</para>
    /// </summary>
    [KnownType(typeof(DeployedStatefulServiceReplicaDetail))]
    [KnownType(typeof(DeployedStatelessServiceInstanceDetail))]
    public abstract class DeployedServiceReplicaDetail
    {
        /// <summary>
        /// <para>
        /// Instantiates a <see cref="System.Fabric.Query.DeployedServiceReplicaDetail" /> object with specified service kind.
        /// </para>
        /// </summary>
        /// <param name="serviceKind">
        /// <para>The type of the service</para>
        /// </param>
        internal protected DeployedServiceReplicaDetail(Query.ServiceKind serviceKind)
        {
            this.ServiceKind = serviceKind;
        }

        /// <summary>
        /// <para>
        /// Gets the type of the service 
        /// </para>
        /// </summary>
        /// <value>
        /// <para>The service kind representing the type of the service this replica belongs to</para>
        /// </value>
        /// <remarks>
        /// <para>
        /// Based on the value of this property this object can be
        /// downcasted to DeployedStatefulServiceReplicaDetail or DeployedStatelessServiceInstanceDetail
        /// </para>
        /// </remarks>
        [JsonCustomization(AppearanceOrder = -2)]
        public ServiceKind ServiceKind { get; private set; }

        /// <summary>
        /// <para>Gets or sets the service name to which this replica belongs.</para>
        /// </summary>
        /// <value>
        /// <para>The service name to which this replica belongs.</para>
        /// </value>
        public Uri ServiceName { get; internal set; }

        /// <summary>
        /// <para>Gets or sets the partition id associated with this replica.</para>
        /// </summary>
        /// <value>
        /// <para>The partition id associated with this replica.</para>
        /// </value>
        public Guid PartitionId { get; internal set; }

        /// <summary>
        /// <para>Gets or sets the current API call that is being executed on the replica.</para>
        /// </summary>
        /// <value>
        /// <para>The current API call that is being executed on the replica.</para>
        /// </value>
        public ServiceOperationName CurrentServiceOperation { get; internal set; }

        /// <summary>
        /// <para>Gets or sets the start time of the current service operation in UTC format.</para>
        /// </summary>
        /// <value>
        /// <para>The start time of the current service operation in UTC format.</para>
        /// </value>
        public DateTime CurrentServiceOperationStartTimeUtc { get; internal set; }

        /// <summary>
        /// <para>Gets or sets the load reported by this replica.</para>
        /// </summary>
        /// <value>
        /// <para>The load reported by this replica.</para>
        /// </value>
        public IList<LoadMetricReport> ReportedLoad { get; internal set; }

        internal static unsafe DeployedServiceReplicaDetail CreateFromNative(
           NativeClient.IFabricGetDeployedServiceReplicaDetailResult nativeResult)
        {
            var rv = DeployedServiceReplicaDetail.FromNative(*(NativeTypes.FABRIC_DEPLOYED_SERVICE_REPLICA_DETAIL_QUERY_RESULT_ITEM*)nativeResult.get_ReplicaDetail());

            GC.KeepAlive(nativeResult);

            return rv;
        }

        internal static unsafe DeployedServiceReplicaDetail FromNative(
           NativeTypes.FABRIC_DEPLOYED_SERVICE_REPLICA_DETAIL_QUERY_RESULT_ITEM nativeResultItem)
        {
            if ((ServiceKind)nativeResultItem.Kind == ServiceKind.Stateless)
            {
                var nativeStatelessInstanceQueryResult =
                    *(NativeTypes.FABRIC_DEPLOYED_STATELESS_SERVICE_INSTANCE_DETAIL_QUERY_RESULT_ITEM*)nativeResultItem.Value;
                return DeployedStatelessServiceInstanceDetail.CreateFromNative(nativeStatelessInstanceQueryResult);
            }
            else if ((ServiceKind)nativeResultItem.Kind == ServiceKind.Stateful)
            {
                var nativeStatefulServiceQueryResult =
                    *(NativeTypes.FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_DETAIL_QUERY_RESULT_ITEM*)nativeResultItem.Value;
                return DeployedStatefulServiceReplicaDetail.CreateFromNative(nativeStatefulServiceQueryResult);
            }
            else
            {
                AppTrace.TraceSource.WriteNoise(
                    "DeployedServiceReplica.CreateFromNative",
                    "Ignoring the result with unsupported ServiceKind value {0}",
                    (int)nativeResultItem.Kind);
                return null;
            }
        }
    }
}