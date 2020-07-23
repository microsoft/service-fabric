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
    using System.Globalization;
    using System.Text;
    using System.Fabric.BackupRestore.BackupRestoreTypes.JsonConverter;
    using System.Fabric.BackupRestore.BackupRestoreExceptions;

    /// <summary>
    ///  Represents a backup partition response
    /// </summary>
    [DataContract]
    public class BackupProgress
    {
        /// <summary>
        /// Specifies the current state of the Backup Requested.
        /// </summary>
        [JsonConverter(typeof(StringEnumConverter))]
        [DataMember]
        public BackupState BackupState { get; set; }


        /// <summary>
        /// TimeStamp when operation succeded or failed
        /// </summary>
        [DataMember]
        [JsonConverter(typeof(DateTimeJsonConverter))]
        public DateTime TimeStampUtc { get; set; }

        /// <summary>
        /// Backup ID
        /// </summary>
        [DataMember]
        public Guid BackupId { get; set; }

        /// <summary>
        /// Backup location relative to the backup store
        /// </summary>
        [DataMember]
        public string BackupLocation { get; set; }

        /// <summary>
        /// Gets or sets the Epoch of last backed up record
        /// </summary>
        [DataMember]
        public BackupEpoch EpochOfLastBackupRecord { get; set; }

        /// <summary>
        /// Gets or sets the LSN of last backed up record
        /// </summary>
        [DataMember]
        public long LsnOfLastBackupRecord { get; set; }

        /// <summary>
        /// Contains Details of the Error
        /// </summary>
        [DataMember]
        public FabricError FailureError { set; get; }

        /// <summary>
        /// Override ToString
        /// </summary>
        /// <returns>String representation of RecoveryPoint object</returns>
        public override string ToString()
        {
            var sb = new StringBuilder(base.ToString());
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "BackupState={0}", this.BackupState));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "TimeStampUtc={0}", this.TimeStampUtc));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "BackupId={0}", this.BackupId));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "BackupLocation={0}", this.BackupLocation));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "EpochOfLastBackupRecord={0}", this.EpochOfLastBackupRecord));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "LsnOfLastBackupRecord={0}", this.LsnOfLastBackupRecord));
            return sb.ToString();
        }

    }
}