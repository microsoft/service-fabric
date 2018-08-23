// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Generic;
    using System.Fabric.Data.Common;
    using System.IO;
    using System.Text;

    using Globalization;

    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// Contains redo or undo information for store modifications.
    /// </summary>
    internal class MetadataOperationData : OperationData
    {
        #region Instance Members
        /// <summary>
        /// Serialized size of the MetadataOperationData's metadata.
        /// </summary>
        private const int SerializedMetadataSize = sizeof(int) + // version
                                                   sizeof(long) + // transaction id
                                                   sizeof(byte) + // store modification type
                                                   sizeof(bool); // key segment exists

        /// <summary>
        /// Indicates the type of modification.
        /// </summary>
        private StoreModificationType modification;

        /// <summary>
        /// Key bytes. Cannot be null.
        /// </summary>
        private ArraySegment<byte> keyBytes;

        /// <summary>
        /// Transaction id that created this operation.
        /// </summary>
        private long storeTransaction;

        /// <summary>
        ///  Version of this replication operation.
        /// </summary>
        private int version;

        #endregion

        /// <summary>
        /// Proper constructor.
        /// </summary>
        /// <param name="version">Store version.</param>
        /// <param name="modification">Store modification.</param>
        /// <param name="keyBytes">Key bytes for the operation.</param>
        /// <param name="storeTransaction">Transaction id that created this operation.</param>
        public MetadataOperationData(
            int version,
            StoreModificationType modification,
            ArraySegment<byte> keyBytes,
            long storeTransaction)
        {
            this.version = version;
            this.modification = modification;
            this.keyBytes = keyBytes;
            this.storeTransaction = storeTransaction;

            this.Intialize();
        }

        #region Instance Properties

        /// <summary>
        /// Returns the modification done by the store.
        /// </summary>
        public StoreModificationType Modification
        {
            get
            {
                return this.modification;
            }
        }

        /// <summary>
        /// Key that was modified.
        /// </summary>
        public ArraySegment<byte> KeyBytes
        {
            get
            {
                return this.keyBytes;
            }
        }

        /// <summary>
        /// Store transaction that created this change.
        /// </summary>
        public long StoreTransaction
        {
            get
            {
                return this.storeTransaction;
            }
        }

        #endregion

        /// <summary>
        /// Deserializes a set of bytes into a redo/undo operation.
        /// </summary>
        /// <param name="version">Version to deserialize.</param>
        /// <param name="operationData">Bytes to deserialize.</param>
        /// <returns></returns>
        public static MetadataOperationData FromBytes(int version, OperationData operationData)
        {
            // Check arguments.
            if (operationData == null)
            {
                throw new ArgumentNullException(SR.Error_OperationData);
            }

            // OperationData must contain at least the metadata and key (values may be null).
            if (operationData.Count == 0)
            {
                throw new ArgumentException(SR.Error_OperationData);
            }

            // Array segment zero contains the metadata
            var metadataBytes = operationData[0];
            using (var stream = new MemoryStream(metadataBytes.Array, metadataBytes.Offset, metadataBytes.Count))
            using (var reader = new BinaryReader(stream, Encoding.Unicode))
            {
                // Read version.
                var versionRead = reader.ReadInt32();

                // Check version.
                if (versionRead != version)
                {
                    throw new NotSupportedException(
                        string.Format(CultureInfo.CurrentCulture, SR.Error_VersionMismatch, version, versionRead));
                }

                // Read transaction id.
                var storeTransaction = reader.ReadInt64();

                // Read modification.
                var modification = (StoreModificationType)reader.ReadByte();

                // Read how many value and new value segments there are
                var hasKeySegment = reader.ReadBoolean();
                var keySegmentCount = hasKeySegment ? 1 : 0;

                // Check the actual number of segments matches the expected number (metadata, key, and count for the value segments)
                var expectedSegmentCount = 1 + keySegmentCount;
                if (operationData.Count != expectedSegmentCount)
                {
                    throw new InvalidDataException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            SR.Error_MetadataOperationData_ExpectedSegments,
                            operationData.Count,
                            expectedSegmentCount));
                }

                // Read key.
                var keyBytes = hasKeySegment ? operationData[1] : new ArraySegment<byte>();

                return new MetadataOperationData(version, modification, keyBytes, storeTransaction);
            }
        }

        private void Intialize()
        {
            var enumerator = this.GetArraySegmentEnumerator();
            while (enumerator.MoveNext())
            {
                this.Add(enumerator.Current);
            }
        }

        /// <summary>
        /// Enumerates the buffers for this operation.
        /// </summary>
        /// <returns></returns>
        private IEnumerator<ArraySegment<byte>> GetArraySegmentEnumerator()
        {
            using (var stream = new MemoryStream(SerializedMetadataSize))
            using (var writer = new BinaryWriter(stream, Encoding.Unicode))
            {
                // Write version.
                writer.Write(this.version);

                // Write transaction id.
                writer.Write(this.storeTransaction);

                // Write modification type.
                writer.Write((byte)this.modification);

                // Write out whether we have valid key bytes.
                var hasKeyBytes = this.keyBytes.Array != null;
                writer.Write(hasKeyBytes);

#if DEBUG
                // Verify we are writing the same bytes as the initially created memory stream buffer
                Diagnostics.Assert(
                    (int)stream.Position == SerializedMetadataSize,
                    DifferentialStoreConstants.TraceType,
                    "MetadataOperationData.SerializedMetadataSize ({0}) needs to be updated.  Metadata size has changed to {1} bytes.",
                    SerializedMetadataSize, (int)stream.Position);
#endif

                // Return the metadata array segment
                yield return new ArraySegment<byte>(stream.GetBuffer(), 0, (int)stream.Position);
            }

            // Key bytes
            if (this.keyBytes.Array != null)
            {
                yield return this.keyBytes;
            }
        }
    }

    /// <summary>
    /// MetaDataOperationData that has a cached TKey to avoid deserialization on Primary ApplyAsync path.
    /// </summary>
    internal class MetadataOperationData<TKey> : MetadataOperationData
    {
        /// <summary>
        /// Cached TKey for the primary.
        /// </summary>
        public readonly TKey Key;

        /// <summary>
        /// Initializes a new instance of the MetadataOperationData class.
        /// </summary>
        public MetadataOperationData(TKey key, int version, StoreModificationType modification, ArraySegment<byte> keyBytes, long storeTransaction, string traceType)
            : base(version, modification, keyBytes, storeTransaction)
        {
            Diagnostics.Assert(
                this.Modification == StoreModificationType.Add || 
                this.Modification == StoreModificationType.Update ||
                this.Modification == StoreModificationType.Remove,
                traceType,
                "Must be add or update or remove.");
            this.Key = key;
        }
    }

    /// <summary>
    /// MetaDataOperationData that has a cached TKey and TValue to avoid deserialization on Primary ApplyAsync path.
    /// </summary>
    internal class MetadataOperationData<TKey, TValue> : MetadataOperationData<TKey>
    {
        /// <summary>
        /// Cached TValue for the primary.
        /// </summary>
        public readonly TValue Value;

        /// <summary>
        /// Initializes a new instance of the MetadataOperationData class.
        /// </summary>
        /// <param name="key">The key</param>
        /// <param name="value">The value</param>
        /// <param name="version">The version</param>
        /// <param name="modification">The type of modiciation</param>
        /// <param name="keyBytes">The key bytes</param>
        /// <param name="storeTransaction">The transaction</param>
        /// <param name="traceType">trace id</param>
        public MetadataOperationData(TKey key, TValue value, int version, StoreModificationType modification, ArraySegment<byte> keyBytes, long storeTransaction, string traceType)
            :base(key, version, modification, keyBytes, storeTransaction, traceType)
        {
            Diagnostics.Assert(this.Modification == StoreModificationType.Add || this.Modification == StoreModificationType.Update, traceType, "Must be add or update.");
            this.Value = value;
        }
    }
}