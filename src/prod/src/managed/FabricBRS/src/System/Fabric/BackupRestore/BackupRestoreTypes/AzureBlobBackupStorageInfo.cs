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
    /// Describes the Azure Storage
    /// </summary>
    [DataContract]
    public class AzureBlobBackupStorageInfo : BackupStorage
    {
        /// <summary>
        /// Default constructor
        /// </summary>
        public AzureBlobBackupStorageInfo() : base(BackupStorageType.AzureBlobStore)
        {
            
        }

        /// <summary>
        /// Connection String for Azure Storage
        /// </summary>
        [DataMember]
        [Required]
        public string ConnectionString { set; get; }

        /// <summary>
        /// Container Name to store Backup's
        /// </summary>
        [DataMember]
        [Required]
        public string ContainerName { set; get; }

       public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder(base.ToString());
            stringBuilder.AppendFormat("ContainerName : {0}", this.ContainerName).AppendLine();
            return stringBuilder.ToString();
        }
    }
}