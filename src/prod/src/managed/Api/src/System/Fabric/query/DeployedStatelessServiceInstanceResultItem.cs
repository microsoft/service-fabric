// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a deployed stateless service instance.</para>
    /// </summary>
    public sealed class DeployedStatelessServiceInstance : DeployedServiceReplica
    {
        /// <summary>
        /// <para>Initializes an instance of the <see cref="System.Fabric.Query.DeployedStatelessServiceInstance" /> class.</para>
        /// </summary>
        public DeployedStatelessServiceInstance()
            : base(Query.ServiceKind.Stateless)
        { }

        /// <summary>
        /// <para>Gets the instance ID.</para>
        /// </summary>
        /// <value>
        /// <para>The instance ID.</para>
        /// </value>
        public long InstanceId { get; internal set; }

        internal static unsafe DeployedStatelessServiceInstance FromNative(NativeTypes.FABRIC_DEPLOYED_STATELESS_SERVICE_INSTANCE_QUERY_RESULT_ITEM nativeObj)
        {
            var serviceManifestName = string.Empty;
            var servicePackageActivationId = string.Empty;
            long hostProcessId = 0;
            if (nativeObj.Reserved != IntPtr.Zero)
            {
                var extended = (NativeTypes.FABRIC_DEPLOYED_STATELESS_SERVICE_INSTANCE_QUERY_RESULT_ITEM_EX1*)nativeObj.Reserved;
                serviceManifestName = NativeTypes.FromNativeString(extended->ServiceManifestName);

                if (extended->Reserved != IntPtr.Zero)
                {
                    var nativeResultItemEx2 =
                        (NativeTypes.FABRIC_DEPLOYED_STATELESS_SERVICE_INSTANCE_QUERY_RESULT_ITEM_EX2*)extended->Reserved;

                    servicePackageActivationId = NativeTypes.FromNativeString(nativeResultItemEx2->ServicePackageActivationId);

                    if(nativeResultItemEx2->Reserved != IntPtr.Zero)
                    {
                        var extended3 = (NativeTypes.FABRIC_DEPLOYED_STATELESS_SERVICE_INSTANCE_QUERY_RESULT_ITEM_EX3*)nativeResultItemEx2->Reserved;
                        hostProcessId = extended3->HostProcessId;
                    }
                }
            }

            return new DeployedStatelessServiceInstance
            {
                ServiceName = new Uri(NativeTypes.FromNativeString(nativeObj.ServiceName)),
                ServiceTypeName = NativeTypes.FromNativeString(nativeObj.ServiceTypeName),
                ServiceManifestName = serviceManifestName,
                ServicePackageActivationId = servicePackageActivationId,
                CodePackageName = NativeTypes.FromNativeString(nativeObj.CodePackageName),
                Partitionid = nativeObj.PartitionId,
                ReplicaStatus = (ServiceReplicaStatus)nativeObj.ReplicaStatus,
                Address = NativeTypes.FromNativeString(nativeObj.Address),
                InstanceId = nativeObj.InstanceId,
                ServiceManifestVersion_ = NativeTypes.FromNativeString(nativeObj.ServiceManifestVersion),
                HostProcessId = hostProcessId,
            };
        }
    }
}