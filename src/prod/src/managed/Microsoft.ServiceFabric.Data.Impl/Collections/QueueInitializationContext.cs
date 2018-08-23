// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections
{
    using System;
    using System.Globalization;
    using System.IO;
    using Microsoft.ServiceFabric.Replicator;

    /// <summary>
    /// Initialization context for IReliableQueue (InternalDistributedQueue).
    /// </summary>
    internal class QueueInitializationContext : FileProperties
    {
        /// <summary>
        /// Current serialization version.
        /// </summary>
        private const int SerializationVersion = 1;

        /// <summary>
        /// Estimated size (to minimize MemoryStream reallocations).
        /// </summary>
        private const int EstimatedSize = 128;

        /// <summary>
        /// Gets the serialization version number.
        /// </summary>
        public int Version { get; private set; }

        /// <summary>
        /// Gets the value indicating whether automatic deep copy of every returned value is enabled.
        /// </summary>
        public bool IsAutomaticCloningEnabled { get; private set; }

        private const string IsAutomaticCloningEnabledPropertyName = "clone";

        /// <summary>
        /// Creates a new instance of the <see cref="QueueInitializationContext"/> class.
        /// </summary>
        public QueueInitializationContext()
        {
        }

        /// <summary>
        /// Creates a new instance of the <see cref="QueueInitializationContext"/> class.
        /// </summary>
        /// <param name="isAutomaticCloningEnabled">Is deep copy enabled.</param>
        public QueueInitializationContext(bool isAutomaticCloningEnabled)
        {
            this.Version = SerializationVersion;
            this.IsAutomaticCloningEnabled = isAutomaticCloningEnabled;
        }

        /// <summary>
        /// Serialize the <see cref="QueueInitializationContext"/> into the given stream.
        /// </summary>
        /// <param name="writer">Stream to write to.</param>
        public override void Write(BinaryWriter writer)
        {
            // Allow the base class to write first.
            base.Write(writer);

            // 'clone' - bool
            writer.Write(IsAutomaticCloningEnabledPropertyName);
            VarInt.Write(writer, (int) sizeof(bool));
            writer.Write(this.IsAutomaticCloningEnabled);
        }

        /// <summary>
        /// Read the given property value.
        /// </summary>
        /// <param name="reader">Stream to read from.</param>
        /// <param name="property">Property to read.</param>
        /// <param name="valueSize">The size in bytes of the value.</param>
        protected override void ReadProperty(BinaryReader reader, string property, int valueSize)
        {
            switch (property)
            {
                case IsAutomaticCloningEnabledPropertyName:
                    this.IsAutomaticCloningEnabled = reader.ReadBoolean();
                    break;

                default:
                    base.ReadProperty(reader, property, valueSize);
                    break;
            }
        }

        /// <summary>
        /// Convert to initialization context.
        /// </summary>
        /// <param name="bytes">Serialized bytes.</param>
        /// <returns>Deserialized initialization context.</returns>
        public static QueueInitializationContext FromByteArray(byte[] bytes)
        {
            if (bytes == null)
            {
                throw new ArgumentNullException(string.Format(CultureInfo.CurrentCulture, SR.Error_NullArgument_Formatted, "bytes"));
            }

            using (var stream = new MemoryStream(bytes))
            using (var reader = new BinaryReader(stream))
            {
                // Read and check the version first.  This is for future proofing - if the format dramatically changes,
                // we can change the serialization version and modify the serialization/deserialization code.
                // For now, we verify the version matches exactly the expected version.
                var version = reader.ReadInt32();
                if (version != SerializationVersion)
                {
                    throw new InvalidDataException(
                        string.Format(
                            System.Globalization.CultureInfo.CurrentCulture,
                            SR.Error_QueueInitializationContext_Deserialized,
                            version,
                            SerializationVersion));
                }

                // Currently, the rest of the properties are serialized as [string, value] pairs (see ReadProperty and Write methods).
                var handle = new BlockHandle(offset: sizeof(int), size: bytes.Length - sizeof(int));
                var initializationContext = Read<QueueInitializationContext>(reader, handle);

                initializationContext.Version = version;
                return initializationContext;
            }
        }

        /// <summary>
        /// Convert to byte array.
        /// </summary>
        /// <returns>Serialized bytes.</returns>
        public byte[] ToByteArray()
        {
            using (var stream = new MemoryStream(EstimatedSize))
            using (var writer = new BinaryWriter(stream))
            {
                // Always write the version first, so we can change the serialization code if necessary.
                writer.Write(this.Version);

                // Write the remainder of the properties as [string, value] pairs.
                this.Write(writer);

                return stream.ToArray();
            }
        }
    }
}