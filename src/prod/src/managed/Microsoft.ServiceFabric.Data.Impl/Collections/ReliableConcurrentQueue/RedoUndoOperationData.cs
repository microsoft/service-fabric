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

    internal class RedoUndoOperationData : IOperationData
    {
        private const int SerializedMetadataSize = sizeof(int); // value segment count

        public ArraySegment<byte> ValueBytes { get; private set; }

        public RedoUndoOperationData(ArraySegment<byte> valueBytes)
        {
            if (valueBytes.Count == 0)
            {
                throw new ArgumentException("Unexpected zero-length valueBytes segment.");
            }

            this.ValueBytes = valueBytes;
        }

        public RedoUndoOperationData(OperationData operationData)
        {
            // Check arguments.
            if (operationData == null)
            {
                throw new ArgumentNullException("operationData");
            }

            // OperationData must contain at least the metadata.
            if (operationData.Count == 0)
            {
                throw new InvalidDataException("operationData");
            }

            // Array segment zero contains the metadata
            var metadataBytes = operationData[0];
            if (metadataBytes.Count == 0)
            {
                throw new InvalidDataException("Empty operationData data segment.");
            }

            if (metadataBytes.Count < SerializedMetadataSize)
            {
                throw new InvalidDataException(string.Format("Unexpected metadata length: {0}", metadataBytes.Count));
            }

            var valueSegmentCount = BitConverter.ToInt32(metadataBytes.Array, metadataBytes.Offset);
            var expectedSegmentCount = 2;
            if (operationData.Count != expectedSegmentCount)
            {
                throw new InvalidDataException(
                    string.Format(
                        "OperationData contained {0} array segments, but expected to find {1} array segments.", 
                        operationData.Count, 
                        expectedSegmentCount));
            }

            var valueBytes = new ArraySegment<byte>();
            if (valueSegmentCount > 0)
            {
                if (valueSegmentCount != 1)
                {
                    throw new InvalidDataException(string.Format("Unexpected (!=1) valueSegmentCount: {0}", valueSegmentCount));
                }

                valueBytes = operationData[1];
            }

            this.ValueBytes = valueBytes;
        }

        /// <summary>
        /// Returns an enumerator that iterates through the collection.
        /// </summary>
        /// <returns>
        /// A <see cref="T:System.Collections.Generic.IEnumerator`1"/> that can be used to iterate through the collection.
        /// </returns>
        public IEnumerator<ArraySegment<byte>> GetEnumerator()
        {
            if (this.ValueBytes.Count > 0)
            {
                yield return new ArraySegment<byte>(BitConverter.GetBytes(1));
                yield return this.ValueBytes;
            }
            else
            {
                yield return new ArraySegment<byte>(BitConverter.GetBytes(0));
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