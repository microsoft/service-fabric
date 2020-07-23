// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.IO;
using System.Text;
using System.Fabric.BackupRestore.FileFormat;

namespace System.Fabric.BackupRestore
{
    /// <summary>
    /// Properties are key value pairs
    /// </summary>
    internal class RecoveryPointMetadataFileProperties : FileProperties
    {
        private const string BackupTimePropertyName = "BackupTime";

        private const string ParentBackupLocationPropertyName = "ParentBackupLocation";

        private const string BackupLocationPropertyName = "BackupLocation";

        private const string BackupIdPropertyName = "BackupId";

        private const string ParentBackupIdPropertyName = "ParentBackupId";

        private const string BackupChainIdPropertyName = "BackupChainId";

        private const string BackupEpochPropertyName = "BackupEpoch";

        private const string BackupLsnPropertyName = "BackupLsn";

        private const string PartitionKindPropertyName = "PartitionKind";

        private const string PartitionKeyPropertyName = "PartitionKey";

        private const string ServiceManifestVersionPropertyName = "ServiceManifestVersion";

        public DateTime BackupTime { get; set; }

        public string ParentBackupLocation { get; set; }

        public string BackupLocation { get; set; }

        public Guid BackupId { get; set; }

        public Guid ParentBackupId { get; set; }

        public Guid BackupChainId { get; set; }

        public Epoch EpochOfLastBackupRecord { get; set; }

        public long LsnOfLastBackupRecord { get; set; }

        public string ServiceManifestVersion { get; set; }

        public ServicePartitionInformation PartitionInformation { get; set; }

        /// <summary>
        /// Serialize the <see cref="RecoveryPointMetadataFileProperties"/> into given stream/>
        /// </summary>
        /// <param name="writer">Stream to write to</param>
        public override void Write(BinaryWriter writer)
        {
            // Allow base class to write first
            base.Write(writer);

            // 'BackupTime' - long
            writer.Write(BackupTimePropertyName);
            VarInt.Write(writer, (int) sizeof(long));
            writer.Write(BackupTime.Ticks);

            // 'ParentBackupLocation' - byte[]
            writer.Write(ParentBackupLocationPropertyName);
            WriteStringValue(writer, ParentBackupLocation);

            // 'BackupLocation' - byte[]
            writer.Write(BackupLocationPropertyName);
            WriteStringValue(writer, BackupLocation);

            // 'BackupId' - byte[16]
            writer.Write(BackupIdPropertyName);
            WriteGuidValue(writer, BackupId);

            // 'ParentBackupId' - byte[16]
            writer.Write(ParentBackupIdPropertyName);
            WriteGuidValue(writer, ParentBackupId);

            // 'BackupChainId' - byte[16]
            writer.Write(BackupChainIdPropertyName);
            WriteGuidValue(writer, BackupChainId);

            // 'BackupEpoch' - Epoch
            writer.Write(BackupEpochPropertyName);
            VarInt.Write(writer, (int)(sizeof(long) * 2));
            writer.Write(this.EpochOfLastBackupRecord.DataLossNumber);
            writer.Write(this.EpochOfLastBackupRecord.ConfigurationNumber);

            // 'BackupLsn' - long
            writer.Write(BackupLsnPropertyName);
            VarInt.Write(writer, (int)sizeof(long));
            writer.Write(this.LsnOfLastBackupRecord);

            // 'PartitionInformation' - byte
            writer.Write(PartitionKindPropertyName);
            VarInt.Write(writer, 1);
            writer.Write((byte)PartitionInformation.Kind);

            // 'PartitionKey' - byte[]
            WritePartitionKey(writer);

            // 'ServicePackageCodeVersion' - byte[]
            writer.Write(ServiceManifestVersionPropertyName);
            WriteStringValue(writer, ServiceManifestVersion);

        }

        private void WritePartitionKey(BinaryWriter writer)
        {
            switch (PartitionInformation.Kind)
            {
                case ServicePartitionKind.Int64Range:
                    writer.Write(PartitionKeyPropertyName);
                    var highKey = ((Int64RangePartitionInformation) PartitionInformation).HighKey;
                    var lowKey = ((Int64RangePartitionInformation)PartitionInformation).LowKey;
                    VarInt.Write(writer, (int)(sizeof(long) * 2));
                    writer.Write(lowKey);
                    writer.Write(highKey);
                    break;
                case ServicePartitionKind.Named:
                    writer.Write(PartitionKeyPropertyName);
                    WriteStringValue(writer, ((NamedPartitionInformation) PartitionInformation).Name);
                    break;
            }
        }

        private void WriteStringValue(BinaryWriter writer, string value)
        {
            if (String.IsNullOrEmpty(value))
            {
                VarInt.Write(writer, 0);
                return;
            }

            var valueInBytes = Encoding.UTF8.GetBytes(value);
            VarInt.Write(writer, valueInBytes.Length);
            writer.Write(valueInBytes);
        }

        private void WriteGuidValue(BinaryWriter writer, Guid value)
        {
            var valueInBytes = value.ToByteArray();
            VarInt.Write(writer, valueInBytes.Length);
            writer.Write(valueInBytes);
        }

        private void ReadPartitionKind(BinaryReader reader, int valueSize)
        {
            var partitionKind = (ServicePartitionKind)reader.ReadByte();
            switch (partitionKind)
            {
                case ServicePartitionKind.Int64Range:
                    this.PartitionInformation = new Int64RangePartitionInformation();
                    break;
                case ServicePartitionKind.Named:
                    this.PartitionInformation = new NamedPartitionInformation();
                    break;
                case ServicePartitionKind.Singleton:
                    this.PartitionInformation = new SingletonPartitionInformation();
                    break;
            }
        }

        private void ReadPartitionKey(BinaryReader reader, int valueSize)
        {
            switch (PartitionInformation.Kind)
            {
                case ServicePartitionKind.Int64Range:
                    (PartitionInformation as Int64RangePartitionInformation).LowKey = reader.ReadInt64();
                    (PartitionInformation as Int64RangePartitionInformation).HighKey = reader.ReadInt64();
                    break;
                case ServicePartitionKind.Named:
                    (PartitionInformation as NamedPartitionInformation).Name = Encoding.UTF8.GetString(reader.ReadBytes(valueSize));
                    break;
            }
        }

        /// <summary>
        /// Reads the given property value
        /// </summary>
        /// <param name="reader">Stream to read from</param>
        /// <param name="property">Property to read</param>
        /// <param name="valueSize">The size in bytes of the value</param>
        protected override void ReadProperty(BinaryReader reader, string property, int valueSize)
        {
            switch (property)
            {
                case BackupLocationPropertyName:
                    this.BackupLocation = Encoding.UTF8.GetString(reader.ReadBytes(valueSize));
                    break;
                case ParentBackupLocationPropertyName:
                    if (valueSize > 0)
                    {
                        this.ParentBackupLocation = Encoding.UTF8.GetString(reader.ReadBytes(valueSize));
                    }
                    break;
                case BackupTimePropertyName:
                    this.BackupTime = new DateTime(reader.ReadInt64(), DateTimeKind.Utc);
                    break;
                case BackupIdPropertyName:
                    this.BackupId = new Guid(reader.ReadBytes(valueSize));
                    break;
                case ParentBackupIdPropertyName:
                    this.ParentBackupId = new Guid(reader.ReadBytes(valueSize));
                    break;
                case BackupChainIdPropertyName:
                    this.BackupChainId = new Guid(reader.ReadBytes(valueSize));
                    break;
                case BackupEpochPropertyName:
                    var backupDataLossNumber = reader.ReadInt64();
                    var backupConfigurationNumber = reader.ReadInt64();
                    this.EpochOfLastBackupRecord = new Epoch(backupDataLossNumber, backupConfigurationNumber);
                    break;
                case BackupLsnPropertyName:
                    this.LsnOfLastBackupRecord = reader.ReadInt64();
                    break;
                case PartitionKindPropertyName:
                    ReadPartitionKind(reader, valueSize);
                    break;
                case PartitionKeyPropertyName:
                    ReadPartitionKey(reader, valueSize);
                    break;
                case ServiceManifestVersionPropertyName:
                    this.ServiceManifestVersion = Encoding.UTF8.GetString(reader.ReadBytes(valueSize));
                    break;
                default:
                    base.ReadProperty(reader, property, valueSize);
                    break;
            }
        }
    }
}