// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Query;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Runtime.Serialization;
    using System.Text;

    /// <summary>
    /// <para>Describes the health of a stateful service replica or a stateless service instance as returned by <see cref="System.Fabric.FabricClient.HealthClient.GetReplicaHealthAsync(Description.ReplicaHealthQueryDescription)" />.</para>
    /// </summary>
    [KnownType(typeof(StatefulServiceReplicaHealth))]
    [KnownType(typeof(StatelessServiceInstanceHealth))]
    public abstract class ReplicaHealth : EntityHealth
    {
        internal ReplicaHealth(ServiceKind kind)
        {
            this.Kind = kind;
        }

        /// <summary>
        /// <para>Gets the kind of the replica, either stateless or stateful. Based on this, the replica health can be converted to either 
        /// <see cref="System.Fabric.Health.StatefulServiceReplicaHealth" /> or <see cref="System.Fabric.Health.StatelessServiceInstanceHealth" />.</para>
        /// </summary>
        /// <value>
        /// <para>The service kind, which indicates whether the replica is stateful or stateless.</para>
        /// </value>
        [JsonCustomization(PropertyName = "ServiceKind")]
        public ServiceKind Kind
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the partition identifier.</para>
        /// </summary>
        /// <value>
        /// <para>The partition ID.</para>
        /// </value>
        public Guid PartitionId
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the stateful replica ID or the stateless service instance ID.</para>
        /// </summary>
        /// <value>
        /// <para>The stateful replica ID or stateless instance ID.</para>
        /// </value>
        [JsonCustomization(IsIgnored = true)]
        public long Id
        {
            get;
            internal set;
        }

        /// <summary>
        /// Gets a string representation of the <see cref="System.Fabric.Health.ReplicaHealth"/>.
        /// </summary>
        /// <returns>A string representation of the <see cref="System.Fabric.Health.ReplicaHealth"/>.</returns>
        public override string ToString()
        {
            var sb = new StringBuilder();
            sb.Append(
                string.Format(
                    CultureInfo.CurrentCulture,
                    "ReplicaHealth {0}+{1}: {2}, {3} events",
                    this.PartitionId,
                    this.Id,
                    this.AggregatedHealthState,
                    this.HealthEvents.Count));
            
            if (this.UnhealthyEvaluations != null && this.UnhealthyEvaluations.Count > 0)
            {
                sb.Append(string.Format(CultureInfo.CurrentCulture, ", {0}", this.UnhealthyEvaluations));
            }

            return sb.ToString();
        }

        internal static unsafe ReplicaHealth FromNativeResult(NativeClient.IFabricReplicaHealthResult nativeResult)
        {
            var nativePtr = nativeResult.get_ReplicaHealth();
            ReleaseAssert.AssertIf(nativePtr == IntPtr.Zero, string.Format(CultureInfo.CurrentCulture, StringResources.Error_NativeDataNull_Formatted, "ReplicaHealth"));

            var retval = FromNative((NativeTypes.FABRIC_REPLICA_HEALTH*)nativePtr);

            GC.KeepAlive(nativeResult);
            return retval;
        }

        private static unsafe ReplicaHealth FromNative(NativeTypes.FABRIC_REPLICA_HEALTH* nativeHealth)
        {
            if (nativeHealth->Kind == NativeTypes.FABRIC_SERVICE_KIND.FABRIC_SERVICE_KIND_STATELESS)
            {
                return StatelessServiceInstanceHealth.FromNative((NativeTypes.FABRIC_STATELESS_SERVICE_INSTANCE_HEALTH*)nativeHealth->Value);
            }
            else if (nativeHealth->Kind == NativeTypes.FABRIC_SERVICE_KIND.FABRIC_SERVICE_KIND_STATEFUL)
            {
                return StatefulServiceReplicaHealth.FromNative((NativeTypes.FABRIC_STATEFUL_SERVICE_REPLICA_HEALTH*)nativeHealth->Value);
            }
            else
            {
                AppTrace.TraceSource.WriteError("ReplicaHealth.FromNative", "Unknown service kind: {0}", nativeHealth->Kind);
                ReleaseAssert.Failfast(string.Format(CultureInfo.CurrentCulture, StringResources.Error_ServiceKindInvalid_Formatted, nativeHealth->Kind));
                return null;
            }
        }
    }
}