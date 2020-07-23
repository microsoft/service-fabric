// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Runtime.Serialization;
    using Newtonsoft.Json;
    using System.ComponentModel.DataAnnotations;

    /// <summary>
    /// Describes the Meta Data of Applying Backup Policies to Application/Service/Partition ID
    /// </summary>
    [DataContract]
    public class CleanBackupView
    {
        /// <summary>
        /// Name of the Backup Policy to be applied
        /// </summary>
        [DataMember]
        [Required]
        public bool CleanBackup { set; get; } = false;

        public CleanBackupView(bool cleanBackup = false)
        {
            this.CleanBackup = cleanBackup;
        }
    }
}