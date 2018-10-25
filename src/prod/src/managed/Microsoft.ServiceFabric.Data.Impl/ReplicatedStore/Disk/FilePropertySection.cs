// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Globalization;
    using System.IO;
    using Microsoft.ServiceFabric.Replicator;
    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// Properties are key-value pairs.
    /// </summary>
    internal abstract class FilePropertySection
    {
        /// <summary>
        /// Serialize the <see cref="FilePropertySection"/> into the given stream.
        /// 
        /// The format must be written as: (property id, value length, value bytes)
        /// - (int, varint, byte[]), (int, varint, byte[]), ...
        /// </summary>
        /// <param name="writer">The stream to write to.</param>
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
        protected virtual void ReadProperty(BinaryReader reader, int property, int valueSize)
        {
            // Unknown property - skip over the value.
            reader.BaseStream.Position += valueSize;
        }

        /// <summary>
        /// Deserialize the <see cref="FilePropertySection"/> from the given stream.
        /// </summary>
        /// <param name="reader">The stream to read from.</param>
        /// <param name="handle">Starting offset and size within the stream for the file properties.</param>
        /// <returns>The deserialized <see cref="FilePropertySection"/> section.</returns>
        public static TProperties Read<TProperties>(BinaryReader reader, BlockHandle handle) where TProperties : FilePropertySection, new()
        {
            var properties = new TProperties();
            if (handle.Offset < 0)
            {
                throw new InvalidDataException(SR.Error_FilePropertySection_Missing);
            }
            if (handle.Size == 0)
            {
                return properties;
            }

            // The FileProperties start at the given 'offset' within the stream, and contains properties up to 'offset' + 'size'.
            reader.BaseStream.Position = handle.Offset;
            while (reader.BaseStream.Position < handle.EndOffset)
            {
                // Read the property name.
                var property = reader.ReadInt32();

                // Read the size in bytes of the property's value.
                var valueSize = VarInt.ReadInt32(reader);
                if (valueSize < 0)
                {
                    throw new InvalidDataException(string.Format(CultureInfo.CurrentCulture, SR.Error_FilePropertySection_CorruptSizeNegative_OneArgs, valueSize));
                }
                if (reader.BaseStream.Position + valueSize > handle.EndOffset)
                {
                    throw new InvalidDataException(SR.Error_FilePropertySection_CorruptSizeExtend);
                }

                // Read the property's value.
                properties.ReadProperty(reader, property, valueSize);
            }

            // Validate the properties section ended exactly where expected.
            if (reader.BaseStream.Position != handle.EndOffset)
            {
                throw new InvalidDataException(SR.Error_FilePropertySection_CorruptSizeIncorrect);
            }

            return properties;
        }
    }
}