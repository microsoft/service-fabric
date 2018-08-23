// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.IO;
    using Microsoft.ServiceFabric.Replicator;

    /// <summary>
    /// Properties for the ValueCheckpointFile.
    /// </summary>
    internal sealed class ValueCheckpointFileProperties : FilePropertySection
    {
        private enum PropertyId : int
        {
            ValuesHandle = 1,
            ValueCount = 2,
            FileId = 3,
        }

        public BlockHandle ValuesHandle { get; set; }

        public long ValueCount { get; set; }

        public uint FileId { get; set; }

        /// <summary>
        /// Serialize the <see cref="ValueCheckpointFileProperties"/> into the given stream.
        /// </summary>
        /// <param name="writer">The stream to write to.</param>
        /// <remarks>
        /// The data is written is 8 bytes aligned.
        /// Name                Type        Size
        /// 
        /// ValuesHandle.PID    int         4
        /// SerializedSize      VarInt      1
        /// RESERVED                        3
        /// KeyHandle           bytes       16
        /// 
        /// ValueCount.PID      int         4
        /// SerializedSize      VarInt      1
        /// RESERVED                        3
        /// ValueCount          long        8
        /// 
        /// FileId.PID          int         4
        /// Size                VarInt      1
        /// RESERVED                        3
        /// FileId              bytes       4
        /// RESERVED                        4
        /// 
        /// RESERVED: Fixed padding that is usable to add fields in future.
        /// PADDING:  Due to dynamic size, cannot be used for adding fields.
        /// </remarks>
        public override void Write(BinaryWriter writer)
        {
            ByteAlignedReadWriteHelper.AssertIfNotAligned(writer.BaseStream);

            // 'ValuesHandle' - BlockHandle
            writer.Write((int) PropertyId.ValuesHandle);
            VarInt.Write(writer, (int) BlockHandle.SerializedSize);
            ByteAlignedReadWriteHelper.WritePaddingUntilAligned(writer);
            this.ValuesHandle.Write(writer);

            // 'ValueCount' - long
            writer.Write((int) PropertyId.ValueCount);
            VarInt.Write(writer, (int) sizeof(long));
            ByteAlignedReadWriteHelper.WritePaddingUntilAligned(writer);
            writer.Write((long) this.ValueCount);

            // 'FileId' - int
            writer.Write((int) PropertyId.FileId);
            VarInt.Write(writer, (int) sizeof(int));
            ByteAlignedReadWriteHelper.WritePaddingUntilAligned(writer);
            writer.Write((int) this.FileId);
            ByteAlignedReadWriteHelper.WritePaddingUntilAligned(writer);

            ByteAlignedReadWriteHelper.AssertIfNotAligned(writer.BaseStream);
        }

        /// <summary>
        /// Read the given property value.
        /// </summary>
        /// <param name="reader">Stream to read from.</param>
        /// <param name="property">Property to read.</param>
        /// <param name="valueSize">The size in bytes of the value.</param>
        protected override void ReadProperty(BinaryReader reader, int property, int valueSize)
        {
            ByteAlignedReadWriteHelper.ReadPaddingUntilAligned(reader, true);
            switch ((PropertyId) property)
            {
                case PropertyId.ValuesHandle:
                    this.ValuesHandle = BlockHandle.Read(reader);
                    break;

                case PropertyId.ValueCount:
                    this.ValueCount = reader.ReadInt64();
                    break;

                case PropertyId.FileId:
                    this.FileId = reader.ReadUInt32();
                    ByteAlignedReadWriteHelper.ReadPaddingUntilAligned(reader, true);
                    break;

                default:
                    base.ReadProperty(reader, property, valueSize);
                    ByteAlignedReadWriteHelper.ReadPaddingUntilAligned(reader, true);
                    break;
            }

            ByteAlignedReadWriteHelper.ReadPaddingUntilAligned(reader, true);
        }
    }
}