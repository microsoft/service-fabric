// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System.Fabric;
    using System.IO;

    /// <summary>
    /// Properties are key-value pairs.
    /// </summary>
    internal class BackupLogFileProperties : FileProperties
    {
        private const string CountPropertyName = "count";

        private const string IndexingRecordEpochPropertyName = "indexepoch";

        private const string IndexingRecordLsnPropertyName = "indexlsn";

        private const string LastBackedUpEpochPropertyName = "backupepoch";

        private const string LastBackedUpLsnPropertyName = "backuplsn";

        private const string RecordsHandlePropertyName = "records";

        /// <summary>
        /// The number of log records.
        /// </summary>
        private uint count = 0;

        /// <summary>
        /// The epoch of the first record in the backup log.
        /// </summary>
        private Epoch indexingRecordEpoch = LogicalSequenceNumber.InvalidEpoch;

        /// <summary>
        /// The logical sequence number of the first record in the backup log.
        /// </summary>
        private LogicalSequenceNumber indexingRecordLsn = LogicalSequenceNumber.InvalidLsn;

        /// <summary>
        /// The Epoch of the last record in the backup log.
        /// </summary>
        private Epoch lastBackedUpEpoch = LogicalSequenceNumber.InvalidEpoch;

        /// <summary>
        /// The logical sequence number of the last record in the backup log.
        /// </summary>
        private LogicalSequenceNumber lastBackedUpLsn = LogicalSequenceNumber.InvalidLsn;

        /// <summary>
        /// The handle to the log records block.
        /// </summary>
        private BlockHandle recordsHandle;

        public uint Count
        {
            get { return this.count; }

            set { this.count = value; }
        }

        public Epoch IndexingRecordEpoch
        {
            get { return this.indexingRecordEpoch; }

            set { this.indexingRecordEpoch = value; }
        }

        public LogicalSequenceNumber IndexingRecordLsn
        {
            get { return this.indexingRecordLsn; }

            set { this.indexingRecordLsn = value; }
        }

        public Epoch LastBackedUpEpoch
        {
            get { return this.lastBackedUpEpoch; }

            set { this.lastBackedUpEpoch = value; }
        }

        public LogicalSequenceNumber LastBackedUpLsn
        {
            get { return this.lastBackedUpLsn; }

            set { this.lastBackedUpLsn = value; }
        }

        public BlockHandle RecordsHandle
        {
            get { return this.recordsHandle; }

            set { this.recordsHandle = value; }
        }

        /// <summary>
        /// Serialize the <see cref="BackupLogFileProperties"/> into the given stream.
        /// </summary>
        /// <param name="writer">Stream to write to.</param>
        public override void Write(BinaryWriter writer)
        {
            // Allow the base class to write first.
            base.Write(writer);

            // 'records' - BlockHandle
            writer.Write(RecordsHandlePropertyName);
            VarInt.Write(writer, (int) BlockHandle.SerializedSize);
            this.RecordsHandle.Write(writer);

            // 'count' - uint
            writer.Write(CountPropertyName);
            VarInt.Write(writer, (int) sizeof(uint));
            writer.Write(this.Count);

            // 'indexepoch' - Epoch
            writer.Write(IndexingRecordEpochPropertyName);
            VarInt.Write(writer, (int) (sizeof(long)*2));
            writer.Write(this.IndexingRecordEpoch.DataLossNumber);
            writer.Write(this.IndexingRecordEpoch.ConfigurationNumber);

            // 'indexlsn' - long
            writer.Write(IndexingRecordLsnPropertyName);
            VarInt.Write(writer, (int) sizeof(long));
            writer.Write(this.IndexingRecordLsn.LSN);

            // 'backupepoch' - Epoch
            writer.Write(LastBackedUpEpochPropertyName);
            VarInt.Write(writer, (int) (sizeof(long)*2));
            writer.Write(this.LastBackedUpEpoch.DataLossNumber);
            writer.Write(this.LastBackedUpEpoch.ConfigurationNumber);

            // 'backuplsn' - long
            writer.Write(LastBackedUpLsnPropertyName);
            VarInt.Write(writer, (int) sizeof(long));
            writer.Write(this.LastBackedUpLsn.LSN);
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
                case RecordsHandlePropertyName:
                    this.RecordsHandle = BlockHandle.Read(reader);
                    break;

                case CountPropertyName:
                    this.Count = reader.ReadUInt32();
                    break;

                case IndexingRecordEpochPropertyName:
                    var indexDataLossNumber = reader.ReadInt64();
                    var indexConfigurationNumber = reader.ReadInt64();
                    this.IndexingRecordEpoch = new Epoch(indexDataLossNumber, indexConfigurationNumber);
                    break;

                case IndexingRecordLsnPropertyName:
                    this.IndexingRecordLsn = new LogicalSequenceNumber(reader.ReadInt64());
                    break;

                case LastBackedUpEpochPropertyName:
                    var backupDataLossNumber = reader.ReadInt64();
                    var backupConfigurationNumber = reader.ReadInt64();
                    this.LastBackedUpEpoch = new Epoch(backupDataLossNumber, backupConfigurationNumber);
                    break;

                case LastBackedUpLsnPropertyName:
                    this.LastBackedUpLsn = new LogicalSequenceNumber(reader.ReadInt64());
                    break;

                default:
                    base.ReadProperty(reader, property, valueSize);
                    break;
            }
        }
    }
}