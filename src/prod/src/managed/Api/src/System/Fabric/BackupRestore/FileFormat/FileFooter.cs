// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.FileFormat
{
    using System.IO;

    /// <summary>
    /// Footer for a file.
    /// </summary>
    internal sealed class FileFooter
    {
        /// <summary>
        /// Create a new <see cref="FileFooter"/>.
        /// </summary>
        /// <param name="propertiesHandle">Stream offset and size to the Properties section.</param>
        /// <param name="version">Serialization version number for the file.</param>
        public FileFooter(BlockHandle propertiesHandle, int version)
        {
            this.PropertiesHandle = propertiesHandle;
            this.Version = version;
        }

        /// <summary>
        /// Gets the serialized size of the <see cref="FileFooter"/>.
        /// </summary>
        public static int SerializedSize
        {
            get { return BlockHandle.SerializedSize + sizeof(int); }
        }

        /// <summary>
        /// Gets the block handle for the Properties section of the file.
        /// </summary>
        public BlockHandle PropertiesHandle { get; private set; }

        /// <summary>
        /// Gets the serialization version number for the file.
        /// </summary>
        public int Version { get; private set; }

        /// <summary>
        /// Deserialize a <see cref="FileFooter"/> from the given stream.
        /// </summary>
        /// <param name="reader">Stream to read from.</param>
        /// <param name="handle">Starting offset and size within the stream for the file footer.</param>
        /// <returns>The <see cref="FileFooter"/> read from the stream.</returns>
        public static FileFooter Read(BinaryReader reader, BlockHandle handle)
        {
            var propertiesHandle = BlockHandle.Read(reader);
            var version = reader.ReadInt32();

            return new FileFooter(propertiesHandle, version);
        }

        /// <summary>
        /// Serialize the <see cref="FileFooter"/> into the given stream.
        /// </summary>
        /// <param name="writer">Stream to write to.</param>
        public void Write(BinaryWriter writer)
        {
            this.PropertiesHandle.Write(writer);
            writer.Write(this.Version);
        }
    }
}