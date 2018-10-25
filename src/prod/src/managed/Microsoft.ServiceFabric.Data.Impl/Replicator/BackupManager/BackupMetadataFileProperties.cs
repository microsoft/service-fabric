// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Fabric;
    using System.IO;
    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// Properties are key-value pairs.
    /// </summary>
    internal class BackupMetadataFileProperties : FileProperties
    {
        private const string BackupEpochPropertyName = "epoch";

        private const string BackupIdPropertyName = "backupid";

        private const string BackupLsnPropertyName = "lsn";

        private const string BackupOptionPropertyName = "option";

        private const string ParentBackupIdPropertyName = "parentBackupId";

        private const string PartitionIdPropertyName = "partitionid";

        private const string ReplicaIdPropertyName = "replicaid";

        private const string StartingEpochPropertyName = "startingEpoch";

        private const string StartLsnPropertyName = "startingLsn";

        /// <summary>
        /// The epoch at the time the backup was taken.
        /// </summary>
        private Epoch backupEpoch;

        /// <summary>
        /// The backup id.
        /// </summary>
        private Guid backupId;

        /// <summary>
        /// The logical sequence number at the time the backup was taken.
        /// </summary>
        private long backupLsn;

        /// <summary>
        /// The backup option.
        /// </summary>
        private BackupOption backupOption;

        /// <summary>
        /// Parent backup id.
        /// </summary>
        private Guid parentBackupId;

        /// <summary>
        /// The partition id for the backup.
        /// </summary>
        private Guid partitionId;

        /// <summary>
        /// The replica id for the backup.
        /// </summary>
        private long replicaId;

        /// <summary>
        /// The epoch at the time the backup was taken.
        /// </summary>
        private Epoch startingEpoch;

        /// <summary>
        /// The logical sequence number at the time the backup was taken.
        /// </summary>
        private long startingLsn;

        public Epoch BackupEpoch
        {
            get { return this.backupEpoch; }

            set { this.backupEpoch = value; }
        }

        public Guid BackupId
        {
            get { return this.backupId; }

            set { this.backupId = value; }
        }

        public long BackupLsn
        {
            get { return this.backupLsn; }

            set { this.backupLsn = value; }
        }

        public BackupOption BackupOption
        {
            get { return this.backupOption; }

            set { this.backupOption = value; }
        }

        public Guid ParentBackupId
        {
            get { return this.parentBackupId; }

            set { this.parentBackupId = value; }
        }

        public Guid PartitionId
        {
            get { return this.partitionId; }

            set { this.partitionId = value; }
        }

        public long ReplicaId
        {
            get { return this.replicaId; }

            set { this.replicaId = value; }
        }

        public Epoch StartingEpoch
        {
            get { return this.startingEpoch; }

            set { this.startingEpoch = value; }
        }

        public long StartingLsn
        {
            get { return this.startingLsn; }

            set { this.startingLsn = value; }
        }

        /// <summary>
        /// Serialize the <see cref="BackupMetadataFileProperties"/> into the given stream.
        /// </summary>
        /// <param name="writer">Stream to write to.</param>
        public override void Write(BinaryWriter writer)
        {
            // Allow the base class to write first.
            base.Write(writer);

            // 'option' - uint32
            writer.Write(BackupOptionPropertyName);
            VarInt.Write(writer, (int) sizeof(uint));
            writer.Write((uint) this.BackupOption);

            // 'parentbackupid' - Guid
            writer.Write(ParentBackupIdPropertyName);
            var parentBackupIdBytes = this.ParentBackupId.ToByteArray();
            VarInt.Write(writer, (int) parentBackupIdBytes.Length);
            writer.Write(parentBackupIdBytes);

            // 'backupid' - Guid
            writer.Write(BackupIdPropertyName);
            var backupIdBytes = this.BackupId.ToByteArray();
            VarInt.Write(writer, (int) backupIdBytes.Length);
            writer.Write(backupIdBytes);

            // 'partitionid' - Guid
            writer.Write(PartitionIdPropertyName);
            var partitionIdBytes = this.PartitionId.ToByteArray();
            VarInt.Write(writer, (int) partitionIdBytes.Length);
            writer.Write(partitionIdBytes);

            // 'replicaid' - long
            writer.Write(ReplicaIdPropertyName);
            VarInt.Write(writer, (int) sizeof(long));
            writer.Write(this.ReplicaId);

            // 'epoch' - Epoch
            writer.Write(StartingEpochPropertyName);
            VarInt.Write(writer, (int) (sizeof(long)*2));
            writer.Write(this.StartingEpoch.DataLossNumber);
            writer.Write(this.StartingEpoch.ConfigurationNumber);

            // 'lsn' - long
            writer.Write(StartLsnPropertyName);
            VarInt.Write(writer, (int) sizeof(long));
            writer.Write(this.StartingLsn);

            // 'epoch' - Epoch
            writer.Write(BackupEpochPropertyName);
            VarInt.Write(writer, (int) (sizeof(long)*2));
            writer.Write(this.BackupEpoch.DataLossNumber);
            writer.Write(this.BackupEpoch.ConfigurationNumber);

            // 'lsn' - long
            writer.Write(BackupLsnPropertyName);
            VarInt.Write(writer, (int) sizeof(long));
            writer.Write(this.BackupLsn);
        }

        /// <summary>
        /// Read the given property value.
        /// </summary>
        /// <param name="reader">Stream to read from.</param>
        /// <param name="property">Property to read.</param>
        /// <param name="valueSize">The size in bytes of the value.</param>
        protected override void ReadProperty(BinaryReader reader, string property, int valueSize)
        {
            switch (property)
            {
                case BackupOptionPropertyName:
                    this.BackupOption = (BackupOption) reader.ReadUInt32();
                    break;

                case ParentBackupIdPropertyName:
                    this.ParentBackupId = new Guid(reader.ReadBytes(valueSize));
                    break;

                case BackupIdPropertyName:
                    this.BackupId = new Guid(reader.ReadBytes(valueSize));
                    break;

                case PartitionIdPropertyName:
                    this.PartitionId = new Guid(reader.ReadBytes(valueSize));
                    break;

                case ReplicaIdPropertyName:
                    this.ReplicaId = reader.ReadInt64();
                    break;

                case StartingEpochPropertyName:
                    var startingDataLossNumber = reader.ReadInt64();
                    var startingConfigurationNumber = reader.ReadInt64();
                    this.StartingEpoch = new Epoch(startingDataLossNumber, startingConfigurationNumber);
                    break;

                case StartLsnPropertyName:
                    this.StartingLsn = reader.ReadInt64();
                    break;

                case BackupEpochPropertyName:
                    var backupDataLossNumber = reader.ReadInt64();
                    var backupConfigurationNumber = reader.ReadInt64();
                    this.BackupEpoch = new Epoch(backupDataLossNumber, backupConfigurationNumber);
                    break;

                case BackupLsnPropertyName:
                    this.BackupLsn = reader.ReadInt64();
                    break;

                default:
                    base.ReadProperty(reader, property, valueSize);
                    break;
            }
        }
    }
}