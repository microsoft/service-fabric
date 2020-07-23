// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.BackupRestoreTypes.JsonConverter;
using System.Text;

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Fabric.BackupRestore.Enums;
    using System.Runtime.Serialization;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;
    using System.ComponentModel.DataAnnotations;

    /// <summary>
    ///  Describes storage information for backups
    /// </summary>
    [DataContract]
    [KnownType(typeof(FileShareBackupStorageInfo))]
    [KnownType(typeof(AzureBlobBackupStorageInfo))]
    [JsonConverter(typeof(BackupStorageConverter))]

    public class BackupStorage
    {
        public BackupStorage()
        {
            
        }
        /// <summary>
        /// Constructor to initialize from sub class
        /// </summary>
        /// <param name="backupStorageType"></param>
        
        protected internal BackupStorage(BackupStorageType backupStorageType)
        {
            this.FriendlyName = String.Empty;
            this.StorageKind = backupStorageType;
        }

        /// <summary>
        /// Defines the storage type
        /// </summary>
        [JsonConverter(typeof(StringEnumConverter))]
        [JsonProperty(Order = -2)]
        [DataMember]
        [Required]
        public BackupStorageType StorageKind { set; get; }

        [DataMember]
        public string FriendlyName { set; get; }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("StorageType : {0}", this.StorageKind).AppendLine();
            stringBuilder.AppendFormat("FriendlyName : {0}", this.FriendlyName).AppendLine();
            return stringBuilder.ToString();
        }
    }
}