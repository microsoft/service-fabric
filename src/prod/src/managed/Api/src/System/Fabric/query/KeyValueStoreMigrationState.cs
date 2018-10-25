// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;

    /// <summary>
    /// Indicates the underlying state of an ongoing key/value store migration phase (see <see cref="System.Fabric.Query.KeyValueStoreMigrationPhase" />).
    /// </summary>
    public enum KeyValueStoreMigrationState
    {
        /// <summary>
        /// The migration is currently inactive.
        /// </summary>
        Inactive = NativeTypes.FABRIC_KEY_VALUE_STORE_MIGRATION_STATE.FABRIC_KEY_VALUE_STORE_MIGRATION_STATE_INACTIVE,
        
        /// <summary>
        /// The current migration phase is actively being processed.
        /// </summary>
        Processing = NativeTypes.FABRIC_KEY_VALUE_STORE_MIGRATION_STATE.FABRIC_KEY_VALUE_STORE_MIGRATION_STATE_PROCESSING,
        
        /// <summary>
        /// The current migration phase has been completed.
        /// </summary>
        Completed = NativeTypes.FABRIC_KEY_VALUE_STORE_MIGRATION_STATE.FABRIC_KEY_VALUE_STORE_MIGRATION_STATE_COMPLETED,
        
        /// <summary>
        /// The current migration phase has been canceled.
        /// </summary>
        Canceled = NativeTypes.FABRIC_KEY_VALUE_STORE_MIGRATION_STATE.FABRIC_KEY_VALUE_STORE_MIGRATION_STATE_CANCELED,
        
        /// <summary>
        /// The current migration phase has failed.
        /// </summary>
        Failed = NativeTypes.FABRIC_KEY_VALUE_STORE_MIGRATION_STATE.FABRIC_KEY_VALUE_STORE_MIGRATION_STATE_FAILED,
    }
}
