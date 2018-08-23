// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Interop;

    /// <summary>
    /// Indicates a phase in the overall key/value store migration workflow.
    /// </summary>
    public enum KeyValueStoreMigrationPhase
    {
        /// <summary>
        /// The migration is currently inactive.
        /// </summary>
        Inactive = NativeTypes.FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE.FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE_INACTIVE,
        
        /// <summary>
        /// The replica is migrating data between the source and target databases.
        /// </summary>
        Migration = NativeTypes.FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE.FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE_MIGRATION,
        
        /// <summary>
        /// The replica is swapping from the source to target database.
        /// </summary>
        TargetDatabaseSwap = NativeTypes.FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE.FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE_TARGET_DATABASE_SWAP,
        
        /// <summary>
        /// The replica is cleaning up the target database.
        /// </summary>
        TargetDatabaseCleanup = NativeTypes.FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE.FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE_TARGET_DATABASE_CLEANUP,
        
        /// <summary>
        /// The replica is cleaning up the source database.
        /// </summary>
        SourceDatabaseCleanup = NativeTypes.FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE.FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE_SOURCE_DATABASE_CLEANUP,
        
        /// <summary>
        /// The migration target database is attached and running as the active database.
        /// </summary>
        TargetDatabaseActive = NativeTypes.FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE.FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE_TARGET_DATABASE_ACTIVE,
        
        /// <summary>
        /// The source database is being restored from a migration backup.
        /// </summary>
        RestoreSourceBackup = NativeTypes.FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE.FABRIC_KEY_VALUE_STORE_MIGRATION_PHASE_RESTORE_SOURCE_BACKUP,
    }
}
