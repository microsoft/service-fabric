// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.IO;
    using Microsoft.ServiceFabric.Replicator;

    /// <summary>
    /// Properties for the MetadataManager.
    /// </summary>
    internal sealed class MetadataManagerFileProperties : FilePropertySection
    {
        internal static bool Test_IgnoreCheckpointLSN = false;

        /// <summary>
        /// If it does not exist, it is set to invalid.
        /// </summary>
        private long checkpointLSN = DifferentialStoreConstants.InvalidLSN;

        private enum PropertyId : int
        {
            MetadataHandle = 1,
            FileCount = 2,
            CheckpointLSN = 3,
        }

        public BlockHandle MetadataHandle { get; set; }

        public int FileCount { get; set; }

        public long CheckpointLSN
        {
            get
            {
                return this.checkpointLSN;
            }
            set
            {
                this.checkpointLSN = value;
            }
        }

        /// <summary>
        /// Serialize the <see cref="MetadataManagerFileProperties"/> into the given stream.
        /// </summary>
        /// <param name="writer">The stream to write to.</param>
        public override void Write(BinaryWriter writer)
        {
            // 'MetadataHandle' - BlockHandle
            writer.Write((int) PropertyId.MetadataHandle);
            VarInt.Write(writer, (int) BlockHandle.SerializedSize);
            this.MetadataHandle.Write(writer);

            // 'FileCount' - int
            writer.Write((int) PropertyId.FileCount);
            VarInt.Write(writer, (int) sizeof(int));
            writer.Write((int) this.FileCount);

            if (Test_IgnoreCheckpointLSN)
            {
                return;
            }

            // 'CheckpointLSN' - long
            writer.Write((int)PropertyId.CheckpointLSN);
            VarInt.Write(writer, (int)sizeof(long));
            writer.Write(this.checkpointLSN);
        }

        /// <summary>
        /// Read the given property value.
        /// </summary>
        /// <param name="reader">Stream to read from.</param>
        /// <param name="property">Property to read.</param>
        /// <param name="valueSize">The size in bytes of the value.</param>
        protected override void ReadProperty(BinaryReader reader, int property, int valueSize)
        {
            switch ((PropertyId) property)
            {
                case PropertyId.MetadataHandle:
                    this.MetadataHandle = BlockHandle.Read(reader);
                    break;

                case PropertyId.FileCount:
                    this.FileCount = reader.ReadInt32();
                    break;

                case PropertyId.CheckpointLSN:
                    this.checkpointLSN = reader.ReadInt64();
                    break;

                default:
                    base.ReadProperty(reader, property, valueSize);
                    break;
            }
        }
    }
}