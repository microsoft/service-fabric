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
    /// Initialization parameters for TStore.
    /// </summary>
    internal class StoreInitializationParameters : FileProperties
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
        /// Gets the serialization version.
        /// </summary>
        public int Version { get; private set; }

        /// <summary>
        /// Gets the store behavior.
        /// </summary>
        public StoreBehavior StoreBehavior { get; private set; }

        private const string StoreBehaviorPropertyName = "behavior";

        /// <summary>
        /// Gets a value indicating whether the store can be read from on secondaries.
        /// </summary>
        public bool AllowReadableSecondary { get; private set; }

        private const string AllowReadableSecondaryPropertyName = "readablesecondary";

        /// <summary>
        /// Creates a new instance of the <see cref="StoreInitializationParameters"/> class.
        /// </summary>
        public StoreInitializationParameters()
        {
        }

        /// <summary>
        /// Creates a new instance of the <see cref="StoreInitializationParameters"/> class.
        /// </summary>
        /// <param name="storeBehavior">Store behavior.</param>
        /// <param name="allowReadableSecondary">Allow reads on secondaries.</param>
        public StoreInitializationParameters(StoreBehavior storeBehavior, bool allowReadableSecondary)
        {
            this.Version = SerializationVersion;
            this.StoreBehavior = storeBehavior;
            this.AllowReadableSecondary = allowReadableSecondary;
        }

        /// <summary>
        /// Serialize the <see cref="StoreInitializationParameters"/> into the given stream.
        /// </summary>
        /// <param name="writer">Stream to write to.</param>
        public override void Write(BinaryWriter writer)
        {
            // Allow the base class to write first.
            base.Write(writer);

            // 'behavior' - byte
            writer.Write(StoreBehaviorPropertyName);
            VarInt.Write(writer, (int) sizeof(byte));
            writer.Write((byte) this.StoreBehavior);

            // 'readablesecondary' - bool
            writer.Write(AllowReadableSecondaryPropertyName);
            VarInt.Write(writer, (int) sizeof(bool));
            writer.Write(this.AllowReadableSecondary);
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
                case StoreBehaviorPropertyName:
                    this.StoreBehavior = (StoreBehavior) reader.ReadByte();
                    break;

                case AllowReadableSecondaryPropertyName:
                    this.AllowReadableSecondary = reader.ReadBoolean();
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
        public static StoreInitializationParameters FromByteArray(byte[] bytes)
        {
            if (bytes == null)
            {
                throw new ArgumentNullException(SR.Error_BytesNull);
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
                            SR.Error_StoreInitParameters_Deserialized,
                            version,
                            SerializationVersion));
                }

                // Currently, the rest of the properties are serialized as [string, value] pairs (see ReadProperty and Write methods).
                var handle = new BlockHandle(offset: sizeof(int), size: bytes.Length - sizeof(int));
                var initializationParameters = Read<StoreInitializationParameters>(reader, handle);

                initializationParameters.Version = version;
                return initializationParameters;
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