// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Query;

    /// <summary>
    /// <para>Represents the health state of a stateless service instance health entity, which contains the instance id and the aggregated health state.</para>
    /// </summary>
    public sealed class StatelessServiceInstanceHealthState : ReplicaHealthState
    {
        internal StatelessServiceInstanceHealthState()
            : base(ServiceKind.Stateless)
        {
        }

        /// <summary>
        /// <para>Gets the instance ID.</para>
        /// </summary>
        /// <value>
        /// <para>The instance ID.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.ReplicaId)]
        public long InstanceId
        {
            get
            {
                return this.Id;
            }

            internal set
            {
                this.Id = value;
            }
        }

        internal static unsafe StatelessServiceInstanceHealthState FromNative(NativeTypes.FABRIC_STATELESS_SERVICE_INSTANCE_HEALTH_STATE nativeState)
        {
            var statelessServiceInstanceState = new StatelessServiceInstanceHealthState();

            statelessServiceInstanceState.Id = nativeState.InstanceId;
            statelessServiceInstanceState.PartitionId = nativeState.PartitionId;
            statelessServiceInstanceState.AggregatedHealthState = (HealthState)nativeState.AggregatedHealthState;

            return statelessServiceInstanceState;
        }
    }
}