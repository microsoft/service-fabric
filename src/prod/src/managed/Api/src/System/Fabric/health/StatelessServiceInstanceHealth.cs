// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Fabric.Interop;
    using System.Fabric.Query;

    /// <summary>
    /// <para>Describes the health of the stateless instance as returned by <see cref="System.Fabric.FabricClient.HealthClient.GetReplicaHealthAsync(Description.ReplicaHealthQueryDescription)" />.</para>
    /// </summary>
    public sealed class StatelessServiceInstanceHealth : ReplicaHealth
    {
        internal StatelessServiceInstanceHealth()
            : base(ServiceKind.Stateless)
        {
        }

        /// <summary>
        /// <para>Gets the instance ID of the stateless service instance.</para>
        /// </summary>
        /// <value>
        /// <para>The instance ID.</para>
        /// </value>
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

        internal static unsafe StatelessServiceInstanceHealth FromNative(NativeTypes.FABRIC_STATELESS_SERVICE_INSTANCE_HEALTH* nativeHealth)
        {
            var managedHealth = new StatelessServiceInstanceHealth();

            managedHealth.AggregatedHealthState = (HealthState)nativeHealth->AggregatedHealthState;
            managedHealth.HealthEvents = HealthEvent.FromNativeList(nativeHealth->HealthEvents);
            managedHealth.PartitionId = nativeHealth->PartitionId;
            managedHealth.Id = nativeHealth->InstanceId;

            if (nativeHealth->Reserved != IntPtr.Zero)
            {
                var nativeHealthEx1 = (NativeTypes.FABRIC_STATELESS_SERVICE_INSTANCE_HEALTH_EX1*)nativeHealth->Reserved;
                managedHealth.UnhealthyEvaluations = HealthEvaluation.FromNativeList(nativeHealthEx1->UnhealthyEvaluations);
            }

            return managedHealth;
        }
    }
}