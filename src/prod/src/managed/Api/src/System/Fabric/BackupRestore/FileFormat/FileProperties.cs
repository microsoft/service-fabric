// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore.FileFormat
{
    using System.IO;

    /// <summary>
    /// Properties are key-value pairs.
    /// </summary>
    internal abstract class FileProperties
    {
        /// <summary>
        /// Deserialize the <see cref="FileProperties"/> from the given stream.
        /// </summary>
        /// <param name="reader">The stream to read from.</param>
        /// <param name="handle">Starting offset and size within the stream for the file properties.</param>
        /// <returns>The deserialized <see cref="FileProperties"/> section.</returns>
        public static TProperties Read<TProperties>(BinaryReader reader, BlockHandle handle)
            where TProperties : FileProperties, new()
        {
            var properties = new TProperties();
            if (handle.Size == 0)
            {
                return properties;
            }

            // The FileProperties start at the given 'offset' within the stream, and contains properties up to 'offset' + 'size'.
            reader.BaseStream.Position = handle.Offset;
            while (reader.BaseStream.Position < handle.EndOffset)
            {
                // Read the property name.
                var property = reader.ReadString();

                // Read the size in bytes of the property's value.
                var valueSize = VarInt.ReadInt32(reader);

                if (valueSize < 0)
                {
                    throw new InvalidDataException(string.Format(SR.Error_FileProperties_Corrupt, SR.Error_FileProperties_NegativeSize));
                }

                if (reader.BaseStream.Position + valueSize > handle.EndOffset)
                {
                    throw new InvalidDataException(
                        string.Format(SR.Error_FileProperties_Corrupt, SR.Error_FileProperties_LargeSize));
                }

                // Read the property's value.
                properties.ReadProperty(reader, property, valueSize);
            }

            // Validate the properties section ended exactly where expected.
            if (reader.BaseStream.Position != handle.EndOffset)
            {
                throw new InvalidDataException(string.Format(SR.Error_FileProperties_Corrupt, SR.Error_FileProperties_SectionSize));
            }

            return properties;
        }

        public virtual void Write(BinaryWriter writer)
        {
            // Base class has no properties to write
        }

        /// <summary>
        /// Read the given property value.
        /// </summary>
        /// <param name="reader">Stream to read from.</param>
        /// <param name="property">Property to read.</param>
        /// <param name="valueSize">The size in bytes of the value.</param>
        protected virtual void ReadProperty(BinaryReader reader, string property, int valueSize)
        {
            // Unknown property - skip over the value.
            reader.BaseStream.Position += valueSize;
        }
    }
}