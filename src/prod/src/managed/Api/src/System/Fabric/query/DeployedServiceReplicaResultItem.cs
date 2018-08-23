// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Runtime.Serialization;

    /// <summary>
    /// <para>Represents the view of a replica on a node.</para>
    /// </summary>
    [KnownType(typeof(DeployedStatefulServiceReplica))]
    [KnownType(typeof(DeployedStatelessServiceInstance))]
    public abstract class DeployedServiceReplica
    {
        internal DeployedServiceReplica()
        {
        }

        /// <summary>
        ///   <para>Initializes a new instance of the <see cref="System.Fabric.Query.DeployedServiceReplica"/> class.</para>
        /// </summary>
        /// <param name="kind">
        ///   <para>The service kind.</para>
        /// </param>
        internal protected DeployedServiceReplica(ServiceKind kind)
        {
            this.ServiceKind = kind;
            this.ServicePackageActivationId = string.Empty;
        }

        /// <summary>
        /// <para>The type of the replica (Stateful or Stateless).</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.Fabric.Query.ServiceKind" />.</para>
        /// </value>
        [JsonCustomization(AppearanceOrder = -2)]
        public ServiceKind ServiceKind { get; private set; }

        /// <summary>
        /// <para>The name of the service.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the service.</para>
        /// </value>
        public Uri ServiceName { get; internal set; }

        /// <summary>
        /// <para>The name of the service type.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the service type.</para>
        /// </value>
        public string ServiceTypeName { get; internal set; }

        /// <summary>
        /// <para>The version of the service manifest.</para>
        /// </summary>
        /// <value>
        /// <para>The version of the service manifest.</para>
        /// </value>
        [Obsolete("This property is no longer supported", false)]
        public string ServiceManifestVersion { get; internal set; }

        /// <summary>
        /// <para>The name of the code package that hosts this replica.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the code package that hosts this replica.</para>
        /// </value>
        public string CodePackageName { get; internal set; }

        /// <summary>
        /// <para>The partition id for this replia.</para>
        /// </summary>
        /// <value>
        /// <para>The partition id for this replia.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.PartitionId)]
        public Guid Partitionid { get; internal set; }

        /// <summary>
        /// <para>The name of the service package that contains the code package hosting this replica.</para>
        /// </summary>
        /// <value>
        /// <para>Returns <see cref="System.String" />.</para>
        /// </value>
        public string ServiceManifestName { get; internal set; }

        /// <summary>
        /// The ActivationId of service package.
        /// </summary>
        /// <value>
        /// <para>
        /// A string representing ActivationId of deployed service package. 
        /// If <see cref="System.Fabric.Description.ServicePackageActivationMode"/> specified at the time of creating the service is 
        /// <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/> (or if it is not specified, in
        /// which case it defaults to <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/>), then value of 
        /// <see cref="System.Fabric.Query.DeployedServiceReplica.ServicePackageActivationId"/> is always an empty string.
        /// For more details please see <see cref="System.Fabric.Description.ServicePackageActivationMode"/>.
        /// </para>
        /// <remarks>
        /// This can have null value if <see cref="System.Fabric.Query.DeployedServiceReplica.ReplicaStatus"/> is other than <see cref="System.Fabric.Query.ServiceReplicaStatus.InBuild"/>,
        /// <see cref="System.Fabric.Query.ServiceReplicaStatus.Standby"/> or <see cref="System.Fabric.Query.ServiceReplicaStatus.Ready"/>.
        /// </remarks>
        /// </value>
        public string ServicePackageActivationId { get; internal set; }

        /// <summary>
        /// <para>The status of the replica.</para>
        /// </summary>
        /// <value>
        /// <para>The status of the replica.</para>
        /// </value>
        public ServiceReplicaStatus ReplicaStatus { get; internal set; }

        /// <summary>
        /// <para>The last address returned by the replica in Open or ChangeRole.</para>
        /// </summary>
        /// <value>
        /// <para>The last address returned by the replica in Open or ChangeRole.</para>
        /// </value>
        public string Address { get; internal set; }

        /// <summary>
        /// <para>The host process id.</para>
        /// <value>This will be zero if the replica is down.</value>
        /// </summary>
        public long HostProcessId { get; internal set; }

        /// <summary>
        /// Internal property to access ServiceManifestVersion with #pragma warning disable around the accessor
        /// This is because even though the property is obsolete, product and unit test code still needs to validate it is correct
        /// </summary>
#pragma warning disable 0618
        [JsonCustomization(IsIgnored = true)]
        internal string ServiceManifestVersion_
        {
            get
            {
                return this.ServiceManifestVersion;
            }

            set
            {
                this.ServiceManifestVersion = value;
            }
        }
#pragma warning restore 0618

        internal static unsafe DeployedServiceReplica CreateFromNative(
           NativeTypes.FABRIC_DEPLOYED_SERVICE_REPLICA_QUERY_RESULT_ITEM nativeResultItem)
        {
            if ((ServiceKind)nativeResultItem.Kind == ServiceKind.Stateless)
            {
                NativeTypes.FABRIC_DEPLOYED_STATELESS_SERVICE_INSTANCE_QUERY_RESULT_ITEM nativeStatelessInstanceQueryResult =
                    *(NativeTypes.FABRIC_DEPLOYED_STATELESS_SERVICE_INSTANCE_QUERY_RESULT_ITEM*)nativeResultItem.Value;
                return DeployedStatelessServiceInstance.FromNative(nativeStatelessInstanceQueryResult);
            }
            else if ((ServiceKind)nativeResultItem.Kind == ServiceKind.Stateful)
            {
                NativeTypes.FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_QUERY_RESULT_ITEM nativeStatefulServiceQueryResult =
                    *(NativeTypes.FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_QUERY_RESULT_ITEM*)nativeResultItem.Value;
                return DeployedStatefulServiceReplica.FromNative(nativeStatefulServiceQueryResult);
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