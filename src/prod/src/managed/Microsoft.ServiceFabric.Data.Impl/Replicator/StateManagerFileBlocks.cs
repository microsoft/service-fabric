// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System.IO;
    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// Stores the sizes of blocks of state provider metadata records, to improve sequential reading.
    /// </summary>
    internal sealed class StateManagerFileBlocks
    {
        /// <summary>
        /// Creates a <see cref="StateManagerFileBlocks"/> with the given block sizes.
        /// </summary>
        /// <param name="recordBlockSizes">In-order sizes to blocks of records.</param>
        public StateManagerFileBlocks(int[] recordBlockSizes)
        {
            this.RecordBlockSizes = recordBlockSizes;
        }

        private StateManagerFileBlocks()
        {
            // Reserved for static StateManagerFileBlocks.Read()
        }

        /// <summary>
        /// Gets the sizes of blocks of sorted table records.
        /// </summary>
        public int[] RecordBlockSizes { get; private set; }

        /// <summary>
        /// Deserialize a <see cref="StateManagerFileBlocks"/> from the given stream.
        /// </summary>
        /// <param name="reader">Stream to deserialize from.</param>
        /// <param name="handle">Starting offset and size within the stream for the table index.</param>
        /// <returns>The deserialized <see cref="StateManagerFileBlocks"/>.</returns>
        public static StateManagerFileBlocks Read(BinaryReader reader, BlockHandle handle)
        {
            if (handle.Size == 0)
            {
                return null;
            }

            var blocks = new StateManagerFileBlocks();
            reader.BaseStream.Position = handle.Offset;

            // Read the record block sizes.
            blocks.RecordBlockSizes = ReadArray(reader);

            // Validate the section was read correctly.
            if (reader.BaseStream.Position != handle.EndOffset)
            {
                throw new InvalidDataException(SR.Error_SMBlocksCorrupt);
            }

            return blocks;
        }

        /// <summary>
        /// Serialize a <see cref="StateManagerFileBlocks"/> into the given stream.
        /// </summary>
        /// <param name="writer">Stream to serialize into.</param>
        public void Write(BinaryWriter writer)
        {
            // Write the record block sizes.
            WriteArray(writer, this.RecordBlockSizes);
        }

        /// <summary>
        /// Deserialize the compressed prefix hash table or binary search table.
        /// </summary>
        /// <param name="reader">Stream to read from.</param>
        /// <returns>The deserialized compressed table.</returns>
        private static int[] ReadArray(BinaryReader reader)
        {
            var length = reader.ReadInt32();
            if (length < 0)
            {
                return null;
            }

            var array = new int[length];
            for (var i = 0; i < length; i++)
            {
                array[i] = reader.ReadInt32();
            }

            return array;
        }

        /// <summary>
        /// Serialize the record block sizes.
        /// </summary>
        /// <param name="writer">Stream to write to.</param>
        /// <param name="array">Integer array to serialize.</param>
        private static void WriteArray(BinaryWriter writer, int[] array)
        {
            if (array == null)
            {
                writer.Write((int) -1);
            }
            else
            {
                writer.Write((int) array.Length);
                foreach (var entry in array)
                {
                    writer.Write(entry);
                }
            }
        }
    }
}