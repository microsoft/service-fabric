// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Runtime.Serialization;

    /// <summary>
    /// <para>Provides statistics about the Service Fabric Replicator.</para>
    /// </summary>
    [KnownType(typeof(SecondaryReplicatorStatus))]
    [KnownType(typeof(PrimaryReplicatorStatus))]
    public abstract class ReplicatorStatus
    {
        private readonly ReplicaRole role;

        /// <summary>
        /// Base ReplicatorStatus structure for both primary and secondary replicators
        /// </summary>
        internal ReplicatorStatus()
        {
        }

        /// <summary>
        /// Base ReplicatorStatus structure for both primary and secondary replicators
        /// </summary>
        /// <param name="role">The replica role.</param>
        protected ReplicatorStatus(ReplicaRole role)
        {
            this.role = role;
        }

        // Matching the properties with it's REST representation
        [JsonCustomization(PropertyName = JsonPropertyNames.Kind, AppearanceOrder = -2)]
        internal ReplicaRole ReplicaRole
        {
            get
            {
                return this.role;
            }
        }

        internal static unsafe ReplicatorStatus FromNative(NativeTypes.FABRIC_REPLICATOR_STATUS_QUERY_RESULT* nativeEntryPoint)
        {
            if (nativeEntryPoint->Role == NativeTypes.FABRIC_REPLICA_ROLE.FABRIC_REPLICA_ROLE_PRIMARY)
            {
                return PrimaryReplicatorStatus.CreateFromNative((NativeTypes.FABRIC_PRIMARY_REPLICATOR_STATUS_QUERY_RESULT*)nativeEntryPoint->Value);
            }
            else if (nativeEntryPoint->Role == NativeTypes.FABRIC_REPLICA_ROLE.FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY || nativeEntryPoint->Role == NativeTypes.FABRIC_REPLICA_ROLE.FABRIC_REPLICA_ROLE_IDLE_SECONDARY)
            {
                return SecondaryReplicatorStatus.CreateFromNative((NativeTypes.FABRIC_SECONDARY_REPLICATOR_STATUS_QUERY_RESULT*)nativeEntryPoint->Value);
            }
            else
            {
                AppTrace.TraceSource.WriteNoise(
                    "ReplicatorStatus.CreateFromNative",
                    "Ignoring the result with unsupported Role value {0}",
                    (int)nativeEntryPoint->Role);

                return null;
            }
        }
    }
}