// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.BackupRestoreTypes
{
    using System.Fabric.BackupRestore.BackupRestoreTypes.JsonConverter;
    using System.Fabric.BackupRestore.Enums;
    using System.Globalization;
    using System.Runtime.Serialization;
    using System.Text;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Converters;
    using System.Fabric.BackupRestore.BackupRestoreExceptions;


    /// <summary>
    ///  Represents a recovery point from which backup can be restored
    /// </summary>
    [DataContract]
    public class RestorePoint  
    {
        /// <summary>
        /// Unique restore point ID 
        /// </summary>
        [DataMember]
        public Guid BackupId { get; set; }

        /// <summary>
        /// Unique restore point ID of the previous restore point in chain
        /// </summary>
        internal Guid ParentRestorePointId { get; set; }

        /// <summary>
        /// Unique Restore point chain ID. All restore points part of the same chain has the
        /// same restore point chain id. A restore point chain is comprised of 1 full backup and 
        /// multiple incremental backups
        /// </summary>
        [DataMember]
        public Guid BackupChainId { get; set; }
        
        /// <summary>
        /// Name of the application
        /// </summary>
        [DataMember]
        public Uri ApplicationName { get; set; }

        /// <summary>
        /// Name of the service
        /// </summary>
        [DataMember]
        public Uri ServiceName { get; set; }

        /// <summary>
        /// Gets or sets the partition information
        /// </summary>
        [DataMember]
        public PartitionInformation PartitionInformation { get; set; }

        /// <summary>
        /// Restore point location
        /// </summary>
        [DataMember]
        public string BackupLocation { get; set; }

        /// <summary>
        /// Denotes the type of backup, full or incremental
        /// </summary>
        [DataMember]
        [JsonConverter(typeof(StringEnumConverter))]
        public BackupOptionType BackupType { get; set; }

        /// <summary>
        /// Epoch of the last backed up record in this backup.
        /// </summary>
        [DataMember]
        public BackupEpoch EpochOfLastBackupRecord { get; set; }

        /// <summary>
        /// LSN of the last backed up record in this backup.
        /// </summary>
        [DataMember]
        [JsonConverter(typeof(LongToStringConverter))]
        public long LsnOfLastBackupRecord { set; get; }

        /// <summary>
        /// Time in UTC when this backup was taken
        /// </summary>
        [DataMember]
        [JsonConverter(typeof(DateTimeJsonConverter))]
        public DateTime CreationTimeUtc { get; set; }

        /// <summary>
        /// Failure for the Backup Point
        /// </summary>
        [DataMember]
        public FabricError FailureError { set; get; }

        /// <summary>
        /// Service Manifest Version.
        /// </summary>
        [DataMember]
        public string ServiceManifestVersion { set; get; }

        /// <summary>
        /// Override ToString
        /// </summary>
        /// <returns>String representation of RecoveryPoint object</returns>
        public override string ToString()
        {
            var sb = new StringBuilder(base.ToString());
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "ApplicationName={0}", this.ApplicationName));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "ServiceName={0}", this.ServiceName));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "PartitionInformation={0}", this.PartitionInformation));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "BackupId={0}", this.BackupId));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "ParentRestorePointId={0}", this.ParentRestorePointId));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "BackupChainId={0}", this.BackupChainId));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "BackupLocation={0}", this.BackupLocation));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "BackupType={0}", this.BackupType));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "EpochOfLastBackupRecord={0}", this.EpochOfLastBackupRecord));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "LsnOfLastBackupRecord={0}", this.LsnOfLastBackupRecord));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "CreationTimeUtc={0}", this.CreationTimeUtc));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "Failure={0}", this.FailureError));
            sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "ServiceManifestVersion={0}", this.ServiceManifestVersion));
            return sb.ToString();
        }
    }
}