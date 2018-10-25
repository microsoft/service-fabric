// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    /// <summary>
    /// Indicates the kind of the backup.
    /// </summary>
    public enum BackupOption : int
    {
        /// <summary>
        /// A full backup of all state managed by the <see cref="IReliableStateManager"/>.
        /// </summary>
        Full = 0,

        /// <summary>
        /// Incremental backup of the replica. i.e. only the changes 
        /// since the last full or incremental backup will be backed up.
        /// </summary>
        Incremental = 1,
    }
}