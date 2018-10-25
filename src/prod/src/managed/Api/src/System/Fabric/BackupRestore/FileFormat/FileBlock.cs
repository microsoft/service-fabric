// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.FileFormat
{
    using System;
    using System.Globalization;
    using System.IO;
    using System.Threading.Tasks;
    using System.Fabric.BackupRestore;

    /// <summary>
    /// Represents a checksum block of data within a file.
    /// </summary>
    internal static class FileBlock
    {
        /// <summary>
        /// Default size of the memory stream when writing a block (4 KB).
        /// </summary>
        private const int DefaultMemoryStreamSize = 4*1024;

        /// <summary>
        /// Deserialize a block from the given stream.
        /// </summary>
        /// <param name="stream">Stream to read from.</param>
        /// <param name="blockHandle"></param>
        /// <param name="deserializer"></param>
        /// <returns>The block read from the stream.</returns>
        public static async Task<TBlock> ReadBlockAsync<TBlock>(
            Stream stream,
            BlockHandle blockHandle,
            Func<BinaryReader, BlockHandle, TBlock> deserializer)
        {
            // Data validation.
            if (blockHandle.Offset < 0)
            {
                throw new InvalidDataException(string.Format(CultureInfo.CurrentCulture, SR.Error_FileBlock_Corrupt, SR.Error_Negative_StreamOffset));
            }

            if (blockHandle.Size < 0)
            {
                throw new InvalidDataException(string.Format(CultureInfo.CurrentCulture, SR.Error_FileBlock_Corrupt, SR.Error_Negative_BlockSize));
            }

            if (blockHandle.EndOffset + sizeof(ulong) > stream.Length)
            {
                throw new InvalidDataException(string.Format(CultureInfo.CurrentCulture, SR.Error_FileBlock_Corrupt, SR.Error_Block_Beyond_Stream));
            }

            // Move the stream to the block's position within the stream.
            stream.Position = blockHandle.Offset;
            var blockSize = checked((int) blockHandle.Size);
            var blockSizeWithChecksum = blockSize + sizeof(ulong);

            // Read the bytes for the block, to calculate the checksum.
            var blockBytes = new byte[blockSizeWithChecksum];
            await stream.ReadAsync(blockBytes, 0, blockBytes.Length).ConfigureAwait(false);

            // Calculate the checksum.
            var blockChecksum = CRC64.ToCRC64(blockBytes, 0, blockSize);

            // Read the actual checksum (checksum is after the block bytes).
            var checksum = BitConverter.ToUInt64(blockBytes, blockSize);

            // Verify the checksum.
            if (checksum != blockChecksum)
            {
                throw new InvalidDataException(string.Format(CultureInfo.CurrentCulture, SR.Error_FileBlock_Corrupt, SR.Error_Checksum_Mismatch));
            }

            // Deserialize the actual file block.
            using (var memoryStream = new MemoryStream(blockBytes, 0, blockSize, writable: false))
            using (var memoryReader = new BinaryReader(memoryStream))
            {
                return deserializer.Invoke(memoryReader, new BlockHandle(0, blockHandle.Size));
            }
        }

        /// <summary>
        /// Serialize the block into the given stream.
        /// </summary>
        /// <param name="stream">Stream to write to.</param>
        /// <param name="serializer"></param>
        public static async Task<BlockHandle> WriteBlockAsync(Stream stream, Action<BinaryWriter> serializer)
        {
            using (var memoryStream = new MemoryStream(DefaultMemoryStreamSize))
            using (var memoryWriter = new BinaryWriter(memoryStream))
            {
                // Serialize the table section.
                serializer.Invoke(memoryWriter);

                var blockSize = checked((int) memoryStream.Position);

                // Calculate the checksum.
                var checksum = CRC64.ToCRC64(memoryStream.GetBuffer(), 0, blockSize);

                // Get the position and size of this section within the stream.
                var sectionHandle = new BlockHandle(stream.Position, blockSize);

                // Write the checksum.
                memoryWriter.Write(checksum);

                var blockSizeWithChecksum = checked((int) memoryStream.Position);
                //Utility.Assert((blockSizeWithChecksum - blockSize) == 8, "unexpected block size.");

                // Write the buffer and checksum to the stream.
                await stream.WriteAsync(memoryStream.GetBuffer(), 0, blockSizeWithChecksum).ConfigureAwait(false);

                return sectionHandle;
            }
        }
    }
}