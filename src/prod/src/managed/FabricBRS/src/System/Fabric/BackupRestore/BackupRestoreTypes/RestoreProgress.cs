// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Fabric.BackupRestore.Enums;
    using System.Runtime.Serialization;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;
    using System.Fabric.BackupRestore.BackupRestoreExceptions;
    using System.Text;
    using System.Fabric.BackupRestore.BackupRestoreTypes.JsonConverter;

    /// <summary>
    ///  Represents a restore request response
    /// </summary>
    [DataContract]
    public class RestoreProgress
    {
        /// <summary>
        /// Backup Location from where it needs to be restored 
        /// </summary>
        [JsonConverter(typeof(StringEnumConverter))]
        [DataMember]
        public RestoreState RestoreState { get; set; }

        /// <summary>
        /// Details about the restore
        /// </summary>
        /// <summary>
        /// Timestamp when restore completed
        /// </summary>
        [DataMember]
        [JsonConverter(typeof(DateTimeJsonConverter))]
        public DateTime TimeStampUtc { set; get; }

        /// <summary>
        /// Restore Epoch
        /// </summary>
        [DataMember]
        public BackupEpoch RestoredEpoch { set; get; }

        /// <summary>
        /// Restore Lsn for the Partition
        /// </summary>
        [DataMember]
        public long RestoredLsn { set; get; }

        /// <summary>
        /// Reason for failure of the Restore
        /// </summary>
        public FabricError FailureError { set; get; }

        public override string ToString()
        {
            StringBuilder stringBuilder = new StringBuilder();
            stringBuilder.AppendFormat(base.ToString()).AppendLine();
            stringBuilder.AppendFormat("RestoreEpoch : {0}", this.RestoredEpoch).AppendLine();
            stringBuilder.AppendFormat("RestoreLsn : {0}", this.RestoredLsn).AppendLine();
            stringBuilder.AppendFormat("TimeStamp : {0}", this.TimeStampUtc).AppendLine();
            return stringBuilder.ToString();
        }

    }
}