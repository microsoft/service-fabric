// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a deployed stateful service replica.</para>
    /// </summary>
    public sealed class DeployedStatefulServiceReplica : DeployedServiceReplica
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Query.DeployedStatefulServiceReplica" /> class.</para>
        /// </summary>
        public DeployedStatefulServiceReplica()
            : base(Query.ServiceKind.Stateful)
        {
        }

        /// <summary>
        /// <para>Gets the replica ID.</para>
        /// </summary>
        /// <value>
        /// <para>The replica ID.</para>
        /// </value>
        public long ReplicaId { get; internal set; }

        /// <summary>
        /// <para>Gets the current replica role.</para>
        /// </summary>
        /// <value>
        /// <para>The replica role.</para>
        /// </value>
        public ReplicaRole ReplicaRole { get; internal set; }

        /// <summary>
        /// <para>Gets additional information about the current reconfiguration like previous configuration role,
        ///  reconfiguration type, reconfiguration phase and reconfiguration start date time.</para>
        /// <value>Reconfiguration Information.</value>
        /// </summary>
        public ReconfigurationInformation ReconfigurationInformation { get; internal set; }

        internal static unsafe DeployedStatefulServiceReplica FromNative(NativeTypes.FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_QUERY_RESULT_ITEM nativeObj)
        {
            var serviceManifestName = string.Empty;
            string servicePackageActivationId = null;
            long hostProcessId = 0;
            var reconfigInformation = new ReconfigurationInformation();
            if (nativeObj.Reserved != IntPtr.Zero)
            {
                var extended = (NativeTypes.FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_QUERY_RESULT_ITEM_EX1*)nativeObj.Reserved;
                serviceManifestName = NativeTypes.FromNativeString(extended->ServiceManifestName);

                if (extended->Reserved != IntPtr.Zero)
                {
                    var nativeResultItemEx2 =
                        (NativeTypes.FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_QUERY_RESULT_ITEM_EX2*)extended->Reserved;

                    servicePackageActivationId = NativeTypes.FromNativeString(nativeResultItemEx2->ServicePackageActivationId);

                    if(nativeResultItemEx2->Reserved != IntPtr.Zero)
                    {
                        var extended3 = (NativeTypes.FABRIC_DEPLOYED_STATEFUL_SERVICE_REPLICA_QUERY_RESULT_ITEM_EX3*)nativeResultItemEx2->Reserved;
                        hostProcessId = extended3->HostProcessId;

                        var reconfigurationInformation = (NativeTypes.FABRIC_RECONFIGURATION_INFORMATION_QUERY_RESULT*)extended3->ReconfigurationInformation;
                        reconfigInformation = new ReconfigurationInformation(
                            (ReplicaRole)reconfigurationInformation->PreviousConfigurationRole,
                            (ReconfigurationPhase)reconfigurationInformation->ReconfigurationPhase,
                            (ReconfigurationType)reconfigurationInformation->ReconfigurationType,
                            NativeTypes.FromNativeFILETIME(reconfigurationInformation->ReconfigurationStartTimeUtc));
                    }
                }
            }

            return new DeployedStatefulServiceReplica
            {
                ServiceName = new Uri(NativeTypes.FromNativeString(nativeObj.ServiceName)),
                ServiceTypeName = NativeTypes.FromNativeString(nativeObj.ServiceTypeName),
                CodePackageName = NativeTypes.FromNativeString(nativeObj.CodePackageName),
                ServiceManifestName = serviceManifestName,
                ServicePackageActivationId = servicePackageActivationId,
                Partitionid = nativeObj.PartitionId,
                ReplicaStatus = (ServiceReplicaStatus)nativeObj.ReplicaStatus,
                Address = NativeTypes.FromNativeString(nativeObj.Address),
                ReplicaId = nativeObj.ReplicaId,
                ReplicaRole = (ReplicaRole)nativeObj.ReplicaRole,
                ServiceManifestVersion_ = NativeTypes.FromNativeString(nativeObj.ServiceManifestVersion),
                HostProcessId = hostProcessId,
                ReconfigurationInformation = reconfigInformation,
            };
        }
    }

}