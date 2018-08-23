// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Fabric.Interop;
    using System.Fabric.Query;

    /// <summary>
    /// <para>Describes the health of a stateful service replica as returned by <see cref="System.Fabric.FabricClient.HealthClient.GetReplicaHealthAsync(Description.ReplicaHealthQueryDescription)" />.</para>
    /// </summary>
    public sealed class StatefulServiceReplicaHealth : ReplicaHealth
    {
        internal StatefulServiceReplicaHealth()
            : base(ServiceKind.Stateful)
        {
        }

        /// <summary>
        /// <para>Gets the replica ID.</para>
        /// </summary>
        /// <value>
        /// <para>The replica ID.</para>
        /// </value>
        public long ReplicaId
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

        internal static unsafe StatefulServiceReplicaHealth FromNative(NativeTypes.FABRIC_STATEFUL_SERVICE_REPLICA_HEALTH* nativeHealth)
        {
            var managedHealth = new StatefulServiceReplicaHealth();

            managedHealth.AggregatedHealthState = (HealthState)nativeHealth->AggregatedHealthState;
            managedHealth.HealthEvents = HealthEvent.FromNativeList(nativeHealth->HealthEvents);
            managedHealth.PartitionId = nativeHealth->PartitionId;
            managedHealth.Id = nativeHealth->ReplicaId;

            if (nativeHealth->Reserved != IntPtr.Zero)
            {
                var nativeHealthEx1 = (NativeTypes.FABRIC_STATEFUL_SERVICE_REPLICA_HEALTH_EX1*)nativeHealth->Reserved;
                managedHealth.UnhealthyEvaluations = HealthEvaluation.FromNativeList(nativeHealthEx1->UnhealthyEvaluations);
            }

            return managedHealth;
        }
    }
}