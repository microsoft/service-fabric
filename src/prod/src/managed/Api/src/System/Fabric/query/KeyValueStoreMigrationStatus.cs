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
    /// Query status for the migration workflow of a key/value store replica
    /// </summary>
    public sealed class KeyValueStoreMigrationStatus
    {
        internal KeyValueStoreMigrationStatus()
        {
        }

        /// <summary>
        /// Gets a value indicating the current migration phase
        /// </summary>
        /// <value>The current migration phase</value>
        public KeyValueStoreMigrationPhase CurrentPhase { get; private set; }
        
        /// <summary>
        /// Gets a value indicating the state of the current migration phase
        /// </summary>
        /// <value>The current state.</value>
        public KeyValueStoreMigrationState State { get; private set; }

        
        /// <summary>
        /// Gets a value indicating the next migration phase after the current phase completes
        /// </summary>
        /// <value>The next migration phase.</value>
        public KeyValueStoreMigrationPhase NextPhase { get; private set; }

        internal static unsafe KeyValueStoreMigrationStatus CreateFromNative(IntPtr nativeStatus)
        {
            if (nativeStatus == IntPtr.Zero) { return null; }

            var castedStatus = (NativeTypes.FABRIC_KEY_VALUE_STORE_MIGRATION_QUERY_RESULT*)nativeStatus;

            var managedStatus = new KeyValueStoreMigrationStatus();

            managedStatus.CurrentPhase = (KeyValueStoreMigrationPhase)castedStatus->CurrentPhase;
            managedStatus.State = (KeyValueStoreMigrationState)castedStatus->State;
            managedStatus.NextPhase = (KeyValueStoreMigrationPhase)castedStatus->NextPhase;

            return managedStatus;
        }
    };
}
