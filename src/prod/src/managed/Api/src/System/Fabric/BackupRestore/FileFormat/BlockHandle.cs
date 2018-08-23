// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.FileFormat
{
    using System.IO;

    /// <summary>
    /// A handle (offset and size) to a block-based section within a stream or file.
    /// </summary>
    internal struct BlockHandle
    {
        /// <summary>
        /// Gets the offset of this block within the stream.
        /// </summary>
        public readonly long Offset;

        /// <summary>
        /// Gets the size of this block within the stream.
        /// </summary>
        public readonly long Size;

        /// <summary>
        /// Create a <see cref="BlockHandle"/> with the given offset and size.
        /// </summary>
        /// <param name="offset">Offset of the block within the stream.</param>
        /// <param name="size">Size of the block within the stream.</param>
        public BlockHandle(long offset, long size)
        {
            this.Offset = offset;
            this.Size = size;
        }

        /// <summary>
        /// Gets the serialized size of a <see cref="BlockHandle"/>.
        /// </summary>
        public static int SerializedSize
        {
            get { return sizeof(long) + sizeof(long); }
        }

        /// <summary>
        /// Gets the end offset of this block within the stream.
        /// </summary>
        public long EndOffset
        {
            get { return this.Offset + this.Size; }
        }

        /// <summary>
        /// Deserialize a <see cref="BlockHandle"/> from the given stream.
        /// </summary>
        /// <param name="reader">Stream to read from.</param>
        /// <returns>The deserialized <see cref="BlockHandle"/>.</returns>
        public static BlockHandle Read(BinaryReader reader)
        {
            var offset = reader.ReadInt64();
            var size = reader.ReadInt64();

            return new BlockHandle(offset, size);
        }

        /// <summary>
        /// Serialize a <see cref="BlockHandle"/> into the given stream.
        /// </summary>
        /// <param name="writer">Stream to write to.</param>
        public void Write(BinaryWriter writer)
        {
            writer.Write(this.Offset);
            writer.Write(this.Size);
        }
    }
}