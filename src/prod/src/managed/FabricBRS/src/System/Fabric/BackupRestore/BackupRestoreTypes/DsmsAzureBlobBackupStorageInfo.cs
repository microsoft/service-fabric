// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Fabric.BackupRestore.Enums;
    using System.Runtime.Serialization;
    using Newtonsoft.Json;
    using System.ComponentModel.DataAnnotations;
    using System.Text;

    /// <summary>
    /// Describes the dSMS managed Azure Blob Storage
    /// </summary>
    [DataContract]
    public class DsmsAzureBlobBackupStorageInfo : BackupStorage
    {
        /// <summary>
        /// Default constructor
        /// </summary>
        public DsmsAzureBlobBackupStorageInfo() : base(BackupStorageType.DsmsAzureBlobStore)
        {
            
        }

        /// <summary>
        /// Source location of the storage credentials.
        /// </summary>
        [DataMember]
        [Required]
        public string StorageCredentialsSourceLocation { set; get; }

        /// <summary>
        /// Container Name to store Backups
        /// </summary>
        [DataMember]
        [Required]
        public string ContainerName { set; get; }

       public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder(base.ToString());
            stringBuilder.AppendFormat("StorageCredentialsSourceLocation : {0}", this.StorageCredentialsSourceLocation).AppendLine();
            stringBuilder.AppendFormat("ContainerName : {0}", this.ContainerName).AppendLine();
            return stringBuilder.ToString();
        }
    }
}