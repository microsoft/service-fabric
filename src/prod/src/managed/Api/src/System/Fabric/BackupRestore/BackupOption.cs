// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore
{
    /// <summary>
    /// Describes the Details of Backup Taken 
    /// </summary>
    public enum BackupOption
    {
        /// <summary>
        /// Defines a full Backup
        /// </summary>
        Full,
        /// <summary>
        /// Defines a Incremental Backup
        /// </summary>
        Incremental,
    }
}