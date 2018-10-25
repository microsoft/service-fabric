// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Health
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Fabric.Query;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Runtime.Serialization;

    /// <summary>
    /// <para>Represents a base class for stateful service replica or stateless service instance health state.</para>
    /// </summary>
    [KnownType(typeof(StatefulServiceReplicaHealthState))]
    [KnownType(typeof(StatelessServiceInstanceHealthState))]
    public abstract class ReplicaHealthState : EntityHealthState
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Health.ReplicaHealthState" /> class.</para>
        /// </summary>
        /// <param name="kind">The service kind, which shows whether the service is stateful or stateless.</param>
        protected ReplicaHealthState(ServiceKind kind)
        {
            this.Kind = kind;
        }

        /// <summary>
        /// <para>Gets the kind of the service replica.</para>
        /// </summary>
        /// <value>
        /// <para>The service replica kind.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.ServiceKind, AppearanceOrder = -2)]
        public ServiceKind Kind
        {
            get;
            private set;
        }

        /// <summary>
        /// <para>Gets the partition ID.</para>
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
        /// Creates a string description of the replica health state.
        /// </summary>
        /// <returns>String description of the health state.</returns>
        public override string ToString()
        {
            return string.Format(
                CultureInfo.CurrentCulture,
                "{0}+{1}: {2}",
                this.PartitionId,
                this.Id,
                this.AggregatedHealthState);
        }

        /// <summary>
        /// <para>Gets the stateful replica ID or stateless service instance ID.</para>
        /// </summary>
        /// <value>
        /// <para>The replica or instance ID.</para>
        /// </value>
        [JsonCustomization(IsIgnored = true)]
        public long Id
        {
            get;
            internal set;
        }

        internal static unsafe IList<ReplicaHealthState> FromNativeList(IntPtr nativeListPtr)
        {
            if (nativeListPtr != IntPtr.Zero)
            {
                var nativeList = (NativeTypes.FABRIC_REPLICA_HEALTH_STATE_LIST*)nativeListPtr;
                return ReplicaHealthStateList.FromNativeList(nativeList);
            }
            else
            {
                return new ReplicaHealthStateList();
            }
        }

        internal static unsafe ReplicaHealthState FromNative(NativeTypes.FABRIC_REPLICA_HEALTH_STATE nativeState)
        {
            if (nativeState.Kind == NativeTypes.FABRIC_SERVICE_KIND.FABRIC_SERVICE_KIND_STATELESS)
            {
                return StatelessServiceInstanceHealthState.FromNative(*(NativeTypes.FABRIC_STATELESS_SERVICE_INSTANCE_HEALTH_STATE*)nativeState.Value);
            }
            else if (nativeState.Kind == NativeTypes.FABRIC_SERVICE_KIND.FABRIC_SERVICE_KIND_STATEFUL)
            {
                return StatefulServiceReplicaHealthState.FromNative(*(NativeTypes.FABRIC_STATEFUL_SERVICE_REPLICA_HEALTH_STATE*)nativeState.Value);
            }
            else
            {
                AppTrace.TraceSource.WriteError("ReplicaHealthState.FromNative", "Unknown service kind: {0}", nativeState.Kind);
                ReleaseAssert.Failfast(string.Format(CultureInfo.CurrentCulture, StringResources.Error_ServiceKindInvalid_Formatted, nativeState.Kind));
                return null;
            }
        }

        // Method used by JsonSerializer to resolve derived type using json property "ServiceKind".
        // The base class needs to have attributes [KnownType()]
        // This method must be static with one parameter input which represented by given json property.
        // Provide name of the json property which will be used as parameter to this method.
        [DerivedTypeResolverAttribute(JsonPropertyNames.ServiceKind)]
        internal static Type ResolveDerivedClass(ServiceKind kind)
        {
            switch (kind)
            {
                case ServiceKind.Stateless:
                    return typeof(StatelessServiceInstanceHealthState);

                case ServiceKind.Stateful:
                    return typeof(StatefulServiceReplicaHealthState);

                default:
                    return null;
            }
        }
    }
}