// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections.ReliableConcurrentQueue
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Fabric;
    using System.IO;
    using System.Text;

    internal class MetadataOperationData : IOperationData
    {
        public const int SerializedMetadataSize = sizeof(short) + // version
                                                  sizeof(byte) + // operation type
                                                  sizeof(long); // item id

        public OperationType OperationType { get; private set; }

        public short Version { get; private set; }

        public long ItemId { get; private set; }

        public MetadataOperationData(short version, OperationType operationType, long itemId)
        {
            this.Version = version;
            this.OperationType = operationType;
            this.ItemId = itemId;
        }

        public MetadataOperationData(OperationData operationData)
        {
            if (operationData == null)
            {
                throw new ArgumentNullException("operationData");
            }

            if (operationData.Count != 1)
            {
                throw new InvalidDataException(string.Format("Unexpected operationData count: {0}", operationData.Count));
            }

            ArraySegment<byte> dataSegment;
            using (var enumerator = operationData.GetEnumerator())
            {
                bool res = enumerator.MoveNext();
                if (!res)
                {
                    throw new InvalidDataException("operationData enumeration is empty");
                }

                dataSegment = enumerator.Current;
            }

            if (dataSegment.Count <= 0)
            {
                throw new InvalidDataException("Empty operationData data segment.");
            }

            // Array segment zero contains the metadata
            var metadataBytes = operationData[0];
            if (metadataBytes.Count < SerializedMetadataSize)
            {
                throw new InvalidDataException(string.Format("Unexpected metadata length: {0}", metadataBytes.Count));
            }

            using (var stream = new MemoryStream(metadataBytes.Array, metadataBytes.Offset, metadataBytes.Count))
            using (var reader = new BinaryReader(stream, Encoding.UTF8))
            {
                // Read version.
                var versionRead = reader.ReadInt16();

                // Check version.
                if (versionRead != Constants.SerializedVersion)
                {
                    throw new InvalidDataException(
                        string.Format("Version mismatch.  Expected version {0}, but read version {1}", Constants.SerializedVersion, versionRead));
                }

                // Read operation type.
                var operationType = (OperationType)reader.ReadByte();
                if (operationType == OperationType.Invalid)
                {
                    throw new InvalidDataException("Invalid operationType.");
                }

                // Read item id.
                var itemId = reader.ReadInt64();

                this.Version = versionRead;
                this.OperationType = operationType;
                this.ItemId = itemId;
            }
        }

        /// <summary>
        /// Enumerates the buffers for this operation.
        /// </summary>
        public IEnumerator<ArraySegment<byte>> GetEnumerator()
        {
            using (var stream = new MemoryStream(SerializedMetadataSize))
            using (var writer = new BinaryWriter(stream, Encoding.UTF8))
            {
                writer.Write(this.Version);
                writer.Write((byte)this.OperationType);
                writer.Write(this.ItemId);

                yield return new ArraySegment<byte>(stream.GetBuffer(), 0, (int)stream.Position);
            }
        }

        /// <summary>
        /// Returns an enumerator that iterates through a collection.
        /// </summary>
        /// <returns>
        /// An <see cref="T:System.Collections.IEnumerator"/> object that can be used to iterate through the collection.
        /// </returns>
        IEnumerator IEnumerable.GetEnumerator()
        {
            return this.GetEnumerator();
        }
    }
}