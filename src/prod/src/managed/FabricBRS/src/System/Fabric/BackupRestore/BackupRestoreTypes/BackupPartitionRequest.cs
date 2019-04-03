// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Runtime.Serialization;
    using Newtonsoft.Json;
    using System.Fabric.BackupRestore.BackupRestoreTypes.JsonConverter;
    using System.ComponentModel.DataAnnotations;
    using System.Text;

    /// <summary>
    ///  Backup Partition Request for requesting backup on a specific Partiton
    /// </summary>
    [DataContract]
    public class BackupPartitionRequest
    {
        /// <summary>
        /// Location for storing Backups
        /// </summary>
        [JsonConverter(typeof(NullableBackupStorageConverter))]
        [DataMember]
        public BackupStorage BackupStorage { set; get; }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat("Backup Storage : {0}", this.BackupStorage).AppendLine();
            return stringBuilder.ToString();
        }

    }
}