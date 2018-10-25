// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Health;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a stateless service instance.</para>
    /// </summary>
    public sealed class StatelessServiceInstance : Replica
    {
        internal StatelessServiceInstance(
            ServiceReplicaStatus replicaStatus,
            HealthState healthState,
            string replicaAddress,
            string nodeName,
            long instanceId,
            TimeSpan lastInBuildDuration)
            : base(ServiceKind.Stateless, instanceId, replicaStatus, healthState, replicaAddress, nodeName, lastInBuildDuration)
        {
        }

        private StatelessServiceInstance()
            : this(ServiceReplicaStatus.Invalid, HealthState.Invalid, null, null, 0, TimeSpan.Zero)
        { }

        internal long InstanceId
        {
            get
            {
                return this.Id;
            }

            private set
            {
                this.Id = value;
            }
        }

        internal static unsafe StatelessServiceInstance FromNative(NativeTypes.FABRIC_STATELESS_SERVICE_INSTANCE_QUERY_RESULT_ITEM statelessInstanceResultItem)
        {
            return new StatelessServiceInstance(
                (System.Fabric.Query.ServiceReplicaStatus)statelessInstanceResultItem.ReplicaStatus,
                (HealthState)statelessInstanceResultItem.HealthState,
                NativeTypes.FromNativeString(statelessInstanceResultItem.ReplicaAddress),
                NativeTypes.FromNativeString(statelessInstanceResultItem.NodeName),
                statelessInstanceResultItem.InstanceId,
                TimeSpan.FromSeconds(statelessInstanceResultItem.LastInBuildDurationInSeconds));
        }
    }
}