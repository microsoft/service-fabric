// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System.IO;

    /// <summary>
    /// Supports serializing and de-serializing variable-sized integers.
    /// 
    /// Small integers (0-127) will be encoded in one byte.
    /// Large integers and negative integers will be encoded in 5-bytes.
    /// 
    /// The encoding is done in the following way:
    /// - The int32 data is written in 7-bit chunks with 1-bit (MSB) indicating whether more data exists.
    /// - The int32 data is written from LSB to MSB.
    ///
    /// For example, 0x0406 would be written as:
    /// 
    /// Raw bits:      [0000 0100 0000 0110]
    /// Encoding:      1 | [000 0110] + 0 | [000 1000]
    /// Encoded bits:  [1000 0110] [0000 1000] => 0x8608
    /// 
    /// To read the data back, bytes are read until the MSB 1-bit is not set.
    /// The 7-bit chunks are shifted into final place as they are read.
    /// 
    /// <![CDATA[
    /// Encoded bits:       [1000 0110] [0000 1000]
    /// Read one byte:      [1000 0110] =>               (0x06 << 0)
    /// Read another byte:  [0000 1000] => (0x80 << 7) | (0x06 << 0)
    /// Decoded bits:       [0000 0100] [0000 0110] => 0x0406
    /// ]]>
    /// </summary>
    internal static class VarInt
    {
        private const byte ByteMask = 0x7F;

        private const uint MinFiveByteValue = 0x01 << 28;

        private const uint MinFourByteValue = 0x01 << 21;

        private const uint MinThreeByteValue = 0x01 << 14;

        private const uint MinTwoByteValue = 0x01 << 7;

        private const byte MoreBytesMask = 0x80;

        /// <summary>
        /// Computes the serialized size of the given integer value in the varint format.
        /// </summary>
        /// <param name="value">The value to be serialized.</param>
        /// <returns>The serialized size.</returns>
        public static int GetSerializedSize(int value)
        {
            return GetSerializedSize(unchecked((uint) value));
        }

        /// <summary>
        /// Computes the serialized size of the given integer value in the varint format.
        /// </summary>
        /// <param name="value">The value to be serialized.</param>
        /// <returns>The serialized size.</returns>
        public static int GetSerializedSize(uint value)
        {
            if (value < MinTwoByteValue)
            {
                return 1;
            }
            else if (value < MinThreeByteValue)
            {
                return 2;
            }
            else if (value < MinFourByteValue)
            {
                return 3;
            }
            else if (value < MinFiveByteValue)
            {
                return 4;
            }

            return 5;
        }

        /// <summary>
        /// Deserialize the integer value from the varint32 format.
        /// </summary>
        /// <param name="reader">Stream to read from.</param>
        /// <returns>Deserialized integer.</returns>
        public static int ReadInt32(BinaryReader reader)
        {
            return unchecked((int) ReadUInt32(reader));
        }

        /// <summary>
        /// Deserialize the integer value from the varint32 format.
        /// </summary>
        /// <param name="reader">Stream to read from.</param>
        /// <returns>Deserialized integer.</returns>
        public static uint ReadUInt32(BinaryReader reader)
        {
            uint value = 0;

            // The maximum number of chunks to read.
            const int Chunks = 5; // Math.Ceiling(sizeof(uint) * 8.0 / 7.0);
            for (var c = 0; c < Chunks; c++)
            {
                // Read one byte.
                var b = reader.ReadByte();

                // The bytes are serialized from the low-order bits to high-order bits in 7-bit chunks.
                var chunk = unchecked((uint) (b & ByteMask) << (7*c));

                // Add in the new 7-bit chunk in the correct final place.
                value |= chunk;

                // Check if there are more 7-bit chunks.
                if ((b & MoreBytesMask) == 0)
                {
                    break;
                }
            }

            return value;
        }

        /// <summary>
        /// Serialize the integer value in the varint32 format.
        /// </summary>
        /// <param name="writer">Stream to write to.</param>
        /// <param name="value">Value to serialize.</param>
        public static void Write(BinaryWriter writer, int value)
        {
            Write(writer, unchecked((uint) value));
        }

        /// <summary>
        /// Serialize the integer value in the varint32 format.
        /// </summary>
        /// <param name="writer">Stream to write to.</param>
        /// <param name="value">Value to serialize.</param>
        public static void Write(BinaryWriter writer, uint value)
        {
            var bytes = value;

            // The maximum number of chunks to write.
            const int Chunks = 5; // Math.Ceiling(sizeof(uint) * 8.0 / 7.0);
            for (var c = 0; c < Chunks; c++)
            {
                // Serialize in 7-bit chunks from the low-order bits to the high-order bits.
                var b = (byte) (bytes & ByteMask);

                // Check if there are more byte chunks to serialize.
                if (bytes < MoreBytesMask)
                {
                    writer.Write(b);
                    break;
                }
                else
                {
                    // Write the 7-bit chunk with the MSB 1-bit set to indicate more bytes will be written.
                    writer.Write((byte) (MoreBytesMask | b));

                    // Shift the value down by 7-bits to prepare for writing the next 7-bit chunk.
                    bytes >>= 7;
                }
            }
        }
    }
}