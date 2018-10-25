// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents the information about a stateful replica running in a code package.</para>
    /// </summary>
    public sealed class DeployedStatefulServiceReplicaDetail : DeployedServiceReplicaDetail
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Query.DeployedStatefulServiceReplicaDetail" /> class.</para>
        /// </summary>
        public DeployedStatefulServiceReplicaDetail()
            : base(ServiceKind.Stateful)
        {
        }

        /// <summary>
        /// <para>Gets the replica ID of this replica.</para>
        /// </summary>
        /// <value>
        /// <para>The replica ID.</para>
        /// </value>
        public long ReplicaId { get; internal set; }

        /// <summary>
        /// <para>Gets the current APIs running on the replicator.</para>
        /// </summary>
        /// <value>
        /// <para>The current APIs running on the replicator.</para>
        /// </value>
        public ReplicatorOperationName CurrentReplicatorOperation { get; internal set; }

        /// <summary>
        /// <para>Gets the current read status for this replica.</para>
        /// </summary>
        /// <value>
        /// <para>The current read status for this replica.</para>
        /// </value>
        public PartitionAccessStatus ReadStatus { get; internal set; }

        /// <summary>
        /// <para>Gets the current write status of the replica.</para>
        /// </summary>
        /// <value>
        /// <para>The current write status of the replica.</para>
        /// </value>
        public PartitionAccessStatus WriteStatus { get; internal set; }

        /// <summary>
        /// <para>Gets the information about the replicator if the replica is using the Service Fabric Replicator</para>
        /// </summary>
        /// <value>
        /// <para>The replicator status.</para>
        /// </value>
        public ReplicatorStatus ReplicatorStatus { get; internal set; }

        /// <summary>
        /// Gets a value indicating the status of the current replica.
        /// </summary>
        /// <value>The replica status.</value>
        /// <remarks>Currently, only replicas of type <see cref="System.Fabric.KeyValueStoreReplica" /> will produce query status details.</remarks>
        public ReplicaStatus ReplicaStatus { get; internal set; }

        /// <summary>
        /// <para>Gets additonal details about the deployed service replica like replica role, host processId, information about reconfiguration.</para>
        /// <value>Replica Detail.</value>
        /// </summary>
        public DeployedStatefulServiceReplica DeployedServiceReplica { get; internal set; }

        internal static unsafe DeployedServiceReplicaDetail CreateFromNative(
           NativeTypes.FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_DETAIL_QUERY_RESULT_ITEM nativeResultItem)
        {
            var rv = new DeployedStatefulServiceReplicaDetail
            {
                CurrentReplicatorOperation = (ReplicatorOperationName)nativeResultItem.CurrentReplicatorOperation,
                CurrentServiceOperation = (ServiceOperationName)nativeResultItem.CurrentServiceOperation,
                CurrentServiceOperationStartTimeUtc = NativeTypes.FromNativeFILETIME(nativeResultItem.CurrentServiceOperationStartTimeUtc),
                PartitionId = nativeResultItem.PartitionId,
                ReadStatus = (PartitionAccessStatus)nativeResultItem.ReadStatus,
                ReplicaId = nativeResultItem.ReplicaId,
                ReportedLoad = LoadMetricReport.CreateFromNativeList((NativeTypes.FABRIC_LOAD_METRIC_REPORT_LIST*)nativeResultItem.ReportedLoad),
                ServiceName = new Uri(NativeTypes.FromNativeString(nativeResultItem.ServiceName)),
                WriteStatus = (PartitionAccessStatus)nativeResultItem.WriteStatus
            };

            if (nativeResultItem.ReplicatorStatus != IntPtr.Zero)
            {
                rv.ReplicatorStatus = ReplicatorStatus.FromNative((NativeTypes.FABRIC_REPLICATOR_STATUS_QUERY_RESULT*)nativeResultItem.ReplicatorStatus);
            }

            if (nativeResultItem.Reserved != IntPtr.Zero)
            {
                var ex1 = (NativeTypes.FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_DETAIL_QUERY_RESULT_ITEM_EX1*)nativeResultItem.Reserved;
                var replicaStatus = (NativeTypes.FABRIC_REPLICA_STATUS_QUERY_RESULT*)ex1->ReplicaStatus;

                if (replicaStatus != null && replicaStatus->Kind == NativeTypes.FABRIC_SERVICE_REPLICA_KIND.FABRIC_SERVICE_REPLICA_KIND_KEY_VALUE_STORE)
                {
                    rv.ReplicaStatus = KeyValueStoreReplicaStatus.CreateFromNative(replicaStatus->Value);
                }

                if (ex1->Reserved != IntPtr.Zero)
                {
                    var extended2 = (NativeTypes.FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_DETAIL_QUERY_RESULT_ITEM_EX2*)ex1->Reserved;

                    var deployedServiceReplica = *(NativeTypes.FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_QUERY_RESULT_ITEM*)extended2->DeployedServiceReplica;
                    rv.DeployedServiceReplica = DeployedStatefulServiceReplica.FromNative(deployedServiceReplica);
                }
            }

            return rv;
        }
    }
}