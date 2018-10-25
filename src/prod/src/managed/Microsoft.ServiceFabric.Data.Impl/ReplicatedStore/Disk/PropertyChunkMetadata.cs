// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using Microsoft.ServiceFabric.Replicator;

    /// <summary>
    /// Metadata about the property block written at the begining of the block.
    /// Can be considered as a header.
    /// </summary>
    /// <devnote>
    /// MCoskun: Note that KeyBlockMetadata.BlockSize includes KeyBlockMetadata and KeyBlock.
    /// It does not include the checksum
    /// </devnote>
    /// <devnote>
    /// MCoskun: Note that checksum includes the KeyBlockMetadata and KeyBlock.
    /// </devnote>
    /// <devnote>
    /// TODO: We could use GCHandle and StructureToPtr to do copy free serialization and de-serialization.
    /// </devnote>
    internal struct PropertyChunkMetadata
    {
        /// <summary>
        /// int for size field and 4 bytes of reserved space.
        /// </summary>
        public const int Size = sizeof(int) + 4;

        public readonly int BlockSize;

        public PropertyChunkMetadata(int blockSize)
        {
            this.BlockSize = blockSize;
        }

        public static PropertyChunkMetadata Read(InMemoryBinaryReader reader)
        {
            reader.ThrowIfNotAligned();

            var currentBlockSize = reader.ReadInt32();
            reader.ReadPaddingUntilAligned(true);

            reader.ThrowIfNotAligned();

            return new PropertyChunkMetadata(currentBlockSize);
        }

        public void Write(InMemoryBinaryWriter writer)
        {
            writer.AssertIfNotAligned();

            writer.Write(this.BlockSize);
            writer.WritePaddingUntilAligned();

            writer.AssertIfNotAligned();
        }
    }
}