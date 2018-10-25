// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;
    using System.Runtime.Serialization;

    /// <summary>
    /// Represents base class for query status of a replica
    /// </summary>
    [KnownType(typeof(KeyValueStoreReplicaStatus))]
    public abstract class ReplicaStatus
    {
        /// <summary>
        /// Represents kind of a replica.
        /// </summary>
        public enum ReplicaKind
        {
            /// <summary>
            /// Represents an invalid replica kind.
            /// </summary>
            Invalid = NativeTypes.FABRIC_SERVICE_REPLICA_KIND.FABRIC_SERVICE_REPLICA_KIND_INVALID,

            /// <summary>
            /// Represents a key value store replica.
            /// </summary>
            KeyValueStore = NativeTypes.FABRIC_SERVICE_REPLICA_KIND.FABRIC_SERVICE_REPLICA_KIND_KEY_VALUE_STORE,
        };

        internal ReplicaStatus(ReplicaKind kind)
        {
            this.Kind = kind;
        }

        /// <summary>
        /// Represents kind of a replica.
        /// </summary>
        /// <value>
        /// A <see cref="System.Fabric.Query.ReplicaStatus.ReplicaKind"/> representing kind of replica.
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Kind, AppearanceOrder = -2)]
        public ReplicaKind Kind { get; private set; }
    };

    /// <summary>
    /// Query status for a key/value store replica
    /// </summary>
    public sealed class KeyValueStoreReplicaStatus : ReplicaStatus
    {
        internal KeyValueStoreReplicaStatus() : base(ReplicaStatus.ReplicaKind.KeyValueStore)
        {
        }

        /// <summary>
        /// Gets a value indicating the estimated number of rows in the underlying database.
        /// </summary>
        /// <value>The estimated number of rows.</value>
        public long DatabaseRowCountEstimate { get; private set; }

        /// <summary>
        /// Gets a value indicating the estimated logical size of the underlying database.
        /// </summary>
        /// <value>The estimated logical database size in bytes.</value>
        public long DatabaseLogicalSizeEstimate { get; private set; }

        /// <summary>
        /// Gets a value indicating the latest key-prefix filter applied to enumeration during the <see cref="System.Fabric.KeyValueStoreReplica.OnCopyComplete(System.Fabric.KeyValueStoreEnumerator)" /> callback. Null if there is no pending callback.
        /// </summary>
        /// <value>The key prefix filter.</value>
        public string CopyNotificationCurrentKeyFilter { get; private set; }

        /// <summary>
        /// Gets a value indicating the latest number of keys enumerated during the <see cref="System.Fabric.KeyValueStoreReplica.OnCopyComplete(System.Fabric.KeyValueStoreEnumerator)" /> callback. 0 if there is no pending callback.
        /// </summary>
        /// <value>The number of keys.</value>
        public long CopyNotificationCurrentProgress { get; private set; }

        /// <summary>
        /// Gets a value indicating the current status details (if any) of the replica.
        /// </summary>
        /// <value>The status.</value>
        public string StatusDetails { get; private set; }

        /// <summary>
        /// Gets a value indicating the type of state provider backing this key/value store replica.
        /// </summary>
        /// <value>The provider kind.</value>
        public KeyValueStoreProviderKind ProviderKind { get; private set; }

        /// <summary>
        /// Gets a value indicating the current state provider migration status (if any)
        /// </summary>
        /// <value>The migration status.</value>
        public KeyValueStoreMigrationStatus MigrationStatus { get; private set; }

        internal static unsafe KeyValueStoreReplicaStatus CreateFromNative(IntPtr nativeKvs)
        {
            if (nativeKvs == IntPtr.Zero) { return null; }

            var castedKvs = (NativeTypes.FABRIC_KEY_VALUE_STORE_STATUS_QUERY_RESULT*)nativeKvs;

            var managedKvs = new KeyValueStoreReplicaStatus();

            managedKvs.DatabaseRowCountEstimate = castedKvs->DatabaseRowCountEstimate;
            managedKvs.DatabaseLogicalSizeEstimate = castedKvs->DatabaseLogicalSizeEstimate;
            managedKvs.CopyNotificationCurrentKeyFilter = NativeTypes.FromNativeString(castedKvs->CopyNotificationCurrentKeyFilter);
            managedKvs.CopyNotificationCurrentProgress = castedKvs->CopyNotificationCurrentProgress;
            managedKvs.StatusDetails = NativeTypes.FromNativeString(castedKvs->StatusDetails);

            if (castedKvs->Reserved != IntPtr.Zero)
            {
                var castedEx1 = (NativeTypes.FABRIC_KEY_VALUE_STORE_STATUS_QUERY_RESULT_EX1*)castedKvs->Reserved;

                managedKvs.ProviderKind = (KeyValueStoreProviderKind)castedEx1->ProviderKind;
                managedKvs.MigrationStatus = KeyValueStoreMigrationStatus.CreateFromNative(castedEx1->MigrationStatus);
            }

            return managedKvs;
        }
    };
}
