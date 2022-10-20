// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Runtime.Serialization;
    using System.ComponentModel.DataAnnotations;
    using System.Fabric.BackupRestore.Enums;
    using System.Text;
    using Newtonsoft.Json;

    /// <summary>
    /// Describes the File Share Storage
    /// </summary>
    [DataContract]
    
    public class FileShareBackupStorageInfo : BackupStorage
    {
        /// <summary>
        /// Default constructor
        /// </summary>
        public FileShareBackupStorageInfo() : base(BackupStorageType.FileShare)
        {
        }

        /// <summary>
        /// Location of File Share for storing Backups
        /// </summary>
        [DataMember]
        [Required]
        public string Path { set; get; }

        /// <summary>
        /// Primary User name to access the FileShare.
        /// </summary>
        [DataMember]
        public string PrimaryUserName { set; get; }

        /// <summary>
        /// Password to access the share location
        /// </summary>
        [DataMember]
        public string PrimaryPassword { set; get; }

       /// <summary>
        /// Secondary User name to access the FileShare.
        /// </summary>
        [DataMember]
        public string SecondaryUserName { set; get; }

        /// <summary>
        /// Password to access the share location
        /// </summary>
        [DataMember]
        public string SecondaryPassword { set; get; }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder(base.ToString());
            stringBuilder.AppendFormat("FileSharePath : {0}", this.Path).AppendLine();
            stringBuilder.AppendFormat("PrimaryUserName : {0}", this.PrimaryUserName).AppendLine();
            stringBuilder.AppendFormat("SecondaryUserName : {0}", this.SecondaryUserName).AppendLine();
            return stringBuilder.ToString();
        }
    }
}