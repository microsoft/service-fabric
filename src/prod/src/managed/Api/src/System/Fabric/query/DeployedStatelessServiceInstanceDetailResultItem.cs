// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents the information about a stateless instance running in a code package.</para>
    /// </summary>
    public sealed class DeployedStatelessServiceInstanceDetail : DeployedServiceReplicaDetail
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Query.DeployedStatelessServiceInstanceDetail" /> class.</para>
        /// </summary>
        public DeployedStatelessServiceInstanceDetail()
            : base(ServiceKind.Stateless)
        { }

        /// <summary>
        /// <para>Gets or sets the instance identifier of this instance.</para>
        /// </summary>
        /// <value>
        /// <para>The instance identifier of this instance.</para>
        /// </value>
        public long InstanceId { get; internal set; }

        /// <summary>
        /// <para>Gets additonal details about deployed service instance like host process id, code package name.</para>
        /// <value>Replica Detail.</value>
        /// </summary>
        public DeployedStatelessServiceInstance DeployedServiceReplicaInstance { get; internal set; }

        internal static unsafe DeployedServiceReplicaDetail CreateFromNative(
           NativeTypes.FABRIC_DEPLOYED_STATELESS_SERVICE_INSTANCE_DETAIL_QUERY_RESULT_ITEM nativeResultItem)
        {
            DeployedStatelessServiceInstance deployedReplicaInstance = new DeployedStatelessServiceInstance();
            if (nativeResultItem.Reserved != IntPtr.Zero)
            {
                var extended1 = (NativeTypes.FABRIC_DEPLOYED_STATELESS_SERVICE_INSTANCE_DETAIL_QUERY_RESULT_ITEM_EX1*)nativeResultItem.Reserved;

                var deployedServiceReplica = *(NativeTypes.FABRIC_DEPLOYED_STATELESS_SERVICE_INSTANCE_QUERY_RESULT_ITEM*)extended1->DeployedServiceReplica;
                deployedReplicaInstance = DeployedStatelessServiceInstance.FromNative(deployedServiceReplica);
            }

            var rv = new DeployedStatelessServiceInstanceDetail
            {
                CurrentServiceOperation = (ServiceOperationName)nativeResultItem.CurrentServiceOperation,
                CurrentServiceOperationStartTimeUtc = NativeTypes.FromNativeFILETIME(nativeResultItem.CurrentServiceOperationStartTimeUtc),
                InstanceId = nativeResultItem.InstanceId,
                PartitionId = nativeResultItem.PartitionId,
                ReportedLoad = LoadMetricReport.CreateFromNativeList((NativeTypes.FABRIC_LOAD_METRIC_REPORT_LIST*)nativeResultItem.ReportedLoad),
                ServiceName = new Uri(NativeTypes.FromNativeString(nativeResultItem.ServiceName)),
                DeployedServiceReplicaInstance = deployedReplicaInstance,
            };

            return rv;
        }
    }
}